//--------------------------------------------------------------------------------------
// VariableShading.h
//
// Modifications Copyright (C) 2020. Advanced Micro Devices, Inc. All Rights Reserved.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef __clang__
#pragma clang diagnostic ignored "-Wnonportable-include-path"
#endif

#include "DeviceResources.h"
#include "StepTimer.h"
#include "MSAAHelper.h"
#include "RenderTexture.h"
#include "FullScreenQuad\FullScreenQuad.h"
#include "GLTF/GltfResources.h"
#include "GLTF/GltfPbrPass.h"
#include "GLTF/GltfDepthPass.h"
#include "GLTF/GltfMotionVectorsPass.h"

#include <FidelityFX/host/ffx_vrs.h>

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // IDeviceNotify
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;

    // Messages
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    void GetDefaultSize(int& width, int& height) const noexcept;

    // thread-safe constant buffer allocator to pass to FidelityFX backend contexts for allocations
    static FfxConstantAllocation ffxAllocateConstantBuffer(void* data, FfxUInt64 dataSize);
private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void Clear();
    void RenderUI(ID3D12GraphicsCommandList* commandList);
    void RenderShadingRateImageOverlay(ID3D12GraphicsCommandList* commandList);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void UpdateVRSContext(bool enabled);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptors;
    std::unique_ptr<DirectX::DescriptorHeap>        m_renderDescriptors;
    std::unique_ptr<DirectX::DescriptorHeap>        m_depthDescriptors;
    std::unique_ptr<DirectX::CommonStates>          m_states;

    enum Descriptors
    {
        SceneTex,
        SceneVelocity,
        Font,
        CtrlFont,
        ColorCtrlFont,
        VariableShadingImg_OutputUAV,
        VariableShadingImgOverlay_InputSRV,
        MotionDepthSRV,
        MotionVectorsSRV,
        Count
    };

    enum RTDescriptors
    {
        SceneRTV,
        MotionVectorsRTV,
        RTCount
    };

    enum DSDescriptors
    {
        ShadowAtlasDV,
        MotionVectorDV,
        DSCount
    };


    // Controls
    float                                           m_pitch;
    float                                           m_yaw;

    DirectX::SimpleMath::Matrix                     m_proj;
    DirectX::SimpleMath::Matrix                     m_view;
    DirectX::SimpleMath::Matrix                     m_prevView;

    // Scene rendering
    std::unique_ptr<DirectX::EffectFactory>         m_fxFactory;
    DirectX::Model::EffectCollection                m_modelEffect;

    // Render Target resources
    std::unique_ptr<DX::RenderTexture>              m_scene;

    // UI rendering
    std::unique_ptr<DirectX::SpriteBatch>           m_batch;
    std::unique_ptr<DirectX::SpriteFont>            m_font;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
    std::unique_ptr<DirectX::SpriteFont>            m_colorCtrlFont;

    // Post-processing
    std::unique_ptr<DX::FullScreenQuad>             m_fullScreenQuad;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_passthroughPSO;

    std::unique_ptr<DX::GPUTimer>                   m_gpuTimer;

#pragma region Variable Rate Shading: Image Generation
    // Variable Shading Image Generation
    enum class VariableShadingImageGenMode : unsigned int
    {
        Enable = 0,
        Enable_Wave32,
        Count,
        Disable
    };

    enum class VariableShadingImageGenTileSize : unsigned int
    {
        TileSize8 = 0,
        TileSize16,
        TileSize32,
        TileSize_Count,
        TileSize_Disable
    };

    constexpr unsigned int VariableShadingImageGenTileSizeToTileSizeValue(VariableShadingImageGenTileSize imgGenTileSize)
    {
        return static_cast<unsigned int>(8) << static_cast<unsigned int>(imgGenTileSize);
    }

    enum class VariableShadingImageGenAdditionalSizesSupported : unsigned int
    {
        Disable,
        Enable,
        Count
    };

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_variableShadingPSOs[(unsigned int)VariableShadingImageGenMode::Count][(unsigned int)VariableShadingImageGenTileSize::TileSize_Count][(unsigned int)VariableShadingImageGenAdditionalSizesSupported::Count];
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_computeRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_shadingRateImagePSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_shadingRateImageOverlayRootSignature;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_ShadingRateImage;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_motionVectors;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_motionVectorDepth;
    
    VariableShadingImageGenMode                     m_variableShadingImgGenMode;
    VariableShadingImageGenTileSize                 m_variableShadingImgGenTileSize;
    VariableShadingImageGenAdditionalSizesSupported m_variableShadingImgGenAdditionalSizesSupported;

    FfxVrsContext                                   m_vrsContext;
    FfxVrsContextDescription                        m_vrsInitParams;
    float                                           m_vrsMotionFactor;
    float                                           m_vrsThreshold;
   
    unsigned int DeviceDefaultShadingRateImageTileSize() const;
    bool DeviceSupportsAdditionalShadingRates() const;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>& GetVariableShadingPipelineState(VariableShadingImageGenMode imgGenMode, VariableShadingImageGenTileSize imgGenTileSize, VariableShadingImageGenAdditionalSizesSupported imgGenAdditionalSizes)
    {
        return m_variableShadingPSOs[(unsigned int)imgGenMode][(unsigned int)imgGenTileSize][(unsigned int)imgGenAdditionalSizes];
    }
    void GenerateShadingRateImage(ID3D12GraphicsCommandList* commandList);


    static const wchar_t* GetShadingRateImageGenerationModeDescription(VariableShadingImageGenMode mode)
    {
        switch (mode)
        {
        case VariableShadingImageGenMode::Enable:
            return L"Enable";
        case VariableShadingImageGenMode::Enable_Wave32:
            return L"Enable Wave32";
        case VariableShadingImageGenMode::Disable:
            return L"Disable";
        case VariableShadingImageGenMode::Count:
            return L"Invalid";
        default:
            return L"Unsupported";
        }
    }

    static const wchar_t* GetShadingRateImageTileSizeDescription(int tileSize)
    {
        switch (tileSize)
        {      
        case 8:
            return L"8";
        case 16:
            return L"16";
        case 32:
            return L"32";
        default:
            return L"Unsupported";
        }
    }
#pragma endregion

#pragma region Variable Rate Shading: Draw State
    // Variable Rate Shading Draw State
    bool                                            m_variableShadingOverlayEnable;
    bool                                            m_variableShadingEnable;
    D3D12_SHADING_RATE_COMBINER                     m_variableShadingCombinerState[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT];
    void EnableVRS(ID3D12GraphicsCommandList5* commandList);
    void DisableVRS(ID3D12GraphicsCommandList5* commandList);

    static const wchar_t* GetShadingRateDescription(D3D12_SHADING_RATE rate)
    {
        switch (rate)
        {
        case D3D12_SHADING_RATE_1X1:
            return L"1x1";
        case D3D12_SHADING_RATE_1X2:
            return L"1x2";
        case D3D12_SHADING_RATE_2X1:
            return L"2x1";
        case D3D12_SHADING_RATE_2X2:
            return L"2x2";
        case D3D12_SHADING_RATE_2X4:
            return L"2x4";
        case D3D12_SHADING_RATE_4X2:
            return L"4x2";
        case D3D12_SHADING_RATE_4X4:
            return L"4x4";
        default:
            return L"Unsupported";
        }
    }


    static const wchar_t* GetShadingRateCombinerDescription(D3D12_SHADING_RATE_COMBINER combiner)
    {
        switch (combiner)
        {
        case D3D12_SHADING_RATE_COMBINER_PASSTHROUGH:
            return L"Passthrough";
        case D3D12_SHADING_RATE_COMBINER_OVERRIDE:
            return L"Override";
        case D3D12_SHADING_RATE_COMBINER_MIN:
            return L"Min";
        case D3D12_SHADING_RATE_COMBINER_MAX:
            return L"Max";
        case D3D12_SHADING_RATE_COMBINER_SUM:
            return L"Sum";
        default:
            return L"Unsupported";
        }
    }
#pragma endregion

#pragma region GLTF
    AMDTK::GLTFResources m_GLTFResources;
    AMDTK::GltfPbrPass* m_gltfPBR;
    AMDTK::GltfDepthPass* m_gltfDepth;
    AMDTK::GltfMotionVectorsPass* m_gltfMotionVectors;
    AMDTK::GLTFFile* m_gltfModel;

    std::unique_ptr<DirectX::EffectFactory>         m_gltfFxFactory;
    DirectX::SimpleMath::Vector4                    m_lookAt;
    DirectX::SimpleMath::Vector4                    m_eye;
    size_t                                          m_currentCamera;
    DirectX::Model::EffectCollection                m_gltfEffect;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_shadowAtlas;
    size_t m_shadowAtlasIdx;
#pragma endregion
};
