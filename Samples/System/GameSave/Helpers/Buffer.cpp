//--------------------------------------------------------------------------------------
// Buffer.cpp
//
// Buffer implementation
//
// Xbox Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Buffer.h"

namespace ATG
{
   /**
    * Allocates a new HeapBuffer object. If the allocation fails, the returned buffer will not be valid.
    * ( \see Buffer::IsValid ).
    *
    * \param size_t sizeBytes - the size of the buffer to allocate in bytes.
    * \param HANDLE win32Heap - the Win32 heap to use to perform the allocation, or nullptr to use the default 
    *                           process heap.
    * \return ATG::HeapBuffer - a buffer object which holds information about the allocation.
    */
   ATG::HeapBuffer HeapBuffer::Allocate( size_t sizeBytes, HANDLE win32Heap )
   {
      HeapBuffer buffer;

      if ( win32Heap == nullptr )
      {
         win32Heap = ::GetProcessHeap();
      }
     
      buffer.data = ::HeapAlloc( win32Heap, 0, sizeBytes );

      assert( buffer.data && "Memory allocation failed." );

      if ( buffer.data )
      {
         buffer.size = sizeBytes;
      }
      else
      {
         Log::WriteAndDisplay( "ERROR: Win32 error (%u) occurred while allocating buffer\n", GetLastError() );
      }

      return buffer;
   }
}
