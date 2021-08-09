//--------------------------------------------------------------------------------------
// FontViewerScreen.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"

#include "FontViewerScreen.h"
#include "FontManager.h"
#include "FrontPanel/FrontPanelDisplay.h"

using namespace ATG;

const wchar_t *FontViewerScreen::s_defaultSampleText = L"ABCEDFGHIJKLMNOPQRSTUVWXYZ\nabcdefghijklmnopqrstuvwxyz\n0123456789";

FontViewerScreen::FontViewerScreen(FrontPanelManager * owner, const wchar_t * faceName, unsigned titleHeight, const wchar_t * fileName)
    : PanelScreen(owner)
    , m_titleHeight(titleHeight)
    , m_faceName(faceName)
{
    m_currentFont = m_heightToFontFile.cbegin();
    AddFontFile(titleHeight, fileName);
}

void FontViewerScreen::AddFontFile(unsigned height, const wchar_t * fileName, const wchar_t *sampleText)
{
    unsigned curHeight = height;
    if (m_currentFont != m_heightToFontFile.cend())
    {
        curHeight = m_currentFont->first;
    }

    FontData data{ fileName, sampleText };

    m_heightToFontFile[height] = data;

    m_currentFont = m_heightToFontFile.find(curHeight);
}

void FontViewerScreen::RenderFrontPanel()
{
    // Get the fonts
    ATG::RasterFont &titleFont = FontManager::Instance().LoadFont(m_heightToFontFile[m_titleHeight].filename);
    ATG::RasterFont &curFont = FontManager::Instance().LoadFont(m_currentFont->second.filename);

    auto &frontPanelDisplay = FrontPanelDisplay::Get();
    frontPanelDisplay.Clear();

    BufferDesc fpDesc = frontPanelDisplay.GetBufferDescriptor();

    unsigned int x = 0;
    unsigned int y = 0;

    // Draw the title text
    titleFont.DrawStringFmt(fpDesc, x, y, L"%s %u", m_faceName, m_currentFont->first);
    y += titleFont.GetLineSpacing();

    // Draw the sample text
    curFont.DrawString(fpDesc, x, y, m_currentFont->second.sampleText);

    // Draw the navigation hints
    {
        x = fpDesc.width - m_nav.GetWidth();
        y = 0;

        if (m_currentFont != m_heightToFontFile.cbegin())
        {
            m_nav.DrawLeftIndicator(fpDesc, int(x), int(y));
        }

        auto tmp = m_currentFont;
        ++tmp;
        if (tmp != m_heightToFontFile.cend())
        {
            m_nav.DrawRightIndicator(fpDesc, int(x), int(y));
        }

        if (m_upNeighbor)
        {
            m_nav.DrawUpIndicator(fpDesc, int(x), int(y));
        }

        if (m_downNeighbor)
        {
            m_nav.DrawDownIndicator(fpDesc, int(x), int(y));
        }
    }

    frontPanelDisplay.Present();
}

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#pragma warning(disable : 4061)

bool FontViewerScreen::OnButtonPressed(XFrontPanelButton whichButton)
{
    switch (whichButton)
    {
    case XFrontPanelButton::DPadLeft:
        if (m_currentFont != m_heightToFontFile.cbegin())
        {
            --m_currentFont;
            RenderFrontPanel();
            return true;
        }
        break;

    case XFrontPanelButton::DPadRight:
    {
        auto tmp = m_currentFont;
        ++tmp;
        if (tmp != m_heightToFontFile.cend())
        {
            m_currentFont = tmp;
            RenderFrontPanel();
            return true;
        }
    }
    break;

    case XFrontPanelButton::DPadUp:
        if (m_upNeighbor)
        {
            m_upNeighbor->Resume(this);
            return true;
        }
        break;

    case XFrontPanelButton::DPadDown:
        if (m_downNeighbor)
        {
            m_downNeighbor->Resume(this);
            return true;
        }
        break;

    default:
        break;
    }

    return false;
}
