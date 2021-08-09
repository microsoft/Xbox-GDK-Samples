//--------------------------------------------------------------------------------------
// GameSaveManager.h
//
// Handles game save logic and operations for the Word Game game in this sample.
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

#include "GameBoard.h"
#include "GameSave.h"
#include <DirectXMath.h>
#include "..\Helpers\XGameSaveHandleWrappers.h"
#include "..\Helpers\XUserHandleWrapper.h"
#include "..\Helpers\ScopedLockWrappers.h"
#include "..\Helpers\User.h"
#include "..\Helpers\AsyncAction.h"

/**
 *	The container name used for the game board index.
 */
constexpr const char* GAME_BOARD_INDEX_NAME = u8"game_board_index";

/**
 *	The display name used for the game board index container.
 */
constexpr const char * GAME_BOARD_INDEX_DISPLAY_NAME = u8"Game Board Index";

/**
 *	The container name prefix used for a specific numbered game board. (The game board number is appended to this string).
 */
constexpr const char* GAME_BOARD_NAME_PREFIX = u8"game_board_";

/**
 *	The display name prefix used for a game board container. (The game board number is appended to this string).
 */
constexpr const char* GAME_BOARD_DISPLAY_NAME_PREFIX = u8"Game Board ";

namespace GameSaveSample
{
   /**
    *	A structure used to keep track of the game boards in play.
    */
   struct GameBoardIndex
   {
      uint32_t m_version = 1;
      uint32_t m_updateCount = 0;
      uint32_t m_activeBoard = 1;
   };

   /*
    *	Function called every time the GameBoardIndex is saved (\see GameSave::OnSave).
    */
   const std::function<void( GameBoardIndex& )> fnSaveContainerIndex = [] ( GameBoardIndex& dataToSave )
   {
      dataToSave.m_updateCount++;
   };

   /**
    *	The save mode to use when saving the current state of the game.
    */
   enum class SaveMode
   {
      SaveOnlyIfDirty,     //< Save only occurs if the state is dirty.
      SaveAlways           //< A save is forced.
   };

   /**
    *	The sync mode to use for the title session. This is selected at startup. 
    *
    * \remarks Typically in your own title code you would pick one mode or the other for your title to use 
    *          based on your title's save-game needs. Most titles do *not* require sync-on-demand, and no title needs
    *          to be able to flexibly switch between the two - this is only for the purposes of showing both methods
    *          in this sample code.
    */
   enum class SyncMode
   {
      FullSyncAlways,   //< All containers are synchronized as the provider is opened for this title.
      SyncOnDemand      //< Containers are synchronized on-demand as they are needed, initiated under title control.
   };

   /**
    *	GameSaveManager manages the loading, saving, and PLM handling of the game save blobs that are specific
    * to this game. These include an index blob and a blob for each game board saved.
    *
    * \note **IMPLEMENTATION NOTE:** Here, each blob is saved within its own container. This design enables the sample
    * to demonstrate more of the functionality of the GameSave API, including sync-on-demand.
    *
    * However in a retail game, given the small size of each game save, a better implementation might be to
    * put all the blobs in one container to reduce the number of calls made to the web service.
    */
   class GameSaveManager final
   {
   public:
      /**
       * Creates a new GameSaveManager instance.
       * 
       * \param char const* serviceConfigId - the service config ID (SCID) used by this title. For Game Core titles
       *                                      this is typically of the form 00000000-0000-0000-0000-0000NNNNNNNN, where
       *                                      NNNNNNNN is the title ID as a hexadecimal value.
       */
      GameSaveManager( char const * serviceConfigId );

      /**
       *	Destructor for GameSaveManager.
       */
      ~GameSaveManager();

      /**
       * Clear the index and games saves loaded in memory and reset the user context
       */
      void Reset();

      /**
       * Creates a new ATG::AsyncAction task which initializes or refreshes the save game state for the user and 
       * produces an HRESULT indicating success or failure. 
       * 
       * The task runs on the general thread pool, and uses the async task queue. 
       *
       * \note Because this task is tracked via a single instance object, you cannot call this function again until
       *       the original operation has completed (or has been canceled, or failed).
       * 
       * \param std::shared_ptr<ATG::User> & user - the user whose state we care about.
       * \param _In_ bool refreshOnly         - if true, refreshes state rather than fully initializing.
       * \return ATG::AsyncAction< HRESULT >& - the ATG::AsyncAction which will run on the thread pool. It produces an
       *                                        HRESULT representing the success or failure of the operation when 
       *                                        complete.
       */
      ATG::AsyncAction< HRESULT >& InitializeForUser( std::shared_ptr<ATG::User>& user, _In_ bool refreshOnly );

      /**
       * This function initializes or refreshes the save game state for the user, establishes a user context for game 
       * saves, syncing with the cloud if possible, and loading the index file (or creating one if needed) and produces 
       * an HRESULT indicating success or failure.
       *
       * It is intended to be called on the thread pool from an asynchronous task. Multiple operations
       * utilize this task body code.
       *
       * \note This version of the function is a blocking synchonous operation. A wrapped version of this function
       *       which runs on the thread pool is provided by @see InitializeForUser.
       *
       * \param std::shared_ptr<ATG::User> & user - the user whose state we care about.
       * \param _In_ bool refreshOnly - if true, refreshes state rather than fully initializing.
       * \return HRESULT - the success or error result code for the operation.
       */
      HRESULT InitializeForUserTask( std::shared_ptr<ATG::User>& user, bool refreshOnly );

      /**
       * Resume the game for the user who was playing when the game was suspended.
       *
       * The task runs on the general thread pool, and uses the async task queue.
       *
       * \note Because this task is tracked via a single instance object, you cannot call this function again until
       *       the original operation has completed (or has been canceled, or failed).
       *
       * \param std::shared_ptr<ATG::User> & user - the user object for the user who was playing when the game was
       *                                            suspended. (Note: this may no longer be a valid user).
       * \return ATG::AsyncAction< HRESULT >& - Returns an ATG::AsyncAction task which will produce the HRESULT error code
       *                                        for the operation when completed.
       */ATG::AsyncAction< HRESULT >& ResumeForUser( std::shared_ptr<ATG::User>& user );

      /**
       * Loads or refresh a specific game save container and blob metadata
       *
       * Use queryBlobs bool to specify whether or not to query and update blob info - querying blobs has the side
       * effect of forcing a sync for sync-on-demand save contexts, which may be undesirable.
       * 
       * \param const char * containerName - the container name to sync
       * \param bool queryBlobs - whether or not to query the blob metadata for each container.
       * \return HRESULT - The error code. If this operation succeeded, it will return S_OK. If no matching container
       *                   is found it will return S_FALSE.
       */
      HRESULT LoadContainerMetadataBlocking( _In_z_ const char* containerName, _In_ bool queryBlobs );

      /**
       * Enumerates all containers, optionally querying their blob metadata
       *
       * Use queryBlobs bool to specify whether or not to query and update blob info - querying blobs has the side
       * effect of forcing a sync for sync-on-demand save contexts, which may be undesirable.
       * 
       * \param _In_ bool queryBlobs - whether or not to query the blob metadata for each container.
       * \return HRESULT - The error code. If this operation succeeded, it will return S_OK. If no containers are found,
       *                   It will return S_FALSE.
       */
      HRESULT EnumerateContainersBlocking( _In_ bool queryBlobs );

      
      /**
       * Obtains the current remaining quota in bytes for this user for this title, optionally returning the value.
       * 
       * The value is used internally for tracking, and is updated after any updates to the containers. Returning the
       * value for use outside of the GameSaveManager is optional.
       * 
       * \param _Out_opt_ int64_t * quota - the variable to return the quota size in, or nullptr.
       * \return HRESULT - the success or failure result code.
       */
      HRESULT UpdateRemainingQuotaBlocking( _Out_opt_ int64_t* quota = nullptr );

      
      /**
       * Handles user sign-out, saving the index and current game board as needed. 
       *
       * This method is part of a larger chain of methods called to perform the signout task. It's meant to be called
       * within the context of an asynchronous task on a different thread to the main thread, as it may block.
       * 
       * \return HRESULT - whether the sign-out operation succeeded or failed.
       */
      HRESULT OnSignOutTaskBody();

      /**
       * Suspends the game, writing out any dirty data to the game save system.
       */
      void Suspend();

      /**
       * Write current container metadata (and optionally blob info) to the game display log
       * 
       * \param bool listBlobs - whether or not to display the blob metadata
       */
      void WriteGameSaveMetadataToDisplayLog( bool listBlobs );

      //
      // Game Index Tasks
      //

      /**
       * Load the index blob that tells us the last board played by the current user.
       *
       * \note This function is synchronous and could block. It's meant to be called from within an async task running
       *       on the thread pool.
       *
       * \return HRESULT - the success or error code for the result of the operation.
       */
      HRESULT LoadIndexBlocking();

      /**
       * Save the index blob that tells us the last board played by the current user.
       * 
       * \note This function is synchronous and could block. It's meant to be called from within an async task running
       *       on the thread pool.
       *
       * \param bool saveOnlyIfDirty - 
       * \return HRESULT - the success or error code for the result of the operation.
       */
      HRESULT SaveIndexBlocking( bool saveOnlyIfDirty );

      //
      // Game Board Tasks
      //

      /**
       * Delete the game board container and all its blobs
       *
       * \note This function is synchronous and could block. It's meant to be called from within an async task running
       *       on the thread pool.
       * 
       * \param uint32_t boardNumber - the board to delete.
       * \return HRESULT - the success or error code for the result of the operation.
       */
      HRESULT DeleteBlocking( uint32_t boardNumber );

      /**
       * Delete only the game board blobs, not the container (for developer education only, not relevant to game play)
       * 
       * \note This function is synchronous and could block. It's meant to be called from within an async task running
       *       on the thread pool.
       * 
       * \param uint32_t boardNumber - the board to delete.
       * \return HRESULT - the success or error code for the result of the operation.
       */
      HRESULT DeleteBlobsBlocking( uint32_t boardNumber );

      /**
       * Load the game board from the local game save cache
       * 
       * \note This function is synchronous and could block. It's meant to be called from within an async task running
       *       on the thread pool.
       *
       * \param uint32_t boardNumber - the board to read.
       * \return HRESULT - the success or error code for the result of the operation.
       */
      HRESULT ReadBlocking( uint32_t boardNumber );

      /**
       * Save the game board for uploading to the cloud. This function is synchronous and could block.
       *
       * \note This function is synchronous and could block. It's meant to be called from within an async task running
       *       on the thread pool.
       *
       * \param uint32_t boardNumber - the board to save.
       * \param SaveMode mode - the SaveMode to use when performing the save.
       * \return HRESULT - the success or error code for the result of the operation.
       */
      HRESULT SaveBlocking( uint32_t boardNumber, SaveMode mode );

      /**
       * The internals of the save call, without the checks for whether or not the board is dirty. Called by both
       * Save and SaveBlocking.
       *
       * \param uint32_t boardNumber - the board to save.
       * \return HRESULT - the success or error code for the result of the operation.
       */
      HRESULT SaveInternal( uint32_t boardNumber );

      /**
       * Marks the current active game board as dirty (its state has changed, and it needs to be saved).
       */
      void MarkActiveBoardDirty();

      //
      // GameSaveManager Public Properties
      //

      /**
       * Returns a reference to the GameSaveProvider handle currently in use.
       * \return ATG::GameSaveProviderHandle& - returns an ATG::GameSaveProviderHandle RAII wrapper around the 
       *                                        XGameSaveProviderHandle.
       */
      ATG::GameSaveProviderHandle& GetGameSaveProvider() noexcept;

      /**
       * Returns true if we have an active board, false if not.
       */
      bool HasActiveBoard() const noexcept;

      /**
       * Returns true after the sync on demand mode has been set during initialization (this can only be set once per
       * title session - typically a game will pick one mode or another to operate in).
       */
      static bool HasSyncOnDemandModeBeenSet() noexcept;

      
      /**
       * Returns true once the initialization process has completed (regardless of whether any load operation was 
       * successful).
       */
      bool IsInitialized() const noexcept
      {
         return m_isInitialized;
      }

      // Returns true if a sync-on-demand context was specified, or false if a full-sync context was specified (once set, cannot be changed until next app launch)
      /**
       * Returns true if a sync-on-demand context was specified, or false if a full-sync context was specified (once set, cannot be changed until next app launch)
       * \return bool -
       */
      SyncMode GetSyncMode() const noexcept;

      /**
       * Sets the sync mode for the sample.
       * 
       * This value can only be set once per run of the sample - i.e. when GameSaveManager::HasSyncOnDemandModeBeenSet 
       * returns false.
       * 
       * \param SyncMode value - the synchronization mode to demonstrate in the sample.
       */
      void SetSyncMode( SyncMode value );

      /**
       * Returns the update count for the index object.
       * 
       * \return uint32_t - the index update count.
       */
      uint32_t GetIndexUpdateCount() const noexcept;

      /**
       * Obtains a reference to the active game board.
       * 
       * \return GameSaveSample::GameBoard& - a reference to an instance of a GameSaveSample::GameBoard object.
       */
      GameBoard& GetActiveBoard();

      /**
       * Gets a shared pointer to the current active game board. If there is no current active board, an empty
       * shared_ptr will be returned.
       *
       * \example You can check if the shared_ptr is empty with the following code:
       * \code{.cpp}
       * auto board = GetActiveBoardGameSave();
       * if ( board )
       * {
       *    // The shared_ptr is not empty and contains a valid game board.
       * }
       * \endcode
       * 
       * \return std::shared_ptr<GameSave<GameSaveSample::GameBoard>> -
       */
      std::shared_ptr<GameSave<GameBoard>> GetActiveBoardGameSave() const;

      /**
       * Returns the active board number. A value of zero indicates that there is no active game board.
       * 
       * \note Valid game board numbers are 1-based.
       * 
       * \return uint32_t - the active game board number.
       */
      uint32_t GetActiveBoardNumber() const noexcept;

      /**
       * Sets the active game board to the specified board number. 
       *
       * This sets the active game board to a different board, saving the existing active board's state if it is dirty.
       * 
       * \note This is an asynchronous task and is performed on the thread pool via an ATG::AsyncAction<void> object.
       *
       * \param uint32_t boardNumber - the number of the game board to make active.
       */
      void SetActiveBoardNumberAsync( uint32_t boardNumber );

      /**
       * This function performs the work for SetActiveBoardNumberAsync. 
       * 
       * This is a blocking function which sets the active game board to a different board, saving the existing active 
       * board's state if it is dirty.
       * 
       * \param uint32_t boardNumber - the number of the board to make active.
       */
      void SetActiveBoardNumberBlocking( uint32_t boardNumber );

      /**
       * Returns true if the active game board is dirty.
       */
      bool IsActiveBoardDirty() const noexcept;


      /**
       * Returns true if the save index metadata is dirty.
       */
      bool IsSaveIndexDirty() const noexcept;


      /**
       * Returns the internal cached value for the remaining save game free space on the service.
       * 
       * \return int64_t - the number of bytes that can be used for adding save game data.
       */
      int64_t GetRemainingQuotaInBytes() const noexcept;

   private:

      /**
       * This is the instance-method of the container info callback used by LoadContainerMetadataBlocking. It is called
       * in response to enumerating the save game containers, or when querying a specific container.
       * 
       * \param const XGameSaveContainerInfo * info - the metadata for this callback, valid for the duration of the 
       *                                              callback.
       * \param bool queryBlobs - true if the call to LoadContainerMetadataBlocking requested that we query blobs for 
       *                          each container.
       * \return HRESULT - the success or error result code for the entire operation.
       */
      HRESULT ContainerInfoCallback( const XGameSaveContainerInfo* info, bool queryBlobs );


      /**
       * This is the static callback entrypoint for the blob metadata enumeration callback used by 
       * LoadContainerMetadataBlocking.
       * 
       * \param _In_ const XGameSaveBlobInfo * info - the blob metadata returned by the callback, valid for the duration 
       *                                              of the callback.
       * \param _In_opt_ void * context - the EnumerationMetadata container the results will be filled into. 
       *                                  (\see EnumerationMetadata).
       * \return bool - true if enumeration should continue, false if it should stop.
       */
      static bool CALLBACK EnumerateBlobInfoCallback( _In_ const XGameSaveBlobInfo* info, 
                                                                            _In_opt_ void* context );

      /**
       * Writes the container metadata out to the display log, debug output, and the log file (if enabled).
       * 
       * \param bool listBlobs - if true, lists all of the blobs in each container.
       * \param const GameSaveContainerMetadata & containerMetadata - the container to emit diagnostic information about.
       */
      void WriteContainerMetadataToDisplayLog( bool listBlobs, const GameSaveContainerMetadata& containerMetadata );


      /**
       * Closes the GameSave provider if the handle is stale (that is the HRESULT from an operation 
       * was E_GS_HANDLE_EXPIRED).
       * 
       * \param HRESULT hr - the HRESULT to check to see if it's a stale provider error.
       */
      void CloseProviderIfStale( HRESULT hr );

      /**
       *	An RAII wrapper around the XGameSaveProviderHandle for this session.
       */
      ATG::GameSaveProviderHandle                             m_gameSaveProvider;

      /**
       *	A shared pointer to the current active user we're tracking game saves for.
       */
      std::shared_ptr<ATG::User>                              m_user;

      /**
       *	Whether or not the game save has been initialized for this user's session.
       */
      bool                                                    m_isInitialized;

      /**
       *	The game save manager object-wide lock.
       */
      mutable std::mutex                                      m_mutex;

      /**
       *	The SCID for this title, used to identify it in the Xbox Live backend and access its storage there.
       */
      const char*                                             m_scid;

      /**
       *	If true, the title is currently in the middle of suspending.
       */
      bool                                                    m_isSuspending;

      /**
       *	The synchronization mode to demonstrate in the sample for this session.
       */
      SyncMode                                                m_syncMode;
      
      /**
       *	The cached current value of the user's save-game quota for this title.
       */
      int64_t                                                 m_remainingQuotaInBytes;

      /**
       *	A unique_ptr for the GameBoardIndex object, which tracks game save metadata.
       */
      std::unique_ptr<GameSave<GameBoardIndex>>               m_gameBoardIndex;

      /**
       *	All of the valid game boards for this user.
       */
      std::vector<std::shared_ptr<GameSave<GameBoard>>>       m_gameBoardSaves;

      /**
       *	If true, the synchronization mode for the title has been set at least once, and can no longer be changed.
       */
      static bool                                             s_hasBeenInitialized;

      /**
       *	Tracking object for the async task kicked off by InitializeForUser. 
       *
       * \note It is an error for this to be destroyed before any running task inside it is complete.
       */
      ATG::AsyncAction< HRESULT > m_asyncInitializeForUserTask;
   };
}
