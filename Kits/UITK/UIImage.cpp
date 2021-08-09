//--------------------------------------------------------------------------------------
// File: UIImage.cpp
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
#include "UIImage.h"
#include "UIStyleImpl.h"

NAMESPACE_ATG_UITK_BEGIN

/*public:*/

void UIImage::UseTextureData(const uint8_t* wicData, size_t wicDataSize)
{
    auto& styleManager = m_uiManager.GetStyleManager();
    auto& styleRenderer = styleManager.GetStyleRenderer();

    if (styleRenderer.IsValidTextureHandle(m_textureDataHandle))
    {
        styleRenderer.ModifyTextureFromData(m_textureDataHandle, wicData, wicDataSize);
    }
    else
    {
        m_textureDataHandle = styleRenderer.CacheTextureFromData(wicData, wicDataSize);
    }
}

/*protected:*/

/*virtual*/ void UIImage::Render()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIImage_Render");
    PIXSetMarker(PIX_COLOR_DEFAULT, L"Image: %hs", m_style->GetID().AsCStr());
    auto screenRect = GetScreenRectInPixels();
    m_spriteStyle->RenderSprite(screenRect, m_textureDataHandle);
}

/*virtual*/ void UIImage::HandleStyleIdChanged()
{
    UIElement::HandleStyleIdChanged();
    m_spriteStyle = m_uiManager.GetStyleManager().GetTypedById<UISpriteStyle>(m_elementDataProperties.styleId);
}

UIImage* UIImageFactory::Create(UIManager& manager, ID id, UIDataPtr data)
{
    auto newImage = UIElementFactory<UIImage>::Create(manager, id, data);
    newImage->m_spriteStyle = manager.GetStyleManager().GetTypedById<UISpriteStyle>(newImage->m_elementDataProperties.styleId);
    return newImage;
}

NAMESPACE_ATG_UITK_END
