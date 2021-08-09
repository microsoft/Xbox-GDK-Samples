//--------------------------------------------------------------------------------------
// File: UIStyles.h
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

#include "UIStyleManager.h"
#include "UIStyle.h"

NAMESPACE_ATG_UITK_BEGIN

//! @private
/// Template which has default UI style creation boilerplate logic
/// that many simple UI style factories will use.
template <typename T>
class UIStyleFactory : public UIStyleFactoryBase
{
public:
    ~UIStyleFactory() = default;
protected:
    T* Create(
        UIStyleManager& styleManager,
        const ID& id,
        const ID& inheritsFromId,
        UIDataPtr data) override
    {
        auto newStyle = new T(styleManager, id, inheritsFromId);
        UIStyleFactoryBase::DeserializeDataProperties(
            data,
            newStyle->m_styleProperties);
        return newStyle;
    }
};

//! @private
/// A style class for those elements that do not need their own style
class UINullStyleFactory;
class UINullStyle : public UIStyle
{
public:
    static ID ClassId() { return ID("NullStyle"); }

    ID GetClassID() const override { return UINullStyle::ClassId(); }

protected:
    UINullStyle() = delete;
    UINullStyle(UIStyleManager& styleManager, ID id, ID inheritsFromId) :
        UIStyle(styleManager, id, inheritsFromId)
    {
    }

    friend class UIStyleManager;
    friend class UIStyleFactory<UINullStyle>;
    friend class UINullStyleFactory;
};

//! @private
/// A style class for those elements that do not need their own style
class UIBasicStyleFactory;
class UIBasicStyle : public UIStyle
{
public:
    static ID ClassId() { return ID("BasicStyle"); }

    ID GetClassID() const override { return UIBasicStyle::ClassId(); }

protected:
    UIBasicStyle() = delete;
    UIBasicStyle(UIStyleManager& styleManager, ID id, ID inheritsFromId) :
        UIStyle(styleManager, id, inheritsFromId)
    {
    }

    friend class UIStyleManager;
    friend class UIStyleFactory<UIBasicStyle>;
    friend class UIBasicStyleFactory;
};

//! @private
/// A style class that extends from the basic style (UIStyle) and adds
/// style properties relevant to displaying text.
class UITextStyleFactory;
class UITextStyle : public UIStyle
{
public:
    static ID ClassId() { return ID("TextStyle"); }

    ID GetClassID() const override { return UITextStyle::ClassId(); }
    void Flatten() override;

    long GetScaledLineHeight();
    long GetUnscaledLineHeight();

    long GetScaledWordWidth(const std::string& word);
    long GetUnscaledWordWidth(const std::string& word);

    static size_t SplitIntoLines(const std::string& content, std::vector<std::string>& splitLines);

    void RenderText(
        const Rectangle& screenRectInPixels,
        const UIDisplayString& content,
        HorizontalTextWrapping horzTextWrapping,
        VerticalTextTruncation vertTextTruncation);

    void RenderLegendSpriteText(
        const Rectangle& screenRectInPixels,
        const UIDisplayString& content,
        HorizontalTextWrapping horzTextWrapping,
        VerticalTextTruncation vertTextTruncation);

    FontType GetFontType() { return m_textStyleProperties.m_fontType; }

protected:
    /// overrideable properties
    StyleTextProperties     m_textStyleProperties;
    std::string             m_fontName;
    std::string             m_legendFontName;
    FontHandle              m_fontHandle;
    FontHandle              m_legendFontHandle;

protected:
    UITextStyle() = delete;
    UITextStyle(UIStyleManager& styleManager, ID id, ID inheritsFromId) :
        UIStyle(styleManager, id, inheritsFromId),
        m_textStyleProperties(),
        m_fontName(),
        m_fontHandle {},
        m_legendFontHandle {}
    {
    }

    std::string GetFontFilename();
    std::string GetLegendFontFilename();

    Vector2 GetFontTextCoordinates(
        FontHandle& fontHandle,
        const UIDisplayString& displayString,
        const Rectangle& containingRectInPixels);
    Vector2 GetLegendTextCoordinates(
        FontHandle& fontHandle,
        FontHandle& legendFontHandle,
        const UIDisplayString& displayString,
        const Rectangle& containingRectInPixels);

    friend class UIStyleManager;
    friend class UIStyleFactory<UITextStyle>;
    friend class UITextStyleFactory;
};

//! @private
/// A style class that extends from the basic style (UIStyle) and adds
/// style properties relevant to displaying a single sprite from within a texture
/// either simply or as a nine-slice.
class UISpriteStyleFactory;
class UISpriteStyle : public UIStyle
{
public:
    static ID ClassId() { return ID("SpriteStyle"); }

    ID GetClassID() const override { return UISpriteStyle::ClassId(); }
    void Flatten() override;

    void RenderSprite(const Rectangle& screenRectInPixels, TextureHandle overrideTexture = 0xFFFFFFFF);
    void RenderCroppedSimpleSprite(const Rectangle& screenRectInPixels, Vector2 percentages);

protected:
    /// overrideable properties
    StyleSpriteProperties          m_spriteStyleProperties;
    uint32_t                       m_textureHandle;

protected:
    UISpriteStyle() = delete;
    UISpriteStyle(UIStyleManager& styleManager, ID id, ID inheritsFromId) :
        UIStyle(styleManager, id, inheritsFromId),
        m_textureHandle {}
    {
    }

    friend class UIStyleManager;
    friend class UIStyleFactory<UISpriteStyle>;
    friend class UISpriteStyleFactory;
};

//! @private
/// A factory for creating UINullStyles from data to be managed by
/// the provided UIStyleManager.
class UINullStyleFactory : public UIStyleFactory<UINullStyle> {};

//! @private
class UIBasicStyleFactory : public UIStyleFactory<UIBasicStyle>
{
protected:
    UIBasicStyle* Create(UIStyleManager& styleManager, const ID& id, const ID& inheritsFromId, UIDataPtr data) override
    {
        auto newBasicStyle = UIStyleFactory<UIBasicStyle>::Create(styleManager, id, inheritsFromId, data);
        return newBasicStyle;
    }
};

//! @private
/// A factory for creating UITextStyles from data to be managed by
/// the provided UIStyleManager.
class UITextStyleFactory : public UIStyleFactory<UITextStyle>
{
protected:
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ StyleTextProperties&);

protected:
    UITextStyle* Create(UIStyleManager& styleManager, const ID& id, const ID& inheritsFromId, UIDataPtr data) override
    {
        auto newTextStyle = UIStyleFactory<UITextStyle>::Create(styleManager, id, inheritsFromId, data);
        UITextStyleFactory::DeserializeDataProperties(
            data,
            newTextStyle->m_textStyleProperties);
        newTextStyle->m_fontName = newTextStyle->GetFontFilename();
        newTextStyle->m_fontHandle = styleManager.GetStyleRenderer().CacheFont(newTextStyle->m_textStyleProperties.m_fontType, newTextStyle->m_fontName, newTextStyle->m_textStyleProperties.m_typeSize);
        newTextStyle->m_legendFontName = newTextStyle->GetLegendFontFilename();
        newTextStyle->m_legendFontHandle = styleManager.GetStyleRenderer().CacheFont(FontType::Sprite, newTextStyle->m_legendFontName, newTextStyle->m_textStyleProperties.m_typeSize);        
        return newTextStyle;
    }
};

//! @private
/// A factory for creating UISpriteStyles from data to be managed by
/// the provided UIStyleManager.
class UISpriteStyleFactory : public UIStyleFactory<UISpriteStyle>
{
protected:
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ StyleSpriteProperties&);

protected:
    UISpriteStyle* Create(UIStyleManager& styleManager, const ID& id, const ID& inheritsFromId, UIDataPtr data) override
    {
        auto newSpriteStyle = UIStyleFactory<UISpriteStyle>::Create(styleManager, id, inheritsFromId, data);
        UISpriteStyleFactory::DeserializeDataProperties(
            data,
            newSpriteStyle->m_spriteStyleProperties);
        newSpriteStyle->m_textureHandle = styleManager.GetStyleRenderer().CacheTexture(
            newSpriteStyle->m_spriteStyleProperties.m_textureFilename);
        return newSpriteStyle;
    }
};

using UINullStylePtr = std::shared_ptr<UINullStyle>;
using UIBasicStylePtr = std::shared_ptr<UIBasicStyle>;
using UITextStylePtr = std::shared_ptr<UITextStyle>;
using UISpriteStylePtr = std::shared_ptr<UISpriteStyle>;

NAMESPACE_ATG_UITK_END
