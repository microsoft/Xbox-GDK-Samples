//--------------------------------------------------------------------------------------
// File: UIElement.cpp
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
#include "UIElement.h"
#include "UIMath.h"
#include "UIStyleImpl.h"

NAMESPACE_ATG_UITK_BEGIN

INITIALIZE_CLASS_LOG_TRACE(UIElement);

/*public:*/

/*virtual*/ bool UIElement::HitTestPixels(int x, int y)
{
    return GetScreenRectInPixels().Contains(long(x), long(y));
}

/*virtual*/ bool UIElement::HitTestRefUnits(int x, int y)
{
    return GetScreenRectInRefUnits().Contains(long(x), long(y));
}

Rectangle UIElement::GetPaddedRectInPixels()
{
    auto compute = [&]()
    {
        return UIMath::GetScaledRectangle(
            GetPaddedRectInRefUnits(), m_uiManager.GetRefUnitsToPixelsScale());
    };

    return m_paddedRectInPixels.GetValue(compute);
}

Rectangle UIElement::GetPaddedRectInRefUnits()
{
    auto compute = [&]()
    {
        return m_style ? m_style->SubtractPaddingFromRect(GetScreenRectInRefUnits()) : GetScreenRectInRefUnits();
    };

    return m_paddedRectInRefUnits.GetValue(compute);
}

Rectangle UIElement::GetMarginedRectInPixels()
{
    auto compute = [&]()
    {
        return UIMath::GetScaledRectangle(
            GetMarginedRectInRefUnits(), m_uiManager.GetRefUnitsToPixelsScale());
    };

    return m_marginedRectInPixels.GetValue(compute);
}

Rectangle UIElement::GetMarginedRectInRefUnits()
{
    auto compute = [&]()
    {
        return m_style ? m_style->AddMarginToRect(GetScreenRectInRefUnits()) : GetScreenRectInRefUnits();
    };

    return m_marginedRectInRefUnits.GetValue(compute);
}

/*virtual*/ Rectangle UIElement::GetScreenRectInRefUnits()
{
    auto compute = [&]()
    {
        if (m_parent)
        {
            PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIElement_GetScreenRectInRefUnits");
            assert(m_style);

            Vector2 sizeInRefUnits = GetSizeInRefUnits();

            auto parentRect = m_parent->GetPaddedRectInRefUnits();

            // calculate our rectangle based on our anchoring, position, and size

            float positionX = m_elementDataProperties.relativePosition.x;
            float positionY = m_elementDataProperties.relativePosition.y;

            switch (m_elementDataProperties.positioningAnchor.Horizontal)
            {
            case HorizontalAnchor::Left: positionX += float(parentRect.x) + float(m_style->Margin().left); break;
            case HorizontalAnchor::Center: positionX += parentRect.Center().x; break;
            case HorizontalAnchor::Right: positionX += float(parentRect.x + parentRect.width) - float(m_style->Margin().right); break;
            default:
                throw UIException(m_id, "Unknown positioning horizontal anchor.");
            }

            switch (m_elementDataProperties.positioningAnchor.Vertical)
            {
            case VerticalAnchor::Top: positionY += float(parentRect.y) + float(m_style->Margin().top); break;
            case VerticalAnchor::Middle: positionY += parentRect.Center().y; break;
            case VerticalAnchor::Bottom: positionY += float(parentRect.y + parentRect.height) - float(m_style->Margin().bottom); break;
            default:
                throw UIException(m_id, "Unknown positioning vertical anchor.");
            }

            switch (m_elementDataProperties.sizingAnchor.Horizontal)
            {
            case HorizontalAnchor::Left: /* no change needed */ break;
            case HorizontalAnchor::Center: positionX -= sizeInRefUnits.x * 0.5f; break;
            case HorizontalAnchor::Right: positionX -= sizeInRefUnits.x;  break;
            default:
                throw UIException(m_id, "Unknown sizing horizontal anchor.");
            }

            switch (m_elementDataProperties.sizingAnchor.Vertical)
            {
            case VerticalAnchor::Top: /* no changed needed */ break;
            case VerticalAnchor::Middle: positionY -= sizeInRefUnits.y * 0.5f; break;
            case VerticalAnchor::Bottom: positionY -= sizeInRefUnits.y; break;
            default:
                throw UIException(m_id, "Unknown sizing vertical anchor.");
            }

            auto result = Rectangle(
                long(positionX),
                long(positionY),
                long(sizeInRefUnits.x),
                long(sizeInRefUnits.y));

            UILOG_TRACE_SCOPED("GetScreenRectInRefUnits", "%s -> P: %c%c S: %c%c | (%d, %d) x (%d, %d)",
                GetID().AsCStr(),
                Anchor::DebugShortName(m_elementDataProperties.positioningAnchor.Horizontal),
                Anchor::DebugShortName(m_elementDataProperties.positioningAnchor.Vertical),
                Anchor::DebugShortName(m_elementDataProperties.sizingAnchor.Horizontal),
                Anchor::DebugShortName(m_elementDataProperties.sizingAnchor.Vertical),
                int(result.x),
                int(result.y),
                int(result.x + result.width),
                int(result.y + result.height));

            return result;
        }
        else
        {
            UILOG_TRACE_SCOPED("GetScreenRectInRefUnits", "%s", GetID().AsCStr());
            return m_uiManager.GetScreenRectInRefUnits();
        }
    };

    return m_screenRectInPixels.GetValue(compute);
}

/*virtual*/ Rectangle UIElement::GetScreenRectInPixels()
{
    auto compute = [&]() {
        return UIMath::GetScaledRectangle(
            GetScreenRectInRefUnits(), m_uiManager.GetRefUnitsToPixelsScale());
    };

    return m_screenRectInRefUnits.GetValue(compute);
}

Vector2 UIElement::GetSizeInRefUnits()
{
    auto compute = [&]()
    {
        if (m_elementDataProperties.relativeSize == ElementDataProperties::c_undefinedSize)
        {
            auto paddedRect = m_parent->GetPaddedRectInRefUnits();
            return Vector2{ float(paddedRect.width), float(paddedRect.height) };
        }
        else
        {
            return m_elementDataProperties.relativeSize;
        }
    };

    return m_sizeInPixels.GetValue(compute);
}

void UIElement::AddChild(UIElementPtr child)
{
    m_uiManager.AttachTo(child, shared_from_this(), false);
}

void UIElement::AddSubElement(UIElementPtr subElement)
{
    m_uiManager.AttachTo(subElement, shared_from_this(), true);
}

UIElementPtr UIElement::AddChildFromLayout(const std::string& layoutFilePath)
{
    auto element = m_uiManager.LoadLayoutFromFile(layoutFilePath);
    AddChild(element);
    return element;
}

UIElementPtr UIElement::AddChildFromPrefab(const std::string& prefabFilePath)
{
    auto element = m_uiManager.InstantiatePrefab(prefabFilePath);
    AddChild(element);
    return element;
}

bool UIElement::IsAncestorOfMe(UIElementPtr element) const
{
    assert(element != nullptr);

    auto ancestor = GetParent();
    while (ancestor && ancestor != element)
    {
        ancestor = ancestor->GetParent();
    }
    return ancestor && ancestor == element;
}

/*protected:*/

/*virtual*/ void UIElement::PostRender()
{
    m_style->PostRender();
}

/*virtual*/ void UIElement::HandleStyleIdChanged()
{
    m_style = m_uiManager.GetStyleManager().GetById(m_elementDataProperties.styleId);
}

void UIElement::Clear()
{
    m_parent.reset();
    for (auto& child : m_children)
    {
        child->Clear();
        child.reset();
    }
    m_children.clear();
}

/*public:*/

/*static*/ void UIElementFactoryBase::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _In_ UIStyleManager& styleManager,
    _Out_ ElementDataProperties& elementProperties)
{
    // base class element properties loaded in here...

    elementProperties.visible = data->GetIfExists<bool>(
        UITK_FIELD(visible),
        ElementDataProperties::c_defaultVisibility);

    elementProperties.enabled = data->GetIfExists<bool>(
        UITK_FIELD(enabled),
        ElementDataProperties::c_defaultEnabling);

    elementProperties.focusable = data->GetIfExists<bool>(
        UITK_FIELD(focusable),
        ElementDataProperties::c_defaultFocusable);

    if (!data->GetTo(UITK_FIELD(styleId), elementProperties.styleId))
    {
        if (data->Exists(UITK_FIELD(style)))
        {
            elementProperties.styleId = styleManager.LoadStyleFromData(
                data->GetIfExists<ID>(UITK_FIELD(id), ID::Default),
                data->GetObjectValue(UITK_FIELD(style)),
                true);
        }
        else
        {
            // TODO: Fix this - this is the way it should work, but we need to register
            // a styleid with the name "nullstyle" that maps to the null style.
            elementProperties.styleId = UINullStyle::ClassId();
        }
    }

    data->GetTo(UITK_FIELD(positioningAnchor), elementProperties.positioningAnchor);
    data->GetTo(UITK_FIELD(position), elementProperties.relativePosition);
    data->GetTo(UITK_FIELD(sizingAnchor), elementProperties.sizingAnchor);
    data->GetTo(UITK_FIELD(size), elementProperties.relativeSize);
}

NAMESPACE_ATG_UITK_END
