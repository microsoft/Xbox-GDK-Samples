//--------------------------------------------------------------------------------------
// File: UIInputState.cpp
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

#include "pch.h"

#include <algorithm>

#include "UIInputState.h"

namespace ATG
{
    namespace UITK
    {
        using namespace DirectX;

        UIInputState::UIInputState()
        {
        }

        void UIInputState::Update(float, GamePad& gamepad)
        {
            for (size_t padIndex = 0; padIndex < c_maxGamePads; ++padIndex)
            {
                m_gamePadStates[padIndex].m_prevState = m_gamePadStates[padIndex].m_state;
                m_gamePadStates[padIndex].m_state = gamepad.GetState(int(padIndex));
                if (!m_gamePadStates[padIndex].m_state.IsConnected())
                {
                    m_gamePadStates[padIndex].m_buttons.Reset();
                }
                else
                {
                    m_gamePadStates[padIndex].m_buttons.Update(m_gamePadStates[padIndex].m_state);
                }
            }
        }

        void UIInputState::Update(float, Keyboard& keyboard)
        {
            m_keyboardState.m_prevState = m_keyboardState.m_state;
            m_keyboardState.m_state = keyboard.GetState();
            m_keyboardState.m_keys.Update(m_keyboardState.m_state);
        }

        void UIInputState::Update(float, Mouse& mouse)
        {
            m_mouseState.m_prevState = m_mouseState.m_state;
            m_mouseState.m_state = mouse.GetState();
            m_mouseState.m_buttons.Update(m_mouseState.m_state);
        }

        void UIInputState::Update(float elapsedTimeInS, Keyboard& keyboard, Mouse& mouse)
        {
            Update(elapsedTimeInS, keyboard);
            Update(elapsedTimeInS, mouse);
        }

        void UIInputState::Update(float elapsedTimeInS, GamePad& gamepad, Keyboard& keyboard, Mouse& mouse)
        {
            Update(elapsedTimeInS, gamepad);
            Update(elapsedTimeInS, keyboard, mouse);
        }

        void UIInputState::Reset()
        {
            for (auto& state : m_gamePadStates)
            {
                state.m_buttons.Reset();
            }

            m_keyboardState.m_keys.Reset();
            m_mouseState.m_buttons.Reset();
        }

        const GamePad::State& UIInputState::GetPrevGamePadState(size_t index) const
        {
            return m_gamePadStates[index].m_prevState;
        }

        const GamePad::State& UIInputState::GetGamePadState(size_t index) const
        {
            return m_gamePadStates[index].m_state;
        }

        const UIInputState::GamePadButtonStates& UIInputState::GetGamePadButtons(size_t index) const
        {
            return m_gamePadStates[index].m_buttons;
        }

        bool UIInputState::AnyAIsState(GamePadButtonStates::ButtonState state) const
        {
            return std::any_of(m_gamePadStates.begin(), m_gamePadStates.end(),
                [state](GamePadState gamePadState) 
                {
                    return gamePadState.m_buttons.a == state;
                });
        }

        bool UIInputState::AnyDPLIsState(GamePadButtonStates::ButtonState state) const
        {
            return std::any_of(m_gamePadStates.begin(), m_gamePadStates.end(),
                [state](GamePadState gamePadState)
                {
                    return gamePadState.m_buttons.dpadLeft == state;
                });
        }

        bool UIInputState::AnyDPRIsState(GamePadButtonStates::ButtonState state) const
        {
            return std::any_of(m_gamePadStates.begin(), m_gamePadStates.end(),
                [state](GamePadState gamePadState)
                {
                    return gamePadState.m_buttons.dpadRight == state;
                });
        }

        bool UIInputState::AnyDPUIsState(GamePadButtonStates::ButtonState state) const
        {
            return std::any_of(m_gamePadStates.begin(), m_gamePadStates.end(),
                [state](GamePadState gamePadState)
                {
                    return gamePadState.m_buttons.dpadUp == state;
                });
        }

        bool UIInputState::AnyDPDIsState(GamePadButtonStates::ButtonState state) const
        {
            return std::any_of(m_gamePadStates.begin(), m_gamePadStates.end(),
                [state](GamePadState gamePadState)
                {
                    return gamePadState.m_buttons.dpadDown == state;
                });
        }

        bool UIInputState::AnyLTStickUp() const
        {
            return std::any_of(m_gamePadStates.begin(), m_gamePadStates.end(),
                [](GamePadState gamePadState)
                {
                    return gamePadState.m_state.IsLeftThumbStickUp();
                });
        }

        bool UIInputState::AnyLTStickDown() const
        {
            return std::any_of(m_gamePadStates.begin(), m_gamePadStates.end(),
                [](GamePadState gamePadState)
                {
                    return gamePadState.m_state.IsLeftThumbStickDown();
                });
        }

        bool UIInputState::AnyLTStickLeft() const
        {
            return std::any_of(m_gamePadStates.begin(), m_gamePadStates.end(),
                [](GamePadState gamePadState)
                {
                    return gamePadState.m_state.IsLeftThumbStickLeft();
                });
        }

        bool UIInputState::AnyLTStickRright() const
        {
            return std::any_of(m_gamePadStates.begin(), m_gamePadStates.end(),
                [](GamePadState gamePadState)
                {
                    return gamePadState.m_state.IsLeftThumbStickRight();
                });
        }

        const Keyboard::State& UIInputState::GetPrevKeyboardState() const
        {
            return m_keyboardState.m_prevState;
        }

        const Keyboard::State& UIInputState::GetKeyboardState() const
        {
            return m_keyboardState.m_state;
        }

        const UIInputState::KeyboardKeyStates& UIInputState::GetKeyboardKeys() const
        {
            return m_keyboardState.m_keys;
        }

        const Mouse::State& UIInputState::GetPrevMouseState() const
        {
            return m_mouseState.m_prevState;
        }

        const Mouse::State& UIInputState::GetMouseState() const
        {
            return m_mouseState.m_state;
        }

        const UIInputState::MouseButtonStates& UIInputState::GetMouseButtons() const
        {
            return m_mouseState.m_buttons;
        }
    }
}
