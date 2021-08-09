//--------------------------------------------------------------------------------------
// GPURenderScreen.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"

#include "GPURenderScreen.h"

#include "FrontPanel/FrontPanelDisplay.h"

using namespace DirectX;
using namespace ATG;

GPURenderScreen::GPURenderScreen(FrontPanelManager * owner)
    : PanelScreen(owner)
{
    // Set up the front panel render target
    m_frontPanelRenderTarget = std::make_unique<FrontPanelRenderTarget>();
}

void GPURenderScreen::RenderFrontPanel()
{
    auto& frontPanelDisplay = FrontPanelDisplay::Get();
    BufferDesc fpDesc = frontPanelDisplay.GetBufferDescriptor();
    
    // Draw the navigation hints
    {
        int x = int(fpDesc.width - m_nav.GetWidth());
        int y = 0;

        if (m_leftNeighbor)
        {
            m_nav.DrawLeftIndicator(fpDesc, x, y);
        }

        if (m_rightNeighbor)
        {
            m_nav.DrawRightIndicator(fpDesc, x, y);
        }

        if (m_upNeighbor)
        {
            m_nav.DrawUpIndicator(fpDesc, x, y);
        }

        if (m_downNeighbor)
        {
            m_nav.DrawDownIndicator(fpDesc, x, y);
        }
    }

    frontPanelDisplay.Present();
}

void GPURenderScreen::CreateDeviceDependentResources(DX::DeviceResources * deviceResources)
{
    auto device = deviceResources->GetD3DDevice();
    m_frontPanelRenderTarget->CreateDeviceDependentResources(device, deviceResources->GetBackBufferCount());
}

void GPURenderScreen::CreateWindowSizeDependentResources(DX::DeviceResources * deviceResources)
{
    auto device = deviceResources->GetD3DDevice();

    // Create the front panel render target resources
    // Assuming max of 3 render targets
    ID3D12Resource* pRenderTargets[3] = {};
    for(unsigned int rtIndex = 0; rtIndex < deviceResources->GetBackBufferCount(); ++rtIndex)
    {
        pRenderTargets[rtIndex] = deviceResources->GetRenderTarget(rtIndex);
    }
    m_frontPanelRenderTarget->CreateWindowSizeDependentResources(
        device, 
        deviceResources->GetBackBufferCount(),
        pRenderTargets);
}

void GPURenderScreen::GPURender(DX::DeviceResources * deviceResources)
{
    auto& frontPanelDisplay = FrontPanelDisplay::Get();
    auto device = deviceResources->GetD3DDevice();
    auto commandList = deviceResources->GetCommandList();
    auto commandQueue = deviceResources->GetCommandQueue();
    unsigned int renderTargetIndex = deviceResources->GetCurrentFrameIndex();
    auto renderTarget = deviceResources->GetRenderTarget();
    auto fpDesc = frontPanelDisplay.GetBufferDescriptor();

    m_frontPanelRenderTarget->GPUBlit(commandList, renderTarget, renderTargetIndex);
    m_frontPanelRenderTarget->CopyToBuffer(device, commandQueue, renderTargetIndex, fpDesc);

    RenderFrontPanel();
}
