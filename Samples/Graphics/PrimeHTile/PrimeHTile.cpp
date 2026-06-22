//--------------------------------------------------------------------------------------
// PrimeHTile.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PrimeHTile.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "DirectXHelpers.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const wchar_t* g_SampleTitle = L"Priming HTile Sample";
    const wchar_t* g_SampleDescription = L"Demonstrates priming the HTile with depth";
    const ATG::HelpButtonAssignment g_HelpButtons[] = {
        { ATG::HelpID::MENU_BUTTON,         L"Show/Hide Help" },
        { ATG::HelpID::VIEW_BUTTON,         L"Exit" },
        { ATG::HelpID::LEFT_STICK,          L"Occlusion Scale" },
        { ATG::HelpID::RIGHT_STICK,         L"Render Scale" },
        { ATG::HelpID::LEFT_TRIGGER,        L"Decrease depth bias on HTile population" },
        { ATG::HelpID::RIGHT_TRIGGER,       L"Increase depth bias on HTile population" },
        { ATG::HelpID::LEFT_SHOULDER,       L"Toggle Rotation" },
        { ATG::HelpID::RIGHT_SHOULDER,      L"Toggle Rotation" },
        { ATG::HelpID::DPAD_LEFT,           L"Decrease Occluder Iterations" },
        { ATG::HelpID::DPAD_RIGHT,          L"Increase Occluder Iterations" },
        { ATG::HelpID::DPAD_UP,             L"Increase Main View Iterations" },
        { ATG::HelpID::DPAD_DOWN,           L"Decrease Main View Iterations" },
        { ATG::HelpID::A_BUTTON,            L"Toggle HTile Pre-Population" },
        { ATG::HelpID::X_BUTTON,            L"Toggle Stencil" },
        { ATG::HelpID::Y_BUTTON,            L"Cycle Per Sample Bias" },
        { ATG::HelpID::B_BUTTON,            L"Toggle Parallax Mapping" },
    };

    constexpr uint32_t c_hTileTileWidth = 8;
    constexpr uint32_t c_hTileTileHeight = 8;

    constexpr uint32_t c_threadGroupDecodeHtileX = 8;
    constexpr uint32_t c_threadGroupDecodeHtileY = 8;

    constexpr XG_FORMAT c_formatOcclusion = XG_FORMAT_R32_TYPELESS;
    constexpr DXGI_FORMAT c_formatOcclusionDSV = DXGI_FORMAT_D32_FLOAT;
    constexpr DXGI_FORMAT c_formatOcclusionSRV = DXGI_FORMAT_R32_FLOAT;
    constexpr DXGI_FORMAT c_htileUAV = DXGI_FORMAT_R32_UINT;

    constexpr XG_FORMAT c_stencilFormat = XG_FORMAT_R32G8X24_TYPELESS;
    constexpr DXGI_FORMAT c_stencilFormatDSV = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

    constexpr uint32_t c_number_of_verts = 24;
    constexpr uint32_t c_number_of_indices = 36;

    const XMVECTORF32 s_eye = { 0.0f, 2.5f, -5.5f, 1.0f };
    constexpr float s_nearPlane = 0.1f;
    constexpr float s_farPlane = 20.0f;

    struct Vertex
    {
        XMFLOAT3 pos;
        XMFLOAT3 normal;
        XMFLOAT4 tangentAndBinnormalFlip;
        XMFLOAT2 uv;
    };

    const D3D12_INPUT_ELEMENT_DESC c_inputElementDesc[] =
    {
        { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "UV",          0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "I_TRANSFORM", 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    };

    const D3D12_INPUT_LAYOUT_DESC c_inputLayout
    {
        c_inputElementDesc,
        static_cast<UINT>(std::size(c_inputElementDesc))
    };

    struct DepthOnlyVertex
    {
        XMFLOAT3 pos;
    };

    const D3D12_INPUT_ELEMENT_DESC c_depthOnlyInputElementDesc[] =
    {
        { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "I_TRANSFORM", 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    };

    const D3D12_INPUT_LAYOUT_DESC c_depthOnlyInputLayout
    {
        c_depthOnlyInputElementDesc,
        static_cast<UINT>(std::size(c_depthOnlyInputElementDesc))
    };

    struct DepthVisualiseConstants
    {
        float m_fFarNearRatio;
        float m_fRemapMin;
        float m_fRemapDelta;
        float padding[1];
    };

    static_assert((sizeof(DepthVisualiseConstants) % 16) == 0, "CB size not padded correctly");

    struct PrimeHTileConstants
    {
        uint32_t m_htileInfo;
        int32_t m_depthBias;	// a real title would never use an signed bias / allow negative, only done here for demonstration purposes
        uint32_t m_pad[2];
    };

    static_assert((sizeof(PrimeHTileConstants) % 16) == 0, "CB size not padded correctly");

    struct ParallaxMappingConstants
    {
        XMMATRIX worldMatrix;
        XMMATRIX worldViewProjectionMatrix;
        XMVECTOR eyePosition;
        XMVECTOR lightDir;
        XMVECTOR lightColor;
        float mainRenderScale;
        float occlusionScale;
        uint32_t pad[2];
    };

    static_assert((sizeof(ParallaxMappingConstants) % 16) == 0, "CB size not padded correctly");

    constexpr uint32_t c_texture_size = 256;

    void CreateNormalAndHeightMaps(uint8_t(&normalMap)[c_texture_size][c_texture_size][2], uint16_t(&heightmap)[c_texture_size][c_texture_size])
    {
        // this code builds a simple circular bump in the middle of a square array
        float minZ = FLT_MAX;
        float maxZ = -FLT_MAX;

        const float border = 0.2f;
        float delta = ((2.0f * border) + 2.0f) / float(c_texture_size);
        float fy = -1.0f - border;

        // Heap-allocated to avoid stack overflow (256*256*4 = 256 KB)
        float(*heightmapFloat)[c_texture_size] = new float[c_texture_size][c_texture_size]();

        for (uint32_t y = 0; y < c_texture_size; ++y, fy += delta)
        {
            float fx = -1.0f - border;
            float fy2 = fy * fy;

            for (uint32_t x = 0; x < c_texture_size; ++x, fx += delta)
            {
                float magSqr = (fx * fx) + fy2;

                // this will generate a flat surface, the shader will generate a Z value of 1.0
                normalMap[y][x][0] = 0;
                normalMap[y][x][1] = 0;
                heightmapFloat[y][x] = 0.0f;

                if (magSqr < 1.0f)
                {
                    // we are inside the circle, normalize as if on the surface of a sphere to generate a bump
                    float r = 1.0f / sqrtf(1.0f + magSqr);

                    normalMap[y][x][0] = uint8_t(fx * r * -127.0f);
                    normalMap[y][x][1] = uint8_t(fy * r *  127.0f); // flip the y axis

                    float z = sqrtf(1.0f - magSqr);

                    heightmapFloat[y][x] = z;

                    minZ = std::min(minZ, z);
                    maxZ = std::max(maxZ, z);
                }
            }
        }

        const float deltaZ = 65535.0f / (maxZ - minZ);

        for (uint32_t y = 0; y < c_texture_size; ++y)
        {
            for (uint32_t x = 0; x < c_texture_size; ++x)
            {
                float z = heightmapFloat[y][x];

                heightmap[y][x] = (z > 0.0f) ? 65535u - uint16_t((z - minZ) * deltaZ) : 65535u;
            }
        }

        delete[] heightmapFloat;
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_vertexBufferSize(0),
    m_depthOnlyVertexBufferSize(0),
    m_indexBufferSize(0),
    m_instanceBufferSize(0),
    m_depthTextureAddress(nullptr),
    m_depthTextureAddresses{},
    m_widthHtile(0),
    m_heightHtile(0),
    m_htileInfo(0),
    m_deviceType(XSystemGetDeviceType()),
    m_showHelp(false),
    m_stencil(false),
    m_enableHTilePrePopulation(true),
    m_enableParallaxMapping(true),
    m_enableRotation(true),
    m_reset(false),
    m_firstFrameSinceRTAllocation(false),
    m_perSampleBiasMode(PerSampleBiasMode::Furthest3x3AndPhantoms),
    m_primingIterationCount(3),
    m_requestedPrimingIterationCount(3),
    m_currentIterationCount(5),
    m_hTileEncodeFloatDepthBias(0.f),
    m_occlusionRenderScale(1.f),
    m_mainRenderScale(1.f),
    m_curRotationAngle(0.f),
    m_maxInstances(0),
    m_instancesCountAtLevel{},
    m_maxInstancesAtLevel{}
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_D32_FLOAT,
        2,
        DX::DeviceResources::c_Enable4K_UHD);
    m_deviceResources->SetClearColor(ATG::Colors::Background);

    m_help = std::make_unique<ATG::Help>(g_SampleTitle, g_SampleDescription, g_HelpButtons, std::size(g_HelpButtons));

    int cubesThisIteration = 1;

    for (size_t i = 0; i < c_maxRecursionDepth; ++i)
    {
        m_maxInstances += cubesThisIteration;
        m_instancesCountAtLevel[i] = cubesThisIteration;
        m_maxInstancesAtLevel[i] = m_maxInstances;

        cubesThisIteration *= 6;
    }
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

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        using bst = GamePad::ButtonStateTracker;

        // Gamepad input handling for controller help menu.
        if (m_gamePadButtons.menu == bst::PRESSED)
        {
            m_showHelp = !m_showHelp;
        }
        else if (m_showHelp && m_gamePadButtons.b == bst::PRESSED)
        {
            m_showHelp = false;
        }
        else if (pad.IsViewPressed())
        {
            ExitSample();
        }
        else
        {
            if (m_gamePadButtons.x == bst::PRESSED)
            {
                m_stencil = !m_stencil;

                m_reset = true;
            }
            if (m_gamePadButtons.y == bst::PRESSED)
            {
                m_perSampleBiasMode = PerSampleBiasMode((static_cast<int>(m_perSampleBiasMode) + 1) % static_cast<int>(PerSampleBiasMode::Count));
            }

            if (m_gamePadButtons.a == bst::PRESSED)
            {
                m_enableHTilePrePopulation = !m_enableHTilePrePopulation;
            }
            if (m_gamePadButtons.b == bst::PRESSED)
            {
                m_enableParallaxMapping = !m_enableParallaxMapping;
            }

            if (m_gamePadButtons.leftShoulder == bst::HELD && m_gamePadButtons.rightShoulder == bst::HELD)
            {
                m_curRotationAngle = 0.f;
            }
            else if (m_gamePadButtons.leftShoulder == bst::RELEASED && m_gamePadButtons.rightShoulder == bst::RELEASED)
            {
            }
            else if (m_gamePadButtons.leftShoulder == bst::RELEASED || m_gamePadButtons.rightShoulder == bst::RELEASED)
            {
                m_enableRotation = !m_enableRotation;
            }

            if (m_gamePadButtons.dpadUp == bst::PRESSED)
            {
                if (m_currentIterationCount < int(c_maxRecursionDepth - 1))
                    ++m_currentIterationCount;
            }
            else if (m_gamePadButtons.dpadDown == bst::PRESSED)
            {
                if (m_currentIterationCount > 0)
                    --m_currentIterationCount;
            }

            if (m_gamePadButtons.dpadRight == bst::PRESSED)
            {
                if (m_requestedPrimingIterationCount < int(c_maxRecursionDepth - 1))
                {
                    ++m_requestedPrimingIterationCount;
                }
            }
            else if (m_gamePadButtons.dpadLeft == bst::PRESSED)
            {
                if (m_requestedPrimingIterationCount > 0)
                {
                    --m_requestedPrimingIterationCount;
                }
            }

            if (pad.thumbSticks.leftX != 0.f)
            {
                m_occlusionRenderScale += 0.01f * pad.thumbSticks.leftX;
                m_occlusionRenderScale = std::min(m_occlusionRenderScale, 1.0f);
                m_occlusionRenderScale = std::max(m_occlusionRenderScale, 0.0f);
            }

            if (pad.thumbSticks.rightX != 0.f)
            {
                m_mainRenderScale += 0.01f * pad.thumbSticks.rightX;
                m_mainRenderScale = std::min(m_mainRenderScale, 1.0f);
                m_mainRenderScale = std::max(m_mainRenderScale, 0.67f);
            }

            if (pad.triggers.left != 0.f)
            {
                float value = pad.triggers.left;
                value *= value;
                value *= 0.25f;

                m_hTileEncodeFloatDepthBias -= value;
                m_hTileEncodeFloatDepthBias = std::max(m_hTileEncodeFloatDepthBias, -1.0f);
            }

            if (pad.triggers.right != 0.f)
            {
                float value = pad.triggers.right;
                value *= value;
                value *= 0.25f;

                m_hTileEncodeFloatDepthBias += value;
                m_hTileEncodeFloatDepthBias = std::min(m_hTileEncodeFloatDepthBias, float((1 << 14) - 1));
            }

            m_primingIterationCount = std::min(m_requestedPrimingIterationCount, m_currentIterationCount);
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    float elapsedTime = float(timer.GetElapsedSeconds());
    m_curRotationAngle += m_enableRotation ? (elapsedTime / 3.f) : 0.f;
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

    if (m_reset)
    {
        InitializeDepthResources();
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    m_gpuTimer.BeginFrame(commandList);

    if (m_showHelp)
    {
        // Render controller help
        Clear();

        m_help->Render(commandList);
    }
    else
    {
        // Clear
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");
        m_gpuTimer.Start(commandList, PerfQuery::Setup);

        Clear(); // Doesn't clear depth/stencil since we have custom behavior here

        if (!m_enableHTilePrePopulation)
        {
            auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
            auto const dsvDescriptor = m_dsvDescriptorHeap->GetCpuHandle(DSVDescriptors::DSVDepth);

            commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);

            commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        }
        else if (m_firstFrameSinceRTAllocation)
        {
            auto const dsvDescriptor = m_dsvDescriptorHeap->GetCpuHandle(DSVDescriptors::DSVDepth);
            commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        }

        if (m_enableHTilePrePopulation)
        {
            const D3D12_VIEWPORT viewport = { 0.f, 0.f, float(m_widthHtile), float(m_heightHtile), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
            auto const scissorRect = CD3DX12_RECT(0, 0, LONG(m_widthHtile), LONG(m_heightHtile));
            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);

            auto const dsvDescriptor = m_dsvDescriptorHeap->GetCpuHandle(DSVDescriptors::DSVOcclusion);
            commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvDescriptor);
            commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

            if (m_firstFrameSinceRTAllocation)
            {
                m_firstFrameSinceRTAllocation = false;
            }
        }
        PIXEndEvent(commandList);

        m_gpuTimer.Stop(commandList, PerfQuery::Setup);
        m_gpuTimer.Start(commandList, PerfQuery::OcclusionRender);

        // Setup per-frame constant buffer
        GraphicsResource cbPerFrame;
        {
            static const XMVECTORF32 s_lightDir = { -0.577f, 0.577f, -0.577f, 1.0f };

            XMMATRIX yRotation = XMMatrixRotationY(m_curRotationAngle);
            XMMATRIX transpose = XMMatrixTranspose(yRotation);

            XMMATRIX wvp = XMMatrixMultiply(m_view, m_proj);
            wvp = XMMatrixMultiply(yRotation, wvp);

            ParallaxMappingConstants pmap = {};
            pmap.worldMatrix = transpose;
            pmap.worldViewProjectionMatrix = XMMatrixTranspose(wvp);
            pmap.eyePosition = XMVector3Transform(s_eye, transpose);
            pmap.lightDir = XMVector3Transform(s_lightDir, transpose);
            pmap.lightColor = Colors::Gray;
            pmap.mainRenderScale = m_mainRenderScale;
            pmap.occlusionScale = m_mainRenderScale * m_occlusionRenderScale;
            cbPerFrame = m_graphicsMemory->AllocateConstant(pmap);
        }

        // Low resolution occlusion buffer render
        if (m_enableHTilePrePopulation)
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Depth only pass");

            commandList->SetGraphicsRootSignature(m_rootSig.Get());

            commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::ConstantBuffer, cbPerFrame.GpuAddress());

            commandList->SetPipelineState(m_depthOnlyPSO.Get());

            D3D12_VERTEX_BUFFER_VIEW vbv[2];
            vbv[0].BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
            vbv[0].StrideInBytes = sizeof(Vertex);
            vbv[0].SizeInBytes = m_vertexBufferSize;

            vbv[1].BufferLocation = m_instanceBuffer->GetGPUVirtualAddress();
            vbv[1].StrideInBytes = sizeof(Instance);
            vbv[1].SizeInBytes = m_instanceBufferSize;
            commandList->IASetVertexBuffers(0, static_cast<UINT>(std::size(vbv)), vbv);

            D3D12_INDEX_BUFFER_VIEW ibv;
            ibv.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
            ibv.SizeInBytes = m_indexBufferSize;
            ibv.Format = DXGI_FORMAT_R16_UINT;
            commandList->IASetIndexBuffer(&ibv);

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            commandList->DrawIndexedInstanced(c_number_of_indices, UINT(m_maxInstancesAtLevel[m_primingIterationCount]), 0, 0, 0);

            PIXEndEvent(commandList);
        }

        m_gpuTimer.Stop(commandList, PerfQuery::OcclusionRender);
        m_gpuTimer.Start(commandList, PerfQuery::Priming);

        // Prime HTile
        if (m_enableHTilePrePopulation)
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Prime HTile");

            D3D12_RESOURCE_BARRIER barriers[2] =
            {
                CD3DX12_RESOURCE_BARRIER::Transition(m_occlusionTexture.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(m_depthTexture.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            };
            commandList->ResourceBarrier(static_cast<UINT>(std::size(barriers)), barriers);

            PrimeHTileConstants phtile = {};
            phtile.m_htileInfo = m_htileInfo;
            phtile.m_depthBias = static_cast<int32_t>(m_hTileEncodeFloatDepthBias);
            GraphicsResource cb = m_graphicsMemory->AllocateConstant(phtile);

            commandList->SetComputeRootSignature(m_rootSigCS.Get());
            commandList->SetComputeRootConstantBufferView(RootParameterIndexCS::ConstantBufferCS, cb.GpuAddress());
            commandList->SetComputeRootDescriptorTable(RootParameterIndexCS::TextureCS_SRV, m_resourceDescriptors->GetGpuHandle(Descriptors::Occlusion));
            commandList->SetComputeRootDescriptorTable(RootParameterIndexCS::TextureCS_UAV, m_resourceDescriptors->GetGpuHandle(Descriptors::HTileUAV));

            commandList->SetPipelineState(m_primeHTilePSO[static_cast<int>(m_perSampleBiasMode)][m_stencil ? 1 : 0].Get());

            UINT widthInThreadGroups = AlignUp(m_widthHtile, c_threadGroupDecodeHtileX) / c_threadGroupDecodeHtileX;
            UINT heightInThreadGroups = AlignUp(m_heightHtile, c_threadGroupDecodeHtileY) / c_threadGroupDecodeHtileY;
            commandList->Dispatch(widthInThreadGroups, heightInThreadGroups, 1);

            barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

            barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;

            commandList->ResourceBarrier(static_cast<UINT>(std::size(barriers)), barriers);

            auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
            auto const dsvDescriptor = m_dsvDescriptorHeap->GetCpuHandle(DSVDescriptors::DSVDepth);

            commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);

            auto const viewport = m_deviceResources->GetScreenViewport();
            auto const scissorRect = m_deviceResources->GetScissorRect();
            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);

            PIXEndEvent(commandList);
        }

        m_gpuTimer.Stop(commandList, PerfQuery::Priming);
        m_gpuTimer.Start(commandList, PerfQuery::MainScene);

        // Render main view
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Main render");

            commandList->SetGraphicsRootSignature(m_rootSig.Get());

            commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::Texture1SRV, m_resourceDescriptors->GetGpuHandle(Descriptors::NormalMap));
            commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::Texture2SRV, m_resourceDescriptors->GetGpuHandle(Descriptors::HeightMap));

            commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::ConstantBuffer, cbPerFrame.GpuAddress());

            commandList->SetPipelineState(m_enableParallaxMapping ? m_parallaxPSO.Get() : m_bumpPSO.Get());

#ifdef _GAMING_XBOX_SCARLETT
            // IMPORTANT !!!: On Xbox Series X|S, the driver chooses HTile with Depth-only encoding when both DepthStencil format is set to D32_FLOAT and VRS is disabled
            // Setting DepthStencil format to D32_FLOAT alone doesn't enable HTile with Depth-only encoding
            //
            // Therefore, to ensure Depth-only HTile format is enabled, we disable VRS.
            // However, if disabling VRS isn't appropriate for the title (for example, the title uses VRS but wants to prime HTile), the title must prime
            // HTile as if Stencil portion of HTile is enabled
            D3D12XBOX_SHADING_RATE_SETTINGS shadingRateSettings = {};
            shadingRateSettings.Type                        = D3D12XBOX_SHADING_RATE_SETTINGS_TYPE_HTILE_MODE_TYPE;
            shadingRateSettings.HTileMode.HtileEncodingType = m_stencil ? D3D12XBOX_SHADING_RATE_SETTINGS_HTILE_MODE_TYPE_4BIT_ENCODING : D3D12XBOX_SHADING_RATE_SETTINGS_HTILE_MODE_TYPE_DISABLE;

            commandList->RSSetShadingRateX(&shadingRateSettings);
#endif

            D3D12_VERTEX_BUFFER_VIEW vbv[2];
            vbv[0].BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
            vbv[0].StrideInBytes = sizeof(Vertex);
            vbv[0].SizeInBytes = m_vertexBufferSize;

            vbv[1].BufferLocation = m_instanceBuffer->GetGPUVirtualAddress();
            vbv[1].StrideInBytes = sizeof(Instance);
            vbv[1].SizeInBytes = m_instanceBufferSize;
            commandList->IASetVertexBuffers(0, static_cast<UINT>(std::size(vbv)), vbv);

            D3D12_INDEX_BUFFER_VIEW ibv;
            ibv.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
            ibv.SizeInBytes = m_indexBufferSize;
            ibv.Format = DXGI_FORMAT_R16_UINT;
            commandList->IASetIndexBuffer(&ibv);

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            commandList->DrawIndexedInstanced(c_number_of_indices, UINT(m_maxInstancesAtLevel[m_currentIterationCount]), 0, 0, 0);

            PIXEndEvent(commandList);
        }

        m_gpuTimer.Stop(commandList, PerfQuery::MainScene);

        auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

        auto const viewport = m_deviceResources->GetScreenViewport();
        auto const scissorRect = m_deviceResources->GetScissorRect();
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);

        // Draw occlusion buffer thumbnail
        if (m_enableHTilePrePopulation)
        {
            PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Visualize Depth");

            auto const size = m_deviceResources->GetOutputSize();

            auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));

            const float spacing = m_smallFont->GetLineSpacing();

            m_batchThumbnail->Begin(commandList, SpriteSortMode_Immediate);

            // Set extra constant buffer
            commandList->SetGraphicsRootConstantBufferView(RootParameterIndexVD::ConstantBuffer1VD, m_cbDepthVisualize.GpuAddress());

            m_batchThumbnail->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Occlusion), XMUINT2(m_widthHtile, m_heightHtile), XMFLOAT2(float(safe.left), float(safe.top) + spacing * 20.f));

            m_batchThumbnail->End();

            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_occlusionTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            commandList->ResourceBarrier(1, &barrier);
        }

        // Render UI
        RenderUI(commandList);
    }

    m_gpuTimer.EndFrame(commandList);
    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#pragma warning(disable : 4061)

void Sample::RenderUI(ID3D12GraphicsCommandList* commandList)
{
    PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Render UI");

    // Set the descriptor heaps
    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        m_resourceDescriptors->Heap()
    };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(descriptorHeaps)), descriptorHeaps);

    auto const size = m_deviceResources->GetOutputSize();

    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));

    m_batch->Begin(commandList);

    float y = float(safe.top);

    float spacing = m_smallFont->GetLineSpacing();

    m_smallFont->DrawString(m_batch.get(), g_SampleTitle, XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);

    y += spacing * 1.5f;

    wchar_t buff[128] = {};
    swprintf_s(buff, L"%ux%u - HTile %ux%u %s", size.right, size.bottom, m_widthHtile, m_heightHtile, m_stencil ? L"with stencil" : L"no stencil");
    m_smallFont->DrawString(m_batch.get(), buff, XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
    y += spacing;

    // render timings
    float clearTime = m_gpuTimer.GetAverageMS(PerfQuery::Setup);
    float occlusionRenderTime = m_gpuTimer.GetAverageMS(PerfQuery::OcclusionRender);
    float primingTime = m_gpuTimer.GetAverageMS(PerfQuery::Priming);
    float mainSceneTime = m_gpuTimer.GetAverageMS(PerfQuery::MainScene);

    swprintf_s(buff, L"Total time %3.3fms", clearTime + occlusionRenderTime + primingTime + mainSceneTime);
    m_smallFont->DrawString(m_batch.get(), buff, XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
    y += spacing;

    if (m_enableHTilePrePopulation)
    {
        swprintf_s(buff, L"Occlusion depth clear %3.3fms", clearTime);
        m_smallFont->DrawString(m_batch.get(), buff, XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
        y += spacing;

        swprintf_s(buff, L"Occlusion render %3.3fms", occlusionRenderTime);
        m_smallFont->DrawString(m_batch.get(), buff, XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
        y += spacing;

        swprintf_s(buff, L"Prime HTile %3.3fms", primingTime);
        m_smallFont->DrawString(m_batch.get(), buff, XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
        y += spacing;
    }
    else
    {
        swprintf_s(buff, L"Depth clear %3.3fms", clearTime);
        m_smallFont->DrawString(m_batch.get(), buff, XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
        y += spacing;
    }

    swprintf_s(buff, L"Render scene %3.3fms", mainSceneTime);
    m_smallFont->DrawString(m_batch.get(), buff, XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
    y += spacing;

    // render controls / settings
    DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(),
        m_enableHTilePrePopulation ? L"[A] HTile pre-population - On" : L"[A] HTile pre-population - Off",
        XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
    y += spacing;

    DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(),
        L"[X] Toggle Stencil",
        XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
    y += spacing;

    DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(),
        m_enableParallaxMapping ? L"[B] Parallax mapping - On" : L"[B] Parallax mapping - Off",
        XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
    y += spacing;

    DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(),
        m_enableRotation ? L"[RB] /[LB] Rotation - On" : L"[RB] /[LB] Rotation - Off",
        XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
    y += spacing;

    swprintf_s(buff, L"[RThumb] Render scale %1.2f", m_mainRenderScale);
    DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(), buff,
        XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
    y += spacing;

    swprintf_s(buff, L"[DPad] (U/D) Iteration count %u", m_currentIterationCount);
    DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(), buff,
        XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
    y += spacing;

    if (m_enableHTilePrePopulation)
    {
        swprintf_s(buff, L"[DPad] (L/R) Priming iteration count %u (actual used %u)", m_requestedPrimingIterationCount, m_primingIterationCount);
        DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(), buff,
            XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
        y += spacing;

        int iDepthBias = static_cast<int>(m_hTileEncodeFloatDepthBias);
        swprintf_s(buff, L"[RT] / [LT] HTile encode depth bias %i", iDepthBias);
        DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(), buff,
            XMFLOAT2(float(safe.left), y), (iDepthBias >= 0) ? ATG::Colors::LightGrey : ATG::Colors::Orange);
        y += spacing;

        swprintf_s(buff, L"[LThumb] Occlusion render scale %1.2f", m_occlusionRenderScale);
        DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(), buff,
            XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
        y += spacing;

        const wchar_t* psb = nullptr;

        switch (m_perSampleBiasMode)
        {
        default:
            psb = L"[Y] Per Sample Bias - Off";
            break;
        case PerSampleBiasMode::Furthest3x3:
            psb = L"[Y] Per Sample Bias - Furthest 3x3";
            break;
        case PerSampleBiasMode::Furthest3x3AndPhantoms:
            psb = L"[Y] Per Sample Bias - Furthest 3x3 & Phantoms";
            break;
        }

        DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(), psb,
            XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
    }

    DX::DrawControllerString(m_batch.get(),
        m_smallFont.get(), m_ctrlFont.get(),
        L"[View] Exit   [Menu] Help",
        XMFLOAT2(float(safe.left),
            float(safe.bottom) - m_smallFont->GetLineSpacing()),
        ATG::Colors::LightGrey);

    m_batch->End();
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

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    m_dsvDescriptorHeap = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        DSVDescriptors::DSVCount);

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    // create pipeline state objects (PSO)
    {
        EffectPipelineStateDescription pd(&c_inputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            rtState);

        auto vsBlob = DX::ReadData(L"PrimeHTileBumpMappingVS.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vsBlob.data(), vsBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSig.ReleaseAndGetAddressOf())));

        m_rootSig->SetName(L"Main RS");

        auto psBlob = DX::ReadData(L"PrimeHTileBumpMappingPS.cso");

        D3D12_SHADER_BYTECODE vs = { vsBlob.data(), vsBlob.size() };
        D3D12_SHADER_BYTECODE ps = { psBlob.data(), psBlob.size() };

        pd.CreatePipelineState(device, m_rootSig.Get(), vs, ps, m_bumpPSO.ReleaseAndGetAddressOf());

        vsBlob = DX::ReadData(L"PrimeHTileParallaxMappingVS.cso");
        psBlob = DX::ReadData(L"PrimeHTileParallaxMappingPS.cso");

        vs = { vsBlob.data(), vsBlob.size() };
        ps = { psBlob.data(), psBlob.size() };

        pd.CreatePipelineState(device, m_rootSig.Get(), vs, ps, m_parallaxPSO.ReleaseAndGetAddressOf());
    }

    {
        EffectPipelineStateDescription pd(&c_depthOnlyInputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            rtState);

        auto vsBlob = DX::ReadData(L"MeshRender.cso");

        D3D12_SHADER_BYTECODE vs = { vsBlob.data(), vsBlob.size() };
        D3D12_SHADER_BYTECODE ps = { nullptr, 0 };

        pd.CreatePipelineState(device, m_rootSig.Get(), vs, ps, m_depthOnlyPSO.ReleaseAndGetAddressOf());
    }

    {
        static const wchar_t* primeTileCS[]
        {
            L"PrimeHTileCS.cso",
            L"PrimeHTileCSStencil.cso",

            L"PrimeHTileMaxDepthCSNoStencil.cso",
            L"PrimeHTileMaxDepthCSStencil.cso",

            L"PrimeHTilePhantomDepthCSNoStencil.cso",
            L"PrimeHTilePhantomDepthCSStencil.cso",
        };

        size_t shader = 0;

        for (int i = 0; i < static_cast<int>(PerSampleBiasMode::Count); ++i)
        {
            for (int k = 0; k < 2; ++k)
            {
                assert(shader < std::size(primeTileCS));
                auto str = primeTileCS[shader];
                ++shader;

                auto csBlob = DX::ReadData(str);

                if (!i && !k)
                {
                    DX::ThrowIfFailed(
                        device->CreateRootSignature(0, csBlob.data(), csBlob.size(),
                            IID_GRAPHICS_PPV_ARGS(m_rootSigCS.ReleaseAndGetAddressOf())));

                    m_rootSigCS->SetName(L"Compute RS");
                }

                D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
                desc.pRootSignature = m_rootSigCS.Get();
                desc.CS.pShaderBytecode = csBlob.data();
                desc.CS.BytecodeLength = csBlob.size();

                DX::ThrowIfFailed(
                    device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_primeHTilePSO[i][k].ReleaseAndGetAddressOf())));

                m_primeHTilePSO[i][k]->SetName(str);
            }
        }
    }

    ResourceUploadBatch upload(device);
    upload.Begin();

    const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

    // create vertex buffers
    {
        static const float s_uvScale[6] = { 4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f };

        static const Vertex s_vertices[c_number_of_verts] =
        {
            // top face
            { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
            { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f), XMFLOAT2(s_uvScale[0], 0.0f) },
            { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f), XMFLOAT2(s_uvScale[0], s_uvScale[0]) },
            { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, s_uvScale[0]) },

            // bottom face
            { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
            { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, -1.0f), XMFLOAT2(s_uvScale[1], 0.0f) },
            { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, -1.0f), XMFLOAT2(s_uvScale[1], s_uvScale[1]) },
            { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, s_uvScale[1]) },

            // left face
            { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
            { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f), XMFLOAT2(s_uvScale[2], 0.0f) },
            { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f), XMFLOAT2(s_uvScale[2], s_uvScale[2]) },
            { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, s_uvScale[2]) },

            // right face
            { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
            { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, -1.0f), XMFLOAT2(s_uvScale[3], 0.0f) },
            { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, -1.0f), XMFLOAT2(s_uvScale[3], s_uvScale[3]) },
            { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, s_uvScale[3]) },

            // back face
            { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(0.0f, -1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
            { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(0.0f, -1.0f, 0.0f, 1.0f), XMFLOAT2(s_uvScale[4], 0.0f) },
            { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(0.0f, -1.0f, 0.0f, 1.0f), XMFLOAT2(s_uvScale[4], s_uvScale[4]) },
            { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(0.0f, -1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, s_uvScale[4]) },

            // front face
            { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT4(0.0f, -1.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
            { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT4(0.0f, -1.0f, 0.0f, -1.0f), XMFLOAT2(s_uvScale[5], 0.0f) },
            { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT4(0.0f, -1.0f, 0.0f, -1.0f), XMFLOAT2(s_uvScale[5], s_uvScale[5]) },
            { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT4(0.0f, -1.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, s_uvScale[5]) },
        };

        m_vertexBufferSize = sizeof(Vertex) * c_number_of_verts;

        SharedGraphicsResource vertexBufferUpload = m_graphicsMemory->Allocate(m_vertexBufferSize);
        memcpy(vertexBufferUpload.Memory(), s_vertices, m_vertexBufferSize);

        SharedGraphicsResource depthOnlyVertexBufferUpload;
        {
            DepthOnlyVertex depthOnlyVertices[c_number_of_verts];

            for (size_t i = 0; i < c_number_of_verts; ++i)
            {
                depthOnlyVertices[i].pos.x = s_vertices[i].pos.x;
                depthOnlyVertices[i].pos.y = s_vertices[i].pos.y;
                depthOnlyVertices[i].pos.z = s_vertices[i].pos.z;
            }

            m_depthOnlyVertexBufferSize = sizeof(DepthOnlyVertex) * c_number_of_verts;

            depthOnlyVertexBufferUpload = m_graphicsMemory->Allocate(m_depthOnlyVertexBufferSize);
            memcpy(depthOnlyVertexBufferUpload.Memory(), depthOnlyVertices, m_depthOnlyVertexBufferSize);
        }

        // Create VB resources and schedule data upload
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferSize);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_vertexBuffer.GetAddressOf())
        ));

        upload.Upload(m_vertexBuffer.Get(), vertexBufferUpload);

        upload.Transition(m_vertexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        desc.Width = m_depthOnlyVertexBufferSize;

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_depthOnlyVertexBuffer.GetAddressOf())
        ));

        upload.Upload(m_depthOnlyVertexBuffer.Get(), depthOnlyVertexBufferUpload);

        upload.Transition(m_depthOnlyVertexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    }

    // create index buffer
    {
        static const uint16_t s_indices[c_number_of_indices] =
        {
            3,1,0,
            2,1,3,

            6,4,5,
            7,4,6,

            11,9,8,
            10,9,11,

            14,12,13,
            15,12,14,

            19,17,16,
            18,17,19,

            22,20,21,
            23,20,22
        };

        SharedGraphicsResource indexBufferUpload = m_graphicsMemory->Allocate(sizeof(uint16_t) * c_number_of_indices);
        memcpy(indexBufferUpload.Memory(), s_indices, sizeof(uint16_t) * c_number_of_indices);

        // Create IB resource and schedule data upload
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferSize);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_indexBuffer.GetAddressOf())
        ));

        upload.Upload(m_indexBuffer.Get(), indexBufferUpload);

        upload.Transition(m_indexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    }

    // create instance transforms
    {
        using namespace DirectX::PackedVector;

        assert(m_maxInstances > 0);

        m_instanceBufferSize = sizeof(Instance) * m_maxInstances;

        SharedGraphicsResource instanceBufferUpload = m_graphicsMemory->Allocate(m_instanceBufferSize);

        auto instances = reinterpret_cast<Instance*>(instanceBufferUpload.Memory());

        HALF one = XMConvertFloatToHalf(1.0f);
        instances[0].PositionAndScale = XMHALF4(0, 0, 0, one);

        BuildInstanceTransforms(instances, 1, 0, instances[0]);

        // Create VB resource and schedule data upload
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(m_instanceBufferSize);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_instanceBuffer.GetAddressOf())
        ));

        upload.Upload(m_instanceBuffer.Get(), instanceBufferUpload);

        upload.Transition(m_instanceBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    }

    // Create UI objects
    {
        SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    {
        SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::Opaque);

        auto vsBlob = DX::ReadData(L"VisualizeDepthVS.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vsBlob.data(), vsBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSigVD.ReleaseAndGetAddressOf())));

        m_rootSigVD->SetName(L"Visualize Depth RS");

        auto psBlob = DX::ReadData(L"VisualizeDepth.cso");

        pd.customVertexShader = { vsBlob.data(), vsBlob.size() };
        pd.customPixelShader = { psBlob.data(), psBlob.size() };
        pd.customRootSignature = m_rootSigVD.Get();

        m_batchThumbnail = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    // Create normal and height maps
    {
        // Heap-allocated to avoid excessive stack usage (~262 KB)
        struct TextureData
        {
            uint8_t normalMap[c_texture_size][c_texture_size][2];
            uint16_t heightmap[c_texture_size][c_texture_size];
        };
        auto texData = std::make_unique<TextureData>();

        CreateNormalAndHeightMaps(texData->normalMap, texData->heightmap);

        auto desc = CD3DX12_RESOURCE_DESC(
            D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
            c_texture_size, c_texture_size, 1, 1,
            DXGI_FORMAT_R8G8_SNORM,
            1, 0,
            D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE);

        CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_normalMap.GetAddressOf())));

        SetDebugObjectName(m_normalMap.Get(), L"NormalMap");

        CreateShaderResourceView(device, m_normalMap.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::NormalMap));

        D3D12_SUBRESOURCE_DATA initData = { texData->normalMap, c_texture_size * 2, 0 };
        upload.Upload(m_normalMap.Get(), 0, &initData, 1);

        upload.Transition(m_normalMap.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        desc.Format = DXGI_FORMAT_R16_UNORM;
        initData.pData = texData->heightmap;

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_heightMap.GetAddressOf())));

        SetDebugObjectName(m_heightMap.Get(), L"HeightMap");

        CreateShaderResourceView(device, m_heightMap.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::HeightMap));

        upload.Upload(m_heightMap.Get(), 0, &initData, 1);

        upload.Transition(m_heightMap.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    // Set help device information.
    m_help->RestoreDevice(device, upload, rtState);

    auto finish = upload.End(m_deviceResources->GetCommandQueue());

    m_deviceResources->WaitForGpu();

    finish.wait();

    m_view = XMMatrixLookAtLH(s_eye, g_XMNegIdentityR0, g_XMIdentityR1);

    m_gpuTimer.RestoreDevice(device, m_deviceResources->GetCommandQueue());
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const size = m_deviceResources->GetOutputSize();

    auto device = m_deviceResources->GetD3DDevice();

    ResourceUploadBatch upload(device);
    upload.Begin();

    m_smallFont = std::make_unique<SpriteFont>(device, upload,
        (size.bottom > 1080) ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::TextFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::TextFont));

    m_ctrlFont = std::make_unique<SpriteFont>(device, upload,
        (size.bottom > 1080) ? L"XboxOneControllerLegend.spritefont" : L"XboxOneControllerLegendSmall.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    auto finish = upload.End(m_deviceResources->GetCommandQueue());

    m_deviceResources->WaitForGpu();

    finish.wait();

    m_batch->SetViewport(m_deviceResources->GetScreenViewport());
    m_batchThumbnail->SetViewport(m_deviceResources->GetScreenViewport());

    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), s_nearPlane, s_farPlane);

    // Set help rendering size.
    m_help->SetWindow(size);

    InitializeDepthResources();
}

void Sample::InitializeDepthResources()
{
    if (m_depthTextureAddress)
    {
        m_deviceResources->WaitForGpu();

        m_depthTexture.Reset();
        m_occlusionTexture.Reset();

        VirtualFree(m_depthTextureAddress, 0, MEM_RELEASE);

        m_depthTextureAddress = nullptr;
    }

    //--- Create depth buffer --------------------------------------------------------------
    auto const size = m_deviceResources->GetOutputSize();

    const uint32_t width = static_cast<uint32_t>(size.right);
    const uint32_t height = static_cast<uint32_t>(size.bottom);

    m_widthHtile = AlignUp(width, c_hTileTileWidth) / c_hTileTileWidth;
    m_heightHtile = AlignUp(height, c_hTileTileHeight) / c_hTileTileHeight;

    // sort out depth/stencil types (no point using D24S8 on XBox One, always prefer D32S8)
    XG_FORMAT formatDepth = m_stencil ? c_stencilFormat : c_formatOcclusion;
    DXGI_FORMAT formatDepthDSV = m_stencil ? c_stencilFormatDSV : c_formatOcclusionDSV;

    // retrieve tile modes
#ifdef _GAMING_XBOX_SCARLETT
    XG_SWIZZLE_MODE tileModeDepth = XG_SWIZZLE_MODE_INVALID;
    XG_SWIZZLE_MODE tileModeOcclusionDepth = XG_SWIZZLE_MODE_64KB_Z_X;
    XGComputeOptimalDepthStencilSwizzleMode(formatDepth, width, height, 1, 1, TRUE, FALSE, FALSE, &tileModeDepth);
#else
    XG_TILE_MODE tileModeDepth = XG_TILE_MODE_INVALID, tileModeStencil = XG_TILE_MODE_INVALID;
    XG_TILE_MODE tileModeOcclusionDepth = XG_TILE_MODE_COMP_DEPTH_4;	// see XFest 2018 Tales from the Trenches
    XGComputeOptimalDepthStencilTileModes(formatDepth, width, height, 1, 1, TRUE, FALSE, FALSE, &tileModeDepth, &tileModeStencil);
    assert(tileModeDepth == tileModeStencil);
#endif

    XG_RESOURCE_DESC depthBufferXGDesc =
    {
        XG_RESOURCE_DIMENSION_TEXTURE2D,
        0,
        width,
        height,
        1U,
        1U,
        formatDepth,
        { 1U, 0U, },
        static_cast<XG_TEXTURE_LAYOUT>(0x100 | tileModeDepth),
        XG12_RESOURCE_MISC_ALLOW_DEPTH_STENCIL | XG12_RESOURCE_MISC_ALLOW_UNORDERED_ACCESS,
    };

    XG_RESOURCE_DESC occlusionBufferXGDesc =
    {
        XG_RESOURCE_DIMENSION_TEXTURE2D,
        0,
        m_widthHtile,
        m_heightHtile,
        1U,
        1U,
        c_formatOcclusion,
        { 1U, 0U, },
        static_cast<XG_TEXTURE_LAYOUT>(0x100 | tileModeOcclusionDepth),
        XG12_RESOURCE_MISC_ALLOW_DEPTH_STENCIL,
    };

    ComPtr<XGTextureAddressComputer> computer;
    DX::ThrowIfFailed(XGCreateTextureComputer(&depthBufferXGDesc, computer.GetAddressOf()));

    XG_RESOURCE_LAYOUT depthBufferLayout = {};
    DX::ThrowIfFailed(computer->GetResourceLayout(&depthBufferLayout));

    DX::ThrowIfFailed(XGCreateTextureComputer(&occlusionBufferXGDesc, computer.ReleaseAndGetAddressOf()));

    XG_RESOURCE_LAYOUT occlusionBufferLayout = {};
    DX::ThrowIfFailed(computer->GetResourceLayout(&occlusionBufferLayout));

    // Reserve virtual address
    DWORD flAllocType = MEM_64K_PAGES | MEM_RESERVE | MEM_COMMIT;

#ifdef _GAMING_XBOX_XBOXONE
    if (m_deviceType == XSystemDeviceType::XboxOne || m_deviceType == XSystemDeviceType::XboxOneS)
    {
        // Using ESRAM
        flAllocType &= ~MEM_COMMIT;
    }
#endif

    uint64_t occlusionBufferOffset = AlignUp(depthBufferLayout.SizeBytes, occlusionBufferLayout.BaseAlignmentBytes);
    uint64_t alignedSize = AlignUp(
        occlusionBufferOffset + occlusionBufferLayout.SizeBytes,
        std::max(depthBufferLayout.BaseAlignmentBytes, occlusionBufferLayout.BaseAlignmentBytes));

    DWORD flProtect = PAGE_READONLY | PAGE_WRITECOMBINE | PAGE_GRAPHICS_READWRITE;
    m_depthTextureAddress = XMemVirtualAlloc(nullptr,
        alignedSize,
        flAllocType,
        XMEM_GRAPHICS,
        flProtect);
    if (!m_depthTextureAddress)
    {
        throw std::bad_alloc();
    }
#ifdef _GAMING_XBOX_XBOXONE
    if (m_deviceType == XSystemDeviceType::XboxOne || m_deviceType == XSystemDeviceType::XboxOneS)
    {
        // Map ESRAM pages
        constexpr uint32_t c_pageSize = 64 * 1024;
        constexpr uint32_t c_ESRAMSize = 32 * 1024 * 1024;

        uint32_t numESRAMPages = static_cast<uint32_t>(AlignUp(alignedSize, c_pageSize) / c_pageSize);
        uint32_t ESRAMPageList[c_ESRAMSize / c_pageSize];
        for (uint32_t i = 0; i < static_cast<UINT>(std::size(ESRAMPageList)); ++i)
        {
            ESRAMPageList[i] = i;
        }

        DX::ThrowIfFailed(D3DMapEsramMemory(D3D11_MAP_ESRAM_LARGE_PAGES,
            m_depthTextureAddress,
            numESRAMPages,
            ESRAMPageList));
    }
#endif

    // Find the address of each plane
    memset(&m_depthTextureAddresses, 0, sizeof(m_depthTextureAddresses));

    uint32_t paddedWidthElements = 0;
    uint32_t paddedHeightElements = 0;
    uint32_t htileAlignmentInBytes = 0;
    uint32_t htileSizeBytes = 0;

    for (uint32_t i = 0; i < depthBufferLayout.Planes; ++i)
    {
        switch (depthBufferLayout.Plane[i].Usage)
        {
        case XG_PLANE_USAGE_HTILE:
            m_depthTextureAddresses.DepthStencilTarget.HTile = reinterpret_cast<UINT64&>(m_depthTextureAddress) + depthBufferLayout.Plane[i].BaseOffsetBytes;
            htileAlignmentInBytes = static_cast<uint32_t>(depthBufferLayout.Plane[i].BaseAlignmentBytes);
            htileSizeBytes = static_cast<uint32_t>(depthBufferLayout.Plane[i].SizeBytes);
            break;

        case XG_PLANE_USAGE_DEPTH:
            m_depthTextureAddresses.DepthStencilTarget.DepthSamples = reinterpret_cast<UINT64&>(m_depthTextureAddress) + depthBufferLayout.Plane[i].BaseOffsetBytes;

            paddedWidthElements = depthBufferLayout.Plane[i].MipLayout[0].PaddedWidthElements;
            paddedHeightElements = depthBufferLayout.Plane[i].MipLayout[0].PaddedHeightElements;
            break;

        case XG_PLANE_USAGE_STENCIL:
            m_depthTextureAddresses.DepthStencilTarget.StencilSamples = reinterpret_cast<UINT64&>(m_depthTextureAddress) + depthBufferLayout.Plane[i].BaseOffsetBytes;
            break;

        default:
            break;
        }
    }

    assert(0 != m_depthTextureAddresses.DepthStencilTarget.HTile);
    assert(0 != m_depthTextureAddresses.DepthStencilTarget.DepthSamples);

    if (m_stencil)
    {
        assert(0 != m_depthTextureAddresses.DepthStencilTarget.StencilSamples);
    }
    else
    {
        assert(0 == m_depthTextureAddresses.DepthStencilTarget.StencilSamples);
    }

    // Generate an HTile descriptor for the compute decompression system
    uint32_t TileCountX = paddedWidthElements / c_hTileTileWidth;
    uint32_t TileCountY = paddedHeightElements / c_hTileTileHeight;
#ifdef _GAMING_XBOX_SCARLETT
    const uint32_t IsHTileLinear = 0u;
    const uint32_t PipeCount = m_deviceType == XSystemDeviceType::XboxScarlettLockhart ? 8u : 32u;
    const uint32_t MacroTileWidth = m_deviceType == XSystemDeviceType::XboxScarlettLockhart ? 64u : 128u;
    const uint32_t MacroTileHeight = m_deviceType == XSystemDeviceType::XboxScarlettLockhart ? 64u : 128u;
#else
    const uint32_t IsHTileLinear = width * height < 0x200000 ? 1u : 0u;
    const uint32_t PipeCount = m_deviceType >= XSystemDeviceType::XboxOneX ? 8u : 4u;
    const uint32_t MacroTileWidth = 64u;
    const uint32_t MacroTileHeight = 8u * PipeCount;
#endif
    if (!IsHTileLinear)
    {
        TileCountX = AlignUp(TileCountX, MacroTileWidth);
        TileCountY = AlignUp(TileCountY, MacroTileHeight);
        assert(htileSizeBytes == TileCountX * TileCountY * 4);
    }
    m_htileInfo = IsHTileLinear << 31 | PipeCount << 24 | TileCountX | TileCountY << 12;

    auto device = m_deviceResources->GetD3DDevice();

    // Creat resources
    D3D12_CLEAR_VALUE clearValue = { formatDepthDSV, 1.f };

    DX::ThrowIfFailed(device->CreatePlacedResourceX(
        *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(&m_depthTextureAddress),
        reinterpret_cast<D3D12_RESOURCE_DESC*>(&depthBufferXGDesc),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_GRAPHICS_PPV_ARGS(m_depthTexture.GetAddressOf())));

    m_depthTexture->SetName(L"Depth texture");

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = formatDepthDSV;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    device->CreateDepthStencilView(m_depthTexture.Get(), &dsvDesc, m_dsvDescriptorHeap->GetCpuHandle(DSVDescriptors::DSVDepth));

    auto occlusionBufferVirtualAddress = reinterpret_cast<void *>(reinterpret_cast<UINT64&>(m_depthTextureAddress) + occlusionBufferOffset);
    
    clearValue.Format = c_formatOcclusionDSV;
    DX::ThrowIfFailed(device->CreatePlacedResourceX(
        *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(&occlusionBufferVirtualAddress),
        reinterpret_cast<D3D12_RESOURCE_DESC*>(&occlusionBufferXGDesc),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_GRAPHICS_PPV_ARGS(m_occlusionTexture.GetAddressOf())));

    m_occlusionTexture->SetName(L"Occlusion texture");

    dsvDesc.Format = c_formatOcclusionDSV;
    device->CreateDepthStencilView(m_occlusionTexture.Get(), &dsvDesc, m_dsvDescriptorHeap->GetCpuHandle(DSVDescriptors::DSVOcclusion));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = c_formatOcclusionSRV;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(m_occlusionTexture.Get(), &srvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::Occlusion));

    D3D12_RESOURCE_DESC hTileDesc = CD3DX12_RESOURCE_DESC::Buffer(
        htileSizeBytes,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        htileAlignmentInBytes
    );

    D3D12XBOX_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format              = c_htileUAV;
    uavDesc.ViewDimension       = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.NumElements  = htileSizeBytes / sizeof(uint32_t);
    uavDesc.ResourceLocation    = m_depthTextureAddresses.DepthStencilTarget.HTile;
    device->CreatePlacedRawUnorderedAccessViewX(&hTileDesc, &uavDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::HTileUAV));

    // Setup constant buffer for visualizing depth
    DepthVisualiseConstants depthVisualizeConstants = {};
    depthVisualizeConstants.m_fFarNearRatio = s_farPlane / s_nearPlane;
    depthVisualizeConstants.m_fRemapMin = 0.2f;
    depthVisualizeConstants.m_fRemapDelta = 3.0f;
    m_cbDepthVisualize = m_graphicsMemory->AllocateConstant(depthVisualizeConstants);

    m_reset = false;
    m_firstFrameSinceRTAllocation = true;
}

//--------------------------------------------------------------------------------------
// Recursive function to build instance transforms for fractal
//--------------------------------------------------------------------------------------
void Sample::BuildInstanceTransforms(Instance * const instances, uint32_t recursionDepth, const uint32_t layerOffset, const Instance &parent)
{
    using namespace DirectX::PackedVector;

    const float oldScale = XMConvertHalfToFloat(parent.PositionAndScale.w);
    const HALF newScale = XMConvertFloatToHalf(0.5f * oldScale);
    const float x = XMConvertHalfToFloat(parent.PositionAndScale.x);
    const float y = XMConvertHalfToFloat(parent.PositionAndScale.y);
    const float z = XMConvertHalfToFloat(parent.PositionAndScale.z);

    UINT writeOffset = m_maxInstancesAtLevel[recursionDepth - 1] + layerOffset;

    if (static_cast<int>(writeOffset + 5) >= m_maxInstances)
    {
        throw std::out_of_range("BuildInstanceTransforms");
    }

    instances[writeOffset + 0].PositionAndScale = XMHALF4(XMConvertFloatToHalf(x + oldScale), XMConvertFloatToHalf(y), XMConvertFloatToHalf(z), newScale);
    instances[writeOffset + 1].PositionAndScale = XMHALF4(XMConvertFloatToHalf(x - oldScale), XMConvertFloatToHalf(y), XMConvertFloatToHalf(z), newScale);
    instances[writeOffset + 2].PositionAndScale = XMHALF4(XMConvertFloatToHalf(x), XMConvertFloatToHalf(y + oldScale), XMConvertFloatToHalf(z), newScale);
    instances[writeOffset + 3].PositionAndScale = XMHALF4(XMConvertFloatToHalf(x), XMConvertFloatToHalf(y - oldScale), XMConvertFloatToHalf(z), newScale);
    instances[writeOffset + 4].PositionAndScale = XMHALF4(XMConvertFloatToHalf(x), XMConvertFloatToHalf(y), XMConvertFloatToHalf(z + oldScale), newScale);
    instances[writeOffset + 5].PositionAndScale = XMHALF4(XMConvertFloatToHalf(x), XMConvertFloatToHalf(y), XMConvertFloatToHalf(z - oldScale), newScale);

    const UINT newRecursionDepth = recursionDepth + 1;

    if (newRecursionDepth == c_maxRecursionDepth)
        return;

    const UINT nInstancesAtLevel = UINT(m_instancesCountAtLevel[recursionDepth]);

    BuildInstanceTransforms(instances, newRecursionDepth, layerOffset + 0 * nInstancesAtLevel, instances[writeOffset + 0]);
    BuildInstanceTransforms(instances, newRecursionDepth, layerOffset + 1 * nInstancesAtLevel, instances[writeOffset + 1]);
    BuildInstanceTransforms(instances, newRecursionDepth, layerOffset + 2 * nInstancesAtLevel, instances[writeOffset + 2]);
    BuildInstanceTransforms(instances, newRecursionDepth, layerOffset + 3 * nInstancesAtLevel, instances[writeOffset + 3]);
    BuildInstanceTransforms(instances, newRecursionDepth, layerOffset + 4 * nInstancesAtLevel, instances[writeOffset + 4]);
    BuildInstanceTransforms(instances, newRecursionDepth, layerOffset + 5 * nInstancesAtLevel, instances[writeOffset + 5]);
}
#pragma endregion
