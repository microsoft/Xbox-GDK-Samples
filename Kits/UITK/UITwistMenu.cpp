//--------------------------------------------------------------------------------------
// File: UITwistMenu.cpp
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
#include "UITwistMenu.h"
#include "UIStaticText.h"
#include "UIButton.h"
#include "UIPipStrip.h"
#include "UIKeywords.h"

NAMESPACE_ATG_UITK_BEGIN

const UIDisplayString& UITwistMenu::GetItemDisplayString(uint32_t index) const
{
    if (index < GetItemCount())
    {
        return m_twistMenuDataProperties.m_items[size_t(index)];
    }

    return emptyItemDisplayString;
}

uint32_t UITwistMenu::AddItem(UIConstDisplayString displayString)
{
    if (m_twistMenuDataProperties.IsDefaultList())
    {
        m_twistMenuDataProperties.m_items.clear();
    }

    uint32_t newIndex = uint32_t(m_twistMenuDataProperties.m_items.size());
    m_twistMenuDataProperties.m_items.emplace_back(displayString);
    RefreshPips();
    return newIndex;
}

void UITwistMenu::ClearItems()
{
    m_twistMenuDataProperties.MakeDefaultList();
    SelectedItemState().ClearTo(SelectedItem());
    RefreshPips();
}

/*protected:*/

/*virtual*/ bool UITwistMenu::HandleInputEvent(const InputEvent& /*inputEvent*/)
{
    // we do not need to handle input in the twist menu just yet
    return false;
}

/*virtual*/ void UITwistMenu::Update(float /*elapsedTimeInS*/)
{
    // we do not need to update anything specific to the twist menu
}

/*virtual*/ void UITwistMenu::Render()
{
    // we do not need to render anything specific to the twist menu
}

void UITwistMenu::SetSelectedItem(uint32_t selectedIndex)
{
    RefreshPips();

    if (selectedIndex < GetItemCount())
    {
        auto currentState = SelectedItemState().Get();
        if (currentState.CurrentItem != selectedIndex)
        {
            if (m_cachedDisplayText)
            {
                m_cachedDisplayText->SetDisplayText(
                    GetItemDisplayString(selectedIndex)
                );
            }

            if (m_cachedPipStrip)
            {
                m_cachedPipStrip->SetActivePipIndex(selectedIndex);
            }

            SelectedItemState().Set(
                UITwistMenu::SelectedItem{ selectedIndex, currentState.CurrentItem },
                this
            );
        }
    }
}

void UITwistMenu::WireUpElements()
{
    if (!m_cachedDisplayText)
    {
        m_cachedDisplayText = GetTypedSubElementById<UIStaticText>(
            m_twistMenuDataProperties.m_displayTextSubElementId);
        m_cachedDisplayText->SetDisplayText(GetCurrentDisplayString());
    }

    if (!m_cachedLeftButton)
    {
        m_cachedLeftButton = GetTypedSubElementById<UIButton>(
            m_twistMenuDataProperties.m_leftButtonSubElementId);
        m_cachedLeftButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
            {
                DecrementSelectedItem();
            });
    }

    if (!m_cachedRightButton)
    {
        m_cachedRightButton = GetTypedSubElementById<UIButton>(
            m_twistMenuDataProperties.m_rightButtonSubElementId);
        m_cachedRightButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
            {
                IncrementSelectedItem();
            });
    }

    if (!m_cachedPipStrip)
    {
        m_cachedPipStrip = GetTypedSubElementById<UIPipStrip>(
            m_twistMenuDataProperties.m_pipStripSubElementId);
        RefreshPips();
        m_cachedPipStrip->SetActivePipIndex(GetCurrentItemIndex());
    }
}

void UITwistMenu::RefreshPips()
{
    if (m_cachedPipStrip)
    {
        m_cachedPipStrip->SetPipCount(GetItemCount());
    }
}

/*protected:*/

/*static*/ void UITwistMenuFactory::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _Out_ TwistMenuDataProperties& twistMenuDataProperties)
{
    data->GetTo<ID>(UITK_FIELD(leftButtonSubElementId), twistMenuDataProperties.m_leftButtonSubElementId);
    data->GetTo<ID>(UITK_FIELD(rightButtonSubElementId), twistMenuDataProperties.m_rightButtonSubElementId);
    data->GetTo<ID>(UITK_FIELD(displayTextSubElementId), twistMenuDataProperties.m_displayTextSubElementId);
    data->GetTo<ID>(UITK_FIELD(pipStripSubElementId), twistMenuDataProperties.m_pipStripSubElementId);
    data->GetTo(UITK_FIELD(items), twistMenuDataProperties.m_items);
    data->GetTo(UITK_FIELD(infinitelyCycleItems), twistMenuDataProperties.m_cycleItems);
}

NAMESPACE_ATG_UITK_END
