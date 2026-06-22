//--------------------------------------------------------------------------------------
// DXRContent.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "OptionSet.h"
#include "Scene.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final 
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
    void RenderHUD();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    bool SkipContent(size_t sceneIndex) const;
    uint32_t GetCurrentSceneIndex(size_t sceneIndex) const;
    void BuildTLASFromGLTF();
    void BuildBLASesFromGLTF();
    void GetBLASPostBuildSize();

    struct TLASOption
    {
        enum
        {
            FP16BoxInflationThreshold,
            TreeletFirstLevel,
            TreeletNumLevels,
            TreeletStepSizeBetweenRounds,
            TreeletRoundsLimiter,
            NumExtraElementsForInstanceSplitting,
            InstanceSplit16Fraction,
#if _GXDK_VER >= 0x63360C57 // This option is only available in March 2024 GDK or later
            IsolateSARatio,
#endif
            BuildMode,
            Count
        };
    };

    using TLASOptions = ATG::OptionSet<TLASOption::Count>;

    struct BLASOption
    {
        enum
        {
            OfflineBuild,
            BatchedBuild,
            BuildMode,
            QuadJoinThreshold,
            FP16BoxInflationThreshold,
            TriangleSplitFactor,
            KDOPTesselationFactorN,
            Count
        };
    };

    using BLASOptions = ATG::OptionSet<BLASOption::Count>;

    struct DebugOption
    {
        enum
        {
            Mode,
            IterationScale,
            TimeScale,
            Count
        };
    };

    using DebugOptions = ATG::OptionSet<DebugOption::Count>;

public:
    struct ContentOption
    {
        enum
        {
            Ship,
            Island,
            Tree,
            Pier,
            Ocean,
            Fence,
            Shovel,
            Skybox,
            Count
        };
    };

private:
    using ContentOptions = ATG::OptionSet<ContentOption::Count>;

    enum class OptionMenu
    {
        BLAS,
        TLAS,
        Debug,
        Content,
        Count
    };

    // Device resources.
    std::unique_ptr<DX::DeviceResources>                m_deviceResources;

    // Rendering loop timer.
    uint64_t                                            m_frame;
    DX::StepTimer                                       m_timer;
    std::unique_ptr<DX::GPUTimer>                       m_gpuTimer;
    std::unique_ptr<DX::CPUTimer>                       m_cpuTimer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>                   m_gamePad;
    DirectX::GamePad::ButtonStateTracker                m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>            m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>            m_srvHeap;
    std::unique_ptr<DirectX::DescriptorHeap>            m_samplerHeap;
    std::unique_ptr<DirectX::EffectTextureFactory>      m_textureFactory;

    // D3D12 objects
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_sceneConstants;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_debugConstants;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_rtOutput;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_grayTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_heatmapTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_skyboxTexture;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_rootSignatureRT;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_pipelineStateRT;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_pipelineStateRTDebug;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_TLASBuildScratch;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_TLAS;
    UINT64                                              m_tlasSize;
    UINT64                                              m_blasTotalSize;
    UINT64                                              m_blasPostBuildSize;
    uint32_t                                            m_postbuildInfoCount;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_postbuildInfos;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_postbuildInfosCpu;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_meshInfoBuffer;

    // Camera
    DX::FlyCamera                                       m_camera;

    // HUD
    std::unique_ptr<DirectX::SpriteBatch>               m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>                m_font;
    std::unique_ptr<DirectX::SpriteFont>                m_fontBold;
    std::unique_ptr<DirectX::SpriteFont>                m_ctrlFont;

    // Options
    bool                                                m_buildTLAS;
    bool                                                m_buildBLAS;
    bool                                                m_showBuildMessage;
    bool                                                m_showBLASTimeGpu;
    bool                                                m_useGoodContent;
    bool                                                m_useDefaultTLASValues;
    bool                                                m_useDefaultBLASValues;
    TLASOptions                                         m_tlasOptions;
    BLASOptions                                         m_blasOptions;
    DebugOptions                                        m_debugOptions;
    ContentOptions                                      m_contentOptions;
    OptionMenu                                          m_currentMenu;

    // Scene
    std::vector <std::unique_ptr<Scene>>                m_scenes;
};
