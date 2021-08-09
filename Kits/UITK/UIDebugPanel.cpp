//--------------------------------------------------------------------------------------
// File: UIDebugPanel.cpp
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
#include "UIDebugPanel.h"
#include "UIStyleImpl.h"

NAMESPACE_ATG_UITK_BEGIN

void UIDebugPanel::Render()
{
    m_basicStyle->BeforeRender();
    auto& sr = m_uiManager.GetStyleManager().GetStyleRenderer();
    auto rect = GetScreenRectInPixels();
    auto index = sr.IntersectScissorRectangle(rect);
    sr.RenderGrid(
        { float(rect.x), float(rect.y) },
        { float(rect.width), float(rect.height) },
        m_debugPanelDataProperties.m_gridSize);
    sr.PopScissorRectangle(index);
    m_basicStyle->AfterRender();
}

void UIDebugPanel::HandleStyleIdChanged()
{
    UIElement::HandleStyleIdChanged();

    m_basicStyle = nullptr;
    if (m_style->GetClassID() == UIBasicStyle::ClassId())
    {
        m_basicStyle = m_uiManager.GetStyleManager().GetTypedById<UIBasicStyle>(m_elementDataProperties.styleId);
        UI_ASSERT(m_basicStyle, "DebugPanel must use BasicStyle for its style class.");
    }
}

/*static*/ void UIDebugPanelFactory::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _Out_ UIDebugPanel::DebugPanelDataProperties& panelDataProperties)
{
    data->GetTo(UITK_FIELD(gridSize), panelDataProperties.m_gridSize);
}

UIDebugPanel* UIDebugPanelFactory::Create(UIManager& manager, ID id, UIDataPtr data)
{
    auto newPanel = UIElementFactory<UIDebugPanel>::Create(manager, id, data);
    UIDebugPanelFactory::DeserializeDataProperties(
        data,
        newPanel->m_debugPanelDataProperties);

    if (newPanel->m_style->GetClassID() == UIBasicStyle::ClassId())
    {
        newPanel->m_basicStyle = manager.GetStyleManager().GetTypedById<UIBasicStyle>(newPanel->m_elementDataProperties.styleId);
        UI_ASSERT(newPanel->m_basicStyle, "DebugPanel must use BasicStyle for its style class.");
    }

    return newPanel;
}

NAMESPACE_ATG_UITK_END
