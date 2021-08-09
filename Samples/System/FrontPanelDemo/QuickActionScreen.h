//--------------------------------------------------------------------------------------
// QuickActionScene.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "PanelScreen.h"
#include "NavigationHint.h"
#include "StepTimer.h"

class QuickActionScreen : public PanelScreen
{
public:
    QuickActionScreen(FrontPanelManager *owner, XFrontPanelButton buttonId);

    void Update(DX::StepTimer const& timer) override;
    void RenderFrontPanel() override;

private:
    void SetLightState(bool on);

    XFrontPanelButton   m_myButton;
    BasicNavigationHint m_nav;
    bool                m_curLightState;
    double              m_blinkSeconds;

};
