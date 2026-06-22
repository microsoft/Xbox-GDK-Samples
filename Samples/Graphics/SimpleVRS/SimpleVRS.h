//--------------------------------------------------------------------------------------
// SimpleVRS.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include <DirectXMath.h>
#include "PerformanceTimersXbox.h"
#include "RenderTexture.h"
#include "FullScreenQuad\FullScreenQuad.h" 

using VertexType = DirectX::VertexPositionColor;

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final 
{
public:
    friend class SharedSimplePBR;

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
    void RenderUI(ID3D12GraphicsCommandList* graphicsCmdList);

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void AllocateShadingRateImage(UINT width, UINT height);
    void InitializeSpriteFonts(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload, const DirectX::RenderTargetState& rtState);
    void InitializeCloudAlphaBuffer();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>                    m_deviceResources;

    // Input device.
    std::unique_ptr<DirectX::GamePad>                       m_gamePad;
    DirectX::GamePad::ButtonStateTracker                    m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>                m_graphicsMemory;

    // Timers
    std::unique_ptr<DX::GPUTimer>                           m_gpuTimer;
    uint64_t                                                m_frame;
    DX::StepTimer                                           m_timer;

    // App Settings
    bool                                                    m_VRSEnabled = true;
    bool                                                    m_shadingRateVisualizationEnabled = false;

    // Render Cloud Alpha to Texture Pass
    std::unique_ptr<DX::RenderTexture>                      m_cloudAlphaScene;
    std::unique_ptr<DirectX::DescriptorHeap>                m_rtvDescriptorHeap;

    // VRS Shading Rate Image Generation Pass
    static constexpr uint32_t                               c_VRSTileSize = 8;     // Tile size on Scarlett is always 8x8, according to documentation in GDK.chm
    std::unique_ptr<DirectX::DescriptorHeap>                m_SRVUAVDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>             m_VRSRS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>             m_VRSPSO;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5>      m_commandList5;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_shadingRateImage;
    UINT                                                    m_shadingRateImageSizeBytes = 0;
    uint32_t                                                m_timerCounter = 0;
    double                                                  m_gpuTimeElapsed = 0;

    // Triangle Rendering Pass
    Microsoft::WRL::ComPtr<ID3D12RootSignature>             m_triangleRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>             m_trianglePSO;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_triangleVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW                                m_triangleVertexBufferView = {};

    // Cloud Sprite Rendering Pass
    struct SpriteState
    {
        DirectX::SimpleMath::Vector2            Position;
        void Reset(float screenWidth, float screenHeight, float offset = 0);
        void Update(float time, float screenWidth, float screenHeight);
    };

    static DirectX::XMUINT2                                 c_cloudTextureSize;
    static constexpr uint32_t                               c_spriteScaleFactor = 2;
    static constexpr size_t                                 c_spriteWaves = 4;
    static constexpr size_t                                 c_cloudsPerWave = 10;
    static constexpr uint32_t                               c_spriteSpacing = 20;
    static constexpr float                                  c_maxSpeed = 750.f;
    static DirectX::SimpleMath::Vector2                     c_spriteSpeed;
    std::unique_ptr<DirectX::SpriteBatch>                   m_alphaCloudSprites;
    std::unique_ptr<DirectX::SpriteBatch>                   m_cloudSprites;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_cloudTexture;
    std::vector<std::shared_ptr<SpriteState>>	            m_spriteStates;
    void DrawCloudSprites(DirectX::SpriteBatch* sprites, ID3D12GraphicsCommandList5* commandList, D3D12_VIEWPORT viewport);

    // UI Pass
    std::unique_ptr<DirectX::SpriteBatch>                   m_fontBatch;
    std::unique_ptr<DirectX::SpriteFont>                    m_fontText;
    std::unique_ptr<DirectX::SpriteFont>                    m_ctrlFont;
    std::unique_ptr<DirectX::BasicEffect>                   m_basicEffect;
    std::unique_ptr<DirectX::PrimitiveBatch<VertexType>>    m_vertexBatch;
    static float                                            c_uiScale;
    static constexpr DirectX::SimpleMath::Vector4           c_UIBackground = DirectX::SimpleMath::Vector4(0.f, 0.f, 0.f, 0.7f);
    static constexpr float                                  c_UIRectangleSceneDepth = 0.05f;
    static constexpr float                                  c_minX = -0.97f;
    static constexpr float                                  c_maxX = -0.30f;
    static constexpr float                                  c_minYGPUTiming = 0.1f;
    static constexpr float                                  c_maxYGPUTiming = 0.95f;
    static constexpr float                                  c_minYControls = -0.92f;
    static constexpr float                                  c_maxYControls = -0.81f;
    static constexpr float                                  c_minYVRSVisualization = -0.32f;
    static constexpr float                                  c_maxYVRSVisualization = 0.05f;

    // VRS Visualization Pass
    struct Constants
    {
        float shadingRateImageWidth;
        float shadingRateImageHeight;
    };

    Microsoft::WRL::ComPtr<ID3D12PipelineState>             m_VisualizeVRSPSO;
    std::unique_ptr<DX::FullScreenQuad>                     m_fullScreenQuad;
    float                                                   m_shadingRateImageWidth = 0;
    float                                                   m_shadingRateImageHeight = 0;

    // Descriptors
    enum Descriptors
    {
        CloudSRV,
        TextFont,
        ControlFont,
        CloudAlphaSRV,
        ShadingRateImageUAV,
        ShadingRateImageSRV,
        Count
    };

    enum RTDescriptors
    {
        CloudAlphaRTV,
        RTCount
    };
};
