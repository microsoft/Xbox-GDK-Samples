//--------------------------------------------------------------------------------------
// BankRotation.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// DirectXTK headers
#include "ResourceUploadBatch.h"
#include "Model.h"

// ATGTK headers
#include "ControllerFont.h"
#include "FlyCamera.h"
#include "ControllerHelp.h"

#include "DeviceResources.h"
#include "StepTimer.h"
#include "SampleEffect.h"
#include "PerformanceTimersXbox.h"

namespace
{
    constexpr uint32_t c_NumGBuffersPerPass = 4;

    // Set GBuffer formats and names
    static DXGI_FORMAT gbufferFormat[c_NumGBuffersPerPass] =
    {
        DXGI_FORMAT_B8G8R8A8_UNORM,         // Diffuse
        DXGI_FORMAT_R10G10B10A2_UNORM,      // Normal
        DXGI_FORMAT_B8G8R8A8_UNORM,         // Gbuffer2
        DXGI_FORMAT_B8G8R8A8_UNORM,         // Gbuffer3
    };
    static LPCWSTR gbufferName[c_NumGBuffersPerPass] =
    {
        L"Diffuse Buffer",
        L"Normal GBuffer",
        L"Tangent GBuffer",
        L"Bitangent GBuffer",
    };
    enum class ResourceType
    {
        COMMITTED,
        PLACED,
        COMPONENT_PLACED,
        TOTAL_RESOURCE_TYPES
    };
};

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);

    ~Sample();

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();
    void OnConstrained() {}
    void OnUnConstrained() {}

    // Properties
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // Resources
    uint32_t                                    m_descrptorIncrementSizeCBVSRVUAV;

    // Effect
    struct ModelEffect
    {
        std::shared_ptr<SampleEffect>                  m_effect;
        std::unique_ptr<DirectX::Model>                m_model;
        std::unique_ptr<DirectX::EffectTextureFactory> m_textures;
        DirectX::Model::EffectCollection               m_meshPartsEffects;
    };
    ModelEffect                                        m_modelEffect;
    std::unique_ptr<SampleEffectFactory>               m_effectFactory;

    // Mesh
    void LoadMesh(ID3D12Device * device, DirectX::ResourceUploadBatch& resourceUpload);

    // Committed Resources
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>       m_rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>       m_srvDescriptorHeap;
    struct GBuffer
    {
        D3D12_CPU_DESCRIPTOR_HANDLE                    m_rtvCpuHandle{};
        D3D12_CPU_DESCRIPTOR_HANDLE                    m_srvCpuHandle{};
        D3D12_GPU_DESCRIPTOR_HANDLE                    m_srvGpuHandle{};
        D3D12_RESOURCE_STATES                          m_state{};
        Microsoft::WRL::ComPtr<ID3D12Resource>         m_resource;
    };
    GBuffer                                            m_gbuffersCommitted[c_NumGBuffersPerPass];
    GBuffer                                            m_gbuffersCommittedRotated[c_NumGBuffersPerPass];
    GBuffer                                            m_gbuffersPlaced[c_NumGBuffersPerPass];
    GBuffer                                            m_gbuffersPlacedRotated[c_NumGBuffersPerPass];
    GBuffer                                            m_gbuffersComponentPlaced[c_NumGBuffersPerPass];
    GBuffer                                            m_gbuffersComponentPlacedRotated[c_NumGBuffersPerPass];
    static constexpr uint32_t                          c_NumGbufferSets = 6;

    void CreateResources(ID3D12Device* device);
    void RenderMesh(ID3D12GraphicsCommandList* commandList, GBuffer* gbuffers, uint32_t currFrameIndex, uint32_t timerCounter);

    // Rect Resources
    D3D12_VERTEX_BUFFER_VIEW                           m_rectVBView;
    Microsoft::WRL::ComPtr<ID3D12Resource>             m_rectVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>        m_rectPso;
    void CreateRectResources(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload);
    void RenderRect(ID3D12GraphicsCommandList* commandList, GBuffer* currGbuffer, bool rotated);
    void RenderGbufferToBackbuffer(ID3D12GraphicsCommandList* commandList, GBuffer* currGbuffer, bool rotated);

    // UI
    std::unique_ptr<DirectX::SpriteBatch>              m_fontBatch;
    std::unique_ptr<DirectX::SpriteFont>               m_fontText;
    std::unique_ptr<DirectX::SpriteFont>               m_fontController;
    void InitializeSpriteFonts(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload, const DirectX::RenderTargetState& rtState);
    void RenderUI(ID3D12GraphicsCommandList* graphicsCmdList);

    // Camera
    std::unique_ptr<DX::FlyCamera>                     m_camera;

    // Timers
    std::unique_ptr<DX::GPUTimer>                      m_gpuTimer;
    std::unique_ptr<DX::CPUTimer>                      m_cpuTimer;

    // Help
    std::unique_ptr<ATG::Help>                         m_help;
    bool                                               m_showHelp;

    // Selection variables
    ResourceType                                       m_resourceType;
};
