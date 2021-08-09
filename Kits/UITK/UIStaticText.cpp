//--------------------------------------------------------------------------------------
// File: UIStaticText.cpp
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
#include "UIStaticText.h"
#include "UIStyleImpl.h"

NAMESPACE_ATG_UITK_BEGIN

INITIALIZE_CLASS_LOG_DEBUG(UIStaticText);

ENUM_LOOKUP_TABLE(HorizontalTextWrapping,
    ID_ENUM_PAIR(UITK_VALUE(overflow), HorizontalTextWrapping::Overflow),
    ID_ENUM_PAIR(UITK_VALUE(wrapAtSpace), HorizontalTextWrapping::WrapAtSpace)
)

ENUM_LOOKUP_TABLE(VerticalTextTruncation,
    ID_ENUM_PAIR(UITK_VALUE(overflow), VerticalTextTruncation::Overflow),
    ID_ENUM_PAIR(UITK_VALUE(truncate), VerticalTextTruncation::Truncate)
)

/*static*/ void UIStaticText::WordWrapText(
    UITextStylePtr textStyle,
    Vector2 availableSize,
    HorizontalTextWrapping wrapping,
    std::string& textToModify)
{
    auto sizeInRefUnits = availableSize;
    auto rectWidth = long(sizeInRefUnits.x);
    auto lineHeight = textStyle->GetUnscaledLineHeight();

    auto lineStartOffset = long(0);
    auto lineStartingCharIndex = size_t(0);

    // iterate through the display text line by line
    while (lineStartingCharIndex < textToModify.size())
    {
        // first, bail out completely if there is no more text to render
        auto nextLineWordStart = textToModify.find_first_not_of(" \n", lineStartingCharIndex);
        if (nextLineWordStart == std::string::npos)
        {
            textToModify.erase(lineStartingCharIndex);
            break;
        }
        
        // next, advance the line if we are not horizontally wrapping and
        // encounter a newline
        auto nextLineNewlineIndex = textToModify.find_first_of('\n', lineStartingCharIndex);
        if (nextLineNewlineIndex != std::string::npos && wrapping == HorizontalTextWrapping::Overflow)
        {
            lineStartOffset += lineHeight;
            lineStartingCharIndex = nextLineNewlineIndex + 1;
            continue;
        }

        // next, terminate if we are not horizontally wrapping and there is
        // additional newline
        if (wrapping == HorizontalTextWrapping::Overflow)
        {
            break;
        }

        // next, advance the line if there is a newline before the next
        // legitimate word
        if (nextLineNewlineIndex < nextLineWordStart)
        {
            lineStartOffset += lineHeight;
            lineStartingCharIndex = nextLineNewlineIndex + 1;
            continue;
        }

        // next, to perform the horizontal wrapping and break the line up if
        // possible when it no longer fits
        if (wrapping == HorizontalTextWrapping::WrapAtSpace)
        {
            auto wordStartOffset = long(0);
            auto wordStartingCharIndex = lineStartingCharIndex;
            auto wordBeginIndex = nextLineWordStart;

            // iterate through the line text word by word
            while (wordStartingCharIndex < textToModify.size())
            {
                auto nextSpaceIndex = textToModify.find_first_of(' ', wordBeginIndex);
                auto nextNewlineIndex = textToModify.find_first_of('\n', wordBeginIndex);
                auto wordEndIndex = std::min(nextSpaceIndex, nextNewlineIndex);
                auto nextWordCharIndex = textToModify.find_first_not_of(" \n", wordEndIndex);

                auto word = textToModify.substr(wordStartingCharIndex, wordEndIndex - wordStartingCharIndex);
                auto wordWidth = textStyle->GetUnscaledWordWidth(word);
                auto wordExceedsHorizontalSize = (wordStartOffset + wordWidth) > rectWidth;

                // cases:
                // A. next word does not fit on current line:
                // if we've added a word...
                if (wordExceedsHorizontalSize && wordStartingCharIndex > lineStartingCharIndex)
                {
                    // insert newline at wordBeginIndex & break
                    textToModify.insert(wordBeginIndex, size_t(1), '\n');
                    lineStartingCharIndex = wordBeginIndex;
                    break;
                }
                // ...else we've not added a word yet
                // B. next word does fit on current line:
                else
                {
                    // if no next word
                    if (nextWordCharIndex == std::string::npos)
                    {
                        // advance line at end of word & break
                        lineStartingCharIndex = wordEndIndex;
                        break;
                    }
                    // else if newline before next word
                    else if (nextNewlineIndex < nextWordCharIndex)
                    {
                        // advance line at newline & break
                        lineStartingCharIndex = nextNewlineIndex;
                        break;
                    }
                    // else
                    else
                    {
                        // keep word on current line & advance word
                        wordStartOffset += wordWidth;
                        wordStartingCharIndex = wordEndIndex;
                        wordBeginIndex = nextWordCharIndex;
                    }
                }
            }
        }
    }
}

/*protected:*/

void UIStaticText::ProcessText()
{
    m_wordWrappedText = m_staticTextDataProperties.m_displayString;

    // this is used to populate a substr operation, default is full string
    m_textRange.first = 0;
    m_textRange.second = m_wordWrappedText.size();

    if (m_staticTextDataProperties.m_horzTextWrapping == HorizontalTextWrapping::Overflow &&
        m_staticTextDataProperties.m_vertTextTruncation == VerticalTextTruncation::Overflow)
    {
        return;
    }

    WordWrapText(
        m_textStyle,
        GetSizeInRefUnits(),
        m_staticTextDataProperties.m_horzTextWrapping,
        m_wordWrappedText);

    // adjust in case of any inserted newlines
    m_textRange.second = m_wordWrappedText.size();

    m_lineStartIndices.clear();
    m_lineStartIndices.push_back(0);

    if (m_staticTextDataProperties.m_vertTextTruncation == VerticalTextTruncation::Truncate)
    {
        // Index all line beginnings
        for (uint32_t i = 0; i < m_wordWrappedText.size(); ++i)
        {
            if (m_wordWrappedText.at(i) == '\n')
            {
                m_lineStartIndices.push_back(i + 1);
            }
        }

        // Set slider size, ranges
        AdjustSlider();

        // Set text start index and substring size
        SetTextRange();
    }
    else
    {
        if (m_cachedSlider)
        {
            m_cachedSlider->SetVisible(false);
        }
    }

}

void UIStaticText::AdjustSlider()
{
    if (m_cachedSlider)
    {
        auto maxLines = GetMaxDisplayLines();
        auto numLines = m_lineStartIndices.size();

        if (long(numLines) > maxLines)
        {
            m_cachedSlider->SetHeight(GetSizeInRefUnits().y);
            m_cachedSlider->ModifySliderRange(0.f, float(numLines - maxLines), uint32_t(numLines - maxLines) + 1);
            m_cachedSlider->SetSliderValue(0.f);
            m_cachedSlider->SetVisible(true);
        }
        else
        {
            m_cachedSlider->SetVisible(false);
        }
    }
}

long UIStaticText::GetMaxDisplayLines()
{
    auto rectHeight = long(GetSizeInRefUnits().y);
    auto lineHeight = m_textStyle->GetUnscaledLineHeight();
    return rectHeight / lineHeight;
}

void UIStaticText::SetTextRange()
{
    auto numLines = m_lineStartIndices.size();

    m_textRange.second = m_wordWrappedText.size();

    if (m_staticTextDataProperties.m_vertTextTruncation == VerticalTextTruncation::Truncate && numLines > 1)
    {
        auto maxLines = GetMaxDisplayLines();

        if (long(numLines) > maxLines)
        {
            // select the subset of lines to display based on slider position
            // if no slider, then just truncate to maxLines from first line
            auto startLine = (m_cachedSlider) ? uint32_t(std::round(m_cachedSlider->GetCurrentValue())) : 0;
            auto startIndex = m_lineStartIndices.at(startLine);
            auto endIndex = (startLine + maxLines) < m_lineStartIndices.size() ?
                m_lineStartIndices.at(startLine + maxLines) :
                m_wordWrappedText.size();

            auto count = endIndex - startIndex;

            m_textRange.first = startIndex;
            m_textRange.second = count;
        }
    }
}

/*virtual*/ void UIStaticText::HandleStyleIdChanged()
{
    UIElement::HandleStyleIdChanged();
    m_textStyle = m_uiManager.GetStyleManager().GetTypedById<UITextStyle>(m_elementDataProperties.styleId);
}

void UIStaticText::WireUpElements()
{
    if (!m_cachedSlider)
    {
        m_cachedSlider = GetTypedSubElementById<UISlider>(m_staticTextDataProperties.m_sliderSubElementId);

        if (m_cachedSlider)
        {
            m_cachedSlider->CurrentValueState().AddListener([this](UISlider*)
                {
                    SetTextRange();
                });
        }
    }
}

/*virtual*/ void UIStaticText::Render()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStaticText_Render - %hs", GetID().AsCStr());
    auto screenRect = GetScreenRectInPixels();

    if (m_wordWrappedText.empty() && !m_staticTextDataProperties.m_displayString.empty())
    {
        ProcessText();
    }

    if (m_staticTextDataProperties.m_isLegend)
    {
        m_textStyle->RenderLegendSpriteText(
            screenRect,
            m_wordWrappedText.substr(m_textRange.first, m_textRange.second),
            // note: we handle wrapping and truncation operations at the static text instance
            // level for performance reasons
            HorizontalTextWrapping::Overflow,
            VerticalTextTruncation::Overflow);
    }
    else
    {
        m_textStyle->RenderText(
            screenRect,
            m_wordWrappedText.substr(m_textRange.first, m_textRange.second),
            // note: we handle wrapping and truncation operations at the static text instance
            // level for performance reasons
            HorizontalTextWrapping::Overflow,
            VerticalTextTruncation::Overflow);
    }
}

/*protected:*/

/*static*/ void UIStaticTextFactory::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _Out_ StaticTextDataProperties& staticTextDataProperties)
{
    ID tempID;

    staticTextDataProperties.m_displayString =
        data->GetIfExists<std::string>(UITK_FIELD(text), StaticTextDataProperties::c_defaultDisplayString);

    if (data->GetTo(UITK_FIELD(horzWrap), tempID))
    {
        UIEnumLookup(tempID, staticTextDataProperties.m_horzTextWrapping);
    }

    if (data->GetTo(UITK_FIELD(vertTrunc), tempID))
    {
        UIEnumLookup(tempID, staticTextDataProperties.m_vertTextTruncation);
    }

    data->GetTo(UITK_FIELD(isLegend), staticTextDataProperties.m_isLegend);

    // optional slider that shows for vertical scrolling
    staticTextDataProperties.m_sliderSubElementId = data->GetIfExists<ID>(
        UITK_FIELD(sliderSubElementId), ID::Default);
}

UIStaticText* UIStaticTextFactory::Create(UIManager& manager, ID id, UIDataPtr data)
{
    auto newStaticText = UIElementFactory<UIStaticText>::Create(manager, id, data);
    UIStaticTextFactory::DeserializeDataProperties(
        data,
        newStaticText->m_staticTextDataProperties);
    newStaticText->m_textStyle = manager.GetStyleManager().GetTypedById<UITextStyle>(newStaticText->m_elementDataProperties.styleId);
    return newStaticText;
}

NAMESPACE_ATG_UITK_END
