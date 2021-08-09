//--------------------------------------------------------------------------------------
// QuickActionMappingScreen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "PanelScreen.h"
#include "NavigationHint.h"
#include "FrontPanelManager.h"

class QuickActionMappingScreen : public PanelScreen
{
public:
    QuickActionMappingScreen(FrontPanelManager *owner);
    void RenderFrontPanel() override;
    bool OnButtonPressed(XFrontPanelButton whichButton) override;

private:
    XFrontPanelButton GetActionAssignment(const FrontPanelManager::ActionRecord &action) const;
    bool ChangeOrToggleAssignment(XFrontPanelButton whichButton);

    int                 m_currentActionIndex;
    BasicNavigationHint m_nav;
};
