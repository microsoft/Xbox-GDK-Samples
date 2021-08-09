//--------------------------------------------------------------------------------------
// File: UIStackPanel.h
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
#include "UISliderElement.h"

NAMESPACE_ATG_UITK_BEGIN

enum class StackingOrientation : int
{
    Down = 0x1,
    Up = 0x2,
    Right = 0x4,
    Left = 0x8,
};

//! @anchor StackPanel
//! @class StackPanelDataProperties UISstackPanel.h "UITK/UIStackPanel.h"
//! @brief Horizontal or Vertical stacking container UI element instance properties that are specific to stack panels.
struct StackPanelDataProperties
{
    StackPanelDataProperties() noexcept :
        elementPadding(c_defaultElementPadding),
        stackingOrientation(c_defaultStackingOrientation),
        untypedAlignmentAnchor(0),
        maxVisibleItems(0),
        sliderSubElementId(ID::Default)
    {
    }

    // @subpage stackElementPadding "The padding space to place between elements in ref units"
    float elementPadding;

    // @subpage stackingOrientation "The direction to stack the elements (from the bottom, or from the top)"
    StackingOrientation stackingOrientation;

    //! @private
    /// note that doxygen can't seem to document the union in a custom way
    union {
        // @subpage stackElementAlignment "The alignment with which to place contained stacked elements"
        int untypedAlignmentAnchor;
        //! @private
        HorizontalAnchor horizontalAlignmentAnchor;
        //! @private
        VerticalAnchor verticalAlignmentAnchor;
    };

    // @subpage maxVisibleItems "Number of elements to be visible"
    uint32_t maxVisibleItems;

    //! @subpage sliderSubElementId "Optional sliderSubElementId JSON property as scrollbar"
    ID sliderSubElementId;

    //! @privatesection
    /// defaults for properties if not specified in data

    static constexpr float c_defaultElementPadding = 0.0f;
    static constexpr StackingOrientation c_defaultStackingOrientation = StackingOrientation::Down;
    static constexpr StackingOrientation c_defaultVerticalStackingOrientation = StackingOrientation::Down;
    static constexpr StackingOrientation c_defaultHorizontalStackingOrientation = StackingOrientation::Right;
    static constexpr VerticalAnchor c_defaultVerticalAlignmentAnchor = VerticalAnchor::Top;
    static constexpr HorizontalAnchor c_defaultHorizontalAlignmentAnchor = HorizontalAnchor::Left;
    static constexpr bool c_defaultOverrideChildAlignment = true;
};

//! @private
class UIStackPanel : public UIElement
{
    DECLARE_CLASS_LOG();

    UI_ELEMENT_CLASS_INIT(UIStackPanel, StackPanel);

public:
    static ID VerticalStackPanelClassID()
    {
        static ID id_("VerticalStackPanel");
        return id_;
    }
    static ID HorizontalStackPanelClassID()
    {
        static ID id_("HorizontalStackPanel");
        return id_;
    }

    virtual ~UIStackPanel() = default;

    bool HandleInputEvent(const InputEvent& /*inputEvent*/) override;
    bool HandleGlobalInputState(const UIInputState& /*inputState*/) override;

    void Update(float /*elapsedTimeInS*/) override;
    void Render() override {}

    void PostLoad() override
    {
        WireUpElements();
    }

    void Reset();


    static bool IsHorizontal(StackingOrientation orientation)
    {
        const int c_hflags = (int(StackingOrientation::Left) | int(StackingOrientation::Right));
        return int(orientation) & c_hflags;
    }
    static bool IsVertical(StackingOrientation orientation)
    {
        const int c_vflags = (int(StackingOrientation::Up) | int(StackingOrientation::Down));
        return int(orientation) & c_vflags;
    }

    static bool IsStackPanelId(const ID& id)
    {
        return (id == UIStackPanel::ClassID());
    }

    static constexpr uint32_t c_noUpdate = UINT32_MAX - 1;

protected:
    UIStackPanel(UIManager& uiManager, ID id) : UIElement(uiManager, id)
    {
        m_startIndex = 0;
        m_numChildren = 0;
        m_updateFocus = c_noUpdate;
        m_cachedSlider = nullptr;
    }

    StackingOrientation GetOrientation() const { return m_stackPanelDataProperties.stackingOrientation; }

    void WireUpElements();

    bool IsFocusedAtBeginning();
    bool IsFocusedAtEnd();
    bool HasMorePreviousElements();
    bool HasMoreNextElements();
    bool ShiftPrevious(bool isMouse);
    bool ShiftNext(bool isMouse);

    inline uint32_t GetEndIndex() const { return m_startIndex + m_stackPanelDataProperties.maxVisibleItems - 1; }

protected:
    StackPanelDataProperties m_stackPanelDataProperties;
    Vector2 m_offsetSignVector;
    Anchor m_positioningAndSizingAnchor;
    StackingOrientation m_orientation;

    uint32_t m_startIndex;
    uint32_t m_numChildren;
    uint32_t m_updateFocus;

    std::shared_ptr<UISlider> m_cachedSlider;
};

//! @private
class UIStackPanelFactory : public UIElementFactory<UIStackPanel>
{
protected:
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ StackPanelDataProperties&);

protected:
    UIStackPanel* Create(UIManager& manager, ID id, UIDataPtr data) override
    {
        auto newVerticalStack = UIElementFactory<UIStackPanel>::Create(manager, id, data);
        UIStackPanelFactory::DeserializeDataProperties(
            data,
            newVerticalStack->m_stackPanelDataProperties);
        return newVerticalStack;
    }
};

NAMESPACE_ATG_UITK_END

