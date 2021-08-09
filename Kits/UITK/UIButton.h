//--------------------------------------------------------------------------------------
// File: UIButton.h
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

// #include "UIManager.h"
// #include "UIStyle.h"
#include "UIEvent.h"
#include "UIElement.h"

NAMESPACE_ATG_UITK_BEGIN

//! @anchor Button
//! @class ButtonDataProperties UIButton.h "UITK/UIButton.h"
//! @brief Button UI element instance properties specific to buttons.
struct ButtonDataProperties
{
    ButtonDataProperties()
    {
    }

    //! @subpage disabledStyleId "disabledStyleId JSON property"
    ID m_disableStyleId; // TODO: default to a default style from UIStyleManager?
    //! @subpage focusedStyleId "focusedStyleId JSON property"
    ID m_focusedStyleId; // TODO: default to a default style from UIStyleManager?
    //! @subpage pressedStyleId "pressedStyleId JSON property"
    ID m_pressedStyleId; // TODO: default to a default style from UIStyleManager?
    //! @subpage hoveredStyleId "hoveredStyleId JSON property"
    ID m_hoveredStyleId; // TODO: default to a default style from UIStyleManager?
};

//! @private
/// <summary> UIButtons are intended to be a simple clickable element.  UIButtons 
/// will be show-able & enable-able like all UIElements, but also should 
/// </summary> handle highlighted-ness and focused-ness on its own.
class UIButton : public UIElement
{
    UI_ELEMENT_CLASS_INIT(UIButton, Button)

public:
    enum class State : uint32_t
    {
        Normal = 0,
        Hovered = 1,
        Focused = 2,
        Pressed = 3,
        Down = 4
    };

    virtual ~UIButton() = default;

    bool Clicked() const { return ButtonState().Get() == State::Pressed; }

    // events that can be listened to rather than polled...

    UIStateEvent<UIButton, State>& ButtonState()
    {
        return m_buttonState;
    }

    const UIStateEvent<UIButton, State>& ButtonState() const
    {
        return m_buttonState;
    }

protected:

    ButtonDataProperties m_buttonDataProperties;
    UIStateEvent<UIButton, State> m_buttonState;

protected:
    UIButton(class UIManager& uiManager, ID id) : UIElement(uiManager, id)
    {
        ButtonState().ClearTo(State::Normal);
    }

    bool HandleInputEvent(const InputEvent& inputEvent) override;
    void Update(float) override {}
    void Render() override;
    void PostRender() override;

    void SetCurrentState(State state)
    {
        ButtonState().Set(state, this);
    }

    std::shared_ptr<class UISpriteStyle> GetCurrentStyle();
};

//! @private
// A factory for creating UIButtons from data to be managed by
// the provided UIManager.
class UIButtonFactory : public UIElementFactory<UIButton>
{
protected:
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ ButtonDataProperties&);

protected:
    /*virtual*/ UIButton* Create(UIManager& manager, ID id, UIDataPtr data)
    {
        auto newButton = UIElementFactory<UIButton>::Create(manager, id, data);
        UIButtonFactory::DeserializeDataProperties(
            data,
            newButton->m_buttonDataProperties);
        return newButton;
    }
};

NAMESPACE_ATG_UITK_END
