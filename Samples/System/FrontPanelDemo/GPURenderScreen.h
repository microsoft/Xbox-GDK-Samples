//--------------------------------------------------------------------------------------
// GPURenderScreen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "PanelScreen.h"
#include "NavigationHint.h"

#include "FrontPanel/FrontPanelRenderTarget.h"

class GPURenderScreen : public PanelScreen
{
public:
    GPURenderScreen(FrontPanelManager *owner);
    void RenderFrontPanel() override;
    void CreateDeviceDependentResources(DX::DeviceResources *deviceResources) override;
    void CreateWindowSizeDependentResources(DX::DeviceResources *deviceResources) override;
    void GPURender(DX::DeviceResources *deviceResources) override;

private:
    // Front Panel Render Target
    // Helper class to convert a GPU resource to grayscale and then render to the Front Panel
    std::unique_ptr<ATG::FrontPanelRenderTarget>    m_frontPanelRenderTarget;
	
    BasicNavigationHint                             m_nav;
};

