//--------------------------------------------------------------------------------------
// QuickActionMappingScreen.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"

#include "QuickActionMappingScreen.h"
#include "FontManager.h"
#include "FrontPanelManager.h"

#include "FrontPanel/FrontPanelDisplay.h"

using namespace ATG;

QuickActionMappingScreen::QuickActionMappingScreen(FrontPanelManager * owner)
    : PanelScreen(owner)
    , m_currentActionIndex(0)
{
}

void QuickActionMappingScreen::RenderFrontPanel()
{
    // Get the fonts
    ATG::RasterFont &titleFont = FontManager::Instance().LoadFont(L"assets\\Segoe_UI_bold16.rasterfont");
    ATG::RasterFont &descriptionFont = FontManager::Instance().LoadFont(L"assets\\Segoe_UI16.rasterfont");
    ATG::RasterFont &symbolFont = FontManager::Instance().LoadFont(L"assets\\Symbols32.rasterfont");
    ATG::RasterFont &buttonFont = FontManager::Instance().LoadFont(L"assets\\Segoe_UI_bold24.rasterfont");

    // Render to the front panel
    auto& frontPanelDisplay = FrontPanelDisplay::Get();

    frontPanelDisplay.Clear();

    BufferDesc fpDesc = frontPanelDisplay.GetBufferDescriptor();

    size_t count = m_owner->ButtonActionCount();
    int x = 0;
    int y = 0;

    // Draw the title text
    if(count == 0)
    {
        titleFont.DrawString(fpDesc, unsigned(x), unsigned(y), L"NO ACTIONS DEFINED");
        if (m_upNeighbor)
        {
            m_nav.DrawUpIndicator(fpDesc, x, y);
        }
        frontPanelDisplay.Present();
        return;
    }
    else
    {
        titleFont.DrawString(fpDesc, unsigned(x), unsigned(y), L"Press any button to change/toggle:");
    }
    y += titleFont.GetLineSpacing();
    x = 40;

    auto action = (*m_owner)[size_t(m_currentActionIndex)];
    XFrontPanelButton btn = GetActionAssignment(action);
    bool isActionAssigned = (btn != XFrontPanelButton::None);

    // Draw the button graphic
    {
        wchar_t buttonGlyph = isActionAssigned ? wchar_t(FrontPanelManager::GetIndexForButtonId(btn) + L'1') : L'?';
        int bx = 2;
        int by = 16;

        symbolFont.DrawGlyph(fpDesc, unsigned(bx), unsigned(by), 0xE48C, isActionAssigned ? 0xF0u : 0x40u);

        RECT rCrcl = symbolFont.MeasureGlyph(0xE48C);
        float wCrcl = float(rCrcl.right - rCrcl.left);
        float hCrcl = float(rCrcl.bottom - rCrcl.top);

        RECT rBtn = buttonFont.MeasureGlyph(buttonGlyph);
        float wBtn = float(rBtn.right - rBtn.left);
        float hBtn = float(rBtn.bottom - rBtn.top);

        bx += int(floor(0.5f + (wCrcl - wBtn) / 2.0f));
        by += int(floor(0.5f + (hCrcl - hBtn) / 2.0f));

        buttonFont.DrawGlyph(fpDesc, unsigned(bx), unsigned(by), buttonGlyph, 0x00u);
    }

    // Draw the description text
    descriptionFont.DrawStringFmt(fpDesc, unsigned(x), unsigned(y), L"%s\n%s", action.name.c_str(), action.description.c_str());
    
    // Draw the navigation hints
    {
        x = int(fpDesc.width - m_nav.GetWidth());
        y = 0;

        if(m_currentActionIndex == 0)
        {
            if (m_upNeighbor)
            {
                m_nav.DrawUpIndicator(fpDesc, x, y);
            }
        }

        if (m_currentActionIndex > 0)
        {
            m_nav.DrawUpIndicator(fpDesc, x, y);
        }

        if (m_currentActionIndex < int(count - 1))
        {
            m_nav.DrawDownIndicator(fpDesc, x, y);
        }
    }

    frontPanelDisplay.Present();
}

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#pragma warning(disable : 4061)

bool QuickActionMappingScreen::OnButtonPressed(XFrontPanelButton whichButton)
{
    switch (whichButton)
    {
    default:
        break;

    case XFrontPanelButton::DPadUp:
        if (m_currentActionIndex == 0)
        {
            if (m_upNeighbor)
            {
                m_upNeighbor->Resume(this);
                return true;
            }
        }
        else if (m_currentActionIndex > 0)
        {
            --m_currentActionIndex;
            RenderFrontPanel();
            return true;
        }
        break;

    case XFrontPanelButton::DPadDown:
        if (m_currentActionIndex < int(m_owner->ButtonActionCount() - 1))
        {
            ++m_currentActionIndex;
            RenderFrontPanel();
            return true;
        }
        break;

    case XFrontPanelButton::Button1:
    case XFrontPanelButton::Button2:
    case XFrontPanelButton::Button3:
    case XFrontPanelButton::Button4:
    case XFrontPanelButton::Button5:
        if (unsigned(m_currentActionIndex) < m_owner->ButtonActionCount())
        {
            return ChangeOrToggleAssignment(whichButton);
        }
        break;
    
    }

    return false;
}

XFrontPanelButton QuickActionMappingScreen::GetActionAssignment(const FrontPanelManager::ActionRecord & action) const
{
    XFrontPanelButton ButtonList[5] = 
    {
        XFrontPanelButton::Button1, 
        XFrontPanelButton::Button2,
        XFrontPanelButton::Button3,
        XFrontPanelButton::Button4,
        XFrontPanelButton::Button5
    };
    for (int i = 0; i < 5; ++i)
    {
        XFrontPanelButton btn = ButtonList[i];
        if (m_owner->IsActionAssigned(btn))
        {
            auto& other = m_owner->GetActionAssignment(btn);
            if (action == other)
                return btn;
        }
    }

    return XFrontPanelButton::None;
}

bool QuickActionMappingScreen::ChangeOrToggleAssignment(XFrontPanelButton whichButton)
{
    auto& action = (*m_owner)[size_t(m_currentActionIndex)];
    XFrontPanelButton assignedButton = GetActionAssignment(action);
 
    // If the action is assigned, then clear the current assignement
    if (assignedButton != XFrontPanelButton::None)
    {
        m_owner->ClearActionAssignment(assignedButton);
    }

    // Assign the action to another button (but not the previous assigned button)
    if (whichButton != assignedButton)
    {
        m_owner->AssignActionToButton(action, whichButton);
    }
    RenderFrontPanel();
    return true;
}
