//--------------------------------------------------------------------------------------
// DeferredParticles.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include "Particles.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:
    Sample() noexcept(false);
    ~Sample();

    // Initialization and management
    void Initialize(HWND window);

    // Basic Sample loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

    // Properties
    bool RequestHDRMode() const { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

    // Helper callback functions that setup the pipeline state for the scene & sky mesh types.
    void SetupPipelineScene(ID3D12GraphicsCommandList* commandList, const DirectX::ModelMeshPart& part, int instIndex, int texOffset);
    void SetupPipelineSky(ID3D12GraphicsCommandList* commandList, const DirectX::ModelMeshPart& part, int instIndex, int texOffset);

private:
    // Represents an instance of a scene object.
    struct ObjectInstance
    {
        using DrawCallback = DirectX::ModelMeshPart::DrawCallback;

        DirectX::XMFLOAT4X4 World;
        DirectX::Model*     Model;
        DrawCallback        SetupFunc;
    };

private:
    void Update(const DX::StepTimer& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

private:
    // Common pipeline setup function leveraged by SetupPipeline*(...)
    void SetupPipelineCommon(ID3D12GraphicsCommandList* commandList, const DirectX::ModelMeshPart& part, int instIndex, int texOffset, DirectX::FXMMATRIX view);

    // indexes of root parameters
    enum RootParameters
    {
        RootParamCBModel = 0,
        RootParamCBScene,
        RootParamCBGlowLights,
        RootParamSRV,
        RootParamCount
    };

    enum DescriptorHeapIndex
    {
        SRV_Font = 0,
        SRV_CtrlFont,
        SRV_Count
    };

private:
    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;
    int                                             m_displayWidth;
    int                                             m_displayHeight;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;

    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::CommonStates>          m_commonStates;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_textureFactory;

    // HUD
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;

    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_scenePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_skyPSO;

    // Scene transform data
    DirectX::SimpleMath::Vector3                    m_camPos;
    DirectX::SimpleMath::Matrix                     m_view;
    DirectX::SimpleMath::Matrix                     m_proj;

    std::vector<std::unique_ptr<DirectX::Model>>    m_models;
    std::vector<ObjectInstance>                     m_scene;
    std::unique_ptr<DirectX::DescriptorPile>        m_srvPile;

    ATG::ParticleWorld                              m_particleWorld;
    D3D12_GPU_VIRTUAL_ADDRESS                       m_sceneCBAddr;

    float                                           m_orbitRadius;
    float                                           m_orbitAngle;
    float                                           m_cameraHeight;

    bool                                            m_isPaused;
};
