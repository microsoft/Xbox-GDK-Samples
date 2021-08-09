//--------------------------------------------------------------------------------------
// File: UIButton.cpp
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
#include "UIButton.h"
#include "UIStyleImpl.h"

NAMESPACE_ATG_UITK_BEGIN

/*protected:*/

/*virtual*/ bool UIButton::HandleInputEvent(const InputEvent& inputEvent)
{
    auto mouseButtonHeld = inputEvent.m_inputState.GetMouseButtons().leftButton == Mouse::ButtonStateTracker::HELD;
    auto mouseButtonPressed = inputEvent.m_inputState.GetMouseButtons().leftButton == Mouse::ButtonStateTracker::PRESSED;
    auto mouseButtonReleased = inputEvent.m_inputState.GetMouseButtons().leftButton == Mouse::ButtonStateTracker::RELEASED;

    auto returnPressed = inputEvent.m_inputState.GetKeyboardKeys().IsKeyPressed(DirectX::Keyboard::Keys::Enter);
    auto returnReleased = inputEvent.m_inputState.GetKeyboardKeys().IsKeyReleased(DirectX::Keyboard::Keys::Enter);

    auto gamepadAPressed = inputEvent.m_inputState.AnyAIsState(GamePad::ButtonStateTracker::PRESSED);
    auto gamepadAReleased = inputEvent.m_inputState.AnyAIsState(GamePad::ButtonStateTracker::RELEASED);

    switch (inputEvent.m_inputEvent)
    {
    case InputEvent::MouseOverFocus:
        if (mouseButtonPressed) SetCurrentState(State::Pressed);
        else if (mouseButtonHeld) SetCurrentState(State::Down);
        else SetCurrentState(State::Hovered);
        return true;

    case InputEvent::MouseOver:
        SetCurrentState(State::Hovered);
        return true;

    case InputEvent::MouseOut:
    case InputEvent::LoseFocus:
        SetCurrentState(State::Normal);
        return true;

    case InputEvent::MouseOutFocus:
        SetCurrentState(State::Focused);
        return true;

    case InputEvent::GainFocus:
        if (mouseButtonPressed) SetCurrentState(State::Pressed);
        else SetCurrentState(State::Focused);
        return true;

    case InputEvent::InputStateChange:
        if (returnPressed || gamepadAPressed) SetCurrentState(State::Pressed);
        else if (mouseButtonReleased || returnReleased || gamepadAReleased) SetCurrentState(State::Focused);
        return true;

    case InputEvent::MouseInFocus:
    case InputEvent::MouseIn:
    default:
        return false;
    }
}

/*virtual*/ void UIButton::Render()
{
    auto spriteStyle = GetCurrentStyle();
    if (spriteStyle)
    {
        auto screenRect = GetScreenRectInPixels();
        spriteStyle->RenderSprite(screenRect);
    }
}

/*virtual*/ void UIButton::PostRender()
{
    auto spriteStyle = GetCurrentStyle();
    if (spriteStyle)
    {
        spriteStyle->PostRender();
    }
}

/*protected:*/

std::shared_ptr<UISpriteStyle> UIButton::GetCurrentStyle()
{
    UIStyleManager& styleManager = m_uiManager.GetStyleManager();

    if (!IsEnabled())
    {
        return styleManager.GetTypedById<UISpriteStyle>(m_buttonDataProperties.m_disableStyleId);
    }
    else
    {
        switch (ButtonState().Get())
        {
        case State::Down:
        case State::Pressed:
            return styleManager.GetTypedById<UISpriteStyle>(m_buttonDataProperties.m_pressedStyleId);

        case State::Focused:
            return styleManager.GetTypedById<UISpriteStyle>(m_buttonDataProperties.m_focusedStyleId);

        case State::Hovered:
            return styleManager.GetTypedById<UISpriteStyle>(m_buttonDataProperties.m_hoveredStyleId);

        case State::Normal:
        default:
            return styleManager.GetTypedById<UISpriteStyle>(m_elementDataProperties.styleId);
        }
    }
}

/*static*/ void UIButtonFactory::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _Out_ ButtonDataProperties& buttonDataProperties)
{
    buttonDataProperties.m_disableStyleId = ID(data->GetIfExists<std::string>(UITK_FIELD(disabledStyleId), ""));
    buttonDataProperties.m_pressedStyleId = ID(data->GetIfExists<std::string>(UITK_FIELD(pressedStyleId), ""));
    buttonDataProperties.m_hoveredStyleId = ID(data->GetIfExists<std::string>(UITK_FIELD(hoveredStyleId), ""));
    buttonDataProperties.m_focusedStyleId = ID(data->GetIfExists<std::string>(UITK_FIELD(focusedStyleId), ""));
}

NAMESPACE_ATG_UITK_END
