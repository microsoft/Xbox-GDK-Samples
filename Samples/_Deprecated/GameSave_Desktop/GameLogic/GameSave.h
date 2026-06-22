// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

#include "..\Helpers\Buffer.h"
#include "..\Helpers\XGameSaveHandleWrappers.h"
#include "GameSaveContainerMetadata.h"
#include <XGameSave.h>
#include <map>
#include <mutex>
#include <wrl/implements.h>
#include "RDTSCPStopWatch.h"


#ifdef _DEBUG
// set this to a value > 0 and <= 16 MB to add padding for the purposes of simulating large game save scenarios, for 
// demos and debugging (DO NOT SHIP A GAME WITH PADDING!)
constexpr uint32_t DEFAULT_MINIMUM_SAVE_SIZE = 1024 * 1024;
#else
constexpr uint32_t DEFAULT_MINIMUM_SAVE_SIZE = 0;
#endif

constexpr size_t CalculateMaxBlobReadTransferBufferSize( size_t blobCount )
{
   return ( GS_MAX_BLOB_SIZE + GS_MAX_BLOB_NAME_SIZE + sizeof( XGameSaveBlob ) ) * blobCount;
}

// The type of operation we can perform on a blob - basically write/update it, or delete it.
enum class BlobChangeType
{
   Set = 0,
   Delete
};

// Info we group together to perform operations on blobs.
struct BlobUpdate
{
   BlobChangeType changeType;
   const char* blobName;
   uint8_t* buffer;
   size_t bufferSizeBytes;
};

template <typename TData>
class GameSave final
{
public:
   GameSave( const std::string& containerName, const std::string& containerDisplayName,
      std::function<void( TData& )> saveFunction,
      uint32_t minSaveSizeInBytes = DEFAULT_MINIMUM_SAVE_SIZE )
      : OnSave( saveFunction ),
      m_containerMetadata(containerName, containerDisplayName),
      m_isGameDataDirty( false ),
      m_isGameDataLoaded( false ),
      m_minSaveSize( minSaveSizeInBytes ),
      m_currentDataBuffer( 0 ),
      m_data{}
   {
   }

   inline bool operator==( const GameSave& rhs )
   {
      return m_containerMetadata.m_containerName.compare( rhs.m_containerMetadata.m_containerName ) == 0;
   }

   inline bool operator!=( const GameSave& rhs )
   {
      return !( *this == rhs );
   }

   GameSaveContainerMetadata& GetContainerMetadata() noexcept
   {
      return m_containerMetadata;
   }

   std::function<void( TData& )> OnSave; // always run this function on the data in the front buffer during a save

   void ResetData()
   {
      std::lock_guard<std::mutex> lock( m_mutex );

      m_isGameDataDirty = false;
      m_isGameDataLoaded = false;
      m_containerMetadata.ResetData();

      TData emptyData;
      errno_t err = memcpy_s( &BackBuffer(), sizeof( TData ), &emptyData, sizeof( TData ) );
      if ( err != 0 )
      {
         Log::Write( "ERROR: GameSave::ResetData(): memcpy of game save data failed\n" );
         return;
      }

      SwapBuffers();
   }

   bool SetData( TData* newData, bool markDirtyOnSuccess = true )
   {
      std::lock_guard<std::mutex> lock( m_mutex );

      if ( newData != nullptr && newData != &FrontBuffer() )
      {
         TData* backBuffer = &BackBuffer();
         if ( newData != backBuffer )
         {
            errno_t err = memcpy_s( backBuffer, sizeof( TData ), newData, sizeof( TData ) );
            if ( err != 0 )
            {
               Log::Write( "ERROR: GameSave::SetData(): memcpy of game save data failed\n" );
               return false;
            }
         }

         SwapBuffers();
      }

      if ( markDirtyOnSuccess )
      {
         m_isGameDataDirty = true;
      }

      return true;
   }

   HRESULT Load( ATG::GameSaveContainerHandle& withContainer )
   {
      Log::Write( "GameSave::Load(%s)\n", m_containerMetadata.m_containerName.c_str() );

      HRESULT hr = ReadData( withContainer );

      bool readSuccess = SUCCEEDED( hr );

      std::lock_guard<std::mutex> lock( m_mutex );

      if ( !m_isGameDataLoaded )
      {
         m_isGameDataLoaded = readSuccess;
      }

      if ( readSuccess )
      {
         m_isGameDataDirty = false;
      }

      return hr;
   }

   HRESULT Save( ATG::GameSaveContainerHandle& withContainer, const char* containerName )
   {
      Log::Write( u8"GameSave::Save(%s)\n", containerName );

      bool wasGameDataDirty = m_isGameDataDirty; // preserve the current state of dirtiness in case the save fails
      m_isGameDataDirty = false;

      OnSave( FrontBuffer() );

      std::vector<BlobUpdate> updates;
      BlobUpdate update;
      update.changeType = BlobChangeType::Set;
      update.blobName = u8"data";
      update.buffer = reinterpret_cast<uint8_t*>( &FrontBuffer() );
      update.bufferSizeBytes = sizeof( TData );
      updates.push_back( update );

      ATG::HeapBuffer paddingBuffer;

      if ( update.bufferSizeBytes < m_minSaveSize )
      {
         // a default minimum save size was specified for this game save data (should ONLY be used for debug or demo purposes)
         // so we'll add another buffer here.
         paddingBuffer = std::move( MakePaddingBuffer( (uint32_t) update.bufferSizeBytes, true ) );

         BlobUpdate padding = {};
         padding.changeType = BlobChangeType::Set;
         padding.blobName = u8"padding";
         padding.buffer = reinterpret_cast<uint8_t*>( paddingBuffer.GetBuffer() );
         padding.bufferSizeBytes = paddingBuffer.GetSize();
         updates.push_back( padding );
      }

      HRESULT hr = SaveData( withContainer, updates );

      bool saveSuccess = SUCCEEDED( hr );

      std::lock_guard<std::mutex> lock( m_mutex );
      if ( !m_isGameDataLoaded )
      {
         m_isGameDataLoaded = saveSuccess;
      }

      if ( wasGameDataDirty && !saveSuccess )
      {
         m_isGameDataDirty = true;
      }

      return hr;

   }

   HRESULT DeleteBlobs( ATG::GameSaveContainerHandle& withContainer )
   {
      Log::Write( "GameSave::DeleteBlobs(%s)\n", m_containerMetadata.m_containerName.c_str() );

      std::vector<BlobUpdate> toDelete;
      BlobUpdate blobData = {};
      blobData.changeType = BlobChangeType::Delete;
      blobData.blobName = u8"data";
      toDelete.push_back( blobData );

      BlobUpdate blobPadding = {};
      blobPadding.changeType = BlobChangeType::Delete;
      blobPadding.blobName = u8"data";
      toDelete.push_back( blobPadding );

      HRESULT hr = SaveData( withContainer, toDelete );

      if ( SUCCEEDED( hr ) )
      {
         ResetData();
      }

      return hr;
   }

   const TData& FrontBuffer() const
   {
      return m_data[ m_currentDataBuffer ];
   }

   TData& FrontBuffer()
   {
      return m_data[ m_currentDataBuffer ];
   }

   GameSaveContainerMetadata                   m_containerMetadata;
   bool                                        m_isGameDataDirty;
   bool                                        m_isGameDataLoaded;
   uint32_t                                    m_minSaveSize; // adds padding data so that a save is this minimum size (see DEFAULT_MINIMUM_SAVE_SIZE above, for debugging only)
   mutable std::mutex                          m_mutex;
   ATG::PageBuffer4KB                          m_readTransferBuffer;

private:
   void CreateReadTransferBuffer()
   {
      if ( m_readTransferBuffer.IsValid() )
         return;

      //NOTE: You can be more clever about your GameSave buffer and use
      //      scratch space rather than holding onto it if you prefer.
      constexpr size_t MaxNumBlobsPerContainer = 16;
      constexpr size_t bufferSize = CalculateMaxBlobReadTransferBufferSize( MaxNumBlobsPerContainer );
      m_readTransferBuffer = ATG::PageBuffer4KB::Allocate( bufferSize );
   }

   void FreeReadTransferBuffer()
   {
      m_readTransferBuffer.Free();
   }

   HRESULT ReadData( _In_ XGameSaveContainerHandle withContainer )
   {
      CreateReadTransferBuffer();

      ATG::RDTSCPStopWatch stopwatch;
      stopwatch.Start();
      

      const char* blobNames[] = {
         u8"data"
      };

      XGameSaveBlob* readBuffer = reinterpret_cast<XGameSaveBlob*>( m_readTransferBuffer.GetBuffer() );

      uint32_t blobCount = _countof( blobNames );

      HRESULT hr = XGameSaveReadBlobData( withContainer, blobNames, &blobCount, m_readTransferBuffer.GetSize(),
         readBuffer );

      double durationMS = stopwatch.GetCurrentMilliseconds();
      stopwatch.Reset();

      Log::WriteAndDisplay( "XGameSaveReadBlobData duration: %lfms\n", durationMS );

      bool readSuccess = SUCCEEDED( hr );

      if ( readSuccess )
      {
         Log::Write( "XGameSaveReadBlobData task succeeded\n" );

         //CONSIDER: Do we check the final value of blobCount here?

         // You'd want to associate your blobs with the buffers they should be copied into here. We only have one
         // blob per container that we care about.
         assert( _stricmp( blobNames[ 0 ], readBuffer->info.name ) == 0 && "Name of read blob and requested blob must match" );

         memcpy_s( &BackBuffer(), sizeof( TData ), readBuffer->data, readBuffer->info.size );

         std::lock_guard<std::mutex> lock( m_mutex );
         SwapBuffers();
      }
      else
      {
         if ( hr == E_GS_BLOB_NOT_FOUND )
         {
            Log::WriteAndDisplay( "XGameSaveReadBlobData result: Blob not found (E_GS_BLOB_NOT_FOUND)\n" );
         }
         else
         {
            Log::WriteAndDisplay( "ERROR: XGameSaveReadBlobData task returned exception (%s)\n",
               FormatHResult( hr ).c_str() );
         }
      }

      return hr;
   }

   HRESULT SaveData( ATG::GameSaveContainerHandle& withContainer, const std::vector<BlobUpdate>& changes )
   {
	  ATG::RDTSCPStopWatch saveDataSW;
	  saveDataSW.Start();

      HRESULT hr;
      {
         ATG::GameSaveUpdateHandle gameSaveUpdate; // this is closed when we exit the enclosing scope.

		 ATG::RDTSCPStopWatch createUpdateSW;
		 createUpdateSW.Start();

         hr = XGameSaveCreateUpdate( withContainer, m_containerMetadata.m_containerDisplayName.c_str(), gameSaveUpdate.GetPtrToHandle() );

		 Log::WriteAndDisplay( "XGameSaveCreateUpdate duration: %lfms\n", createUpdateSW.GetCurrentMilliseconds() );

         for ( auto& change : changes )
         {
            switch ( change.changeType )
            {
               case BlobChangeType::Set:
               {
                  ATG::RDTSCPStopWatch blobWriteSW;
                  blobWriteSW.Start();
                  
                  hr = XGameSaveSubmitBlobWrite( gameSaveUpdate, change.blobName, change.buffer, change.bufferSizeBytes );

                  Log::WriteAndDisplay( "XGameSaveSubmitBlobWrite duration: %lfms\n", blobWriteSW.GetCurrentMilliseconds() );
                  break;
               }
               case BlobChangeType::Delete:
               {
                  ATG::RDTSCPStopWatch blobDeleteSW;
                  blobDeleteSW.Start();
                  
                  hr = XGameSaveSubmitBlobDelete( gameSaveUpdate, change.blobName );

                  Log::WriteAndDisplay( "XGameSaveSubmitBlobDelete duration: %lfms\n", blobDeleteSW.GetCurrentMilliseconds() );
                  break;
               }
               default:
               {
                  assert( !"Shouldn't ever get here" );
                  __assume( 0 );
               }
            }

            if ( FAILED( hr ) )
            {
               Log::WriteAndDisplay( "ERROR: GameSave::SaveData - (%s, %s) : Failed while building update - error 0x%08X\n", m_containerMetadata.m_containerName.c_str(), m_containerMetadata.m_containerDisplayName.c_str(), static_cast<unsigned int>(hr) );
               return hr;
            }
         }

         // Done building our update.
		 ATG::RDTSCPStopWatch submitUpdateSW;
		 submitUpdateSW.Start();

         hr = XGameSaveSubmitUpdate( gameSaveUpdate );

		 Log::WriteAndDisplay( "XGameSaveSubmitUpdate duration: %lfms\n", submitUpdateSW.GetCurrentMilliseconds() );
      }

      if ( SUCCEEDED( hr ) )
      {
         Log::Write( "XGameSaveSubmitUpdate task succeeded\n" );
      }
      else
      {
         Log::WriteAndDisplay( "ERROR: XGameSaveSubmitUpdate returned HRESULT 0x%08X\n", static_cast<unsigned int>(hr) );
      }

	  Log::WriteAndDisplay( "GameSave::SaveData total duration: %lfms\n", saveDataSW.GetCurrentMilliseconds() );

      return hr;
   }

   ATG::HeapBuffer MakePaddingBuffer( uint32_t size, bool fillWithRandomData ) const // (for debugging only)
   {
      size = m_minSaveSize - size;

      ATG::HeapBuffer buffer = ATG::HeapBuffer::Allocate( size );
	  assert( buffer.IsValid() && "Buffer wasn't valid - out of memory?" );

      if ( fillWithRandomData )
      {
         int* data = reinterpret_cast<int*>( buffer.GetBuffer() );
         size_t randNumbersToWrite = size / sizeof( *data );
         for ( size_t i = 0; i < randNumbersToWrite; ++i )
         {
            data[ i ] = rand();
         }
      }

      return buffer;
   }

   TData& BackBuffer()
   {
      return m_data[ ( m_currentDataBuffer + 1 ) & 1 ];
   }

   void SwapBuffers()
   {
      m_currentDataBuffer = ( m_currentDataBuffer + 1 ) & 1;
   }

   size_t                  m_currentDataBuffer;
   TData                   m_data[ 2 ];
};
