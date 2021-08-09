//--------------------------------------------------------------------------------------
// File: UIPipStrip.cpp
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
#include "UIPipStrip.h"
#include "UIStyleImpl.h"

NAMESPACE_ATG_UITK_BEGIN

/*virtual*/ void UIPipStrip::Render()
{
    SetPipCount(m_pipStripDataProperties.m_numberOfPips);

    if (m_pipStripDataProperties.m_numberOfPips == 0)
    {
        return;
    }

    auto& styleManager = m_uiManager.GetStyleManager();
    auto inactiveSpriteStyle = styleManager.GetTypedById<UISpriteStyle>(m_pipStripDataProperties.m_inactivePipStyleId);
    auto activeSpriteStyle = styleManager.GetTypedById<UISpriteStyle>(m_pipStripDataProperties.m_activePipStyleId);

    auto screenRectOfPips = GetScreenRectInPixels();
    auto screenRectOfRenderPip = screenRectOfPips;

    auto leftAdder = long(0);
    auto topAdder = long(0);

    auto spacingInPixels = long(m_pipStripDataProperties.m_pipSpacing * m_uiManager.GetRefUnitsToPixelsScale());

    switch (m_pipStripDataProperties.m_stripOrientation)
    {
    case PipStripOrientation::LeftToRight:
        screenRectOfRenderPip.width = long(m_pipStripDataProperties.m_pipSize.x * m_uiManager.GetRefUnitsToPixelsScale());
        leftAdder = screenRectOfRenderPip.width + spacingInPixels;
        break;

    case PipStripOrientation::TopToBottom:
        screenRectOfRenderPip.height = long(m_pipStripDataProperties.m_pipSize.y * m_uiManager.GetRefUnitsToPixelsScale());
        topAdder = screenRectOfRenderPip.height + spacingInPixels;
        break;

    default:
        throw std::exception("Unhandled pip strip orientation in UIPipStrip::Render()");
    }

    for (auto pipIndex = uint32_t(0); pipIndex < m_pipStripDataProperties.m_numberOfPips; ++pipIndex)
    {
        if (pipIndex == m_activePipIndex)
        {
            activeSpriteStyle->RenderSprite(screenRectOfRenderPip);
        }
        else
        {
            inactiveSpriteStyle->RenderSprite(screenRectOfRenderPip);
        }

        screenRectOfRenderPip.x += leftAdder;
        screenRectOfRenderPip.y += topAdder;
    }
}

ENUM_LOOKUP_TABLE(PipStripOrientation,
    ID_ENUM_PAIR(UITK_VALUE(leftToRight), PipStripOrientation::LeftToRight),
    ID_ENUM_PAIR(UITK_VALUE(topToBottom), PipStripOrientation::TopToBottom)
)

/*static*/ void UIPipStripFactory::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _Out_ PipStripDataProperties& pipStripDataProperties)
{
    // the sprite style to render an inactive pip
    data->GetTo(Keywords::c_inactivePipStyleId, pipStripDataProperties.m_inactivePipStyleId);

    // the sprite style to render an active pip
    data->GetTo(Keywords::c_activePipStyleId, pipStripDataProperties.m_activePipStyleId);

    // the initial number of pips to support
    data->GetTo(Keywords::c_numberOfPips, pipStripDataProperties.m_numberOfPips);

    // the size of a single pip
    data->GetTo(Keywords::c_pipSize, pipStripDataProperties.m_pipSize);

    // the amount of spacing between pips
    data->GetTo(Keywords::c_pipSpacing, pipStripDataProperties.m_pipSpacing);

    // the orientation of the pip strip
    ID stripOrientationID;
    if (data->GetTo(UITK_FIELD(sliderOrientation), stripOrientationID))
    {
        UIEnumLookup(stripOrientationID, pipStripDataProperties.m_stripOrientation);
    }
}

NAMESPACE_ATG_UITK_END
