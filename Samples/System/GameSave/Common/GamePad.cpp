//--------------------------------------------------------------------------------------
// File: GamePad.cpp
//
// Gaming.*.x64-specific GamePad implementation.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "..\pch.h"
#include <XUser.h>
#include <GameInput.h>
#include "GamePad.h"
#include "InputDeviceManager.h"

using namespace ATG;
using Microsoft::WRL::ComPtr;

namespace
{
   const float c_XboxOneThumbDeadZone = .24f;  // Recommended Xbox One controller deadzone

   float ApplyLinearDeadZone( float value, float maxValue, float deadZoneSize )
   {
      if ( value < -deadZoneSize )
      {
         // Increase negative values to remove the deadzone discontinuity.
         value += deadZoneSize;
      }
      else if ( value > deadZoneSize )
      {
         // Decrease positive values to remove the deadzone discontinuity.
         value -= deadZoneSize;
      }
      else
      {
         // Values inside the deadzone come out zero.
         return 0;
      }

      // Scale into 0-1 range.
      float scaledValue = value / ( maxValue - deadZoneSize );
      return std::max( -1.f, std::min( scaledValue, 1.f ) );
   }

   void ApplyStickDeadZone( float x, float y, GamePad::DeadZone deadZoneMode, float maxValue, float deadZoneSize,
      _Out_ float& resultX, _Out_ float& resultY )
   {
      switch ( deadZoneMode )
      {
      case GamePad::DEAD_ZONE_INDEPENDENT_AXES:
         resultX = ApplyLinearDeadZone( x, maxValue, deadZoneSize );
         resultY = ApplyLinearDeadZone( y, maxValue, deadZoneSize );
         break;

      case GamePad::DEAD_ZONE_CIRCULAR:
      {
         float dist = sqrtf( x*x + y * y );
         float wanted = ApplyLinearDeadZone( dist, maxValue, deadZoneSize );

         float scale = ( wanted > 0.f ) ? ( wanted / dist ) : 0.f;

         resultX = std::max( -1.f, std::min( x * scale, 1.f ) );
         resultY = std::max( -1.f, std::min( y * scale, 1.f ) );
      }
      break;

      case GamePad::DEAD_ZONE_NONE:
      default:
         resultX = ApplyLinearDeadZone( x, maxValue, 0 );
         resultY = ApplyLinearDeadZone( y, maxValue, 0 );
         break;
      }
   }
}

//======================================================================================
// GameInput
//======================================================================================

#include <GameInput.h>

class GamePad::Impl
{
public:
   Impl( GamePad* owner ) :
      mOwner( owner ),
      mCtrlChanged( INVALID_HANDLE_VALUE ),
      mUserChanged( INVALID_HANDLE_VALUE )
   {
      if ( s_gamePad )
      {
         throw std::exception( "GamePad is a singleton" );
      }

      s_gamePad = this;

      DX::ThrowIfFailed( GameInputCreate( mGameInput.GetAddressOf() ) );
   }

   ~Impl()
   {
      s_gamePad = nullptr;
   }

   void GetState( int playerIndex, _Out_ State& state, DeadZone deadZoneMode )
   {
      ComPtr<IGameInputDevice> gamePad;
      ComPtr<IGameInputReading> reading;

      HRESULT hr = S_OK;

      if ( playerIndex != ATG::GamePad::MERGED_CONTROLLER_INPUT_PLAYER_INDEX )
      {
         // Handle a specific GamePad for a user.

         gamePad = ATG::GetGamePadByIndex( playerIndex );
         if (!gamePad)
         {
            ::ZeroMemory(&state, sizeof(State));
            // state.connected will be false after zeroing out.
            return;
         }
      }
      
      hr = mGameInput->GetCurrentReading(GameInputKindGamepad, gamePad.Get(), reading.GetAddressOf());
      
      DX::ThrowIfFailed( hr );

      TranslateReading( state, nullptr, reading.Get(), deadZoneMode );
   }

   void GetCapabilities( int playerIndex, _Out_ Capabilities& caps )
   {
      memset( &caps, 0, sizeof( Capabilities ) );

      if ( playerIndex == ATG::GamePad::MERGED_CONTROLLER_INPUT_PLAYER_INDEX )
      {
         // Hard-coded capabilities for now
         ComPtr<IGameInputReading> reading;
         if ( SUCCEEDED( mGameInput->GetCurrentReading( GameInputKindGamepad, nullptr, reading.GetAddressOf() ) ) )
         {
            GameInputGamepadState pad;
            if ( reading->GetGamepadState( &pad ) )
            {
               caps.connected = true;
               caps.gamepadType = Capabilities::GAMEPAD;
            }
         }
      }
      else
      {
         // Read user-bound capabilities
         Microsoft::WRL::ComPtr< IGameInputDevice > device = ATG::GetGamePadByIndex( playerIndex );
         if ( !device )
         {
            return;
         }
         else
         {
            GameInputDeviceStatus status = device->GetDeviceStatus();
            caps.connected = status & GameInputDeviceConnected;

            auto info = device->GetDeviceInfo();
            memcpy( &caps.id, &( info->deviceId ), sizeof( APP_LOCAL_DEVICE_ID ) );
            caps.gamepadType = Capabilities::GAMEPAD;
            //NOTE: We only ever get gamepads at this time, so we don't need to decode the other device info.
            //      This may change in future.
         }
      }
   }

   bool SetVibration( int /*player*/, float /*leftMotor*/, float /*rightMotor*/, float /*leftTrigger*/, float /*rightTrigger*/ )
   {
      // The GameInput API doesn't support force-feedback yet.
      
      return false;
   }

   void Suspend()
   {
   }

   void Resume()
   {
   }

   GamePad*    mOwner;

   static GamePad::Impl* s_gamePad;

   HANDLE mCtrlChanged;
   HANDLE mUserChanged;

   Microsoft::WRL::ComPtr<IGameInput> mGameInput;

private:
   static void TranslateReading( ATG::GamePad::State& state, IGameInputDevice* gamePad, IGameInputReading* reading, 
                                 GamePad::DeadZone deadZoneMode );
};

GamePad::Impl* GamePad::Impl::s_gamePad = nullptr;

//======================================================================================
// GamePad
//======================================================================================

#pragma warning( disable : 4355 )

// Public constructor.
GamePad::GamePad() noexcept( false )
   : pImpl( std::make_unique<Impl>( this ) )
{
}


// Move constructor.
GamePad::GamePad( GamePad&& moveFrom ) noexcept
   : pImpl( std::move( moveFrom.pImpl ) )
{
   pImpl->mOwner = this;
}


// Move assignment.
GamePad& GamePad::operator= ( GamePad&& moveFrom ) noexcept
{
   pImpl = std::move( moveFrom.pImpl );
   pImpl->mOwner = this;
   return *this;
}


// Public destructor.
GamePad::~GamePad()
{
}


GamePad::State GamePad::GetState( int player, DeadZone deadZoneMode )
{
   State state;
   pImpl->GetState( player, state, deadZoneMode );
   return state;
}


GamePad::Capabilities GamePad::GetCapabilities( int player )
{
   Capabilities caps;
   pImpl->GetCapabilities( player, caps );
   return caps;
}

bool GamePad::SetVibration( int player, float leftMotor, float rightMotor, float leftTrigger, float rightTrigger )
{
   return pImpl->SetVibration( player, leftMotor, rightMotor, leftTrigger, rightTrigger );
}


void GamePad::Suspend()
{
   pImpl->Suspend();
}


void GamePad::Resume()
{
   pImpl->Resume();
}

GamePad& GamePad::Get()
{
   if ( !Impl::s_gamePad || !Impl::s_gamePad->mOwner )
      throw std::exception( "GamePad is a singleton" );

   return *Impl::s_gamePad->mOwner;
}



//======================================================================================
// ButtonStateTracker
//======================================================================================

#define UPDATE_BUTTON_STATE(field) field = static_cast<ButtonState>( ( !!state.buttons.field ) | ( ( !!state.buttons.field ^ !!lastState.buttons.field ) << 1 ) );

void GamePad::ButtonStateTracker::Update( const GamePad::State& state )
{
   UPDATE_BUTTON_STATE( a );

   assert( ( !state.buttons.a && !lastState.buttons.a ) == ( a == UP ) );
   assert( ( state.buttons.a && lastState.buttons.a ) == ( a == HELD ) );
   assert( ( !state.buttons.a && lastState.buttons.a ) == ( a == RELEASED ) );
   assert( ( state.buttons.a && !lastState.buttons.a ) == ( a == PRESSED ) );

   UPDATE_BUTTON_STATE( b );
   UPDATE_BUTTON_STATE( x );
   UPDATE_BUTTON_STATE( y );

   UPDATE_BUTTON_STATE( leftStick );
   UPDATE_BUTTON_STATE( rightStick );

   UPDATE_BUTTON_STATE( leftShoulder );
   UPDATE_BUTTON_STATE( rightShoulder );

   UPDATE_BUTTON_STATE( back );
   UPDATE_BUTTON_STATE( start );

   dpadUp = static_cast<ButtonState>( ( !!state.dpad.up ) | ( ( !!state.dpad.up ^ !!lastState.dpad.up ) << 1 ) );
   dpadDown = static_cast<ButtonState>( ( !!state.dpad.down ) | ( ( !!state.dpad.down ^ !!lastState.dpad.down ) << 1 ) );
   dpadLeft = static_cast<ButtonState>( ( !!state.dpad.left ) | ( ( !!state.dpad.left ^ !!lastState.dpad.left ) << 1 ) );
   dpadRight = static_cast<ButtonState>( ( !!state.dpad.right ) | ( ( !!state.dpad.right ^ !!lastState.dpad.right ) << 1 ) );

   assert( ( !state.dpad.up && !lastState.dpad.up ) == ( dpadUp == UP ) );
   assert( ( state.dpad.up && lastState.dpad.up ) == ( dpadUp == HELD ) );
   assert( ( !state.dpad.up && lastState.dpad.up ) == ( dpadUp == RELEASED ) );
   assert( ( state.dpad.up && !lastState.dpad.up ) == ( dpadUp == PRESSED ) );

   // Handle 'threshold' tests which emulate buttons

   bool threshold = state.IsLeftThumbStickUp();
   leftStickUp = static_cast<ButtonState>( ( !!threshold ) | ( ( !!threshold ^ !!lastState.IsLeftThumbStickUp() ) << 1 ) );

   threshold = state.IsLeftThumbStickDown();
   leftStickDown = static_cast<ButtonState>( ( !!threshold ) | ( ( !!threshold ^ !!lastState.IsLeftThumbStickDown() ) << 1 ) );

   threshold = state.IsLeftThumbStickLeft();
   leftStickLeft = static_cast<ButtonState>( ( !!threshold ) | ( ( !!threshold ^ !!lastState.IsLeftThumbStickLeft() ) << 1 ) );

   threshold = state.IsLeftThumbStickRight();
   leftStickRight = static_cast<ButtonState>( ( !!threshold ) | ( ( !!threshold ^ !!lastState.IsLeftThumbStickRight() ) << 1 ) );

   threshold = state.IsRightThumbStickUp();
   rightStickUp = static_cast<ButtonState>( ( !!threshold ) | ( ( !!threshold ^ !!lastState.IsRightThumbStickUp() ) << 1 ) );

   threshold = state.IsRightThumbStickDown();
   rightStickDown = static_cast<ButtonState>( ( !!threshold ) | ( ( !!threshold ^ !!lastState.IsRightThumbStickDown() ) << 1 ) );

   threshold = state.IsRightThumbStickLeft();
   rightStickLeft = static_cast<ButtonState>( ( !!threshold ) | ( ( !!threshold ^ !!lastState.IsRightThumbStickLeft() ) << 1 ) );

   threshold = state.IsRightThumbStickRight();
   rightStickRight = static_cast<ButtonState>( ( !!threshold ) | ( ( !!threshold ^ !!lastState.IsRightThumbStickRight() ) << 1 ) );

   threshold = state.IsLeftTriggerPressed();
   leftTrigger = static_cast<ButtonState>( ( !!threshold ) | ( ( !!threshold ^ !!lastState.IsLeftTriggerPressed() ) << 1 ) );

   threshold = state.IsRightTriggerPressed();
   rightTrigger = static_cast<ButtonState>( ( !!threshold ) | ( ( !!threshold ^ !!lastState.IsRightTriggerPressed() ) << 1 ) );

   lastState = state;
}

#undef UPDATE_BUTTON_STATE


void GamePad::ButtonStateTracker::Reset() noexcept
{
   memset( this, 0, sizeof( ButtonStateTracker ) );
}

// Translate reading from GameInput state to DirectX::GamePad::State
void GamePad::Impl::TranslateReading( ATG::GamePad::State& state, IGameInputDevice* gamePad, IGameInputReading* reading, GamePad::DeadZone deadZoneMode )
{
   ZeroMemory( &state, sizeof( ATG::GamePad::State ) );

   GameInputDeviceStatus status;
   if ( gamePad ) {
      status = gamePad->GetDeviceStatus();
   }
   else
   {
      status = GameInputDeviceConnected; // Merged input is always treated as connected.
   }

   GameInputGamepadState pad;
   bool success = reading->GetGamepadState( &pad );
   assert( success && "This should never fail if we're reading from a GamePad" );

   if ( success )
   {
      state.connected = ( status & GameInputDeviceConnected ) != 0;
      state.packet = reading->GetTimestamp();

      state.buttons.a = ( pad.buttons & GameInputGamepadA ) != 0;
      state.buttons.b = ( pad.buttons & GameInputGamepadB ) != 0;
      state.buttons.x = ( pad.buttons & GameInputGamepadX ) != 0;
      state.buttons.y = ( pad.buttons & GameInputGamepadY ) != 0;
      state.buttons.leftStick = ( pad.buttons & GameInputGamepadLeftThumbstick ) != 0;
      state.buttons.rightStick = ( pad.buttons & GameInputGamepadRightThumbstick ) != 0;
      state.buttons.leftShoulder = ( pad.buttons & GameInputGamepadLeftShoulder ) != 0;
      state.buttons.rightShoulder = ( pad.buttons & GameInputGamepadRightShoulder ) != 0;
      state.buttons.view = ( pad.buttons & GameInputGamepadView ) != 0;
      state.buttons.menu = ( pad.buttons & GameInputGamepadMenu ) != 0;

      state.dpad.up = ( pad.buttons & GameInputGamepadDPadUp ) != 0;
      state.dpad.down = ( pad.buttons & GameInputGamepadDPadDown ) != 0;
      state.dpad.right = ( pad.buttons & GameInputGamepadDPadRight ) != 0;
      state.dpad.left = ( pad.buttons & GameInputGamepadDPadLeft ) != 0;

      ApplyStickDeadZone( pad.leftThumbstickX, pad.leftThumbstickY,
         deadZoneMode, 1.f, c_XboxOneThumbDeadZone,
         state.thumbSticks.leftX, state.thumbSticks.leftY );

      ApplyStickDeadZone( pad.rightThumbstickX, pad.rightThumbstickY,
         deadZoneMode, 1.f, c_XboxOneThumbDeadZone,
         state.thumbSticks.rightX, state.thumbSticks.rightY );

      state.triggers.left = pad.leftTrigger;
      state.triggers.right = pad.rightTrigger;
   }
}
