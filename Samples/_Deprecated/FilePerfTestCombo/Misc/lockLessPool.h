//--------------------------------------------------------------------------------------
// locklessPool.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

// It's required to include the proper OS header based on target platform before including this header
#include <atomic>
#include <mutex>
#include <malloc.h>
#include <cstdint>
#include <cassert>

// Provides a lockless implementation of a fixed size memory allocator
// The allocator can only return a fixed size block which is stored as an underlying block of memory
// backed by a bitfield for which blocks are available.
// If the pool is allowed to grow then there is a lock that is taken in this case. The user of the pool
// can control this functionality when constructing the pool
#pragma warning (push)
#pragma warning (disable:4324)	// extra padding
namespace ATG
{
    template <typename dataType, size_t initialSize = 100, bool autoGrow = true, bool checkNumAllocate = true>
    class LockLessMemoryPool : public std::allocator<dataType>
    {
    public:
        typedef dataType value_type;

        typedef value_type* pointer;
        typedef const value_type* const_pointer;

        typedef value_type& reference;
        typedef const value_type& const_reference;

        typedef size_t size_type;
        typedef ptrdiff_t difference_type;

        // It's actually treated as a pre allocated block of memory
        // There is a potential lock if the pool is exhausted or has too much contention and it needs to allocate from main memory
        // Thought: test under heavy contention, testing bit flags at 16 or 8 bit chunks may result in less contention
        //		might be possible to make the transition automatic
    private:

        // just use a 32 bit size for the bit mask used as alloc/free in the pool
        // this works in both 32-bit and 64-bit versions
        static const uint32_t s_ChunkBitSize = 32;
        static const uint32_t s_ChunkBitMask = s_ChunkBitSize - 1;

        // The memory pool is treated as a linked list of blocks containing sub elements of the requested size
        // The only lock on the pool is if a new block needs to be allocated
        // The lock can be removed by specifying the pool cannot grow, however it can be exhausted if the pool is not large enough
        // blocks are never freed until the pool is destroyed
        struct BlockDescription
        {
            uint8_t* m_memoryBlock = nullptr;				// array of elements in the pool
            uint8_t* m_endBlock    = nullptr;				// end address for this block, for quick checks if memory elements are in this block
            uint32_t* m_usedFlags  = nullptr;				// block of bit flags for which elements are free in the pool, 1 means that block is free
            std::atomic<size_t> m_numFree;		    // number of free elements in this block
            std::atomic<BlockDescription*> m_next;	// held in a linked list, only GrowPool can write to this field which is protected in a lock
        };

        BlockDescription* m_blocks;	                // head to the linked list of blocks in the pool
        uint32_t m_elementSize;		                // size for an element in the pool, all memory allocations are this size

        size_t m_blockSize;			                // number of elements in the block description

        size_t m_numChunks;			                // blockSize divided by the number of bits in a uint64_t or uint32_t, used for usedFlags array
        bool m_canGrow;				                // can we grow the pool, if false the pool is a fixed size

        std::atomic<size_t> m_numFree;              // The total number of free elements in the pool
        std::mutex m_growLock;                      // The only lock in this pool, used to protect when we need to allocate a new block

                                                    // create a new block for use in the linked list, does not actually add to the list
                                                    // This function is thread safe
        BlockDescription* CreateBlock()
        {
            BlockDescription* toret = new(std::nothrow) BlockDescription;
            if (!toret)
                return nullptr;
            toret->m_next = nullptr;

            // all memory for the block is 64 byte aligned, this makes it easy for user to control sub alignment based on size requested
            toret->m_memoryBlock = reinterpret_cast<uint8_t*> (_aligned_malloc(m_elementSize * m_blockSize, 64));
            toret->m_endBlock = toret->m_memoryBlock + (m_elementSize * m_blockSize);
            toret->m_usedFlags = new uint32_t[m_numChunks];
            memset(toret->m_usedFlags, 0xff, m_numChunks * 4);
            toret->m_numFree.store(m_blockSize);
            return toret;
        }

        // request made to grow the pool
        // A lock is taken in this function to modify the linked list, however the lock is only to create a new block
        // Other threads can still call alloc/free and they will be lockfree if space available at the time of the call
        bool GrowPool()
        {
            // handle case where the user wants zero size and all alloc requests to fail, do this outside the lock
            if ((m_elementSize * m_blockSize) == 0)
                return false;

            // Thought: This could be made lockfree by just letting multiple blocks created at the same time. Do a compare and exchange at the end on the linked list
            {
                std::lock_guard<std::mutex> localLock(m_growLock);
                // someone else already came in and made enough room while we were blocked
                if (m_numFree.load(std::memory_order_relaxed) > 0)
                    return true;
                BlockDescription* newBlock = CreateBlock();				// allocate and setup a new block
                if (!newBlock)
                    return false;
                if (m_blocks == nullptr)                                // initial block in the linked list
                {
                    m_blocks = newBlock;
                    m_numFree += m_blockSize;
                }
                else
                {
                    BlockDescription* curBlock = m_blocks;
                    while (curBlock)                                    // find the end of the linked list and append the block
                    {
                        if (curBlock->m_next.load() == nullptr)			// add it to the end of the list
                        {
                            curBlock->m_next.store(newBlock);          // we're the only thread that can modify the list, so this is safe with an interlock
                            m_numFree.fetch_add(m_blockSize);           // free blocks are not available until new block has been inserted into the list
                            break;
                        }
                        curBlock = curBlock->m_next;					// we'll catch the end of the list above on insertion
                    }
                }
            }
            return true;
        }

        // helper function to allocate an element in the pool
        // this function can fail if another threads gets in here before we do
        // really here to help in code readability
        __forceinline bool AllocElement(uint32_t& bitToUse, BlockDescription* curBlock, uint32_t chunkIndex)
        {
            // find first free bit, represents free element
            if (_BitScanForward(reinterpret_cast<DWORD*> (&bitToUse), curBlock->m_usedFlags[chunkIndex]))
            {
                // try and set bit, if we did then we found a good location and we've marked it
                if (_interlockedbittestandreset(reinterpret_cast<LONG*> (&(curBlock->m_usedFlags[chunkIndex])), static_cast<LONG> (bitToUse)) == 1)
                {
                    return true;
                }
            }
            // we failed to allocate an element, either nothing available or failed to grab it before another thread did
            return false;
        }

    public:

        LockLessMemoryPool(const LockLessMemoryPool&) = delete;
        LockLessMemoryPool(LockLessMemoryPool&& rhs) = delete;  // can't really do this since we don't lock
        LockLessMemoryPool& operator= (const LockLessMemoryPool&) = delete;
        LockLessMemoryPool& operator= (const LockLessMemoryPool&&) = delete;

        // It is permitted for elementSize and initialSize to be zero, it's an odd case but it means the user wants all allocation to fail
        LockLessMemoryPool(size_t startElementCount = initialSize, bool grow = autoGrow)
        {
            m_canGrow = grow;
            m_blockSize = startElementCount;
            m_blockSize += s_ChunkBitMask;								// round up to nearest 32
            m_blockSize &= ~s_ChunkBitMask;                              // we have a multiple of 32 available entries in free list
            m_numFree.store(0);
            m_elementSize = sizeof(dataType);
            m_numChunks = m_blockSize / s_ChunkBitSize;				    // determine number of 32 elements for used flags
            m_blocks = nullptr;                                         // start with no blocks, m_canGrow is ignored on first call to GrowBlocks
        }

        ~LockLessMemoryPool()
        {
            BlockDescription* curBlock = m_blocks;
            while (curBlock)
            {
                BlockDescription* thisBlock = curBlock;
                curBlock = thisBlock->m_next;
                _aligned_free(thisBlock->m_memoryBlock);
                delete[] thisBlock->m_usedFlags;
                delete thisBlock;
            }
        }

        //void construct(pointer p, const_reference val)
        //{
        //	new((void *)p) dataType(val);
        //}

        __declspec (allocator) pointer allocate(size_type numObjects = 1)
        {
            pointer toret;

            toret = allocate_nothrow(numObjects);
            if (!toret)
            {
                throw std::bad_alloc();
            }
            return toret;
        }
        // request an allocation from the list, size of the allocation will be the initial element size setup in the constructor
        // this function is lockless as long as memory is available in the pool
        __declspec (allocator) pointer allocate_nothrow(size_type numObjects = 1) noexcept
        {
            if (checkNumAllocate)
            {
                //assert(numObjects == 1);
                if (numObjects != 1)
                    return nullptr;
            }
            //UNREFERENCED_PARAMETER(numObjects);
            for (;;)
            {
                while (m_numFree.load() == 0)		        // not enough free so just grow the pool if we're able
                {
                    if (m_canGrow || (m_blocks == nullptr))   // we always allow initial growth of the pool even if grow pool is false
                    {
                        if (!GrowPool())
                        {
                            return nullptr;
                        }
                    }
                    else                                    // the user has requested the pool never grow
                    {
                        return nullptr;
                    }
                }

                BlockDescription* curBlock = m_blocks;      // scan through the blocks looking for a free element
                while (curBlock)                            // Thought: Think about starting at the last place we looked for perf reasons,
                {
                    // perform a quick check just to see if anything is available in this block
                    if (curBlock->m_numFree.load(std::memory_order_relaxed) != 0)
                    {
                        for (uint32_t i = 0; i < m_numChunks; i++)
                        {
                            uint32_t bitToUse;
                            if (AllocElement(bitToUse, curBlock, i))     // use the helper function to try and allocate an element in this chunk
                            {                                           // helper function is to make code easier to read, function will be inlined
                                --m_numFree;                            // we've grabbed the element, go ahead and setup data to return to caller
                                --curBlock->m_numFree;
                                uint64_t blockToUse = (i * s_ChunkBitSize + bitToUse);
                                return new(curBlock->m_memoryBlock + (blockToUse * m_elementSize)) dataType();
                            }
                        }
                    }
                    curBlock = curBlock->m_next;
                }
            }
            // Can never hit this point of the function, all paths above return
        }

        // free a pointer original allocated in this heap
        // The function will handle a pointer not allocated in the heap by just calling delete on it
        void deallocate(pointer ptrToTemplateType, size_type numObjects = 1) noexcept
        {
            assert(numObjects == 1);
            ((void)numObjects);
            if (!ptrToTemplateType)
                return;
            ptrToTemplateType->~dataType();
            uint8_t* ptrToData = reinterpret_cast<uint8_t*> (ptrToTemplateType);

            BlockDescription* curBlock = m_blocks;
            while (curBlock)
            {
                if ((ptrToData >= curBlock->m_memoryBlock) && (ptrToData <= curBlock->m_endBlock))          // scan through blocks looking for the one that owns this pointer
                {
                    // don't need to test return value, we're the only one that can set it, and thus the only one that can free it
                    // technical this could return failure in the case of a double free or invalid pointer free
                    // the heap is still consistent, no other elements will be affected
                    int32_t bitIndex = static_cast<int32_t> (ptrToData - curBlock->m_memoryBlock);		// determine which chunk and which bit in the chunk this block is
                    bitIndex /= m_elementSize;
                    int32_t arrayIndex = bitIndex >> 5;
                    bitIndex = bitIndex & 0x01f;
                    // This Interlock should never fail, data should only be released by the single owner
                    _interlockedbittestandset(reinterpret_cast<LONG*> (&(curBlock->m_usedFlags[arrayIndex])), bitIndex);
                    ++m_numFree;
                    ++curBlock->m_numFree;
                    return;
                }
                curBlock = curBlock->m_next;
            }
            assert(false);  // The caller gave us a bogus pointer, just ignore it, but assert in builds asserts are enabled.
        }

        //pointer address(reference _Val) const noexcept
        //{	// return address of mutable _Val
        //	return (_STD addressof(_Val));
        //}

        //const_pointer address(const_reference _Val) const noexcept
        //{	// return address of nonmutable _Val
        //	return (_STD addressof(_Val));
        //}

        size_t max_size() const noexcept
        {
            return 1;
        }
    };
}
#pragma warning (pop)
