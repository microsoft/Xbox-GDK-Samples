//--------------------------------------------------------------------------------------
// File: UITwistMenu.h
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
#include "UIEvent.h"
#include "UIElement.h"

NAMESPACE_ATG_UITK_BEGIN

//! @anchor TwistMenu
//! @class TwistMenuDataProperties UITwistMenu.h "UITK/UITwistMenu.h"
//! @brief TwistMenu UI element instance properties specific to twist menus.
struct TwistMenuDataProperties
{
    //! @private
    using UIDisplayStringList = std::vector<UIDisplayString>;

    TwistMenuDataProperties() :
        m_items{ c_defaultItem },
        m_cycleItems(c_defaultCycleItems)
    {
    }

    //! @private
    bool IsDefaultList() const
    {
        return m_items.size() == size_t(1) && m_items[0] == c_defaultItem;
    }

    //! @private
    void MakeDefaultList()
    {
        m_items.clear();
        m_items.emplace_back(TwistMenuDataProperties::c_defaultItem);
    }

    // element used to decrement the current selected item
    //! @subpage leftButtonSubElementId "leftButtonSubElementId JSON property"
    ID m_leftButtonSubElementId;

    // element used to display the current selected item
    //! @subpage displayTextSubElementId "displayTextSubElementId JSON property"
    ID m_displayTextSubElementId;

    // element used to increment the current selected item
    //! @subpage rightButtonSubElementId "rightButtonSubElementId JSON property"
    ID m_rightButtonSubElementId;

    // optional element to show the selected item with the total count
    //! @subpage pipStripSubElementId "pipStripSubElementId JSON property"
    ID m_pipStripSubElementId;

    // the actual items to select between
    //! @subpage items "items JSON property"
    UIDisplayStringList m_items;

    // whether or not to infinitely cycle through the items
    //! @subpage infinitelyCycleItems "infinitelyCycleItems JSON property"
    bool m_cycleItems;

    //! @privatesection
    /// defaults for properties if not specified in data
    static constexpr UIConstDisplayString c_defaultItem = u8"<empty>";
    static constexpr bool c_defaultCycleItems = false;
};


//! @private
/// UITwistMenus utilize 2 buttons and 1 static text sub elements to
/// accomplish allowing the user to change a selection from a set of
/// available options.
/// UITwistMenus will be show-able & enable-able like all UIElements, but also
/// can support highlighted-ness and focused-ness for its sub elements.
class UITwistMenu : public UIElement
{
    UI_ELEMENT_CLASS_INIT(UITwistMenu, TwistMenu)

public:
    struct SelectedItem
    {
        SelectedItem() : CurrentItem(0), PreviousItem(0) {}
        SelectedItem(uint32_t currentItem, uint32_t previousItem) : CurrentItem(currentItem), PreviousItem(previousItem) {}

        bool operator==(const SelectedItem& other) const
        {
            return CurrentItem == other.CurrentItem && PreviousItem == other.PreviousItem;
        }

        bool operator!=(const SelectedItem& other) const
        {
            return !(CurrentItem == other.CurrentItem && PreviousItem == other.PreviousItem);
        }

        uint32_t CurrentItem;
        uint32_t PreviousItem;
    };

    virtual ~UITwistMenu() = default;

    bool ChangedSelectedItem() const
    {
        auto currentState = SelectedItemState().Get();
        return currentState.CurrentItem != currentState.PreviousItem;
    }

    void IncrementSelectedItem()
    {
        if (m_twistMenuDataProperties.m_cycleItems || SelectedItemState().Get().CurrentItem < (GetItemCount() - 1))
        {
            SetSelectedItem(
                (SelectedItemState().Get().CurrentItem + 1) % GetItemCount()
            );
        }
    }

    void DecrementSelectedItem()
    {
        if (m_twistMenuDataProperties.m_cycleItems || SelectedItemState().Get().CurrentItem > 0)
        {
            SetSelectedItem(
                (SelectedItemState().Get().CurrentItem + GetItemCount() - 1) % GetItemCount()
            );
        }
    }

    UIStateEvent<UITwistMenu, SelectedItem>& SelectedItemState()
    {
        return m_selectedItemState;
    }

    const UIStateEvent<UITwistMenu, SelectedItem>& SelectedItemState() const
    {
        return m_selectedItemState;
    }

    uint32_t GetItemCount() const
    {
        return uint32_t(m_twistMenuDataProperties.m_items.size());
    }

    const UIDisplayString& GetItemDisplayString(uint32_t index) const;

    uint32_t GetCurrentItemIndex() const
    {
        return SelectedItemState().Get().CurrentItem;
    }

    const UIDisplayString& GetCurrentDisplayString() const
    {
        return GetItemDisplayString(GetCurrentItemIndex());
    }

    uint32_t AddItem(UIConstDisplayString displayString);
    void ClearItems();

protected:

    TwistMenuDataProperties m_twistMenuDataProperties;
    std::shared_ptr<class UIButton> m_cachedLeftButton;
    std::shared_ptr<class UIButton> m_cachedRightButton;
    std::shared_ptr<class UIStaticText> m_cachedDisplayText;
    std::shared_ptr<class UIPipStrip> m_cachedPipStrip;
    UIStateEvent<UITwistMenu, SelectedItem> m_selectedItemState;

protected:
    UITwistMenu(UIManager& uiManager, ID id) : UIElement(uiManager, id)
    {
    }

    void PostLoad() override
    {
        WireUpElements();
    }

    void ResetEventState() override
    {
        auto currentState = SelectedItemState().Get();
        SelectedItemState().ClearTo(
            SelectedItem{ currentState.CurrentItem, currentState.CurrentItem }
        );
    }

    bool HandleInputEvent(const InputEvent& inputEvent) override;
    void Update(float /*elapsedTimeInS*/) override;
    void Render() override;

    void SetSelectedItem(uint32_t selectedIndex);
    void WireUpElements();
    void RefreshPips();
};

//! @private
/// A factory for creating UITwistMenus from data to be managed by
/// the provided UIManager.
class UITwistMenuFactory : public UIElementFactory<UITwistMenu>
{
protected:
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ TwistMenuDataProperties&);

protected:
    /*virtual*/ UITwistMenu* Create(UIManager& manager, ID id, UIDataPtr data)
    {
        auto newTwistMenu = UIElementFactory<UITwistMenu>::Create(manager, id, data);
        UITwistMenuFactory::DeserializeDataProperties(
            data,
            newTwistMenu->m_twistMenuDataProperties);
        return newTwistMenu;
    }
};

NAMESPACE_ATG_UITK_END
