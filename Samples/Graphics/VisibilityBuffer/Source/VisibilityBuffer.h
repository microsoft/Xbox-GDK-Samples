//--------------------------------------------------------------------------------------
// VisibilityBuffer.h
//
// A sample demonstrating the use of ResourceDescriptorHeap[] and SamplerDescriptorHeap[] in HLSL SM 6.6.
// Shows a visibility-buffer-based deferred renderer, using Dynamic Resources.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "RenderTexture.h"
#include "DescriptorHeap.h"
#include "FreeCamera.h"
#include "ControllerFont.h"
#include "ControllerHelp.h"
#include "CommonHLSL.h"
#ifdef _GAMING_XBOX
#include "PerformanceTimersXbox.h"
#else
#include "PerformanceTimers.h"
#endif

namespace RTDescriptors
{
    enum
    {
        VisibilityRTV,
        RTCount
    };
}

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

private:
    void Update(DX::StepTimer const& timer);
    void Render();
    void RenderForwardMode();
    void RenderVisibilityMode();
    void RenderHUD();

    void Clear();

    void CreateDeviceDependentResources();
    void BuildHUDObjects();
    void CreateSamplers();
    void BuildObjectBuffer();
    void LoadModels();
    void BuildPSOs();
    void LoadTextures();
    void CreateWindowSizeDependentResources();

    void LoadTexture(const wchar_t* path,
        Microsoft::WRL::ComPtr<ID3D12Resource>& texture,
        uint32_t textureIndex,
        bool compressed);
    void SplitMeshlets(const uint32_t* indexBufferData,
        uint32_t indexCount,
        std::vector<uint32_t>& outputUniqueIndices,
        std::vector<MeshletDesc>& outputMeshlets,
        std::vector<uint32_t>& outputPrimitiveIndices,
        uint32_t maxVertices, uint32_t maxPrimitives);

    enum RootParameters
    {
        RootParamCB = 0,
    };

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;
    std::unique_ptr<DX::GPUTimer>                   m_gpuTimer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    DirectX::GamePad::ButtonStateTracker            m_buttonTracker;

    bool                                            m_useVisibility;
    bool                                            m_useMeshShaders;
    OverlayModes::Value                             m_overlayMode;

    std::unique_ptr<ATG::Help>                      m_help;
    bool                                            m_showHelp;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::SpriteBatch>           m_spriteBatch;

    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_bigFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;

    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptors;
    std::unique_ptr<DirectX::DescriptorHeap>        m_samplerDescriptors;
    std::unique_ptr<DirectX::DescriptorHeap>        m_renderDescriptors;

    std::unique_ptr<DX::RenderTexture>              m_visibilityBuffer;

    // Direct3D 12 objects
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_visibilityMSPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_visibilityVSPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_rasterPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_visibilityRootSignature;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rasterRootSignature;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_texture[3];
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_objectBuffer;

    std::unique_ptr<DirectX::ModelMeshPart>         m_model[2];
    D3D12_VERTEX_BUFFER_VIEW                        m_vertexBufferView[ARRAYSIZE(m_model)];
    D3D12_INDEX_BUFFER_VIEW                         m_indexBufferView[ARRAYSIZE(m_model)];

    std::vector<uint32_t>                           m_uniqueIndicesData[ARRAYSIZE(m_model)];
    std::vector<MeshletDesc>                        m_meshletsData[ARRAYSIZE(m_model)];
    std::vector<uint32_t>                           m_primitiveIndicesData[ARRAYSIZE(m_model)];

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_uniqueIndicesBuffer[ARRAYSIZE(m_model)];
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_meshletsBuffer[ARRAYSIZE(m_model)];
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_primitiveIndicesBuffer[ARRAYSIZE(m_model)];

    static constexpr uint32_t                       s_numDragons = 50;

    ObjectInfo                                      m_objectData[s_numDragons + 1];

    FreeCamera                                      m_camera;

    // Compute
    static constexpr uint32_t                       s_numShaderThreads = 8;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_renderTexture[2];
    uint32_t                                        m_ThreadGroupX;
    uint32_t                                        m_ThreadGroupY;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_computePSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_computeRootSignature;
};
