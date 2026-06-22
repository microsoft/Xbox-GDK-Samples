//--------------------------------------------------------------------------------------
// BVHIntrinsic.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "FreeCamera.h"

namespace SRVUAVDescriptors
{
    enum Descriptor
    {
        OutputUAV,

        SkyRadianceResource,
        SkyIrradianceResource,

        FontSRVSmall,
        FontSRVBig,
        ControllerFontSRV,

        EnumCount
    };
};

struct RenderMode
{
    enum Enum
    {
        Normal,
        Depth,
        TraversalCost,
        PrimitiveIndex,
        EnumCount
    };
};

struct ShaderConstants
{
    DirectX::XMMATRIX screenToCameraSpace;
    DirectX::XMVECTOR cameraPosition;

    uint64_t bvhAddress;
    uint32_t bvhSize;
    uint32_t colour;

    uint32_t firstLeafNodeIndex;
    uint32_t renderMode;
    float elapsedTime;
    uint32_t _padding[1];
};

struct Scenes
{
    enum Enum
    {
        Triangles,
        Voxels,
        EnumCount
    };
};



// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample() = default;

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

    void LoadBuffer(const wchar_t* file, Microsoft::WRL::ComPtr<ID3D12Resource>& buffer);
    void LoadCompressedBuffer(const wchar_t* file, Microsoft::WRL::ComPtr<ID3D12Resource>& buffer);
    void LoadSky(const wchar_t* radianceTex, const wchar_t* irradianceTex);

    void PrepareToRenderVoxels(ID3D12GraphicsCommandList* cl, ShaderConstants& constants);
    void PrepareToRenderTriangles(ID3D12GraphicsCommandList* cl, ShaderConstants& constants);

    void RenderHUD(ID3D12GraphicsCommandList* cl);

    // Constants
    static constexpr uint32_t NUM_LODS = 7;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;
    
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_outputTex;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_trianglesBVH[NUM_LODS];
    uint32_t m_trianglesPerLOD[NUM_LODS];

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_voxelsBVH, m_voxelsBlocks;
    uint32_t m_numVoxels;
    uint32_t m_firstLeafNodeIndex;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_statsBufferDefault, m_statsBufferReadback;

    // Root Signatures
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_voxelsRootSig, m_trianglesRootSig;

    // PSOs
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_voxelsEmulatedPSO, m_voxelsNativePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_trianglesEmulatedPSO, m_trianglesNativePSO;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_radianceResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_irradianceResource;
    
    std::unique_ptr<DirectX::DescriptorHeap> m_csuHeap;
    uint32_t m_csuDescriptorSize;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    FreeCamera m_cameras[Scenes::EnumCount];

    // Configurables
    uint32_t m_renderMode;
    uint32_t m_currentSceneIndex;
    uint32_t m_currentColour;
    uint32_t m_currentResolution;
    uint32_t m_numResolutions;
    uint32_t m_currentLOD;
    bool m_emulatedIntrinsic;
    bool m_hideHUD;

    DX::GPUTimer m_gpuTimer;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // HUD
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_bigFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
};
