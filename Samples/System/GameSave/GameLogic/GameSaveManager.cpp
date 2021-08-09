//--------------------------------------------------------------------------------------
// GameSaveManager.cpp
//
// GameSaveManager implementation.
//
// Xbox Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------------

#include "..\pch.h"
#include <XUser.h>
#include <XGameSave.h>
#include "..\Helpers\UTF8Helper.h"
#include "GameSaveManager.h"
#include "..\Helpers\TaskQueue.h"
#include <functional>
#include "..\SampleGame.h"

namespace GameSaveSample
{
   bool GameSaveManager::s_hasBeenInitialized = false;

   /**
    * Emits text to the log for the different kind of container-related errors one can receive from XGameSave...
    * \param const char * name - the name of the container in use.
    * \param HRESULT hr - the error code.
    */
   void EmitContainerErrors( const char* name, HRESULT hr )
   {
      switch ( hr )
      {
         case E_GS_INVALID_CONTAINER_NAME:
            Log::WriteAndDisplay( "ERROR: \'%s\' name is invalid for a container  hr=0x%08X\n", name, static_cast<unsigned int>(hr) );
            break;
         case E_GS_USER_CANCELED:
            Log::WriteAndDisplay( "ERROR: Container %s failed to sync user canceled hr=0x%08x\n", name, static_cast<unsigned int>(hr) );
            break;
         case E_GS_CONTAINER_NOT_IN_SYNC:
         case E_GS_CONTAINER_SYNC_FAILED:
            Log::WriteAndDisplay( "ERROR: Container %s failed to sync hr=0x%08x\n", name, static_cast<unsigned int>(hr) );
            break;
         case E_GS_HANDLE_EXPIRED:
            Log::WriteAndDisplay( "ERROR: Container %s failed, re-initialize provider  hr=0x%08X\n", name, static_cast<unsigned int>(hr) );
            break;
         case S_OK:
            break;
         default:
            Log::WriteAndDisplay( "ERROR: Unknown Container error %s hr=0x%08X\n", name, static_cast<unsigned int>(hr) );
      }
   }


   GameSaveManager::GameSaveManager( const char * serviceConfigId ) :
      m_isInitialized(false),
      m_scid(serviceConfigId),
      m_isSuspending( false ),
      m_syncMode( SyncMode::FullSyncAlways ),
      m_remainingQuotaInBytes(0)
   {
      Reset();
   }

   void GameSaveManager::Reset()
   {
      Log::Write( "GameSaveManager::Reset()\n" );

      {
         std::lock_guard<std::mutex> lock( m_mutex );

         m_isInitialized = false;
         m_isSuspending = false;
         m_remainingQuotaInBytes = 0;
         m_gameSaveProvider.Close();
         m_user.reset();
         m_gameBoardIndex.reset();
         m_gameBoardSaves.clear();

         for ( uint32_t i = 1; i <= c_saveSlotCount; ++i )
         {
            std::string containerName( ATG::Text::FormatStringScratch( u8"%s%d", GAME_BOARD_NAME_PREFIX, i ) );
            std::string containerDisplayName( ATG::Text::FormatStringScratch( u8"%s%d", GAME_BOARD_DISPLAY_NAME_PREFIX, i ) );

            m_gameBoardSaves.push_back( std::make_shared<GameSave<GameBoard>>( containerName, containerDisplayName, fnSaveGameBoard ) );
         }
      }
   }

   HRESULT GameSaveManager::InitializeForUserTask( std::shared_ptr<ATG::User>& user, bool refreshOnly )
   {
      std::lock_guard< std::mutex > lock( m_mutex );
      assert( user && user->IsValid() && "user object must be valid" );
      
      if ( !user || !user->IsValid() )
      {
         m_gameSaveProvider.Close();
         Log::WriteAndDisplay( u8"ERROR: Game Save Manager not set - new user was invalid\n" );
         return E_INVALIDARG;
      }

      bool syncOnDemand = ( GetSyncMode() == SyncMode::SyncOnDemand );

      Log::WriteAndDisplay( u8"InitializeForUserTask(%s, refreshOnly=%s) (%s)\n",
         FormatUserName( *user ).c_str(), refreshOnly ? "true" : "false",
         syncOnDemand ? u8"sync-on-demand" : u8"full sync" );

      m_user = user;

      const char* initializeModeDebugText = syncOnDemand ? u8"XGameSaveInitializeProvider (Sync On Demand mode)"
                                                         : u8"XGameSaveInitializeProvider (Full Sync mode)";

      ATG::RDTSCPStopWatch initProviderSW;
      initProviderSW.Start();

      // Close the handle if it's left open.
      m_gameSaveProvider.Close();

      HRESULT hr = XGameSaveInitializeProvider( *m_user, m_scid, syncOnDemand, 
                                                m_gameSaveProvider.GetPtrToHandle() );

      double durationMS = initProviderSW.GetCurrentMilliseconds();
      Log::WriteAndDisplay( u8"%s duration: %lfms\n", initializeModeDebugText, durationMS );

      if ( FAILED( hr ) )
      {
         Log::WriteAndDisplay( u8"ERROR: %s returned exception (%s)\n", initializeModeDebugText, FormatHResult( hr ).c_str() );
         return hr;
      }

      s_hasBeenInitialized = true;

      Log::WriteAndDisplay( u8"ConnectedStorageSpace %s\n", refreshOnly ? u8"refreshed" : u8"created" );

      if ( !m_isSuspending )
      {
         LoadIndexBlocking();

         m_isInitialized = true;

         // We only query the blobs if the mode is FullSyncAlways, as querying will trigger a sync.

         bool queryBlobs = !syncOnDemand;

         hr = EnumerateContainersBlocking( queryBlobs );

         if ( !refreshOnly )
         {
            WriteGameSaveMetadataToDisplayLog( queryBlobs );
         }

         hr = UpdateRemainingQuotaBlocking();

         DX::ThrowIfFailed( hr );
      }

      return S_OK;
   }

   ATG::AsyncAction< HRESULT >& GameSaveManager::InitializeForUser( std::shared_ptr<ATG::User>& user, bool refreshOnly )
   {
      assert( user->IsValid() && "user object must be valid" );

      m_asyncInitializeForUserTask.Reset();
      m_asyncInitializeForUserTask.SetFunction( std::function< HRESULT() >(

         [ this, &user, refreshOnly ]() -> HRESULT
         {
            return InitializeForUserTask( user, refreshOnly );
         }

      ) );

      DX::ThrowIfFailed( m_asyncInitializeForUserTask.Start( g_Game->GetGeneralThreadPoolTaskQueue() ) );

      return m_asyncInitializeForUserTask;
   }

   ATG::AsyncAction< HRESULT >& GameSaveManager::ResumeForUser( std::shared_ptr<ATG::User>& user )
   {
      return InitializeForUser( user, true );
   }

#pragma region GameSaveManager::LoadContainerMetadata related

   HRESULT GameSaveManager::EnumerateContainersBlocking( _In_ bool queryBlobs /*= true */ )
   {
      assert( m_gameSaveProvider != nullptr && "Game Save Provider was not initialized" );

      // The container info context we'll use to track some of the results of this operation, and that is used to pass 
      // parameters to the callback.
      struct ContainerInfoContext
      {
         bool queryBlobs;
         std::function< bool( const XGameSaveContainerInfo*, bool ) > instanceCallbackFn;
         size_t containerCount;
         HRESULT hr;
      };

      ContainerInfoContext context;
      context.queryBlobs = queryBlobs;
      context.instanceCallbackFn = std::bind( &GameSaveManager::ContainerInfoCallback, this,
         std::placeholders::_1, std::placeholders::_2 );
      context.containerCount = 0;
      context.hr = S_FALSE;

      Log::WriteAndDisplay( u8"EnumerateContainersBlocking: XGameSaveEnumerateContainerInfo: enumerating all containers\n");

      // we initialize context.hr to S_FALSE. If the callback doesn't return any values at all, the callback function
      // will never be run, and context.hr will never be updated. 

      HRESULT hr = XGameSaveEnumerateContainerInfo( m_gameSaveProvider, &context,
         [] ( _In_ const XGameSaveContainerInfo* info, _In_ void* context ) -> bool
         {
            auto ctx = static_cast<ContainerInfoContext*>( context );
            ++ctx->containerCount;

            // Bounce to LoadContainerMetadataCallback
            ctx->hr = ctx->instanceCallbackFn( info, ctx->queryBlobs );
            return !FAILED( ctx->hr ); // if failed, stop the query. Otherwise continue.               
         }
      );

      if ( FAILED( hr ) )
      {
         CloseProviderIfStale( hr );
         return hr;
      }

      if ( FAILED( context.hr ) )
      {
         CloseProviderIfStale( context.hr );
         return context.hr;
      }

      if ( context.hr == S_FALSE || context.containerCount == 0 )
      {
         Log::Write( "EnumerateContainersBlocking: no containers found\n" );
      }
      else
      {
         Log::WriteAndDisplay( u8"Game board metadata updated\n" );
      }

      return S_OK;
   }


   HRESULT GameSaveManager::LoadContainerMetadataBlocking( _In_z_ const char* containerName, _In_ bool queryBlobs )
   {
      assert( containerName && "containername cannot be null" );
      assert( m_gameSaveProvider != nullptr && "Game Save Provider was not initialized" );

      // The container info context we'll use to track some of the results of this operation, and that is used to pass 
      // parameters to the callback.
      struct ContainerInfoContext
      {
         bool queryBlobs;
         std::function< bool( const XGameSaveContainerInfo*, bool ) > instanceCallbackFn;
         HRESULT hr;
      };

      ContainerInfoContext context;
      context.queryBlobs = queryBlobs;
      context.instanceCallbackFn = std::bind( &GameSaveManager::ContainerInfoCallback, this,
         std::placeholders::_1, std::placeholders::_2 );
      context.hr = S_FALSE; 

      Log::WriteAndDisplay( u8"LoadContainerMetadataBlocking: XGameSaveGetContainerInfo: getting container \'%s\'\n", containerName );

      // context.hr is initialized to S_FALSE. If the callback doesn't return any values at all, the callback function
      // will never be run, and context.hr will never be updated. 

      HRESULT hr = XGameSaveGetContainerInfo( m_gameSaveProvider, containerName, &context,
         [] ( _In_ const XGameSaveContainerInfo* info, _In_ void* context ) -> bool
         {
            auto ctx = static_cast<ContainerInfoContext*>( context );

            // Bounce to LoadContainerMetadataCallback
            ctx->hr = ctx->instanceCallbackFn( info, ctx->queryBlobs );
            return !FAILED( ctx->hr ); // if failed, stop the query. Otherwise continue.               
         }
      );

      if ( FAILED( hr ) )
      {
         CloseProviderIfStale( hr );
         return hr;
      }

      if ( FAILED( context.hr ) )
      {
         CloseProviderIfStale( context.hr );
         return context.hr;
      }

      if ( context.hr == S_FALSE )
      {
         Log::Write( "LoadContainerMetadataBlocking: container \'%s\' not found\n", containerName );
      }
      else
      {
         Log::WriteAndDisplay( u8"Game board metadata updated\n" );
      }

      return S_OK;
   }

   struct EnumerationMetadata
   {
      std::vector<GameSaveBlobMetadata>* containerMetadata;
   };

   HRESULT GameSaveManager::ContainerInfoCallback( _In_ const XGameSaveContainerInfo* info, bool queryBlobs )
   {
      Log::WriteAndDisplay( u8" ? Found container \'%s\' (displayName=\'%s\', needsSync=%s, blobCount=%u, size=%s (%llu bytes), lastModified=%s)\n",
         info->name ? info->name : "[[nullptr]]",
         info->displayName ? info->displayName : "[[nullptr]]",
         info->needsSync ? "true" : "false",
         info->blobCount,
         FormatByteValueAsStringWithUnits( info->totalSize ).c_str(), info->totalSize,
         FormatLocalTimeFromDateTime( info->lastModifiedTime ).c_str()
      );

      const char* containerName = info->name;

      GameSaveContainerMetadata* gameSaveMetadata = nullptr;

      // Match container found in query to an existing GameSave object

      if ( m_gameBoardIndex->GetContainerMetadata().m_containerName.compare( containerName ) == 0 )
      {
         // Does it match the current container?
         gameSaveMetadata = &( m_gameBoardIndex->GetContainerMetadata() );
      }
      else
      {
         // Does it match any container?
         auto it = std::find_if( m_gameBoardSaves.begin(), m_gameBoardSaves.end(),
            [ containerName ] ( std::shared_ptr<GameSave<GameBoard>> gameBoardSave )
            {
               return gameBoardSave->GetContainerMetadata().m_containerName.compare( containerName ) == 0;
            }
         );

         if ( it != m_gameBoardSaves.end() )
         {
            gameSaveMetadata = &( ( *it )->GetContainerMetadata() );
         }
         else
         {
            Log::WriteAndDisplay( u8"?WARNING: Found non-game-related container: %s (%s) (%s / %llu bytes)\n", containerName,
               info->displayName, FormatByteValueAsStringWithUnits( info->totalSize ).c_str(), info->totalSize );
         }
      }

      // Update saved metadata if the query result matches a known type of data
      if ( gameSaveMetadata != nullptr )
      {
         if ( queryBlobs )
         {
            // Get blob metadata

            XGameSaveContainerHandle container = nullptr;
            HRESULT hr = XGameSaveCreateContainer( m_gameSaveProvider, containerName, &container );

            if ( FAILED( hr ) )
            {
               EmitContainerErrors( containerName, hr );
            }

            EnumerationMetadata metadata;
            metadata.containerMetadata = &( gameSaveMetadata->m_blobs );
            metadata.containerMetadata->clear();

            hr = XGameSaveEnumerateBlobInfo( container, static_cast<void*>( &metadata ), &GameSaveManager::EnumerateBlobInfoCallback );

            if ( FAILED( hr ) )
            {
               CloseProviderIfStale( hr );
               return hr;
            }
         }

         gameSaveMetadata->m_isGameDataOnDisk = true;
         gameSaveMetadata->m_containerDisplayName = std::string( info->displayName );
         gameSaveMetadata->m_lastModified = info->lastModifiedTime;
         gameSaveMetadata->m_needsSync = info->needsSync;
         gameSaveMetadata->m_totalSize = info->totalSize;
      }

      return S_OK;
   }

   bool CALLBACK GameSaveManager::EnumerateBlobInfoCallback( _In_ const XGameSaveBlobInfo* info, _In_opt_ void* context )
   {
      EnumerationMetadata* metadata = static_cast<EnumerationMetadata*>( context );
      metadata->containerMetadata->push_back( GameSaveBlobMetadata( info->name, info->size ) );
      return true;
   }

#pragma endregion GameSaveManager::LoadContainerMetadata related 


   HRESULT GameSaveManager::UpdateRemainingQuotaBlocking( _Out_opt_ int64_t* quota /* = nullptr */ )
   {
      Log::Write( "GameSaveManager::GetRemainingQuotaBlocking()\n" );

      assert( m_gameSaveProvider != nullptr && "Game Save Provider was not initialized" );

      if ( quota != nullptr )
      {
         *quota = -1LL;
      }

      int64_t tempQuota;

      HRESULT hr = XGameSaveGetRemainingQuota( m_gameSaveProvider, &tempQuota );

      if ( FAILED( hr ) )
      {
         Log::WriteAndDisplay( u8"ERROR: XGameSaveGetRemainingQuota returned error (%s)\n", FormatHResult( hr ).c_str() );
         CloseProviderIfStale( hr );
         return hr;
      }

      if ( quota != nullptr )
      {
         *quota = tempQuota;
      }

      m_remainingQuotaInBytes = tempQuota;
      Log::WriteAndDisplay( u8"Remaining quota: %s (%lld bytes)\n",
         FormatByteValueAsStringWithUnits( (uint64_t) ( tempQuota ) ).c_str(), tempQuota );

      return S_OK;
   }


   HRESULT GameSaveManager::OnSignOutTaskBody()
   {
      Log::WriteAndDisplay( u8"GameSaveManager::OnSignOutTaskBody() start...\n" );

      if ( !g_Game->GetGameSaveManager().HasActiveBoard() )
         return S_OK;

      uint32_t activeBoard = g_Game->GetGameSaveManager().GetActiveBoardNumber();

      HRESULT hr = SaveBlocking( activeBoard, SaveMode::SaveOnlyIfDirty );

      if ( FAILED( hr ) )
         return hr;

      if ( FAILED( SaveIndexBlocking( true ) ) )
         return E_FAIL;

      Log::WriteAndDisplay( u8"GameSaveManager::OnSignOutTaskBody() completed successfully\n" );

      // NOTE: User should get reset once sign-out has completed, but this likely happens via the state machine that
      // switches to the AcquireUser screen.
      return hr;
   }


   void GameSaveManager::Suspend()
   {
      Log::WriteAndDisplay( u8"GameSaveManager::Suspend() start...\n" );

      std::lock_guard< std::mutex > lock( m_mutex );

      m_isSuspending = true;

      if ( m_isInitialized )
      {
         m_isInitialized = false;

         // Since the app is suspending, this work needs to be completed as fast as possible.
         {
             uint32_t activeBoard = g_Game->GetGameSaveManager().GetActiveBoardNumber();
             HRESULT hr = SaveBlocking( activeBoard, SaveMode::SaveOnlyIfDirty );
             if ( FAILED( hr ) )
             {
                 Log::WriteAndDisplay( u8"GameSaveManager::Suspend(): call to Save failed with hr=0x%08X\n" );
             }
             else if ( hr == S_FALSE )
             {
                 Log::WriteAndDisplay( u8"GameSaveManager::Suspend(): Save call succeeded without saving (not dirty)\n" );
             }
         
             hr = SaveIndexBlocking( true );
         
             if ( FAILED( hr ) )
             {
                 Log::WriteAndDisplay( u8"GameSaveManager::Suspend(): call to SaveIndex failed\n" );
             }
             else if ( hr == S_FALSE )
             {
                 Log::WriteAndDisplay( u8"GameSaveManager::Suspend(): Save Index succeeded without saving (not dirty)\n" );
             }
         
             Log::WriteAndDisplay( u8"GameSaveManager::Suspend() (with save) complete\n" );
                
             m_isSuspending = false;
         }
      }
      else
      {
         m_isSuspending = false;
         Log::WriteAndDisplay( u8"GameSaveManager::Suspend() complete\n" );
      }
   }

   void GameSaveManager::WriteGameSaveMetadataToDisplayLog( bool listBlobs )
   {
      if ( m_gameBoardIndex == nullptr )
      {
         return;
      }

      WriteContainerMetadataToDisplayLog( listBlobs, m_gameBoardIndex->m_containerMetadata );

      for ( const auto& gameSave : m_gameBoardSaves )
      {
         if ( gameSave->m_containerMetadata.m_isGameDataOnDisk )
         {
            WriteContainerMetadataToDisplayLog( listBlobs, gameSave->m_containerMetadata );
         }
      }
   }

   HRESULT GameSaveManager::LoadIndexBlocking()
   {
      Log::WriteAndDisplay( u8"Loading game board index...\n" );

      assert( m_gameSaveProvider.IsValid() && "Game Save provider must be valid" );

      ATG::GameSaveContainerHandle container;
      HRESULT hr = XGameSaveCreateContainer( m_gameSaveProvider, GAME_BOARD_INDEX_NAME, container.GetPtrToHandle() );
      if ( FAILED( hr ) )
      {
         EmitContainerErrors( GAME_BOARD_INDEX_NAME, hr );
         return hr;
      }

      m_gameBoardIndex = std::make_unique<GameSave<GameBoardIndex>>( GAME_BOARD_INDEX_NAME, GAME_BOARD_INDEX_DISPLAY_NAME, fnSaveContainerIndex );

      hr = m_gameBoardIndex->Load( container );

      if ( SUCCEEDED( hr ) )
      {
         Log::WriteAndDisplay( u8"Game board index loaded\n" );
         return hr;
      }
      else
      {
         Log::WriteAndDisplay( u8"Game board index NOT found...creating new one\n" );

         hr = m_gameBoardIndex->Save( container, GAME_BOARD_INDEX_NAME );

         if ( SUCCEEDED( hr ) )
         {
            Log::WriteAndDisplay( u8"Game board index initialization complete\n" );
         }
         else
         {
            Log::WriteAndDisplay( u8"ERROR: Game board index creation FAILED\n" );
         }

         return hr;
      }
   }

   HRESULT GameSaveManager::SaveIndexBlocking( bool saveOnlyIfDirty )
   {
      assert( m_gameSaveProvider.IsValid() && "Game Save provider must be valid" );

      if ( !m_gameSaveProvider )
      {
         return HRESULT_FROM_WIN32( ERROR_BAD_PROVIDER );
      }

      if ( saveOnlyIfDirty && !IsSaveIndexDirty() )
      {
         return S_FALSE;
      }

      Log::WriteAndDisplay( u8"Saving game board index...\n" );

      auto& containerName = m_gameBoardIndex->m_containerMetadata.m_containerName;

      ATG::GameSaveContainerHandle container;

      HRESULT hr = XGameSaveCreateContainer( m_gameSaveProvider, containerName.c_str(), container.GetPtrToHandle() );
      if ( FAILED( hr ) )
      {
         EmitContainerErrors( containerName.c_str(), hr );
         return hr;
      }

      hr = m_gameBoardIndex->Save( container, containerName.c_str() );

      if ( SUCCEEDED( hr ) )
      {
         Log::WriteAndDisplay( u8"Game board index saved\n" );
         return hr;
      }
      else
      {
         Log::WriteAndDisplay( u8"ERROR: Game board index save FAILED\n" );
         return hr;
      }
   }

   HRESULT GameSaveManager::DeleteBlocking( uint32_t activeBoard )
   {
      assert( m_gameSaveProvider.IsValid() && "Game Save Provider not valid" );
      if ( !m_gameSaveProvider.IsValid() )
      {
         return HRESULT_FROM_WIN32( ERROR_BAD_PROVIDER );
      }

      Log::WriteAndDisplay( u8"Deleting game board %d (DeleteContainerAsync)...\n", activeBoard );

      const char* containerToDeleteName = m_gameBoardSaves[ activeBoard - 1 ]->m_containerMetadata.m_containerName.c_str();

      HRESULT hr = XGameSaveDeleteContainer( m_gameSaveProvider, containerToDeleteName );

      if ( FAILED( hr ) )
      {
         Log::WriteAndDisplay( u8"ERROR: Game board %d delete FAILED\n", activeBoard );
         EmitContainerErrors( containerToDeleteName, hr );
      }

      return hr;
   }

   HRESULT GameSaveManager::SaveBlocking( uint32_t boardNumber, SaveMode mode )
   {
      assert( m_gameSaveProvider.IsValid() && "Game Save provider must be valid" );

      if ( mode == SaveMode::SaveOnlyIfDirty && !m_gameBoardSaves[ boardNumber - 1 ]->m_isGameDataDirty )
      {
         return S_FALSE;
      }

      return SaveInternal( boardNumber );
   }

   HRESULT GameSaveManager::SaveInternal( uint32_t activeBoard )
   {
      HRESULT hr = S_OK;

      const char* containerName = m_gameBoardSaves[ activeBoard - 1 ]->m_containerMetadata.m_containerName.c_str();

      ATG::GameSaveContainerHandle container;
      hr = XGameSaveCreateContainer( m_gameSaveProvider, containerName, container.GetPtrToHandle() );
      EmitContainerErrors( containerName, hr );
      DX::ThrowIfFailed( hr );

      Log::WriteAndDisplay( u8"Saving game board %d ...\n", activeBoard );

      if ( FAILED( hr ) )
      {
         CloseProviderIfStale( hr );
         return hr;
      }

      hr = m_gameBoardSaves[ activeBoard - 1 ]->Save( container, containerName );
      if ( FAILED( hr ) )
      {
         Log::WriteAndDisplay( u8"ERROR: Game board %d save FAILED, error (0x%08X)\n", activeBoard, static_cast<unsigned int>(hr) );

         CloseProviderIfStale( hr );
         return hr;
      }

      Log::WriteAndDisplay( u8"Game board %d saved\n", activeBoard );
      return S_OK;
   }

   void GameSaveManager::MarkActiveBoardDirty()
   {
      auto activeGameSave = GetActiveBoardGameSave();
      assert( activeGameSave && "Do not have an active board gamesave" );

      if ( activeGameSave != nullptr )
      {
         activeGameSave->m_isGameDataDirty = true;
      }
   }

   void GameSaveManager::WriteContainerMetadataToDisplayLog( bool listBlobs, const GameSaveContainerMetadata& containerMetadata )
   {
      char lastModifiedTime[ 32 ];
      ctime_s( lastModifiedTime, sizeof( lastModifiedTime ), &containerMetadata.m_lastModified );

      Log::WriteAndDisplay(
         u8"%s (%s / %llu bytes) (%s)%s Last Modified on: %s",
         containerMetadata.m_containerDisplayName.c_str(), FormatByteValueAsStringWithUnits( containerMetadata.m_totalSize ).c_str(),
         containerMetadata.m_totalSize,
         containerMetadata.m_needsSync ? u8"needs sync" : u8"synced",
         containerMetadata.m_changedSinceLastSync ? u8" (changed on disk)" : u8"",
         lastModifiedTime
      );

      if ( listBlobs )
      {
         for ( const auto& blob : containerMetadata.m_blobs )
         {
            Log::WriteAndDisplay( u8"    %s (%s / %d bytes)\n", blob.m_blobName.c_str(),
               FormatByteValueAsStringWithUnits( blob.m_blobSize ).c_str(), blob.m_blobSize );
         }
      }
   }

   SyncMode GameSaveManager::GetSyncMode() const noexcept
   {
      return m_syncMode;
   }

   void GameSaveManager::SetSyncMode( SyncMode value )
   {
      if ( HasSyncOnDemandModeBeenSet() )
      {
         throw std::runtime_error( "cannot change the sync-on-demand setting once a save context has been set" );
      }
      m_syncMode = value;
   }

   uint32_t GameSaveManager::GetIndexUpdateCount() const noexcept
   {
      if ( m_gameBoardIndex != nullptr )
      {
         return m_gameBoardIndex->FrontBuffer().m_updateCount;
      }

      return 0;
   }

   bool GameSaveManager::HasActiveBoard() const noexcept
   {
      return GetActiveBoardNumber() > 0;
   }

   bool GameSaveManager::HasSyncOnDemandModeBeenSet() noexcept
   {
      return s_hasBeenInitialized;
   }

   ATG::GameSaveProviderHandle& GameSaveManager::GetGameSaveProvider() noexcept
   {
      return m_gameSaveProvider;
   }

   GameBoard& GameSaveManager::GetActiveBoard()
   {
      auto activeBoardGameSave = GetActiveBoardGameSave();
      assert( activeBoardGameSave != nullptr && "Trying to retrieve the active GameBoard when none has been set" );
      return activeBoardGameSave->FrontBuffer();
   }

   std::shared_ptr<GameSave<GameBoard>> GameSaveManager::GetActiveBoardGameSave() const
   {
      auto boardNumber = GetActiveBoardNumber();
      auto index = boardNumber - 1;
      if ( boardNumber == 0 || index >= m_gameBoardSaves.size() )
      {
         return nullptr;
      }
      else
      {
         return m_gameBoardSaves[ index ];
      }
   }

   // Note: game boards are 1-based; board 0 means there is currently no active board
   uint32_t GameSaveManager::GetActiveBoardNumber() const noexcept
   {
      if ( m_gameBoardIndex != nullptr )
      {
         return m_gameBoardIndex->FrontBuffer().m_activeBoard;
      }

      return 0;
   }

   void GameSaveManager::SetActiveBoardNumberAsync( uint32_t value )
   {
      Log::Write( u8"SetActiveBoardNumberAsync - set to board %u\n", value );

      if ( m_gameBoardIndex != nullptr )
      {
         auto& gameBoardIndex = m_gameBoardIndex->FrontBuffer();
         if ( gameBoardIndex.m_activeBoard != value )
         {
            uint32_t activeBoard = g_Game->GetGameSaveManager().GetActiveBoardNumber();

            if ( activeBoard <= m_gameBoardSaves.size() )
            {
               static ATG::AsyncAction<void> setActiveBoardAsync;
               if ( !setActiveBoardAsync.IsReady() )
               {
                  if ( !setActiveBoardAsync.IsInTerminalState() )
                  {
                     Log::WriteAndDisplay( u8"Stall: Waiting for previous board switch to complete\n" );
                     setActiveBoardAsync.WaitForCompletion();
                  }

                  setActiveBoardAsync.Reset();
               }
               setActiveBoardAsync.SetFunction( std::function<void()>(
                  [ this, value ] ()
                  {
                     SetActiveBoardNumberBlocking( value );
                  }
               ) );
               setActiveBoardAsync.Start( g_Game->GetGeneralThreadPoolTaskQueue() );
            }

            gameBoardIndex.m_activeBoard = value;
            m_gameBoardIndex->SetData( &gameBoardIndex );
         }
      }
   }

   void GameSaveManager::SetActiveBoardNumberBlocking( uint32_t boardNumber )
   {
      HRESULT hr = S_OK;

      // save the current board if it is dirty before switching to another
      hr = SaveBlocking( boardNumber, SaveMode::SaveOnlyIfDirty );

      if ( SUCCEEDED( hr ) )
      {
         const char* containerName = m_gameBoardSaves[ boardNumber - 1 ]->GetContainerMetadata().m_containerName.c_str();
         hr = LoadContainerMetadataBlocking( containerName, true );

         if ( FAILED( hr ) )
         {
            Log::WriteAndDisplay( "ERROR: SetActiveBoardNumber: LoadContainerMetadataBlocking failed with error 0x%08X\n", static_cast<unsigned int>(hr) );
            EmitContainerErrors( containerName, hr );
         }

         hr = UpdateRemainingQuotaBlocking();
         if ( FAILED( hr ) )
         {
            Log::WriteAndDisplay( "ERROR: SetActiveBoardNumber: GetRemainingQuotaBlocking failed with error 0x%08X\n", static_cast<unsigned int>(hr) );
         }

      }
   }

   bool GameSaveManager::IsActiveBoardDirty() const noexcept
   {
      auto activeBoardGameSave = GetActiveBoardGameSave();
      return activeBoardGameSave != nullptr && activeBoardGameSave->m_isGameDataDirty;
   }

   bool GameSaveManager::IsSaveIndexDirty() const noexcept
   {
      return m_gameBoardIndex != nullptr && m_gameBoardIndex->m_isGameDataDirty;
   }

   int64_t GameSaveManager::GetRemainingQuotaInBytes() const noexcept
   {
      return m_remainingQuotaInBytes;
   }

   void GameSaveManager::CloseProviderIfStale( HRESULT hr )
   {
      if ( hr == E_GS_HANDLE_EXPIRED && m_gameSaveProvider != nullptr )
      {
         Log::WriteAndDisplay( "Closed stale GameSave provider handle." );
         ::XGameSaveCloseProvider( m_gameSaveProvider );
         m_gameSaveProvider.Close();
      }
   }

   HRESULT GameSaveManager::ReadBlocking( uint32_t boardNumber )
   {
      assert( HasActiveBoard() && "Must have an active board" );
      assert( m_gameSaveProvider.IsValid() && "Game Save provider must be valid" );

      HRESULT hr = S_OK;

      Log::WriteAndDisplay( u8"Loading game board %d (ReadAsync)...\n", boardNumber );

      const char* containerName = m_gameBoardSaves[ boardNumber - 1 ]->m_containerMetadata.m_containerName.c_str();

      ATG::GameSaveContainerHandle container;
      hr = XGameSaveCreateContainer( m_gameSaveProvider, containerName, container.GetPtrToHandle() );
      EmitContainerErrors( containerName, hr );
      DX::ThrowIfFailed( hr );

      hr = m_gameBoardSaves[ boardNumber - 1 ]->Load( container );
      if ( SUCCEEDED( hr ) )
      {
         Log::WriteAndDisplay( u8"Game board %d loaded\n", boardNumber );
      }
      else
      {
         Log::WriteAndDisplay( u8"ERROR: Game board %d load FAILED - Error=0x%08X\n", boardNumber, static_cast<unsigned int>(hr) );
      }

      return hr;
   }

   HRESULT GameSaveManager::DeleteBlobsBlocking( uint32_t boardNumber )
   {
      assert( m_gameSaveProvider.IsValid() && "Game Save provider must be valid" );

      Log::WriteAndDisplay( u8"Deleting game board %d blobs (SubmitUpdatesAsync)...\n", boardNumber );

      const char* containerName = m_gameBoardSaves[ boardNumber - 1 ]->m_containerMetadata.m_containerName.c_str();
      ATG::GameSaveContainerHandle container = nullptr;
      HRESULT hr = XGameSaveCreateContainer( m_gameSaveProvider, containerName, container.GetPtrToHandle() );
      EmitContainerErrors( containerName, hr );
      DX::ThrowIfFailed( hr );

      hr = m_gameBoardSaves[ boardNumber - 1 ]->DeleteBlobs( container );

      if ( SUCCEEDED( hr ) )
      {
         Log::WriteAndDisplay( u8"Game board %d blobs deleted\n", boardNumber );
      }
      else
      {
         Log::WriteAndDisplay( u8"ERROR: Game board %d blob delete FAILED - Error = 0x%08X\n", boardNumber, static_cast<unsigned int>(hr) );
      }

      return hr;
   }

   GameSaveManager::~GameSaveManager()
   {
      if ( !m_asyncInitializeForUserTask.IsReady() )
      {
         m_asyncInitializeForUserTask.WaitForCompletion();
      }
   }

} // namespace GameSaveSample
