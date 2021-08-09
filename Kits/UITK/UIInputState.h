//--------------------------------------------------------------------------------------
// File: UIInputState.h
//
// Authored by: ATG
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

#include <array>
#include <memory>

#include "GamePad.h"
#include "Keyboard.h"
#include "Mouse.h"

namespace ATG
{
    namespace UITK
    {
        using Mouse = DirectX::Mouse;
        using GamePad = DirectX::GamePad;
        using Keyboard = DirectX::Keyboard;

        class UIInputState
        {
        public:

            using GamePadButtonStates = GamePad::ButtonStateTracker;
            using KeyboardKeyStates = Keyboard::KeyboardStateTracker;
            using MouseButtonStates = Mouse::ButtonStateTracker;

            UIInputState();

            int GetMaxGamePads() const
            {
                return c_maxGamePads;
            }

            void Update(float elapsedTimeInS, GamePad& gamepad);
            void Update(float elapsedTimeInS, Keyboard& keyboard);
            void Update(float elapsedTimeInS, Mouse& mouse);
            void Update(float elapsedTimeInS, Keyboard& keyboard, Mouse& mouse);
            void Update(float elapsedTimeInS, GamePad& gamepad, Keyboard& keyboard, Mouse& mouse);
            void Reset();

            const GamePad::State& GetPrevGamePadState(size_t index) const;
            const GamePad::State& GetGamePadState(size_t index) const;
            const GamePadButtonStates& GetGamePadButtons(size_t index) const;

            bool AnyAIsState(GamePadButtonStates::ButtonState state) const;
            bool AnyDPLIsState(GamePadButtonStates::ButtonState state) const;
            bool AnyDPRIsState(GamePadButtonStates::ButtonState state) const;
            bool AnyDPUIsState(GamePadButtonStates::ButtonState state) const;
            bool AnyDPDIsState(GamePadButtonStates::ButtonState state) const;
            bool AnyLTStickUp() const;
            bool AnyLTStickDown() const;
            bool AnyLTStickLeft() const;
            bool AnyLTStickRright() const;

            const Keyboard::State& GetPrevKeyboardState() const;
            const Keyboard::State& GetKeyboardState() const;
            const KeyboardKeyStates& GetKeyboardKeys() const;

            const Mouse::State& GetPrevMouseState() const;
            const Mouse::State& GetMouseState() const;
            const MouseButtonStates& GetMouseButtons() const;

        private:
            static constexpr int c_maxGamePads = 4;

            struct GamePadState
            {
                GamePad::State m_prevState;
                GamePad::State m_state;
                GamePadButtonStates m_buttons;
            };

            std::array<GamePadState, c_maxGamePads> m_gamePadStates;

            struct KeyboardState
            {
                Keyboard::State m_prevState;
                Keyboard::State m_state;
                KeyboardKeyStates m_keys;
            };

            KeyboardState               m_keyboardState;

            struct MouseState
            {
                Mouse::State m_prevState;
                Mouse::State m_state;
                MouseButtonStates m_buttons;
            };

            MouseState                  m_mouseState;
        };

        struct InputEvent
        {
        public:
            enum Event
            {
                /// Called by the UIManager to a UIElement when that element is not
                /// focused, but has been entered into by the mouse cursor.
                MouseIn,

                /// Called by the UIManager to a UIElement when that element is not
                /// focused, but continues to contain the mouse cursor.
                MouseOver,

                /// Called by the UIManager to a UIElement when that element is not
                /// focused, but has been exited from by the mouse cursor.
                MouseOut,

                /// Called by the UIManager to a UIElement when that element has 
                /// become the focused element.
                GainFocus,

                /// Called by the UIManager to a UIElement when that element is no
                /// longer the focused element.
                LoseFocus,

                /// Called by the UIManager to a UIElement when that element is
                /// focused, and has been entered into by the mouse cursor.
                MouseInFocus,

                /// Called by the UIManager to a UIElement when that element is 
                /// focused, and continues to contain the mouse cursor.
                MouseOverFocus,

                /// Called by the UIManager to a UIElement when that element is 
                /// focused, and has been exited from by the mouse cursor.
                MouseOutFocus,

                /// Called by the UIElement to the focused UIElement every frame to
                /// handle frame-by-frame input state changes.
                InputStateChange
            };

            Event               m_inputEvent;
            const UIInputState& m_inputState;

            InputEvent(Event event, const UIInputState& inputState) :
                m_inputEvent(event), m_inputState(inputState)
            {
            }
        };
    }
}
