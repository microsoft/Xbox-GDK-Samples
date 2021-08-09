//--------------------------------------------------------------------------------------
// File: UIPanel.cpp
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
#include "UIPanel.h"
#include "UIStyleImpl.h"

NAMESPACE_ATG_UITK_BEGIN

/*protected:*/

/*virtual*/ void UIPanel::Render()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIPanel_Render");
    UIStyleManager& styleManager = m_uiManager.GetStyleManager();

    if (m_panelDataProperties.m_clipChildren)
    { 
        auto paddedRect = GetPaddedRectInPixels();
        m_scissorStackIndex = styleManager.GetStyleRenderer().IntersectScissorRectangle(paddedRect);
    }

    if (m_spriteStyle)
    {
        auto screenRect = GetScreenRectInPixels();
        m_spriteStyle->RenderSprite(screenRect);
    }
}

/*virtual*/ void UIPanel::PostRender()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIPanel_PostRender");
    if (m_panelDataProperties.m_clipChildren)
    {
        UIStyleManager& styleManager = m_uiManager.GetStyleManager();
        styleManager.GetStyleRenderer().PopScissorRectangle(m_scissorStackIndex);
    }

    UIElement::PostRender();
}

/*virtual*/ void UIPanel::HandleStyleIdChanged()
{
    UIElement::HandleStyleIdChanged();

    m_spriteStyle = nullptr;
    if (m_style->GetClassID() == UISpriteStyle::ClassId())
    {
        m_spriteStyle = m_uiManager.GetStyleManager().GetTypedById<UISpriteStyle>(m_elementDataProperties.styleId);
    }
}

/*protected:*/

/*static*/ void UIPanelFactory::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _Out_ PanelDataProperties& panelDataProperties)
{
    panelDataProperties.m_clipChildren = data->GetIfExists(UITK_FIELD(clipChildren), PanelDataProperties::c_defaultClipChildren);
}

UIPanel* UIPanelFactory::Create(UIManager& manager, ID id, UIDataPtr data)
{
	auto newPanel = UIElementFactory<UIPanel>::Create(manager, id, data);
	UIPanelFactory::DeserializeDataProperties(
		data,
		newPanel->m_panelDataProperties);

	if (newPanel->m_style->GetClassID() == UISpriteStyle::ClassId())
	{
		newPanel->m_spriteStyle = manager.GetStyleManager().GetTypedById<UISpriteStyle>(newPanel->m_elementDataProperties.styleId);
	}

	return newPanel;
}

NAMESPACE_ATG_UITK_END
