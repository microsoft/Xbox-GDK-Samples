//--------------------------------------------------------------------------------------
// AMD_FSR.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include <array>
#include <chrono>

// Only the lightweight, platform-agnostic ffx::Context handle type and the loader function table
// (ffxFunctions) are needed here; the heavier platform-specific API/upscaler headers (and the
// actual FFX calls) live in the .cpp. ffx_api_loader.h casts GetProcAddress() results C-style,
// which trips the unsafe function-pointer cast warning (C4191) that pch.h enables.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4191)
#endif

#include <FidelityFX/api/include/ffx_api.hpp>
#include <FidelityFX/api/include/ffx_api_loader.h>

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

struct Scene;
class AnimationPlayer;
class PathManager;


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
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);
    void OnDisplayChange();

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void RenderScene(size_t sceneIndex, bool alphaBlend, bool shadowPass, bool depthPrepass = false,
        const DirectX::BoundingFrustum* cameraCullFrustum = nullptr,
        const DirectX::BoundingOrientedBox* lightCullBounds = nullptr,
        uint32_t* visibleInstanceCountOut = nullptr,
        uint32_t* totalInstanceCountOut = nullptr,
        bool motionVectorsPass = false,
        const DirectX::XMMATRIX* viewProjectionOverride = nullptr);
    void RenderHUD(ID3D12GraphicsCommandList* commandList);
    uint32_t GetSmokeParticleCount() const noexcept;
    void UpdateFrameTimings();
    void StartCurrentCheckpointAnimation(
        bool assignDuration = true,
        bool smoothTransition = true);

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void CreateSceneDepthViews();
    void CreateMotionVectorResources();
    void CreateFrameTimingResources();

    // FSR4 upscaler integration (upscale only, no frame generation).
    void LoadFfxLoader();
    void UnloadFfxLoader() noexcept;
    void CreateFSRResources();
    void CreateOrUpdateFsrContext(uint32_t width, uint32_t height);
    void DestroyFsrContext();
    void RenderFSR(ID3D12GraphicsCommandList* commandList);
    void RenderMagnifier(ID3D12GraphicsCommandList* commandList);
    static void FfxMsgCallback(uint32_t type, const wchar_t* message);

    // Dynamic resolution + camera jitter (FSR4 upscale pass).
    D3D12_RECT GetScaledRect(const D3D12_RECT& rect) const;
    void UpdateRenderResolutionState();
    void UpdateProjectionJitter();
    void UpdateMagnifier(const DirectX::GamePad::State& pad);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>                m_deviceResources;
    uint64_t                                            m_frame;
    DX::StepTimer                                       m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>                   m_gamePad;
    std::unique_ptr<DirectX::Keyboard>                  m_keyboard;
    std::unique_ptr<DirectX::Mouse>                     m_mouse;
    DirectX::GamePad::ButtonStateTracker                m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker             m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>            m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>            m_srvHeap;
    std::unique_ptr<DirectX::DescriptorHeap>            m_shadowDsvHeap;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_opaquePipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_transparentPipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_shadowPipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_depthPipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_skinnedOpaquePipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_skinnedTransparentPipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_skinnedShadowPipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_skinnedDepthPipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_motionVectorsPipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_skinnedMotionVectorsPipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_skyboxRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_skyboxPipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_particleRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_particlePipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_smokeParticleRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_smokeParticlePipelineState;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5>  m_commandList5;
    bool                                                m_variableShadingRateSupported = false;

    // Full-screen blit (sample FSR's upscale output, write to the real backbuffer) used instead
    // of a plain CopyResource since the upscale output and backbuffer formats can differ.
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_upscaleBlitRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_upscaleBlitPipelineState;

    // Magnifier overlay (optional debug view, ported from the AMD FidelityFX_FSR reference
    // sample): samples the final pre-magnifier color image and draws a zoomed-in circular inset
    // plus connecting border lines, so devs can visually inspect fine detail. Only used/composited
    // when m_magnifierEnabled is true (see RenderFSR()/RenderMagnifier()).
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_magnifierRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_magnifierPipelineState;

    // Dedicated RTV/DSV for the motion-vectors pass, which uses its own depth buffer (mirroring
    // AMD's FidelityFX sample pattern) rather than sharing the main scene depth buffer.
    std::unique_ptr<DirectX::DescriptorHeap>            m_motionVectorsRtvHeap;
    std::unique_ptr<DirectX::DescriptorHeap>            m_motionVectorDsvHeap;

    // RTV for the offscreen scene-color target that the forward passes render into, which FSR
    // then upscales/sharpens before it is blitted to the real backbuffer (see RenderFSR()).
    std::unique_ptr<DirectX::DescriptorHeap>            m_sceneColorRtvHeap;

    // Read-only view of the main depth buffer, used for the depth-only passes (skybox, blended,
    // particles, smoke) so the smoke pass can sample scene depth for soft-particle fading while
    // the normal depth test is still in effect.
    std::unique_ptr<DirectX::DescriptorHeap>            m_sceneDsvReadOnlyHeap;

    Microsoft::WRL::ComPtr<ID3D12Resource>              m_smokeTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_shadowMap;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_motionVectors;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_motionVectorDepth;

    // Offscreen scene-color target rendered by the forward passes, and FSR's upscale output,
    // which is blitted to the real backbuffer before the (un-upscaled) HUD pass.
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_gltfScene;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_upsampleOutput;
    D3D12_RESOURCE_STATES                               m_gltfSceneState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    D3D12_RESOURCE_STATES                               m_upsampleOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    // Intermediate display-sized target that the upscale/bypass blit writes to instead of the
    // backbuffer when the magnifier overlay is enabled, giving the magnifier pass a full final
    // color image to sample from before it composites the result into the real backbuffer.
    // Invariant: always left in RENDER_TARGET state outside of RenderFSR()/RenderMagnifier().
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_magnifierPreOutput;
    D3D12_RESOURCE_STATES                               m_magnifierPreOutputState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_whiteAlbedoTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_flatNormalTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_neutralRmaTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_irradianceTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_radianceTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_brdfLutTexture;
    Microsoft::WRL::ComPtr<ID3D12QueryHeap>             m_gpuTimestampQueryHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_gpuTimestampReadback;

    uint64_t                                            m_gpuTimestampFrequency = 0;
    bool                                                m_gpuTimestampIssued[3] = {};
    float                                               m_cpuFrameTimeMs = 0.0f;
    float                                               m_gpuFrameTimeMs = 0.0f;
    float                                               m_fsrGpuTimeMs = 0.0f;
    std::chrono::steady_clock::time_point               m_previousFrameStart = {};
    bool                                                m_hasPreviousFrameStart = false;
    float                                               m_maxRadianceMip = 0.0f;
    uint32_t                                            m_cullVisibleInstances = 0;
    uint32_t                                            m_cullTotalInstances = 0;
    D3D12_VIEWPORT                                      m_shadowViewport = {};
    D3D12_RECT                                          m_shadowScissor = {};

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_textureResources;

    std::unique_ptr<DirectX::SpriteBatch>               m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>                m_font;
    std::unique_ptr<DirectX::SpriteFont>                m_ctrlFont;

    DX::FlyCamera                                       m_camera;
    std::vector <std::unique_ptr<Scene>>                m_scenes;
    std::unique_ptr<AnimationPlayer>                    m_animationPlayer;
    std::unique_ptr<PathManager>                        m_pathManager;
    std::array<uint32_t, 4>                             m_characterAnimationIndices = {};

    // Previous-frame state used by the motion-vectors pass, snapshotted at the start of Update()
    // before this frame's camera/path/animation updates are applied.
    DirectX::XMMATRIX                                   m_viewProjectionPrev = DirectX::XMMatrixIdentity();
    DirectX::XMFLOAT4X4                                 m_characterWorldTransformPrev =
        DirectX::XMFLOAT4X4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    std::vector<DirectX::XMFLOAT4X4>                    m_previousJointPalette;

    struct TextureMemoryBin
    {
        uint64_t fileBytes = 0;
        uint64_t gpuBytes = 0;
        uint32_t textureCount = 0;
    };

    struct VariantMemory
    {
        std::wstring label;
        TextureMemoryBin albedo;
        TextureMemoryBin normal;
        TextureMemoryBin metallicRoughness;
        uint64_t modelFileBytes = 0;
        uint64_t modelGpuBytes = 0;
        uint64_t textureFileBytes = 0;
        uint64_t textureGpuBytes = 0;
    };

    std::vector<VariantMemory>   m_variantMemory;
    bool                         m_particlesEnabled = true;
    uint32_t                     m_particleCount = 0;
    std::vector<uint32_t>        m_smokeParticleDrawOrder;

    // FidelityFX API entry points, resolved at runtime from the FFX loader DLL with
    // ffxLoadFunctions() instead of being linked through an import library (see LoadFfxLoader()).
    HMODULE                      m_ffxLoaderModule = nullptr;
    ffxFunctions                 m_ffxApi{};
#ifdef _GAMING_XBOX
    // XBOX-only loader exports that switch between the full and "Lite" FSR4 Int8 models.
    using PfnFfxXboxSetUseFsr4Lite = void (*)(bool useLite);
    using PfnFfxXboxGetUseFsr4Lite = bool (*)();
    PfnFfxXboxSetUseFsr4Lite     m_ffxXboxSetUseFsr4Lite = nullptr;
    PfnFfxXboxGetUseFsr4Lite     m_ffxXboxGetUseFsr4Lite = nullptr;
#endif // _GAMING_XBOX

    // FSR4 upscaler state (upscale pass only -- no frame generation).
    ffx::Context                 m_upscalingContext{};
    bool                         m_fsrContextCreated = false;
    bool                         m_fsrHistoryResetPending = true;
    bool                         m_fsrEnabled = true;
    bool                         m_fsrSharpeningEnabled = true;
    float                        m_fsrSharpness = 0.8f;
    const char*                  m_upscalingVersionName = nullptr;
    uint64_t                     m_fsrGpuMemoryUsageBytes = 0;
    uint64_t                     m_fsrGpuAliasableMemoryUsageBytes = 0;
    bool                         m_fsrGpuMemoryUsageAvailable = false;
#ifdef _GAMING_XBOX
    // Runtime choice of upscaler on XBOX, cycled with [DPad-Left]/[T]:
    //   Fsr4Full -> full FSR4 Int8 model
    //   Fsr4Lite -> lighter "Lite" FSR4 Int8 model
    //   Fsr3     -> analytical FSR3 upscaler, selected with an FFX provider version override
    // Only takes effect once the upscaler context is torn down and recreated
    // (see m_fsrModelChangeRequested below).
    enum class UpscalerModel : uint32_t
    {
        Fsr4Full,
        Fsr4Lite,
        Fsr3,
        Count
    };
    UpscalerModel                m_upscalerModel = UpscalerModel::Fsr4Full;

    // Provider version id of the FSR3 (analytical) upscaler, looked up once per device in
    // CreateDeviceDependentResources(). 0 means the runtime doesn't offer an FSR3 provider.
    uint64_t                     m_fsr3VersionId = 0;
#endif // _GAMING_XBOX
    // True for one frame after the upscaler model toggle is pressed; CreateOrUpdateFsrContext()
    // is re-run that frame (outside of a window resize) so the backend picks up the new model.
    bool                         m_fsrModelChangeRequested = false;

    // Magnifier overlay (debug view, ported from the AMD FidelityFX_FSR reference sample).
    // Toggled with [RThumb]-Click; moved with the right stick, mirroring the reference sample.
    bool                         m_magnifierEnabled = false;
    struct MagnifierConstants
    {
        uint32_t    uImageWidth;
        uint32_t    uImageHeight;
        int32_t     iMousePos[2];

        float       fBorderColorRGB[4];     // Linear RGBA

        float       fMagnificationAmount;   // [1-...]
        float       fMagnifierScreenRadius; // [0-1]
        int32_t     iMagnifierOffset[2];    // in pixels
    };
    MagnifierConstants           m_magnifierConstants = {};

    // Render resolution is target resolution divided by the quality-mode ratio. Resources
    // stay target-sized while quality modes change; selecting another target recreates them.
    static constexpr uint32_t       c_renderScaleCount = 5;
    static constexpr float          c_renderScaleRatios[c_renderScaleCount] = { 1.0f, 1.5f, 1.7f, 2.0f, 3.0f };
    static constexpr const wchar_t* c_renderScaleNames[c_renderScaleCount] = { L"Native", L"Quality", L"Balanced", L"Performance", L"Ultra Performance" };

    uint32_t       m_renderScaleIndex = 0;
    uint32_t       m_pendingRenderScaleIndex = 0;
    bool           m_renderScaleWantsChange = false;

    // True for exactly one frame right after the render scale changes; skips the jitter advance
    // that frame (so the resolution change isn't compounded with a jitter pop) and forces the FSR
    // dispatch to reset its temporal history.
    bool           m_renderScaleChanged = false;
    uint32_t       m_renderWidth = 0;
    uint32_t       m_renderHeight = 0;

#ifdef _GAMING_XBOX
    static constexpr uint32_t c_targetResolutionCount = 4;
    static constexpr int c_targetResolutionHeights[c_targetResolutionCount] = { 1080, 1440, 1800, 2160 };
    uint32_t       m_targetResolutionIndex = 1;
    uint32_t       m_pendingTargetResolutionIndex = 1;
    bool           m_targetResolutionWantsChange = false;
#endif

    // Camera jitter (queried from the FSR context each frame). Applied to the projection matrix
    // used by every forward/opaque/skybox/particle pass, but never to the Motion Vectors
    // pass, which must see unjittered current/previous view-projection matrices.
    uint32_t              m_jitterIndex = 0;
    DirectX::XMFLOAT2      m_jitterOffset = {};
    DirectX::XMFLOAT2      m_jitterOffsetNdc = {};
};
