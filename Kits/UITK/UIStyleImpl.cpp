//--------------------------------------------------------------------------------------
// File: UIStyles.cpp
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

#include "UIStyleImpl.h"
#include "UIStyleManager.h"
#include "UIStyleRenderer.h"
#include "UIMath.h"

NAMESPACE_ATG_UITK_BEGIN

ENUM_LOOKUP_TABLE(ColorUsage,
    ID_ENUM_PAIR(UITK_VALUE(useExisting), ColorUsage::UseExisting),
    ID_ENUM_PAIR(UITK_VALUE(setExisting), ColorUsage::SetExisting),
    ID_ENUM_PAIR(UITK_VALUE(override), ColorUsage::Override)
)

enum class ControllerFontCharId : int
{
    LeftThumb = 0,
    DPad = 1,
    RightThumb = 2,
    View = 3,
    Nexus = 4,
    Menu = 5,
    XButton = 6,
    AButton = 7,
    YButton = 8,
    BButton = 9,
    RightShoulder = 10,
    RightTrigger = 11,
    LeftTrigger = 12,
    LeftShoulder = 13,
    Total
};

// Unicode numbers from UX:
//
// A button         1058 / F093
// B button         1057 / F094
// X button         1055 / F096
// Y button         1056 / F095
// LB bumper        1107 / F10C
// RB bumper        1108 / F10D
// LT trigger       1105 / F10A
// RT trigger       1106 / F10B
// Left Stick       1103 / F108
// Right Stick      1104 / F109
// Dpad             1109 / F10E
// View button      1026 / EECA
// Menu button      1008 / EDE3
// Nexus(Xbox)      1207 / EB70

struct LegendCharTraits {
    static constexpr char FontChars[int(ControllerFontCharId::Total)] =
    {
        /*LeftThumb = 0*/       ' ',
        /*DPad = 1*/            '!',
        /*RightThumb = 2*/      '\"',
        /*View = 3*/            '#',
        /*Nexus = 4*/           '$',
        /*Menu = 5*/            '%',
        /*XButton = 6*/         '&',
        /*AButton = 7*/         '\'',
        /*YButton = 8*/         '(',
        /*BButton = 9*/         ')',
        /*RightShoulder = 10*/  '*',
        /*RightTrigger = 11*/   '+',
        /*LeftTrigger = 12*/    ',',
        /*LeftShoulder = 13*/   '-',
    };
    static constexpr const char* UnicodeFontChars[int(ControllerFontCharId::Total)] =
    {
        /*LeftThumb = 0*/       u8"\uF108",
        /*DPad = 1*/            u8"\uF10E",
        /*RightThumb = 2*/      u8"\uF109",
        /*View = 3*/            u8"\uEECA",
        /*Nexus = 4*/           u8"\uEB70",
        /*Menu = 5*/            u8"\uEDE3",
        /*XButton = 6*/         u8"\uF096",
        /*AButton = 7*/         u8"\uF093",
        /*YButton = 8*/         u8"\uF095",
        /*BButton = 9*/         u8"\uF094",
        /*RightShoulder = 10*/  u8"\uF10D",
        /*RightTrigger = 11*/   u8"\uF10B",
        /*LeftTrigger = 12*/    u8"\uF10A",
        /*LeftShoulder = 13*/   u8"\uF10C"
    };
    static constexpr const char* FontCharCodes[int(ControllerFontCharId::Total)] =
    {
        /*LeftThumb = 0*/       "[LThumb]",
        /*DPad = 1*/            "[DPad]",
        /*RightThumb = 2*/      "[RThumb]",
        /*View = 3*/            "[View]",
        /*Nexus = 4*/           "[Nexus]",
        /*Menu = 5*/            "[Menu]",
        /*XButton = 6*/         "[X]",
        /*AButton = 7*/         "[A]",
        /*YButton = 8*/         "[Y]",
        /*BButton = 9*/         "[B]",
        /*RightShoulder = 10*/  "[RB]",
        /*RightTrigger = 11*/   "[RT]",
        /*LeftTrigger = 12*/    "[LT]",
        /*LeftShoulder = 13*/   "[LB]",
    };
};

DirectX::SimpleMath::Vector2 MeasureLegendString(
    UIStyleRenderer& styleRenderer,
    FontHandle textFontHandle,
    FontHandle legendFontHandle,
    _In_z_ char const* text)
{
    using namespace DirectX;
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UITextStyle_MeasureLegendString");

    size_t textLen = strlen(text);

    if (textLen >= 4096)
    {
        throw std::out_of_range("String is too long");
    }

    float textFontHeight = float(styleRenderer.GetScaledFontHeight(textFontHandle));
    float legendFontHeight = float(styleRenderer.GetScaledFontHeight(legendFontHandle));

    size_t j = 0;
    char strBuffer[4096] = {};
    bool buttonText = false;

    XMFLOAT2 outPos = Vector2::Zero;
    Vector2 measuredSize = Vector2::Zero;

    for (size_t ch = 0; ch < textLen; ++ch)
    {
        if (buttonText)
        {
            strBuffer[j++] = text[ch];

            if (text[ch] == ']')
            {
                char button[2] = {};

                for (auto id = 0; id < int(ControllerFontCharId::Total); ++id)
                {
                    if (0 == _stricmp(strBuffer, LegendCharTraits::FontCharCodes[id]))
                    {
                        *button = LegendCharTraits::FontChars[id];
                        break;
                    }
                }

                if (*button)
                {
                    auto buttonSize = float(styleRenderer.GetScaledFontTextSize(legendFontHandle, button).width);
                    outPos.x += buttonSize;
                    measuredSize.x = std::max(outPos.x, measuredSize.x);
                }

                buttonText = false;
                memset(strBuffer, 0, sizeof(strBuffer));
                j = 0;
            }
        }
        else
        {
            switch (text[ch])
            {
            case '\r':
                break;

            case '[':
                if (*strBuffer)
                {
                    outPos.x += float(styleRenderer.GetScaledFontTextSize(textFontHandle, strBuffer).width);
                    measuredSize.x = std::max(outPos.x, measuredSize.x);
                    memset(strBuffer, 0, sizeof(strBuffer));
                    j = 0;
                }

                buttonText = true;
                *strBuffer = '[';
                ++j;
                break;

            case '\n':
                if (*strBuffer)
                {
                    memset(strBuffer, 0, sizeof(strBuffer));
                    j = 0;
                }

                outPos.x = 0.0f;
                outPos.y += textFontHeight > legendFontHeight ? textFontHeight : legendFontHeight;
                break;

            default:
                strBuffer[j++] = text[ch];
                break;
            }
        }
    }

    outPos.x += float(styleRenderer.GetScaledFontTextSize(textFontHandle, strBuffer).width);
    measuredSize.x = std::max(outPos.x, measuredSize.x);

    outPos.y += textFontHeight > legendFontHeight ? textFontHeight : legendFontHeight;
    measuredSize.y = outPos.y;

    return measuredSize;
}

void DrawLegendSpriteString(
    UIStyleRenderer& styleRenderer,
    FontHandle textFontHandle,
    FontHandle legendFontHandle,
    _In_z_ char const* text,
    const DirectX::SimpleMath::Vector2& position)
{
    using namespace DirectX;

    size_t textLen = strlen(text);

    if (textLen >= 4096)
    {
        throw std::out_of_range("String is too long");
    }

    float textFontHeight = float(styleRenderer.GetScaledFontHeight(textFontHandle));
    float legendFontHeight = float(styleRenderer.GetScaledFontHeight(legendFontHandle));

    size_t j = 0;
    char strBuffer[4096] = {};
    bool buttonText = false;

    XMFLOAT2 outPos = position;

    for (size_t ch = 0; ch < textLen; ++ch)
    {
        if (buttonText)
        {
            strBuffer[j++] = text[ch];

            if (text[ch] == ']')
            {
                char button[2] = {};

                for (auto id = 0; id < int(ControllerFontCharId::Total); ++id)
                {
                    if (0 == _stricmp(strBuffer, LegendCharTraits::FontCharCodes[id]))
                    {
                        *button = LegendCharTraits::FontChars[id];
                        break;
                    }
                }

                if (*button)
                {
                    auto buttonSize = float(styleRenderer.GetScaledFontTextSize(legendFontHandle, button).width);
                    styleRenderer.DrawTextString(legendFontHandle, { button, outPos });
                    outPos.x += buttonSize;
                }

                buttonText = false;
                memset(strBuffer, 0, sizeof(strBuffer));
                j = 0;
            }
        }
        else
        {
            switch (text[ch])
            {
            case '\r':
                break;

            case '[':
                if (*strBuffer)
                {
                    styleRenderer.DrawTextString(textFontHandle, { strBuffer, outPos });
                    outPos.x += float(styleRenderer.GetScaledFontTextSize(textFontHandle, strBuffer).width);
                    memset(strBuffer, 0, sizeof(strBuffer));
                    j = 0;
                }

                buttonText = true;
                *strBuffer = '[';
                ++j;
                break;

            case '\n':
                if (*strBuffer)
                {
                    styleRenderer.DrawTextString(textFontHandle, { strBuffer, outPos });
                    memset(strBuffer, 0, sizeof(strBuffer));
                    j = 0;
                }

                outPos.x = position.x;
                outPos.y += textFontHeight > legendFontHeight ? textFontHeight : legendFontHeight;
                break;

            default:
                strBuffer[j++] = text[ch];
                break;
            }
        }
    }

    if (*strBuffer)
    {
        styleRenderer.DrawTextString(textFontHandle, { strBuffer, outPos });
    }
}

void UIStyle::DrawDebugRectangle(const Rectangle& screenRectInPixels)
{
#if UITK_ENABLE_DEBUGDRAW
    if (s_doDebugDraw)
    {
        UIStyleRenderer& styleRenderer = m_styleManager.GetStyleRenderer();
        auto debugTexture = styleRenderer.CacheTexture("Assets/Textures/debug-rect.png");

        auto scissorIndex = styleRenderer.PushScissorRectangle(styleRenderer.GetWindowRectangle());

        // the render rectangle
        auto colorIndex = styleRenderer.PushTintColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
        styleRenderer.DrawTexturedQuads(
            debugTexture,
            { {styleRenderer.GetTextureRect(debugTexture), screenRectInPixels} });
        styleRenderer.PopTintColor(colorIndex);

        // the padded rectangle
        colorIndex = styleRenderer.PushTintColor(Color(1.0f, 0.5f, 0.0f, 1.0f));
        styleRenderer.DrawTexturedQuads(
            debugTexture,
            { {styleRenderer.GetTextureRect(debugTexture), SubtractPaddingFromRect(screenRectInPixels)} });
        styleRenderer.PopTintColor(colorIndex);

        // the margined rectangle
        colorIndex = styleRenderer.PushTintColor(Color(0.0f, 0.5f, 1.0f, 1.0f));
        styleRenderer.DrawTexturedQuads(
            debugTexture,
            { {styleRenderer.GetTextureRect(debugTexture), AddMarginToRect(screenRectInPixels)} });
        styleRenderer.PopTintColor(colorIndex);

        styleRenderer.PopScissorRectangle(scissorIndex);
    }
#else
    UNREFERENCED_PARAMETER(screenRectInPixels);
#endif
}

/*public:*/

void StyleTextProperties::ApplyNonOverriddenProperties(
    const StyleTextProperties& other)
{
    STYLE_PROP_OVERRIDE(StyleTextPropertyFlags::TypeFace, m_typefaceName);
    STYLE_PROP_OVERRIDE(StyleTextPropertyFlags::FontType, m_fontType);
    STYLE_PROP_OVERRIDE(StyleTextPropertyFlags::TypeSize, m_typeSize);
    STYLE_PROP_OVERRIDE(StyleTextPropertyFlags::Weight, m_typeWeight);
    STYLE_PROP_OVERRIDE(StyleTextPropertyFlags::VertAlignment, m_vertAlignment);
    STYLE_PROP_OVERRIDE(StyleTextPropertyFlags::HorzAlignment, m_horzAlignment);
    STYLE_PROP_OVERRIDE(StyleTextPropertyFlags::LegendTypeFace, m_legendTypefaceName);
}

/*public:*/

void UITextStyle::Flatten()
{
    if (InheritsFromID())
    {
        auto ancestor = m_styleManager.GetTypedById<UITextStyle>(InheritsFromID());
        assert(ancestor);

        // flatten the ancestor first so that we only have to traverse
        // through properties one layer deep...

        ancestor->Flatten();

        // what this function is meant to do is to "bake" all of the style
        // data (including what is inherited) for easier access and rendering

        m_textStyleProperties.ApplyNonOverriddenProperties(ancestor->m_textStyleProperties);
        m_textStyleProperties.m_overriddenProperties = StyleTextProperties::c_allBits;

        UIStyle::Flatten();

        // we need to make sure our font cache is up to date since we may have
        // a new font based on inheritance
        m_fontName = GetFontFilename();
        m_fontHandle = m_styleManager.GetStyleRenderer().CacheFont(m_textStyleProperties.m_fontType, m_fontName, m_textStyleProperties.m_typeSize);

        m_legendFontName = GetLegendFontFilename();
        if (m_legendFontName.size() > 0)
        {
            // Always use sprite font for icons
            m_legendFontHandle = m_styleManager.GetStyleRenderer().CacheFont(FontType::Sprite, m_legendFontName, m_textStyleProperties.m_typeSize);
        }
    }
}

long UITextStyle::GetScaledWordWidth(const std::string& word)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UITextStyle_GetScaledWordWidth");
    UIStyleRenderer& styleRenderer = m_styleManager.GetStyleRenderer();
    auto sizeRect = styleRenderer.GetScaledFontTextSize(m_fontHandle, word);
    return sizeRect.width;
}

long UITextStyle::GetUnscaledWordWidth(const std::string& word)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UITextStyle_GetUnscaledWordWidth");
    UIStyleRenderer& styleRenderer = m_styleManager.GetStyleRenderer();
    auto sizeRect = styleRenderer.GetUnscaledFontTextSize(m_fontHandle, word);
    return sizeRect.width;
}

long UITextStyle::GetScaledLineHeight()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UITextStyle_GetScaledLineHeight");
    UIStyleRenderer& styleRenderer = m_styleManager.GetStyleRenderer();
    auto heightTextRenderingSize = styleRenderer.GetScaledFontHeight(m_fontHandle);
    return heightTextRenderingSize;
}

long UITextStyle::GetUnscaledLineHeight()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UITextStyle_GetUnscaledLineHeight");
    UIStyleRenderer& styleRenderer = m_styleManager.GetStyleRenderer();
    auto heightTextRenderingSize = styleRenderer.GetUnscaledFontHeight(m_fontHandle);
    return heightTextRenderingSize;
}

size_t UITextStyle::SplitIntoLines(const std::string& content, std::vector<std::string>& splitLines)
{
    size_t newlinePos = 0;
    size_t newlineOffset = 0;
    size_t lineCount = 1;

    do
    {
        newlinePos = content.find('\n', newlineOffset);

        if (lineCount++ == 1 && newlinePos > content.size())
        {
            splitLines.push_back(content);
            return 1;
        }

        splitLines.emplace_back(content.substr(newlineOffset, newlinePos - newlineOffset));
        newlineOffset = newlinePos + 1;
    } while (newlinePos < content.size());

    return splitLines.size();
}

void UITextStyle::RenderLegendSpriteText(
    const Rectangle& screenRectInPixels,
    const UIDisplayString& content,
    HorizontalTextWrapping,
    VerticalTextTruncation)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UITextStyle_RenderLegendSpriteText - %d - %d - %hs", m_fontHandle, m_legendFontHandle, content.c_str());
    UIStyle::BeforeRender();

    DrawDebugRectangle(screenRectInPixels);

    // NOTE: right now we are doing the most barebones attempt to
    // render a string just to get things started...

    std::vector<std::string> lines;
    auto lineCount = SplitIntoLines(content, lines);

    UIStyleRenderer& styleRenderer = m_styleManager.GetStyleRenderer();

    auto screenRectMinusPadding = SubtractPaddingFromRect(screenRectInPixels);

    // the single line case can use the traditional route...

    if (lineCount == 1)
    {
        auto fontTextCoordinates = GetLegendTextCoordinates(
            m_fontHandle,
            m_legendFontHandle,
            content,
            screenRectMinusPadding);

        DrawLegendSpriteString(
            styleRenderer,
            m_fontHandle,
            m_legendFontHandle,
            content.c_str(),
            fontTextCoordinates);
    }

    // the multiline case requires working line by line...

    else if (lineCount > 1)
    {
        auto lineHeight = styleRenderer.GetScaledFontHeight(m_fontHandle);
        auto startingVerticalOffset = 0.0f;
        switch (m_textStyleProperties.m_vertAlignment)
        {
        case VerticalAlignment::Bottom:
            startingVerticalOffset = -lineHeight * float(lineCount - 1);
            break;
        case VerticalAlignment::Middle:
            startingVerticalOffset = -lineHeight * float(lineCount - 1) * 0.5f;
            break;
        case VerticalAlignment::Top:
        default:
            /* do nothing*/
            break;
        }

        for (auto line = size_t(0); line < lineCount; ++line)
        {
            auto currentVerticalOffset = float(line) * lineHeight;

            auto fontTextCoordinates = GetLegendTextCoordinates(
                m_fontHandle,
                m_legendFontHandle,
                lines[line],
                screenRectMinusPadding);
            fontTextCoordinates.y += startingVerticalOffset + currentVerticalOffset;

            DrawLegendSpriteString(
                styleRenderer,
                m_fontHandle,
                m_legendFontHandle,
                lines[line].c_str(),
                fontTextCoordinates);
        }
    }

    UIStyle::AfterRender();
}

void UITextStyle::RenderText(
    const Rectangle& screenRectInPixels,
    const UIDisplayString& content,
    HorizontalTextWrapping,
    VerticalTextTruncation)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UITextStyle_RenderText");

    UIStyle::BeforeRender();

    DrawDebugRectangle(screenRectInPixels);

    // NOTE: right now we are doing the most barebones attempt to
    // render a string just to get things started...

    UIStyleRenderer& styleRenderer = m_styleManager.GetStyleRenderer();

    auto screenRectMinusPadding = SubtractPaddingFromRect(screenRectInPixels);

    // the single line case can use the traditional route...

    std::vector<std::string> lines;
    auto lineCount = SplitIntoLines(content, lines);

    if (lineCount == 1)
    {
        PIXScopedEvent(PIX_COLOR_DEFAULT, L"UITextStyle_RenderText_SingleLine - %d - %hs", m_fontHandle, content.c_str());
        auto fontTextCoordinates = GetFontTextCoordinates(
            m_fontHandle,
            content,
            screenRectMinusPadding);

        styleRenderer.DrawTextString(
            m_fontHandle,
            { content, fontTextCoordinates });
    }

    // the multiline case requires working line by line...
    else
    {
        PIXScopedEvent(PIX_COLOR_DEFAULT, L"UITextStyle_RenderText_MultiLine - %d - %hs", m_fontHandle, content.c_str());

        auto lineHeight = styleRenderer.GetScaledFontHeight(m_fontHandle);
        auto startingVerticalOffset = 0.0f;
        switch (m_textStyleProperties.m_vertAlignment)
        {
        case VerticalAlignment::Bottom:
            startingVerticalOffset = -lineHeight * float(lineCount - 1);
            break;
        case VerticalAlignment::Middle:
            startingVerticalOffset = -lineHeight * float(lineCount - 1) * 0.5f;
            break;
        case VerticalAlignment::Top:
        default:
            /* do nothing*/
            break;
        }

        for (size_t line = 0; line < lineCount; ++line)
        {
            auto currentVerticalOffset = float(line) * lineHeight;

            auto fontTextCoordinates = GetFontTextCoordinates(
                m_fontHandle,
                lines[line],
                screenRectMinusPadding);
            fontTextCoordinates.y += startingVerticalOffset + currentVerticalOffset;
            styleRenderer.DrawTextString(
                m_fontHandle,
                { lines[line], fontTextCoordinates });
        }
    }

    UIStyle::AfterRender();
    lines.resize(1);
}

/*private:*/

std::string UITextStyle::GetFontFilename()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UITextStyle_GetFontName");
    const char* c_weightSuffixes[] = { "", "_Italic", "_Bold" };
    std::stringstream ss;

    switch (m_textStyleProperties.m_fontType)
    {
    case FontType::Sprite:
        ss << m_textStyleProperties.m_typefaceName
            << '_' << m_textStyleProperties.m_typeSize
            << c_weightSuffixes[int(m_textStyleProperties.m_typeWeight)]
            << ".spritefont";
        break;
    case FontType::FreeType:
        ss << m_textStyleProperties.m_typefaceName;
        break;
    default:
        throw std::logic_error("FontType is not implemented");
    }
    
    return ss.str();
}

std::string UITextStyle::GetLegendFontFilename()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UITextStyle_GetLegendFontName");
    std::stringstream ss;
    ss << m_textStyleProperties.m_legendTypefaceName
            << ".spritefont";
    return ss.str();
}

size_t HashFontString(const FontHandle& fontHandle, const UIDisplayString& displayString)
{
    std::string_view temp = displayString;
    size_t hash = std::hash<std::string_view>{}(temp);
    hash = (hash << 6) | fontHandle;
    return hash;
}

Vector2 UITextStyle::GetLegendTextCoordinates(
    FontHandle& fontHandle,
    FontHandle& legendFontHandle,
    const UIDisplayString& displayString,
    const Rectangle& containingRectInPixels)
{
    static FrameComputedEvictCache<size_t, Rectangle> legendFontSizes(128, 120);

    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UITextStyle_GetLegendTextCoordinates");

    UIStyleRenderer& styleRenderer = m_styleManager.GetStyleRenderer();
    Rectangle legendSizeRect{};

    auto fontKey = HashFontString(legendFontHandle, displayString);
    auto fontSizeIter = legendFontSizes.Find(fontKey);
    if (fontSizeIter == legendFontSizes.End())
    {
        auto legendSize = MeasureLegendString(
            styleRenderer,
            fontHandle,
            legendFontHandle,
            displayString.c_str());
        legendSizeRect = Rectangle(0, 0, long(legendSize.x), long(legendSize.y));
        legendFontSizes.Add(fontKey, legendSizeRect);
    }
    else
    {
        legendSizeRect = fontSizeIter->second;
    }

    auto horzDifference = float(containingRectInPixels.width - legendSizeRect.width);
    auto vertDifference = float(containingRectInPixels.height - legendSizeRect.height);

    float x = float(containingRectInPixels.x);
    float y = float(containingRectInPixels.y);

    switch (m_textStyleProperties.m_horzAlignment)
    {
    case HorizontalAlignment::Left:
        // do nothing
        break;

    case HorizontalAlignment::Center:
        x += horzDifference * 0.5f;
        break;

    case HorizontalAlignment::Right:
        x += horzDifference;
        break;
    }

    switch (m_textStyleProperties.m_vertAlignment)
    {
    case VerticalAlignment::Top:
        // do nothing
        break;

    case VerticalAlignment::Middle:
        y += vertDifference * 0.5f;
        break;

    case VerticalAlignment::Bottom:
        y += vertDifference;
        break;
    }

    return Vector2(x, y);
}

Vector2 UITextStyle::GetFontTextCoordinates(
    FontHandle& fontHandle,
    const UIDisplayString& displayString,
    const Rectangle& containingRectInPixels)
{
    static FrameComputedEvictCache<size_t, Rectangle> fontSizes(1024, 120);

    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UITextStyle_GetFontTextCoordinates");

    UIStyleRenderer& styleRenderer = m_styleManager.GetStyleRenderer();
    Rectangle fontSizeRect{};

    auto fontKey = HashFontString(fontHandle, displayString);
    auto fontSizeIter = fontSizes.Find(fontKey);
    if (fontSizeIter == fontSizes.End())
    {
        fontSizeRect = styleRenderer.GetUnscaledFontTextSize(fontHandle, displayString);
        fontSizes.Add(fontKey, fontSizeRect);
    }
    else
    {
        fontSizeRect = fontSizeIter->second;
    }

    fontSizeRect.width = long(float(fontSizeRect.width) * styleRenderer.GetCurrentFontTextScale());
    fontSizeRect.height = long(float(fontSizeRect.height) * styleRenderer.GetCurrentFontTextScale());

    auto horzDifference = float(containingRectInPixels.width - fontSizeRect.width);
    auto vertDifference = float(containingRectInPixels.height - fontSizeRect.height);

    float x = float(containingRectInPixels.x);
    float y = float(containingRectInPixels.y);

    switch (m_textStyleProperties.m_horzAlignment)
    {
    case HorizontalAlignment::Left:
        // do nothing
        break;

    case HorizontalAlignment::Center:
        x += horzDifference * 0.5f;
        break;

    case HorizontalAlignment::Right:
        x += horzDifference;
        break;
    }

    switch (m_textStyleProperties.m_vertAlignment)
    {
    case VerticalAlignment::Top:
        // do nothing
        break;

    case VerticalAlignment::Middle:
        y += vertDifference * 0.5f;
        break;

    case VerticalAlignment::Bottom:
        y += vertDifference;
        break;
    }

    return Vector2(x, y);
}

/*public:*/

void StyleSpriteProperties::ApplyNonOverriddenProperties(
    const StyleSpriteProperties& other)
{
    STYLE_PROP_OVERRIDE(StyleSpritePropertyFlags::Texture, m_textureFilename);
    STYLE_PROP_OVERRIDE(StyleSpritePropertyFlags::TexelsPerRefUnit, m_texelsPerRefUnit);
    STYLE_PROP_OVERRIDE(StyleSpritePropertyFlags::Type, m_spriteType);
    STYLE_PROP_OVERRIDE_ARRAY(StyleSpritePropertyFlags::OuterUVExtents, m_outerUVExtents);
    STYLE_PROP_OVERRIDE_ARRAY(StyleSpritePropertyFlags::InnerUVExtents, m_innerUVExtents);
}

/*public:*/

void UISpriteStyle::Flatten()
{
    if (InheritsFromID())
    {
        auto ancestor = m_styleManager.GetTypedById<UISpriteStyle>(InheritsFromID());
        assert(ancestor);

        // flatten the ancestor first so that we only have to traverse
        // through properties one layer deep...

        ancestor->Flatten();

        // what this function is meant to do is to "bake" all of the style
        // data (including what is inherited) for easier access and rendering

        m_spriteStyleProperties.ApplyNonOverriddenProperties(ancestor->m_spriteStyleProperties);
        m_spriteStyleProperties.m_overriddenProperties = StyleSpriteProperties::c_allBits;

        UIStyle::Flatten();

        // we need to make sure our texture cache is up to date since we may have
        // a new texture based on inheritance
        m_textureHandle = m_styleManager.GetStyleRenderer().CacheTexture(
            m_spriteStyleProperties.m_textureFilename);
    }
}

void UISpriteStyle::RenderCroppedSimpleSprite(const Rectangle& screenRectInPixels, Vector2 percentages)
{
    UIStyle::BeforeRender();

    DrawDebugRectangle(screenRectInPixels);

    UIStyleRenderer& styleRenderer = m_styleManager.GetStyleRenderer();
    auto textureHandle = styleRenderer.CacheTexture(
        m_spriteStyleProperties.m_textureFilename);
    auto textureRect = styleRenderer.GetTextureRect(textureHandle);

    textureRect = UIMath::GetSubRectangle(
        textureRect,
        m_spriteStyleProperties.m_outerUVExtents[0],
        m_spriteStyleProperties.m_outerUVExtents[1]);

    percentages.Clamp(Vector2::Zero, Vector2::One);

    textureRect = UIMath::GetSubRectangle(textureRect, Vector2::Zero, percentages);

    styleRenderer.DrawTexturedQuads(textureHandle, { {textureRect, screenRectInPixels} });

    UIStyle::AfterRender();
}

std::vector<TexturedQuad> GenerateSlicedQuads(const Rectangle& screenRectInPixels, const Rectangle& textureRect, const StyleSpriteProperties& spriteStyleProperties)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UITextStyle_GenerateSlicedQuads");
    std::vector<TexturedQuad> quads;
    quads.reserve(9);

    auto sourceOuterRect = UIMath::GetSubRectangle(
        textureRect,
        spriteStyleProperties.m_outerUVExtents[0],
        spriteStyleProperties.m_outerUVExtents[1]);
    auto sourceInnerRect = UIMath::GetSubRectangle(
        textureRect,
        spriteStyleProperties.m_innerUVExtents[0],
        spriteStyleProperties.m_innerUVExtents[1]);
    Rectangle destinationInnerRect(
        screenRectInPixels.x + sourceInnerRect.x - sourceOuterRect.x,
        screenRectInPixels.y + sourceInnerRect.y - sourceOuterRect.y,
        screenRectInPixels.width - sourceOuterRect.width + sourceInnerRect.width,
        screenRectInPixels.height - sourceOuterRect.height + sourceInnerRect.height);

    long dx1 = screenRectInPixels.x;
    long dx2 = destinationInnerRect.x;
    long dx3 = destinationInnerRect.x + destinationInnerRect.width;
    long dx4 = screenRectInPixels.x + screenRectInPixels.width;

    long dy1 = screenRectInPixels.y;
    long dy2 = destinationInnerRect.y;
    long dy3 = destinationInnerRect.y + destinationInnerRect.height;
    long dy4 = screenRectInPixels.y + screenRectInPixels.height;

    long sx1 = sourceOuterRect.x;
    long sx2 = sourceInnerRect.x;
    long sx3 = sourceInnerRect.x + sourceInnerRect.width;
    long sx4 = sourceOuterRect.x + sourceOuterRect.width;

    long sy1 = sourceOuterRect.y;
    long sy2 = sourceInnerRect.y;
    long sy3 = sourceInnerRect.y + sourceInnerRect.height;
    long sy4 = sourceOuterRect.y + sourceOuterRect.height;

    // top row (left, center, right)
    quads.emplace_back(TexturedQuad{ RECT{sx1, sy1, sx2, sy2}, RECT{dx1, dy1, dx2, dy2} });
    quads.emplace_back(TexturedQuad{ RECT{sx2, sy1, sx3, sy2}, RECT{dx2, dy1, dx3, dy2} });
    quads.emplace_back(TexturedQuad{ RECT{sx3, sy1, sx4, sy2}, RECT{dx3, dy1, dx4, dy2} });

    // middle row (left, center, right)
    quads.emplace_back(TexturedQuad{ RECT{sx1, sy2, sx2, sy3}, RECT{dx1, dy2, dx2, dy3} });
    // NOTE: the center quad is only drawn if we are a nine-slice
    if (spriteStyleProperties.m_spriteType == SpriteType::NineSliced)
    {
        quads.emplace_back(TexturedQuad{ sourceInnerRect, destinationInnerRect });
    }
    quads.emplace_back(TexturedQuad{ RECT{sx3, sy2, sx4, sy3}, RECT{dx3, dy2, dx4, dy3} });

    // bottom row (left, center, right)
    quads.emplace_back(TexturedQuad{ RECT{sx1, sy3, sx2, sy4}, RECT{dx1, dy3, dx2, dy4} });
    quads.emplace_back(TexturedQuad{ RECT{sx2, sy3, sx3, sy4}, RECT{dx2, dy3, dx3, dy4} });
    quads.emplace_back(TexturedQuad{ RECT{sx3, sy3, sx4, sy4}, RECT{dx3, dy3, dx4, dy4} });

    return quads;
}

void UISpriteStyle::RenderSprite(const Rectangle& screenRectInPixels, TextureHandle overrideTexture)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UISpriteStyle_RenderSprite");
    UIStyle::BeforeRender();

    DrawDebugRectangle(screenRectInPixels);

    // NOTE: right now we are doing the most barebones attempt to
    // render a quad just to get things started...

    UIStyleRenderer& styleRenderer = m_styleManager.GetStyleRenderer();

    auto textureHandle = styleRenderer.IsValidTextureHandle(overrideTexture) ? overrideTexture : m_textureHandle;

    auto textureRect = styleRenderer.GetTextureRect(textureHandle);

    if (m_spriteStyleProperties.m_spriteType == SpriteType::Simple)
    {
        textureRect = UIMath::GetSubRectangle(
            textureRect,
            m_spriteStyleProperties.m_outerUVExtents[0],
            m_spriteStyleProperties.m_outerUVExtents[1]);
        styleRenderer.DrawTexturedQuads(textureHandle, { {textureRect, screenRectInPixels} });
    }
    else if (m_spriteStyleProperties.m_spriteType == SpriteType::NineSliced ||
        m_spriteStyleProperties.m_spriteType == SpriteType::EightSliced)
    {
        std::vector<TexturedQuad> quads = GenerateSlicedQuads(
            screenRectInPixels,
            textureRect,
            m_spriteStyleProperties);

        styleRenderer.DrawTexturedQuads(textureHandle, quads);
    }

    UIStyle::AfterRender();
}

ENUM_LOOKUP_TABLE(FontType,
    ID_ENUM_PAIR(UITK_VALUE(sprite), FontType::Sprite),
    ID_ENUM_PAIR(UITK_VALUE(freetype), FontType::FreeType)    
)

ENUM_LOOKUP_TABLE(TypeWeight,
    ID_ENUM_PAIR(UITK_VALUE(normal), TypeWeight::Normal),
    ID_ENUM_PAIR(UITK_VALUE(italic), TypeWeight::Italic),
    ID_ENUM_PAIR(UITK_VALUE(bold), TypeWeight::Bold)
)

ENUM_LOOKUP_TABLE(VerticalAlignment,
    ID_ENUM_PAIR(UITK_VALUE(top), VerticalAlignment::Top),
    ID_ENUM_PAIR(UITK_VALUE(middle), VerticalAlignment::Middle),
    ID_ENUM_PAIR(UITK_VALUE(bottom), VerticalAlignment::Bottom)
)

ENUM_LOOKUP_TABLE(HorizontalAlignment,
    ID_ENUM_PAIR(UITK_VALUE(left), HorizontalAlignment::Left),
    ID_ENUM_PAIR(UITK_VALUE(center), HorizontalAlignment::Center),
    ID_ENUM_PAIR(UITK_VALUE(right), HorizontalAlignment::Right)
)

ENUM_LOOKUP_TABLE(SpriteType,
    ID_ENUM_PAIR(UITK_VALUE(simple), SpriteType::Simple),
    ID_ENUM_PAIR(UITK_VALUE(nineSliced), SpriteType::NineSliced),
    ID_ENUM_PAIR(UITK_VALUE(eightSliced), SpriteType::EightSliced)
)

/*protected:*/

/*static*/ void UITextStyleFactory::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _Out_ StyleTextProperties& styleProperties)
{
    ID tempID;

    if (data->GetTo(UITK_FIELD(font), styleProperties.m_typefaceName))
    {
        styleProperties.m_overriddenProperties |= uint32_t(StyleTextPropertyFlags::TypeFace);
    }

    if (data->GetTo(UITK_FIELD(fontType), tempID))
    {
        styleProperties.m_overriddenProperties |= uint32_t(StyleTextPropertyFlags::FontType);
        UIEnumLookup(tempID, styleProperties.m_fontType, StyleTextProperties::c_defaultFontType);
    }

    if (data->GetTo(UITK_FIELD(fontSize), styleProperties.m_typeSize) ||
        // ...for backward compatibility
        data->GetTo(UITK_FIELD(size), styleProperties.m_typeSize)
        )
    {
        styleProperties.m_overriddenProperties |= uint32_t(StyleTextPropertyFlags::TypeSize);
    }

    if (data->GetTo(UITK_FIELD(weight), tempID))
    {
        styleProperties.m_overriddenProperties |= uint32_t(StyleTextPropertyFlags::Weight);
        UIEnumLookup(tempID, styleProperties.m_typeWeight, StyleTextProperties::c_defaultTypeWeight);
    }

    if (data->GetTo(UITK_FIELD(horizontalAlign), tempID))
    {
        styleProperties.m_overriddenProperties |= uint32_t(StyleTextPropertyFlags::HorzAlignment);
        UIEnumLookup(tempID, styleProperties.m_horzAlignment, StyleTextProperties::c_defaultHorizontalAlignment);
    }

    if (data->GetTo(UITK_FIELD(verticalAlign), tempID))
    {
        styleProperties.m_overriddenProperties |= uint32_t(StyleTextPropertyFlags::VertAlignment);
        UIEnumLookup(tempID, styleProperties.m_vertAlignment, StyleTextProperties::c_defaultVerticalAlignment);
    }

    if (data->GetTo(UITK_FIELD(legendFont), styleProperties.m_legendTypefaceName))
    {
        styleProperties.m_overriddenProperties |= uint32_t(StyleTextPropertyFlags::LegendTypeFace);
    }
}

/*protected:*/

/*static*/ void UISpriteStyleFactory::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _Out_ StyleSpriteProperties& styleProperties)
{
    ID tempID;
    if (data->GetTo(UITK_FIELD(texture), styleProperties.m_textureFilename))
    {
        styleProperties.m_overriddenProperties |= StyleSpritePropertyFlags::Texture;
    }

    if (data->GetTo(UITK_FIELD(texelsPerRefUnit), styleProperties.m_texelsPerRefUnit))
    {
        styleProperties.m_overriddenProperties |= StyleSpritePropertyFlags::TexelsPerRefUnit;
    }

    if (data->GetTo(UITK_FIELD(spriteType), tempID))
    {
        styleProperties.m_overriddenProperties |= StyleSpritePropertyFlags::Type;
        UIEnumLookup(tempID, styleProperties.m_spriteType);
    }

    Vector4 tempUV;
    if (data->GetTo(UITK_FIELD(outerUVExtents), tempUV))
    {
        tempUV.Clamp(Vector4::Zero, Vector4::One);
        styleProperties.m_overriddenProperties |= StyleSpritePropertyFlags::OuterUVExtents;
        styleProperties.m_outerUVExtents[0] = Vector2(tempUV.x, tempUV.y);
        styleProperties.m_outerUVExtents[1] = Vector2(tempUV.z, tempUV.w);
    }

    if (data->GetTo(UITK_FIELD(innerUVExtents), tempUV))
    {
        tempUV.Clamp(Vector4::Zero, Vector4::One);
        styleProperties.m_overriddenProperties |= StyleSpritePropertyFlags::InnerUVExtents;
        styleProperties.m_innerUVExtents[0] = Vector2(tempUV.x, tempUV.y);
        styleProperties.m_innerUVExtents[1] = Vector2(tempUV.z, tempUV.w);
    }
}

/*public:*/

void StyleProperties::ApplyNonOverriddenProperties(
    const StyleProperties& other)
{
    STYLE_PROP_OVERRIDE(StylePropertyFlags::TintColor, m_colorRGBA);
    STYLE_PROP_OVERRIDE(StylePropertyFlags::TintColorUsage, m_colorUsage);
    STYLE_PROP_OVERRIDE_ARRAY(StylePropertyFlags::Margin, m_margin);
    STYLE_PROP_OVERRIDE_ARRAY(StylePropertyFlags::Padding, m_padding);
}

/*public:*/

bool UIStyle::s_doDebugDraw = false;

/*public:*/

/*virtual*/ void UIStyle::Flatten()
{
    if (InheritsFromID())
    {
        auto ancestor = m_styleManager.GetById(InheritsFromID());
        assert(ancestor);

        m_styleProperties.ApplyNonOverriddenProperties(ancestor->m_styleProperties);
        m_styleProperties.m_overriddenProperties = StyleProperties::c_allBits;

        m_inheritsFromId = ID();
    }
}

/*virtual*/ void UIStyle::BeforeRender()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyle_BeforeRender");
    if (m_styleProperties.m_colorUsage == ColorUsage::SetExisting ||
        m_styleProperties.m_colorUsage == ColorUsage::Override)
    {
        UIStyleRenderer& styleRenderer = m_styleManager.GetStyleRenderer();
        m_colorStackIndex = styleRenderer.PushTintColor(m_styleProperties.m_colorRGBA);
    }
}

/*virtual*/ void UIStyle::AfterRender()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyle_AfterRender");
    if (m_styleProperties.m_colorUsage == ColorUsage::Override)
    {
        UIStyleRenderer& styleRenderer = m_styleManager.GetStyleRenderer();
        styleRenderer.PopTintColor(m_colorStackIndex);
    }
}

/*virtual*/ void UIStyle::PostRender()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyle_PostRender");
    if (m_styleProperties.m_colorUsage == ColorUsage::SetExisting)
    {
        UIStyleRenderer& styleRenderer = m_styleManager.GetStyleRenderer();
        styleRenderer.PopTintColor(m_colorStackIndex);
    }
}

Rectangle UIStyle::AddMarginToRect(const Rectangle& rectangle) const
{
    return UIMath::AddOffsetsFromRect(rectangle, m_styleProperties.m_margin);
}

Rectangle UIStyle::SubtractMarginFromRect(const Rectangle& rectangle) const
{
    return UIMath::SubtractOffsetsFromRect(rectangle, m_styleProperties.m_margin);
}

Rectangle UIStyle::SubtractPaddingFromRect(const Rectangle& rectangle) const
{
    return UIMath::SubtractOffsetsFromRect(rectangle, m_styleProperties.m_padding);
}

/*static*/ void UIStyleFactoryBase::DeserializeDataProperties(
    _In_ UIDataPtr data,
    _Out_ StyleProperties& styleProperties)
{
    ID temp;
    Color color;

    if (data->GetTo(UITK_FIELD(color), color))
    {
        data->Dump();
        styleProperties.m_overriddenProperties |= uint32_t(StylePropertyFlags::TintColor);
        styleProperties.m_colorRGBA = color;
    }

    if (data->GetTo(UITK_FIELD(colorUsage), temp))
    {
        styleProperties.m_overriddenProperties |= uint32_t(StylePropertyFlags::TintColorUsage);
        UIEnumLookup(temp, styleProperties.m_colorUsage);        
    }

    if (data->GetTo<Offsets>(UITK_FIELD(margin), styleProperties.m_margin))
    {
        styleProperties.m_overriddenProperties |= uint32_t(StylePropertyFlags::Margin);
    }

    if (data->GetTo<Offsets>(UITK_FIELD(padding), styleProperties.m_padding))
    {
        styleProperties.m_overriddenProperties |= uint32_t(StylePropertyFlags::Padding);
    }
}

NAMESPACE_ATG_UITK_END
