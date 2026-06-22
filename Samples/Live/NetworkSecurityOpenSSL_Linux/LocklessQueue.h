// Copyright (c) Microsoft Corporation
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

/*****************************************************************************
 
 LocklessQueue is a lock free queue that can take any type as a payload.
 Internally it maintains a demand-allocated block of contiguous nodes that
 are used as the nodes of a linked list.  Data pushed into the queue uses a
 copy, and popping data copies that same data out to your variable.
 
 An initial block size is created up front and the size of the block can be customized
 via the constructor.  If the queue runs out of space, it can create new blocks.  This process
 is also lock / wait free, although it has an increassed "push" cost as the new block
 is allocated and initialized. If there is not enough memory to create a new block
 push_back returns false.
 
 N.B. LocklessQueue MUST be either dynamically allocated or stack allocated as a
 local variable.  It cannot be used as a member variable in a class or structure
 due to alignment restrictions. It can fail spectacularly if not aligned correctly.
 
 Typical Usage:
 
 struct MyPayload
 {
    uint32_t someData;
    // etc...
 };
 
 LocklessQueue<MyPayload*> queue;
 
 MyPayload p = new MyPayload;
 p->someData = 16;
 queue.push_back(p);
 
 if (queue.pop_front(p))
 {
    printf("Value: %d\r\n", p->someData);
    delete p;
 }
 
 
 LocklessQueue is based on the the algorithm described in this paper:

 Simple, Fast, and Practical Non-Blocking and Blocking Concurrent Queue Algorithms

 Maged M. Michael Michael L. Scott
 Department of Computer Science University of Rochester Rochester, NY 14627-0226 fmichael,scottg@cs.rochester.ed

 http://cs.rochester.edu/~scott/papers/1996_PODC_queues.pdf

 In addition to the above algorithm this variant introduces a heap of nodes.  This helps keep the memory
 contiguous and allows for single 64 bit swaps on 64 bit machines (as we don't need to store pointers).

 ******************************************************************************/

// LocklessQueue needs certain alignment.  Disable alignment warning.
#pragma warning(push)
#pragma warning(disable: 4324)

template <typename TData>
class alignas(8) LocklessQueue
{
public:
    
    static void* operator new(_In_ size_t sz)
    {
        void* ptr = aligned_malloc(sz, 8);
        if (ptr == nullptr)
        {
            throw new std::bad_alloc;
        }
        return ptr;
    }
    
    static void* operator new(_In_ size_t sz, _In_ const std::nothrow_t&)
    {
        return aligned_malloc(sz, 8);
    }
    
    static void operator delete(_In_ void* ptr)
    {
        aligned_free(ptr);
    }

    //
    // Creates a new lockless queue.  The blockSize parameter indicates how many
    // node elements are contained in a block.  Additional blocks will be allocated
    // as needed but for best performance use a block size that does not require
    // additional allocation for your work load. Passing zero will use the 
    // minimum size.
    //
    LocklessQueue(_In_ uint32_t blockSize = 0x400) noexcept :
        m_localHeap(*this),
        m_heap(m_localHeap),
        m_activeList(*this),
        m_blockCache(nullptr)
    {
        m_localHeap.init(blockSize);
        Initialize();
    }

    //
    // Creates a new lockless queue.  The shared parameter provides another
    // lockless queue that provides the heap for nodes. Queues that
    // share the same node heap can share addresses. When a lockless queue
    // that is sharing another queue's nodes is destroyed, any outstanding
    // nodes in use are given back to the heap of the shared queue.  Destroying
    // the host queue before destroying queues sharing its heap results in 
    // undefined behavior.
    //
    LocklessQueue(_In_ LocklessQueue& shared) noexcept :
        m_localHeap(*this),
        m_heap(shared.m_heap),
        m_activeList(*this),
        m_blockCache(nullptr)
    {
        Initialize();
    }

    ~LocklessQueue() noexcept
    {
        if (&m_heap != &m_localHeap)
        {
            // This queue is sharing a heap, so
            // we need to put all outstanding nodes
            // back
            TData data;
            while(pop_front(data)) {}

            // And the dummy node too.
            Address dummy = m_activeList.head();
            Node* node = to_node(dummy);
            m_heap.free(node, dummy);
        }
    }

    //
    // Returns true if the queue is empty. Note that 
    // since there are no locks this is a snapshot in time.
    //
    bool empty() noexcept
    {
        return m_activeList.empty();
    }

    //
    // Reserves a link node in the queue, allocating it if
    // necessary.  The node must be used in a later push_back
    // or free_node call or it will be leaked.
    //
    bool reserve_node(_Out_ uint64_t& address) noexcept
    {
        Address a;
        Node* n = m_heap.alloc(a);

        if (n != nullptr)
        {
            address = a;
            return true;
        }
        else
        {
            address = 0;
            return false;
        }
    }

    //
    // Frees a previously reserved node.  Freeing a node
    // that has been pushed results in undefined behavior. It
    // will be hard to debug.
    //
    void free_node(_In_ uint64_t address) noexcept
    {
        Address a;
        a = address;
        Node* n = to_node(a);
        m_heap.free(n, a);
    }

    //
    // Pushes the given data onto the back
    // of the queue.  A copy is made of TData
    // (which may throw, if TData's copy constructor can
    // throw exceptions).
    // If the push fails this returns false.
    //
    bool push_back(_In_ const TData& data)
    {
        TData copy = data;
        return move_back(std::move(copy));
    }

    //
    // Pushes the given data onto the back
    // of the queue.
    //
    bool push_back(_In_ TData&& data) noexcept
    {
        return move_back(std::move(data));
    }

    //
    // Pushes the given data onto the back of the queue,
    // using a reserved node pointer.  This never fails as
    // the node pointer was preallocated. A copy of TData is
    // made (which may throw, if TData's copy constructor can
    // throw exceptions).
    //
    void push_back(_In_ const TData& data, _In_ uint64_t address)
    {
        TData copy = data;
        move_back(std::move(copy), address);
    }

    //
    // Pushes the given data onto the back of the queue,
    // using a reserved node pointer.  This never fails as
    // the node pointer was preallocated.
    //
    void push_back(_In_ TData&& data, _In_ uint64_t address) noexcept
    {
        move_back(std::move(data), address);
    }

    //
    // Pops TData off the head of the queue, returning
    // true if successful or false if the queue is
    // empty.
    //
    bool pop_front(_Out_ TData& data) noexcept
    {
        Address address;
        Node* node = m_activeList.pop(address);
        
        if (node != nullptr)
        {
            data = std::move(node->data);
            node->data = TData {};
            m_heap.free(node, address);
            return true;
        }
        
        return false;
    }

    //
    // Pops TData off the front of the queue, returning
    // true if successful, and returns a node address
    // instead of placing the address back on the free
    // list. This address can later be used in a push_back
    // call or a free_node call.  Failing to call either of these
    // results in a leaked node. Returns false if the queue
    // is empty.
    //
    bool pop_front(_Out_ TData& data, _Out_ uint64_t& address) noexcept
    {
        Address a;
        Node* node = m_activeList.pop(a);
        
        if (node != nullptr)
        {
            data = std::move(node->data);
            node->data = TData {};
            address = a;
            return true;
        }
        
        return false;
    }

    //
    // Removes items from the queue that satisfy the given callback.  The callback
    // is of the form:
    //
    //      bool callback(TData& data, uint64_t address);
    //
    // If the callback returns true, it is taking ownership of the data and the
    // address. If it returns false, the node is placed back on the queue.
    //
    // This is a lock-free call: if there are interleaving calls to push_back
    // while this action is in progress final node order is not guaranteed (nodes
    // this API processes may be interleaved with newly pushed nodes).
    //
    template <typename TCallback>
    void remove_if(TCallback callback)
    {
        LocklessQueue<TData> retain(*this);
        TData entry;
        uint64_t address;

        while (pop_front(entry, address))
        {
            if (!callback(entry, address))
            {
                retain.move_back(std::move(entry), address);
            }
        }

        while (retain.pop_front(entry, address))
        {
            move_back(std::move(entry), address);
        }
    }

private:

    /*
     *
     * Structure Definitions
     *
     */

    // Address - This represents the address of a node.
    // Nodes live in a contiguous memory block, and there are multiple
    // blocks. Address represents the position of the
    // node and must be 64 bits.

    struct Address
    {
        uint64_t index : 32;
        uint64_t block : 16;
        uint64_t aba   : 16;
        
        inline bool operator == (_In_ const Address& other) const
        {
            uint64_t v = *this;
            uint64_t ov = other;
            return v == ov;
        }

        inline bool operator != (_In_ const Address& other) const
        {
            uint64_t v = *this;
            uint64_t ov = other;
            return v != ov;
        }

        inline operator uint64_t () const
        {
            uint64_t v;

            // Note: this looks horribly inefficient.  General consensus
            // is this is the best way of doing type punning in a c++ compliant
            // way, and disassembly of this code shows it amounts to the following:
            //
            //      mov	rax, QWORD PTR [rdx]
            //
            // So, no real call out to memcpy for retail.

            memcpy(&v, this, sizeof(v));
            return v;
        }

        inline Address& operator = (_In_ uint64_t v)
        {
            memcpy(this, &v, sizeof(v));
            return *this;
        }
    };

    static_assert(sizeof(Address) == sizeof(uint64_t), "LocklessQueue Address field must be 64 bits exactly");

#if _MSVC_LANG >= 201703L
    static_assert(std::atomic<Address>::is_always_lock_free, "LocklessQueue requires atomic<Address> to be lock free");
#endif

    // Node - each entry in a list is backed by a node,
    // which contains a single 64 bit value representing
    // the next node of a singly linked list and
    // a payload of type TData. Nodes must be properly aligned
    // in memory so std::atomic works consistently.  We do this
    // by using an aligned allocator.
    struct Node
    {
        std::atomic<Address> next;
        TData data;
    };

    // Block - Represents a contiguous block of nodes. Blocks are
    // only created.  Ideally only one block would be created for
    // a particular use case of this queue, but if the queue runs
    // out of space in a block it will create additional blocks.
    // Blocks are linked together as a singly linked list.
    // Blocks must be properly aligned in memory so std::atomic
    // works consistently.  We do this by using an aligned allocator.
    struct Block
    {
        std::atomic<Block*> next;
        Node* nodes;
        uint32_t id;
        uint32_t padding;
    };

    // List - this is a fully lock free linked list with push and pop
    // operations.  Nodes are provided externally.
    class List
    {
    public:

        List(_In_ LocklessQueue& owner) :
            m_owner(owner)
        {}

        void init(_In_ Address dummy, _In_ Address end) noexcept
        {
            m_head = dummy;
            m_tail = dummy;
            m_end = end;
        }

        inline Address end() { return m_end; }
        inline Address head() { return m_head.load(); }

        // Returns true if the queue is empty. Note that 
        // since there are no locks this is a snapshot in time.
        bool empty() noexcept
        {
            Address head = m_head.load();
            Address tail = m_tail.load();
            Address next = m_owner.to_node(head)->next;
            
            if (head == m_head.load() &&
                head == tail &&
                next == m_end)
            {
                return true;
            }
            
            return false;
        }

        // Push a new node onto the tail of the list.  The address
        // is the address of the node.  The node next pointer is initialized
        // and the address aba is incremented.
        inline void push(_In_ Node* node, _In_ Address address) noexcept
        {
            node->next = m_end;
            address.aba++;
            push_range(address, address);
        }

        // Push a range of nodes into the tail of the list.  The tailAddress
        // is the last node to push and the beginAddress is the first.  The
        // nodes all need to be pre-confgured to follow each other.
        void push_range(_In_ Address beginAddress, _In_ Address tailAddress) noexcept
        {
            while(true)
            {
                Address tail = m_tail.load();
                Node* tailNode = m_owner.to_node(tail);
                Address tailNext = tailNode->next.load();
                
                if (tail == m_tail.load())
                {
                    if (tailNext == m_end)
                    {
                        // The next of the tail points to an invalid node, so
                        // this really is the tail.  Fix up the next pointer to
                        // point to our new node.  If this succeeds we try to
                        // adjust the tail, which isn't guaranteed to succeed.
                        // That's OK, we can fix it up later.

                        if (tailNode->next.compare_exchange_strong(tailNext, beginAddress))
                        {
                            m_tail.compare_exchange_strong(tail, tailAddress);
                            break;
                        }
                    }
                    else
                    {
                        // What we thought was the tail is really not pointing to the
                        // end, so advance down the list.

                        m_tail.compare_exchange_strong(tail, tailNext);
                    }
                }
            }
        }

        // Pop a node from the head of the list.  There is always a dummy
        // node at the head of the list, so part of this process shifts
        // data from head->next out to a temporary variable and then
        // puts the data back in head once it is detached. Returns nullptr
        // if the list is empty.
        Node* pop(_Out_ Address& address) noexcept
        {
            while (true)
            {
                Address head = m_head.load();
                Address tail = m_tail.load();
                Node* headNode = m_owner.to_node(head);
                Address next = headNode->next.load();

                if (head == m_head.load())
                {
                    if (head == tail)
                    {
                        if (next == m_end)
                        {
                            // List is empty
                            address = m_end;
                            return nullptr;
                        }

                        // List is not empty, but is out of
                        // sync. Advance the tail.

                        m_tail.compare_exchange_strong(tail, next);
                    }
                    else
                    {
                        // This is possibly the node we want.  We are going to
                        // shift the head node out of the list, but the list
                        // actually uses a dummy head node, so we must copy the
                        // data out of the next node, save it off, and then
                        // put it into the head node once we safely detach it.

                        TData data = m_owner.to_node(next)->data;

                        if (m_head.compare_exchange_strong(head, next))
                        {
                            headNode->data = std::move(data);
                            address = head;
                            return headNode;
                        }
                    }
                }
            }
        }

    private:

        LocklessQueue& m_owner;
        std::atomic<Address> m_head;
        std::atomic<Address> m_tail;
        Address m_end;
    };

    // Heap - the cache of available nodes. This heap may be shared
    // with other LocklessQueue instances in cases where you need to move
    // items between lists and want to share the nodes.  If you don't
    // also share the heap, sharing nodes will corrupt the queue.
    class Heap
    {
    public:
        Heap(_In_ LocklessQueue& owner) :
            m_freeList(owner)
        {}

        ~Heap()
        {
            Block* block = m_blockList;
            while(block != nullptr)
            {
                Block* d = block;
                block = block->next;

                for (uint32_t idx = 0; idx < m_blockSize; idx++)
                {
                    d->nodes[idx].~Node();
                }

                aligned_free(d);
            }
        }

        void init(_In_ uint32_t blockSize) noexcept
        {
            if (blockSize < 0x40)
            {
                blockSize = 0x40;
            }

            m_blockSize = blockSize;

            while(!allocate_block() && m_blockSize > 0x40)
            {
                m_blockSize = m_blockSize >> 2;
            }
        }

        inline Address end() { return m_freeList.end(); }

        // Returns the node at the given address, caching the block it
        // was in.
        Node* to_node(_Inout_ std::atomic<Block*>& blockCache, _In_ const Address& address)
        {
            Block* block = blockCache.load();

            if (block == nullptr || block->id != address.block)
            {
                for(block = m_blockList; block != nullptr; block = block->next)
                {
                    if (block->id == address.block)
                    {
                        blockCache = block;
                        break;
                    }
                }
            }

            return &block->nodes[address.index];
        }

        // Returns a node back to the heap
        void free(_In_ Node* node, _In_ Address address) noexcept
        {
            m_freeList.push(node, address);
        }

        // Pops a node off the heap, allocating a new
        // block if needed.
        Node* alloc(_Out_ Address& address) noexcept
        {
            Node* node;

            do
            {
                node = m_freeList.pop(address);

                // It's possible that pop_list fails right after
                // allocating a block if there is heavy free
                // list use (Ex another thread drains the free list
                // after we alocate).  So we must loop and possibly
                // allocate again.  We break out if allocation fails.

                if (node == nullptr && !allocate_block())
                {
                    break;
                }
            } while(node == nullptr);

            return node;
        }

    private:

        std::atomic<uint32_t> m_blockCount = { 0 };
        uint32_t m_blockSize = 0;
        Block* m_blockList = nullptr;
        List m_freeList;

        // Allocates a new block of m_blockSize and
        // puts all of its nodes on the free list. Returns
        // true if the allocation was successful.
        bool allocate_block() noexcept
        {
            uint32_t blockId = m_blockCount.fetch_add(1) + 1;

            // Block ID stored in Address is 16 bits, so that's our
            // max.
            if (blockId > 0xFFFF)
            {
                return false;
            }

            size_t size = sizeof(Node) * m_blockSize + sizeof(Block);
            void* mem = aligned_malloc(size, 8);

            if (mem == nullptr)
            {
                return false;
            }
            
            Block* block = new (mem) Block;

            block->id = blockId;        
            block->next = nullptr;
            block->nodes = new (block + 1) Node[m_blockSize];

            // Connect all the nodes in the new block. Element zero is
            // the "tail" of this block.
            
            Address prev = { 0 };
            for (uint32_t index = 0; index < m_blockSize; index++)
            {
                block->nodes[index].next = prev;
                prev.block = static_cast<uint16_t>(block->id);
                prev.index = index;
                prev.aba = 0;
            }

            // Now connect this block to the tail. Because we never delete
            // blocks we can safely traverse the linked list of blocks with
            // no locks or fanciness. The startIndex will be zero except when
            // we are first initializing m_blockList.  Then we steal the first
            // node to act as the free list's dummy node.

            uint32_t startIndex = 0;

            if (m_blockList == nullptr)
            {
                // Initial contruction.  We need to store the block list
                // and then initialize the free list.

                Address end = { 0 };
                Address a = { 0 };
                a.block = static_cast<uint16_t>(block->id);

                block->nodes[0].next = end;
                block->nodes[1].next = end;
                startIndex = 1;
                m_blockList = block;
                m_freeList.init(a, end);
            }
            else
            {
                Block* tail = m_blockList;
                Block* next = tail->next.load();

                while(true)
                {
                    while(next != nullptr)
                    {
                        tail = next;
                        next = tail->next.load();
                    }

                    Block* empty = nullptr;

                    if(tail->next.compare_exchange_strong(empty, block))
                    {
                        break;
                    }

                    next = tail->next.load();
                }
            }

            // Now add the tail and the head to the free list.

            Address rangeBegin { 0 };
            Address rangeEnd { 0 };

            rangeBegin.block = rangeEnd.block = static_cast<uint16_t>(block->id);
            rangeBegin.index = m_blockSize - 1;
            rangeEnd.index = startIndex;
            m_freeList.push_range(rangeBegin, rangeEnd);

            return true;
        }
    };

    /*
     *
     * Members
     *
     */
    
    Heap m_localHeap;
    Heap& m_heap;
    List m_activeList;
    std::atomic<Block*> m_blockCache;
    
    /*
     *
     * Private APIs
     * 
     */

    // Workers for pushing nodes to the back
    // of the queue.  This always moves data
    bool move_back(_In_ TData&& data) noexcept
    {
        Address address;
        Node* node = m_heap.alloc(address);

        if (node != nullptr)
        {
            node->data = std::move(data);
            m_activeList.push(node, address);
            return true;
        }

        return false;
    }

    // Workers for pushing nodes to the back
    // of the queue.  This always moves data
    void move_back(_In_ TData&& data, _In_ uint64_t address) noexcept
    {
        Address a;
        a = address;
        Node* n = to_node(a);
        n->data = std::move(data);
        m_activeList.push(n, a);
    }

    // Called during construction after the heap
    // is setup.
    void Initialize()
    {
        Address end = m_heap.end();
        end.index++;

        Address dummy;
        Node* n = m_heap.alloc(dummy);

        if (n != nullptr)
        {
            n->next = end;
        }
        else
        {
            dummy = end;
        }

        m_activeList.init(dummy, end);
    }

    Node* to_node(_In_ const Address& address)
    {
        return m_heap.to_node(m_blockCache, address);
    }
    
    static inline void* aligned_malloc(_In_ size_t size, _In_ size_t align)
    {
        void *result;
        size_t bytes = (size + align - 1) & ~(align - 1);
#ifdef _MSC_VER
        result = _aligned_malloc(bytes, align);
#else
        if(posix_memalign(&result, align, bytes)) result = 0;
#endif
        return result;
    }
    
    static inline void aligned_free(_In_ void *ptr)
    {
#ifdef _MSC_VER
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }
};

#pragma warning(pop)
