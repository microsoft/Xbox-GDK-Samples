//--------------------------------------------------------------------------------------
// DXRContent.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DXRContent.h"

#include "ATGColors.h"
#include "Constants.h"
#include "ControllerFont.h"
#include "FindMedia.h"
#include "ReadData.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wnonportable-include-path"
#endif

#include "gltf/gltf.h"
#include "gltf/GltfProcessing.h"

#include <filesystem>
#include <latch>

extern void ExitSample() noexcept;

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
    constexpr float c_nearClipDistance = 0.25f;
    constexpr float c_farClipDistance = 10000.0f;

    struct DescriptorIndex
    {
        enum
        {
            Font,
            FontBold,
            ControllerFont,
            RTOutputUAV,
            GrayTexture,
            HeatmapTexture,
            MeshInfo,
            Skybox,
            Count
        };
    };

    struct RootSignature
    {
        enum
        {
            Constants,
            TLAS,
            RTOutput,
            MeshInfo,
            Skybox,
            Heatmap,
            DebugConstants
        };
    };

    struct GpuTimerIndex
    {
        enum
        {
            Raytrace,
            TLASBuild,
            BLASBuild
        };
    };

    struct CpuTimerIndex
    {
        enum
        {
            BLASBuild
        };
    };

    struct BuildMode
    {
        enum
        {
            FastTrace,
            FastBuild,
            MinimizeMemory,
            Count
        };
    };

    struct DebugMode
    {
        enum
        {
            None,
            Iterations,
            ShadowIterations,
            Time,
            GeometryIndex,
            Instances,
            Count
        };
    };

    struct ContentMode
    {
        enum
        {
            Good,
            Bad,
            Off,
            Count,
        };
    };

    // Asset paths
    const wchar_t* c_gltfPaths[] =
    {
        L"ship_pinnace_4k_merged_meshes.gltf", // good
        L"coast_rocks_03_4k.gltf", // neutral
        L"island_tree_01_4k.gltf", // good
        L"ship_pinnace_4k.gltf", // bad
        L"island_tree_01_separate.gltf", // bad
        L"pier.gltf", // good
        L"pier_separated.gltf", // bad
        L"ocean_merged.gltf", // good
        L"ocean_split.gltf", // bad
        L"fence.gltf", // good
        L"fence_merged.gltf", // bad
        L"shovel_rotated.gltf", // good
        L"shovel_rotated_baked.gltf", // bad
        L"skybox.gltf", // bad
    };

    struct ContentPair
    {
        int32_t goodIndex;
        int32_t badIndex;
    };

    const ContentPair c_contentPairs[Sample::ContentOption::Count] =
    {
        { 0, 3 }, // ship
        // note: the island has only one version of the model
        { 1, 1 }, // island
        { 2, 4 }, // tree
        { 5, 6 }, // pier
        { 7, 8 }, // ocean
        { 9, 10 }, // fence
        { 11, 12 }, // shovel
        // note: the good version of the skybox is not having a model
        { -1, 13 }, // skybox
    };

    SceneInstance s_sceneCollection[] =
    {
        { Sample::ContentOption::Ship, XMMatrixRotationX(0.05f) * XMMatrixRotationY(-2.0f) * XMMatrixScaling(2.5f, 2.5f, 2.5f) * XMMatrixTranslation(15.0f, 0.0f, -40.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Ship, XMMatrixRotationX(-0.1f) * XMMatrixRotationY(1.0f) * XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(-295.0f, 0.0f, -30.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Ship, XMMatrixRotationY(0.4f) * XMMatrixScaling(1.5f, 1.5f, 1.5f) * XMMatrixTranslation(295.0f, 3.0f, -130.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Ship, XMMatrixRotationX(0.05f) * XMMatrixRotationY(2.4f) * XMMatrixScaling(2.5f, 2.5f, 2.5f) * XMMatrixTranslation(195.0f, 2.0f, 230.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Ship, XMMatrixRotationY(-1.4f) * XMMatrixScaling(2.f, 2.f, 2.f) * XMMatrixTranslation(-265.0f, 9.0f, 270.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Ship, XMMatrixRotationX(-0.05f) * XMMatrixRotationY(-0.8f) * XMMatrixScaling(2.5f, 2.5f, 2.5f) * XMMatrixTranslation(345.0f, 7.0f, -330.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Ship, XMMatrixRotationY(1.9f) * XMMatrixScaling(2.f, 2.f, 2.f) * XMMatrixTranslation(-265.0f, -5.0f, -400.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Island, XMMatrixRotationX(-0.05f) * XMMatrixRotationZ(-0.08) * XMMatrixRotationY(0.1) * XMMatrixScaling(5, 5, 5) * XMMatrixTranslation(-90.0f, 0.0f, -55.0f),  MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixScaling(5.f, 5.f, 5.f) * XMMatrixTranslation(-110.0f, -0.0f, -100.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Pier,  XMMatrixRotationX(-0.05f) * XMMatrixRotationY(3.2f) * XMMatrixScaling(3, 3, 3) * XMMatrixTranslation(-107.0f, 1.5f, -147.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Ocean, XMMatrixRotationY(1.5f) * XMMatrixScaling(250, 250, 250) * XMMatrixTranslation(0.0f, -5.0f, 0.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Fence, XMMatrixRotationX(-0.05f) * XMMatrixRotationZ(-0.08) * XMMatrixRotationY(0.1) * XMMatrixScaling(5, 5, 5) * XMMatrixTranslation(-90.0f, 0.0f, -55.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Shovel, XMMatrixScaling(10.f, 10.f, 10.f) * XMMatrixTranslation(-115.0f, 10.0f, -14.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Island, XMMatrixRotationX(-0.1f) * XMMatrixRotationY(1.5f) * XMMatrixScaling(8, 8, 8) * XMMatrixTranslation(-370.0f, -2.0f, 155.0f),  MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixRotationY(1.0f) * XMMatrixScaling(7.f, 7.f, 7.f) * XMMatrixTranslation(-472.0f, -4.f, 150.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixRotationY(-1.0f) * XMMatrixScaling(4.f, 4.f, 4.f) * XMMatrixTranslation(-459.0f, -1.5f, 120.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Island, XMMatrixRotationX(-0.08f) * XMMatrixRotationZ(0.04) * XMMatrixRotationY(2.5f) * XMMatrixScaling(7.f, 7.f, 7.f) * XMMatrixTranslation(360.0f, -3.7f, 150.0f),  MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixRotationY(-1.0f) * XMMatrixScaling(7.f, 7.f, 7.f) * XMMatrixTranslation(320.0f, -3.7f, 135.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixRotationY(-1.5f) * XMMatrixScaling(8.f, 8.f, 8.f) * XMMatrixTranslation(300.0f, -3.7f, 205.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixRotationY(1.5f) * XMMatrixScaling(5.f, 5.f, 5.f) * XMMatrixTranslation(395.0f, 2.5f, 85.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixRotationY(0.3f) * XMMatrixScaling(9.f, 9.f, 9.f) * XMMatrixTranslation(420.0f, -3.f, 90.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Island, XMMatrixRotationX(-0.07f) * XMMatrixRotationY(-1.2f) * XMMatrixScaling(10.f, 10.f, 10.f) * XMMatrixTranslation(160.0f, -1.f, -310.0f),  MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixRotationY(2.2f) * XMMatrixScaling(8.f, 8.f, 8.f) * XMMatrixTranslation(230.0f, -4.5f, -400.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixRotationY(-1.7f) * XMMatrixScaling(7.f, 7.f, 7.f) * XMMatrixTranslation(255.0f, -4.f, -380.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Pier,  XMMatrixRotationX(0.05f) * XMMatrixRotationY(3.0f) * XMMatrixScaling(3.f, 3.f, 3.f) * XMMatrixTranslation(250.0f, -6.f, -435.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Shovel, XMMatrixScaling(10.f, 10.f, 10.f) * XMMatrixTranslation(235.0f, 0.5f, -400.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Fence, XMMatrixRotationX(0.1f) * XMMatrixRotationZ(-0.05) * XMMatrixRotationY(2.9) * XMMatrixScaling(4, 4, 4) * XMMatrixTranslation(248.0f, -1.f, -358.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Island, XMMatrixRotationX(-0.07f) * XMMatrixRotationZ(-0.08) * XMMatrixRotationY(2.5f) * XMMatrixScaling(7.f, 7.f, 7.f) * XMMatrixTranslation(-200.0f, -1.5f, -310.0f),  MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixRotationY(1.2f) * XMMatrixScaling(9.f, 9.f, 9.f) * XMMatrixTranslation(-240.0f, -1.5f, -260.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixRotationY(-1.5f) * XMMatrixScaling(10.f, 10.f, 10.f) * XMMatrixTranslation(-145.0f, -1.5f, -370.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixRotationY(0.4f) * XMMatrixScaling(7.f, 7.f, 7.f) * XMMatrixTranslation(-180.0f, -5.5f, -380.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Island, XMMatrixRotationX(-0.05f) * XMMatrixRotationZ(-0.07) * XMMatrixRotationY(-0.1) * XMMatrixScaling(11.f, 11.f, 11.f) * XMMatrixTranslation(190.0f, 0.0f, -55.0f),  MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixRotationY(1.2f) * XMMatrixScaling(9.f, 9.f, 9.f) * XMMatrixTranslation(110.0f, 6.0f, -90.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Pier,  XMMatrixRotationX(0.1f) * XMMatrixRotationY(-1.5f) * XMMatrixScaling(3.5, 3.5, 3.5) * XMMatrixTranslation(60.0f, -1.0f, -90.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Shovel, XMMatrixScaling(10.f, 10.f, 10.f) * XMMatrixTranslation(112.0f, 11.0f, -92.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Island, XMMatrixRotationX(-0.03f) * XMMatrixRotationZ(-0.03) * XMMatrixRotationY(-1.2) * XMMatrixScaling(13.f, 13.f, 13.f) * XMMatrixTranslation(-50.0f, 3.0f, 225.0f),  MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixRotationY(2.0f) * XMMatrixScaling(10.f, 10.f, 10.f) * XMMatrixTranslation(-185.0f, 4.0f, 205.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixRotationY(-0.4f) * XMMatrixScaling(5.f, 5.f, 5.f) * XMMatrixTranslation(40.0f, 3.0f, 125.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Tree, XMMatrixRotationY(-1.1f) * XMMatrixScaling(7.f, 7.f, 7.f) * XMMatrixTranslation(20.0f, 5.5f, 115.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Pier,  XMMatrixRotationX(-0.1f) * XMMatrixRotationY(2.f) * XMMatrixScaling(4.5, 4.5, 4.5) * XMMatrixTranslation(95.0f, 10.0f, 125.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Shovel, XMMatrixScaling(10.f, 10.f, 10.f) * XMMatrixTranslation(26.5f, 11.0f, 113.0f), MaterialType::LitShadowCasting },
        { Sample::ContentOption::Skybox, XMMatrixScaling(SPHERE_RADIUS, SPHERE_RADIUS, SPHERE_RADIUS), MaterialType::UnlitNonShadowCasting },
    };
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_tlasSize(0),
    m_blasTotalSize(0),
    m_blasPostBuildSize(0),
    m_postbuildInfoCount(0),
    m_buildTLAS(true),
    m_buildBLAS(true),
    m_showBuildMessage(false),
    m_showBLASTimeGpu(true),
    m_useGoodContent(true),
    m_useDefaultTLASValues(false),
    m_useDefaultBLASValues(false),
    m_currentMenu(OptionMenu::TLAS)
{
    // Use gamma-correct rendering.
    // Only raytracing, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN, 2, DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD | DX::DeviceResources::c_EnableDXR);

    m_tlasOptions[TLASOption::FP16BoxInflationThreshold] = TLASOptions::Option(0.0f, 0.0f, 1.5f, 0.01f, L"FP16BoxInflationThreshold");
    m_tlasOptions[TLASOption::TreeletFirstLevel] = TLASOptions::Option(uint8_t(0), uint8_t(0), uint8_t(3), uint8_t(1), L"TreeletFirstLevel");
    m_tlasOptions[TLASOption::TreeletNumLevels] = TLASOptions::Option(uint8_t(0), uint8_t(0), uint8_t(64), uint8_t(1), L"TreeletNumLevels");
    m_tlasOptions[TLASOption::TreeletStepSizeBetweenRounds] = TLASOptions::Option(uint8_t(0), uint8_t(0), uint8_t(3), uint8_t(1), L"TreeletStepSizeBetweenRounds");
    m_tlasOptions[TLASOption::TreeletRoundsLimiter] = TLASOptions::Option(uint8_t(0), uint8_t(0), uint8_t(3), uint8_t(1), L"TreeletRoundsLimiter");
    m_tlasOptions[TLASOption::NumExtraElementsForInstanceSplitting] = TLASOptions::Option(0U, 0U, 5000U, 1U, L"NumExtraElementsForInstanceSplitting");
    m_tlasOptions[TLASOption::InstanceSplit16Fraction] = TLASOptions::Option(0.2, 0.f, 1.0f, 0.01f, L"InstanceSplit16Fraction");
#if _GXDK_VER >= 0x63360C57 // This option is only available in March 2024 GDK or later
    m_tlasOptions[TLASOption::IsolateSARatio] = TLASOptions::Option(0.0f, -1.0f, 1.0f, 0.01f, L"IsolateSARatio");
#endif
    std::vector<const wchar_t*> enumNamesTLASBuildMode = { L"Fast Trace", L"Fast Build", L"Minimize Memory" };
    m_tlasOptions[TLASOption::BuildMode] = TLASOptions::Option(BuildMode::FastTrace, BuildMode::Count, enumNamesTLASBuildMode, L"Build Mode");

    m_blasOptions[BLASOption::OfflineBuild] = BLASOptions::Option(false, L"Use Offline Builder");
    m_blasOptions[BLASOption::BatchedBuild] = BLASOptions::Option(true, L"Use Batched Builds");
    std::vector<const wchar_t*> enumNamesBLASBuildMode = { L"Fast Trace", L"Fast Build", L"Minimize Memory" };
    m_blasOptions[BLASOption::BuildMode] = BLASOptions::Option(BuildMode::FastTrace, BuildMode::Count, enumNamesBLASBuildMode, L"Build Mode");
    m_blasOptions[BLASOption::QuadJoinThreshold] = BLASOptions::Option(false, L"Enable Quad Join (Offline Only)");
    m_blasOptions[BLASOption::FP16BoxInflationThreshold] = BLASOptions::Option(0.0f, 0.0f, 1.5f, 0.01f, L"FP16BoxInflationThreshold");
    m_blasOptions[BLASOption::TriangleSplitFactor] = BLASOptions::Option(1.0f, 1.0f, 3.0f, 0.1f, L"TriangleSplitFactor (Offline Only)");
    m_blasOptions[BLASOption::KDOPTesselationFactorN] = BLASOptions::Option(uint8_t(0), uint8_t(0), uint8_t(8), uint8_t(1), L"KDOPTesselationFactorN");

    std::vector<const wchar_t*> enumNamesDebugMode = { L"None", L"Iterations", L"Shadow Iterations", L"Time", L"Geometry Index", L"Instances"};
    m_debugOptions[DebugOption::Mode] = DebugOptions::Option(DebugMode::None, DebugMode::Count, enumNamesDebugMode, L"Debug Mode");
    m_debugOptions[DebugOption::IterationScale] = DebugOptions::Option(256.0f, 1.0f, 500.0f, 1.0f, L"Iteration Scale");
    m_debugOptions[DebugOption::TimeScale] = DebugOptions::Option(25000.0f, 500.0f, 45000.0f, 500.0f, L"Time Scale");

    std::vector<const wchar_t*> enumNamesContentModeShip = { L"Good", L"Bad", L"Off" };
    m_contentOptions[ContentOption::Ship] = ContentOptions::Option(static_cast<uint32_t>(ContentMode::Good), static_cast<uint32_t>(ContentMode::Count), enumNamesContentModeShip, L"Ship");
    std::vector<const wchar_t*> enumNamesContentModeIsland = { L"On", L"On", L"Off" }; // there is only one version of the island
    m_contentOptions[ContentOption::Island] = ContentOptions::Option(static_cast<uint32_t>(ContentMode::Good), static_cast<uint32_t>(ContentMode::Count), enumNamesContentModeIsland, L"Island");
    std::vector<const wchar_t*> enumNamesContentModeTree = { L"Good", L"Bad", L"Off" };
    m_contentOptions[ContentOption::Tree] = ContentOptions::Option(static_cast<uint32_t>(ContentMode::Good), static_cast<uint32_t>(ContentMode::Count), enumNamesContentModeTree, L"Tree");
    std::vector<const wchar_t*> enumNamesContentModePier = { L"Good", L"Bad", L"Off" };
    m_contentOptions[ContentOption::Pier] = ContentOptions::Option(static_cast<uint32_t>(ContentMode::Good), static_cast<uint32_t>(ContentMode::Count), enumNamesContentModePier, L"Pier");
    std::vector<const wchar_t*> enumNamesContentModeOcean = { L"Good", L"Bad", L"Off" };
    m_contentOptions[ContentOption::Ocean] = ContentOptions::Option(static_cast<uint32_t>(ContentMode::Good), static_cast<uint32_t>(ContentMode::Count), enumNamesContentModeOcean, L"Ocean");
    std::vector<const wchar_t*> enumNamesContentModeFence = { L"Good", L"Bad", L"Off" };
    m_contentOptions[ContentOption::Fence] = ContentOptions::Option(static_cast<uint32_t>(ContentMode::Good), static_cast<uint32_t>(ContentMode::Count), enumNamesContentModeFence, L"Fence");
    std::vector<const wchar_t*> enumNamesContentModeShovel = { L"Good", L"Bad", L"Off" };
    m_contentOptions[ContentOption::Shovel] = ContentOptions::Option(static_cast<uint32_t>(ContentMode::Good), static_cast<uint32_t>(ContentMode::Count), enumNamesContentModeShovel, L"Shovel");
    std::vector<const wchar_t*> enumNamesContentModeSkybox = { L"Good", L"Bad", L"Good" }; // turning "off" the skybox is the same as the good version
    m_contentOptions[ContentOption::Skybox] = ContentOptions::Option(static_cast<uint32_t>(ContentMode::Good), static_cast<uint32_t>(ContentMode::Count), enumNamesContentModeSkybox, L"Skybox");
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_deviceResources->WaitForOrigin();

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    // start building the BLASes now that the message has been shown
    if (m_showBuildMessage)
    {
        m_showBuildMessage = false;
        m_buildBLAS = true;
        m_buildTLAS = true;
    }

    auto pad = m_gamePad->GetState(DirectX::GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);
        m_camera.Update(elapsedTime, pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        static float heldTime = 0.0f;
        static float heldReset = 0.0f;
        bool incrementMenu = false;
        bool decrementMenu = false;

        if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
        {
            heldTime = 0.0f;
            heldReset = 0.0f;
            incrementMenu = true;
        }
        else if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::HELD)
        {
            heldTime += elapsedTime;
            if (heldTime > 0.5f)
            {
                heldReset = std::min(heldReset + 0.05f, 0.45f);
                heldTime = heldReset;
                incrementMenu = true;
            }
        }
        else if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
        {
            heldTime = 0.0f;
            heldReset = 0.0f;
            decrementMenu = true;
        }
        else if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::HELD)
        {
            heldTime += elapsedTime;
            if (heldTime > 0.5f)
            {
                heldReset = std::min(heldReset + 0.05f, 0.45f);
                heldTime = heldReset;
                decrementMenu = true;
            }
        }

        if (incrementMenu)
        {
            switch (m_currentMenu)
            {
            case OptionMenu::BLAS:
                m_blasOptions.Increment();
                break;
            case OptionMenu::TLAS:
                m_tlasOptions.Increment();
                break;
            case OptionMenu::Debug:
                m_debugOptions.Increment();
                break;
            case OptionMenu::Content:
            {
                m_contentOptions.Increment();
                m_buildTLAS = true;
                break;
            }
            default:
                throw std::exception("unrecognized menu");
                break;
            }
        }

        if (decrementMenu)
        {
            switch (m_currentMenu)
            {
            case OptionMenu::BLAS:
                m_blasOptions.Decrement();
                break;
            case OptionMenu::TLAS:
                m_tlasOptions.Decrement();
                break;
            case OptionMenu::Debug:
                m_debugOptions.Decrement();
                break;
            case OptionMenu::Content:
            {
                m_contentOptions.Decrement();
                m_buildTLAS = true;
                break;
            }
            default:
                throw std::exception("unrecognized menu");
                break;
            }
        }

        if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
        {
            switch (m_currentMenu)
            {
            case OptionMenu::BLAS:
                m_blasOptions.SelectPrevious();
                break;
            case OptionMenu::TLAS:
                m_tlasOptions.SelectPrevious();
                break;
            case OptionMenu::Debug:
                m_debugOptions.SelectPrevious();
                break;
            case OptionMenu::Content:
                m_contentOptions.SelectPrevious();
                break;
            default:
                throw std::exception("unrecognized menu");
                break;
            }
        }

        if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
        {
            switch (m_currentMenu)
            {
            case OptionMenu::BLAS:
                m_blasOptions.SelectNext();
                break;
            case OptionMenu::TLAS:
                m_tlasOptions.SelectNext();
                break;
            case OptionMenu::Debug:
                m_debugOptions.SelectNext();
                break;
            case OptionMenu::Content:
                m_contentOptions.SelectNext();
                break;
            default:
                throw std::exception("unrecognized menu");
                break;
            }
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_currentMenu = static_cast<OptionMenu>((static_cast<uint32_t>(m_currentMenu) + 1) % static_cast<uint32_t>(OptionMenu::Count));
        }

        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            if (m_currentMenu == OptionMenu::Content)
            {
                m_useGoodContent = !m_useGoodContent;

                // Set all content options to good/bad
                for (uint32_t i = 0; i < ContentOption::Count; i++)
                {
                    m_contentOptions[i].Set(m_useGoodContent ? static_cast<uint32_t>(ContentMode::Good) : static_cast<uint32_t>(ContentMode::Bad));
                }

                m_buildTLAS = true;
            }
            else if (m_currentMenu == OptionMenu::TLAS)
            {
                m_useDefaultTLASValues = !m_useDefaultTLASValues;
                m_tlasOptions[TLASOption::FP16BoxInflationThreshold].SetIsDisabled(m_useDefaultTLASValues);
                m_tlasOptions[TLASOption::TreeletFirstLevel].SetIsDisabled(m_useDefaultTLASValues);
                m_tlasOptions[TLASOption::TreeletNumLevels].SetIsDisabled(m_useDefaultTLASValues);
                m_tlasOptions[TLASOption::TreeletStepSizeBetweenRounds].SetIsDisabled(m_useDefaultTLASValues);
                m_tlasOptions[TLASOption::TreeletRoundsLimiter].SetIsDisabled(m_useDefaultTLASValues);
                m_tlasOptions[TLASOption::NumExtraElementsForInstanceSplitting].SetIsDisabled(m_useDefaultTLASValues);
                m_tlasOptions[TLASOption::InstanceSplit16Fraction].SetIsDisabled(m_useDefaultTLASValues);
#if _GXDK_VER >= 0x63360C57 // This option is only available in March 2024 GDK or later
                m_tlasOptions[TLASOption::IsolateSARatio].SetIsDisabled(m_useDefaultTLASValues);
#endif
                m_buildTLAS = true;
            }
            else if (m_currentMenu == OptionMenu::BLAS)
            {
                m_useDefaultBLASValues = !m_useDefaultBLASValues;
                m_blasOptions[BLASOption::QuadJoinThreshold].SetIsDisabled(m_useDefaultBLASValues);
                m_blasOptions[BLASOption::FP16BoxInflationThreshold].SetIsDisabled(m_useDefaultBLASValues);
                m_blasOptions[BLASOption::TriangleSplitFactor].SetIsDisabled(m_useDefaultBLASValues);
                m_blasOptions[BLASOption::KDOPTesselationFactorN].SetIsDisabled(m_useDefaultBLASValues);
                m_buildBLAS = true;
                m_buildTLAS = true; // requires also rebuilding the TLAS
            }
        }

        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            // for offline builds delay building by one frame to show a message first since it's slow
            if (m_blasOptions[BLASOption::OfflineBuild].GetBool())
            {
                m_showBuildMessage = true;
                m_showBLASTimeGpu = false;
            }
            else
            {
                m_buildBLAS = true;
                m_buildTLAS = true; // requires also rebuilding the TLAS
                m_showBLASTimeGpu = true;
            }
        }

        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            m_buildTLAS = true;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    // This leaves the backbuffer in present state
    m_deviceResources->Prepare(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT);


    auto commandList = m_deviceResources->GetCommandList();
    m_gpuTimer->BeginFrame(commandList);
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    auto const size = m_deviceResources->GetOutputSize();
    auto displayWidth = static_cast<uint32_t>(size.right - size.left);
    auto displayHeight = static_cast<uint32_t>(size.bottom - size.top);
    bool debugMode = m_debugOptions[DebugOption::Mode].GetUint() != DebugMode::None;
    bool waitForBLASSize = m_buildBLAS;

    if (m_buildBLAS)
    {
        m_gpuTimer->Start(commandList, GpuTimerIndex::BLASBuild);
        m_cpuTimer->Start(CpuTimerIndex::BLASBuild);
        BuildBLASesFromGLTF();
        m_cpuTimer->Stop(CpuTimerIndex::BLASBuild);
        m_gpuTimer->Stop(commandList, GpuTimerIndex::BLASBuild);
        m_buildBLAS = false;
    }

    if (m_buildTLAS)
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"TLAS rebuild");

        m_gpuTimer->Start(commandList, GpuTimerIndex::TLASBuild);
        BuildTLASFromGLTF();
        m_gpuTimer->Stop(commandList, GpuTimerIndex::TLASBuild);
        m_buildTLAS = false;
    }

    // Update scene constants
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Update scene constants");

        auto cbUploadMem = m_graphicsMemory->Allocate(sizeof(SceneConstants));
        SceneConstants& constants = *static_cast<SceneConstants*>(cbUploadMem.Memory());

        XMMATRIX camView = m_camera.GetView();
        XMMATRIX camProj = m_camera.GetProjection();
        XMVECTOR camPos = m_camera.GetPosition();
        XMMATRIX worldViewProjection = camView * camProj;

        constants.projectionViewWorld = XMMatrixInverse(nullptr, worldViewProjection);
        XMStoreFloat3(&constants.cameraWorldPos, camPos);
        constants.rayMaxLength = 5000.0f;
        constants.lightDiffuseColor = { 2.0f, 2.0f, 2.0f, 1.0f };
        constants.lightAmbientColor = { 0.2f, 0.2f, 0.2f, 1.0f };
        constants.lightPosition = { 800.f, 1250.f, -1500.f };

        constants.invScreenDimensions = { 1.0f / displayWidth, 1.0f / displayHeight };

        // Copy upload data to constant buffer
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_sceneConstants.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->ResourceBarrier(1, &barrier);

        commandList->CopyBufferRegion(m_sceneConstants.Get(), 0, cbUploadMem.Resource(), cbUploadMem.ResourceOffset(), cbUploadMem.Size());
    }

    // Update debug constants
    if (debugMode)
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Update debug constants");

        auto cbUploadMem = m_graphicsMemory->Allocate(sizeof(DebugConstants));
        DebugConstants& constants = *static_cast<DebugConstants*>(cbUploadMem.Memory());

        switch (m_debugOptions[DebugOption::Mode].GetUint())
        {
        case DebugMode::Iterations:
            constants.heatmapScale = m_debugOptions[DebugOption::IterationScale].GetFloat();
            constants.mode = DEGUG_MODE_ITERATIONS;
            break;
        case DebugMode::ShadowIterations:
            constants.heatmapScale = m_debugOptions[DebugOption::IterationScale].GetFloat();
            constants.mode = DEBUG_MODE_ITERATIONS_SHADOW;
            break;
        case DebugMode::Time:
            constants.heatmapScale = m_debugOptions[DebugOption::TimeScale].GetFloat();
            constants.mode = DEBUG_MODE_TIME;
            break;
        case DebugMode::GeometryIndex:
            constants.mode = DEBUG_MODE_GEOMETRY_INDEX;
            break;
        case DebugMode::Instances:
            constants.mode = DEBUG_MODE_INSTANCES;
            break;
        }

        // Copy upload data to constant buffer
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_debugConstants.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->ResourceBarrier(1, &barrier);

        commandList->CopyBufferRegion(m_debugConstants.Get(), 0, cbUploadMem.Resource(), cbUploadMem.ResourceOffset(), cbUploadMem.Size());
    }

    // Raytrace the scene
    {
        m_gpuTimer->Start(commandList, GpuTimerIndex::Raytrace);
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Raytrace");
        D3D12_RESOURCE_BARRIER barriers[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_sceneConstants.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
            CD3DX12_RESOURCE_BARRIER::Transition(m_rtOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_debugConstants.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
        };
        commandList->ResourceBarrier(debugMode ? ARRAYSIZE(barriers) : ARRAYSIZE(barriers) - 1, barriers);

        ID3D12DescriptorHeap* heaps[] = { m_srvHeap->Heap(), m_samplerHeap->Heap()};
        commandList->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);

        commandList->SetComputeRootSignature(m_rootSignatureRT.Get());
        commandList->SetComputeRootConstantBufferView(RootSignature::Constants, m_sceneConstants->GetGPUVirtualAddress());
        commandList->SetComputeRootShaderResourceView(RootSignature::TLAS, m_TLAS->GetGPUVirtualAddress());
        commandList->SetComputeRootDescriptorTable(RootSignature::MeshInfo, m_srvHeap->GetGpuHandle(DescriptorIndex::MeshInfo));
        commandList->SetComputeRootDescriptorTable(RootSignature::RTOutput, m_srvHeap->GetGpuHandle(DescriptorIndex::RTOutputUAV));
        commandList->SetComputeRootDescriptorTable(RootSignature::Skybox, m_srvHeap->GetGpuHandle(DescriptorIndex::Skybox));

        if (debugMode)
        {
            commandList->SetComputeRootDescriptorTable(RootSignature::Heatmap, m_srvHeap->GetGpuHandle(DescriptorIndex::HeatmapTexture));
            commandList->SetComputeRootConstantBufferView(RootSignature::DebugConstants, m_debugConstants->GetGPUVirtualAddress());
        }

        commandList->SetPipelineState(debugMode ? m_pipelineStateRTDebug.Get() : m_pipelineStateRT.Get());

        commandList->Dispatch(AlignUp(displayWidth, THREAD_GROUP_X) / THREAD_GROUP_X, AlignUp(displayHeight, THREAD_GROUP_Y) / THREAD_GROUP_Y, 1);
        m_gpuTimer->Stop(commandList, GpuTimerIndex::Raytrace);
    }

    // Copy from raytracing output buffer to backbuffer
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Copy to backbuffer");

        D3D12_RESOURCE_BARRIER barriers[2] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_rtOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST)
        };
        commandList->ResourceBarrier(ARRAYSIZE(barriers), barriers);

        commandList->CopyResource(m_deviceResources->GetRenderTarget(), m_rtOutput.Get());

        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &barrier);
    }

    RenderHUD();

    PIXEndEvent(commandList);
    m_gpuTimer->EndFrame(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();

    if (waitForBLASSize)
    {
        m_deviceResources->WaitForGpu();
        GetBLASPostBuildSize();
    }
}

void Sample::RenderHUD()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"RenderHUD");
    m_hudBatch->Begin(commandList);

    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &rtvDescriptor, false, nullptr);

    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(1920, 1080);
    const int boxPadding = 10;
    constexpr size_t textLength = 1024;
    wchar_t topText[textLength] = {};
    wchar_t optionsText[textLength] = {};

    switch (m_currentMenu)
    {
    case OptionMenu::BLAS:
        m_blasOptions.Print(optionsText, textLength, L"BLAS Options");
        break;
    case OptionMenu::TLAS:
        m_tlasOptions.Print(optionsText, textLength, L"TLAS Options");
        break;
    case OptionMenu::Debug:
        m_debugOptions.Print(optionsText, textLength, L"Debug Options");
        break;
    case OptionMenu::Content:
        m_contentOptions.Print(optionsText, textLength, L"Content Options");
        break;
    default:
        throw std::exception("unrecognized menu option");
        break;
    }

    float blasTime = m_showBLASTimeGpu ? m_gpuTimer->GetLastUpdatedMS(GpuTimerIndex::BLASBuild) : static_cast<float>(m_cpuTimer->GetElapsedMS(CpuTimerIndex::BLASBuild));
    swprintf_s(
        topText,
        textLength,
        L"DXR Content\nRaytrace Time: %0.3f ms\nTLAS Build Time: %0.3f ms\nTLAS Size: %lld bytes\nBLAS Build Time: %0.3f ms\nBLAS Prebuild Total Size: %0.4f MB\nBLAS Postbuild Total Size: %0.4f MB\n%ls",
        m_gpuTimer->GetElapsedMS(GpuTimerIndex::Raytrace),
        m_gpuTimer->GetLastUpdatedMS(GpuTimerIndex::TLASBuild),
        m_tlasSize,
        blasTime,
        m_blasTotalSize / 1000000.f,
        m_blasPostBuildSize / 1000000.f,
        optionsText);

    auto textMeasure = m_font->MeasureString(topText);
    int textWidth = static_cast<int>(XMVectorGetX(textMeasure));
    int textHeight = static_cast<int>(XMVectorGetY(textMeasure));
    auto const grayTextureHandle = m_srvHeap->GetGpuHandle(DescriptorIndex::GrayTexture);

    m_hudBatch->Draw(
        grayTextureHandle,
        { 1, 1 },
        RECT{ safe.left - boxPadding, safe.top - boxPadding, safe.left + textWidth + boxPadding, safe.top + textHeight + boxPadding });

    m_font->DrawString(m_hudBatch.get(), topText, XMFLOAT2(float(safe.left), float(safe.top)),Colors::DarkKhaki);

    const wchar_t* bMenuText = m_currentMenu == OptionMenu::Content ? L"[B] Toggle Good/Bad Content\n" : m_currentMenu == OptionMenu::TLAS ? L"[B] Toggle Use Default Xbox TLAS Options\n" : m_currentMenu == OptionMenu::BLAS ? L"[B] Toggle Use Default Xbox BLAS Options\n" : L"";

    wchar_t bottomText[textLength];
    swprintf_s(
        bottomText,
        textLength,
        L"[A] Switch Options Menu\n"\
        "%ls"\
        "[X] Rebuild BLASes\n"\
        "[Y] Rebuild TLAS\n"\
        "[DPAD] Up/Down: Select an option\n"\
        "[DPAD] Left/Right: Modify option value",
        bMenuText);

    textMeasure = m_font->MeasureString(bottomText);
    textWidth = static_cast<int>(XMVectorGetX(textMeasure)) + 20; // pad to account for width of buttons
    textHeight = static_cast<int>(XMVectorGetY(textMeasure));

    m_hudBatch->Draw(
        grayTextureHandle,
        { 1, 1 },
        RECT{ safe.left, safe.bottom - textHeight - boxPadding, safe.left + textWidth, safe.bottom + boxPadding });

    DX::DrawControllerString(m_hudBatch.get(), m_font.get(), m_ctrlFont.get(), bottomText, XMFLOAT2(float(safe.left), float(safe.bottom - textHeight)), DirectX::Colors::DarkKhaki);

    if (m_showBuildMessage)
    {
        const wchar_t* message = L"Building Acceleration Structures";
        textMeasure = m_fontBold->MeasureString(message);
        textWidth = static_cast<int>(XMVectorGetX(textMeasure));
        textHeight = static_cast<int>(XMVectorGetY(textMeasure));
        int textTop = (safe.bottom - safe.top) / 2 + safe.top - textHeight / 2;
        int textLeft = (safe.right - safe.left) / 2 + safe.left - textWidth / 2;
        m_hudBatch->Draw(
            grayTextureHandle,
            { 1, 1 },
            RECT{ textLeft - boxPadding, textTop - boxPadding, textLeft + textWidth + boxPadding, textTop + textHeight + boxPadding });
        m_fontBold->DrawString(m_hudBatch.get(), message, XMFLOAT2(float(textLeft), float(textTop)), DirectX::Colors::DarkKhaki);
    }

    m_hudBatch->End();
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
    m_gamePadButtons.Reset();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());
    m_cpuTimer = std::make_unique<DX::CPUTimer>();

    // Load GLTF scenes
    std::vector<std::unique_ptr<glTF::Asset>> assets(_countof(c_gltfPaths));
    for (size_t i = 0; i < assets.size(); ++i)
    {
        assets[i] = std::make_unique<glTF::Asset>(c_gltfPaths[i]);
    }

    size_t numGltfMeshes = 0;
    size_t numTextures = 0;
    size_t numSamplers = 0;
    uint32_t sceneIndex = 0;
    m_scenes.resize(assets.size());
    for (auto& asset : assets)
    {
        numTextures += asset->m_textures.size();
        numSamplers += asset->m_samplers.size();
        for (const auto& mesh : asset->m_meshes)
        {
            numGltfMeshes += mesh.primitives.size();
        }

        m_scenes[sceneIndex] = std::make_unique<Scene>();
        m_scenes[sceneIndex]->m_uniqueModels.resize(asset->m_meshes.size());
        sceneIndex++;
    }


    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Create descriptor heap.
    m_srvHeap = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        DescriptorIndex::Count + numTextures + numGltfMeshes * 2);

    m_samplerHeap = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        numSamplers);

    // Upload textures to GPU.
    m_textureFactory = std::make_unique<EffectTextureFactory>(device, resourceUpload, m_srvHeap->Heap());

    // Upload skybox texture
    m_textureFactory->CreateTexture(L"kloofendal_48d_partly_cloudy_puresky_4k.png", DescriptorIndex::Skybox);

    uint32_t texOffset = DescriptorIndex::Count;
    uint32_t sampOffset = 0;
    std::vector<uint32_t> texOffsets(_countof(c_gltfPaths));
    std::vector<uint32_t> sampOffsets(_countof(c_gltfPaths));
    for (size_t assetIndex = 0; assetIndex < assets.size(); ++assetIndex)
    {
        const auto& asset = assets[assetIndex];
        texOffsets[assetIndex] = texOffset;
        for (auto& tex : asset->m_textures)
        {
            // remove directory from file name since all files are copied in the root path
            auto filename = std::filesystem::path(tex.source->path).filename().wstring();
            m_textureFactory->CreateTexture(filename.c_str(), static_cast<int>(texOffset));
            ++texOffset;
        }

        sampOffsets[assetIndex] = sampOffset;
        for (size_t samplerIndex = 0; samplerIndex < asset->m_samplers.size(); ++samplerIndex)
        {
            D3D12_SAMPLER_DESC samp = {};
            samp.Filter = asset->m_samplers[samplerIndex].filter;
            samp.AddressU = asset->m_samplers[samplerIndex].wrapS;
            samp.AddressV = asset->m_samplers[samplerIndex].wrapT;
            samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samp.MinLOD = 0;
            samp.MaxLOD = D3D12_FLOAT32_MAX;
            samp.MipLODBias = 0.0f;
            samp.MaxAnisotropy = 1;

            device->CreateSampler(&samp, m_samplerHeap->GetCpuHandle(sampOffset));
            ++sampOffset;
        }
    }

    std::vector<MeshInfo> meshInfos(numGltfMeshes);
    uint32_t meshCounter = 0;

    // process GLTF scene
    sceneIndex = 0;
    for (size_t assetIndex = 0; assetIndex < assets.size(); ++assetIndex)
    {
        const auto& asset = assets[assetIndex];
        auto& scene = m_scenes[sceneIndex++];

        std::function<void(size_t, uint32_t, size_t)> perMeshCallback = [&scene, meshCounter](size_t meshIndex, uint32_t meshOffset, size_t numPrimitives)
            {
                ModelData* model = new ModelData();
                model->m_meshInfoOffset = meshCounter + meshOffset;
                scene->m_uniqueModels[meshIndex].reset(model);
                model->meshBuffers.resize(numPrimitives);
            };

        auto& srvHeapRef = m_srvHeap;
        uint32_t assetTexOffset = texOffsets[assetIndex];
        uint32_t assetSampOffset = sampOffsets[assetIndex];
        std::function<void(size_t, uint32_t, size_t, int, const Renderer::Primitive&)> perPrimitiveCallback
            = [&device, &scene, &resourceUpload, meshCounter, &srvHeapRef, &meshInfos, assetTexOffset, assetSampOffset, texOffset, &asset]
            (size_t meshIndex, uint32_t meshOffset, size_t primitiveIndex, int materialIndex, const Renderer::Primitive& outPrim)
            {
                const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
                ModelData& model = *scene->m_uniqueModels[meshIndex];
                auto& meshVbIb = model.meshBuffers[primitiveIndex];

                // upload vertex buffer
                {
                    auto const desc = CD3DX12_RESOURCE_DESC::Buffer(outPrim.VB->size());
                    DX::ThrowIfFailed(device->CreateCommittedResource(
                        &heapProperties,
                        D3D12_HEAP_FLAG_NONE,
                        &desc,
                        D3D12_RESOURCE_STATE_COPY_DEST,
                        nullptr,
                        IID_GRAPHICS_PPV_ARGS(meshVbIb.vb.ReleaseAndGetAddressOf())
                    ));

                    meshVbIb.vb->SetName(L"mesh vb");
                    meshVbIb.vertexStride = outPrim.vertexStride;
                    meshVbIb.vertexCount = outPrim.vertexCount;

                    D3D12_SUBRESOURCE_DATA initData = { outPrim.VB->data(), 0, 0 };
                    resourceUpload.Upload(meshVbIb.vb.Get(), 0, &initData, 1);

                    resourceUpload.Transition(meshVbIb.vb.Get(),
                        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                }

                // upload index buffer
                {
                    auto const desc = CD3DX12_RESOURCE_DESC::Buffer(outPrim.IB->size());
                    DX::ThrowIfFailed(device->CreateCommittedResource(
                        &heapProperties,
                        D3D12_HEAP_FLAG_NONE,
                        &desc,
                        D3D12_RESOURCE_STATE_COPY_DEST,
                        nullptr,
                        IID_GRAPHICS_PPV_ARGS(meshVbIb.ib.ReleaseAndGetAddressOf())
                    ));

                    meshVbIb.ib->SetName(L"mesh ib");
                    meshVbIb.indexFormat = outPrim.index32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
                    meshVbIb.indexCount = outPrim.primCount;

                    D3D12_SUBRESOURCE_DATA initData = { outPrim.IB->data(), 0, 0 };
                    resourceUpload.Upload(meshVbIb.ib.Get(), 0, &initData, 1);

                    resourceUpload.Transition(meshVbIb.ib.Get(),
                        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                }

                // Create SRVs and geometry descs for model resources
                {
                    MeshInfo& meshInfo = meshInfos[meshCounter + meshOffset];
                    uint32_t srvOffset = 2 * (meshCounter + meshOffset) + texOffset;
                    meshInfo.indicesIndex = srvOffset;
                    meshInfo.vbIndex = srvOffset + 1;
                    meshInfo.texIndex = asset->m_materials[static_cast<uint32_t>(materialIndex)].textures[glTF::Material::kBaseColor]->index - 1 + assetTexOffset;
                    meshInfo.samplerIndex = asset->m_materials[static_cast<uint32_t>(materialIndex)].textures[glTF::Material::kBaseColor]->samplerIndex + assetSampOffset;

                    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

                    // Create either a 16-bit or 32-bit index buffer view
                    srvDesc.Format = meshVbIb.indexFormat;
                    srvDesc.Buffer.FirstElement = 0;
                    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
                    srvDesc.Buffer.NumElements = outPrim.primCount;
                    device->CreateShaderResourceView(meshVbIb.ib.Get(), &srvDesc, srvHeapRef->GetCpuHandle(meshInfo.indicesIndex));

                    // Raw buffer for vertex data
                    srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
                    srvDesc.Buffer.FirstElement = 0;
                    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
                    srvDesc.Buffer.NumElements = outPrim.vertexCount * outPrim.vertexStride / 4;
                    device->CreateShaderResourceView(meshVbIb.vb.Get(), &srvDesc, srvHeapRef->GetCpuHandle(meshInfo.vbIndex));


                    meshInfo.vertexStride = outPrim.vertexStride;
                    meshInfo.normalOffset = outPrim.normalOffset;
                    meshInfo.texOffset = outPrim.texCoordOffset;
                }
            };

        meshCounter += IterateMeshes(*asset, perMeshCallback, perPrimitiveCallback);

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

    DX::ThrowIfFailed(CreateStaticBuffer<MeshInfo>(device,
        resourceUpload,
        meshInfos.data(),
        numGltfMeshes,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        m_meshInfoBuffer.ReleaseAndGetAddressOf()));
    m_meshInfoBuffer->SetName(L"Mesh Info");

    // Structured buffer view for mesh info
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.StructureByteStride = sizeof(MeshInfo);
    srvDesc.Buffer.NumElements = static_cast<UINT>(numGltfMeshes);
    device->CreateShaderResourceView(m_meshInfoBuffer.Get(), &srvDesc, m_srvHeap->GetCpuHandle(DescriptorIndex::MeshInfo));

    {
        m_postbuildInfoCount = 0;
        for (auto& scene : m_scenes)
        {
            m_postbuildInfoCount += static_cast<uint32_t>(scene->m_uniqueModels.size());
        }

        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        uint32_t postbuildInfoSize = sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC) * m_postbuildInfoCount;
        auto postBuildDesc = CD3DX12_RESOURCE_DESC::Buffer(postbuildInfoSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &postBuildDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_postbuildInfos.ReleaseAndGetAddressOf())));
        m_postbuildInfos->SetName(L"Post Build Info");

        postBuildDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        auto readbackHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &readbackHeap,
            D3D12_HEAP_FLAG_NONE,
            &postBuildDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_postbuildInfosCpu.ReleaseAndGetAddressOf())));
        m_postbuildInfosCpu->SetName(L"Post Build Info CPU");
    }

    // Create scene constant buffer
    {
        constexpr size_t sceneConstantsSize = sizeof(SceneConstants);
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sceneConstantsSize, D3D12_RESOURCE_FLAG_NONE);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_sceneConstants.ReleaseAndGetAddressOf())));

        m_sceneConstants->SetName(L"Scene Constants");
    }

    // Create debug constant buffer
    {
        constexpr size_t debugConstantsSize = sizeof(DebugConstants);
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(debugConstantsSize, D3D12_RESOURCE_FLAG_NONE);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_debugConstants.ReleaseAndGetAddressOf())));

        m_debugConstants->SetName(L"Debug Constants");
    }

    {
        // Create single pixel texture for text underlay box
        auto const pixelDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
        const CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &pixelDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_grayTexture.ReleaseAndGetAddressOf())));
        m_grayTexture->SetName(L"Gray Texture");

        device->CreateShaderResourceView(m_grayTexture.Get(), nullptr, m_srvHeap->GetCpuHandle(DescriptorIndex::GrayTexture));

        // Upload a single grey pixel to the underlay resource
        const uint8_t color[4] = { 5, 5, 5, 196 };

        D3D12_SUBRESOURCE_DATA data = {};
        data.pData = color;
        data.RowPitch = D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT;
        data.SlicePitch = 0;

        resourceUpload.Upload(m_grayTexture.Get(), 0, &data, 1);
        resourceUpload.Transition(m_grayTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    {
        // Create a heatmap texture
        constexpr uint32_t c_heatmapColors = 10;
        auto const pixelDesc = CD3DX12_RESOURCE_DESC::Tex1D(DXGI_FORMAT_R8G8B8A8_UNORM, c_heatmapColors);
        const CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &pixelDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_heatmapTexture.ReleaseAndGetAddressOf())));
        m_heatmapTexture->SetName(L"Heatmap Texture");

        device->CreateShaderResourceView(m_heatmapTexture.Get(), nullptr, m_srvHeap->GetCpuHandle(DescriptorIndex::HeatmapTexture));

        // Upload heatmap colors
        const uint8_t color[c_heatmapColors * 4] = {
            0, 2, 91, 255,
            0, 108, 251, 255,
            0, 221, 221, 255,
            51 , 221, 0, 255,
            255, 252, 0, 255,
            255, 180, 0, 255,
            255, 104, 0, 255,
            226, 22, 0, 255,
            191, 0, 83, 255,
            145, 0, 65, 255,
        };

        D3D12_SUBRESOURCE_DATA data = {};
        data.pData = color;
        data.RowPitch = D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT;
        data.SlicePitch = 0;

        resourceUpload.Upload(m_heatmapTexture.Get(), 0, &data, 1);
        resourceUpload.Transition(m_heatmapTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }

    // Create root signature and pipeline states
    {
        auto computeShaderBlob = DX::ReadData(L"InlineRTCS.cso");

        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignatureRT.ReleaseAndGetAddressOf())));
        m_rootSignatureRT->SetName(L"RT RootSig");

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_rootSignatureRT.Get();
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateRT.ReleaseAndGetAddressOf())));
        m_pipelineStateRT->SetName(L"InlineRTCS");

        computeShaderBlob = DX::ReadData(L"InlineRTDebugCS.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateRTDebug.ReleaseAndGetAddressOf())));
        m_pipelineStateRTDebug->SetName(L"InlineRTDebugCS");
    }

    // HUD
    auto const backBufferRts = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto const spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
    m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    m_font = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_srvHeap->GetCpuHandle(DescriptorIndex::Font),
        m_srvHeap->GetGpuHandle(DescriptorIndex::Font));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_48_Bold.spritefont");
    m_fontBold = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_srvHeap->GetCpuHandle(DescriptorIndex::FontBold),
        m_srvHeap->GetGpuHandle(DescriptorIndex::FontBold));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_srvHeap->GetCpuHandle(DescriptorIndex::ControllerFont),
        m_srvHeap->GetGpuHandle(DescriptorIndex::ControllerFont));

    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    finished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const size = m_deviceResources->GetOutputSize();
    auto displayWidth = static_cast<uint32_t>(size.right - size.left);
    auto displayHeight = static_cast<uint32_t>(size.bottom - size.top);

    // Set up camera
    m_camera.SetWindow(static_cast<int>(displayWidth), static_cast<int>(displayHeight));
    m_camera.SetProjectionParameters(DirectX::XM_PIDIV4, c_nearClipDistance, c_farClipDistance, true);
    m_camera.SetLookAt(SimpleMath::Vector3(-13.0f, 91.0f, -300.0f), SimpleMath::Vector3(-90.f, 0.f, 10.f));
    m_camera.SetSensitivity(300.0f, 100.0f, 1000.0f, 10.0f);

    D3D12_VIEWPORT hudViewport = { 0, 0, 1920, 1080 };
    m_hudBatch->SetViewport(hudViewport);

    // Create output resource for raytracing
    auto device = m_deviceResources->GetD3DDevice();
    auto const backbufferFormat = m_deviceResources->GetBackBufferFormat();
    auto rtOutputDesc = CD3DX12_RESOURCE_DESC::Tex2D(backbufferFormat, displayWidth, displayHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    const CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &rtOutputDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_GRAPHICS_PPV_ARGS(m_rtOutput.ReleaseAndGetAddressOf())));
    m_rtOutput->SetName(L"Raytracing output");
    CreateUnorderedAccessView(device, m_rtOutput.Get(), m_srvHeap->GetCpuHandle(DescriptorIndex::RTOutputUAV), 0);
}
#pragma endregion

bool Sample::SkipContent(size_t sceneIndex) const
{
    const uint32_t contentIndex = s_sceneCollection[sceneIndex].contentIndex;
    return (((m_contentOptions[contentIndex].GetUint() == ContentMode::Bad && c_contentPairs[contentIndex].badIndex == -1))
        || ((m_contentOptions[contentIndex].GetUint() == ContentMode::Good && c_contentPairs[contentIndex].goodIndex == -1))
        || (m_contentOptions[contentIndex].GetUint() == ContentMode::Off));
}

uint32_t Sample::GetCurrentSceneIndex(size_t sceneIndex) const
{
    const uint32_t contentIndex = s_sceneCollection[sceneIndex].contentIndex;
    return m_contentOptions[contentIndex].GetUint() == ContentMode::Good ? static_cast<uint32_t>(c_contentPairs[contentIndex].goodIndex) : static_cast<uint32_t>(c_contentPairs[contentIndex].badIndex);
}

void Sample::BuildTLASFromGLTF()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto commandList = m_deviceResources->GetCommandList();

    size_t numInstances = 0;
    for (size_t sceneIndex = 0; sceneIndex < _countof(s_sceneCollection); ++sceneIndex)
    {
        // skip content types that are not being rendered
        if (SkipContent(sceneIndex))
        {
            continue;
        }

        numInstances += m_scenes[GetCurrentSceneIndex(sceneIndex)]->m_instances.size();
    }

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    switch (m_tlasOptions[TLASOption::BuildMode].GetUint())
    {
    case BuildMode::FastTrace:
        buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        break;
    case BuildMode::FastBuild:
        buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
        break;
    case BuildMode::MinimizeMemory:
        buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
        break;
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = tlasBuildDesc.Inputs;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = buildFlags;
    topLevelInputs.NumDescs = static_cast<UINT>(numInstances);
    topLevelInputs.pGeometryDescs = nullptr;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    auto instanceDescUploadMem = m_graphicsMemory->Allocate(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numInstances);

    D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs = reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(instanceDescUploadMem.Memory());

    uint32_t instanceCounter = 0;
    for (size_t sceneIndex = 0; sceneIndex < _countof(s_sceneCollection); ++sceneIndex)
    {
        // skip content types that are not being rendered
        if (SkipContent(sceneIndex))
        {
            continue;
        }

        auto& scene = m_scenes[GetCurrentSceneIndex(sceneIndex)];
        for (auto& modelInstance : scene->m_instances)
        {
            auto& uniqueModel = scene->m_uniqueModels[static_cast<uint32_t>(modelInstance->modelIndex)];
            instanceDescs[instanceCounter] = {};
            XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(&instanceDescs[instanceCounter].Transform), modelInstance->world * s_sceneCollection[sceneIndex].transform);
            instanceDescs[instanceCounter].InstanceMask = s_sceneCollection[sceneIndex].materialType == MaterialType::LitShadowCasting ? 1U : 2U;
            instanceDescs[instanceCounter].InstanceID = uniqueModel->m_meshInfoOffset;
            instanceDescs[instanceCounter].InstanceContributionToHitGroupIndex = s_sceneCollection[sceneIndex].materialType == MaterialType::LitShadowCasting ? 1U : 0U;
            instanceDescs[instanceCounter].AccelerationStructure = uniqueModel->m_BLAS->GetGPUVirtualAddress();
            ++instanceCounter;
        }
    }

    // Set instance memory pointer in TLAS description
    tlasBuildDesc.Inputs.InstanceDescs = instanceDescUploadMem.GpuAddress();

    struct
    {
        D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_PARAMS1 params1;
        D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_PARAMS2 params2;
        D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_PARAMS3 params3;
    } options;

    // NOTE: D3D12XboxMakeDefault_* must be used here instead of zero-initializing these structs.
    // Default values are not always zero and may contain special values to instruct the driver to rely on the topLevelInputs.Flags value instead.
    // See the doc pages dedicated to D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_PARAMS* structs for more information and parameter descriptions.
    D3D12XboxMakeDefault_D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_PARAMS1(&options.params1);
    D3D12XboxMakeDefault_D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_PARAMS2(&options.params2);
    D3D12XboxMakeDefault_D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_PARAMS3(&options.params3);

    if (!m_useDefaultTLASValues)
    {
        // Modify options relating to treelet reordering.
        options.params1.TreeletFirstLevel = m_tlasOptions[TLASOption::TreeletFirstLevel].GetByte();
        options.params1.TreeletNumLevels = m_tlasOptions[TLASOption::TreeletNumLevels].GetByte();
        options.params1.TreeletStepSizeBetweenRounds = m_tlasOptions[TLASOption::TreeletStepSizeBetweenRounds].GetByte();
        options.params1.TreeletRoundsLimiter = m_tlasOptions[TLASOption::TreeletRoundsLimiter].GetByte();  // Very expensive

        options.params1.FP16BoxInflationThreshold = m_tlasOptions[TLASOption::FP16BoxInflationThreshold].GetFloat();

        // Increase the memory budget for splitting instances.
        options.params3.NumExtraElementsForInstanceSplitting = m_tlasOptions[TLASOption::NumExtraElementsForInstanceSplitting].GetUint();
        // Modify the percentage of extra memory devoted to splitting instances into 16 pieces instead of 4 pieces.
        options.params3.InstanceSplit16Fraction = m_tlasOptions[TLASOption::InstanceSplit16Fraction].GetFloat();
        // This value allows "isolating" objects in the TLAS whose surface area is larger than a specified ratio of the total scene surface area.
        // These objects are pulled to a separate branch so as to keep tighter bounds for the other branches.
#if _GXDK_VER >= 0x63360C57 // This option is only available in March 2024 GDK or later
        options.params3.IsolateSARatio = m_tlasOptions[TLASOption::IsolateSARatio].GetFloat();
#endif
    }

    D3D12XBOX_XDXR_STREAM_DESC streamDesc;
    streamDesc.pXdxrStreamDesc = &options;
    streamDesc.SizeInBytes = sizeof(options);

    D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_BATCHED_DESC2 desc = {};
    desc.Desc = tlasBuildDesc;
    desc.ExtraOptions = streamDesc;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfoX(&tlasBuildDesc.Inputs,
        &desc.ExtraOptions,
        &topLevelPrebuildInfo);
    DX::ThrowIfFalse(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0, "Illegal TLAS size");
    m_tlasSize = topLevelPrebuildInfo.ResultDataMaxSizeInBytes;

    // Allocate scratch buffer for build
    UINT64 scratchSize = topLevelPrebuildInfo.ScratchDataSizeInBytes;
    const CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_TLASBuildScratch.ReleaseAndGetAddressOf())));
    m_TLASBuildScratch->SetName(L"TLASBuildScratch");

    bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_TLAS.ReleaseAndGetAddressOf())));
    m_TLAS->SetName(L"TLAS");

    // Set resource pointers in TLAS description
    desc.Desc.DestAccelerationStructureData = m_TLAS->GetGPUVirtualAddress();
    desc.Desc.ScratchAccelerationStructureData = m_TLASBuildScratch->GetGPUVirtualAddress();


    commandList->BuildRaytracingAccelerationStructureBatched2X(1, &desc);

}

void Sample::BuildBLASesFromGLTF()
{
    auto commandList = m_deviceResources->GetCommandList();
    auto device = m_deviceResources->GetD3DDevice();
    m_blasTotalSize = 0;

    struct
    {
        D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_PARAMS1 params1;
        D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_PARAMS2 params2;
        D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_PARAMS3 params3;
    } options;

    // NOTE: D3D12XboxMakeDefault_* must be used here instead of zero-initializing these structs.
    // Default values are not always zero and may contain special values to instruct the driver to rely on the topLevelInputs.Flags value instead.
    D3D12XboxMakeDefault_D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_PARAMS1(&options.params1);
    D3D12XboxMakeDefault_D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_PARAMS2(&options.params2);
    D3D12XboxMakeDefault_D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_PARAMS3(&options.params3);

    if (!m_useDefaultBLASValues)
    {
        options.params1.QuadJoinThreshold = m_blasOptions[BLASOption::QuadJoinThreshold].GetBool() ? 5.0f : -1.f;
        options.params1.FP16BoxInflationThreshold = m_blasOptions[BLASOption::FP16BoxInflationThreshold].GetFloat();
        options.params2.TriangleSplitFactor = m_blasOptions[BLASOption::TriangleSplitFactor].GetFloat();
        options.params2.KDOPTesselationFactorN = m_blasOptions[BLASOption::KDOPTesselationFactorN].GetByte();
    }

    D3D12XBOX_XDXR_STREAM_DESC streamDesc;
    streamDesc.pXdxrStreamDesc = &options;
    streamDesc.SizeInBytes = sizeof(options);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    switch (m_blasOptions[BLASOption::BuildMode].GetUint())
    {
    case BuildMode::FastTrace:
        buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        break;
    case BuildMode::FastBuild:
        buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
        break;
    case BuildMode::MinimizeMemory:
        buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
        break;
    }

    static const unsigned DRIVER_HINT_XDXR_FORCE_CPU_BUILDS = 0x0000010a;
    const bool offlineBuild = m_blasOptions[BLASOption::OfflineBuild].GetBool();
    if (offlineBuild)
    {
        device->SetDriverHintX(DRIVER_HINT_XDXR_FORCE_CPU_BUILDS, 1);
    }

    bool batchedBuilds = m_blasOptions[BLASOption::BatchedBuild].GetBool();
    size_t totalUniqueModels = 0;
    for (auto& scene : m_scenes)
    {
        totalUniqueModels += scene->m_uniqueModels.size();
    }

    std::vector <D3D12XBOX_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_BATCHED_DESC2> descs(totalUniqueModels);
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC*> geoDescsAll(totalUniqueModels);
    std::vector<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC> postbuildInfos(totalUniqueModels);
    uint32_t descIndex = 0;
    for (auto& scene : m_scenes)
    {
        for (size_t modelIndex = 0; modelIndex < scene->m_uniqueModels.size(); ++modelIndex)
        {
            size_t totalGeos = scene->m_uniqueModels[modelIndex]->meshBuffers.size();;
            geoDescsAll[descIndex] = new D3D12_RAYTRACING_GEOMETRY_DESC[totalGeos];

            size_t geoCounter = 0;
            for (auto& mesh : scene->m_uniqueModels[modelIndex]->meshBuffers)
            {
                // Create SRVs and geometry descs for model resources
                {
                    geoDescsAll[descIndex][geoCounter] = {};
                    geoDescsAll[descIndex][geoCounter].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
                    geoDescsAll[descIndex][geoCounter].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
                    geoDescsAll[descIndex][geoCounter].Triangles.IndexBuffer = mesh.ib->GetGPUVirtualAddress();
                    geoDescsAll[descIndex][geoCounter].Triangles.IndexCount = mesh.indexCount;
                    geoDescsAll[descIndex][geoCounter].Triangles.IndexFormat = mesh.indexFormat;
                    geoDescsAll[descIndex][geoCounter].Triangles.Transform3x4 = 0;
                    geoDescsAll[descIndex][geoCounter].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
                    geoDescsAll[descIndex][geoCounter].Triangles.VertexCount = mesh.vertexCount;
                    geoDescsAll[descIndex][geoCounter].Triangles.VertexBuffer.StartAddress = mesh.vb->GetGPUVirtualAddress();
                    geoDescsAll[descIndex][geoCounter].Triangles.VertexBuffer.StrideInBytes = mesh.vertexStride;
                }
                geoCounter++;
            }

            // Build bottom-level-acceleration structure (BLAS)
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC  modelBLASBuildDesc = {};
            modelBLASBuildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            modelBLASBuildDesc.Inputs.Flags = buildFlags;
            modelBLASBuildDesc.Inputs.NumDescs = static_cast<UINT>(totalGeos);
            modelBLASBuildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            modelBLASBuildDesc.Inputs.pGeometryDescs = geoDescsAll[descIndex];

            postbuildInfos[descIndex] = {m_postbuildInfos->GetGPUVirtualAddress() + descIndex * sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC), D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE};

            descs[descIndex] = {};
            descs[descIndex].Desc = modelBLASBuildDesc;
            descs[descIndex].ExtraOptions = streamDesc;
            descs[descIndex].pPostbuildInfoDescs = &postbuildInfos[descIndex];
            descs[descIndex].NumPostbuildInfoDescs = 1;

            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
            device->GetRaytracingAccelerationStructurePrebuildInfoX(&modelBLASBuildDesc.Inputs,
                &descs[descIndex].ExtraOptions,
                &bottomLevelPrebuildInfo);
            DX::ThrowIfFalse(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0, "Illegal BLAS size");

            // Allocate scratch buffer for build
            UINT64 blasCreationScratchBytes = bottomLevelPrebuildInfo.ScratchDataSizeInBytes;
            const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
            auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(blasCreationScratchBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(scene->m_uniqueModels[modelIndex]->m_BLASScratch.ReleaseAndGetAddressOf())));
            scene->m_uniqueModels[modelIndex]->m_BLASScratch->SetName(L"BLASScratch");

            // Allocate acceleration structure buffer
            bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(scene->m_uniqueModels[modelIndex]->m_BLAS.ReleaseAndGetAddressOf())));
            scene->m_uniqueModels[modelIndex]->m_BLAS->SetName(L"BLAS");

            m_blasTotalSize += bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes;

            // Set resource pointers in BLAS description
            descs[descIndex].Desc.ScratchAccelerationStructureData = scene->m_uniqueModels[modelIndex]->m_BLASScratch->GetGPUVirtualAddress();
            descs[descIndex].Desc.DestAccelerationStructureData = scene->m_uniqueModels[modelIndex]->m_BLAS->GetGPUVirtualAddress();

            if (!batchedBuilds)
            {
                commandList->BuildRaytracingAccelerationStructureBatched2X(1, &descs[descIndex]);
            }

            descIndex++;
        }
    }

    if (batchedBuilds)
    {
        if (offlineBuild)
        {
            std::thread pool[DX::DeviceResources::MAX_THREADS_BVH_BUILD];
            std::latch latch(DX::DeviceResources::MAX_THREADS_BVH_BUILD);
            std::atomic<unsigned int> buildCounter = 0U;

            m_deviceResources->PrepareBVHCommandLists();

            for (unsigned int i = 0; i < DX::DeviceResources::MAX_THREADS_BVH_BUILD; ++i)
            {
                pool[i] = std::thread([&](unsigned int threadIndex) {
                    while (true)
                    {
                        unsigned int n = buildCounter.fetch_add(1);
                        if (n >= descs.size())
                        {
                            break;
                        }
                        
                        m_deviceResources->GetBVHCommandList(threadIndex)->BuildRaytracingAccelerationStructureBatched2X(1, &descs[n]);
                    }

                    latch.count_down();
                    }, i);
            }

            latch.wait();

            m_deviceResources->ExecuteBVHCommandLists();

            // Threads have already exited, can't destruct normally:
            for (auto& t : pool) t.detach();
        }
        else
        {
            commandList->BuildRaytracingAccelerationStructureBatched2X(static_cast<UINT>(descs.size()), descs.data());
        }
    }

    // Copy post build info to CPU resource
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_postbuildInfos.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        commandList->ResourceBarrier(1, &barrier);
        commandList->CopyBufferRegion(m_postbuildInfosCpu.Get(), 0, m_postbuildInfos.Get(), 0, m_postbuildInfoCount * sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC));
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_postbuildInfos.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->ResourceBarrier(1, &barrier);
    }

    device->SetDriverHintX(DRIVER_HINT_XDXR_FORCE_CPU_BUILDS, 0);

    for (auto& desc : geoDescsAll)
    {
        delete[] desc;
    }
}

void Sample::GetBLASPostBuildSize()
{
    void* resultValue = nullptr;
    D3D12_RANGE range = { 0, m_postbuildInfoCount * sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC) };
    D3D12_RANGE rangeWritten = {};
    m_postbuildInfosCpu->Map(0, &range, &resultValue);
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC* postBuildInfo = reinterpret_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC*>(resultValue);
    m_blasPostBuildSize = 0;
    for (size_t i = 0; i < m_postbuildInfoCount; ++i)
    {
        m_blasPostBuildSize += postBuildInfo->CurrentSizeInBytes;
        postBuildInfo++;
    }

    m_postbuildInfosCpu->Unmap(0, &rangeWritten);
}
