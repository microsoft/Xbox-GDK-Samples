//--------------------------------------------------------------------------------------
// File: UICheckBox.h
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

#include "UIManager.h"
#include "UIStyle.h"
#include "UIEvent.h"
#include "UIElement.h"

NAMESPACE_ATG_UITK_BEGIN

class UICheckBox : public UIElement
{
    UI_ELEMENT_CLASS_INIT(UICheckBox, CheckBox)

public:
    enum class State
    {
        Normal = 0,
        Checked = 1,
        Hovered = 2,
        Focused = 4
    };

    virtual ~UICheckBox() = default;

    bool Checked() const { return uint32_t(m_checkboxState.Get()) & uint32_t(State::Checked); }
    bool Hovered() const { return uint32_t(m_checkboxState.Get()) & uint32_t(State::Hovered); }
    bool Focused() const { return uint32_t(m_checkboxState.Get()) & uint32_t(State::Focused); }

    // events that can be listened to rather than polled...

    UIStateEvent<UICheckBox, State>& CheckBoxState()
    {
        return m_checkboxState;
    }

    const UIStateEvent<UICheckBox, State>& CheckBoxState() const
    {
        return m_checkboxState;
    }

public:
    // static text specific data-driven properties

    struct CheckBoxDataProperties
    {
        CheckBoxDataProperties() : checked(false)
        {
        }

        /// defaults for properties if not specified in data
        ID disableStyleId; // TODO: default to a default style from UIStyleManager?
        ID focusedStyleId; // TODO: default to a default style from UIStyleManager?
        ID checkedStyleId; // TODO: default to a default style from UIStyleManager?
        ID hoveredStyleId; // TODO: default to a default style from UIStyleManager?
        bool checked;
    };

public:
    void Initialize(CheckBoxDataProperties&& props) { m_checkboxDataProperties = std::move(props); }

    CheckBoxDataProperties m_checkboxDataProperties;
    UIStateEvent<UICheckBox, State> m_checkboxState;

protected:
    UICheckBox(UIManager& uiManager, ID id) : UIElement(uiManager, id), m_checkboxDataProperties()
    {
        if (m_checkboxDataProperties.checked)
        {
            m_checkboxState.ClearTo(State::Normal);
        }
    }

    bool HandleInputEvent(const InputEvent& inputEvent) override;
    void Update(float) override {}
    void Render() override;

    void SetCurrentState(State state)
    {
        m_checkboxState.Set(state, this);
    }
};

/// A factory for creating UIButtons from data to be managed by
/// the provided UIManager.
class UICheckBoxFactory : public UIElementFactory<UICheckBox>
{
protected:
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ UICheckBox::CheckBoxDataProperties& props);

protected:
    /*virtual*/ UICheckBox* Create(UIManager& manager, ID id, UIDataPtr data)
    {
        auto newCheckBox = UIElementFactory<UICheckBox>::Create(manager, id, data);
        UICheckBoxFactory::DeserializeDataProperties(
            data,
            newCheckBox->m_checkboxDataProperties);
        return newCheckBox;
    }
};

NAMESPACE_ATG_UITK_END
