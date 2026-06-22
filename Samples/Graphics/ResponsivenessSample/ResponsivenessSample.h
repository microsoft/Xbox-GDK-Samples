//--------------------------------------------------------------------------------------
// ResponsiveSample.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "ControllerFont.h"
#include "ReadData.h"

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
    void UpdateFrameIntervalAndBackBufferCount();
    void HiccupTheGpu();
    void WaitShader(float ms);
    bool CanRun120Hz();
    void Render();
    void RenderUI();
    HRESULT GetFrameLatencyStatistics(UINT& flip_gpu_latency_us, UINT& gpu_cpu_latency_us, UINT& total_latency_us);
    void DrawString(const DirectX::SimpleMath::Vector2& position, DirectX::FXMVECTOR color, const wchar_t* string, ...);
    void DrawString(const DirectX::SimpleMath::Vector2& position, const wchar_t* string, ...);
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
    std::unique_ptr<DirectX::GraphicsMemory>                                m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>                                m_resourceDescriptors;
    std::unique_ptr<DirectX::CommonStates>                                  m_states;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  m_batch;
    std::unique_ptr<DirectX::SpriteBatch>                                   m_sprites;
    std::unique_ptr<DirectX::SpriteFont>                                    m_font;
    std::unique_ptr<DirectX::SpriteFont>                                    m_legendFont;
    std::unique_ptr<DirectX::SpriteFont>                                    m_ctrlFont;

    Microsoft::WRL::ComPtr<ID3D12RootSignature>								m_waitRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>								m_waitPipelineState;
    D3D12XBOX_GPU_HARDWARE_CONFIGURATION                                    m_gpuHwConfig;

    Microsoft::WRL::ComPtr<ID3D12Resource>                                  m_texture1;
    Microsoft::WRL::ComPtr<ID3D12Resource>                                  m_texture2;

    Microsoft::WRL::ComPtr<ID3D12Resource>                                  m_textureE1;
    Microsoft::WRL::ComPtr<ID3D12Resource>                                  m_textureE2;
    Microsoft::WRL::ComPtr<ID3D12Resource>                                  m_textureE3;
    Microsoft::WRL::ComPtr<ID3D12Resource>                                  m_textureE4;
    Microsoft::WRL::ComPtr<ID3D12Resource>                                  m_textureE5;

    Microsoft::WRL::ComPtr<ID3D12Resource>									m_backgroundTexture;

    Microsoft::WRL::ComPtr<ID3D12Resource>                                  m_textureLine;

    bool                                                                    m_canRun120;
    bool                                                                    m_settingsChanged;
    float                                                                   m_gpuWorkload;
    unsigned int                                                            m_settingsIndex;
    unsigned int                                                            m_presetConfig;
    unsigned int                                                            m_presentInterval;
    unsigned int                                                            m_presentFlags;
    unsigned int                                                            m_backbufferCount;

    // Game State
    struct SpriteState
    {
        DirectX::SimpleMath::Vector2	position;
        DirectX::SimpleMath::Vector2	targetSpeed;
        DirectX::SimpleMath::Vector2	currentSpeed;

        float   aspectRatio = 0; //width : height
        int     spriteSize  = 0;
        void virtual Reset() {};
    };

    struct PlayerState final : SpriteState
    {
        int points;
        PlayerState() :
            points(0)
        {
            spriteSize = 200;
            aspectRatio = 1.0f;
        }

        void Reset()
        {
            points = 0;
        }
    };

    struct TargetState final : SpriteState
    {
        bool isHit;
        int id;
        int numHits;
        int explosionState;
        DirectX::SimpleMath::Vector2 acceleration;

        float amplitude;
        float phase;
        float frequency;
        float movementRadius;
        float theta;

        TargetState() :
             isHit(false),
             id(0),
             numHits(0),
             explosionState(0),
             acceleration(0.3f,0.0),
             amplitude(0),
             phase(0),
             frequency(0),
             movementRadius(1.0f),
             theta(0)
        {
            spriteSize = 600;
            aspectRatio = 0.5f;
        }

        void GenerateHits();
        void CheckCollision(PlayerState& player);
        void Update(PlayerState& player, float time, float screenWidth, float screenHeight);
        void Reset();
    };

    bool											m_playerFiring;
    bool											m_playerFiringHeld;
    std::vector<std::shared_ptr<SpriteState>>		m_spriteStates;
    float											m_targetSpeed;
    float											m_position;

    // Descriptors
    enum Descriptors
    {
        CrossHair,
        Drone,
        Background,
        Explosion1,
        Explosion2,
        Explosion3,
        Explosion4,
        Explosion5,
        SegoeFont,
        LegendFont,
        CtrlFont,
        Line,
        Count = 50
    };
};
