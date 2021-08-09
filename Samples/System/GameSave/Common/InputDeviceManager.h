//--------------------------------------------------------------------------------------
// InputDeviceManager.h
//
// Tracks input devices.
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

namespace ATG
{
   // The default core you should use for input device mgr work.
   constexpr uint16_t DEFAULT_INPUT_WORK_AND_CALLBACK_CORE = 3;

   // The connection event type - did the gamepad we're reporting on just connect or just disconnect?
   enum class GamePadConnectionState
   {
      None,
      Connected,
      Disconnected
   };

   // Notification data structure passed to subscribers when a GamePad connects or disconnects from the system.
   struct GamePadConnectionChanged
   {
      // The action which took place - did the gamepad just connect, or disconnect?
      GamePadConnectionState state;

      // The gamepad device we're talking about.
      Microsoft::WRL::ComPtr< IGameInputDevice > gamePad;

      // The index of the gamePad. Note that this value is valid for the entire visible lifetime of the gamepad.
      int gamePadIndex;

      // The user ID of the user associated with this gamepad. This could be a null user ID if no user is
      // associated with it.
      XUserLocalId associatedUserId;

      // The unique ID of the gamepad device.
      APP_LOCAL_DEVICE_ID gamePadId;
      
      // Default constructor
      GamePadConnectionChanged()
         : state( GamePadConnectionState::None ),
         gamePad( nullptr ),
         gamePadIndex( -1 ),
         associatedUserId( { 0 } ),
         gamePadId( { 0 } )
      {
      }

      // Constructor
      GamePadConnectionChanged( GamePadConnectionState connectionState, IGameInputDevice* gamePadDevice, int index, 
                                XUserLocalId userId, const APP_LOCAL_DEVICE_ID* padId ) : 
         state( connectionState ),
         gamePad( gamePadDevice ),
         gamePadIndex( index ),
         associatedUserId( userId )
      {
         memcpy( &gamePadId, padId, sizeof(APP_LOCAL_DEVICE_ID) );
      }

      // Move assignment operator
      GamePadConnectionChanged& operator=( GamePadConnectionChanged&& moveFrom )
      {
         if ( this != &moveFrom )
         {
            state = moveFrom.state;
            moveFrom.state = GamePadConnectionState::None;
            gamePad.Swap( moveFrom.gamePad );
            gamePadIndex = moveFrom.gamePadIndex;
            moveFrom.gamePadIndex = -1;
            associatedUserId = moveFrom.associatedUserId;
            moveFrom.associatedUserId.value = 0;
            memcpy( &gamePadId, &moveFrom.gamePadId, sizeof( APP_LOCAL_DEVICE_ID ) );
            memset( &moveFrom.gamePadId, 0, sizeof( APP_LOCAL_DEVICE_ID ) );
         }

         return *this;
      }

      // Copy assignment operator
      GamePadConnectionChanged& operator=( const GamePadConnectionChanged& copyFrom )
      {
         state = copyFrom.state;
         gamePad = Microsoft::WRL::ComPtr<IGameInputDevice>( copyFrom.gamePad.Get() );
         gamePadIndex = copyFrom.gamePadIndex;
         associatedUserId = copyFrom.associatedUserId;
         memcpy( &gamePadId, &copyFrom.gamePadId, sizeof( APP_LOCAL_DEVICE_ID ) );
		 return *this;
      }

      // Move constructor
      GamePadConnectionChanged( GamePadConnectionChanged&& moveFrom )
      {
         *this = std::move( moveFrom );
      }

      // Copy constructor
      GamePadConnectionChanged( const GamePadConnectionChanged& copyFrom )
      {
         *this = copyFrom;
      }

      // Validity test.
      operator bool() const noexcept
      {
         return ( state != GamePadConnectionState::None );
      }
   };

   // Notification data structure passed to subscribers when a User/GamePad association changes.
   struct GamePadAssociationChanged
   {
      // The gamepad device we're talking about.
      Microsoft::WRL::ComPtr< IGameInputDevice > gamePad;

      // The index slot of this gamepad. This value is valid until the gamepad is disconnected, and can be used as an
      // identifier during the gamepad's visible lifetime.
      int gamePadIndex;
      
      // The previous user that is associated with this gamepad (this could be a null user ID if the gamepad wasn't 
      // previously attached to another user).
      XUserLocalId previousUserId;
      
      // The new user that is associated with this gamepad (this could be a null user ID if the gamepad isn't being
      // attached to another user).
      XUserLocalId newUserId;

      // The device ID of the gamepad we're talking about.
      APP_LOCAL_DEVICE_ID gamePadId;

      // Default constructor
      GamePadAssociationChanged() :
         gamePad( nullptr ),
         gamePadIndex( -1 ),
         previousUserId( { 0 } ),
         newUserId( { 0 } ),
         gamePadId( { 0 } )
      {
      }

      // Constructor
      GamePadAssociationChanged( IGameInputDevice* gamePadDevice, int index, XUserLocalId prevUser, 
                                 XUserLocalId nextUser, const APP_LOCAL_DEVICE_ID* padId ) :
         gamePad( gamePadDevice ),
         gamePadIndex( index ),
         previousUserId( prevUser ),
         newUserId( nextUser )
      {
         memcpy( &gamePadId, padId, sizeof( APP_LOCAL_DEVICE_ID ) );
      }

      // Move assignment operator
      GamePadAssociationChanged& operator=( GamePadAssociationChanged&& moveFrom )
      {
         if ( this != &moveFrom )
         {
            gamePad.Swap( moveFrom.gamePad );
            gamePadIndex = moveFrom.gamePadIndex;
            moveFrom.gamePadIndex = -1;
            previousUserId = moveFrom.previousUserId;
            moveFrom.previousUserId.value = 0;
            newUserId = moveFrom.newUserId;
            moveFrom.newUserId.value = 0;
            memcpy( &gamePadId, &moveFrom.gamePadId, sizeof( APP_LOCAL_DEVICE_ID ) );
            memset( &moveFrom.gamePadId, 0, sizeof( APP_LOCAL_DEVICE_ID ) );
         }

         return *this;
      }

      // Copy assignment operator
      GamePadAssociationChanged& operator=( const GamePadAssociationChanged& copyFrom )
      {
         gamePad = Microsoft::WRL::ComPtr<IGameInputDevice>( copyFrom.gamePad.Get() );
         gamePadIndex = copyFrom.gamePadIndex;
         previousUserId = copyFrom.previousUserId;
         newUserId = copyFrom.newUserId;
         memcpy( &gamePadId, &copyFrom.gamePadId, sizeof( APP_LOCAL_DEVICE_ID ) );
		 return *this;
      }

      // Move constructor
      GamePadAssociationChanged( GamePadAssociationChanged&& moveFrom )
      {
         *this = std::move( moveFrom );
      }

      // Copy constructor
      GamePadAssociationChanged( const GamePadAssociationChanged& copyFrom )
      {
         *this = copyFrom;
      }

      // Validity test.
      operator bool() const noexcept
      {
         return (bool)gamePad;
      }
   };

   // Data structure used for passing around state for how users and gamepads are associated with each other.
   struct GamePadUserBindingInfo
   {
      // The index of the gamepad. This value is valid for the visible object lifetime of the gamepad.
      int gamePadIndex;

      // The id of the user that is associated with the gamepad. Note: this could be null if no user is associated 
      // with it.
      XUserLocalId userId;

      // A pointer to the GamePad device instance itself.
      Microsoft::WRL::ComPtr< IGameInputDevice > gamePad;

      // Default constructor
      GamePadUserBindingInfo()
         : gamePadIndex( -1 ),
           userId( { 0 } ),
           gamePad( nullptr )
      {
      }

      // Constructor
      GamePadUserBindingInfo( int player, XUserLocalId user, IGameInputDevice* gamePadDevice )
         : gamePadIndex( player ),
           userId( user ),
           gamePad( gamePadDevice )
      {
      }

      // Move constructor
      GamePadUserBindingInfo( GamePadUserBindingInfo&& moveFrom )
      {
         *this = std::move( moveFrom );
      }

      // Move assignment operator
      GamePadUserBindingInfo& operator=( GamePadUserBindingInfo&& moveFrom )
      {
         if ( this != &moveFrom )
         {
            gamePadIndex = moveFrom.gamePadIndex;
            userId = moveFrom.userId;
            gamePad.Swap( moveFrom.gamePad );
         }

         return *this;
      }

      // Copy constructor
      GamePadUserBindingInfo( const GamePadUserBindingInfo& copyFrom )
      {
         *this = copyFrom;
      }

      // Copy assignment operator
      GamePadUserBindingInfo& operator=( const GamePadUserBindingInfo& copyFrom )
      {
         gamePadIndex = copyFrom.gamePadIndex;
         userId = copyFrom.userId;
         gamePad = Microsoft::WRL::ComPtr<IGameInputDevice>( copyFrom.gamePad.Get() );
		 return *this;
      }

      // Validity check.
      operator bool() const noexcept
      {
         return (bool)gamePad;
      }
   };

   // Initialize the Input Device manager, specifying the Asynchronous Task Queue to use for its work and callbacks.
   void InitializeInputDeviceManager( _In_opt_ XTaskQueueHandle taskQueue );

   // Shutdown the Input Device Manager
   void ShutdownInputDeviceManager();

   // Given an IGameInputDevice* gamepad pointer, find the associated index of the gamepad. Indices are valid for the
   // lifetime of a gamepad's connection (from connection to disconnection).
   int FindGamePadIndex( _In_ IGameInputDevice* gamepad );

   // Checks if a user has an associated gamepad.
   bool HasBoundGamePad( _In_ XUserLocalId userId );

   // Finds a Gamepad for a user id, if there is one.
   Microsoft::WRL::ComPtr< IGameInputDevice > FindGamePadForUser( _In_ XUserLocalId userId );

   // Returns the gamepad at a given index.
   Microsoft::WRL::ComPtr< IGameInputDevice > GetGamePadByIndex( _In_ int playerIndex );

   // Returns the GamePad and other user binding information for a given index.
   GamePadUserBindingInfo GetGamePadAndUserByIndex( _In_ int playerIndex );

   // Given an XUserLocalId, obtains Gamepad and other user binding information for that user. Returns true if
   // there was a gamepad associated with that id, false otherwise.
   bool FindGamePadAndUserByUserId( _In_ XUserLocalId userId, _Out_ GamePadUserBindingInfo* userPair );

   // The callback function signature to use for GamePad Connection Changed notifications.
   //
   // Example usage:
   // ```cpp
   // void MyFunction( const ATG::GamePadConnectionChanged& status ) { 
   // (void)status; // Do something with it.
   // }
   // 
   // void SubscribeGamePadConnectionChanged()
   // {
   //    std::function<GamePadConnectionChangedCallbackFn> fn = std::bind( &MyFunction, this, std::placeholders::_1 );
   //    ATG::SubscribeGamePadConnectionChangedEvent( fn );
   // }
   // ```
   using GamePadConnectionChangedCallbackFn = void( const ATG::GamePadConnectionChanged& status );
   
   // Subscribes to GamePad connection-changed notifications (i.e. connection and disconnection events). NOTE: You can
   // only have one subscriber at a time with this implementation.
   // 
   // NOTE: This event fires after the controller has been connected or disconnected (and added to or removed from the list of gamepads).
   void SubscribeGamePadConnectionChangedEvent( std::function<GamePadConnectionChangedCallbackFn>& gamepadConnectionChangedCallback ) noexcept;

   // Unsubscribe from Connection Changed events.
   void UnsubscribeGamePadConnectionChangedEvent() noexcept;

   // The callback function signature to use for Gamepad/User Association change notifications.
   //
   // Example usage:
   // ```cpp
   // void MyFunction( const ATG::GamePadAssociationChanged& status ) {
   //    (void) status; // Do something with it.
   // }
   // 
   // void SubscribeGamePadAssociationChanged()
   // {
   //    std::function<GamePadAssociationChangedCallbackFn> fn = std::bind( &MyFunction, this, std::placeholders::_1 );
   //    ATG::SubscribeGamePadAssociationChangedEvent( fn );
   // }
   // ```
   using GamePadAssociationChangedCallbackFn = void( const ATG::GamePadAssociationChanged& status );

   // Subscribes to GamePad/User association change notifications.  NOTE: You can only have one subscriber at a time 
   // with the current implementation.
   //
   // NOTE: This event fires after the controller has been associated with the new user.
   void SubscribeGamePadAssociationChangedEvent( std::function<GamePadAssociationChangedCallbackFn>& gamepadAssociationChangedCallback ) noexcept;

   // Unsubscribe from GamePad Association changed events.
   void UnsubscribeGamePadAssociationChangedEvent() noexcept;

   // Call when the title is being suspended. We recommend you do this after suspending any other systems which depend
   // on this one - e.g. UserManagers.
   void SuspendInputDeviceManager();
   
   // Call when the title is being resumed. We recommend that you do this before resuming any other systems which depend
   // on this one - e.g. UserManagers.
   void ResumeInputDeviceManager();
}
