//--------------------------------------------------------------------------------------
// Buffer.h
//
// Buffer wrapper classes
//
// Xbox Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------------

#pragma once
#include <xmem.h>

namespace ATG
{
   /**
    *	The base class for a simple buffer wrapper, which knows how to destroy itself, and carries its base address and
    * size information with it.
    */
   class Buffer
   {
   protected:
      void* data;         //< A pointer to the start of the region of memory.
      size_t size;        //< The size (in bytes) of the region of memory.

   public:
      /**
       * Constructor. Creates a new empty (invalid) buffer.
       */
      Buffer() noexcept
         : data( nullptr ), size( 0 )
      {
      }

      /**
       *	Deleted copy constructor. Implicit copying is not allowed.
       */
      Buffer( const Buffer& copyFrom ) = delete;

      /**
       *	Deleted copy-assignment operator. Implicit copying is not allowed.
       */
      Buffer& operator=( const Buffer& copyFrom ) = delete;

      /**
       *	Move constructor.
       * 
       * \param Buffer&& moveFrom - the Buffer object to move from.
       */
      Buffer( Buffer&& moveFrom ) noexcept
      {
         data = moveFrom.data;
         size = moveFrom.size;
         moveFrom.data = nullptr;
         moveFrom.size = 0;
      }

      /**
       *	Move assignment operator.
       *
       * \param Buffer&& moveFrom - the Buffer object to move-assign from.
       */
      Buffer& operator=( Buffer&& moveFrom ) noexcept
      {
         if ( &moveFrom != this )
         {
            data = moveFrom.data;
            size = moveFrom.size;
            moveFrom.data = nullptr;
            moveFrom.size = 0;
         }
         return *this;
      }

      /**
       *	Virtual destructor - free's the memory used by the buffer.
       */
      virtual ~Buffer() noexcept
      {
         Free();
      }

      /**
       *	Returns true if the buffer points to a valid region of memory, false if the buffer is invalid (or empty).
       */
      bool IsValid() const noexcept
      {
         return data && ( size > 0 );
      }

      /**
       *	Returns true if the buffer is valid. \see Buffer::IsValid
       */
      operator bool() const noexcept
      {
         return IsValid();
      }

      /**
       *	Cast to void* operator, returning a pointer to the start of the buffer.
       */
      operator void*( ) noexcept
      {
         return data;
      }

      /**
       * Obtains a pointer to the start of the buffer.
       * 
       * \return A void* pointer to the start of the buffer.
       */
      void* GetBuffer() noexcept
      {
         return data;
      }

      /**
       * Returns the size of the buffer in bytes.
       * 
       * \return the size of the buffer in bytes as a size_t.
       */
      size_t GetSize() const noexcept
      {
         return size;
      }

      /**
       *	Virtual Free function to be overridden in child classes to release the memory.
       */
      virtual void Free() noexcept {}
   };

   /**
    *	A buffer which is allocated and destroy using HeapAlloc and HeapFree.
    *
    * Where possible you should prefer to store objects of this type instead of using the base-type Buffer. Using the
    * concrete final type allows the compiler to optimize virtual calls.
    */
   class HeapBuffer final : public Buffer
   {
   public:

      /**
       * Creates an empty (or invalid) HeapBuffer object.
       */
      HeapBuffer() noexcept : Buffer() {}

      /**
       *	Destroys the HeapBuffer object.
       */
      ~HeapBuffer() noexcept override
      {
      }
      
      /**
       *	Deleted copy constructor. Implicit copying is not allowed.
       */
      HeapBuffer( const HeapBuffer& copyFrom ) = delete;

      /**
       *	Deleted copy-assignment operator. Implicit copying is not allowed.
       */
      HeapBuffer& operator=( const HeapBuffer& copyFrom ) = delete;

      /**
       *	Move constructor.
       * 
       * \param HeapBuffer&& moveFrom - the object to move-construct from.
       */
      HeapBuffer( HeapBuffer&& moveFrom ) noexcept
         : Buffer( std::move(moveFrom) )
      {
      }

      /**
       *	Move-assignment operator.
       *
       * \param HeapBuffer&& moveFrom - the object to move-assign from.
       * \return ATG::HeapBuffer& - the newly assigned-to object.
       */
      HeapBuffer& operator=( HeapBuffer&& moveFrom ) noexcept
      {
         if ( this != &moveFrom )
         {
            Buffer::operator=( std::move( moveFrom ) );
         }
         return *this;
      }
     
      /**
       * If the buffer is valid, frees any memory allocated for it, and then marks the buffer as empty.
       */
      void Free() noexcept override
      {
         if ( data )
         {
            if ( !::HeapFree( GetProcessHeap(), 0, data ) )
            {
               Log::WriteAndDisplay( "ERROR: Win32 error (%u) occurred while freeing buffer\n", GetLastError() );
            }
            data = nullptr;
            size = 0;
         }
      }
      
      /**
       * Allocates a new HeapBuffer object. If the allocation fails, the returned buffer will not be valid. 
       * ( \see Buffer::IsValid ).
       * 
       * \param size_t sizeBytes - the size of the buffer to allocate in bytes.
       * \param HANDLE win32Heap - the Win32 heap to use to perform the allocation, or nullptr to use the default 
       *                           process heap.
       * \return ATG::HeapBuffer - a buffer object which holds information about the allocation.
       */
      static HeapBuffer Allocate( size_t sizeBytes, HANDLE win32Heap = nullptr );
   };

   /**
    *	A buffer which allocates whole pages as its backing store, using XMemVirtualAlloc.
    *
    * To use, use with the Traits classes - for example, Page64KBBufferTraits - to define the page size to allocate.
    */
   template< typename PageBufferTraits >
   class PageBuffer final : public Buffer
   {
   public:
      using Traits = PageBufferTraits;

      /**
       * Creates an empty (or invalid) PageBuffer object.
       */
      PageBuffer() {}

      /**
       *	Destroys the PageBuffer object.
       */
      virtual ~PageBuffer() {}

      /**
       *	Deleted copy constructor. Implicit copying is not allowed.
       */
      PageBuffer( const PageBuffer& copyFrom ) = delete;
      
      /**
       *	Deleted copy-assignment operator. Implicit copying is not allowed.
       */
      PageBuffer& operator=( const PageBuffer& copyFrom ) = delete;

      /**
       *	Move constructor.
       *
       * \param PageBuffer&& moveFrom - the object to move-construct from.
       */
      PageBuffer( PageBuffer&& moveFrom )
         : Buffer( std::move( moveFrom ) )
      {
      }

      /**
       *	Move-assignment operator.
       *
       * \param HeapBuffer&& moveFrom - the object to move-assign from.
       * \return ATG::HeapBuffer& - the newly assigned-to object.
       */
      PageBuffer& operator=( PageBuffer&& moveFrom )
      {
         if ( this != &moveFrom )
         {
            Buffer::operator=( std::move( moveFrom ) );
         }
         return *this;
      }

      /**
       * If the buffer is valid, frees any memory allocated for it, and then marks the buffer as empty.
       */
      void Free() noexcept override
      {
         if ( data )
         {
            ::VirtualFree( data, size, MEM_RELEASE );
            data = nullptr;
            size = 0;
         }
      }

      /**
       * Allocates a new HeapBuffer object. If the allocation fails, the returned buffer will not be valid.
       * ( \see Buffer::IsValid ).
       * 
       * \param size_t sizeBytes - the size of the buffer to allocate (and track).
       * \param DWORD pageTypeFlag - the page type flags to use for the allocation.
       * \param DWORD pageProtection - the page protection flags to use for the allocation.
       * \return ATG::PageBuffer - the buffer tracking object.
       */
      static PageBuffer Allocate( size_t sizeBytes, DWORD pageTypeFlag = XMEM_CPU, DWORD pageProtection = PAGE_READWRITE )
      {
         PageBuffer<PageBufferTraits> pb;

         pb.data = ::XMemVirtualAlloc( nullptr, sizeBytes, MEM_COMMIT | PageBufferTraits::PageSizeFlag,
                                       pageTypeFlag, pageProtection );

         assert( pb.data && "Memory allocation failed." );

         if ( !pb.data )
         {
            Log::WriteAndDisplay( "ERROR: Win32 error (%u) occurred while allocating buffer\n", GetLastError() );
         }
         else
         {
            pb.size = sizeBytes;
         }

         return pb;
      }
   };

   /**
    *	Traits class for 4KiB pages.
    */
   struct Page4KBBufferTraits
   {
      static constexpr size_t PageSize = 4 * 1024;
      static constexpr DWORD PageSizeFlag = 0;
   };

   /**
    *	Traits class for 64KiB pages.
    */
   struct Page64KBBufferTraits
   {
      static constexpr size_t PageSize = 64 * 1024;
      static constexpr DWORD PageSizeFlag = MEM_64K_PAGES;
   };

   /**
    *	Traits class for 2MiB pages.
    */
   struct Page2MBBufferTraits
   {
      static constexpr size_t PageSize = 2 * 1024 * 1024;
      static constexpr DWORD PageSizeFlag = MEM_2MB_PAGES;
   };

   /**
    *	A PageBuffer object that uses 4KiB pages for its allocations.
    */
   using PageBuffer4KB = PageBuffer<Page4KBBufferTraits>;

   /**
    *	A PageBuffer object that uses 64KiB pages for its allocations.
    */
   using PageBuffer64KB = PageBuffer<Page64KBBufferTraits>;

   /**
    *	A PageBuffer object that uses 2MiB pages for its allocations.
    */
   using PageBuffer2MB = PageBuffer<Page2MBBufferTraits>;
}
