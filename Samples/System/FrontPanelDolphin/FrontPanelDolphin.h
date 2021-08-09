//--------------------------------------------------------------------------------------
// FrontPanelDolphin.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Dolphin.h"

#include "FrontPanel\FrontPanelRenderTarget.h"
#include "FrontPanel\FrontPanelDisplay.h"
#include "FrontPanel\FrontPanelInput.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void SetWaterColor(float red, float green, float blue);

    void DrawDolphin(Dolphin &dolphin);

private:

    // Device resources.
    std::unique_ptr<DX::DeviceResources>             m_deviceResources;

    // Rendering loop timer.
    uint64_t                                         m_frame;
    DX::StepTimer                                    m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>                m_gamePad;
    DirectX::GamePad::ButtonStateTracker             m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>         m_graphicsMemory;
    std::unique_ptr<DirectX::ResourceUploadBatch>    m_resourceUploadBatch;
    std::unique_ptr<DirectX::EffectTextureFactory>   m_textureFactory;
    enum TextureDescriptors
    {
        SeaFloor = 0,
        DolphinSkin = 1,
        CausticFirst = 2,
        CausticLast = CausticFirst + 31,
        Count
    };

    // Game state                                    
    Microsoft::WRL::ComPtr<ID3D12Resource>           m_VSConstants;
    Microsoft::WRL::ComPtr<ID3D12Resource>           m_PSConstants;
    ATG::VS_CONSTANT_BUFFER*                         m_mappedVSConstantData;
    ATG::PS_CONSTANT_BUFFER*                         m_mappedPSConstantData;
    D3D12_GPU_VIRTUAL_ADDRESS                        m_VSConstantDataGpuAddr;
    D3D12_GPU_VIRTUAL_ADDRESS                        m_PSConstantDataGpuAddr;

    // Transform matrices                            
    DirectX::SimpleMath::Matrix                      m_matView;
    DirectX::SimpleMath::Matrix                      m_matProj;

    // array of dolphins                             
    std::array<std::shared_ptr<Dolphin>, 4>          m_dolphins;

    // Seafloor object
    std::unique_ptr<DirectX::Model>                  m_seafloor;
    std::unique_ptr<DirectX::IEffect>                m_seaEffect;

    // Water caustics
    unsigned int                                     m_currentCausticTextureView;

    float                                            m_waterColor[4];
    float                                            m_ambient[4];

    // Front Panel Render Target
    // Helper class to convert a GPU resource to grayscale and then render to the Front Panel
    std::unique_ptr<ATG::FrontPanelRenderTarget>     m_frontPanelRenderTarget; 

    // FrontPanel objects
    std::unique_ptr<ATG::FrontPanelDisplay>          m_frontPanelDisplay;
    std::unique_ptr<ATG::FrontPanelInput>            m_frontPanelInput;
    ATG::FrontPanelInput::ButtonStateTracker         m_frontPanelInputButtons;

    // Remember availability of the Front Panel
    bool                                             m_available;
};
