//--------------------------------------------------------------------------------------
// File: UIStyleManager.h
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

#include "UICore.h"
#include "SimpleMath.h"

NAMESPACE_ATG_UITK_BEGIN

enum class ColorUsage : int
{
    UseExisting = 0,
    SetExisting = 1,
    Override = 2,
};

enum class HorizontalTextWrapping : int
{
    Overflow = 0,
    WrapAtSpace = 1,
};

enum class VerticalTextTruncation : int
{
    Overflow = 0,
    Truncate = 1,
};

enum class TypeWeight : int
{
    Normal = 0,
    Italic = 1,
    Bold = 2,
};

enum class VerticalAlignment : int
{
    Top = 0,
    Middle = 1,
    Bottom = 2,
};

enum class HorizontalAlignment : int
{
    Left = 0,
    Center = 1,
    Right = 2,
};

enum class StylePropertyFlags : uint32_t
{
    TintColor = 1 << 1,
    TintColorUsage = 1 << 2,
    Margin = 1 << 3, // added exterior size
    Padding = 1 << 4, // added interior offset
};

enum class FontType : int
{
    Sprite = 0,
    FreeType = 1,    
};

//! @anchor UIStyle
//! @class StyleProperties UIStyle.h "UITK/UIStyle.h"
//! @brief Basic UI style instance properties shared by all UI style property classes.
struct StyleProperties
{
    StyleProperties() :
        m_colorRGBA(c_defaultColorRGBA),
        m_colorUsage(c_defaultColorUsage),
        m_margin{ c_defaultMargin[0], c_defaultMargin[1], c_defaultMargin[2], c_defaultMargin[3] },
        m_padding{ c_defaultPadding[0], c_defaultPadding[1], c_defaultPadding[2], c_defaultPadding[3] },
        m_overriddenProperties(0)
    {
    }

    //! @private
    bool OverridesProperty(StylePropertyFlags flag) const
    {
        return (m_overriddenProperties & uint32_t(flag)) != 0;
    }

    //! @private
    void ApplyNonOverriddenProperties(const StyleProperties& other);

    //! @subpage color "color JSON property"
    Color                       m_colorRGBA;
    //! @subpage colorUsage "colorUsage JSON property"
    ColorUsage                  m_colorUsage;
    //! @subpage margin "margin JSON property"
    Offsets                     m_margin;    // ordered as: left, top, right, bottom (in pixels)
    //! @subpage padding "padding JSON property"
    Offsets                     m_padding;   // ordered as: left, top, right, bottom (in pixels)

    //! @privatesection
    /// a combination of StylePropertyBits
    uint32_t                    m_overriddenProperties;

    //! @privatesection
    /// defaults for properties if not specified and not inherited
    static constexpr Color      c_defaultColorRGBA = Color(1.0f, 1.0f, 1.0f, 1.0f);
    static constexpr ColorUsage c_defaultColorUsage = ColorUsage::UseExisting;
    static constexpr int        c_defaultMargin[4] = { 0, 0, 0, 0 };
    static constexpr int        c_defaultPadding[4] = { 0, 0, 0, 0 };
    static constexpr uint32_t   c_allBits = 0xFFFF;
};

//! @private
/// A basic style class that purely contains an RGBA color value for tinting
/// and making transparent visual UI elements.  All styles can inherit from
/// other styles and override certain property values.
class UIStyle
{
public:

    // macro for handling style property override assignment easier
    // example: OVERRIDE(PropertyBit, PropertyMember)

#define STYLE_PROP_OVERRIDE(bit, prop) \
            if (!OverridesProperty(bit)) { prop = other.prop; }

#define STYLE_PROP_OVERRIDE_ARRAY(bit, prop) \
            if (!OverridesProperty(bit)) { memcpy(&prop, &other.prop, sizeof(prop)); }

    static bool s_doDebugDraw;

public:
    virtual ~UIStyle() = default;

public:
    ID GetID() const { return m_id; }
    ID InheritsFromID() const { return m_inheritsFromId; }

    virtual ID GetClassID() const = 0;

    // important! derived classes should call this method after
    // doing their own flatten logic since the inheritFromId member
    // gets reset
    virtual void Flatten();

    virtual void BeforeRender();
    virtual void AfterRender();
    virtual void PostRender();

    const Offsets& Margin() const { return m_styleProperties.m_margin; }
    const Offsets& Padding() const { return m_styleProperties.m_padding; }
    const Color& ColorRGBA() const { return m_styleProperties.m_colorRGBA; }

    Rectangle AddMarginToRect(const Rectangle& rectangle) const;
    Rectangle SubtractMarginFromRect(const Rectangle& rectangle) const;
    Rectangle SubtractPaddingFromRect(const Rectangle& rectangle) const;

protected:
    class UIStyleManager& m_styleManager;
    ID m_id;
    ID m_inheritsFromId;

    size_t m_colorStackIndex;

    /// overrideable properties
    StyleProperties m_styleProperties;

    static constexpr size_t c_defaultStackIndex = 0x7FFF0000;

protected:
    UIStyle() = delete;
    UIStyle(class UIStyleManager& styleManager, ID id, ID inheritsFromId) :
        m_styleManager(styleManager),
        m_id(id),
        m_inheritsFromId(inheritsFromId),
        m_colorStackIndex(c_defaultStackIndex)
    {
    }

    void DrawDebugRectangle(const Rectangle& screenRectInPixels);

    friend class UIStyleManager;
    friend class UIStyleFactoryBase;
};

enum class SpriteType : int
{
    Simple = 0,
    EightSliced = 1,   // middle quad is not rendered
    NineSliced = 2,    // middle quad is rendered
};

enum StyleSpritePropertyFlags : uint32_t
{
    Texture = 1 << 1,
    TexelsPerRefUnit = 1 << 2,
    Type = 1 << 3,
    OuterUVExtents = 1 << 4,
    InnerUVExtents = 1 << 5,
};

//! @anchor SpriteStyle
//! @class StyleSpriteProperties UIStyle.h "UITK/UIStyle.h"
//! @brief UI sprite style instance properties specific to sprite styles used by elements
//! that render with sprites.
struct StyleSpriteProperties : StyleProperties
{
    StyleSpriteProperties() :
        StyleProperties(),
        m_textureFilename(c_defaultTextureFileName),
        m_texelsPerRefUnit(c_defaultTexelsPerRefUnit),
        m_spriteType(c_defaultSpriteType),
        m_outerUVExtents{ c_defaultOuterUVExtents[0], c_defaultOuterUVExtents[1] },
        m_innerUVExtents{ c_defaultInnerUVExtents[0], c_defaultInnerUVExtents[1] },
        m_overriddenProperties(0)
    {
    }

    //! @private
    void ApplyNonOverriddenProperties(const StyleSpriteProperties& other);

    //! @private
    bool OverridesProperty(StyleSpritePropertyFlags flags) const
    {
        return (m_overriddenProperties & uint32_t(flags)) != 0;
    }

    //! @subpage texture "texture JSON property"
    std::string             m_textureFilename;
    //! @private
    float                   m_texelsPerRefUnit;
    //! @subpage spriteType "spriteType JSON property"
    SpriteType              m_spriteType;
    //! @subpage outerUVExtents "outerUVExtents JSON property"
    Vector2                 m_outerUVExtents[2];    // ordered as minimum(u,v), maximum(u,v)
    //! @subpage innerUVExtents "innerUVExtents JSON property"
    Vector2                 m_innerUVExtents[2];    // ordered as minimum(u,v), maximum(u,v)

    //! @privatesection
    /// a combination of StylePropertyBits
    uint32_t                m_overriddenProperties;

    //! @privatesection
    /// defaults for properties if not specified and not inherited

    static constexpr const char*        c_defaultTextureFileName = u8"\0";
    static constexpr float              c_defaultTexelsPerRefUnit = 1.0f;
    static constexpr SpriteType         c_defaultSpriteType = SpriteType::Simple;
    static constexpr Vector2            c_defaultOuterUVExtents[2] =
    {
        Vector2(0.0f, 0.0f),
        Vector2(1.0f, 1.0f)
    };
    static constexpr Vector2            c_defaultInnerUVExtents[2] =
    {
        Vector2(0.0f, 0.0f),
        Vector2(1.0f, 1.0f)
    };
    static constexpr uint32_t           c_allBits = 0xFFFF;
};

enum class StyleTextPropertyFlags : uint32_t
{
    TypeFace = 1 << 1,
    FontType = 1 << 2,
    TypeSize = 1 << 3,
    Weight = 1 << 4,
    VertAlignment = 1 << 5,
    HorzAlignment = 1 << 6,
    LegendTypeFace = 1 << 7
};

//! @anchor TextStyle
//! @class StyleTextProperties UIStyle.h "UITK/UIStyle.h"
//! @brief UI text style instance properties specific to text styles used by
//! elements for rendering text.
struct StyleTextProperties : StyleProperties
{
    StyleTextProperties() :
        StyleProperties(),
        m_typefaceName(c_defaultTypeFaceName),
        m_fontType(c_defaultFontType),
        m_typeSize(c_defaultTypeSize),
        m_typeWeight(c_defaultTypeWeight),
        m_vertAlignment(c_defaultVerticalAlignment),
        m_horzAlignment(c_defaultHorizontalAlignment),
        m_legendTypefaceName(c_defaultLegendTypeFaceName),
        m_overriddenProperties(0)
    {
    }

    //! @private
    void ApplyNonOverriddenProperties(const StyleTextProperties& other);

    //! @private
    bool OverridesProperty(StyleTextPropertyFlags flag) const
    {
        return (m_overriddenProperties & uint32_t(flag)) != 0;
    }

    //! @subpage font "font JSON property"
    std::string             m_typefaceName;
    //! @subpage fontType "fontType JSON property"
    FontType                m_fontType;
    //! @subpage fontSize "fontSize JSON property"
    uint32_t                m_typeSize;
    //! @subpage weight "weight JSON property"
    TypeWeight              m_typeWeight;
    //! @subpage verticalAlign "verticalAlign JSON property"
    VerticalAlignment       m_vertAlignment;
    //! @subpage horizontalAlign "horizontalAlign JSON property"
    HorizontalAlignment     m_horzAlignment;
    //! @subpage legendFont "legendFont JSON property"
    std::string             m_legendTypefaceName;

    //! @privatesection
    /// a combination of StylePropertyBits
    uint32_t                m_overriddenProperties;

    //! @privatesection
    /// defaults for properties if not specified and not inherited
    // TODO: perhaps separate pathing from naming so that path can be configured...
    static constexpr const char* c_defaultTypeFaceName = "Assets/Fonts/SegoeUI";
    static constexpr FontType               c_defaultFontType = FontType::Sprite;
    static constexpr uint32_t               c_defaultTypeSize = 18;
    static constexpr TypeWeight             c_defaultTypeWeight = TypeWeight::Normal;
    static constexpr VerticalAlignment      c_defaultVerticalAlignment = VerticalAlignment::Top;
    static constexpr HorizontalAlignment    c_defaultHorizontalAlignment = HorizontalAlignment::Left;
    static constexpr const char* c_defaultLegendTypeFaceName = "Assets/Fonts/XboxOneControllerLegendSmall";
    static constexpr uint32_t               c_allBits = 0xFFFF;
};

//! @private
/// This is the base factory class that is used by the UIStyleManager to
/// create intances of UI styles from data.  All UI style classes
/// need to have a corresponding factory class for creating their instances.
class UIStyleFactoryBase
{
public:
    virtual ~UIStyleFactoryBase() = default;
protected:
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ StyleProperties&);

protected:
    virtual UIStyle* Create(UIStyleManager&, const ID&, const ID&, UIDataPtr) = 0;

    friend class UIStyleManager;
};

using UIStylePtr = std::shared_ptr<UIStyle>;
using UIStyleFactoryPtr = std::unique_ptr<class UIStyleFactoryBase>;

class UISpriteStyle;
class UITextStyle;
class UINullStyle;

using UISpriteStylePtr = std::shared_ptr<UISpriteStyle>;
using UITextStylePtr = std::shared_ptr<UITextStyle>;
using UINullStylePtr = std::shared_ptr<UINullStyle>;

NAMESPACE_ATG_UITK_END
