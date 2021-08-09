//--------------------------------------------------------------------------------------
// File: UIVerticalStack.h
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

/// UIVerticalStacks are a purely containing element which renders nothing on its own
/// and simple arranges its direct child elements vertically, one on top of the other
/// with some in-between spacing according to a direction and child horizontal anchoring.
class UIVerticalStack : public UIElement
{
    UI_ELEMENT_CLASS_INIT(UIVerticalStack, VerticalStack)

public:
    enum class StackingDirection
    {
        Downward = 0,
        Upward = 1,
    };

    virtual ~UIVerticalStack() = default;

    // add your type-specific public APIs here...

protected:
    struct VerticalStackDataProperties
    {
        VerticalStackDataProperties() :
            spacingBetweenElements(c_defaultSpacingBetweenElements),
            stackingDirection(c_defaultStackingDirection),
            stackingAnchor(c_defaultStackingAnchor)
        {
        }

        // the vertical space to place between elements in ref units
        float spacingBetweenElements;

        // the direction to stack the elements (from the bottom, or from the top)
        StackingDirection stackingDirection;

        // the positioning and sizing anchor to apply to the stacked elements
        HorizontalAnchor stackingAnchor;

        /// defaults for properties if not specified in data

        static constexpr float c_defaultSpacingBetweenElements = 0.0f;
        static constexpr StackingDirection c_defaultStackingDirection = StackingDirection::Downward;
        static constexpr HorizontalAnchor c_defaultStackingAnchor = HorizontalAnchor::Left;
    };

    VerticalStackDataProperties m_verticalStackDataProperties;

protected:
    UIVerticalStack(UIManager& uiManager, ID id) : UIElement(uiManager, id)
    {
        // add your construction logic here...
    }

    bool HandleInputEvent(const InputEvent& /*inputEvent*/) override { return false; }
    void Update(float /*elapsedTimeInS*/) override;
    void Render() override {}
};

class UIVerticalStackFactory : public UIElementFactory<UIVerticalStack>
{
protected:
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ UIVerticalStack::VerticalStackDataProperties&);

protected:
    UIVerticalStack* Create(UIManager& manager, ID id, UIDataPtr data) override
    {
        auto newVerticalStack = UIElementFactory<UIVerticalStack>::Create(manager, id, data);
        UIVerticalStackFactory::DeserializeDataProperties(
            data,
            newVerticalStack->m_verticalStackDataProperties);
        return newVerticalStack;
    }
};
NAMESPACE_ATG_UITK_END
