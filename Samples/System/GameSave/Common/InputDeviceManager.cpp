//--------------------------------------------------------------------------------------
// InputDeviceManager.cpp
//
// Implementation of Input Device Manager
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
#include <GameInput.h>
#include "GamePad.h"
#include "..\Helpers\ScopedLockWrappers.h"
#include "InputDeviceManager.h"
#include "..\Helpers\XUserHandleWrapper.h"
#include <vector>

static SRWLOCK s_inputDevMgrInitLock = SRWLOCK_INIT;
static IGameInput* s_gameInput = nullptr;

namespace
{
   // Struct that keeps track of:
   //  - GamePads we know about
   //  - Users that are bound to them
   //  - The app-local ID of the GamePad

   struct GamePadData
   {
      XUserLocalId boundUserId = {};
      Microsoft::WRL::ComPtr< IGameInputDevice > device;
      APP_LOCAL_DEVICE_ID id = {};

      // This constructor is pretty much only used when creating instances inside custom containers, which should
      // mostly be using emplace_back anyway.
      GamePadData()
      {
      }

      GamePadData( IGameInputDevice* pDevice )
      {
         assert( pDevice != nullptr && "Device should not be null" );
         device = pDevice;

         if ( pDevice )
         {
            auto info = pDevice->GetDeviceInfo();

            // Get copy of device ID
            memcpy( &id, &info->deviceId, sizeof( APP_LOCAL_DEVICE_ID ) );

            // Find any bound user.
            ATG::UserHandle user;
            HRESULT hr = XUserFindForDevice( &id, user.GetPtrToHandle() );
            if ( SUCCEEDED( hr ) )
            {
               boundUserId = user.GetLocalId();
            }
            else if ( hr == E_GAMEUSER_USER_NOT_FOUND )
            {
               // Couldn't find matching gamepad.
               boundUserId = {};
            }
            else
            {
               // This shouldn't happen.
               DX::ThrowIfFailed( hr );
            }
         }
      }

      ~GamePadData()
      {
      }

      GamePadData( GamePadData&& moveFrom )
      {
         *this = std::move( moveFrom );
      }

      GamePadData& operator=( GamePadData&& moveFrom )
      {
         boundUserId = moveFrom.boundUserId;
         moveFrom.boundUserId = {};
         device.Swap( moveFrom.device );
         id = moveFrom.id;
         moveFrom.id = {};
         return *this;
      }

      GamePadData( GamePadData& copyFrom ) = delete;
      GamePadData& operator=( GamePadData& copyFrom ) = delete;

      void Reset() noexcept
      {
         boundUserId = {};
         device.Reset();
         id = {};
      }
     
      bool HasGamePad() const noexcept
      {
         return (bool) device;
      }

      bool HasBoundUser() const noexcept
      {
         return boundUserId.value != 0;
      }

      bool operator!=( _In_ IGameInputDevice const * const rhs ) const noexcept
      {
         return device.Get() != rhs;
      }

      bool operator!=( _In_ const APP_LOCAL_DEVICE_ID& rhs ) const noexcept
      {
         return memcmp( &id, &rhs, sizeof( APP_LOCAL_DEVICE_ID ) ) != 0;
      }

      bool operator!= ( _In_ const XUserLocalId rhs ) const noexcept
      {
         return boundUserId.value != rhs.value;
      }

      bool operator==( _In_ IGameInputDevice const * const rhs ) const noexcept
      {
         return device.Get() == rhs;
      }

      bool operator==( _In_ const APP_LOCAL_DEVICE_ID& rhs ) const noexcept
      {
         return memcmp( &id, &rhs, sizeof( APP_LOCAL_DEVICE_ID ) ) == 0 ;
      }

      bool operator== ( _In_ const XUserLocalId rhs ) const noexcept
      {
         return boundUserId.value == rhs.value;
      }

      operator bool() const noexcept
      {
         return (bool) device;
      }
      
   };

   enum class ConnectionStateChanged
   {
      Disconnected = 0,
      Connected = GameInputDeviceConnected,
      Noop = GameInputDeviceAnyStatus // Use this value for "Noop" here as it's convenient.
   };

   ConnectionStateChanged DeviceStateToConnectionState( GameInputDeviceStatus currentState, GameInputDeviceStatus previousState )
   {
      currentState = currentState & GameInputDeviceConnected;
      previousState = previousState & GameInputDeviceConnected;
      assert( currentState != previousState && "Shouldn't be getting no-op state-change notifications" );

      if ( currentState > previousState )
      {
         return ConnectionStateChanged::Connected;
      }
      else if ( previousState > currentState )
      {
         return ConnectionStateChanged::Disconnected;
      }
      else
      {
         return ConnectionStateChanged::Noop; // We should NEVER return this.
      }
   }

   class GamePadCollection
   {
      static constexpr size_t MaxTypicalDeviceCount = 8; // Used only to determine initial size of vector to reduce reallocs.

      mutable SRWLOCK lock = SRWLOCK_INIT;

      enum class State
      {
         Uninitialized,
         InitialEnumeration,
         Ready,
         ShuttingDown,
         ShutDown
      };

      XTaskQueueHandle taskQueue;
      std::atomic<int> deviceCount;
      std::atomic<State> state;
      std::vector< GamePadData > devices;
      GameInputCallbackToken devEnumAndConnectionToken;
      XTaskQueueRegistrationToken udaToken;

      std::recursive_mutex connectionChangedEventMutex;
      std::function<ATG::GamePadConnectionChangedCallbackFn> connectionChangedCallbackFn;

      std::recursive_mutex associationChangedEventMutex;
      std::function<ATG::GamePadAssociationChangedCallbackFn> associationChangedCallbackFn;

   public:
      GamePadCollection()
         : taskQueue{},
           deviceCount(0),
           state(State::Uninitialized),
           devices( MaxTypicalDeviceCount ),
           devEnumAndConnectionToken{},
           udaToken{} { }

      GamePadCollection(const GamePadCollection&) = delete;
      GamePadCollection& operator=(const GamePadCollection&) = delete;

      ~GamePadCollection()
      {
         if ( state.load() > State::Uninitialized && state.load() < State::ShuttingDown )
         {
            //NOTE: Should have called this before we get here - this should only be reached on program exit.
            Shutdown();
         }
      }

      void Initialize( XTaskQueueHandle taskQueueHandle )
      {
         assert( state.load() == State::Uninitialized );

         ATG::ScopedExclusiveLock exclusive( lock );
         
         // Subscribe to User-Device Association change notifications 
         // (we'll ignore them until we're in the InitialEnumeration state).
         taskQueue = taskQueueHandle;

         DX::ThrowIfFailed(
            XUserRegisterForDeviceAssociationChanged( taskQueue, static_cast<void*>( this ), &UDAChangedCallbackStatic, 
                                                      &udaToken )
         );

         // Enumerate initial list of devices and hookup for connect/disconnect notifications.
         state = State::InitialEnumeration;

         DX::ThrowIfFailed( s_gameInput->RegisterDeviceCallback(
            nullptr,                      // Want all devices, not just one specific one.
            GameInputKindGamepad,         // NOTE: ONLY GameInputKindGamepad is supported right now by preview API (6/11/2019)
            GameInputDeviceConnected,     // Subscribe to connection-change notifications.
            GameInputBlockingEnumeration, // We want to enumerate Gamepads AND sign-up for events
            static_cast<void*>(this),     // We're bouncing callbacks to our object instance, so pass in this to let us do that
            &DeviceCallbackStatic,        // Static callback function that does the translation to object instance method
            &devEnumAndConnectionToken    // token for this callback registration
         ) );

         state = State::Ready;
      }

      void SubscribeGamePadConnectionChangedEvent( std::function<ATG::GamePadConnectionChangedCallbackFn>& gamepadConnectionChangedCallback ) noexcept
      {
         std::lock_guard<std::recursive_mutex> lockEvent( connectionChangedEventMutex );
         assert( !connectionChangedCallbackFn && "Already subscribed to Connection Changed event" );

         connectionChangedCallbackFn = gamepadConnectionChangedCallback;
      }

      void UnsubscribeGamePadConnectionChangedEvent() noexcept
      {
         std::lock_guard<std::recursive_mutex> lockEvent( connectionChangedEventMutex );
         assert( !!connectionChangedCallbackFn && "Not subscribed to Connection Changed event" );

         connectionChangedCallbackFn = nullptr;
      }

      void NotifyGamePadConnectionChangedEvent( ATG::GamePadConnectionChanged& change )
      {
         //NOTE: The exclusive lock on this object should not be held at this point.
         std::function<ATG::GamePadConnectionChangedCallbackFn> tempCopy;

         {
            std::lock_guard<std::recursive_mutex> lockEvent( connectionChangedEventMutex );
            tempCopy = connectionChangedCallbackFn;
         }

         if ( tempCopy )
         {
            tempCopy( change );
         }
      }

      void SubscribeGamePadAssociationChangedEvent( std::function< ATG::GamePadAssociationChangedCallbackFn >& gamepadAssociationChangedCallback ) noexcept
      {
         std::lock_guard<std::recursive_mutex> lockEvent( associationChangedEventMutex );
         assert( !associationChangedCallbackFn && "Already subscribed to Connection Changed event" );

         associationChangedCallbackFn = gamepadAssociationChangedCallback;
      }

      void UnsubscribeGamePadAssociationChangedEvent() noexcept
      {
         std::lock_guard<std::recursive_mutex> lockEvent( associationChangedEventMutex );
         assert( !!associationChangedCallbackFn && "Not subscribed to Connection Changed event" );

         associationChangedCallbackFn = nullptr;
      }

      void NotifyGamePadAssociationChangedEvent( ATG::GamePadAssociationChanged& change )
      {
         //NOTE: The exclusive lock on this object should not be held at this point.

         std::function<ATG::GamePadAssociationChangedCallbackFn> tempCopy;
         {
            std::lock_guard<std::recursive_mutex> lockEvent( associationChangedEventMutex );
            tempCopy = associationChangedCallbackFn;
         }

         if ( tempCopy )
         {
            tempCopy( change );
         }
      }

      void Shutdown()
      {
         ATG::ScopedExclusiveLock exclusive( lock );

         assert( state.load() < State::ShuttingDown && "Already shutting down" );
         assert( state.load() > State::Uninitialized && "Not initialized" );
         
         state = State::ShuttingDown;

         // Unregister the UDA Change callback.
         if ( !XUserUnregisterForDeviceAssociationChanged( udaToken, true ) )
         {
            // Couldn't unregister. Shouldn't ever happen.
            DX::ThrowIfFailed(E_FAIL);
         }

         // Unregister the connection/disconnect callback.

         constexpr uint64_t TimeoutMicroseconds = 30000;
         if ( !s_gameInput->UnregisterCallback( devEnumAndConnectionToken, TimeoutMicroseconds ) )
         {
            // Unregistering the callback failed.
            DX::ThrowIfFailed( E_UNEXPECTED );
         }
         devEnumAndConnectionToken = 0;

         state = State::ShutDown;
      }

      ATG::GamePadUserBindingInfo GetGamePadAndUserByIndex( _In_ int playerIndex )
      {
         XUserLocalId userId = {};

         IGameInputDevice* gamePad = nullptr;

         ATG::ScopedSharedLock shared( lock );
         if ( playerIndex >= 0 && playerIndex < int(devices.size()) )
         {
            GamePadData& data = devices[ size_t(playerIndex) ];
            if ( data.HasGamePad() )
            {
               userId = data.boundUserId;
               gamePad = data.device.Get();
            }
         }
         return ATG::GamePadUserBindingInfo( playerIndex, userId, gamePad );
      }

      bool FindGamePadAndUserByUserId( _In_ XUserLocalId userId, _In_ ATG::GamePadUserBindingInfo* userPair )
      {
         int index = FindIndex( userId );

         if ( index != -1 )
         {
            *userPair = ATG::GamePadUserBindingInfo( index, userId, devices[ size_t(index) ].device.Get() );
         }
         else
         {
            *userPair = ATG::GamePadUserBindingInfo( -1, { 0 }, nullptr );
         }

         return index != -1;
      }

   private:

      void InitialDeviceEnumerationCallback( _In_ IGameInputDevice* device ) noexcept
      {
         // NOTE: Use InternalAdd, because our caller will have an exclusive lock already. This also means
         // we only need to take the lock once per round of enumeration.
         InternalAdd( device );
      }

      void OnGamepadConnectionChange( _In_ IGameInputDevice* device, _In_ ConnectionStateChanged stateChange ) noexcept
      {
         ATG::GamePadConnectionChanged data;

         if ( stateChange == ConnectionStateChanged::Connected )
         {
            data = Add( device );
         }
         else
         {
            data = Remove( device );
         }

         NotifyGamePadConnectionChangedEvent( data );
      }

      void OnUDAChanged( const XUserDeviceAssociationChange* change ) noexcept
      {
         assert( change != nullptr && "Bad change data." );

         if ( state.load() != State::Ready ) // Ignore pre-enumeration callbacks.
            return;
         
         ATG::GamePadAssociationChanged gpac;

         {
            ATG::ScopedExclusiveLock exclusive( lock );

            // #FUTURE: On PC, the device ID can be null.  If deviceId is null the user change applies to all devices 
            //          (this is the behavior on Windows where all devices are associated to the signed in user).

            int index = InternalFindIndex( change->deviceId );

            // This can happen from using the XBOM virtual controller
            if (index == -1)
            {
                return;
            }

            auto& device = devices[ size_t(index) ];

            assert( device == change->oldUser && "Controller was not bound to the previous user we expected - mismatch between system and our view" );

            device.boundUserId = change->newUser;

            Log::Write(u8" - InputDeviceManager::OnUDAChanged: device with index [%d] associated with user %d\n", index, device.boundUserId );

            gpac = ATG::GamePadAssociationChanged( device.device.Get(), index, change->oldUser, change->newUser, &device.id );
            // release lock
         }

         NotifyGamePadAssociationChangedEvent( gpac );
      }

      ATG::GamePadConnectionChanged InternalAdd( _In_ IGameInputDevice* device ) noexcept
      {
         // NOTE: This function does not take a lock.

         assert( device != nullptr && "Can't add a null device" );

#if _DEBUG

         // DEBUG ONLY: Verify that the device isn't already in the list (it shouldn't be).

         for ( size_t i = 0; i < devices.size(); ++i )
         {
            // NOTE: IGameInputDevice pointers can be compared for equality - see documentation.

            assert( devices[ i ] != device && "Shouldn't ever add the same device multiple times" );
         }

#endif // _DEBUG

         // Find the first empty slot.

         int emptySlot = -1;
         for ( size_t i = 0; i < devices.size(); ++i )
         {
            // Cache the first empty slot.
            if ( !devices[i].HasGamePad() )
            {
               emptySlot = int(i);
               break;
            }
         }

         if ( emptySlot == -1 )
         {
            emptySlot = (int)devices.size();
            devices.emplace_back( device );
         }
         else
         {
            GamePadData g( device );
            devices[ size_t(emptySlot) ] = std::move( g );
         }

         Log::Write( u8"GamePad Device Connected at index [%d]: ID=%s\n", emptySlot,
            FormatControllerId( devices[ size_t(emptySlot) ].id ).c_str() );


         int count = ++deviceCount;

         Log::Write( u8"%d controller(s) now connected\n", count );

         return ATG::GamePadConnectionChanged(
            ATG::GamePadConnectionState::Connected, devices[ size_t(emptySlot) ].device.Get(), emptySlot,
            devices[ size_t(emptySlot) ].boundUserId, &devices[ size_t(emptySlot) ].id
         );
      }

      ATG::GamePadConnectionChanged InternalRemove( _In_ IGameInputDevice* device ) noexcept
      {
         // NOTE: This function does not take a lock.
         // NOTE: IGameInputDevice pointers can be compared for equality - see documentation.

         int index = InternalFindIndex( device );
         assert( index != -1 && "Attempted to remove item that wasn't in the list" );

         GamePadData g = std::move( devices[ size_t(index) ] ); // note - resets the item we're moving from.

         Log::Write( u8"GamePad Device Disconnected from Index [%d]: ID=%s\n", index, FormatControllerId( g.id ).c_str() );

         int count = --deviceCount;
         
         Log::Write( u8"%d controller(s) connected\n", count );

         return ATG::GamePadConnectionChanged( ATG::GamePadConnectionState::Disconnected, g.device.Get(), index, 
                                               g.boundUserId, &g.id );
      }

      int InternalFindIndex( _In_ IGameInputDevice* device ) const noexcept
      {
         for ( size_t i = 0; i < devices.size(); ++i )
         {
            // NOTE: IGameInputDevice pointers can be compared for equality - see documentation.

            if ( devices[ i ] == device )
            {
               return int(i);
            }
         }
         return -1;
      }

      int InternalFindIndex( _In_ const APP_LOCAL_DEVICE_ID& id ) const noexcept
      {
         for ( size_t i = 0; i < devices.size(); ++i )
         {
            if ( devices[ i ] == id )
            {
               return int(i);
            }
         }
         return -1;
      }

      int InternalFindIndex( _In_ XUserLocalId id ) const noexcept
      {
         for ( size_t i = 0; i < devices.size(); ++i )
         {
            if ( devices[ i ] == id )
            {
               return int(i);
            }
         }
         return -1;
      }


      static void CALLBACK DeviceCallbackStatic(
         _In_ GameInputCallbackToken callbackToken,
         _In_ void * context,
         _In_ IGameInputDevice * device,
         _In_ uint64_t timestamp,
         _In_ GameInputDeviceStatus currentStatus,
         _In_ GameInputDeviceStatus previousStatus
      );

      static void CALLBACK UDAChangedCallbackStatic(
         _In_opt_ void* context,
         _In_ const XUserDeviceAssociationChange* change
      );

   public:
      void Suspend()
      {
         // #NOTE: Theoretically we don't need to do anything here, but we're providing this for future use, in case
         //        that behavior changes in the future.
      }

      void Resume()
	  {
		  // #NOTE: Theoretically we don't need to do anything here, but we're providing this for future use, in case
		  //        that behavior changes in the future.
	  }

      ATG::GamePadConnectionChanged Add( _In_ IGameInputDevice* device ) noexcept
      {
         ATG::ScopedExclusiveLock exclusive( lock );
         return InternalAdd( device );
      }

      ATG::GamePadConnectionChanged Remove( _In_ IGameInputDevice* device ) noexcept
      {
         ATG::ScopedExclusiveLock exclusive( lock );
         return InternalRemove( device );
      }

      int FindIndex( _In_ IGameInputDevice* device ) const noexcept
      {
         ATG::ScopedSharedLock shared( lock );
         return InternalFindIndex( device );
      }

      int FindIndex( _In_ const APP_LOCAL_DEVICE_ID& id ) const noexcept
      {
         ATG::ScopedSharedLock shared( lock );
         return InternalFindIndex( id );
      }

      int FindIndex( _In_ XUserLocalId id ) const noexcept
      {
         ATG::ScopedSharedLock shared( lock );
         return InternalFindIndex( id );
      }

      Microsoft::WRL::ComPtr<IGameInputDevice> GetPadAtIndex( _In_ int playerIndex )
      {
         ATG::ScopedSharedLock shared( lock );

         Microsoft::WRL::ComPtr<IGameInputDevice> gamePad; // == nullptr 

         if ( playerIndex >= 0 && playerIndex < int(devices.size()) )
         {
            gamePad = devices[ size_t(playerIndex) ].device.Get();
         }
         
         return gamePad;
      }

      const GamePadData& operator[] ( const int index ) const noexcept
      {
         return devices[ size_t(index) ];
      }

      GamePadData& operator[] ( const int index ) noexcept
      {
         return devices[ size_t(index) ];
      }

      Microsoft::WRL::ComPtr< IGameInputDevice > FindGamePadForUser( _In_ XUserLocalId userId ) const noexcept
      {
         ATG::ScopedSharedLock shared( lock );
         int index = InternalFindIndex( userId );

         Microsoft::WRL::ComPtr<IGameInputDevice> device;
         if ( index != -1 )
         {
            device = devices[ size_t(index) ].device;
         }
         return device;
      }

      // Total number of current valid indices.      
      int Capacity() const noexcept
      {
         ATG::ScopedSharedLock shared( lock );
         return (int)devices.size();
      }
   };

   void CALLBACK GamePadCollection::DeviceCallbackStatic( 
      _In_ GameInputCallbackToken /*callbackToken*/,
      _In_ void * context,
      _In_ IGameInputDevice * device,
      _In_ uint64_t /*timestamp*/, // unused
      _In_ GameInputDeviceStatus currentStatus,
      _In_ GameInputDeviceStatus previousStatus )
   {
      GamePadCollection* instance = static_cast<GamePadCollection*>( context );

      if ( instance->state.load() == State::InitialEnumeration )
      {
         instance->InitialDeviceEnumerationCallback( device );
      }
      else
      {
         instance->OnGamepadConnectionChange( device, DeviceStateToConnectionState( currentStatus, previousStatus ) );
      }
   }

   void CALLBACK GamePadCollection::UDAChangedCallbackStatic(
      _In_opt_ void* context,
      _In_ const XUserDeviceAssociationChange* change )
   {
      Log::Write( u8"User Device Association changed - Device ID: %s, Old user: %llu, New user: %llu\n", FormatControllerId( change->deviceId ).c_str(), 
                  change->oldUser.value, change->newUser.value );

#ifdef VERIFY_USER_DEVICE_ASSOCIATION_VIA_REVERSE_PATH
      ATG::UserHandle handle;
      HRESULT hr = XUserFindForDevice(&(change->deviceId), handle.GetPtrToHandle());
      DX::ThrowIfFailed(hr);
      Log::Write(u8"Lookup of user for device ID = %llu, user=%s\n", handle.GetLocalId(), FormatUserName(handle).c_str());
      assert(handle.GetLocalId() == change->newUser);
#endif

      GamePadCollection* instance = static_cast<GamePadCollection*>( context );
      instance->OnUDAChanged( change );
   }

   static GamePadCollection s_gamePads;

   void InitializeInputDeviceManagerInternal( _In_opt_ XTaskQueueHandle taskQueue )
   {
      // NOTE: This function is being called with an exclusive lock held on s_inputDevMgrInitLock.

      // Initialize the GamePadCollection object. This tracks all connected game pads.
      s_gamePads.Initialize( taskQueue );
   }

   void ShutdownInputDeviceManagerInternal()
   {
      //NOTE: This function is being called with an exclusive lock held on s_inputDevMgrInitLock.

      s_gamePads.Shutdown();
   }
}

void ATG::InitializeInputDeviceManager( _In_opt_ XTaskQueueHandle taskQueue )
{
   ATG::ScopedExclusiveLock exclusive( s_inputDevMgrInitLock );
   assert( !s_gameInput && "Initialize was already called on InputDeviceManager" );
   DX::ThrowIfFailed( GameInputCreate( &s_gameInput ) );
   InitializeInputDeviceManagerInternal( taskQueue );
}

bool ATG::HasBoundGamePad( _In_ XUserLocalId userId )
{
   assert( s_gameInput != nullptr && "Not initialized" );
   return s_gamePads.FindIndex( userId ) != -1;
}

Microsoft::WRL::ComPtr< IGameInputDevice > ATG::FindGamePadForUser( _In_ XUserLocalId userId )
{
   assert( s_gameInput != nullptr && "Not initialized" );
   return s_gamePads.FindGamePadForUser( userId );
}

Microsoft::WRL::ComPtr< IGameInputDevice > ATG::GetGamePadByIndex( _In_ int playerIndex )
{
   assert( s_gameInput != nullptr && "Not initialized" );
   Microsoft::WRL::ComPtr<IGameInputDevice> gamePad;
   gamePad = s_gamePads.GetPadAtIndex( playerIndex );
   return gamePad;
}

void ATG::ShutdownInputDeviceManager()
{
   ATG::ScopedExclusiveLock exclusive( s_inputDevMgrInitLock );
   if ( s_gameInput )
   {
         ShutdownInputDeviceManagerInternal();
         s_gameInput = nullptr;
   }
}

int ATG::FindGamePadIndex( _In_ IGameInputDevice* gamepad )
{
   assert( s_gameInput != nullptr && "Not initialized" );
   return s_gamePads.FindIndex( gamepad );
}

ATG::GamePadUserBindingInfo ATG::GetGamePadAndUserByIndex( _In_ int playerIndex )
{
   assert( s_gameInput != nullptr && "Not initialized" );
   return s_gamePads.GetGamePadAndUserByIndex( playerIndex );
}

bool ATG::FindGamePadAndUserByUserId( _In_ XUserLocalId userId, _In_ ATG::GamePadUserBindingInfo* userPair )
{
   assert( s_gameInput != nullptr && "Not initialized" );
   return s_gamePads.FindGamePadAndUserByUserId( userId, userPair );
}

void ATG::SubscribeGamePadConnectionChangedEvent( std::function<GamePadConnectionChangedCallbackFn>& gamepadConnectionChangedCallback ) noexcept
{
   assert( s_gameInput != nullptr && "Not initialized" );
   s_gamePads.SubscribeGamePadConnectionChangedEvent( gamepadConnectionChangedCallback );
}

void ATG::UnsubscribeGamePadConnectionChangedEvent() noexcept
{
   assert( s_gameInput != nullptr && "Not initialized" );
   s_gamePads.UnsubscribeGamePadConnectionChangedEvent();
}

void ATG::SubscribeGamePadAssociationChangedEvent( std::function<GamePadAssociationChangedCallbackFn>& gamepadAssociationChangedCallback ) noexcept
{
   assert( s_gameInput != nullptr && "Not initialized" );
   s_gamePads.SubscribeGamePadAssociationChangedEvent( gamepadAssociationChangedCallback );
}

void ATG::UnsubscribeGamePadAssociationChangedEvent() noexcept
{
   assert( s_gameInput != nullptr && "Not initialized" );
   s_gamePads.UnsubscribeGamePadAssociationChangedEvent();
}

void ATG::SuspendInputDeviceManager()
{
   assert( s_gameInput != nullptr && "Not initialized" );
   return s_gamePads.Suspend();
}

void ATG::ResumeInputDeviceManager()
{
   assert( s_gameInput != nullptr && "Not initialized" );
   return s_gamePads.Resume();
}
