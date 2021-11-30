//--------------------------------------------------------------------------------------
// File: GamePad.h
//
// Gaming.*.x64-specific GamePad implementation.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <memory>
#include <stdint.h>

namespace ATG
{
   class GamePad
   {
   public:
      GamePad() noexcept( false );
      GamePad( GamePad&& moveFrom ) noexcept;
      GamePad& operator= ( GamePad&& moveFrom ) noexcept;

      GamePad( GamePad const& ) = delete;
      GamePad& operator=( GamePad const& ) = delete;

      virtual ~GamePad();

      static constexpr int MERGED_CONTROLLER_INPUT_PLAYER_INDEX = -1; 
      static constexpr int MAX_PLAYER_COUNT = 8;

      enum DeadZone
      {
         DEAD_ZONE_INDEPENDENT_AXES = 0,
         DEAD_ZONE_CIRCULAR,
         DEAD_ZONE_NONE,
      };

      struct Buttons
      {
         bool a;
         bool b;
         bool x;
         bool y;
         bool leftStick;
         bool rightStick;
         bool leftShoulder;
         bool rightShoulder;
         union
         {
            bool back;
            bool view;
         };
         union
         {
            bool start;
            bool menu;
         };
      };

      struct DPad
      {
         bool up;
         bool down;
         bool right;
         bool left;
      };

      struct ThumbSticks
      {
         float leftX;
         float leftY;
         float rightX;
         float rightY;
      };

      struct Triggers
      {
         float left;
         float right;
      };

      struct State
      {
         bool        connected;
         uint64_t    packet;
         Buttons     buttons;
         DPad        dpad;
         ThumbSticks thumbSticks;
         Triggers    triggers;

         bool __cdecl IsConnected() const { return connected; }

         // Is the button pressed currently?
         bool __cdecl IsAPressed() const { return buttons.a; }
         bool __cdecl IsBPressed() const { return buttons.b; }
         bool __cdecl IsXPressed() const { return buttons.x; }
         bool __cdecl IsYPressed() const { return buttons.y; }

         bool __cdecl IsLeftStickPressed() const { return buttons.leftStick; }
         bool __cdecl IsRightStickPressed() const { return buttons.rightStick; }

         bool __cdecl IsLeftShoulderPressed() const { return buttons.leftShoulder; }
         bool __cdecl IsRightShoulderPressed() const { return buttons.rightShoulder; }

         bool __cdecl IsBackPressed() const { return buttons.back; }
         bool __cdecl IsViewPressed() const { return buttons.view; }
         bool __cdecl IsStartPressed() const { return buttons.start; }
         bool __cdecl IsMenuPressed() const { return buttons.menu; }

         bool __cdecl IsDPadDownPressed() const { return dpad.down; }
         bool __cdecl IsDPadUpPressed() const { return dpad.up; }
         bool __cdecl IsDPadLeftPressed() const { return dpad.left; }
         bool __cdecl IsDPadRightPressed() const { return dpad.right; }

         bool __cdecl IsLeftThumbStickUp() const { return ( thumbSticks.leftY > 0.5f ) != 0; }
         bool __cdecl IsLeftThumbStickDown() const { return ( thumbSticks.leftY < -0.5f ) != 0; }
         bool __cdecl IsLeftThumbStickLeft() const { return ( thumbSticks.leftX < -0.5f ) != 0; }
         bool __cdecl IsLeftThumbStickRight() const { return ( thumbSticks.leftX > 0.5f ) != 0; }

         bool __cdecl IsRightThumbStickUp() const { return ( thumbSticks.rightY > 0.5f ) != 0; }
         bool __cdecl IsRightThumbStickDown() const { return ( thumbSticks.rightY < -0.5f ) != 0; }
         bool __cdecl IsRightThumbStickLeft() const { return ( thumbSticks.rightX < -0.5f ) != 0; }
         bool __cdecl IsRightThumbStickRight() const { return ( thumbSticks.rightX > 0.5f ) != 0; }

         bool __cdecl IsLeftTriggerPressed() const { return ( triggers.left > 0.5f ) != 0; }
         bool __cdecl IsRightTriggerPressed() const { return ( triggers.right > 0.5f ) != 0; }
      };

      struct Capabilities
      {
         enum Type
         {
            UNKNOWN = 0,
            GAMEPAD,
            WHEEL,
            ARCADE_STICK,
            FLIGHT_STICK,
            DANCE_PAD,
            GUITAR,
            GUITAR_ALTERNATE,
            DRUM_KIT,
            GUITAR_BASS = 11,
            ARCADE_PAD = 19,
         };

         bool            connected;
         Type            gamepadType;
         APP_LOCAL_DEVICE_ID id;

         bool __cdecl IsConnected() const { return connected; }
      };

      class ButtonStateTracker
      {
      public:
         enum ButtonState
         {
            UP = 0,         // Button is up
            HELD = 1,       // Button is held down
            RELEASED = 2,   // Button was just released
            PRESSED = 3,    // Button was just pressed
         };

         ButtonState a;
         ButtonState b;
         ButtonState x;
         ButtonState y;

         ButtonState leftStick;
         ButtonState rightStick;

         ButtonState leftShoulder;
         ButtonState rightShoulder;

         union
         {
            ButtonState back;
            ButtonState view;
         };

         union
         {
            ButtonState start;
            ButtonState menu;
         };

         ButtonState dpadUp;
         ButtonState dpadDown;
         ButtonState dpadLeft;
         ButtonState dpadRight;

         ButtonState leftStickUp;
         ButtonState leftStickDown;
         ButtonState leftStickLeft;
         ButtonState leftStickRight;

         ButtonState rightStickUp;
         ButtonState rightStickDown;
         ButtonState rightStickLeft;
         ButtonState rightStickRight;

         ButtonState leftTrigger;
         ButtonState rightTrigger;

#pragma prefast(suppress: 26495, "Reset() performs the initialization")
         ButtonStateTracker() noexcept { Reset(); }

         void __cdecl Update( const State& state );

         void __cdecl Reset() noexcept;

         State __cdecl GetLastState() const { return lastState; }

      private:
         State lastState;
      };

      // Retrieve the current state of the gamepad of the associated player index
      State __cdecl GetState( int player, DeadZone deadZoneMode = DEAD_ZONE_INDEPENDENT_AXES );

      // Retrieve the current capabilities of the gamepad of the associated player index
      Capabilities __cdecl GetCapabilities( int player );

      // Set the vibration motor speeds of the gamepad
      bool __cdecl SetVibration( int player, float leftMotor, float rightMotor, float leftTrigger = 0.f, float rightTrigger = 0.f );

      // Handle suspending/resuming
      void __cdecl Suspend();
      void __cdecl Resume();

      // Singleton
      static GamePad& __cdecl Get();

   private:
      // Private implementation.
      class Impl;

      std::unique_ptr<Impl> pImpl;
   };
}
