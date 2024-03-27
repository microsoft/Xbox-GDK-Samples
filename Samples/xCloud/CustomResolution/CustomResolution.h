//--------------------------------------------------------------------------------------
// CustomResolution.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "PerformanceTimersXbox.h"
#include "ResolutionSet.h"
#include "StepTimer.h"

// UI toolkit
#include "UIManager.h"
#include "UIInputState.h"
#include "UIWidgets.h"
#include "UIStyleRendererD3D.h"

#include "XGameStreaming.h"


using namespace ATG::UITK;

namespace
{
    // Constant buffer struct for sending the sphere data
    // No need to pad, there is only one instance of this CB needed for all frames
    struct sphereConstantBufferStruct
    {
        DirectX::SimpleMath::Matrix  g_mMVP;      // Model View Projection matrix
    };
    struct sphereConstantBufferStructPadded
    {
        sphereConstantBufferStruct  data;
#if _GAMING_DESKTOP
        uint8_t                     padding[192];
#endif
    };
    static_assert(sizeof(sphereConstantBufferStructPadded) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0 &&
        "Allignment issue for constant buffer.");
}

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample : public D3DResourcesProvider
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);
    void InitializeUI();
    bool CheckDeviceType();

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

    // Properties
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

    // UIStyleManager::D3DResourcesProvider interface methods
    virtual ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    virtual ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    virtual ID3D12GraphicsCommandList* GetCommandList() const override
    {
        return m_deviceResources->GetCommandList();
    }


    XTaskQueueHandle GetTaskQueue() const noexcept { return m_queue; };

    void RetrieveStreamingCharacteristics();
    void SetAutoResolution();
    std::map<XGameStreamingClientId, XTaskQueueRegistrationToken> m_clients;

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void RenderUI();

    void SetResolution(uint32_t width, uint32_t height);
    void DetermineResolution();

    void SetAndClearTargets(
        const D3D12_VIEWPORT* viewport,
        const D3D12_CPU_DESCRIPTOR_HANDLE* rtvDescriptor,
        const D3D12_CPU_DESCRIPTOR_HANDLE* dsvDescriptor,
        const float* colorRGBA);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Represents an instance of a scene object.
    struct ObjectInstance
    {
        using EffectList = DirectX::Model::EffectCollection;

        DirectX::SimpleMath::Matrix world;
        DirectX::Model*             model;
        EffectList                  effects;
    };

    static constexpr uint32_t                       c_numBackBuffers = 3;

    XSystemDeviceType                               m_deviceType;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Input device.
    ATG::UITK::UIInputState                         m_uiInputState;
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    
    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::CommonStates>          m_commonStates;
    std::unique_ptr<DirectX::DescriptorPile>        m_srvPile;
    
    // HUD
    ATG::UITK::UIManager                            m_uiManager;
    std::shared_ptr<ATG::UITK::UIPanel>             m_sidePanel;
    std::shared_ptr<ATG::UITK::UIStaticText>        m_dynamicRezText;
    std::shared_ptr<ATG::UITK::UIStaticText>        m_presetText;
    std::shared_ptr<ATG::UITK::UIStaticText>        m_fpsText;
    std::shared_ptr<ATG::UITK::UIStaticText>        m_text1;
    std::shared_ptr<ATG::UITK::UIStaticText>        m_text2;
    std::shared_ptr<ATG::UITK::UIStaticText>        m_text3;

    std::unique_ptr<DirectX::SpriteBatch>           m_uiSpriteBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;

    // Camera
    float                                           m_cameraAngle;
    float                                           m_cameraElevation;
    float                                           m_cameraDistance;
    DirectX::SimpleMath::Matrix                     m_proj;
    DirectX::SimpleMath::Matrix                     m_view;

    // Assets & Scene                                    
    std::unique_ptr<DirectX::EffectTextureFactory>  m_textureFactory;
    std::vector<std::unique_ptr<DirectX::Model>>    m_models;
    std::vector<ObjectInstance>                     m_scene;

    // Sample settings

    // The adjustable dimensions
    uint32_t                                        m_frameWidth;
    uint32_t                                        m_frameHeight;
    D3D12_VIEWPORT                                  m_frameViewportDynamic;
    D3D12_VIEWPORT                                  m_frameViewportIdeal;

    static constexpr uint32_t                       c_frameWidthIncrement = 1920 / 10;
    static constexpr uint32_t                       c_frameHeightIncrement = 0;
    static constexpr uint32_t                       c_frameWidthMinimum = 800;
    static constexpr uint32_t                       c_frameHeightMinimum = 600;
    static constexpr uint32_t                       c_frameWidthMaximum = 3840;
    static constexpr uint32_t                       c_frameHeightMaximum = 2160;
    static constexpr float                          c_aspectWidest = 64.f / 9.f;
    static constexpr float                          c_aspectTallest = 4.f / 9.f;

    // Resources for the possible resolutions
    ResolutionSet                                   m_resolutionSet;
    ResolutionData                                  m_currentRezData;

    // Sphere Mesh
    std::unique_ptr<DirectX::GeometricPrimitive>    m_sphereMesh;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_sphereMeshCBResource;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_spherePSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_sphereRS;

    // Flag to ensure thread finishes cleanly at sample exit
    std::atomic<bool>                               m_isExiting;

    XTaskQueueRegistrationToken                     m_token;
    XTaskQueueHandle                                m_queue;
};
