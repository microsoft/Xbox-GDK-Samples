//--------------------------------------------------------------------------------------
// File: UIImage.h
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
#include "UIStyle.h"
#include "UIEvent.h"
#include "UIElement.h"

NAMESPACE_ATG_UITK_BEGIN

/// A UIImage element is intended to be a simple element for displaying
/// a colored & textured rectangle on screen with a particular Sprite
/// drawing style.  It can be a sub-image with a larger texture, and can support
/// simple, 3-slicing, 8-slicing, or 9-slicing of the source texels.
/// UIImages will be show-able & enable-able like all UIElements, but also
/// should inherit highlighted-ness and focused-ness from its parent.
class UIImage : public UIElement
{
    UI_ELEMENT_CLASS_INIT(UIImage, Image)

public:
    virtual ~UIImage() = default;

    void UseTextureData(const uint8_t* wicData, size_t wicDataSize);

protected:
    // no image specific data-driven properties

    ATG::UITK::TextureHandle m_textureDataHandle;

protected:
    UIImage(UIManager& uiManager, ID id) :
        UIElement(uiManager, id),
        m_textureDataHandle(0xFFFFFFFF) {}

    void Render() override;
    void HandleStyleIdChanged() override;

    UISpriteStylePtr m_spriteStyle;
};

/// A factory for creating UIImages from data to be managed by
/// the provided UIManager.
class UIImageFactory : public UIElementFactory<UIImage>
{
    /*virtual*/ UIImage* Create(UIManager& manager, ID id, UIDataPtr data);
};

NAMESPACE_ATG_UITK_END
