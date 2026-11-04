//--------------------------------------------------------------------------------------
// AMD_FSR.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AMD_FSR.h"

#include "AnimationPlayer.h"
#include "PathManager.h"
#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h"
#include "gltf/gltf.h"
#include "gltf/GltfProcessing.h"
#include "ReadData.h"
#include "Scene.h"

#include <filesystem>
#include <unordered_set>

// FSR4 upscaler (upscale pass only -- no frame generation). Platform-specific API header first,
// then the upscaler effect header, matching the include order required by the FFX API headers.
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wswitch"
#elif defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable : 4061 4062)
#endif

#ifdef _GAMING_XBOX
#include <FidelityFX/api/include/xbox/ffx_api_xbox.hpp>
#else
#include <FidelityFX/api/include/dx12/ffx_api_dx12.hpp>
#endif
#include <FidelityFX/upscalers/include/ffx_upscale.hpp>

#if defined(__clang__)
    #pragma clang diagnostic pop
#elif defined(_MSC_VER)
    #pragma warning(pop)
#endif

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr float c_environmentIntensity = 0.3f;

    // The FFX loader DLL is loaded at runtime (see Sample::LoadFfxLoader()) rather than linked
    // through its import library. The ffx::CreateContext()/Query()/Dispatch()/DestroyContext()
    // C++ wrappers call the loader's exports directly, so these helpers mirror them but call
    // through the ffxFunctions table filled in by ffxLoadFunctions(). As with the wrappers, any
    // number of descs can be passed; ffx::LinkHeaders() chains their headers via pNext.
    template<class... Desc>
    ffx::ReturnCode FfxCreateContext(const ffxFunctions& api, ffx::Context& context, Desc&... desc)
    {
        return static_cast<ffx::ReturnCode>(api.CreateContext(&context, ffx::LinkHeaders(desc.header...), nullptr));
    }

    ffx::ReturnCode FfxDestroyContext(const ffxFunctions& api, ffx::Context& context)
    {
        return static_cast<ffx::ReturnCode>(api.DestroyContext(&context, nullptr));
    }

    // Queries an existing context (e.g. jitter, provider version, memory usage).
    template<class... Desc>
    ffx::ReturnCode FfxQuery(const ffxFunctions& api, ffx::Context& context, Desc&... desc)
    {
        return static_cast<ffx::ReturnCode>(api.Query(&context, ffx::LinkHeaders(desc.header...)));
    }

    // Context-less query, answered by the loader itself (e.g. which providers are available).
    template<class... Desc>
    ffx::ReturnCode FfxQueryGlobal(const ffxFunctions& api, Desc&... desc)
    {
        return static_cast<ffx::ReturnCode>(api.Query(nullptr, ffx::LinkHeaders(desc.header...)));
    }

    template<class... Desc>
    ffx::ReturnCode FfxDispatch(const ffxFunctions& api, ffx::Context& context, Desc&... desc)
    {
        return static_cast<ffx::ReturnCode>(api.Dispatch(&context, ffx::LinkHeaders(desc.header...)));
    }

#ifdef _GAMING_XBOX
    // GetProcAddress() returns a generic FARPROC. Casting it directly to another function-pointer
    // type triggers C4191 (unsafe conversion), so go through void*; the real signature of each
    // export is documented by the FFX SDK.
    template<class Pfn>
    Pfn GetFfxLoaderExport(HMODULE module, const char* name)
    {
        return reinterpret_cast<Pfn>(reinterpret_cast<void*>(GetProcAddress(module, name)));
    }

    // Asks the FFX loader which upscaler providers it can create on this device and returns the
    // version id of the FSR3 (analytical) provider, or 0 if none is offered. The returned id is
    // passed back to FfxCreateContext() through CreateContextDescOverrideVersion to force that
    // provider instead of the default (newest) FSR4 one.
    uint64_t FindFsr3UpscalerVersionId(const ffxFunctions& api, ID3D12Device* device)
    {
        ffx::QueryDescGetVersions versionQuery{};
        versionQuery.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
        versionQuery.device = device;

        // First call with null output arrays returns just the number of available providers.
        uint64_t versionCount = 0;
        versionQuery.outputCount = &versionCount;
        if (FfxQueryGlobal(api, versionQuery) != ffx::ReturnCode::Ok || versionCount == 0)
        {
            return 0;
        }

        // Second call fills in the ids and display names (e.g. "4.1.2", "3.1.6").
        std::vector<uint64_t> versionIds(static_cast<size_t>(versionCount));
        std::vector<const char*> versionNames(static_cast<size_t>(versionCount));
        versionQuery.versionIds = versionIds.data();
        versionQuery.versionNames = versionNames.data();
        if (FfxQueryGlobal(api, versionQuery) != ffx::ReturnCode::Ok)
        {
            return 0;
        }

        // Providers are identified by their display version: FSR3 upscaler versions are "3.x.y".
        for (size_t i = 0; i < static_cast<size_t>(versionCount); ++i)
        {
            if (versionNames[i] != nullptr && versionNames[i][0] == '3')
            {
                return versionIds[i];
            }
        }
        return 0;
    }
#endif // _GAMING_XBOX

    // Asset paths
    const wchar_t* c_folderPaths[] =
    {
        L"Media\\GLTF\\SponzaNew",
        L"Media\\GLTF\\RampagingTRex",
        0
    };

    const wchar_t* c_environmentFolderPaths[] =
    {
        L"Media\\CubeMaps\\HDR-Image based lighting",
        L"Media\\HDR",
        0
    };

    // Textures are standard BC-compressed DDS files shared by all platforms.
    const wchar_t* c_particleFolderPaths[] =
    {
        L"Media\\Textures\\Particles",
        0
    };

    const wchar_t* c_sponzaTextureFolderPaths[] =
    {
        L"Media\\Textures\\SponzaNew",
        0
    };

    const wchar_t* c_characterTextureFolderPaths[] =
    {
        L"Media\\Textures\\RampagingTRex",
        0
    };

    const wchar_t* c_gltfPaths[] =
    {
        L"MainSponza.gltf",
        L"RampagingTRex.glb",
    };

    const wchar_t* c_variantNames[] =
    {
        L"SponzaNew",
        L"Rampaging T-Rex",
    };

    // Scene/asset index 1 is always the animated character; any other scene index is
    // treated as static world geometry.
    constexpr size_t c_characterSceneIndex = 1;

    const char* c_characterAnimationNames[] =
    {
        "idle",
        "bite",
        "run",
        "attack_tail",
    };
    constexpr std::array<const wchar_t*, 4> c_characterTextureFiles =
    {
        L"TRex_BaseColor.dds",
        L"TRex_MetallicRoughness.dds",
        L"TRex_Normal.dds",
        L"TRex_Occlusion.dds",
    };
    constexpr uint32_t c_walkingAnimationIndex = 2;
    constexpr wchar_t c_characterPathFile[] = L"CharacterPath.json";
    constexpr bool c_smoothCharacterAnimationTransitions = true;
    constexpr float c_characterAnimationTransitionDuration = 0.25f;

    constexpr uint32_t c_shadowMapSize = 2048;

    // Physical near/far distances for the main camera's reverse-Z projection (see
    // SetProjectionParameters below, which intentionally swaps these for the projection matrix).
    constexpr float c_cameraNearDistance = 1.0f;
    constexpr float c_cameraFarDistance = 10000.0f;

    // Floating particles parameters
    constexpr uint32_t c_particleMaximumCount = 1024;
    constexpr uint32_t c_particleMinimumCount = c_particleMaximumCount / 8;
    constexpr uint32_t c_particleDefaultCount = c_particleMinimumCount * 2;
    static_assert(c_particleMaximumCount % 8 == 0);
    constexpr float c_particleMinimumSpeed = 0.01f;
    constexpr float c_particleMaximumSpeed = 1.50f;
    constexpr float c_particleLifetime = 14.0f;
    const XMFLOAT3 c_particleVolumeCenter = { 12.0f, 18.0f, 4.0f };
    const XMFLOAT3 c_particleVolumeExtents = { 75.0f, 20.0f, 35.0f };

    // Smoke billboards spawn from a plane roughly matching the Sponza courtyard footprint
    // (reusing the same X/Z center as the ember swarm above, at floor height). They rise
    // quickly to a shallow ground-hugging layer height, hold there, then fade out, so with
    // enough particles spawning uniformly they read as a continuous low layer of ground fog.
    constexpr uint32_t c_smokeDefaultParticleCount = 220;
    constexpr float c_smokeParticleCountScale = 1.3f;
    constexpr float c_smokeLifetime = 12.0f;
    constexpr float c_smokeRiseFraction = 0.12f;   // fraction of lifetime spent rising to settle height
    constexpr float c_smokeMinimumSettleHeight = 0.6f;
    constexpr float c_smokeMaximumSettleHeight = 1.8f;
    constexpr float c_smokeMinimumSize = 4.0f;
    constexpr float c_smokeMaximumSize = 8.0f;

    // World-space distance over which a smoke billboard fades out as it nears intersecting scene
    // geometry (soft particles), used to hide the hard line where smoke meets the floor.
    constexpr float c_smokeDepthFadeDistance = 1.5f;
    constexpr float c_smokeDriftAmplitude = 0.5f;
    constexpr uint32_t c_smokeSpriteSheetColumns = 10;
    constexpr uint32_t c_smokeSpriteSheetRows = 6;
    constexpr float c_smokeSpriteSheetFrameSizePixels = 128.0f;
    const XMFLOAT3 c_smokeEmitterCenter = { 12.0f, 0.1f, 4.0f };
    const XMFLOAT2 c_smokeEmitterExtents = { 70.0f, 32.0f };

    // Parameters related to animated model
    constexpr float c_animatedModelScale = 3.0f;
    constexpr float c_animatedModelSpeed = 25.0f;
    constexpr float c_animatedModelAcceleration = 500.0f;
    constexpr float c_animatedModelHeadingOffset = XM_PI;

    // Scene light parameters
    const XMVECTORF32 c_lightDirection = { -0.25f, -1.0f, 0.2f, 0.0f };
    const XMVECTORF32 c_lightColor = { 4.0f, 4.0f, 4.0f, 1.0f };

    struct DescriptorIndex
    {
        enum : uint32_t
        {
            Font,
            CtrlFont,
            ShadowMap,
            Irradiance,
            Radiance,
            BrdfLut,
            SceneDepth,
            MotionVectors,
            MotionVectorDepth,
            UpsampleOutput,
            GltfSceneColor,
            MagnifierPreOutput,
            WhiteAlbedo,
            FlatNormal,
            NeutralRma,
            SmokeTexture,
            Count
        };
    };

    enum RootParameter : uint32_t
    {
        RootParameterSceneConstants,
        RootParameterMeshConstants,
        RootParameterJointConstants,
        RootParameterPreviousJointConstants,
        RootParameterAlbedo,
        RootParameterNormal,
        RootParameterMetallicRoughness,
        RootParameterShadow,
        RootParameterIrradiance,
        RootParameterRadiance,
        RootParameterBrdfLut,
    };

    struct alignas(16) SceneConstants
    {
        XMFLOAT4X4 viewProjection;
        XMFLOAT4X4 lightViewProjection;
        XMFLOAT4 eyePosition;
        XMFLOAT4 lightDirection;
        XMFLOAT4 lightColor;
        XMFLOAT4 environmentParameters;
        XMFLOAT4X4 previousViewProjection;
    };

    static_assert((sizeof(SceneConstants) % 16) == 0);

    struct alignas(16) MeshConstants
    {
        XMFLOAT4X4 world;
        XMFLOAT4X4 worldInverseTranspose;
        XMFLOAT4 baseColorFactor;
        XMFLOAT4 materialFactors;
        XMFLOAT4X4 previousWorld;
    };

    static_assert((sizeof(MeshConstants) % 16) == 0);

    struct alignas(16) JointConstants
    {
        XMFLOAT4X4 joints[AnimationPlayer::MaxJoints];
    };

    static_assert((sizeof(JointConstants) % 16) == 0);

    struct alignas(16) SkyboxConstants
    {
        XMFLOAT4X4 inverseViewProjection;
        XMFLOAT4 eyePosition;
        XMFLOAT4 environmentParameters;
    };

    static_assert((sizeof(SkyboxConstants) % 16) == 0);

    struct alignas(16) ParticleConstants
    {
        XMFLOAT4X4 viewProjection;
        XMFLOAT4 cameraRightAndSize;
        XMFLOAT4 cameraUpAndTime;
        XMFLOAT4 volumeCenterAndLifetime;
        XMFLOAT4 volumeExtents;
        XMFLOAT4 speedRange;
    };

    static_assert((sizeof(ParticleConstants) % 16) == 0);

    struct alignas(16) SmokeParticleConstants
    {
        XMFLOAT4X4 viewProjection;
        XMFLOAT4 cameraRightAndSizeScale;
        XMFLOAT4 cameraUpAndTime;
        XMFLOAT4 emitterCenterAndLifetime;
        XMFLOAT4 emitterExtentsAndDrift;
        XMFLOAT4 settleHeightRange;         // z = depth-fade distance (world units), w = unused
        XMFLOAT4 sizeRange;
        XMFLOAT4 cameraForwardAndNear;      // xyz = camera forward (world space), w = camera near distance
        XMFLOAT4 eyePositionAndFar;         // xyz = camera world position, w = camera far distance
        XMFLOAT4 spriteSheetGrid;           // x = columns, y = rows, z = frame count, w = per-cell UV inset (avoids bilinear bleed across frames)
    };

    static_assert((sizeof(SmokeParticleConstants) % 16) == 0);

    // Mirrors the hash used in SmokeParticleVertexShader.hlsl, so the CPU can estimate each
    // smoke particle's approximate world position for back-to-front draw-order sorting.
    uint32_t SmokeParticleHash(uint32_t value)
    {
        value ^= value >> 16;
        value *= 0x7feb352du;
        value ^= value >> 15;
        value *= 0x846ca68bu;
        value ^= value >> 16;
        return value;
    }

    float SmokeParticleHashFloat(uint32_t value)
    {
        return static_cast<float>(SmokeParticleHash(value) & 0x00ffffffu) / 16777216.0f;
    }

    XMFLOAT3 SmokeParticleHashFloat3(uint32_t value)
    {
        return XMFLOAT3(
            SmokeParticleHashFloat(value),
            SmokeParticleHashFloat(value ^ 0x68bc21ebu),
            SmokeParticleHashFloat(value ^ 0x02e5be93u));
    }

    const D3D12_INPUT_ELEMENT_DESC c_pbrInputElements[] =
    {
        { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    const D3D12_INPUT_LAYOUT_DESC c_pbrInputLayout =
        { c_pbrInputElements, _countof(c_pbrInputElements) };

    const D3D12_INPUT_ELEMENT_DESC c_skinnedPbrInputElements[] =
    {
        { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R16G16B16A16_UINT, 0, 36,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT", 0, DXGI_FORMAT_R16G16B16A16_UNORM, 0, 44,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    const D3D12_INPUT_LAYOUT_DESC c_skinnedPbrInputLayout =
        { c_skinnedPbrInputElements, _countof(c_skinnedPbrInputElements) };

    constexpr float c_lightOrthoWidth = 620.0f;
    constexpr float c_lightOrthoHeight = 400.0f;
    constexpr float c_lightNearZ = 1.0f;
    constexpr float c_lightFarZ = 650.0f;

    XMMATRIX GetLightView()
    {
        const XMVECTOR direction = XMVector3Normalize(c_lightDirection);
        const XMVECTOR target = XMVectorSet(30.0f, 120.0f, -10.0f, 1.0f);
        const XMVECTOR position = XMVectorSubtract(target, XMVectorScale(direction, 300.0f));
        return XMMatrixLookAtLH(position, target, XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));
    }

    XMMATRIX GetLightViewProjection()
    {
        const XMMATRIX view = GetLightView();
        const XMMATRIX projection = XMMatrixOrthographicLH(c_lightOrthoWidth, c_lightOrthoHeight, c_lightNearZ, c_lightFarZ);
        return view * projection;
    }

    // Builds a world-space oriented box tightly matching the directional light's orthographic
    // shadow frustum, used to cull scene instances from the shadow pass.
    DirectX::BoundingOrientedBox GetLightCullBounds()
    {
        const XMMATRIX lightView = GetLightView();
        const XMMATRIX lightWorld = XMMatrixInverse(nullptr, lightView);

        const float halfWidth = c_lightOrthoWidth * 0.5f;
        const float halfHeight = c_lightOrthoHeight * 0.5f;
        const XMFLOAT3 viewSpaceCorners[8] =
        {
            { -halfWidth, -halfHeight, c_lightNearZ },
            {  halfWidth, -halfHeight, c_lightNearZ },
            { -halfWidth,  halfHeight, c_lightNearZ },
            {  halfWidth,  halfHeight, c_lightNearZ },
            { -halfWidth, -halfHeight, c_lightFarZ },
            {  halfWidth, -halfHeight, c_lightFarZ },
            { -halfWidth,  halfHeight, c_lightFarZ },
            {  halfWidth,  halfHeight, c_lightFarZ },
        };

        XMFLOAT3 worldCorners[8];
        for (size_t i = 0; i < 8; ++i)
        {
            const XMVECTOR corner = XMVector3TransformCoord(XMLoadFloat3(&viewSpaceCorners[i]), lightWorld);
            XMStoreFloat3(&worldCorners[i], corner);
        }

        DirectX::BoundingOrientedBox lightBounds;
        DirectX::BoundingOrientedBox::CreateFromPoints(lightBounds, 8, worldCorners, sizeof(XMFLOAT3));
        return lightBounds;
    }

    // Builds a world-space camera frustum from the active view/projection matrices, used to cull
    // scene instances from the depth prepass, opaque, and transparent forward passes.
    DirectX::BoundingFrustum GetCameraCullFrustum(FXMMATRIX view, CXMMATRIX projection)
    {
        DirectX::BoundingFrustum viewSpaceFrustum(projection);

        // This sample's camera uses a reverse-Z projection (near/far swapped when building the
        // perspective matrix for better depth precision). BoundingFrustum's constructor assumes a
        // conventional projection and derives Near/Far directly from it, so with reverse-Z it ends
        // up with Near > Far (an inverted, empty Z range) unless swapped back here.
        if (viewSpaceFrustum.Near > viewSpaceFrustum.Far)
        {
            std::swap(viewSpaceFrustum.Near, viewSpaceFrustum.Far);
        }

        DirectX::BoundingFrustum worldSpaceFrustum;
        const XMMATRIX cameraWorld = XMMatrixInverse(nullptr, view);
        viewSpaceFrustum.Transform(worldSpaceFrustum, cameraWorld);
        return worldSpaceFrustum;
    }

    uint64_t GetResourceSize(ID3D12Device* device, ID3D12Resource* resource)
    {
        const auto desc = resource->GetDesc();
        return device->GetResourceAllocationInfo(0, 1, &desc).SizeInBytes;
    }

    uint64_t GetFileSize(const wchar_t* path)
    {
        std::error_code error;
        const auto size = std::filesystem::file_size(path, error);
        return error ? 0 : size;
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    m_particleCount = c_particleDefaultCount;
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_D32_FLOAT,
        3,
        DX::DeviceResources::c_ReverseDepth);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    // The upscaler context lives in the loader DLL, so destroy it before unloading the DLL.
    DestroyFsrContext();
    UnloadFfxLoader();
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    wchar_t characterPath[MAX_PATH] = {};
    DX::FindMediaFile(characterPath, MAX_PATH, c_characterPathFile);

    PathManager::Settings pathSettings = {};
    pathSettings.scale = c_animatedModelScale;
    pathSettings.speed = c_animatedModelSpeed;
    pathSettings.acceleration = c_animatedModelAcceleration;
    pathSettings.headingOffset = c_animatedModelHeadingOffset;
    m_pathManager = std::make_unique<PathManager>();
    DX::ThrowIfFalse(
        m_pathManager->Initialize(characterPath, std::move(pathSettings)),
        "Animated model path settings are invalid");

    m_gamePad = std::make_unique<GamePad>();

    // Resolve the FidelityFX API from the FFX loader DLL before any FFX call is made.
    LoadFfxLoader();

#ifdef _GAMING_XBOX
    // Seed our UI/toggle state from the upscaler DLL's current active model so the HUD and the
    // actual active model agree before the user presses the model toggle for the first time.
    m_upscalerModel = m_ffxXboxGetUseFsr4Lite() ? UpscalerModel::Fsr4Lite : UpscalerModel::Fsr4Full;
#endif // _GAMING_XBOX

    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

#ifdef _GAMING_XBOX
    m_targetResolutionIndex = XSystemGetDeviceType() == XSystemDeviceType::XboxScarlettLockhart ? 0u : 3u;
    m_pendingTargetResolutionIndex = m_targetResolutionIndex;
    height = c_targetResolutionHeights[m_targetResolutionIndex];
    width = height * 16 / 9;
    m_renderScaleIndex = 2; // Balanced on both Xbox Series X and Series S.
    m_pendingRenderScaleIndex = m_renderScaleIndex;
#endif

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    const SimpleMath::Vector3 cameraPosition(70.0f, 20.0f, 4.0f);
    const SimpleMath::Vector3 cameraDirection(-1.0f, 0.0f, 0.0f);
    m_camera.SetLookAt(cameraPosition, cameraPosition + cameraDirection * 100.0f);
    m_camera.SetSensitivity(80.0f, 80.0f, 80.0f, 0.0f);
    m_camera.SetBoundingBox(DirectX::BoundingBox(cameraPosition, SimpleMath::Vector3::Zero));
    m_camera.SetFlags(m_camera.GetFlags() | DX::FlyCamera::c_FlagsDisableSensitivityControl);
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

#ifdef _GAMING_XBOX
    // Apply pending resize requests between frames, before acquiring the next presentation
    // token. Preserve the selected quality mode when recreating resources for the new target.
    if (m_targetResolutionWantsChange)
    {
        m_targetResolutionIndex = m_pendingTargetResolutionIndex;
        m_targetResolutionWantsChange = false;
        const int height = c_targetResolutionHeights[m_targetResolutionIndex];
        OnWindowSizeChanged(height * 16 / 9, height);
    }
#endif

    UpdateFrameTimings();
#ifdef _GAMING_XBOX
    m_deviceResources->WaitForOrigin();
#endif

    const UINT frameIndex = m_deviceResources->GetCurrentFrameIndex();
    if (m_gpuTimestampIssued[frameIndex] && m_gpuTimestampFrequency != 0)
    {
        // Four timestamps per frame: [0] frame start, [1] FSR dispatch start, [2] FSR dispatch
        // end, [3] frame end (see Render()/RenderFSR()).
        const UINT64 readbackOffset = static_cast<UINT64>(frameIndex) * 4 * sizeof(UINT64);
        const D3D12_RANGE readRange =
        {
            static_cast<SIZE_T>(readbackOffset),
            static_cast<SIZE_T>(readbackOffset + 4 * sizeof(UINT64))
        };

        UINT64* timestamps = nullptr;
        DX::ThrowIfFailed(m_gpuTimestampReadback->Map(0, &readRange, reinterpret_cast<void**>(&timestamps)));
        timestamps += frameIndex * 4;
        if (timestamps[3] > timestamps[0])
        {
            m_gpuFrameTimeMs = static_cast<float>(
                static_cast<double>(timestamps[3] - timestamps[0]) * 1000.0
                / static_cast<double>(m_gpuTimestampFrequency));
        }
        if (timestamps[2] > timestamps[1])
        {
            m_fsrGpuTimeMs = static_cast<float>(
                static_cast<double>(timestamps[2] - timestamps[1]) * 1000.0
                / static_cast<double>(m_gpuTimestampFrequency));
        }
        const D3D12_RANGE writtenRange = { 0, 0 };
        m_gpuTimestampReadback->Unmap(0, &writtenRange);
    }

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    m_mouse->EndOfInputFrame();

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    // Snapshot last frame's camera/character transforms before this frame's input and animation
    // updates are applied below, so the motion-vectors pass can compute a per-pixel displacement
    // between last frame's and this frame's positions.
    m_viewProjectionPrev = m_camera.GetView() * m_camera.GetProjection();
    if (m_pathManager)
    {
        XMStoreFloat4x4(&m_characterWorldTransformPrev, m_pathManager->GetWorldTransform());
    }
    if (m_animationPlayer)
    {
        m_previousJointPalette = m_animationPlayer->GetJointPalette();
    }

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);
        m_camera.Update(elapsedTime, pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

    }
    else
    {
        m_gamePadButtons.Reset();
        const XMVECTOR cameraPosition = m_camera.GetPosition();
        m_camera.Update(elapsedTime, *m_mouse, *m_keyboard);
        m_camera.SetPosition(cameraPosition);
    }

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
        ExitSample();
    }

    if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED
        || m_keyboardButtons.IsKeyPressed(Keyboard::P))
    {
        m_particlesEnabled = !m_particlesEnabled;
    }

    if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED
        || m_keyboardButtons.IsKeyPressed(Keyboard::O))
    {
        m_particleCount = std::min(m_particleCount * 2, c_particleMaximumCount);
    }

    if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED
        || m_keyboardButtons.IsKeyPressed(Keyboard::I))
    {
        m_particleCount = std::max(m_particleCount / 2, c_particleMinimumCount);
    }

    // FSR dynamic resolution: cycle the render-scale ratio. The actual change
    // is applied below as a one-frame pulse (m_renderScaleChanged), mirroring the reference FSR
    // sample, so the jitter sequence and FSR's temporal history both reset cleanly on the frame
    // the resolution changes instead of mid-frame.
    if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED
        || m_keyboardButtons.IsKeyPressed(Keyboard::K))
    {
        m_pendingRenderScaleIndex = (m_renderScaleIndex + 1) % c_renderScaleCount;
        m_renderScaleWantsChange = true;
    }
    else if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED
        || m_keyboardButtons.IsKeyPressed(Keyboard::L))
    {
        m_pendingRenderScaleIndex = (m_renderScaleIndex == 0) ? (c_renderScaleCount - 1) : (m_renderScaleIndex - 1);
        m_renderScaleWantsChange = true;
    }

    if (m_renderScaleWantsChange)
    {
        m_renderScaleIndex = m_pendingRenderScaleIndex;
        m_renderScaleChanged = true;
        m_renderScaleWantsChange = false;
    }
    else
    {
        m_renderScaleChanged = false;
    }

#ifdef _GAMING_XBOX
    if (m_gamePadButtons.rightTrigger == GamePad::ButtonStateTracker::PRESSED)
    {
        m_pendingTargetResolutionIndex = (m_targetResolutionIndex + 1) % c_targetResolutionCount;
        m_targetResolutionWantsChange = true;
    }
    else if (m_gamePadButtons.leftTrigger == GamePad::ButtonStateTracker::PRESSED)
    {
        m_pendingTargetResolutionIndex = (m_targetResolutionIndex + c_targetResolutionCount - 1) % c_targetResolutionCount;
        m_targetResolutionWantsChange = true;
    }
#endif

    // FSR enable/disable: [DPad-Right] toggles between the FSR4 upscaler and presenting the
    // raw render-resolution image (so devs can compare FSR's reconstruction against the same
    // resolution scaled only by the blit's hardware bilinear texture sampler). Re-enabling forces
    // a one-frame reset so FSR doesn't resume blending against a stale temporal history built up
    // while it was off.

    const bool fsrWasEnabled = m_fsrEnabled;
    if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED
        || m_keyboardButtons.IsKeyPressed(Keyboard::Space))
    {
        m_fsrEnabled = !m_fsrEnabled;
    }
    if (m_fsrEnabled && !fsrWasEnabled)
    {
        m_renderScaleChanged = true;
    }

#ifdef _GAMING_XBOX
    // Upscaler model: cycles FSR4 Full -> FSR4 Lite -> FSR3 -> FSR4 Full. The switch only takes
    // effect once the upscaler context is torn down and recreated: the FSR4 backend reads its
    // Full/Lite choice from an internal flag at context-creation time, and FSR3 is selected with a
    // provider version override passed to FfxCreateContext() (see CreateOrUpdateFsrContext()).
    if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED
        || m_keyboardButtons.IsKeyPressed(Keyboard::T))
    {
        const auto nextModel = (static_cast<uint32_t>(m_upscalerModel) + 1u)
            % static_cast<uint32_t>(UpscalerModel::Count);
        m_upscalerModel = static_cast<UpscalerModel>(nextModel);

        // The Lite flag only applies to the FSR4 provider; clear it for FSR3 so it is left in
        // the default (Full) state.
        m_ffxXboxSetUseFsr4Lite(m_upscalerModel == UpscalerModel::Fsr4Lite);
        m_fsrModelChangeRequested = true;
    }
#endif // _GAMING_XBOX

    if (m_fsrModelChangeRequested)
    {
        // Unlike the render-scale path (which only flips a dispatch flag read by RenderFSR()),
        // this actually tears down and recreates the FFX context's GPU resources. Make sure the
        // GPU is done with any in-flight frames that may still reference the old context first.
        m_deviceResources->WaitForGpu();

        const auto outputSize = m_deviceResources->GetOutputSize();
        const uint32_t width = std::max<uint32_t>(static_cast<uint32_t>(outputSize.right - outputSize.left), 1u);
        const uint32_t height = std::max<uint32_t>(static_cast<uint32_t>(outputSize.bottom - outputSize.top), 1u);
        CreateOrUpdateFsrContext(width, height);
        m_fsrModelChangeRequested = false;
        m_renderScaleChanged = true;
    }

    // FSR sharpening: Toggles RCAS sharpening on/off;
    if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED
        || m_keyboardButtons.IsKeyPressed(Keyboard::M))
    {
        m_fsrSharpeningEnabled = !m_fsrSharpeningEnabled;
    }

    constexpr float c_sharpnessStep = 0.05f;
    if (m_fsrSharpeningEnabled &&
        (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::PRESSED
        || m_keyboardButtons.IsKeyPressed(Keyboard::B)))
    {
        m_fsrSharpness = std::max(0.0f, m_fsrSharpness - c_sharpnessStep);
    }
    else if (m_fsrSharpeningEnabled &&
        (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::PRESSED
        || m_keyboardButtons.IsKeyPressed(Keyboard::N)))
    {
        m_fsrSharpness = std::min(1.0f, m_fsrSharpness + c_sharpnessStep);
    }

    // Magnifier overlay: Toggle Magnifying glass on/off; left stick (or arrow keys on
    // desktop) moves the magnified-area reticle. 
    if (m_gamePadButtons.leftStick == GamePad::ButtonStateTracker::PRESSED
        || m_keyboardButtons.IsKeyPressed(Keyboard::Tab))
    {
        m_magnifierEnabled = !m_magnifierEnabled;
    }
    UpdateMagnifier(pad);

    if (m_pathManager)
    {
        m_pathManager->Update(elapsedTime);
        if (m_pathManager->ConsumeMovementStarted() && m_animationPlayer)
        {
            DX::ThrowIfFalse(
                m_animationPlayer->SetAnimation(
                    m_characterAnimationIndices[c_walkingAnimationIndex],
                    c_smoothCharacterAnimationTransitions
                        ? c_characterAnimationTransitionDuration
                        : 0.0f),
                "Failed to select the character walking animation");
        }
        if (m_pathManager->NeedsCheckpointAnimation())
        {
            StartCurrentCheckpointAnimation();
        }
    }

    if (m_animationPlayer)
    {
        m_animationPlayer->Update(elapsedTime);
    }

    PIXEndEvent();
}

void Sample::StartCurrentCheckpointAnimation(
    bool assignDuration,
    bool smoothTransition)
{
    if (!m_animationPlayer || !m_pathManager)
    {
        return;
    }

    const auto& selector = m_pathManager->GetCurrentCheckpointAnimation();
    uint32_t animationIndex = UINT32_MAX;
    if (selector.isIndex)
    {
        DX::ThrowIfFalse(
            selector.index < m_characterAnimationIndices.size(),
            "Checkpoint animation index must be in the logical range 0-3");
        animationIndex = m_characterAnimationIndices.at(selector.index);
    }
    else
    {
        animationIndex = m_animationPlayer->FindAnimation(selector.name);
    }

    DX::ThrowIfFalse(animationIndex != UINT32_MAX, "Checkpoint animation was not found");
    DX::ThrowIfFalse(
        m_animationPlayer->SetAnimation(
            animationIndex,
            smoothTransition && c_smoothCharacterAnimationTransitions
                ? c_characterAnimationTransitionDuration
                : 0.0f),
        "Failed to select the checkpoint animation");
    if (assignDuration)
    {
        m_pathManager->SetCheckpointAnimationDuration(
            m_animationPlayer->GetAnimationDuration(animationIndex));
    }
}

void Sample::UpdateFrameTimings()
{
    const auto frameStart = std::chrono::steady_clock::now();
    if (m_hasPreviousFrameStart)
    {
        m_cpuFrameTimeMs = std::chrono::duration<float, std::milli>(
            frameStart - m_previousFrameStart).count();
    }
    m_previousFrameStart = frameStart;
    m_hasPreviousFrameStart = true;
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    m_deviceResources->Prepare();
    auto commandList = m_deviceResources->GetCommandList();
    const UINT frameIndex = m_deviceResources->GetCurrentFrameIndex();
    const UINT timestampIndex = frameIndex * 4;
    commandList->EndQuery(m_gpuTimestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, timestampIndex);

    Clear();

    ID3D12DescriptorHeap* heaps[] = { m_srvHeap->Heap() };
    commandList->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);

    // Precompute this frame's culling volumes once: a world-space camera frustum (reused by the
    // depth prepass, opaque/transparent forward passes, and motion vectors, which share the same
    // camera) and a world-space oriented box matching the directional light's shadow frustum.
    // Per-instance bounding boxes are tested against these to skip fully off-screen/off-frustum
    // geometry instead of submitting draw calls for it.
    const DirectX::BoundingFrustum cameraCullFrustum = GetCameraCullFrustum(m_camera.GetView(), m_camera.GetProjection());
    const DirectX::BoundingOrientedBox lightCullBounds = GetLightCullBounds();
    m_cullVisibleInstances = 0;
    m_cullTotalInstances = 0;

    // Recompute this frame's render resolution (display size / current scale ratio) and camera
    // jitter. The jittered view-projection below feeds the depth prepass and every forward/opaque/skybox/particle
    // pass; the shadow and motion-vectors passes deliberately keep using the camera's unjittered
    // view-projection (RenderScene()'s default when no override is supplied).
    UpdateRenderResolutionState();
    UpdateProjectionJitter();
    const XMMATRIX jitteredProjection =
        m_camera.GetProjection() * XMMatrixTranslation(m_jitterOffsetNdc.x, m_jitterOffsetNdc.y, 0.0f);
    const XMMATRIX viewProjection = m_camera.GetView() * jitteredProjection;

    /////////////////////////
    ///  SHADOW MAP PASS  ///
    /////////////////////////
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"ShadowMap Pass");

    auto shadowBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMap.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    commandList->ResourceBarrier(1, &shadowBarrier);

    commandList->RSSetViewports(1, &m_shadowViewport);
    commandList->RSSetScissorRects(1, &m_shadowScissor);
    const auto shadowDsv = m_shadowDsvHeap->GetFirstCpuHandle();
    commandList->OMSetRenderTargets(0, nullptr, FALSE, &shadowDsv);
    commandList->ClearDepthStencilView(shadowDsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetPipelineState(m_shadowPipelineState.Get());

    RenderScene(0, false, true, false, nullptr, &lightCullBounds);
    RenderScene(1, false, true, false, nullptr, &lightCullBounds);

    const auto shadowReadBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMap.Get(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &shadowReadBarrier);

    PIXEndEvent(commandList);

    // Viewport/scissor sized to the current render resolution (display size / scale ratio); the
    // underlying resources (depth buffer, scene color, motion vectors) all stay
    // allocated at max/display size, so only this sub-rect of each is touched this frame.
    D3D12_VIEWPORT viewport = m_deviceResources->GetScreenViewport();
    viewport.Width = static_cast<float>(m_renderWidth);
    viewport.Height = static_cast<float>(m_renderHeight);
    const D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(m_renderWidth), static_cast<LONG>(m_renderHeight) };
    const auto dsv = m_deviceResources->GetDepthStencilView();

    ///////////////////////
    ///  DEPTH PREPASS  ///
    ///////////////////////
    // Populate reverse depth without a pixel shader or color target so the opaque pass can
    // reject hidden fragments before running the expensive PBR shader. Reuse its vertex shaders,
    // jittered transforms, culling, and render viewport so both passes produce matching depth.
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Depth Prepass");

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetPipelineState(m_depthPipelineState.Get());

    RenderScene(0, false, false, true, &cameraCullFrustum, nullptr, nullptr, nullptr, false, &viewProjection);
    RenderScene(1, false, false, true, &cameraCullFrustum, nullptr, nullptr, nullptr, false, &viewProjection);

    PIXEndEvent(commandList);

    //////////////////////////////
    ///  MOTION VECTORS PASS   ///
    //////////////////////////////
    // Full-geometry redraw producing a UV-space current->previous displacement per pixel for
    // the temporal upscaler. Uses the same camera frustum culling as the opaque pass and only
    // draws opaque geometry (no particles/smoke), since transparent/particle motion is handled
    // separately (e.g. via a reactive mask).
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Motion Vectors Pass");

    CD3DX12_RESOURCE_BARRIER motionWriteBarriers[2];
    motionWriteBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_motionVectors.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    motionWriteBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_motionVectorDepth.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    commandList->ResourceBarrier(static_cast<UINT>(std::size(motionWriteBarriers)), motionWriteBarriers);

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    const auto motionVectorsRtv = m_motionVectorsRtvHeap->GetFirstCpuHandle();
    const auto motionVectorDsv = m_motionVectorDsvHeap->GetFirstCpuHandle();
    const float clearMotionVectors[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    commandList->OMSetRenderTargets(1, &motionVectorsRtv, FALSE, &motionVectorDsv);
    commandList->ClearRenderTargetView(motionVectorsRtv, clearMotionVectors, 0, nullptr);
    commandList->ClearDepthStencilView(motionVectorDsv, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetPipelineState(m_motionVectorsPipelineState.Get());

    RenderScene(0, false, false, false, &cameraCullFrustum, nullptr, nullptr, nullptr, true);
    RenderScene(1, false, false, false, &cameraCullFrustum, nullptr, nullptr, nullptr, true);

    CD3DX12_RESOURCE_BARRIER motionReadBarriers[2];
    motionReadBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_motionVectors.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    motionReadBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_motionVectorDepth.Get(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(static_cast<UINT>(std::size(motionReadBarriers)), motionReadBarriers);

    PIXEndEvent(commandList);

    ////////////////////////////
    ///  RENDER OPAQUE PASS  ///
    ////////////////////////////
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render Opaque");

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    // Render into the offscreen scene-color target (not the backbuffer) that FSR takes as input.
    // RenderFSR() later blits the upscaled result into the real backbuffer before the HUD pass.
    const auto sceneColorRtv = m_sceneColorRtvHeap->GetFirstCpuHandle();
    commandList->OMSetRenderTargets(1, &sceneColorRtv, FALSE, &dsv);

    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetGraphicsRootDescriptorTable(RootParameterShadow, m_srvHeap->GetGpuHandle(DescriptorIndex::ShadowMap));
    commandList->SetGraphicsRootDescriptorTable(RootParameterIrradiance, m_srvHeap->GetGpuHandle(DescriptorIndex::Irradiance));
    commandList->SetGraphicsRootDescriptorTable(RootParameterRadiance, m_srvHeap->GetGpuHandle(DescriptorIndex::Radiance));
    commandList->SetGraphicsRootDescriptorTable(RootParameterBrdfLut, m_srvHeap->GetGpuHandle(DescriptorIndex::BrdfLut));

    commandList->SetPipelineState(m_opaquePipelineState.Get());
    RenderScene(0, false, false, false, &cameraCullFrustum, nullptr, &m_cullVisibleInstances, &m_cullTotalInstances, false, &viewProjection);
    RenderScene(1, false, false, false, &cameraCullFrustum, nullptr, nullptr, nullptr, false, &viewProjection);
    PIXEndEvent(commandList);

    // Nothing for the rest of the frame writes depth (skybox, blended, particles, and smoke all
    // use DepthWriteMask = ZERO), so transition the depth buffer to a combined depth-read /
    // shader-read state and rebind it with a read-only DSV. This lets the smoke pass sample
    // SceneDepth as an SRV (for soft-particle depth fading) while depth testing stays in effect.
    const CD3DX12_RESOURCE_BARRIER depthToReadBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_deviceResources->GetDepthStencil(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &depthToReadBarrier);
    const auto readOnlyDsv = m_sceneDsvReadOnlyHeap->GetFirstCpuHandle();
    commandList->OMSetRenderTargets(1, &sceneColorRtv, FALSE, &readOnlyDsv);

    ////////////////////////////
    ///  RENDER SKYBOX PASS  ///
    ////////////////////////////
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render Skybox");
    SkyboxConstants skyboxConstants = {};
    XMStoreFloat4x4(&skyboxConstants.inverseViewProjection, XMMatrixTranspose(XMMatrixInverse(nullptr, viewProjection)));
    XMStoreFloat4(&skyboxConstants.eyePosition, m_camera.GetPosition());
    skyboxConstants.environmentParameters = XMFLOAT4(c_environmentIntensity, 0.0f, 0.0f, 0.0f);
    const auto skyboxConstantAllocation = m_graphicsMemory->AllocateConstant<SkyboxConstants>(skyboxConstants);

    commandList->SetGraphicsRootSignature(m_skyboxRootSignature.Get());
    commandList->SetPipelineState(m_skyboxPipelineState.Get());
    commandList->SetGraphicsRootConstantBufferView(0, skyboxConstantAllocation.GpuAddress());
    commandList->SetGraphicsRootDescriptorTable(1, m_srvHeap->GetGpuHandle(DescriptorIndex::Radiance));
    commandList->IASetVertexBuffers(0, 0, nullptr);
    commandList->IASetIndexBuffer(nullptr);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    PIXEndEvent(commandList);

    ////////////////////////
    ///  RENDER BLENDED  ///
    ////////////////////////
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render Blended");

    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetGraphicsRootDescriptorTable(RootParameterShadow, m_srvHeap->GetGpuHandle(DescriptorIndex::ShadowMap));
    commandList->SetGraphicsRootDescriptorTable(RootParameterIrradiance,  m_srvHeap->GetGpuHandle(DescriptorIndex::Irradiance));
    commandList->SetGraphicsRootDescriptorTable(RootParameterRadiance, m_srvHeap->GetGpuHandle(DescriptorIndex::Radiance));
    commandList->SetGraphicsRootDescriptorTable(RootParameterBrdfLut, m_srvHeap->GetGpuHandle(DescriptorIndex::BrdfLut));
    commandList->SetPipelineState(m_transparentPipelineState.Get());
    RenderScene(0, true, false, false, &cameraCullFrustum, nullptr, nullptr, nullptr, false, &viewProjection);
    RenderScene(1, true, false, false, &cameraCullFrustum, nullptr, nullptr, nullptr, false, &viewProjection);

    PIXEndEvent(commandList);

    ///////////////////////
    ///  PARTICLE PASS  ///
    ///////////////////////
    if (m_particlesEnabled)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render Particles");

        const XMMATRIX inverseView = XMMatrixInverse(nullptr, m_camera.GetView());
        ParticleConstants particleConstants = {};
        XMStoreFloat4x4(&particleConstants.viewProjection, XMMatrixTranspose(viewProjection));
        XMStoreFloat4(&particleConstants.cameraRightAndSize, XMVectorSetW(inverseView.r[0], 0.18f));
        XMStoreFloat4(
            &particleConstants.cameraUpAndTime,
            XMVectorSetW(
                inverseView.r[1],
                static_cast<float>(m_timer.GetTotalSeconds())));
        particleConstants.volumeCenterAndLifetime = XMFLOAT4(
            c_particleVolumeCenter.x,
            c_particleVolumeCenter.y,
            c_particleVolumeCenter.z,
            c_particleLifetime);
        particleConstants.volumeExtents = XMFLOAT4(
            c_particleVolumeExtents.x,
            c_particleVolumeExtents.y,
            c_particleVolumeExtents.z,
            0.0f);
        particleConstants.speedRange = XMFLOAT4(
            c_particleMinimumSpeed,
            c_particleMaximumSpeed,
            0.0f,
            0.0f);
        const auto particleConstantAllocation = m_graphicsMemory->AllocateConstant<ParticleConstants>(particleConstants);

        commandList->SetGraphicsRootSignature(m_particleRootSignature.Get());
        commandList->SetPipelineState(m_particlePipelineState.Get());
        commandList->SetGraphicsRootConstantBufferView(0, particleConstantAllocation.GpuAddress());
        commandList->IASetVertexBuffers(0, 0, nullptr);
        commandList->IASetIndexBuffer(nullptr);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawInstanced(6, m_particleCount, 0, 0);

        PIXEndEvent(commandList);
    }

    ////////////////////////////
    ///  SMOKE PARTICLE PASS ///
    ////////////////////////////
    if (m_particlesEnabled)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render Smoke");

        const uint32_t smokeParticleCount = GetSmokeParticleCount();
        const XMMATRIX inverseView = XMMatrixInverse(nullptr, m_camera.GetView());
        const float elapsedTime = static_cast<float>(m_timer.GetTotalSeconds());

        SmokeParticleConstants smokeConstants = {};
        XMStoreFloat4x4(&smokeConstants.viewProjection, XMMatrixTranspose(viewProjection));
        XMStoreFloat4(&smokeConstants.cameraRightAndSizeScale, XMVectorSetW(inverseView.r[0], 1.0f));
        XMStoreFloat4(&smokeConstants.cameraUpAndTime, XMVectorSetW(inverseView.r[1], elapsedTime));
        smokeConstants.emitterCenterAndLifetime = XMFLOAT4(
            c_smokeEmitterCenter.x,
            c_smokeEmitterCenter.y,
            c_smokeEmitterCenter.z,
            c_smokeLifetime);
        smokeConstants.emitterExtentsAndDrift = XMFLOAT4(
            c_smokeEmitterExtents.x,
            c_smokeEmitterExtents.y,
            c_smokeDriftAmplitude,
            0.0f);
        smokeConstants.settleHeightRange = XMFLOAT4(c_smokeMinimumSettleHeight, c_smokeMaximumSettleHeight, c_smokeDepthFadeDistance, 0.0f);
        smokeConstants.sizeRange = XMFLOAT4(c_smokeMinimumSize, c_smokeMaximumSize, 0.0f, 0.0f);
        XMStoreFloat4(&smokeConstants.cameraForwardAndNear, XMVectorSetW(inverseView.r[2], c_cameraNearDistance));
        XMStoreFloat4(&smokeConstants.eyePositionAndFar, XMVectorSetW(m_camera.GetPosition(), c_cameraFarDistance));
        const float spriteSheetCellInset = 1.0f / c_smokeSpriteSheetFrameSizePixels;
        smokeConstants.spriteSheetGrid = XMFLOAT4(
            static_cast<float>(c_smokeSpriteSheetColumns),
            static_cast<float>(c_smokeSpriteSheetRows),
            static_cast<float>(c_smokeSpriteSheetColumns * c_smokeSpriteSheetRows),
            spriteSheetCellInset);
        const auto smokeConstantAllocation = m_graphicsMemory->AllocateConstant<SmokeParticleConstants>(smokeConstants);

        // Estimate each particle's current world position on the CPU (mirroring the vertex
        // shader's placement formula, minus the small sway drift) purely to get a back-to-front
        // draw order; the GPU computes the authoritative position used for rendering.
        m_smokeParticleDrawOrder.resize(smokeParticleCount);
        std::vector<float> smokeViewDepths(smokeParticleCount);
        const XMMATRIX view = m_camera.GetView();
        for (uint32_t particleId = 0; particleId < smokeParticleCount; ++particleId)
        {
            m_smokeParticleDrawOrder[particleId] = particleId;

            const float initialPhase = SmokeParticleHashFloat(particleId ^ 0x9e3779b9u);
            const float lifePosition = elapsedTime / c_smokeLifetime + initialPhase;
            const float age = lifePosition - std::floor(lifePosition);
            const uint32_t generation = static_cast<uint32_t>(std::floor(lifePosition));
            const uint32_t seed = particleId * 747796405u + generation * 2891336453u;
            const XMFLOAT3 random = SmokeParticleHashFloat3(seed);

            const float settleHeight = c_smokeMinimumSettleHeight + (c_smokeMaximumSettleHeight - c_smokeMinimumSettleHeight) * random.x;
            const float riseT = std::clamp(age / c_smokeRiseFraction, 0.0f, 1.0f);
            const float riseProgress = riseT * riseT * (3.0f - 2.0f * riseT); // smoothstep(0, riseFraction, age)
            const float spawnX = c_smokeEmitterCenter.x + (random.y * 2.0f - 1.0f) * c_smokeEmitterExtents.x;
            const float spawnZ = c_smokeEmitterCenter.z + (random.z * 2.0f - 1.0f) * c_smokeEmitterExtents.y;
            const float height = c_smokeEmitterCenter.y + settleHeight * riseProgress;

            const XMVECTOR position = XMVectorSet(spawnX, height, spawnZ, 1.0f);
            smokeViewDepths[particleId] = XMVectorGetZ(XMVector3TransformCoord(position, view));
        }
        std::sort(
            m_smokeParticleDrawOrder.begin(),
            m_smokeParticleDrawOrder.end(),
            [&smokeViewDepths](uint32_t left, uint32_t right)
            {
                return smokeViewDepths[left] > smokeViewDepths[right];
            });

        // Upload the sorted draw order as a structured buffer; the vertex shader indexes it with
        // SV_InstanceID so a single instanced draw call still renders back-to-front, instead of
        // issuing one DrawInstanced call per particle.
        const auto smokeOrderAllocation = m_graphicsMemory->Allocate(sizeof(uint32_t) * smokeParticleCount, sizeof(uint32_t));
        memcpy(smokeOrderAllocation.Memory(), m_smokeParticleDrawOrder.data(), sizeof(uint32_t) * smokeParticleCount);

        commandList->SetGraphicsRootSignature(m_smokeParticleRootSignature.Get());
        commandList->SetPipelineState(m_smokeParticlePipelineState.Get());
        commandList->SetGraphicsRootConstantBufferView(0, smokeConstantAllocation.GpuAddress());
        commandList->SetGraphicsRootShaderResourceView(1, smokeOrderAllocation.GpuAddress());
        commandList->SetGraphicsRootDescriptorTable(2, m_srvHeap->GetGpuHandle(DescriptorIndex::SmokeTexture));
        commandList->SetGraphicsRootDescriptorTable(3, m_srvHeap->GetGpuHandle(DescriptorIndex::SceneDepth));
        commandList->IASetVertexBuffers(0, 0, nullptr);
        commandList->IASetIndexBuffer(nullptr);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Shade this pass at a coarser 2x2 rate: the smoke billboards are soft, low-frequency,
        // and heavily overlapping, so quartering the shaded-pixel count costs no visible detail
        // while directly cutting the texture/ALU cost of the pass's main overdraw bottleneck.
        if (m_variableShadingRateSupported)
        {
            m_commandList5->RSSetShadingRate(D3D12_SHADING_RATE_2X2, nullptr);
        }

        commandList->DrawInstanced(6, smokeParticleCount, 0, 0);

        if (m_variableShadingRateSupported)
        {
            m_commandList5->RSSetShadingRate(D3D12_SHADING_RATE_1X1, nullptr);
        }

        PIXEndEvent(commandList);
    }

    // Transition the depth buffer back to a writable state so the next frame's Clear() can write
    // it again.
    const CD3DX12_RESOURCE_BARRIER depthToWriteBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_deviceResources->GetDepthStencil(),
        D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE);
    commandList->ResourceBarrier(1, &depthToWriteBarrier);

    ///////////////////////
    ///  FSR4 UPSCALE   ///
    ///////////////////////
    RenderFSR(commandList);

    ////////////////////
    ///  RENDER HUD  ///
    ////////////////////
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render HUD");
    RenderHUD(commandList);
    PIXEndEvent(commandList);

    commandList->EndQuery(m_gpuTimestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, timestampIndex + 3);
    const UINT64 readbackOffset = static_cast<UINT64>(frameIndex) * 4 * sizeof(UINT64);
    commandList->ResolveQueryData(
        m_gpuTimestampQueryHeap.Get(),
        D3D12_QUERY_TYPE_TIMESTAMP,
        timestampIndex,
        4,
        m_gpuTimestampReadback.Get(),
        readbackOffset);
    m_gpuTimestampIssued[frameIndex] = true;

    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::RenderScene(size_t sceneIndex, bool alphaBlend, bool shadowPass, bool depthPrepass,
    const DirectX::BoundingFrustum* cameraCullFrustum,
    const DirectX::BoundingOrientedBox* lightCullBounds,
    uint32_t* visibleInstanceCountOut,
    uint32_t* totalInstanceCountOut,
    bool motionVectorsPass,
    const DirectX::XMMATRIX* viewProjectionOverride)
{
    auto& scene = m_scenes[sceneIndex];
    auto commandList = m_deviceResources->GetCommandList();

    if (sceneIndex == c_characterSceneIndex)
    {
        if (shadowPass)
        {
            commandList->SetPipelineState(m_skinnedShadowPipelineState.Get());
        }
        else if (motionVectorsPass)
        {
            commandList->SetPipelineState(m_skinnedMotionVectorsPipelineState.Get());
        }
        else if (depthPrepass)
        {
            commandList->SetPipelineState(m_skinnedDepthPipelineState.Get());
        }
        else
        {
            commandList->SetPipelineState(alphaBlend
                ? m_skinnedTransparentPipelineState.Get()
                : m_skinnedOpaquePipelineState.Get());
        }
    }

    const XMMATRIX viewProjection = viewProjectionOverride
        ? *viewProjectionOverride
        : (m_camera.GetView() * m_camera.GetProjection());
    const XMMATRIX lightViewProjection = GetLightViewProjection();

    SceneConstants sceneConstants = {};
    XMStoreFloat4x4(&sceneConstants.viewProjection, XMMatrixTranspose(viewProjection));
    XMStoreFloat4x4(&sceneConstants.lightViewProjection, XMMatrixTranspose(lightViewProjection));
    XMStoreFloat4(&sceneConstants.eyePosition, m_camera.GetPosition());
    XMStoreFloat4(&sceneConstants.lightDirection, XMVector3Normalize(c_lightDirection));
    XMStoreFloat4(&sceneConstants.lightColor, c_lightColor);
    sceneConstants.environmentParameters = XMFLOAT4(c_environmentIntensity, m_maxRadianceMip, 0.0f, 0.0f);
    XMStoreFloat4x4(&sceneConstants.previousViewProjection, XMMatrixTranspose(m_viewProjectionPrev));
    const auto sceneConstantsMem = m_graphicsMemory->AllocateConstant<SceneConstants>(sceneConstants);
    commandList->SetGraphicsRootConstantBufferView(RootParameterSceneConstants, sceneConstantsMem.GpuAddress());

    if (sceneIndex == c_characterSceneIndex && m_animationPlayer)
    {
        // Fill the upload allocation directly instead of staging an 8 KB palette on the stack.
        const auto jointConstantsMem = m_graphicsMemory->AllocateConstant<JointConstants>();
        auto& jointConstants = *static_cast<JointConstants*>(jointConstantsMem.Memory());
        for (auto& joint : jointConstants.joints)
        {
            XMStoreFloat4x4(&joint, XMMatrixTranspose(XMMatrixIdentity()));
        }

        const auto& palette = m_animationPlayer->GetJointPalette();
        DX::ThrowIfFalse(palette.size() <= AnimationPlayer::MaxJoints,
            "Animated model exceeds the shader joint-palette limit");

        for (size_t jointIndex = 0; jointIndex < palette.size(); ++jointIndex)
        {
            XMStoreFloat4x4(
                &jointConstants.joints[jointIndex],
                XMMatrixTranspose(XMLoadFloat4x4(&palette[jointIndex])));
        }

        commandList->SetGraphicsRootConstantBufferView(RootParameterJointConstants, jointConstantsMem.GpuAddress());

        if (motionVectorsPass)
        {
            const auto previousJointConstantsMem = m_graphicsMemory->AllocateConstant<JointConstants>();
            auto& previousJointConstants = *static_cast<JointConstants*>(previousJointConstantsMem.Memory());
            for (auto& joint : previousJointConstants.joints)
            {
                XMStoreFloat4x4(&joint, XMMatrixTranspose(XMMatrixIdentity()));
            }

            for (size_t jointIndex = 0; jointIndex < m_previousJointPalette.size() &&
                jointIndex < AnimationPlayer::MaxJoints; ++jointIndex)
            {
                XMStoreFloat4x4(&previousJointConstants.joints[jointIndex],
                    XMMatrixTranspose(XMLoadFloat4x4(&m_previousJointPalette[jointIndex])));
            }

            commandList->SetGraphicsRootConstantBufferView(RootParameterPreviousJointConstants,
                previousJointConstantsMem.GpuAddress());
        }
    }

    // glTF is right-handed; reflect Z after node/joint transforms and before sample placement
    // to retain the standard left-handed ATG camera without mirroring the rendered scene.
    const XMMATRIX gltfToSample = XMMatrixScaling(1.0f, 1.0f, -1.0f);
    const XMMATRIX sceneTransform = gltfToSample * (sceneIndex == c_characterSceneIndex
        ? m_pathManager->GetWorldTransform()
        : XMMatrixScaling(15.0f, 15.0f, 15.0f));
    const XMMATRIX sceneTransformPrev = sceneIndex == c_characterSceneIndex
        ? gltfToSample * XMLoadFloat4x4(&m_characterWorldTransformPrev)
        : sceneTransform;

    for (auto& modelInstance : scene->m_instances)
    {
        if (!modelInstance || modelInstance->modelIndex < 0)
        {
            continue;
        }

        auto& model = scene->m_uniqueModels[static_cast<uint32_t>(modelInstance->modelIndex)];
        XMMATRIX world = sceneTransform;
        XMMATRIX previousWorld = sceneTransformPrev;
        if (sceneIndex != c_characterSceneIndex)
        {
            world = modelInstance->world * sceneTransform;
            previousWorld = modelInstance->world * sceneTransformPrev;
        }

        if (model->hasLocalBounds && (cameraCullFrustum || lightCullBounds))
        {
            if (totalInstanceCountOut)
            {
                ++(*totalInstanceCountOut);
            }

            // Transform an AABB so the handedness reflection is not treated as a quaternion rotation.
            DirectX::BoundingBox instanceBounds;
            model->localBounds.Transform(instanceBounds, world);

            const bool visible = cameraCullFrustum
                ? (cameraCullFrustum->Contains(instanceBounds) != DirectX::DISJOINT)
                : (lightCullBounds->Contains(instanceBounds) != DirectX::DISJOINT);

            if (!visible)
            {
                continue;
            }

            if (visibleInstanceCountOut)
            {
                ++(*visibleInstanceCountOut);
            }
        }

        const XMMATRIX worldInverseTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, world));
        for (auto& mesh : model->meshBuffers)
        {
            auto& material = scene->m_materials[mesh.materialIndex];
            if (material.alphaBlend != alphaBlend || (shadowPass && material.alphaBlend))
            {
                continue;
            }

            MeshConstants meshConstants = {};
            XMStoreFloat4x4(&meshConstants.world, XMMatrixTranspose(world));
            XMStoreFloat4x4(&meshConstants.worldInverseTranspose, XMMatrixTranspose(worldInverseTranspose));
            meshConstants.baseColorFactor = material.baseColorFactor;
            meshConstants.materialFactors = XMFLOAT4(material.metallicFactor, material.roughnessFactor, 0.0f, 0.0f);
            XMStoreFloat4x4(&meshConstants.previousWorld, XMMatrixTranspose(previousWorld));
            const auto meshConstantsMem = m_graphicsMemory->AllocateConstant<MeshConstants>(meshConstants);

            commandList->SetGraphicsRootConstantBufferView(RootParameterMeshConstants, meshConstantsMem.GpuAddress());

            if (!shadowPass && !motionVectorsPass && !depthPrepass)
            {
                commandList->SetGraphicsRootDescriptorTable(RootParameterAlbedo, m_srvHeap->GetGpuHandle(material.albedoDescriptor));
                commandList->SetGraphicsRootDescriptorTable(RootParameterNormal, m_srvHeap->GetGpuHandle(material.normalDescriptor));
                commandList->SetGraphicsRootDescriptorTable(RootParameterMetallicRoughness, m_srvHeap->GetGpuHandle(material.metallicRoughnessDescriptor));
            }

            commandList->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);
            commandList->IASetIndexBuffer(&mesh.indexBufferView);
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            commandList->DrawIndexedInstanced(mesh.indexCount, 1, 0, 0, 0);
        }
    }
}

uint32_t Sample::GetSmokeParticleCount() const noexcept
{
    // Derive from the selected count step to avoid accumulating integer-rounding loss
    // when decreasing and then increasing the smoke count.
    float smokeParticleCount = static_cast<float>(c_smokeDefaultParticleCount);
    uint32_t particleCount = c_particleDefaultCount;
    while (particleCount < m_particleCount)
    {
        particleCount *= 2;
        smokeParticleCount *= c_smokeParticleCountScale;
    }
    while (particleCount > m_particleCount)
    {
        particleCount /= 2;
        smokeParticleCount /= c_smokeParticleCountScale;
    }
    return static_cast<uint32_t>(smokeParticleCount);
}

void Sample::RenderHUD(ID3D12GraphicsCommandList* commandList)
{
    if (m_variantMemory.empty())
    {
        return;
    }

    const auto size = m_deviceResources->GetOutputSize();
    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(static_cast<UINT>(size.right), static_cast<UINT>(size.bottom));
    safe.right -= 100;

    const XMFLOAT2 origin = {};
    const float uiScale = std::max(1.0f, static_cast<float>(size.bottom) / 1080.0f);
    constexpr float c_titleScale = 1.25f;
    const float yLineSpacing = static_cast<float>(m_font->GetLineSpacing()) * uiScale;
    const auto white = Colors::White;
    const auto green = ATG::Colors::Green;

    // Small helper matching the reference FSR sample's pattern of drawing a label followed by a
    // colored value (green when the setting is enabled/on, white otherwise) on the same line.
    const auto drawSetting = [&](XMFLOAT2 labelPos, const wchar_t* label, const wchar_t* value, bool enabled)
    {
        m_font->DrawString(m_hudBatch.get(), label, labelPos, white, 0, origin, uiScale);
        const XMFLOAT2 valuePos(labelPos.x + 8.0f * yLineSpacing, labelPos.y);
        m_font->DrawString(m_hudBatch.get(), value, valuePos, enabled ? green : white, 0, origin, uiScale);
    };

    m_hudBatch->Begin(commandList);

    ////////////////////////////
    ///  LEFT: INFO / STATS  ///
    ////////////////////////////
    XMFLOAT2 leftPos(40.0f * uiScale, static_cast<float>(safe.top));

    m_font->DrawString(m_hudBatch.get(), L"AMD FSR", leftPos, white, 0, origin, uiScale * c_titleScale);
    leftPos.y += yLineSpacing * (c_titleScale + 0.5f);

    wchar_t textBuffer[256] = {};

    swprintf_s(textBuffer, L"FSR API: %hs", m_upscalingVersionName ? m_upscalingVersionName : "unavailable");
    m_font->DrawString(m_hudBatch.get(), textBuffer, leftPos, white, 0, origin, uiScale);
    leftPos.y += yLineSpacing;

    drawSetting(leftPos, L"FSR Upscaler:", m_fsrEnabled ? L"On" : L"Off (native, HW-scaled)", m_fsrEnabled);
    leftPos.y += yLineSpacing;

#ifdef _GAMING_XBOX
    {
        const wchar_t* modelName = L"FSR4 Full";
        if (m_upscalerModel == UpscalerModel::Fsr4Lite)
        {
            modelName = L"FSR4 Lite";
        }
        else if (m_upscalerModel == UpscalerModel::Fsr3)
        {
            modelName = L"FSR3";
        }
        drawSetting(leftPos, L"FSR Model:", modelName, m_upscalerModel != UpscalerModel::Fsr4Full);
    }
    leftPos.y += 2.0f * yLineSpacing;
#else
    leftPos.y += yLineSpacing;
#endif

    swprintf_s(textBuffer, L"Target Resolution: %u x %u", size.right - size.left, size.bottom - size.top);
    m_font->DrawString(m_hudBatch.get(), textBuffer, leftPos, white, 0, origin, uiScale);
    leftPos.y += yLineSpacing;

    const float renderScaleRatio = c_renderScaleRatios[m_renderScaleIndex];
    swprintf_s(
        textBuffer,
        L"Render Resolution: %u x %u (%s, %.1fx)",
        m_renderWidth,
        m_renderHeight,
        c_renderScaleNames[m_renderScaleIndex],
        renderScaleRatio);
    m_font->DrawString(m_hudBatch.get(), textBuffer, leftPos, white, 0, origin, uiScale);
    leftPos.y += 2.0f * yLineSpacing;

    if (m_fsrEnabled)
    {
        swprintf_s(textBuffer, L"FSR duration: %.2f ms", m_fsrGpuTimeMs);
        m_font->DrawString(m_hudBatch.get(), textBuffer, leftPos, white, 0, origin, uiScale);
    }
    leftPos.y += yLineSpacing;

    swprintf_s(textBuffer, L"Frametime: %.2f ms (CPU)  %.2f ms (GPU)", m_cpuFrameTimeMs, m_gpuFrameTimeMs);
    m_font->DrawString(m_hudBatch.get(), textBuffer, leftPos, white, 0, origin, uiScale);
    leftPos.y += yLineSpacing;

    if (m_fsrGpuMemoryUsageAvailable)
    {
        swprintf_s(textBuffer, L"FSR GPU memory: %.1f MiB (aliasable %.1f MiB)",
            static_cast<double>(m_fsrGpuMemoryUsageBytes) / 1048576.0,
            static_cast<double>(m_fsrGpuAliasableMemoryUsageBytes) / 1048576.0);
    }
    else
    {
        swprintf_s(textBuffer, L"FSR GPU memory: unavailable");
    }
    m_font->DrawString(m_hudBatch.get(), textBuffer, leftPos, white, 0, origin, uiScale);
    leftPos.y += yLineSpacing * 1.5f;
    
    drawSetting(leftPos, L"RCAS Sharpening:", m_fsrSharpeningEnabled ? L"On" : L"Off", m_fsrSharpeningEnabled);
    leftPos.y += yLineSpacing;

    swprintf_s(textBuffer, L"%.2f", m_fsrSharpness);
    drawSetting(leftPos, L"Sharpness:", textBuffer, m_fsrSharpeningEnabled);
    leftPos.y += yLineSpacing;

    swprintf_s(textBuffer, L"%u", m_particlesEnabled ? m_particleCount : 0u);
    drawSetting(leftPos, L"Particles:", textBuffer, m_particlesEnabled);
    leftPos.y += yLineSpacing;

    swprintf_s(textBuffer, L"%u", m_particlesEnabled ? GetSmokeParticleCount() : 0u);
    drawSetting(leftPos, L"Smoke:", textBuffer, m_particlesEnabled);
    leftPos.y += yLineSpacing;

    drawSetting(leftPos, L"Magnifier:", m_magnifierEnabled ? L"On" : L"Off", m_magnifierEnabled);
    leftPos.y += yLineSpacing * 1.5f;

    ///////////////////////////////
    ///  RIGHT: CONTROLS LEGEND ///
    ///////////////////////////////
    XMFLOAT2 rightPos(static_cast<float>(safe.right) - (m_ctrlFont->GetLineSpacing() * 8.0f * uiScale),
        static_cast<float>(safe.top));

    DX::DrawControllerString(
        m_hudBatch.get(),
        m_font.get(),
        m_ctrlFont.get(),
        L"Global Controls\n"
#ifdef _GAMING_XBOX
        L"[View] Exit\n"
        L"[RThumb] Rotate Camera\n"
        L"[RThumb]-Press Reset Camera",
#else
        L"(Esc) Exit\n"
        L"(LClick-drag) Rotate Camera\n"
        L"(Home) Reset Camera",
#endif
        rightPos,
        white,
        uiScale);

    rightPos.y += yLineSpacing * 5.0f;

    DX::DrawControllerString(
        m_hudBatch.get(),
        m_font.get(),
        m_ctrlFont.get(),
        L"Upscaler Controls\n"
#ifdef _GAMING_XBOX
        L"[RT][LT] next/prev Target Res.\n"
        L"[B][A] next/prev quality mode\n"
        L"[Y] Toggle Sharpening\n"
        L"[RB][LB] +/- Sharpness\n"
        L"[DPad]Right Toggle FSR\n"
        L"[DPad]Left FSR4 Full/Lite/FSR3",
#else
        L"(L/K) next/prev quality mode\n"
        L"(M) Toggle Sharpening\n"
        L"(N/B) +/- Sharpness\n"
        L"(Space) Toggle FSR",
#endif
        rightPos,
        white,
        uiScale);

#ifdef _GAMING_XBOX
    rightPos.y += yLineSpacing * 8.0f;
#else
    rightPos.y += yLineSpacing * 6.0f;
#endif

    DX::DrawControllerString(
        m_hudBatch.get(),
        m_font.get(),
        m_ctrlFont.get(),
        L"Particle Controls\n"
#ifdef _GAMING_XBOX
        L"[X] Toggle Particles\n"
        L"[DPad]Up More Particles\n"
        L"[DPad]Down Less Particles",
#else
        L"(P) Toggle Particles\n"
        L"(O) More Particles\n"
        L"(I) Less Particles",
#endif
        rightPos,
        white,
        uiScale);

    rightPos.y += yLineSpacing * 5.0f;

    DX::DrawControllerString(
        m_hudBatch.get(),
        m_font.get(),
        m_ctrlFont.get(),
        L"Magnifier Controls\n"
#ifdef _GAMING_XBOX
        L"[LThumb]-Press Toggle Magnifier\n"
        L"[LThumb] Move Magnifier",
#else
        L"(Tab) Toggle Magnifier\n"
        L"(Arrow Keys) Move Magnifier",
#endif
        rightPos,
        white,
        uiScale);

    m_hudBatch->End();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the offscreen scene-color target (the forward passes render into this instead of the
    // backbuffer -- see RenderFSR()) and the main depth buffer.
    if (m_gltfSceneState != D3D12_RESOURCE_STATE_RENDER_TARGET)
    {
        const auto gltfSceneWriteBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_gltfScene.Get(),
            m_gltfSceneState, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &gltfSceneWriteBarrier);
    }
    m_gltfSceneState = D3D12_RESOURCE_STATE_RENDER_TARGET;

    const auto rtvDescriptor = m_sceneColorRtvHeap->GetFirstCpuHandle();
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(
        rtvDescriptor,
        ATG::Colors::Background,
        0,
        nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_hasPreviousFrameStart = false;
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
}

void Sample::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnDisplayChange()
{
}

void Sample::OnWindowSizeChanged(int width, int height)
{
#ifdef _GAMING_XBOX
    // The Xbox window/display dimensions need not match the selected presentation surface.
    // Ignore window-size notifications that would otherwise undo the user's target choice.
    height = c_targetResolutionHeights[m_targetResolutionIndex];
    width = height * 16 / 9;
    const auto size = m_deviceResources->GetOutputSize();
    if (!m_deviceResources->GetWindow() || (size.right == width && size.bottom == height))
        return;

    m_deviceResources->WaitForGpu();
    // Release the old FSR history before allocating the new, potentially larger targets.
    DestroyFsrContext();
#endif

    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

// Properties
void Sample::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 1920;
    height = 1080;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

#ifdef _GAMING_XBOX
    // Look up the FSR3 upscaler provider once per device, outside of any resize or context
    // teardown. The id doesn't change at runtime, so CreateOrUpdateFsrContext() reuses it.
    m_fsr3VersionId = FindFsr3UpscalerVersionId(m_ffxApi, device);
#endif

    // Variable Rate Shading support (used by the smoke particle pass below): Tier 1 is enough
    // since only a uniform per-draw-call rate is needed, not a shading-rate image or
    // per-primitive rates. The command list object is created once and reused every frame (see
    // DeviceResources::CreateDeviceResources()), so QI'ing it for the newer interface once here
    // avoids doing it every frame in Render().
    D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
    m_variableShadingRateSupported =
        SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6)))
        && options6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1
        && SUCCEEDED(m_deviceResources->GetCommandList()->QueryInterface(IID_GRAPHICS_PPV_ARGS(m_commandList5.ReleaseAndGetAddressOf())));

    m_variantMemory.clear();
    m_variantMemory.resize(_countof(c_gltfPaths));
    for (size_t i = 0; i < m_variantMemory.size(); ++i)
    {
        m_variantMemory[i].label = c_variantNames[i];
    }
    
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    CreateFrameTimingResources();

    // Load GLTF scenes
    std::vector<std::unique_ptr<glTF::Asset>> assets(_countof(c_gltfPaths));
    for (size_t i = 0; i < assets.size(); ++i)
    {
        wchar_t modelPath[MAX_PATH] = {};
        DX::FindMediaFile(modelPath, MAX_PATH, c_gltfPaths[i], c_folderPaths);

        const bool isBinaryGltf = _wcsicmp(std::filesystem::path(c_gltfPaths[i]).extension().c_str(), L".glb") == 0;
        assets[i] = std::make_unique<glTF::Asset>(isBinaryGltf ? modelPath : c_gltfPaths[i], c_folderPaths);

        // HUD memory reporting: a .glb embeds its binary buffer, so the file size covers it.
        // A .gltf references one or more external .bin files (MainSponza is split into
        // several so that no single file exceeds GitHub's 100 MB limit); add the size of
        // each loaded buffer, which matches its file size on disk.
        uint64_t bufferFileBytes = 0;
        if (!isBinaryGltf)
        {
            for (const auto& buffer : assets[i]->m_buffers)
            {
                bufferFileBytes += buffer->size();
            }
        }
        m_variantMemory[i].modelFileBytes = GetFileSize(modelPath) + bufferFileBytes;
    }

    size_t numTextures = 0;
    uint32_t sceneIndex = 0;
    m_scenes.resize(assets.size());
    for (auto& asset : assets)
    {
        numTextures += asset->m_textures.size();
        m_scenes[sceneIndex] = std::make_unique<Scene>();
        m_scenes[sceneIndex]->m_uniqueModels.resize(asset->m_meshes.size());
        m_scenes[sceneIndex]->m_materials.resize(asset->m_materials.size());
        sceneIndex++;
    }

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Create descriptor heap.
    m_srvHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, DescriptorIndex::Count + numTextures);
    m_shadowDsvHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
    m_sceneDsvReadOnlyHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
    m_motionVectorsRtvHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
    // Slot 0: offscreen scene-color target (m_gltfScene). Slot 1: magnifier's intermediate
    // pre-composite target (m_magnifierPreOutput), only written when the magnifier is enabled.
    m_sceneColorRtvHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 2);
    m_motionVectorDsvHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);

    const uint32_t white = 0xffffffff;
    D3D12_SUBRESOURCE_DATA whiteData = { &white, sizeof(white), sizeof(white) };
    DX::ThrowIfFailed(CreateTextureFromMemory(
        device,
        resourceUpload,
        1, 1,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        whiteData,
        m_whiteAlbedoTexture.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(
        device,
        m_whiteAlbedoTexture.Get(),
        m_srvHeap->GetCpuHandle(DescriptorIndex::WhiteAlbedo));

    const uint32_t flatNormal = 0xffff8080;
    D3D12_SUBRESOURCE_DATA flatNormalData = { &flatNormal, sizeof(flatNormal), sizeof(flatNormal) };
    DX::ThrowIfFailed(CreateTextureFromMemory(
        device,
        resourceUpload,
        1, 1,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        flatNormalData,
        m_flatNormalTexture.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(
        device,
        m_flatNormalTexture.Get(),
        m_srvHeap->GetCpuHandle(DescriptorIndex::FlatNormal));

    const uint32_t neutralRma = 0xffffffff;
    D3D12_SUBRESOURCE_DATA neutralRmaData = { &neutralRma, sizeof(neutralRma), sizeof(neutralRma) };
    DX::ThrowIfFailed(CreateTextureFromMemory(
        device,
        resourceUpload,
        1, 1,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        neutralRmaData,
        m_neutralRmaTexture.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(
        device,
        m_neutralRmaTexture.Get(),
        m_srvHeap->GetCpuHandle(DescriptorIndex::NeutralRma));

    wchar_t environmentPath[MAX_PATH] = {};
    DX::FindMediaFile(environmentPath, MAX_PATH, L"SunSubMixerHRes_diffuseIBL.dds", c_environmentFolderPaths);
    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, resourceUpload, environmentPath, m_irradianceTexture.ReleaseAndGetAddressOf(), false));
    CreateShaderResourceView(device, m_irradianceTexture.Get(), m_srvHeap->GetCpuHandle(DescriptorIndex::Irradiance), true);

    DX::FindMediaFile(environmentPath, MAX_PATH, L"SunSubMixerHRes_specularIBL.dds", c_environmentFolderPaths);
    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, resourceUpload, environmentPath, m_radianceTexture.ReleaseAndGetAddressOf(), false));
    CreateShaderResourceView(device, m_radianceTexture.Get(), m_srvHeap->GetCpuHandle(DescriptorIndex::Radiance), true);
    m_maxRadianceMip = static_cast<float>(std::max<int>(0, m_radianceTexture->GetDesc().MipLevels - 3));

    DX::FindMediaFile(environmentPath, MAX_PATH, L"Sponza_BRDF_LUT.dds", c_environmentFolderPaths);
    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, resourceUpload, environmentPath, m_brdfLutTexture.ReleaseAndGetAddressOf(), false));
    CreateShaderResourceView(device, m_brdfLutTexture.Get(), m_srvHeap->GetCpuHandle(DescriptorIndex::BrdfLut));

    wchar_t smokeTexturePath[MAX_PATH] = {};
    DX::FindMediaFile(smokeTexturePath, MAX_PATH, L"SmokeSpriteSheet.dds", c_particleFolderPaths);
    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, resourceUpload, smokeTexturePath, m_smokeTexture.ReleaseAndGetAddressOf(), false));
    CreateShaderResourceView(device, m_smokeTexture.Get(), m_srvHeap->GetCpuHandle(DescriptorIndex::SmokeTexture));

    // Upload textures to GPU.
    m_textureResources.reserve(numTextures);
    uint32_t texOffset = DescriptorIndex::Count;
    std::vector<uint32_t> texOffsets(_countof(c_gltfPaths));
    std::vector<std::vector<uint64_t>> textureFileBytes(assets.size());
    std::vector<std::vector<uint64_t>> textureGpuBytes(assets.size());
    for (size_t assetIndex = 0; assetIndex < assets.size(); ++assetIndex)
    {
        const auto& asset = assets[assetIndex];
        std::unordered_set<size_t> albedoTextureIndices;
        std::unordered_set<size_t> normalTextureIndices;
        std::unordered_set<size_t> metallicRoughnessTextureIndices;

        const auto recordSemantic = [](glTF::Texture* texture, std::unordered_set<size_t>& textureIndices)
        {
            if (texture && texture->index > 0 && !texture->source->path.empty())
            {
                // glTF::Texture::index is 1-based (0 means "no texture"), so convert it back to the
                // 0-based position in asset->m_textures before recording it.
                textureIndices.insert(texture->index - 1);
            }
        };

        for (const auto& material : asset->m_materials)
        {
            recordSemantic(material.textures[glTF::Material::kBaseColor], albedoTextureIndices);
            recordSemantic(material.textures[glTF::Material::kNormal], normalTextureIndices);
            recordSemantic(material.textures[glTF::Material::kMetallicRoughness], metallicRoughnessTextureIndices);
        }

        textureFileBytes[assetIndex].reserve(asset->m_textures.size());
        textureGpuBytes[assetIndex].reserve(asset->m_textures.size());

        std::vector<ComPtr<ID3D12Resource>> embeddedImageResources(asset->m_images.size());
        texOffsets[assetIndex] = texOffset;

        for (size_t textureIndex = 0; textureIndex < asset->m_textures.size(); ++textureIndex)
        {
            auto& tex = asset->m_textures[textureIndex];
            const size_t imageIndex = tex.source ? static_cast<size_t>(tex.source - asset->m_images.data()) : SIZE_MAX;

            if (assetIndex == 1
                && imageIndex < embeddedImageResources.size()
                && embeddedImageResources[imageIndex])
            {
                device->CreateShaderResourceView(embeddedImageResources[imageIndex].Get(), nullptr, m_srvHeap->GetCpuHandle(texOffset));
                textureFileBytes[assetIndex].push_back(0);
                textureGpuBytes[assetIndex].push_back(0);
                ++texOffset;
                continue;
            }

            wchar_t resolvedPath[MAX_PATH] = {};
            std::wstring filename;
            if (assetIndex == 1)
            {
                filename = c_characterTextureFiles.at(imageIndex);
            }
            else
            {
                if (!tex.source)
                {
                    throw std::exception("Texture source is null");
                }

                // remove directory from file name since all files are copied in the root path
                filename = std::filesystem::path(tex.source->path).filename().wstring();
            }

            DX::FindMediaFile(
                resolvedPath,
                MAX_PATH,
                filename.c_str(),
                assetIndex == 1 ? c_characterTextureFolderPaths : c_sponzaTextureFolderPaths);
            ComPtr<ID3D12Resource> textureResource;
            DX::ThrowIfFailed(CreateDDSTextureFromFile(
                device,
                resourceUpload,
                resolvedPath,
                textureResource.ReleaseAndGetAddressOf(),
                false));

            device->CreateShaderResourceView(
                textureResource.Get(),
                nullptr,
                m_srvHeap->GetCpuHandle(texOffset));

            m_textureResources.emplace_back(textureResource);

            const uint64_t fileBytes = GetFileSize(resolvedPath);
            const uint64_t gpuBytes = GetResourceSize(device, textureResource.Get());
            textureFileBytes[assetIndex].push_back(fileBytes);
            textureGpuBytes[assetIndex].push_back(gpuBytes);
            m_variantMemory[assetIndex].textureFileBytes += fileBytes;
            m_variantMemory[assetIndex].textureGpuBytes += gpuBytes;
            if (assetIndex == 1 && imageIndex < embeddedImageResources.size())
            {
                embeddedImageResources[imageIndex] = textureResource;
            }
            ++texOffset;
        }

        const auto accumulateBin =
            [&](const std::unordered_set<size_t>& textureIndices, TextureMemoryBin& bin)
        {
            for (const size_t textureIndex : textureIndices)
            {
                if (textureIndex >= textureFileBytes[assetIndex].size())
                {
                    continue;
                }

                bin.fileBytes += textureFileBytes[assetIndex][textureIndex];
                bin.gpuBytes += textureGpuBytes[assetIndex][textureIndex];
                ++bin.textureCount;
            }
        };

        accumulateBin(albedoTextureIndices, m_variantMemory[assetIndex].albedo);
        accumulateBin(normalTextureIndices, m_variantMemory[assetIndex].normal);
        accumulateBin(
            metallicRoughnessTextureIndices,
            m_variantMemory[assetIndex].metallicRoughness);

    }

    const CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    auto shadowDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R32_TYPELESS,
        c_shadowMapSize,
        c_shadowMapSize,
        1,
        1,
        1,
        0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

#ifdef _GAMING_XBOX
    shadowDesc.Flags |= D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY;
#endif

    const D3D12_CLEAR_VALUE shadowClear = { DXGI_FORMAT_D32_FLOAT, { 1.0f, 0 } };
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &shadowDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &shadowClear,
        IID_GRAPHICS_PPV_ARGS(m_shadowMap.ReleaseAndGetAddressOf())));
    m_shadowMap->SetName(L"Directional shadow map");

    D3D12_DEPTH_STENCIL_VIEW_DESC shadowDsvDesc = {};
    shadowDsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    shadowDsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(
        m_shadowMap.Get(),
        &shadowDsvDesc,
        m_shadowDsvHeap->GetFirstCpuHandle());

    D3D12_SHADER_RESOURCE_VIEW_DESC shadowSrvDesc = {};
    shadowSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    shadowSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shadowSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shadowSrvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(
        m_shadowMap.Get(),
        &shadowSrvDesc,
        m_srvHeap->GetCpuHandle(DescriptorIndex::ShadowMap));

    m_shadowViewport = {
        0.0f, 0.0f,
        static_cast<float>(c_shadowMapSize),
        static_cast<float>(c_shadowMapSize),
        0.0f, 1.0f
    };
    m_shadowScissor = {
        0, 0,
        static_cast<LONG>(c_shadowMapSize),
        static_cast<LONG>(c_shadowMapSize)
    };

    // Load Shaders, create Root signatures and PSOs
    {
        auto vertexShader = DX::ReadData(L"VertexShader.cso");
        auto skinnedVertexShader = DX::ReadData(L"SkinnedVertexShader.cso");
        auto pixelShader = DX::ReadData(L"PixelShader.cso");
        auto shadowVertexShader = DX::ReadData(L"ShadowVertexShader.cso");
        auto skinnedShadowVertexShader = DX::ReadData(L"SkinnedShadowVertexShader.cso");
        auto motionVectorsVertexShader = DX::ReadData(L"MotionVectorsVertexShader.cso");
        auto skinnedMotionVectorsVertexShader = DX::ReadData(L"SkinnedMotionVectorsVertexShader.cso");
        auto motionVectorsPixelShader = DX::ReadData(L"MotionVectorsPixelShader.cso");
        auto skyboxVertexShader = DX::ReadData(L"SkyboxVertexShader.cso");
        auto skyboxPixelShader = DX::ReadData(L"SkyboxPixelShader.cso");
        auto particleVertexShader = DX::ReadData(L"ParticleVertexShader.cso");
        auto particlePixelShader = DX::ReadData(L"ParticlePixelShader.cso");
        auto smokeParticleVertexShader = DX::ReadData(L"SmokeParticleVertexShader.cso");
        auto smokeParticlePixelShader = DX::ReadData(L"SmokeParticlePixelShader.cso");
        auto upscaleBlitVertexShader = DX::ReadData(L"UpscaleBlitVertexShader.cso");
        auto upscaleBlitPixelShader = DX::ReadData(L"UpscaleBlitPixelShader.cso");
        auto magnifierVertexShader = DX::ReadData(L"MagnifierVertexShader.cso");
        auto magnifierPixelShader = DX::ReadData(L"MagnifierPixelShader.cso");

        DX::ThrowIfFailed(device->CreateRootSignature(
            0,
            vertexShader.data(),
            vertexShader.size(),
            IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
        pipelineDesc.InputLayout = c_pbrInputLayout;
        pipelineDesc.pRootSignature = m_rootSignature.Get();
        pipelineDesc.VS = { vertexShader.data(), vertexShader.size() };
        pipelineDesc.PS = { pixelShader.data(), pixelShader.size() };
        pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        pipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        pipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        pipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        pipelineDesc.SampleMask = UINT_MAX;
        pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineDesc.NumRenderTargets = 1;
        pipelineDesc.RTVFormats[0] =
            m_deviceResources->GetBackBufferFormat();
        pipelineDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        pipelineDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(
            &pipelineDesc,
            IID_GRAPHICS_PPV_ARGS(m_opaquePipelineState.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC skinnedPipelineDesc = pipelineDesc;
        skinnedPipelineDesc.InputLayout = c_skinnedPbrInputLayout;
        skinnedPipelineDesc.VS = { skinnedVertexShader.data(), skinnedVertexShader.size() };
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(
            &skinnedPipelineDesc,
            IID_GRAPHICS_PPV_ARGS(m_skinnedOpaquePipelineState.ReleaseAndGetAddressOf())));

        // Keep the opaque pass's reverse-depth state and vertex transforms, but omit all
        // pixel shading and color writes. No normal buffer or SSAO resources are needed.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC depthPipelineDesc = pipelineDesc;
        depthPipelineDesc.PS = {};
        depthPipelineDesc.NumRenderTargets = 0;
        depthPipelineDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(
            &depthPipelineDesc,
            IID_GRAPHICS_PPV_ARGS(m_depthPipelineState.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC skinnedDepthPipelineDesc = depthPipelineDesc;
        skinnedDepthPipelineDesc.InputLayout = c_skinnedPbrInputLayout;
        skinnedDepthPipelineDesc.VS = { skinnedVertexShader.data(), skinnedVertexShader.size() };
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(
            &skinnedDepthPipelineDesc,
            IID_GRAPHICS_PPV_ARGS(m_skinnedDepthPipelineState.ReleaseAndGetAddressOf())));

        // Motion-vectors PSOs: same rasterizer/depth-compare settings as the opaque pass (still
        // derived from the un-mutated pipelineDesc/skinnedPipelineDesc at this point), but writing
        // to the dedicated R16G16_FLOAT motion-vector target and motion-vector depth buffer.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC motionVectorsPipelineDesc = pipelineDesc;
        motionVectorsPipelineDesc.VS = { motionVectorsVertexShader.data(), motionVectorsVertexShader.size() };
        motionVectorsPipelineDesc.PS = { motionVectorsPixelShader.data(), motionVectorsPixelShader.size() };
        motionVectorsPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R16G16_FLOAT;
        motionVectorsPipelineDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(
            &motionVectorsPipelineDesc,
            IID_GRAPHICS_PPV_ARGS(m_motionVectorsPipelineState.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC skinnedMotionVectorsPipelineDesc = motionVectorsPipelineDesc;
        skinnedMotionVectorsPipelineDesc.InputLayout = c_skinnedPbrInputLayout;
        skinnedMotionVectorsPipelineDesc.VS =
            { skinnedMotionVectorsVertexShader.data(), skinnedMotionVectorsVertexShader.size() };
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(
            &skinnedMotionVectorsPipelineDesc,
            IID_GRAPHICS_PPV_ARGS(m_skinnedMotionVectorsPipelineState.ReleaseAndGetAddressOf())));

        auto& blendTarget = pipelineDesc.BlendState.RenderTarget[0];
        blendTarget.BlendEnable = TRUE;
        blendTarget.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendTarget.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendTarget.BlendOp = D3D12_BLEND_OP_ADD;
        blendTarget.SrcBlendAlpha = D3D12_BLEND_ONE;
        blendTarget.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        blendTarget.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendTarget.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(
            &pipelineDesc,
            IID_GRAPHICS_PPV_ARGS(m_transparentPipelineState.ReleaseAndGetAddressOf())));

        auto& skinnedBlendTarget = skinnedPipelineDesc.BlendState.RenderTarget[0];
        skinnedBlendTarget = blendTarget;
        skinnedPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(
            &skinnedPipelineDesc,
            IID_GRAPHICS_PPV_ARGS(m_skinnedTransparentPipelineState.ReleaseAndGetAddressOf())));

        pipelineDesc.VS = { shadowVertexShader.data(), shadowVertexShader.size() };
        pipelineDesc.PS = {};
        pipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        pipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        pipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        pipelineDesc.RasterizerState.DepthBias = 1500;
        pipelineDesc.RasterizerState.SlopeScaledDepthBias = 1.5f;
        pipelineDesc.NumRenderTargets = 0;
        pipelineDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
        pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(
            &pipelineDesc,
            IID_GRAPHICS_PPV_ARGS(m_shadowPipelineState.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC skinnedShadowPipelineDesc = pipelineDesc;
        skinnedShadowPipelineDesc.InputLayout = c_skinnedPbrInputLayout;
        skinnedShadowPipelineDesc.VS =
            { skinnedShadowVertexShader.data(), skinnedShadowVertexShader.size() };
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(
            &skinnedShadowPipelineDesc,
            IID_GRAPHICS_PPV_ARGS(m_skinnedShadowPipelineState.ReleaseAndGetAddressOf())));

        DX::ThrowIfFailed(device->CreateRootSignature(
            0,
            skyboxVertexShader.data(),
            skyboxVertexShader.size(),
            IID_GRAPHICS_PPV_ARGS(m_skyboxRootSignature.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC skyboxPipelineDesc = {};
        skyboxPipelineDesc.pRootSignature = m_skyboxRootSignature.Get();
        skyboxPipelineDesc.VS = { skyboxVertexShader.data(), skyboxVertexShader.size() };
        skyboxPipelineDesc.PS = { skyboxPixelShader.data(), skyboxPixelShader.size() };
        skyboxPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        skyboxPipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        skyboxPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        skyboxPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        skyboxPipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
        skyboxPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        skyboxPipelineDesc.SampleMask = UINT_MAX;
        skyboxPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        skyboxPipelineDesc.NumRenderTargets = 1;
        skyboxPipelineDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        skyboxPipelineDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        skyboxPipelineDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(
            &skyboxPipelineDesc,
            IID_GRAPHICS_PPV_ARGS(m_skyboxPipelineState.ReleaseAndGetAddressOf())));

        DX::ThrowIfFailed(device->CreateRootSignature(
            0,
            particleVertexShader.data(),
            particleVertexShader.size(),
            IID_GRAPHICS_PPV_ARGS(m_particleRootSignature.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC particlePipelineDesc = {};
        particlePipelineDesc.pRootSignature = m_particleRootSignature.Get();
        particlePipelineDesc.VS = { particleVertexShader.data(), particleVertexShader.size() };
        particlePipelineDesc.PS = { particlePixelShader.data(), particlePixelShader.size() };
        particlePipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        particlePipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        particlePipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        auto& particleBlend = particlePipelineDesc.BlendState.RenderTarget[0];
        particleBlend.BlendEnable = TRUE;
        particleBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        particleBlend.DestBlend = D3D12_BLEND_ONE;
        particleBlend.BlendOp = D3D12_BLEND_OP_ADD;
        particleBlend.SrcBlendAlpha = D3D12_BLEND_ZERO;
        particleBlend.DestBlendAlpha = D3D12_BLEND_ONE;
        particleBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        particleBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        particlePipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        particlePipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        particlePipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        particlePipelineDesc.SampleMask = UINT_MAX;
        particlePipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        particlePipelineDesc.NumRenderTargets = 1;
        particlePipelineDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        particlePipelineDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        particlePipelineDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(
            &particlePipelineDesc,
            IID_GRAPHICS_PPV_ARGS(m_particlePipelineState.ReleaseAndGetAddressOf())));

        DX::ThrowIfFailed(device->CreateRootSignature(
            0,
            smokeParticleVertexShader.data(),
            smokeParticleVertexShader.size(),
            IID_GRAPHICS_PPV_ARGS(m_smokeParticleRootSignature.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC smokeParticlePipelineDesc = {};
        smokeParticlePipelineDesc.pRootSignature = m_smokeParticleRootSignature.Get();
        smokeParticlePipelineDesc.VS = { smokeParticleVertexShader.data(), smokeParticleVertexShader.size() };
        smokeParticlePipelineDesc.PS = { smokeParticlePixelShader.data(), smokeParticlePixelShader.size() };
        smokeParticlePipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        smokeParticlePipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        smokeParticlePipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        auto& smokeParticleBlend = smokeParticlePipelineDesc.BlendState.RenderTarget[0];
        smokeParticleBlend.BlendEnable = TRUE;
        smokeParticleBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        smokeParticleBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        smokeParticleBlend.BlendOp = D3D12_BLEND_OP_ADD;
        smokeParticleBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
        smokeParticleBlend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        smokeParticleBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        smokeParticleBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        smokeParticlePipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        smokeParticlePipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        smokeParticlePipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        smokeParticlePipelineDesc.SampleMask = UINT_MAX;
        smokeParticlePipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        smokeParticlePipelineDesc.NumRenderTargets = 1;
        smokeParticlePipelineDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        smokeParticlePipelineDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        smokeParticlePipelineDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(
            &smokeParticlePipelineDesc,
            IID_GRAPHICS_PPV_ARGS(m_smokeParticlePipelineState.ReleaseAndGetAddressOf())));

        // FSR upscale-output -> backbuffer blit (see RenderFSR()).
        DX::ThrowIfFailed(device->CreateRootSignature(
            0,
            upscaleBlitVertexShader.data(),
            upscaleBlitVertexShader.size(),
            IID_GRAPHICS_PPV_ARGS(m_upscaleBlitRootSignature.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC upscaleBlitPipelineDesc = {};
        upscaleBlitPipelineDesc.pRootSignature = m_upscaleBlitRootSignature.Get();
        upscaleBlitPipelineDesc.VS = { upscaleBlitVertexShader.data(), upscaleBlitVertexShader.size() };
        upscaleBlitPipelineDesc.PS = { upscaleBlitPixelShader.data(), upscaleBlitPixelShader.size() };
        upscaleBlitPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        upscaleBlitPipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        upscaleBlitPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        upscaleBlitPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        upscaleBlitPipelineDesc.DepthStencilState.DepthEnable = FALSE;
        upscaleBlitPipelineDesc.DepthStencilState.StencilEnable = FALSE;
        upscaleBlitPipelineDesc.SampleMask = UINT_MAX;
        upscaleBlitPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        upscaleBlitPipelineDesc.NumRenderTargets = 1;
        upscaleBlitPipelineDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        upscaleBlitPipelineDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(
            &upscaleBlitPipelineDesc,
            IID_GRAPHICS_PPV_ARGS(m_upscaleBlitPipelineState.ReleaseAndGetAddressOf())));

        // Magnifier overlay composite -> backbuffer (see RenderMagnifier()).
        DX::ThrowIfFailed(device->CreateRootSignature(
            0,
            magnifierVertexShader.data(),
            magnifierVertexShader.size(),
            IID_GRAPHICS_PPV_ARGS(m_magnifierRootSignature.ReleaseAndGetAddressOf())));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC magnifierPipelineDesc = {};
        magnifierPipelineDesc.pRootSignature = m_magnifierRootSignature.Get();
        magnifierPipelineDesc.VS = { magnifierVertexShader.data(), magnifierVertexShader.size() };
        magnifierPipelineDesc.PS = { magnifierPixelShader.data(), magnifierPixelShader.size() };
        magnifierPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        magnifierPipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        magnifierPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        magnifierPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        magnifierPipelineDesc.DepthStencilState.DepthEnable = FALSE;
        magnifierPipelineDesc.DepthStencilState.StencilEnable = FALSE;
        magnifierPipelineDesc.SampleMask = UINT_MAX;
        magnifierPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        magnifierPipelineDesc.NumRenderTargets = 1;
        magnifierPipelineDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        magnifierPipelineDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(
            &magnifierPipelineDesc,
            IID_GRAPHICS_PPV_ARGS(m_magnifierPipelineState.ReleaseAndGetAddressOf())));

        // Defaults mirroring the reference sample: a magenta-bordered 5x zoom inset with a 0.25
        // screen-height radius, initially offset down-right of the reticle.
        m_magnifierConstants = {};
        m_magnifierConstants.fBorderColorRGB[0] = 1.0f;
        m_magnifierConstants.fBorderColorRGB[3] = 1.0f;

        const auto magnifierOutputSize = m_deviceResources->GetOutputSize();
        m_magnifierConstants.uImageWidth = static_cast<uint32_t>(magnifierOutputSize.right);
        m_magnifierConstants.uImageHeight = static_cast<uint32_t>(magnifierOutputSize.bottom);
        m_magnifierConstants.iMousePos[0] = static_cast<int32_t>(magnifierOutputSize.right / 2.35f);
        m_magnifierConstants.iMousePos[1] = static_cast<int32_t>(magnifierOutputSize.bottom / 2.00f);
        m_magnifierConstants.fMagnificationAmount = 5.0f;
        m_magnifierConstants.fMagnifierScreenRadius = 0.25f;
        m_magnifierConstants.iMagnifierOffset[0] = 100;
        m_magnifierConstants.iMagnifierOffset[1] = 100;
    }

    // Create HUD resources
    {
        const RenderTargetState renderTargetState(
            m_deviceResources->GetBackBufferFormat(),
            m_deviceResources->GetDepthBufferFormat());

        const SpriteBatchPipelineStateDescription pipelineState(renderTargetState, &CommonStates::AlphaBlend);
        m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pipelineState);

        wchar_t fontPath[MAX_PATH] = {};
        DX::FindMediaFile(fontPath, MAX_PATH, L"SegoeUI_18.spritefont");
        m_font = std::make_unique<SpriteFont>(
            device,
            resourceUpload,
            fontPath,
            m_srvHeap->GetCpuHandle(DescriptorIndex::Font),
            m_srvHeap->GetGpuHandle(DescriptorIndex::Font));

        // Controller-icon legend font used with DX::DrawControllerString() for the on-screen
        // control hints (e.g. "[A] Lower render scale").
        wchar_t ctrlFontPath[MAX_PATH] = {};
        DX::FindMediaFile(ctrlFontPath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
        m_ctrlFont = std::make_unique<SpriteFont>(
            device,
            resourceUpload,
            ctrlFontPath,
            m_srvHeap->GetCpuHandle(DescriptorIndex::CtrlFont),
            m_srvHeap->GetGpuHandle(DescriptorIndex::CtrlFont));
    }

    // process GLTF scene
    sceneIndex = 0;
    for (size_t assetIndex = 0; assetIndex < assets.size(); ++assetIndex)
    {
        const auto& asset = assets[assetIndex];
        auto& scene = m_scenes[sceneIndex++];
        const uint32_t assetTexOffset = texOffsets[assetIndex];

        for (size_t materialIndex = 0; materialIndex < asset->m_materials.size(); ++materialIndex)
        {
            const auto& material = asset->m_materials[materialIndex];
            auto& renderData = scene->m_materials[materialIndex];
            const auto baseColorTexture = material.textures[glTF::Material::kBaseColor];
            const auto normalTexture = material.textures[glTF::Material::kNormal];
            const auto metallicRoughnessTexture =             material.textures[glTF::Material::kMetallicRoughness];

            renderData.alphaBlend = material.properties.alphaBlend;
            renderData.albedoDescriptor = baseColorTexture ? baseColorTexture->index - 1 + assetTexOffset : DescriptorIndex::WhiteAlbedo;
            renderData.normalDescriptor = normalTexture ? normalTexture->index - 1 + assetTexOffset : DescriptorIndex::FlatNormal;
            renderData.metallicRoughnessDescriptor = metallicRoughnessTexture ? metallicRoughnessTexture->index - 1 + assetTexOffset : DescriptorIndex::NeutralRma;
            renderData.baseColorFactor = XMFLOAT4(
            material.baseColorFactor[0],
            material.baseColorFactor[1],
            material.baseColorFactor[2],
            material.baseColorFactor[3]);
            renderData.metallicFactor = material.metallicFactor;
            renderData.roughnessFactor = material.roughnessFactor;
        }

        std::function<void(size_t, uint32_t, size_t)> perMeshCallback = [&scene](size_t meshIndex, uint32_t, size_t numPrimitives)
            {
                ModelData* model = new ModelData();
                scene->m_uniqueModels[meshIndex].reset(model);
                model->meshBuffers.resize(numPrimitives);
            };

        std::function<void(size_t, uint32_t, size_t, int, const Renderer::Primitive&)> perPrimitiveCallback
            = [this, &device, &scene, &resourceUpload, assetIndex]
            (size_t meshIndex, uint32_t, size_t primitiveIndex, int materialIndex, const Renderer::Primitive& outPrim)
            {
                const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
                ModelData& model = *scene->m_uniqueModels[meshIndex];
                auto& meshVbIb = model.meshBuffers[primitiveIndex];
                meshVbIb.materialIndex = static_cast<uint32_t>(materialIndex);

                // Compute (and merge into the model's) object-space bounding box from the raw
                // vertex positions, used later for per-instance frustum culling. Position is
                // always the first 3 floats (offset 0) of every vertex layout in this sample.
                if (outPrim.vertexCount > 0 && outPrim.vertexStride >= sizeof(XMFLOAT3))
                {
                    const uint8_t* vertexData = outPrim.VB->data();
                    XMVECTOR minPoint = XMVectorReplicate(FLT_MAX);
                    XMVECTOR maxPoint = XMVectorReplicate(-FLT_MAX);
                    for (uint32_t vertexIndex = 0; vertexIndex < outPrim.vertexCount; ++vertexIndex)
                    {
                        XMFLOAT3 position;
                        memcpy(&position, vertexData + static_cast<size_t>(vertexIndex) * outPrim.vertexStride, sizeof(XMFLOAT3));
                        const XMVECTOR point = XMLoadFloat3(&position);
                        minPoint = XMVectorMin(minPoint, point);
                        maxPoint = XMVectorMax(maxPoint, point);
                    }

                    DirectX::BoundingBox primitiveBounds;
                    DirectX::BoundingBox::CreateFromPoints(primitiveBounds, minPoint, maxPoint);

                    if (!model.hasLocalBounds)
                    {
                        model.localBounds = primitiveBounds;
                        model.hasLocalBounds = true;
                    }
                    else
                    {
                        DirectX::BoundingBox::CreateMerged(model.localBounds, model.localBounds, primitiveBounds);
                    }
                }

                // upload vertex buffer
                {
                    auto const desc = CD3DX12_RESOURCE_DESC::Buffer(outPrim.VB->size());
                    DX::ThrowIfFailed(device->CreateCommittedResource(
                        &heapProperties,
                        D3D12_HEAP_FLAG_NONE,
                        &desc,
#ifdef _GAMING_XBOX
                        D3D12_RESOURCE_STATE_COPY_DEST,
#else
                        D3D12_RESOURCE_STATE_COMMON,
#endif
                        nullptr,
                        IID_GRAPHICS_PPV_ARGS(meshVbIb.vb.ReleaseAndGetAddressOf())
                    ));

                    meshVbIb.vb->SetName(L"mesh vb");
                    m_variantMemory[assetIndex].modelGpuBytes += GetResourceSize(device, meshVbIb.vb.Get());

                    D3D12_SUBRESOURCE_DATA initData = { outPrim.VB->data(), 0, 0 };
                    resourceUpload.Upload(meshVbIb.vb.Get(), 0, &initData, 1);

                    resourceUpload.Transition(meshVbIb.vb.Get(),
                        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

                    meshVbIb.vertexBufferView.BufferLocation =
                        meshVbIb.vb->GetGPUVirtualAddress();
                    meshVbIb.vertexBufferView.SizeInBytes =
                        static_cast<UINT>(outPrim.VB->size());
                    meshVbIb.vertexBufferView.StrideInBytes =
                        outPrim.vertexStride;
                }

                // upload index buffer
                {
                    auto const desc = CD3DX12_RESOURCE_DESC::Buffer(outPrim.IB->size());
                    DX::ThrowIfFailed(device->CreateCommittedResource(
                        &heapProperties,
                        D3D12_HEAP_FLAG_NONE,
                        &desc,
#ifdef _GAMING_XBOX
                        D3D12_RESOURCE_STATE_COPY_DEST,
#else
                        D3D12_RESOURCE_STATE_COMMON,
#endif
                        nullptr,
                        IID_GRAPHICS_PPV_ARGS(meshVbIb.ib.ReleaseAndGetAddressOf())
                    ));

                    meshVbIb.ib->SetName(L"mesh ib");
                    meshVbIb.indexCount = outPrim.primCount;
                    m_variantMemory[assetIndex].modelGpuBytes += GetResourceSize(device, meshVbIb.ib.Get());

                    D3D12_SUBRESOURCE_DATA initData = { outPrim.IB->data(), 0, 0 };
                    resourceUpload.Upload(meshVbIb.ib.Get(), 0, &initData, 1);

                    resourceUpload.Transition(meshVbIb.ib.Get(),
                        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

                    meshVbIb.indexBufferView.BufferLocation =
                        meshVbIb.ib->GetGPUVirtualAddress();
                    meshVbIb.indexBufferView.SizeInBytes =
                        static_cast<UINT>(outPrim.IB->size());
                    meshVbIb.indexBufferView.Format =
                        outPrim.index32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
                }
            };

        IterateMeshes(*asset, perMeshCallback, perPrimitiveCallback);

        if (assetIndex == 1)
        {
            m_animationPlayer = std::make_unique<AnimationPlayer>();
            DX::ThrowIfFalse(
                m_animationPlayer->Initialize(*asset),
                "Animated GLB does not contain a supported animation and skin");

            DX::ThrowIfFalse(
                m_animationPlayer->GetAnimationCount() == asset->m_animations.size(),
                "Not all character animations were stored");
            for (size_t animationIndex = 0; animationIndex < m_characterAnimationIndices.size(); ++animationIndex)
            {
                m_characterAnimationIndices[animationIndex] =
                    m_animationPlayer->FindAnimation(c_characterAnimationNames[animationIndex]);
                DX::ThrowIfFalse(
                    m_characterAnimationIndices[animationIndex] != UINT32_MAX,
                    "Required character animation was not found");
            }
            if (m_pathManager->IsMoving())
            {
                DX::ThrowIfFalse(
                    m_animationPlayer->SetAnimation(
                        m_characterAnimationIndices[c_walkingAnimationIndex]),
                    "Failed to restore the character walking animation");
            }
            else
            {
                StartCurrentCheckpointAnimation(
                    m_pathManager->NeedsCheckpointAnimation(),
                    false);
            }
        }

        // iterate through nodes and flatten graph
        {
            scene->m_instances.resize(asset->m_nodes.size());

            std::function callback = [&scene](uint32_t curPos, int modelIndex, const DirectX::SimpleMath::Matrix& xform) -> void
                {
                    ModelInstance* modelInstance = new ModelInstance();
                    scene->m_instances[curPos].reset(modelInstance);
                    modelInstance->world = xform;
                    modelInstance->modelIndex = modelIndex;
                };

            WalkGraph(callback, asset->m_scene->nodes, 0, XMMatrixIdentity());
        }
    }

    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    finished.wait();
}

void Sample::CreateFrameTimingResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    const UINT backBufferCount = m_deviceResources->GetBackBufferCount();

    D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
    queryHeapDesc.Count = backBufferCount * 4;
    queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    DX::ThrowIfFailed(device->CreateQueryHeap(
        &queryHeapDesc,
        IID_GRAPHICS_PPV_ARGS(m_gpuTimestampQueryHeap.ReleaseAndGetAddressOf())));

    const CD3DX12_HEAP_PROPERTIES readbackHeap(D3D12_HEAP_TYPE_READBACK);
    const auto readbackDesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(backBufferCount) * 4 * sizeof(UINT64));
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &readbackHeap,
        D3D12_HEAP_FLAG_NONE,
        &readbackDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_gpuTimestampReadback.ReleaseAndGetAddressOf())));

    DX::ThrowIfFailed(m_deviceResources->GetCommandQueue()->GetTimestampFrequency(
        &m_gpuTimestampFrequency));
    std::fill(std::begin(m_gpuTimestampIssued), std::end(m_gpuTimestampIssued), false);
    m_gpuFrameTimeMs = 0.0f;
    m_fsrGpuTimeMs = 0.0f;
}

void Sample::CreateSceneDepthViews()
{
    auto device = m_deviceResources->GetD3DDevice();

    D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc = {};
    depthSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthSrvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(m_deviceResources->GetDepthStencil(), &depthSrvDesc, m_srvHeap->GetCpuHandle(DescriptorIndex::SceneDepth));

    // Read-only DSV onto the same depth buffer, used for the depth-test-only passes (skybox,
    // blended, particles, smoke) after the opaque pass, so the smoke pass can simultaneously bind
    // SceneDepth as an SRV for soft-particle depth fading.
    D3D12_DEPTH_STENCIL_VIEW_DESC readOnlyDsvDesc = {};
    readOnlyDsvDesc.Format = m_deviceResources->GetDepthBufferFormat();
    readOnlyDsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    readOnlyDsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    device->CreateDepthStencilView(m_deviceResources->GetDepthStencil(), &readOnlyDsvDesc, m_sceneDsvReadOnlyHeap->GetFirstCpuHandle());
}

void Sample::CreateMotionVectorResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    const auto outputSize = m_deviceResources->GetOutputSize();
    const UINT width = std::max<UINT>(static_cast<UINT>(outputSize.right - outputSize.left), 1u);
    const UINT height = std::max<UINT>(static_cast<UINT>(outputSize.bottom - outputSize.top), 1u);
    const CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);

    m_motionVectors.Reset();
    m_motionVectorDepth.Reset();

    auto motionVectorsDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R16G16_FLOAT,
        width,
        height,
        1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    const float motionClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const CD3DX12_CLEAR_VALUE motionClear(DXGI_FORMAT_R16G16_FLOAT, motionClearColor);

#ifdef _GAMING_XBOX
    motionVectorsDesc.Flags |= (D3D12XBOX_RESOURCE_FLAG_ALLOW_DCC | D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY);
#endif
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &motionVectorsDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &motionClear,
        IID_GRAPHICS_PPV_ARGS(m_motionVectors.ReleaseAndGetAddressOf())));
    m_motionVectors->SetName(L"Motion vectors");

    device->CreateRenderTargetView(m_motionVectors.Get(), nullptr, m_motionVectorsRtvHeap->GetFirstCpuHandle());
    CreateShaderResourceView(device, m_motionVectors.Get(), m_srvHeap->GetCpuHandle(DescriptorIndex::MotionVectors));

    // A dedicated depth buffer for the motion-vectors pass (rather than reusing the main scene
    // depth buffer), matching reverse-Z (clear to 0.0f) so it is consistent with the main opaque
    // pass's depth convention.
    auto motionDepthDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R32_TYPELESS,
        width,
        height,
        1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

#ifdef _GAMING_XBOX
    motionDepthDesc.Flags |= D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY;
#endif

    const D3D12_CLEAR_VALUE motionDepthClear = { DXGI_FORMAT_D32_FLOAT, { 0.0f, 0 } };
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &motionDepthDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &motionDepthClear,
        IID_GRAPHICS_PPV_ARGS(m_motionVectorDepth.ReleaseAndGetAddressOf())));
    m_motionVectorDepth->SetName(L"Motion vector depth");

    D3D12_DEPTH_STENCIL_VIEW_DESC motionDsvDesc = {};
    motionDsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    motionDsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(m_motionVectorDepth.Get(), &motionDsvDesc, m_motionVectorDsvHeap->GetFirstCpuHandle());

    D3D12_SHADER_RESOURCE_VIEW_DESC motionDepthSrvDesc = {};
    motionDepthSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    motionDepthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    motionDepthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    motionDepthSrvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(m_motionVectorDepth.Get(), &motionDepthSrvDesc, m_srvHeap->GetCpuHandle(DescriptorIndex::MotionVectorDepth));
}

// Allocates the offscreen scene-color target, the FSR upscale-output target, and (re)creates the
// FSR upscaling context sized for the current window. Called whenever the window size changes.
void Sample::CreateFSRResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    const auto outputSize = m_deviceResources->GetOutputSize();
    const UINT width = std::max<UINT>(static_cast<UINT>(outputSize.right - outputSize.left), 1u);
    const UINT height = std::max<UINT>(static_cast<UINT>(outputSize.bottom - outputSize.top), 1u);
    const CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);

    m_gltfScene.Reset();
    m_upsampleOutput.Reset();
    m_magnifierPreOutput.Reset();

    // Offscreen scene-color target: the opaque/skybox/blended/particle/smoke passes render into
    // this instead of the backbuffer, so FSR has an un-upscaled color buffer to read, and its
    // result is blitted into the real backbuffer before the (un-upscaled) HUD pass.
    auto gltfSceneDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        m_deviceResources->GetBackBufferFormat(),
        width,
        height,
        1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    const float sceneClearColor[] = { ATG::Colors::Background.f[0], ATG::Colors::Background.f[1], ATG::Colors::Background.f[2], ATG::Colors::Background.f[3] };
    const CD3DX12_CLEAR_VALUE gltfSceneClear(m_deviceResources->GetBackBufferFormat(), sceneClearColor);

#ifdef _GAMING_XBOX
    gltfSceneDesc.Flags |= (D3D12XBOX_RESOURCE_FLAG_ALLOW_DCC | D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY);
#endif
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &gltfSceneDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &gltfSceneClear,
        IID_GRAPHICS_PPV_ARGS(m_gltfScene.ReleaseAndGetAddressOf())));
    m_gltfScene->SetName(L"GLTF scene color (pre-upscale)");
    m_gltfSceneState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    device->CreateRenderTargetView(m_gltfScene.Get(), nullptr, m_sceneColorRtvHeap->GetFirstCpuHandle());
    // SRV used only when FSR is disabled, to blit m_gltfScene straight to the backbuffer instead
    // of FSR's upsample output (see RenderFSR()'s bypass path).
    CreateShaderResourceView(device, m_gltfScene.Get(), m_srvHeap->GetCpuHandle(DescriptorIndex::GltfSceneColor));

    // FSR upscale output. Uses a format FSR's compute shaders can reliably write to as a typed
    // UAV on both desktop and Xbox; it is blitted (not CopyResource'd, since the format can
    // differ from the backbuffer) into the real backbuffer via the upscale-blit pass.
    auto upsampleDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R10G10B10A2_UNORM,
        width,
        height,
        1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &upsampleDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_upsampleOutput.ReleaseAndGetAddressOf())));
    m_upsampleOutput->SetName(L"FSR upscale output");
    m_upsampleOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    CreateShaderResourceView(device, m_upsampleOutput.Get(), m_srvHeap->GetCpuHandle(DescriptorIndex::UpsampleOutput));

    // Magnifier's intermediate pre-composite target: same format/size as the backbuffer, used
    // only as the blit destination (instead of the backbuffer directly) when the magnifier
    // overlay is enabled, so RenderMagnifier() has a full final-color image to sample from.
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &gltfSceneDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &gltfSceneClear,
        IID_GRAPHICS_PPV_ARGS(m_magnifierPreOutput.ReleaseAndGetAddressOf())));
    m_magnifierPreOutput->SetName(L"Magnifier pre-composite output");
    m_magnifierPreOutputState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    device->CreateRenderTargetView(m_magnifierPreOutput.Get(), nullptr, m_sceneColorRtvHeap->GetCpuHandle(1));
    CreateShaderResourceView(device, m_magnifierPreOutput.Get(), m_srvHeap->GetCpuHandle(DescriptorIndex::MagnifierPreOutput));

    CreateOrUpdateFsrContext(width, height);
}

void Sample::FfxMsgCallback(uint32_t type, const wchar_t* message)
{
    if (type == FFX_API_MESSAGE_TYPE_ERROR)
    {
        OutputDebugStringW(L"FSR_API_DEBUG_ERROR: ");
    }
    else
    {
        OutputDebugStringW(L"FSR_API_DEBUG_WARNING: ");
    }
    OutputDebugStringW(message);
    OutputDebugStringW(L"\n");
}

// Creates (or, on a resize, recreates) the FSR upscaling context sized for the current window.
// width/height here is always the display size: since every render-scale ratio is >= 1.0 (a
// divisor of the display size), native (ratio 1.0) is also the largest possible render size, so
// maxRenderSize and maxUpscaleSize are both the display size.
void Sample::CreateOrUpdateFsrContext(uint32_t width, uint32_t height)
{
    DestroyFsrContext();

    m_fsrHistoryResetPending = true;
    m_jitterIndex = 0;
    m_jitterOffset = {};
    m_jitterOffsetNdc = {};

    auto device = m_deviceResources->GetD3DDevice();

#ifdef _GAMING_XBOX
    ffx::CreateBackendXboxDesc backendDesc{};
    backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_XBOX;
    backendDesc.device = device;
#else
    ffx::CreateBackendDX12Desc backendDesc{};
    backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
    backendDesc.device = device;
#endif

    ffx::CreateContextDescUpscale createFsr{};
    createFsr.maxRenderSize = { width, height };
    createFsr.maxUpscaleSize = { width, height };
    // Our backbuffer is SDR (B8G8R8A8_UNORM), so no HIGH_DYNAMIC_RANGE; the camera's far plane is
    // finite (c_cameraFarDistance), so no DEPTH_INFINITE; depth is reverse-Z, so DEPTH_INVERTED.
    // Our scene color (m_gltfScene) is tonemapped and gamma-encoded (see PixelShader.hlsl's
    // ToneMap()) before FSR ever sees it, so NON_LINEAR_COLORSPACE must be set so FSR linearizes
    // it internally for its own processing instead of assuming linear input.
    createFsr.flags = FFX_UPSCALE_ENABLE_AUTO_EXPOSURE | FFX_UPSCALE_ENABLE_DEPTH_INVERTED
        | FFX_UPSCALE_ENABLE_NON_LINEAR_COLORSPACE;
#if defined(_DEBUG)
    createFsr.flags |= FFX_UPSCALE_ENABLE_DEBUG_CHECKING;
    createFsr.fpMessage = &Sample::FfxMsgCallback;
#endif

    ffx::CreateContextDescUpscaleVersion headerVersion{};
    headerVersion.version = FFX_UPSCALER_VERSION;

    ffx::ReturnCode retCode = ffx::ReturnCode::Error;
#ifdef _GAMING_XBOX
    if (m_upscalerModel == UpscalerModel::Fsr3)
    {
        // By default the loader creates the newest provider (FSR4). To get FSR3 instead, look up
        // its provider version id and chain an override desc onto the create call.
        const uint64_t fsr3VersionId = m_fsr3VersionId;
        if (fsr3VersionId != 0)
        {
            ffx::CreateContextDescOverrideVersion overrideVersion{};
            overrideVersion.versionId = fsr3VersionId;
            retCode = FfxCreateContext(m_ffxApi, m_upscalingContext, createFsr, backendDesc, headerVersion, overrideVersion);
        }
        else
        {
            // No FSR3 provider on this runtime: fall back to FSR4 Full so the cycle keeps working.
            FfxMsgCallback(FFX_API_MESSAGE_TYPE_WARNING, L"FSR3 upscaler provider not available; using FSR4 Full.");
            m_upscalerModel = UpscalerModel::Fsr4Full;
        }
    }

    if (m_upscalerModel != UpscalerModel::Fsr3)
#endif // _GAMING_XBOX
    {
        retCode = FfxCreateContext(m_ffxApi, m_upscalingContext, createFsr, backendDesc, headerVersion);
    }

    if (retCode == ffx::ReturnCode::Ok)
    {
        m_fsrContextCreated = true;

        ffx::InitHelper<ffxQueryGetProviderVersion> providerVersionQuery{};
        if (FfxQuery(m_ffxApi, m_upscalingContext, providerVersionQuery) == ffx::ReturnCode::Ok)
        {
            m_upscalingVersionName = providerVersionQuery.versionName;
        }

        // Query this context's allocation once, including after a resize or model change.
        // Aliasable memory is a subset of the reported total, not an additional allocation.
        FfxApiEffectMemoryUsage memoryUsage{};
        ffx::QueryDescUpscaleGetGPUMemoryUsage memoryQuery{};
        memoryQuery.gpuMemoryUsageUpscaler = &memoryUsage;
        const ffx::ReturnCode memoryQueryResult = FfxQuery(m_ffxApi, m_upscalingContext, memoryQuery);
        if (memoryQueryResult == ffx::ReturnCode::Ok)
        {
            m_fsrGpuMemoryUsageBytes = memoryUsage.totalUsageInBytes;
            m_fsrGpuAliasableMemoryUsageBytes = memoryUsage.aliasableUsageInBytes;
            m_fsrGpuMemoryUsageAvailable = true;
        }
        else
        {
            wchar_t message[128] = {};
            swprintf_s(message, L"GPU memory query failed (return code %u).",
                static_cast<uint32_t>(memoryQueryResult));
            FfxMsgCallback(FFX_API_MESSAGE_TYPE_WARNING, message);
        }
    }
    else
    {
        // Non-fatal: keep the sample running without the upscaler rather than crashing.
        m_fsrContextCreated = false;
        m_fsrEnabled = false;
    }
}

// Loads the FidelityFX API loader DLL and resolves its entry points. The DLL is loaded at runtime
// rather than linked through an import library (.lib), so the sample needs no static library:
// the SDK's ffxLoadFunctions() (ffx_api_loader.h) fills in the ffxFunctions table with
// GetProcAddress(), and every FFX call goes through that table (see FfxCreateContext() etc.).
// The loader then finds and loads the upscaler DLL (amd_fidelityfx_upscaler_*.dll) itself.
void Sample::LoadFfxLoader()
{
#ifdef _GAMING_XBOX
    m_ffxLoaderModule = LoadLibraryExW(L"amd_fidelityfx_loader_xbox.dll", nullptr, 0);
#else
    // Only search the executable's own folder, so a same-named DLL elsewhere on the search path
    // can't be picked up instead of the one deployed next to the sample.
    m_ffxLoaderModule = LoadLibraryExW(L"amd_fidelityfx_loader_dx12.dll", nullptr, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
#endif
    DX::ThrowIfFalse(m_ffxLoaderModule != nullptr, "Failed to load the FidelityFX loader DLL");

    ffxLoadFunctions(&m_ffxApi, m_ffxLoaderModule);
    DX::ThrowIfFalse(m_ffxApi.CreateContext != nullptr && m_ffxApi.DestroyContext != nullptr
        && m_ffxApi.Configure != nullptr && m_ffxApi.Query != nullptr && m_ffxApi.Dispatch != nullptr,
        "FidelityFX loader DLL is missing an expected export");

#ifdef _GAMING_XBOX
    // Runtime switch between the full FSR4 Int8 model and the lighter "Lite" FSR4 Int8 model.
    // Exported by the XBOX loader, which forwards it to the upscaler DLL. Takes effect on the next
    // upscaler context (re)creation (see CreateOrUpdateFsrContext()).
    m_ffxXboxSetUseFsr4Lite = GetFfxLoaderExport<PfnFfxXboxSetUseFsr4Lite>(m_ffxLoaderModule, "ffxXboxSetUseFsr4Lite");
    m_ffxXboxGetUseFsr4Lite = GetFfxLoaderExport<PfnFfxXboxGetUseFsr4Lite>(m_ffxLoaderModule, "ffxXboxGetUseFsr4Lite");
    DX::ThrowIfFalse(m_ffxXboxSetUseFsr4Lite != nullptr && m_ffxXboxGetUseFsr4Lite != nullptr,
        "FidelityFX XBOX loader DLL is missing the FSR4 Lite exports");
#endif // _GAMING_XBOX
}

void Sample::UnloadFfxLoader() noexcept
{
    m_ffxApi = {};
#ifdef _GAMING_XBOX
    m_ffxXboxSetUseFsr4Lite = nullptr;
    m_ffxXboxGetUseFsr4Lite = nullptr;
#endif // _GAMING_XBOX

    if (m_ffxLoaderModule)
    {
        FreeLibrary(m_ffxLoaderModule);
        m_ffxLoaderModule = nullptr;
    }
}

void Sample::DestroyFsrContext()
{
    m_upscalingVersionName = nullptr;
    m_fsrGpuMemoryUsageBytes = 0;
    m_fsrGpuAliasableMemoryUsageBytes = 0;
    m_fsrGpuMemoryUsageAvailable = false;

    if (m_fsrContextCreated)
    {
        FfxDestroyContext(m_ffxApi, m_upscalingContext);
        m_fsrContextCreated = false;
    }
}

// Scales a display-resolution rect down to the current render resolution by dividing by the
// active render-scale ratio (ratio 1.0 = native, larger ratios render fewer pixels). The result
// is always top-left aligned, matching the sub-rect every pass's viewport/scissor is shrunk to.
D3D12_RECT Sample::GetScaledRect(const D3D12_RECT& rect) const
{
    const float ratio = c_renderScaleRatios[m_renderScaleIndex];
    D3D12_RECT scaled = rect;
    scaled.right = rect.left + static_cast<LONG>(static_cast<float>(rect.right - rect.left) / ratio);
    scaled.bottom = rect.top + static_cast<LONG>(static_cast<float>(rect.bottom - rect.top) / ratio);
    return scaled;
}

// Recomputes the current render resolution (display size / scale ratio).
// Called once per frame -- cheap, and avoids
// needing to special-case the frame a render-scale change is requested.
void Sample::UpdateRenderResolutionState()
{
    const auto displaySize = m_deviceResources->GetOutputSize();
    const D3D12_RECT scaledRect = GetScaledRect(displaySize);
    m_renderWidth = std::max<uint32_t>(static_cast<uint32_t>(scaledRect.right - scaledRect.left), 1u);
    m_renderHeight = std::max<uint32_t>(static_cast<uint32_t>(scaledRect.bottom - scaledRect.top), 1u);
}

// Queries the FSR context for this frame's sub-pixel jitter offset and converts it to an NDC
// translation that Render() bakes into the projection matrix used by every jittered pass. Skipped
// (reusing last frame's offset) when there's no FSR context yet, or during the one-frame
// render-scale-change transition, so a resolution change isn't compounded with a jitter pop.
void Sample::UpdateProjectionJitter()
{
    if (!m_fsrEnabled)
    {
        // FSR is off and we're presenting the raw render-resolution image directly (see the
        // bypass branch in RenderFSR()), so there's no reconstruction pass to benefit from
        // sub-pixel jitter -- it would only show up as render-resolution shimmer. Zero out any
        // jitter left over from when FSR was last enabled instead of just leaving it stale.
        m_jitterOffset = { 0.0f, 0.0f };
        m_jitterOffsetNdc = { 0.0f, 0.0f };
        return;
    }

    if (!m_fsrContextCreated || m_renderScaleChanged || m_fsrHistoryResetPending)
    {
        return;
    }

    const auto displaySize = m_deviceResources->GetOutputSize();
    const uint32_t displayWidth = std::max<uint32_t>(static_cast<uint32_t>(displaySize.right - displaySize.left), 1u);

    int32_t jitterPhaseCount = 0;
    ffx::QueryDescUpscaleGetJitterPhaseCount getJitterPhaseDesc{};
    getJitterPhaseDesc.displayWidth = displayWidth;
    getJitterPhaseDesc.renderWidth = m_renderWidth;
    getJitterPhaseDesc.pOutPhaseCount = &jitterPhaseCount;
    if (FfxQuery(m_ffxApi, m_upscalingContext, getJitterPhaseDesc) != ffx::ReturnCode::Ok)
    {
        return;
    }

    ffx::QueryDescUpscaleGetJitterOffset getJitterOffsetDesc{};
    getJitterOffsetDesc.index = static_cast<int32_t>(m_jitterIndex);
    getJitterOffsetDesc.phaseCount = jitterPhaseCount;
    getJitterOffsetDesc.pOutX = &m_jitterOffset.x;
    getJitterOffsetDesc.pOutY = &m_jitterOffset.y;
    if (FfxQuery(m_ffxApi, m_upscalingContext, getJitterOffsetDesc) != ffx::ReturnCode::Ok)
    {
        return;
    }

    m_jitterOffsetNdc =
    {
         2.0f * m_jitterOffset.x / static_cast<float>(m_renderWidth),
        -2.0f * m_jitterOffset.y / static_cast<float>(m_renderHeight)
    };

    ++m_jitterIndex;
}

// Moves the magnifier's sampled-area reticle with the left stick (or arrow keys on desktop) and
// keeps its on-screen inset non-overlapping with, and within the bounds of, the display --
// ported from the AMD FidelityFX_FSR reference sample's UpdateMagnifier(), with the stick swapped
// from right to left since the right stick is dedicated to camera rotation in this sample.
void Sample::UpdateMagnifier(const DirectX::GamePad::State& pad)
{
    if (!m_magnifierEnabled)
    {
        return;
    }

    constexpr float c_keyboardMagnifierSpeed = 12.0f;
    if (pad.IsConnected())
    {
        m_magnifierConstants.iMousePos[0] += static_cast<int32_t>(pad.thumbSticks.leftX * 20.0f);
        m_magnifierConstants.iMousePos[1] -= static_cast<int32_t>(pad.thumbSticks.leftY * 20.0f);
    }
    if (m_keyboardButtons.lastState.Left)
    {
        m_magnifierConstants.iMousePos[0] -= static_cast<int32_t>(c_keyboardMagnifierSpeed);
    }
    if (m_keyboardButtons.lastState.Right)
    {
        m_magnifierConstants.iMousePos[0] += static_cast<int32_t>(c_keyboardMagnifierSpeed);
    }
    if (m_keyboardButtons.lastState.Up)
    {
        m_magnifierConstants.iMousePos[1] -= static_cast<int32_t>(c_keyboardMagnifierSpeed);
    }
    if (m_keyboardButtons.lastState.Down)
    {
        m_magnifierConstants.iMousePos[1] += static_cast<int32_t>(c_keyboardMagnifierSpeed);
    }

    const auto size = m_deviceResources->GetOutputSize();
    m_magnifierConstants.uImageWidth = static_cast<uint32_t>(size.right);
    m_magnifierConstants.uImageHeight = static_cast<uint32_t>(size.bottom);

    const int32_t IMAGE_SIZE[2] = { static_cast<int32_t>(m_magnifierConstants.uImageWidth), static_cast<int32_t>(m_magnifierConstants.uImageHeight) };
    m_magnifierConstants.iMousePos[0] = std::max(0, std::min(m_magnifierConstants.iMousePos[0], IMAGE_SIZE[0]));
    m_magnifierConstants.iMousePos[1] = std::max(0, std::min(m_magnifierConstants.iMousePos[1], IMAGE_SIZE[1]));

    const int H = IMAGE_SIZE[1];
    const int radiusInPixelsMagnifier = static_cast<int32_t>(m_magnifierConstants.fMagnifierScreenRadius * H);
    const int radiusInPixelsMagnifiedArea = static_cast<int32_t>(m_magnifierConstants.fMagnifierScreenRadius * H / m_magnifierConstants.fMagnificationAmount);

    const bool bCirclesAreOverlapping = (radiusInPixelsMagnifiedArea + radiusInPixelsMagnifier) >
        std::sqrt(static_cast<float>(
            m_magnifierConstants.iMagnifierOffset[0] * m_magnifierConstants.iMagnifierOffset[0]
            + m_magnifierConstants.iMagnifierOffset[1] * m_magnifierConstants.iMagnifierOffset[1]));
    if (bCirclesAreOverlapping)
    {
        m_magnifierConstants.iMagnifierOffset[0] = radiusInPixelsMagnifiedArea + radiusInPixelsMagnifier + 1;
        m_magnifierConstants.iMagnifierOffset[1] = radiusInPixelsMagnifiedArea + radiusInPixelsMagnifier + 1;
    }

    for (int i = 0; i < 2; ++i)
    {
        const bool bMagnifierOutOfScreenRegion =
            m_magnifierConstants.iMousePos[i] + m_magnifierConstants.iMagnifierOffset[i] + radiusInPixelsMagnifier > IMAGE_SIZE[i]
            || m_magnifierConstants.iMousePos[i] + m_magnifierConstants.iMagnifierOffset[i] - radiusInPixelsMagnifier < 0;

        if (bMagnifierOutOfScreenRegion)
        {
            if (!(m_magnifierConstants.iMousePos[i] - m_magnifierConstants.iMagnifierOffset[i] + radiusInPixelsMagnifier > IMAGE_SIZE[i]
                || m_magnifierConstants.iMousePos[i] - m_magnifierConstants.iMagnifierOffset[i] - radiusInPixelsMagnifier < 0))
            {
                m_magnifierConstants.iMagnifierOffset[i] = -m_magnifierConstants.iMagnifierOffset[i];
            }
            else
            {
                if (m_magnifierConstants.iMousePos[i] + m_magnifierConstants.iMagnifierOffset[i] + radiusInPixelsMagnifier > IMAGE_SIZE[i])
                    m_magnifierConstants.iMagnifierOffset[i] = IMAGE_SIZE[i] - m_magnifierConstants.iMousePos[i] - radiusInPixelsMagnifier;
                if (m_magnifierConstants.iMousePos[i] + m_magnifierConstants.iMagnifierOffset[i] - radiusInPixelsMagnifier < 0)
                    m_magnifierConstants.iMagnifierOffset[i] = -m_magnifierConstants.iMousePos[i] + radiusInPixelsMagnifier;
            }
        }
    }
}

// Dispatches the FSR upscale pass: reads the offscreen scene-color/motion-vectors/depth buffers
// (all at render resolution, which here equals display resolution) and writes m_upsampleOutput,
// which is then blitted into the real backbuffer before the HUD pass. When FSR is disabled
// (m_fsrEnabled == false), the dispatch is skipped entirely and m_gltfScene -- still only
// populated in its top-left renderWidth x renderHeight sub-rect -- is blitted straight to the
// backbuffer instead, so the blit's hardware bilinear texture sampler (not FSR) performs any
// stretch. This lets a dev compare FSR's reconstruction against a plain HW-scaled image built
// from the exact same render resolution.
void Sample::RenderFSR(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"AMD FSR");

    const UINT fsrFrameIndex = m_deviceResources->GetCurrentFrameIndex();
    const UINT fsrTimestampIndex = fsrFrameIndex * 4;
    commandList->EndQuery(m_gpuTimestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, fsrTimestampIndex + 1);

    const auto outputSize = m_deviceResources->GetOutputSize();
    const float displayWidth = static_cast<float>(outputSize.right - outputSize.left);
    const float displayHeight = static_cast<float>(outputSize.bottom - outputSize.top);
    const float renderWidth = static_cast<float>(m_renderWidth);
    const float renderHeight = static_cast<float>(m_renderHeight);

    const bool useFsr = m_fsrContextCreated && m_fsrEnabled;

    if (!useFsr)
    {
        // Bypass path: no upscale dispatch at all. m_gltfScene is already in RENDER_TARGET state
        // (the forward passes just rendered into it) -- transition it straight to a pixel-shader
        // SRV for the blit below. The motion-vectors resources need no handling here: the Motion
        // Vectors pass already left them in PIXEL_SHADER_RESOURCE, which is exactly the state the
        // next frame's Motion Vectors pass expects at its own write-barrier.
        const auto gltfToReadBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_gltfScene.Get(),
            m_gltfSceneState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &gltfToReadBarrier);
        m_gltfSceneState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        const auto viewport = m_deviceResources->GetScreenViewport();
        const auto scissorRect = m_deviceResources->GetScissorRect();
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);
        // When the magnifier overlay is enabled, blit into its intermediate pre-composite target
        // instead of the backbuffer directly, so RenderMagnifier() below has a full final-color
        // image to sample from.
        const auto blitRtv = m_magnifierEnabled
            ? m_sceneColorRtvHeap->GetCpuHandle(1)
            : m_deviceResources->GetRenderTargetView();
        commandList->OMSetRenderTargets(1, &blitRtv, FALSE, nullptr);
        commandList->SetGraphicsRootSignature(m_upscaleBlitRootSignature.Get());
        commandList->SetPipelineState(m_upscaleBlitPipelineState.Get());

        // m_gltfScene is allocated at max (display) size but only its top-left
        // renderWidth x renderHeight sub-rect is populated this frame; scale the sampled UV
        // range down to that sub-rect so the blit stretches exactly the rendered pixels, not
        // stale/undefined texels beyond them.
        const float uvScale[2] = { renderWidth / displayWidth, renderHeight / displayHeight };
        commandList->SetGraphicsRoot32BitConstants(1, 2, uvScale, 0);
        commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGpuHandle(DescriptorIndex::GltfSceneColor));
        commandList->IASetVertexBuffers(0, 0, nullptr);
        commandList->IASetIndexBuffer(nullptr);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawInstanced(3, 1, 0, 0);

        // m_gltfScene is left in PIXEL_SHADER_RESOURCE state; Clear() already checks
        // m_gltfSceneState and transitions it back to RENDER_TARGET at the start of next frame.

        if (m_magnifierEnabled)
        {
            RenderMagnifier(commandList);
        }

        commandList->EndQuery(m_gpuTimestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, fsrTimestampIndex + 2);
        PIXEndEvent(commandList);
        return;
    }

    // The forward passes leave m_gltfScene in RENDER_TARGET state, and the motion-vectors pass
    // leaves its outputs in PIXEL_SHADER_RESOURCE state; FSR's backend expects compute-read
    // (NON_PIXEL_SHADER_RESOURCE) inputs, and internally manages the UAV transition/restore for
    // m_upsampleOutput around the dispatch.
    CD3DX12_RESOURCE_BARRIER preDispatchBarriers[4];
    preDispatchBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_gltfScene.Get(),
        m_gltfSceneState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    preDispatchBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_motionVectors.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    preDispatchBarriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_motionVectorDepth.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    preDispatchBarriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(m_upsampleOutput.Get(),
        m_upsampleOutputState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(static_cast<UINT>(std::size(preDispatchBarriers)), preDispatchBarriers);
    m_gltfSceneState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    m_upsampleOutputState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    {
        ffx::DispatchDescUpscale dispatchUpscale{};
        dispatchUpscale.commandList = commandList;

#ifdef _GAMING_XBOX
        dispatchUpscale.color = ffxApiGetResourceXbox(m_gltfScene.Get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
        dispatchUpscale.depth = ffxApiGetResourceXbox(m_motionVectorDepth.Get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
        dispatchUpscale.motionVectors = ffxApiGetResourceXbox(m_motionVectors.Get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
        dispatchUpscale.exposure = ffxApiGetResourceXbox(nullptr, FFX_API_RESOURCE_STATE_COMPUTE_READ);
        dispatchUpscale.output = ffxApiGetResourceXbox(m_upsampleOutput.Get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
        dispatchUpscale.reactive = ffxApiGetResourceXbox(nullptr, FFX_API_RESOURCE_STATE_COMPUTE_READ);
#else
        dispatchUpscale.color = ffxApiGetResourceDX12(m_gltfScene.Get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
        dispatchUpscale.depth = ffxApiGetResourceDX12(m_motionVectorDepth.Get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
        dispatchUpscale.motionVectors = ffxApiGetResourceDX12(m_motionVectors.Get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
        dispatchUpscale.exposure = ffxApiGetResourceDX12(nullptr, FFX_API_RESOURCE_STATE_COMPUTE_READ);
        dispatchUpscale.output = ffxApiGetResourceDX12(m_upsampleOutput.Get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
        dispatchUpscale.reactive = ffxApiGetResourceDX12(nullptr, FFX_API_RESOURCE_STATE_COMPUTE_READ);
#endif
        // Not using a reactive/transparency mask for this first integration.
        dispatchUpscale.transparencyAndComposition = {};

        // Jitter is queried from the FSR context once per frame in UpdateProjectionJitter() and
        // baked into the projection matrix used by every jittered pass; the motion-vectors pass
        // itself stays unjittered (see RenderScene()'s default view-projection).
        dispatchUpscale.jitterOffset = { m_jitterOffset.x, m_jitterOffset.y };
        dispatchUpscale.motionVectorScale = { renderWidth, renderHeight };
        dispatchUpscale.reset = (m_frame == 0) || m_renderScaleChanged || m_fsrHistoryResetPending;
        dispatchUpscale.enableSharpening = m_fsrSharpeningEnabled;
        dispatchUpscale.sharpness = m_fsrSharpness;
        dispatchUpscale.frameTimeDelta = static_cast<float>(m_timer.GetElapsedSeconds() * 1000.0);
        dispatchUpscale.preExposure = 1.0f;
        dispatchUpscale.renderSize = { m_renderWidth, m_renderHeight };
        dispatchUpscale.upscaleSize = { static_cast<uint32_t>(displayWidth), static_cast<uint32_t>(displayHeight) };
        dispatchUpscale.cameraFovAngleVertical = XM_PIDIV4;
        dispatchUpscale.cameraNear = c_cameraNearDistance;
        dispatchUpscale.cameraFar = c_cameraFarDistance;
        dispatchUpscale.flags = 0;

        const ffx::ReturnCode retCode = FfxDispatch(m_ffxApi, m_upscalingContext, dispatchUpscale);
        if (retCode == ffx::ReturnCode::Ok)
        {
            m_fsrHistoryResetPending = false;
            // FFX restores color/depth/motionVectors/output to the declared (compute-read) state
            // passed into the dispatch above.
            m_gltfSceneState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            m_upsampleOutputState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        }
    }

    commandList->EndQuery(m_gpuTimestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, fsrTimestampIndex + 2);

    // Restore the motion-vectors resources to the state the Motion Vectors pass expects at the
    // start of next frame.
    CD3DX12_RESOURCE_BARRIER postDispatchBarriers[2];
    postDispatchBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_motionVectors.Get(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    postDispatchBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_motionVectorDepth.Get(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(static_cast<UINT>(std::size(postDispatchBarriers)), postDispatchBarriers);

    // FfxDispatch() binds its own internal descriptor heap for the upscale compute work and
    // does not restore the caller's heap afterward, so re-bind ours before issuing any further
    // descriptor-table-based draws (the blit below).
    ID3D12DescriptorHeap* heaps[] = { m_srvHeap->Heap() };
    commandList->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);

    // Blit the upscale output into the real backbuffer (a plain CopyResource isn't used since the
    // upscale-output and backbuffer formats can differ).
    const auto upsampleReadBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_upsampleOutput.Get(),
        m_upsampleOutputState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &upsampleReadBarrier);
    m_upsampleOutputState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    const auto viewport = m_deviceResources->GetScreenViewport();
    const auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    // When the magnifier overlay is enabled, blit into its intermediate pre-composite target
    // instead of the backbuffer directly, so RenderMagnifier() below has a full final-color image
    // to sample from.
    const auto blitRtv = m_magnifierEnabled
        ? m_sceneColorRtvHeap->GetCpuHandle(1)
        : m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &blitRtv, FALSE, nullptr);
    commandList->SetGraphicsRootSignature(m_upscaleBlitRootSignature.Get());
    commandList->SetPipelineState(m_upscaleBlitPipelineState.Get());

    // m_upsampleOutput is always exactly display-sized and fully written by FSR, so no UV
    // sub-rect scaling is needed here (unlike the bypass path above).
    const float uvScaleFull[2] = { 1.0f, 1.0f };
    commandList->SetGraphicsRoot32BitConstants(1, 2, uvScaleFull, 0);
    commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGpuHandle(DescriptorIndex::UpsampleOutput));
    commandList->IASetVertexBuffers(0, 0, nullptr);
    commandList->IASetIndexBuffer(nullptr);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    if (m_magnifierEnabled)
    {
        RenderMagnifier(commandList);
    }

    const auto upsampleBackToUavBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_upsampleOutput.Get(),
        m_upsampleOutputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(1, &upsampleBackToUavBarrier);
    m_upsampleOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    PIXEndEvent(commandList);
}

// Composites the magnifier overlay (a zoomed-in circular inset of the area around the
// reticle/mouse position, connected by border lines) from m_magnifierPreOutput into the real
// backbuffer. Only called from RenderFSR() when m_magnifierEnabled is true, and only after the
// upscale-blit (or bypass-blit) has just written the full final-color image into
// m_magnifierPreOutput instead of the backbuffer directly.
void Sample::RenderMagnifier(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Magnifier");

    const auto preOutputReadBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_magnifierPreOutput.Get(),
        m_magnifierPreOutputState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &preOutputReadBarrier);
    m_magnifierPreOutputState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    const auto viewport = m_deviceResources->GetScreenViewport();
    const auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    const auto backBufferRtv = m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &backBufferRtv, FALSE, nullptr);
    commandList->SetGraphicsRootSignature(m_magnifierRootSignature.Get());
    commandList->SetPipelineState(m_magnifierPipelineState.Get());

    const auto magnifierConstantAllocation = m_graphicsMemory->AllocateConstant<MagnifierConstants>(m_magnifierConstants);
    commandList->SetGraphicsRootConstantBufferView(0, magnifierConstantAllocation.GpuAddress());
    commandList->SetGraphicsRootDescriptorTable(1, m_srvHeap->GetGpuHandle(DescriptorIndex::MagnifierPreOutput));
    commandList->IASetVertexBuffers(0, 0, nullptr);
    commandList->IASetIndexBuffer(nullptr);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    // Restore the invariant that m_magnifierPreOutputState is always RENDER_TARGET outside of
    // this function's (and RenderFSR()'s blit-into-it) transient read/write steps.
    const auto preOutputWriteBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_magnifierPreOutput.Get(),
        m_magnifierPreOutputState, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &preOutputWriteBarrier);
    m_magnifierPreOutputState = D3D12_RESOURCE_STATE_RENDER_TARGET;

    PIXEndEvent(commandList);
}


void Sample::CreateWindowSizeDependentResources()
{
    auto const size = m_deviceResources->GetOutputSize(); 
    auto displayWidth = static_cast<int>(size.right - size.left);
    auto displayHeight = static_cast<int>(size.bottom - size.top);

    m_camera.SetWindow(displayWidth, displayHeight);
    m_camera.SetProjectionParameters(XM_PIDIV4, c_cameraFarDistance, c_cameraNearDistance, true);

    // Keep the magnifier's reticle and inset in the same relative position after a resize.
    const float magnifierScaleX = static_cast<float>(displayWidth) / static_cast<float>(m_magnifierConstants.uImageWidth);
    const float magnifierScaleY = static_cast<float>(displayHeight) / static_cast<float>(m_magnifierConstants.uImageHeight);
    m_magnifierConstants.iMousePos[0] = static_cast<int32_t>(static_cast<float>(m_magnifierConstants.iMousePos[0]) * magnifierScaleX);
    m_magnifierConstants.iMousePos[1] = static_cast<int32_t>(static_cast<float>(m_magnifierConstants.iMousePos[1]) * magnifierScaleY);
    m_magnifierConstants.iMagnifierOffset[0] = static_cast<int32_t>(static_cast<float>(m_magnifierConstants.iMagnifierOffset[0]) * magnifierScaleX);
    m_magnifierConstants.iMagnifierOffset[1] = static_cast<int32_t>(static_cast<float>(m_magnifierConstants.iMagnifierOffset[1]) * magnifierScaleY);
    m_magnifierConstants.uImageWidth = static_cast<uint32_t>(displayWidth);
    m_magnifierConstants.uImageHeight = static_cast<uint32_t>(displayHeight);

    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());
    CreateSceneDepthViews();
    CreateMotionVectorResources();
    CreateFSRResources();
    UpdateRenderResolutionState();

    // Discard timing samples from the old target, including the CPU-side resize stall.
    std::fill(std::begin(m_gpuTimestampIssued), std::end(m_gpuTimestampIssued), false);
    m_gpuFrameTimeMs = 0.0f;
    m_fsrGpuTimeMs = 0.0f;
    m_cpuFrameTimeMs = 0.0f;
    m_hasPreviousFrameStart = false;
}

void Sample::OnDeviceLost()
{
    DestroyFsrContext();
    m_textureResources.clear();
    m_scenes.clear();
    m_graphicsMemory.reset();
    m_srvHeap.reset();
    m_shadowDsvHeap.reset();
    m_motionVectorsRtvHeap.reset();
    m_motionVectorDsvHeap.reset();
    m_sceneColorRtvHeap.reset();
    m_rootSignature.Reset();
    m_opaquePipelineState.Reset();
    m_transparentPipelineState.Reset();
    m_shadowPipelineState.Reset();
    m_depthPipelineState.Reset();
    m_skinnedOpaquePipelineState.Reset();
    m_skinnedTransparentPipelineState.Reset();
    m_skinnedShadowPipelineState.Reset();
    m_skinnedDepthPipelineState.Reset();
    m_motionVectorsPipelineState.Reset();
    m_skinnedMotionVectorsPipelineState.Reset();
    m_skyboxRootSignature.Reset();
    m_skyboxPipelineState.Reset();
    m_particleRootSignature.Reset();
    m_particlePipelineState.Reset();
    m_smokeParticleRootSignature.Reset();
    m_smokeParticlePipelineState.Reset();
    m_commandList5.Reset();
    m_variableShadingRateSupported = false;
    m_upscaleBlitRootSignature.Reset();
    m_upscaleBlitPipelineState.Reset();
    m_gpuTimestampQueryHeap.Reset();
    m_gpuTimestampReadback.Reset();
    m_gpuTimestampFrequency = 0;
    std::fill(std::begin(m_gpuTimestampIssued), std::end(m_gpuTimestampIssued), false);
    m_shadowMap.Reset();
    m_motionVectors.Reset();
    m_motionVectorDepth.Reset();
    m_gltfScene.Reset();
    m_upsampleOutput.Reset();
    m_magnifierPreOutput.Reset();
    m_magnifierRootSignature.Reset();
    m_magnifierPipelineState.Reset();
    m_hudBatch.reset();
    m_font.reset();
    m_ctrlFont.reset();
    m_whiteAlbedoTexture.Reset();
    m_flatNormalTexture.Reset();
    m_neutralRmaTexture.Reset();
    m_irradianceTexture.Reset();
    m_radianceTexture.Reset();
    m_brdfLutTexture.Reset();
    m_smokeTexture.Reset();
    m_animationPlayer.reset();
    m_maxRadianceMip = 0.0f;
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
