//--------------------------------------------------------------------------------------
// File: UIVerticalStack.cpp
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
#include "UIVerticalStack.h"
#include "UIKeywords.h"

NAMESPACE_ATG_UITK_BEGIN

/*virtual*/ void UIVerticalStack::Update(float /*elapsedTimeInS*/)
{
    // adjust all child rectangles according to visibility, etc.

    float stackVerticalAnchor = 0.0f;

    Anchor positioningAndSizingAnchor{ m_verticalStackDataProperties.stackingAnchor, VerticalAnchor::Top };
    float verticalPositionSign = 1.0f;

    switch (m_verticalStackDataProperties.stackingDirection)
    {
    case StackingDirection::Downward:
        positioningAndSizingAnchor.Vertical = VerticalAnchor::Top;
        verticalPositionSign = 1.0f;
        break;

    case StackingDirection::Upward:
        positioningAndSizingAnchor.Vertical = VerticalAnchor::Bottom;
        verticalPositionSign = -1.0f;
        break;

    default:
        throw std::exception("Unable to handle stacking direction in UIVerticalStack::Update()");
    }

    for (auto& child : m_children)
    {
        if (!child->IsVisible())
        {
            continue;
        }

        child->SetPositioningAnchor(positioningAndSizingAnchor);
        child->SetSizingAnchor(positioningAndSizingAnchor);
        child->SetRelativePositionInRefUnits(Vector2{ 0.0f, stackVerticalAnchor });
        stackVerticalAnchor += verticalPositionSign *
            (m_verticalStackDataProperties.spacingBetweenElements + child->GetRelativeSizeInRefUnits().y);
    }
}

ENUM_LOOKUP_TABLE(UIVerticalStack::StackingDirection,
    ID_ENUM_PAIR(UITK_VALUE(downward), UIVerticalStack::StackingDirection::Downward),
    ID_ENUM_PAIR(UITK_VALUE(upward), UIVerticalStack::StackingDirection::Upward)
)

/*static*/ void UIVerticalStackFactory::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _Out_ UIVerticalStack::VerticalStackDataProperties& verticalStackDataProperties)
{
    // the spacing between stacked elements
    data->GetTo(UITK_FIELD(spacingBetweenElements), verticalStackDataProperties.spacingBetweenElements);

    // the horizontal anchor to apply to stacked elements
    data->GetTo(UITK_FIELD(stackingAnchor), verticalStackDataProperties.stackingAnchor);

    // the stacking direction of the stack
    ID stackingDirectionID;
    if (data->GetTo(UITK_FIELD(stackingDirection), stackingDirectionID))
    {
        UIEnumLookup(stackingDirectionID, verticalStackDataProperties.stackingDirection);
    }
}

NAMESPACE_ATG_UITK_END
