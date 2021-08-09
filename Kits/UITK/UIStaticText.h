//--------------------------------------------------------------------------------------
// File: UIStaticText.h
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

#include "UISliderElement.h"

NAMESPACE_ATG_UITK_BEGIN

//! @anchor StaticText
//! @class StaticTextDataProperties UIStaticText.h "UITK/UIStaticText.h"
//! @brief Static text label UI element instance properties specific to static text.
struct StaticTextDataProperties
{
    StaticTextDataProperties() :
        m_displayString(c_defaultDisplayString),
        m_horzTextWrapping(c_defaultHorzTextWrapping),
        m_vertTextTruncation(c_defaultVertTextTruncation),
        m_isLegend(c_defaultIsLegend)
    {
    }

    //! @subpage text "String to display"
    UIDisplayString m_displayString;
    //! @subpage horzWrap "Horizontal wrapping option" 
    HorizontalTextWrapping m_horzTextWrapping;
    //! @subpage vertTrunc "Vertical truncation option"
    VerticalTextTruncation m_vertTextTruncation;
    //! @subpage isLegend "Flag for using legend font"
    bool m_isLegend;
    //! @subpage m_sliderSubElementId "Optional scrollbar"
    ID m_sliderSubElementId;

    //! @privatesection
    /// defaults for properties if not specified in data
    static constexpr UIConstDisplayString   c_defaultDisplayString = u8"Lorem Ipsum";
    static constexpr HorizontalTextWrapping c_defaultHorzTextWrapping =
        HorizontalTextWrapping::Overflow;
    static constexpr VerticalTextTruncation c_defaultVertTextTruncation =
        VerticalTextTruncation::Overflow;
    static constexpr bool c_defaultIsLegend = false;
};

//! @private
/// A UIStaticText element is intended to be a simple element for
/// displaying localized text in a particular Text style.  They can
/// either truncate, overflow, or wrap horizontally, as well as truncate
/// or overflow vertically.  UIStaticTexts will be show-able & enable-able
/// like all UIElements, but also should inherit highlighted-ness and
/// focused-ness from its parent.
class UIStaticText : public UIElement
{
    DECLARE_CLASS_LOG();

    UI_ELEMENT_CLASS_INIT(UIStaticText, StaticText)

public:
    void SetDisplayText(const UIDisplayString& text)
    {
        m_staticTextDataProperties.m_displayString = text;
        m_wordWrappedText.clear();
                
        if (m_cachedSlider)
        {
            m_cachedSlider->SetVisible(text.size() > 0);
        }

    }
    inline const UIDisplayString& GetDisplayText() const
    {
        return m_staticTextDataProperties.m_displayString;
    }

protected:
    StaticTextDataProperties    m_staticTextDataProperties;
    UITextStylePtr              m_textStyle;
    UIDisplayString             m_wordWrappedText;

    std::shared_ptr<UISlider>   m_cachedSlider;
    std::vector<uint32_t>       m_lineStartIndices;
    std::pair<size_t, size_t>   m_textRange;

protected:
    // static text specific data-driven properties
    UIStaticText(UIManager& uiManager, ID id) : UIElement(uiManager, id)
    {
        m_cachedSlider = nullptr;
    }

    virtual ~UIStaticText() = default;

    void Render() override;
    void HandleStyleIdChanged() override;

    void PostLoad() override
    {
        WireUpElements();
    }

    void ProcessText();

    void WireUpElements();
    void AdjustSlider();
    long GetMaxDisplayLines();
    void SetTextRange();

    static void WordWrapText(
        UITextStylePtr textStyle,
        Vector2 availableSize,
        HorizontalTextWrapping wrapping,
        std::string& textToModify);
};

//! @private
/// A factory for creating UIStaticTexts from data to be managed by
/// the provided UIManager.
class UIStaticTextFactory : public UIElementFactory<UIStaticText>
{
    static void DeserializeDataProperties(
        _In_ UIDataPtr data,
        _Out_ StaticTextDataProperties& staticTextDataProperties);

protected:
    /*virtual*/ UIStaticText* Create(UIManager& manager, ID id, UIDataPtr data);
};

NAMESPACE_ATG_UITK_END
