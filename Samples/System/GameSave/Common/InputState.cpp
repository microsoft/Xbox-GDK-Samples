// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#include "..\pch.h"
#include "InputState.h"

namespace DirectX {

   bool InputState::s_isAnyGamepadConnected = false;

   InputState::InputState() :
      m_lastLetterKeyEvent( DirectX::Keyboard::Keys::None ),
      m_lastNumericKeyEvent( DirectX::Keyboard::Keys::None ),
      m_letterPressed( 0 ),
      m_numberPressed( uint32_t( -1 ) )
   {
      m_gamePad = std::make_unique<ATG::GamePad>(  );
      m_keyboard = std::make_unique<Keyboard>(  );
      m_keyboardStateTracker = std::make_unique<Keyboard::KeyboardStateTracker>(  );
      m_mouse = std::make_unique<Mouse>(  );
      m_mouseButtonStateTracker = std::make_unique<Mouse::ButtonStateTracker>(  );

      ATG::GamePad::State blankState = ATG::GamePad::State();
      for ( size_t i = 0; i < MaxInputs; i++ )
      {
         LastGamePadStates[ i ] = CurrentGamePadStates[ i ] = blankState;
      }
   }

   void InputState::Suspend()
   {
      m_gamePad->Suspend();
   }

   void InputState::Resume()
   {
      m_gamePad->Resume();

      for ( size_t i = 0; i < MaxInputs; i++ )
      {
         m_buttonStateTracker[ i ].Reset();
      }
      m_keyboardStateTracker->Reset();
      m_mouseButtonStateTracker->Reset();

      m_lastLetterKeyEvent = 0;
      m_lastNumericKeyEvent = 0;
      m_letterPressed = 0;
      m_numberPressed = uint32_t( -1 );
   }

   void InputState::OnKeyUp( int vkey )
   {
      if ( vkey >= DirectX::Keyboard::Keys::A && vkey <= DirectX::Keyboard::Keys::Z )
      {
         m_lastLetterKeyEvent = vkey;
      }
      else if ( vkey >= DirectX::Keyboard::Keys::D0 && vkey <= DirectX::Keyboard::Keys::D9 )
      {
         m_lastNumericKeyEvent = vkey;
      }
   }

   void InputState::Update()
   {
      s_isAnyGamepadConnected = false;

      for ( size_t i = 0; i < MaxInputs; i++ )
      {
         LastGamePadStates[ i ] = CurrentGamePadStates[ i ];
         CurrentGamePadStates[ i ] = m_gamePad->GetState( int(i) );

         if ( CurrentGamePadStates[ i ].IsConnected() )
         {
            s_isAnyGamepadConnected = true;
            m_buttonStateTracker[ i ].Update( CurrentGamePadStates[ i ] );
         }
         else
         {
            m_buttonStateTracker[ i ].Reset();
         }
      }

      auto keyboardState = m_keyboard->GetState();
      m_keyboardStateTracker->Update( keyboardState );

      auto mouseButtonState = m_mouse->GetState();
      m_mouseButtonStateTracker->Update( mouseButtonState );

      m_letterPressed = 0;
      if ( m_lastLetterKeyEvent != DirectX::Keyboard::Keys::None )
      {
         m_letterPressed = (wchar_t) m_lastLetterKeyEvent;
         m_lastLetterKeyEvent = DirectX::Keyboard::Keys::None;
      }

      m_numberPressed = uint32_t( -1 );
      if ( m_lastNumericKeyEvent != DirectX::Keyboard::Keys::None )
      {
         m_numberPressed = uint32_t( m_lastNumericKeyEvent - DirectX::Keyboard::Keys::D0 );
         m_lastNumericKeyEvent = DirectX::Keyboard::Keys::None;
      }
   }

   bool InputState::IsNewButtonPress( GameInputGamepadButtons button, int controllingPlayer, int* playerIndex ) const
   {
      if ( controllingPlayer >= 0 )
      {
         if ( size_t(controllingPlayer) < MaxInputs )
         {
            // Read input from the specified player
            if ( GetButtonState( button, controllingPlayer ) == ATG::GamePad::ButtonStateTracker::ButtonState::PRESSED )
            {
               if ( playerIndex )
               {
                  *playerIndex = controllingPlayer;
               }
               return true;
            }
         }

         if ( playerIndex )
         {
            *playerIndex = -1;
         }
         return false;
      }
      else
      {
         // Accept input from any player
         for ( size_t i = 0; i < MaxInputs; i++ )
         {
            if ( IsNewButtonPress( button, int(i), playerIndex ) )
               return true;
         }
         return false;
      }
   }

   ATG::GamePad::ButtonStateTracker::ButtonState InputState::GetButtonState( GameInputGamepadButtons button, int controllingPlayer ) const
   {
      assert( controllingPlayer != ATG::GamePad::MERGED_CONTROLLER_INPUT_PLAYER_INDEX && "Can't call this with the merged input");
      
      auto& tracker = m_buttonStateTracker[ size_t(controllingPlayer) ];

      switch ( button )
      {
      case GameInputGamepadNone:
         return ATG::GamePad::ButtonStateTracker::ButtonState::UP;
      case GameInputGamepadDPadUp:
         return tracker.dpadUp;
      case GameInputGamepadDPadDown:
         return tracker.dpadDown;
      case GameInputGamepadDPadLeft:
         return tracker.dpadLeft;
      case GameInputGamepadDPadRight:
         return tracker.dpadRight;
      case GameInputGamepadView:
         return tracker.back;
      case GameInputGamepadMenu:
         return tracker.start;
      case GameInputGamepadLeftThumbstick:
         return tracker.leftStick;
      case GameInputGamepadRightThumbstick:
         return tracker.rightStick;
      case GameInputGamepadLeftShoulder:
         return tracker.leftShoulder;
      case GameInputGamepadRightShoulder:
         return tracker.rightShoulder;
      case GameInputGamepadA:
         return tracker.a;
      case GameInputGamepadB:
         return tracker.b;
      case GameInputGamepadX:
         return tracker.x;
      case GameInputGamepadY:
         return tracker.y;
      default:
         return ATG::GamePad::ButtonStateTracker::ButtonState::UP;
      }
   }

   bool InputState::IsNewKeyPress( DirectX::Keyboard::Keys key ) const
   {
      return m_keyboardStateTracker->IsKeyPressed( key );
   }

   bool InputState::IsCursorUp( int controllingPlayer, int* playerIndex ) const
   {
      bool newLeftThumbstickUp = false;
      if ( controllingPlayer >= 0 && size_t(controllingPlayer) < MaxInputs )
      {
         newLeftThumbstickUp = ( m_buttonStateTracker[ size_t(controllingPlayer) ].leftStickUp == ATG::GamePad::ButtonStateTracker::PRESSED );
      }
      else
      {
         // Accept input from any player
         for ( size_t i = 0; i < MaxInputs; i++ )
         {
            newLeftThumbstickUp = ( m_buttonStateTracker[ i ].leftStickUp == ATG::GamePad::ButtonStateTracker::PRESSED );
            if ( newLeftThumbstickUp )
            {
               controllingPlayer = int(i);
               break;
            }
         }
      }

      if ( playerIndex && newLeftThumbstickUp )
         *playerIndex = controllingPlayer;

      return newLeftThumbstickUp
         || IsNewButtonPress( GameInputGamepadDPadUp, controllingPlayer, playerIndex )
         || m_keyboardStateTracker->pressed.Up;
   }

   bool InputState::IsCursorDown( int controllingPlayer, int* playerIndex ) const
   {
      bool newLeftThumbstickDown = false;
      if ( controllingPlayer >= 0 && size_t(controllingPlayer) < MaxInputs )
      {
         newLeftThumbstickDown = CurrentGamePadStates[ size_t(controllingPlayer) ].IsLeftThumbStickDown() && !LastGamePadStates[ size_t(controllingPlayer) ].IsLeftThumbStickDown();
      }
      else
      {
         // Accept input from any player
         for ( size_t i = 0; i < MaxInputs; i++ )
         {
            newLeftThumbstickDown = CurrentGamePadStates[ i ].IsLeftThumbStickDown() && !LastGamePadStates[ i ].IsLeftThumbStickDown();
            if ( newLeftThumbstickDown )
            {
               controllingPlayer = int(i);
               break;
            }
         }
      }

      if ( playerIndex && newLeftThumbstickDown )
         *playerIndex = controllingPlayer;

      return newLeftThumbstickDown
         || IsNewButtonPress( GameInputGamepadDPadDown, controllingPlayer, playerIndex )
         || m_keyboardStateTracker->pressed.Down;
   }

   bool InputState::IsCursorLeft( int controllingPlayer, int* playerIndex ) const
   {
      bool newLeftThumbstickLeft = false;
      if ( controllingPlayer >= 0 && size_t(controllingPlayer) < MaxInputs )
      {
         newLeftThumbstickLeft = CurrentGamePadStates[ size_t(controllingPlayer) ].IsLeftThumbStickLeft() && !LastGamePadStates[ size_t(controllingPlayer) ].IsLeftThumbStickLeft();
      }
      else
      {
         // Accept input from any player
         for ( size_t i = 0; i < MaxInputs; i++ )
         {
            newLeftThumbstickLeft = CurrentGamePadStates[ i ].IsLeftThumbStickLeft() && !LastGamePadStates[ i ].IsLeftThumbStickLeft();
            if ( newLeftThumbstickLeft )
            {
               controllingPlayer = int(i);
               break;
            }
         }
      }

      if ( playerIndex && newLeftThumbstickLeft )
         *playerIndex = controllingPlayer;

      return newLeftThumbstickLeft
         || IsNewButtonPress( GameInputGamepadDPadLeft, controllingPlayer, playerIndex )
         || m_keyboardStateTracker->pressed.Left;
   }

   bool InputState::IsCursorRight( int controllingPlayer, int* playerIndex ) const
   {
      bool newLeftThumbstickRight = false;
      if ( controllingPlayer >= 0 && size_t(controllingPlayer) < MaxInputs )
      {
         newLeftThumbstickRight = CurrentGamePadStates[ size_t(controllingPlayer) ].IsLeftThumbStickRight() && !LastGamePadStates[ size_t(controllingPlayer) ].IsLeftThumbStickRight();
      }
      else
      {
         // Accept input from any player
         for ( size_t i = 0; i < MaxInputs; i++ )
         {
            newLeftThumbstickRight = CurrentGamePadStates[ i ].IsLeftThumbStickRight() && !LastGamePadStates[ i ].IsLeftThumbStickRight();
            if ( newLeftThumbstickRight )
            {
               controllingPlayer = int(i);
               break;
            }
         }
      }

      if ( playerIndex && newLeftThumbstickRight )
         *playerIndex = controllingPlayer;

      return newLeftThumbstickRight
         || IsNewButtonPress( GameInputGamepadDPadRight, controllingPlayer, playerIndex )
         || m_keyboardStateTracker->pressed.Right;
   }

   bool InputState::IsLogScrollUp( int controllingPlayer, int* playerIndex ) const
   {
      bool rightThumbstickUp = false;
      if ( controllingPlayer >= 0 && size_t(controllingPlayer) < MaxInputs )
      {
         rightThumbstickUp = CurrentGamePadStates[ size_t(controllingPlayer) ].IsRightThumbStickUp();
      }
      else
      {
         // Accept input from any player
         for ( size_t i = 0; i < MaxInputs; i++ )
         {
            rightThumbstickUp = CurrentGamePadStates[ i ].IsRightThumbStickUp();
            if ( rightThumbstickUp )
            {
               controllingPlayer = int(i);
               break;
            }
         }
      }

      if ( playerIndex && rightThumbstickUp )
      {
         *playerIndex = controllingPlayer;
      }

      auto kb = m_keyboard->GetState();

      return rightThumbstickUp
         || kb.PageUp
         || kb.OemOpenBrackets;
   }

   bool InputState::IsLogScrollDown( int controllingPlayer, int* playerIndex ) const
   {
      bool rightThumbstickDown = false;
      if ( controllingPlayer >= 0 && size_t(controllingPlayer) < MaxInputs )
      {
         rightThumbstickDown = CurrentGamePadStates[ size_t(controllingPlayer) ].IsRightThumbStickDown();
      }
      else
      {
         // Accept input from any player
         for ( size_t i = 0; i < MaxInputs; i++ )
         {
            rightThumbstickDown = CurrentGamePadStates[ i ].IsRightThumbStickDown();
            if ( rightThumbstickDown )
            {
               controllingPlayer = int(i);
               break;
            }
         }
      }

      if ( playerIndex && rightThumbstickDown )
      {
         *playerIndex = controllingPlayer;
      }

      auto kb = m_keyboard->GetState();

      return rightThumbstickDown
         || kb.PageDown
         || kb.OemCloseBrackets;
   }

   bool InputState::IsMenuSelect( int controllingPlayer, int* playerIndex ) const
   {
      auto kb = m_keyboard->GetState();

      return IsNewButtonPress( GameInputGamepadA, controllingPlayer, playerIndex )
         || ( m_keyboardStateTracker->pressed.Enter && !( kb.LeftAlt || kb.RightAlt ) );
   }

   bool InputState::IsMenuCancel( int controllingPlayer, int* playerIndex ) const
   {
      return IsNewButtonPress( GameInputGamepadB, controllingPlayer, playerIndex )
         || m_keyboardStateTracker->pressed.Escape
         || m_keyboardStateTracker->pressed.Back;
   }

   bool InputState::IsMenuUp( int controllingPlayer, int* playerIndex ) const
   {
      return IsCursorUp( controllingPlayer, playerIndex );
   }

   bool InputState::IsMenuDown( int controllingPlayer, int* playerIndex ) const
   {
      return IsCursorDown( controllingPlayer, playerIndex );
   }

   bool InputState::IsMenuLeft( int controllingPlayer, int* playerIndex ) const
   {
      return IsCursorLeft( controllingPlayer, playerIndex )
         || m_keyboardStateTracker->pressed.OemMinus;
   }

   bool InputState::IsMenuRight( int controllingPlayer, int* playerIndex ) const
   {
      return IsCursorRight( controllingPlayer, playerIndex )
         || m_keyboardStateTracker->pressed.OemPlus;
   }

   bool InputState::IsMenuToggle( int controllingPlayer, int* playerIndex ) const
   {
      return IsNewButtonPress( GameInputGamepadMenu, controllingPlayer, playerIndex )
         || m_keyboardStateTracker->pressed.Tab;
   }

   bool InputState::IsMouseSelect( XMINT2& mouseClick ) const
   {
      if ( m_mouseButtonStateTracker->leftButton == Mouse::ButtonStateTracker::ButtonState::RELEASED )
      {
         auto mouseState = m_mouse->GetState();
         mouseClick.x = mouseState.x;
         mouseClick.y = mouseState.y;
         return true;
      }

      return false;
   }

   bool InputState::IsPauseGame( int controllingPlayer, int* playerIndex ) const
   {
      return IsNewButtonPress( GameInputGamepadMenu, controllingPlayer, playerIndex )
         || m_keyboardStateTracker->pressed.Tab;
   }

   bool InputState::IsTileScrollUp( int controllingPlayer, int * playerIndex ) const
   {
      bool rightThumbstickRight = false;
      if ( controllingPlayer >= 0 && size_t(controllingPlayer) < MaxInputs )
      {
         rightThumbstickRight = CurrentGamePadStates[ size_t(controllingPlayer) ].IsRightThumbStickRight();
      }
      else
      {
         // Accept input from any player
         for ( size_t i = 0; i < MaxInputs; i++ )
         {
            rightThumbstickRight = CurrentGamePadStates[ i ].IsRightThumbStickRight();
            if ( rightThumbstickRight )
            {
               controllingPlayer = int(i);
               break;
            }
         }
      }

      if ( playerIndex && rightThumbstickRight )
         *playerIndex = controllingPlayer;

      return rightThumbstickRight;
   }

   bool InputState::IsTileScrollDown( int controllingPlayer, int * playerIndex ) const
   {
      bool rightThumbstickLeft = false;
      if ( controllingPlayer >= 0 && size_t(controllingPlayer) < MaxInputs )
      {
         rightThumbstickLeft = CurrentGamePadStates[ size_t(controllingPlayer) ].IsRightThumbStickLeft();
      }
      else
      {
         // Accept input from any player
         for ( size_t i = 0; i < MaxInputs; i++ )
         {
            rightThumbstickLeft = CurrentGamePadStates[ i ].IsRightThumbStickLeft();
            if ( rightThumbstickLeft )
            {
               controllingPlayer = int(i);
               break;
            }
         }
      }

      if ( playerIndex && rightThumbstickLeft )
         *playerIndex = controllingPlayer;

      return rightThumbstickLeft;
   }

   bool InputState::IsNewRightThumbstickUp( int controllingPlayer, int* playerIndex ) const
   {
      bool newRightThumbstickUp = false;
      if ( controllingPlayer >= 0 && size_t(controllingPlayer) < MaxInputs )
      {
         newRightThumbstickUp = CurrentGamePadStates[ size_t(controllingPlayer) ].IsRightThumbStickUp() && !LastGamePadStates[ size_t(controllingPlayer) ].IsRightThumbStickUp();
      }
      else
      {
         // Accept input from any player
         for ( size_t i = 0; i < MaxInputs; i++ )
         {
            newRightThumbstickUp = CurrentGamePadStates[ i ].IsRightThumbStickUp() && !LastGamePadStates[ i ].IsRightThumbStickUp();
            if ( newRightThumbstickUp )
            {
               controllingPlayer = int(i);
               break;
            }
         }
      }

      if ( playerIndex && newRightThumbstickUp )
         *playerIndex = controllingPlayer;

      return newRightThumbstickUp;
   }

   bool InputState::IsNewRightThumbstickDown( int controllingPlayer, int* playerIndex ) const
   {
      bool newRightThumbstickDown = false;
      if ( controllingPlayer >= 0 && size_t(controllingPlayer) < MaxInputs )
      {
         newRightThumbstickDown = CurrentGamePadStates[ size_t(controllingPlayer) ].IsRightThumbStickDown() && !LastGamePadStates[ size_t(controllingPlayer) ].IsRightThumbStickDown();
      }
      else
      {
         // Accept input from any player
         for ( size_t i = 0; i < MaxInputs; i++ )
         {
            newRightThumbstickDown = CurrentGamePadStates[ i ].IsRightThumbStickDown() && !LastGamePadStates[ i ].IsRightThumbStickDown();
            if ( newRightThumbstickDown )
            {
               controllingPlayer = int(i);
               break;
            }
         }
      }

      if ( playerIndex && newRightThumbstickDown )
         *playerIndex = controllingPlayer;

      return newRightThumbstickDown;
   }

   bool InputState::IsNewRightThumbstickLeft( int controllingPlayer, int* playerIndex ) const
   {
      bool newRightThumbstickLeft = false;
      if ( controllingPlayer >= 0 && size_t(controllingPlayer) < MaxInputs )
      {
         newRightThumbstickLeft = CurrentGamePadStates[ size_t(controllingPlayer) ].IsRightThumbStickLeft() && !LastGamePadStates[ size_t(controllingPlayer) ].IsRightThumbStickLeft();
      }
      else
      {
         // Accept input from any player
         for ( size_t i = 0; i < MaxInputs; i++ )
         {
            newRightThumbstickLeft = CurrentGamePadStates[ i ].IsRightThumbStickLeft() && !LastGamePadStates[ i ].IsRightThumbStickLeft();
            if ( newRightThumbstickLeft )
            {
               controllingPlayer = int(i);
               break;
            }
         }
      }

      if ( playerIndex && newRightThumbstickLeft )
         *playerIndex = controllingPlayer;

      return newRightThumbstickLeft;
   }

   bool InputState::IsNewRightThumbstickRight( int controllingPlayer, int* playerIndex ) const
   {
      bool newRightThumbstickRight = false;
      if ( controllingPlayer >= 0 && size_t(controllingPlayer) < MaxInputs )
      {
         newRightThumbstickRight = CurrentGamePadStates[ size_t(controllingPlayer) ].IsRightThumbStickRight() && !LastGamePadStates[ size_t(controllingPlayer) ].IsRightThumbStickRight();
      }
      else
      {
         // Accept input from any player
         for ( size_t i = 0; i < MaxInputs; i++ )
         {
            newRightThumbstickRight = CurrentGamePadStates[ i ].IsRightThumbStickRight() && !LastGamePadStates[ i ].IsRightThumbStickRight();
            if ( newRightThumbstickRight )
            {
               controllingPlayer = int(i);
               break;
            }
         }
      }

      if ( playerIndex && newRightThumbstickRight )
         *playerIndex = controllingPlayer;

      return newRightThumbstickRight;
   }

   IGameInputDevice* InputState::GetGamepad( int index ) const
   {
      if ( index < -1 || size_t(index) >= MaxInputs )
         return nullptr;

      if ( index == -1 ) // Find first gamepad.
      {
         for ( size_t i = 0; i < MaxInputs; ++i )
         {
            auto gamePad = ATG::GetGamePadByIndex( int(i) );
            if ( gamePad )
            {
               return gamePad.Get();
            }
         }
         return nullptr;
      }
      else
      {
         auto gamePad = ATG::GetGamePadByIndex( index );
         return gamePad.Get();
      }
   }

   int InputState::GetGamepadIndex( _In_ IGameInputDevice* gamepad ) const
   {
      return ATG::FindGamePadIndex( gamepad );
   }

} // namespace DirectX
