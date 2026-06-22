//--------------------------------------------------------------------------------------
// DrawIndexedX.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "SampleEffect.h"
#include "PerformanceTimersXbox.h"
#include "Common.h"
#include "ControllerHelp.h"
#include "Model.h"

namespace
{
    enum SelectedModel
    {
        ModelAtlas,      // Model draw using texture atlas
        ModelIndividual, // Model draw using individual textures
        NumModels
    };
}

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

    std::unique_ptr<ATG::Help>                  m_help;

    void LoadResources(ID3D12Device* device);
    void InitializeCamera();
    void LoadMeshes(ID3D12Device * device);

    // SpriteFont
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_fontResource;
    std::unique_ptr<DirectX::SpriteBatch>       m_fontBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_fontText;
    std::unique_ptr<DirectX::SpriteFont>        m_fontController;
    void InitializeSpriteFonts(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload, const DirectX::RenderTargetState& rtState);
    void RenderUI(ID3D12GraphicsCommandList* graphicsCmdList);

    // Effect
    struct ModelEffect
    {
        std::unique_ptr<SampleEffect>                  m_effect;
        std::unique_ptr<DirectX::Model>                m_sampleModel;
        uint32_t                                       m_meshVertexStride;
        std::unique_ptr<DirectX::EffectTextureFactory> m_textures;
        D3D12_GPU_VIRTUAL_ADDRESS                      m_singleVertexBufferAddress;
        Microsoft::WRL::ComPtr<ID3D12Resource>         m_singleVertexBuffer;
        uint64_t                                       m_singleVertexBufferSize;
    };

    ModelEffect m_modelEffect[SelectedModel::NumModels];
    uint32_t    m_descrptorIncrementSizeCBVSRVUAV;
    uint32_t    m_currFrameIndex;

    // Full screen resources to display depth
    void CreateFullScreenRectResources(ID3D12Device * device, DirectX::ResourceUploadBatch& resourceUpload);
    Microsoft::WRL::ComPtr<ID3D12PipelineState>        m_fullScreenPSO;
    Microsoft::WRL::ComPtr<ID3D12Resource>             m_fullScreenVB;
    D3D12_VERTEX_BUFFER_VIEW                           m_fullScreenVBView;
    D3D12_RESOURCE_STATES                              m_dsvState;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>       m_cbvSrvUavDescriptorHeap;

    // Timers
    std::unique_ptr<DX::GPUTimer>                      m_gpuTimer;
    std::unique_ptr<DX::CPUTimer>                      m_cpuTimer;

    // Buffer
    void CreateBuffers(ID3D12Device* device);
    Microsoft::WRL::ComPtr<ID3D12Resource>             m_bufferWaves;
    D3D12_CPU_DESCRIPTOR_HANDLE                        m_bufferWavesDescriptorCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE                        m_bufferWavesDescriptorGPU;
    uint32_t*                                          m_bufferWavesMapped;

    // Variable to change on-screen options
    bool                                               m_showDebugData;
    bool                                               m_renderDepthBuffer;
    bool                                               m_drawUsingDrawIndexedX;
    bool                                               m_showHelp;
    SelectedModel                                      m_selectedModel;
    size_t                                             m_textureDescriptorHeapEntry;
    bool                                               m_isXboxOneX;

    // Storage for new index buffer for DrawIndexedX
    // This is required just to run both normal draws and DrawIndexedX at the same time
    DXGI_FORMAT                     m_indexFormat[MAX_DRAWS];
    DirectX::SharedGraphicsResource m_indexBuffer[MAX_DRAWS];
    uint64_t                        m_indexBufferSize[MAX_DRAWS];
    uint32_t                        m_totalVertexOffset[MAX_DRAWS];
};
