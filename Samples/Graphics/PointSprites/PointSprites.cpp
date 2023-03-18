//--------------------------------------------------------------------------------------
// PointSprites.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PointSprites.h"

#include "ATGColors.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    //-------------------------------------
    // Sample Constants

    const wchar_t*  g_sampleTitle = L"Point Sprites";

    constexpr int   c_vertexStride = (3 + 4) * 4;
    constexpr int   c_numParticles = 32 * 1024;
    constexpr float c_maxParticleColor = 0.2f;
    constexpr float c_maxParticleSize = 30.0f;
    constexpr float c_defaultParticleSize = 4.0f;
    constexpr int   c_runsPerTest = 3;
    constexpr int   c_runsPerSequence = 2;

    constexpr uint32_t c_elementDescNum = 2;

    const D3D12_INPUT_ELEMENT_DESC s_elementDesc[c_elementDescNum] =
    {
        { "TEXCOORD",   0, DXGI_FORMAT_R32G32B32_FLOAT,     0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",      0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    const D3D12_INPUT_ELEMENT_DESC s_instanceElementDesc[c_elementDescNum] =
    {
        { "TEXCOORD",   0, DXGI_FORMAT_R32G32B32_FLOAT,     0, 0,  D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "COLOR",      0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 12, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    };
}


Sample::Sample() noexcept(false)
    : m_displayWidth(0)
    , m_displayHeight(0)
    , m_frame(0)
    , m_vbView{}
    , m_particleSize(c_defaultParticleSize)
    , m_zeroViewport(false)
    , m_refreshParticles(true)
    , m_selectorIdx(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT,
        2,
        DX::DeviceResources::c_GeometryShaders | DX::DeviceResources::c_TessellationShaders);

    RegisterTests();
    CreateForwardTestSequence(m_forwardSequence);
    CreateReverseTestSequence(m_reverseSequence);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
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
void Sample::Update(DX::StepTimer const&)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (pad.IsAPressed())
        {
            m_particleSize = std::min(m_particleSize + 0.5f, c_maxParticleSize);
            m_refreshParticles = true;
        }

        if (pad.IsBPressed())
        {
            m_particleSize = std::max(m_particleSize - 0.5f, 1.f);
            m_refreshParticles = true;
        }

        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            m_zeroViewport = !m_zeroViewport;
        }

        if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
        {
            m_selectorIdx = std::min(m_selectorIdx + 1, int(m_testList.size()) - 1);
        }

        if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
        {
            m_selectorIdx = std::max(m_selectorIdx - 1, 0);
        }

        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            ToggleTest(unsigned(m_selectorIdx));
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
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();

    if (m_refreshParticles)
    {
        // Only need to regenerate and upload vertex data if our max particle size has been modified.
        UpdateVertexData(commandList);
    }

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");
    m_gpuTimer->BeginFrame(commandList);

    // Zero out the viewport if requested, effectively eliminating pixel shader invocations.
    if (m_zeroViewport)
    {
        D3D12_VIEWPORT vp = { };
        commandList->RSSetViewports(1, &vp);
    }

    // Set the common resource state to the pipeline.
    ID3D12DescriptorHeap* heaps[] = { m_srvPile->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    commandList->SetGraphicsRootSignature(m_commonRS.Get());

    float cbData[] = { 1.f / m_displayWidth, 1.f / m_displayHeight, };
    commandList->SetGraphicsRoot32BitConstants(RS_ConstantBuffer, 2, &cbData, 0);
    commandList->SetGraphicsRoot32BitConstant(RS_ConstantBuffer, UINT(c_numParticles), 2);
    commandList->SetGraphicsRootShaderResourceView(RS_VertexBuffer, m_vertices->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(RS_ParticleTex, m_srvPile->GetGpuHandle(SRV_ParticleTex));

    commandList->IASetVertexBuffers(0, 1, &m_vbView);

    ClearTestResults();

    // Run the forward sequence test c_runsPerSequence times
    for (int i = 0; i < c_runsPerSequence; ++i)
    {
        RunTests(m_forwardSequence, commandList);
    }

    // Run the random sequence test c_runsPerSequence times
    for (int i = 0; i < c_runsPerSequence; ++i)
    {
        CreateRandomTestSequence(m_randomSequence);
        RunTests(m_randomSequence, commandList);
    }

    // Run the reverse sequence test c_runsPerSequence times
    for (int i = 0; i < c_runsPerSequence; ++i)
    {
        RunTests(m_reverseSequence, commandList);
    }

    //  // Find the winning test and how many tests actually ran
    int enabledTestCount;
    Test* winnerTest = ComputeAverageTestTimeAndWinner(enabledTestCount);

    // restore viewport
    auto viewport = m_deviceResources->GetScreenViewport();
    commandList->RSSetViewports(1, &viewport);

    {
        ScopedPixEvent hud(commandList, PIX_COLOR_DEFAULT, L"HUD");

        auto safe = DirectX::SimpleMath::Viewport::ComputeTitleSafeArea(UINT(m_displayWidth), UINT(m_displayHeight));
        auto lineHeight = m_smallFont->GetLineSpacing();
        auto textColor = ATG::Colors::White;

        m_hudBatch->Begin(commandList);

        const float ParticleCountPerTest = float(c_numParticles * c_runsPerTest) * 0.000001f; // In millions
        const float TotalParticleCount = float(c_numParticles * c_runsPerTest * c_runsPerSequence * 3 * enabledTestCount) * 0.000001f; // In millions

        wchar_t textBuffer[1024] = {};
        swprintf_s(textBuffer,
            L"Particles per Test: %.2f mln \n"
            "Total Rendered Particles: %.2f mln\n"
            "ParticleSize: %.1f\n"
            "ZeroViewport: %s\n"
            "Best method for this run: %s",
            ParticleCountPerTest,
            TotalParticleCount,
            m_particleSize,
            m_zeroViewport ? L"Yes" : L"No",
            winnerTest ? winnerTest->Name : L"None");

        float xPos = float(safe.left);
        float yPos = float(safe.top);

        m_smallFont->DrawString(m_hudBatch.get(), g_sampleTitle, XMFLOAT2(xPos, yPos), textColor);
        yPos += lineHeight * 2;
        m_smallFont->DrawString(m_hudBatch.get(), textBuffer, XMFLOAT2(xPos, yPos), textColor);

        const wchar_t* controlString =
            L"[DPad] Move Test Cursor (up/down)\n"
            "[X] Toggle Selected Test\n"
            "[Y] Toggle Null Viewport\n"
            "[A] Increase Sprite Size\n"
            "[B] Decrease Sprite Size\n"
            "[View] Exit Sample";
        yPos = float(safe.bottom) - lineHeight * 6;

        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), controlString, XMFLOAT2(xPos, yPos));

        // Draw timing results
        const float resultYPos = float(safe.bottom) - lineHeight * m_testList.size();
        const wchar_t* timingString = L"Average timings of different methods (ms)";

        xPos = float(safe.right);
        float offset = XMVectorGetX(m_smallFont->MeasureString(timingString));

        m_smallFont->DrawString(m_hudBatch.get(), timingString, XMFLOAT2(xPos - offset, resultYPos - lineHeight), textColor);

        yPos = resultYPos;
        for (auto it = m_testList.begin(); it != m_testList.end(); ++it)
        {
            Test& test = *it;
            auto testColor = test.Enabled ? DirectX::Colors::Green : DirectX::Colors::Red;

            swprintf_s(textBuffer, L"%s", test.Name);
            m_smallFont->DrawString(m_hudBatch.get(), textBuffer, XMFLOAT2(xPos - 460, yPos), testColor);

            swprintf_s(textBuffer, L"%.2f", test.AverageTimeResult);
            float xOffset = XMVectorGetX(m_smallFont->MeasureString(textBuffer));
            m_smallFont->DrawString(m_hudBatch.get(), textBuffer, XMFLOAT2(xPos - xOffset, yPos), testColor);

            yPos += lineHeight;
        }

        // Draw selector
        float selectorYPos = (float)m_selectorIdx * lineHeight + resultYPos;
        m_smallFont->DrawString(m_hudBatch.get(), L">", XMFLOAT2(xPos - 480, selectorYPos), textColor);

        m_hudBatch->End();
    }

    m_gpuTimer->EndFrame(commandList);
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
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, DirectX::Colors::Black, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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
    m_commonStates = std::make_unique<DirectX::CommonStates>(device);
    m_srvPile = std::make_unique<DescriptorPile>(device,
        128,
        SRV_Count);

    auto resourceUpload = ResourceUploadBatch(device);
    resourceUpload.Begin();

    const RenderTargetState backBufferRts(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
    m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, resourceUpload, L"iris.dds", m_particleTex.ReleaseAndGetAddressOf()));
    device->CreateShaderResourceView(m_particleTex.Get(), nullptr, m_srvPile->GetCpuHandle(SRV_ParticleTex));

    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    finished.wait();

    // Create our vertex buffer resource.
    const CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    auto desc = CD3DX12_RESOURCE_DESC::Buffer(c_vertexStride * c_numParticles);

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_vertices.ReleaseAndGetAddressOf())));

    m_vertices->SetName(L"Vertex Buffer");

    m_vbView.BufferLocation = m_vertices->GetGPUVirtualAddress();
    m_vbView.SizeInBytes = c_vertexStride * c_numParticles;
    m_vbView.StrideInBytes = c_vertexStride;

    CreateTestPSOs();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    const auto size = m_deviceResources->GetOutputSize();

    // Calculate display dimensions.
    m_displayWidth = size.right - size.left;
    m_displayHeight = size.bottom - size.top;

    // Set hud sprite viewport
    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());

    // Begin uploading texture resources
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
            (size.bottom > 1080) ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
            m_srvPile->GetCpuHandle(DescriptorHeapIndex::SRV_Font),
            m_srvPile->GetGpuHandle(DescriptorHeapIndex::SRV_Font));

        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            (size.bottom > 1080) ? L"XboxOneControllerLegend.spritefont" : L"XboxOneControllerLegendSmall.spritefont",
            m_srvPile->GetCpuHandle(DescriptorHeapIndex::SRV_CtrlFont),
            m_srvPile->GetGpuHandle(DescriptorHeapIndex::SRV_CtrlFont));

        auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        finished.wait();
    }
}

void Sample::CreateTestPSOs()
{
    // Create the thousands of PSOs needed for every man, woman, and dog across the land.
    auto device = m_deviceResources->GetD3DDevice();

    // Multiply-used shader bytecode.
    auto nullVS = DX::ReadData(L"NullVS.cso");
    auto passVS = DX::ReadData(L"PassVS.cso");
    auto passOnChipVS = DX::ReadData(L"PassOnChipVS.cso");
    auto nullOnChipVS = DX::ReadData(L"NullOnChipVS.cso");
    auto renderPS = DX::ReadData(L"RenderPS.cso");

    // Strip the root signature from one of the shaders (they all leverage the same root signature.)
    DX::ThrowIfFailed(device->CreateRootSignature(0, passVS.data(), passVS.size(), IID_GRAPHICS_PPV_ARGS(m_commonRS.ReleaseAndGetAddressOf())));

    // Common PSO desc
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_commonRS.Get();
    psoDesc.PS = { renderPS.data(), renderPS.size() };
    psoDesc.DepthStencilState = m_commonStates->DepthNone;
    psoDesc.BlendState = m_commonStates->AlphaBlend;
    psoDesc.RasterizerState = m_commonStates->CullNone;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
    psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();

    // VSNativeQuads test
    {
        auto nativeQuadVS = DX::ReadData(L"RenderNativeQuadVS.cso");

        psoDesc.InputLayout = {};
        psoDesc.VS = { nativeQuadVS.data(), nativeQuadVS.size() };
        psoDesc.PrimitiveTopologyType = D3D12XBOX_PRIMITIVE_TOPOLOGY_TYPE_QUAD;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_nativeQuadPSO.ReleaseAndGetAddressOf())));
    }


    // VSNativeInstancedQuads test
    {
        auto nativeQuadInstVS = DX::ReadData(L"RenderNativeQuadInstancingVS.cso");

        psoDesc.InputLayout.pInputElementDescs = s_instanceElementDesc;
        psoDesc.InputLayout.NumElements = c_elementDescNum;
        psoDesc.VS = { nativeQuadInstVS.data(), nativeQuadInstVS.size() };
        psoDesc.PrimitiveTopologyType = D3D12XBOX_PRIMITIVE_TOPOLOGY_TYPE_QUAD;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_nativeQuadInstPSO.ReleaseAndGetAddressOf())));
    }

    // VSTriangleStripInstancedQuads test
    {
        auto triStripQuadInstVS = DX::ReadData(L"RenderTriangleStripQuadInstancingVS.cso");

        psoDesc.InputLayout.pInputElementDescs = s_instanceElementDesc;
        psoDesc.InputLayout.NumElements = c_elementDescNum;
        psoDesc.VS = { triStripQuadInstVS.data(), triStripQuadInstVS.size() };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_triStripInstQuadPSO.ReleaseAndGetAddressOf())));
    }

    // VSRectListQuads test
    {
        auto rectListQuadVS = DX::ReadData(L"RenderRectListQuadVS.cso");

        psoDesc.InputLayout = {};
        psoDesc.VS = { rectListQuadVS.data(), rectListQuadVS.size() };
        psoDesc.PrimitiveTopologyType = D3D12XBOX_PRIMITIVE_TOPOLOGY_TYPE_RECT;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_rectListQuadPSO.ReleaseAndGetAddressOf())));
    }

    // VSTriangles test
    {
        auto render3VS = DX::ReadData(L"Render3VS.cso");

        psoDesc.InputLayout = {};
        psoDesc.VS = { render3VS.data(), render3VS.size() };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_triPSO.ReleaseAndGetAddressOf())));
    }

    // VSQuads test
    {
        auto render4VS = DX::ReadData(L"Render4VS.cso");

        psoDesc.InputLayout = {};
        psoDesc.VS = { render4VS.data(), render4VS.size() };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_quadPSO.ReleaseAndGetAddressOf())));
    }

    // VSInstancedTriangles test
    {
        auto render3InstVS = DX::ReadData(L"Render3InstancingVS.cso");

        psoDesc.InputLayout.pInputElementDescs = s_instanceElementDesc;
        psoDesc.InputLayout.NumElements = c_elementDescNum;
        psoDesc.VS = { render3InstVS.data(), render3InstVS.size() };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_triInstPSO.ReleaseAndGetAddressOf())));
    }

    // VSInstancedQuads test
    {
        auto render4InstVS = DX::ReadData(L"Render4InstancingVS.cso");

        psoDesc.InputLayout.pInputElementDescs = s_instanceElementDesc;
        psoDesc.InputLayout.NumElements = c_elementDescNum;
        psoDesc.VS = { render4InstVS.data(), render4InstVS.size() };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_quadInstPSO.ReleaseAndGetAddressOf())));
    }

    // GSTriangles test
    {
        auto expand3GS = DX::ReadData(L"Expand3GS.cso");

        // OffChip
        psoDesc.InputLayout.pInputElementDescs = s_elementDesc;
        psoDesc.InputLayout.NumElements = c_elementDescNum;
        psoDesc.VS = { passVS.data(), passVS.size() };
        psoDesc.GS = { expand3GS.data(), expand3GS.size() };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_triGeoOffChipPSO.ReleaseAndGetAddressOf())));
    }

#ifdef _GAMING_XBOX_XBOXONE // Xbox Series X|S doesn't have on/off-chip specification
    {
        auto expand3OnChipGS = DX::ReadData(L"Expand3OnChipGS.cso");

        // OnChip
        psoDesc.VS = { passOnChipVS.data(), passOnChipVS.size() };
        psoDesc.GS = { expand3OnChipGS.data(), expand3OnChipGS.size() };
        psoDesc.Flags = D3D12XBOX_PIPELINE_STATE_FLAG_GS_ONCHIP_TS_OFFCHIP;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_triGeoOnChipPSO.ReleaseAndGetAddressOf())));
    }
#endif

    // GSQuads test
    {
        auto expand4GS = DX::ReadData(L"Expand4GS.cso");

        // OffChip
        psoDesc.InputLayout.pInputElementDescs = s_elementDesc;
        psoDesc.InputLayout.NumElements = c_elementDescNum;
        psoDesc.VS = { passVS.data(), passVS.size() };
        psoDesc.GS = { expand4GS.data(), expand4GS.size() };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_quadGeoOffChipPSO.ReleaseAndGetAddressOf())));
    }

#ifdef _GAMING_XBOX_XBOXONE // Xbox Series X|S doesn't have on/off-chip specification
    {
        auto expand4OnChipGS = DX::ReadData(L"Expand4OnChipGS.cso");

        // OnChip
        psoDesc.VS = { passOnChipVS.data(), passOnChipVS.size() };
        psoDesc.GS = { expand4OnChipGS.data(), expand4OnChipGS.size() };
        psoDesc.Flags = D3D12XBOX_PIPELINE_STATE_FLAG_GS_ONCHIP_TS_OFFCHIP;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_quadGeoOnChipPSO.ReleaseAndGetAddressOf())));
    }
#endif

    // GSOnlyTriangles test
    {
        auto onlyExpand3GS = DX::ReadData(L"OnlyExpand3GS.cso");

        // OffChip
        psoDesc.InputLayout = {};
        psoDesc.VS = { nullVS.data(), nullVS.size() };
        psoDesc.GS = { onlyExpand3GS.data(), onlyExpand3GS.size() };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_onlyTriGeoOffChipPSO.ReleaseAndGetAddressOf())));
    }

#ifdef _GAMING_XBOX_XBOXONE // Xbox Series X|S doesn't have on/off-chip specification
    {
        auto onlyExpand3OnChipGS = DX::ReadData(L"OnlyExpand3OnChipGS.cso");

        // OnChip
        psoDesc.VS = { nullOnChipVS.data(), nullOnChipVS.size() };
        psoDesc.GS = { onlyExpand3OnChipGS.data(), onlyExpand3OnChipGS.size() };
        psoDesc.Flags = D3D12XBOX_PIPELINE_STATE_FLAG_GS_ONCHIP_TS_OFFCHIP;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_onlyTriGeoOnChipPSO.ReleaseAndGetAddressOf())));
    }
#endif

    // GSOnlyQuads test
    {
        auto onlyExpand4GS = DX::ReadData(L"OnlyExpand4GS.cso");

        // OffChip
        psoDesc.InputLayout = {};
        psoDesc.VS = { nullVS.data(), nullVS.size() };
        psoDesc.GS = { onlyExpand4GS.data(), onlyExpand4GS.size() };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_onlyQuadGeoOffChipPSO.ReleaseAndGetAddressOf())));
    }

#ifdef _GAMING_XBOX_XBOXONE // Xbox Series X|S doesn't have on/off-chip specification
    {
        auto onlyExpand4OnChipGS = DX::ReadData(L"OnlyExpand4OnChipGS.cso");

        // OnChip
        psoDesc.VS = { nullOnChipVS.data(), nullOnChipVS.size() };
        psoDesc.GS = { onlyExpand4OnChipGS.data(), onlyExpand4OnChipGS.size() };
        psoDesc.Flags = D3D12XBOX_PIPELINE_STATE_FLAG_GS_ONCHIP_TS_OFFCHIP;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_onlyQuadGeoOnChipPSO.ReleaseAndGetAddressOf())));
    }
#endif

    // DSTriangles test
    {
        auto render3HS = DX::ReadData(L"Render3HS.cso");
        auto render3DS = DX::ReadData(L"Render3DS.cso");

        psoDesc.InputLayout.pInputElementDescs = s_elementDesc;
        psoDesc.InputLayout.NumElements = c_elementDescNum;
        psoDesc.VS = { passVS.data(), passVS.size() };
        psoDesc.HS = { render3HS.data(), render3HS.size() };
        psoDesc.DS = { render3DS.data(), render3DS.size() };
        psoDesc.GS = {};
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_triDomainPSO.ReleaseAndGetAddressOf())));
    }

    // DSQuads test
    {
        auto render4HS = DX::ReadData(L"Render4HS.cso");
        auto render4DS = DX::ReadData(L"Render4DS.cso");

        psoDesc.InputLayout.pInputElementDescs = s_elementDesc;
        psoDesc.InputLayout.NumElements = c_elementDescNum;
        psoDesc.VS = { passVS.data(), passVS.size() };
        psoDesc.HS = { render4HS.data(), render4HS.size() };
        psoDesc.DS = { render4DS.data(), render4DS.size() };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_quadDomainPSO.ReleaseAndGetAddressOf())));
    }

#ifdef _GAMING_XBOX_SCARLETT
    // MS test
    {
        auto renderMS = DX::ReadData(L"RenderMS.cso");

        // Normally use a root signature without ALLOW_INPUT_ASSEMBLER flag for mesh shader pipelines.
        // Omitted in this case as it adds complexity to the sample without improving performance.
        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC msPsoDesc = {};
        msPsoDesc.pRootSignature = m_commonRS.Get(); 
        msPsoDesc.MS = { renderMS.data(), renderMS.size() };
        msPsoDesc.PS = { renderPS.data(), renderPS.size() };
        msPsoDesc.DepthStencilState = m_commonStates->DepthNone;
        msPsoDesc.BlendState = m_commonStates->AlphaBlend;
        msPsoDesc.RasterizerState = m_commonStates->CullNone;
        msPsoDesc.SampleDesc.Count = 1;
        msPsoDesc.SampleMask = UINT_MAX;
        msPsoDesc.NumRenderTargets = 1;
        msPsoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        msPsoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        msPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        auto meshStreamDesc = CD3DX12_PIPELINE_MESH_STATE_STREAM(msPsoDesc);

        // Point to our populated stream desc
        D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
        streamDesc.SizeInBytes = sizeof(meshStreamDesc);
        streamDesc.pPipelineStateSubobjectStream = &meshStreamDesc;

        DX::ThrowIfFailed(device->CreatePipelineState(&streamDesc, IID_GRAPHICS_PPV_ARGS(m_msPSO.ReleaseAndGetAddressOf())));
    }
#endif
}
#pragma endregion


// Recreate particles vertex data when the size changes
void Sample::UpdateVertexData(ID3D12GraphicsCommandList* commandList)
{
    m_refreshParticles = false;
    srand(0);

    // Generate the vertex data
    static float vbData[7 * c_numParticles];
    for (int i = 0; i < c_numParticles; ++i)
    {
        float* p = &vbData[i * 7];

        p[0] = m_displayWidth * (float)rand() / (float)RAND_MAX;
        p[1] = m_displayHeight * (float)rand() / (float)RAND_MAX;
        p[2] = m_particleSize * (float)rand() / (float)RAND_MAX;

        p[3] = c_maxParticleColor * (float)rand() / (float)RAND_MAX;
        p[4] = c_maxParticleColor * (float)rand() / (float)RAND_MAX;
        p[5] = c_maxParticleColor * (float)rand() / (float)RAND_MAX;
        p[6] = (float)rand() / (float)RAND_MAX;
    }

    // Grab memory from the upload heap.
    auto res = m_graphicsMemory->Allocate(sizeof(vbData));
    std::memcpy(res.Memory(), vbData, sizeof(vbData));

    // Copy from the upload heap memory to our default heap resource.
    TransitionResource(commandList, m_vertices.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->CopyBufferRegion(m_vertices.Get(), 0, res.Resource(), 0, sizeof(vbData));
    TransitionResource(commandList, m_vertices.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
}

// Registers a test by name in ordered to be used by the framework
void Sample::RegisterTest(bool enabledByDefault, const wchar_t* name, TestMemFn testFn)
{
    m_testList.emplace_back(Test(enabledByDefault, unsigned(m_testList.size()), name, testFn));
}

// Registers all the tests used by the framework
void Sample::RegisterTests()
{
    m_testList.reserve(14);

    RegisterTest(true, L"VS native quads", &Sample::TestVSNativeQuads);
    RegisterTest(true, L"VS native instanced quads", &Sample::TestVSNativeInstancedQuads);
    RegisterTest(true, L"VS triangle strip instanced quads", &Sample::TestVSTriangleStripInstancedQuads);
    RegisterTest(true, L"VS rectlist quads", &Sample::TestVSRectListQuads);
    RegisterTest(true, L"VS triangles", &Sample::TestVSTriangles);
    RegisterTest(true, L"VS quads", &Sample::TestVSQuads);
    RegisterTest(true, L"VS instanced triangles", &Sample::TestVSInstancedTriangles);
    RegisterTest(true, L"VS instanced quads", &Sample::TestVSInstancedQuads);

#ifdef _GAMING_XBOX_XBOXONE // Xbox Series X|S doesn't have on/off-chip specification
    RegisterTest(true, L"GS triangles (off chip - default)", &Sample::TestGSTriangles<false>);
    RegisterTest(true, L"GS quads (off chip - default)", &Sample::TestGSQuads<false>);
    RegisterTest(true, L"GS only triangles (off chip - default)", &Sample::TestGSOnlyTriangles<false>);
    RegisterTest(true, L"GS only quads (off chip - default)", &Sample::TestGSOnlyQuads<false>);

    RegisterTest(true, L"GS triangles (on chip)", &Sample::TestGSTriangles<true>);
    RegisterTest(true, L"GS quads (on chip)", &Sample::TestGSQuads<true>);
    RegisterTest(true, L"GS only triangles (on chip)", &Sample::TestGSOnlyTriangles<true>);
    RegisterTest(true, L"GS only quads (on chip)", &Sample::TestGSOnlyQuads<true>);
#else
    RegisterTest(true, L"GS triangles", &Sample::TestGSTriangles<false>);
    RegisterTest(true, L"GS quads", &Sample::TestGSQuads<false>);
    RegisterTest(true, L"GS only triangles", &Sample::TestGSOnlyTriangles<false>);
    RegisterTest(true, L"GS only quads", &Sample::TestGSOnlyQuads<false>);
#endif

    RegisterTest(true, L"DS triangles", &Sample::TestDSTriangles);
    RegisterTest(true, L"DS quads", &Sample::TestDSQuads);

#ifdef _GAMING_XBOX_SCARLETT
    RegisterTest(true, L"MS triangles", &Sample::TestMSTriangles);
#endif
}

void Sample::RunTests(const TestSequence& testSequence, ID3D12GraphicsCommandList* commandList)
{
    for (auto it = testSequence.begin(); it != testSequence.end(); ++it)
    {
        Test* test = *it;

        if (test->Enabled)
        {
            ScopedPixEvent testScope(commandList, PIX_COLOR_DEFAULT, test->Name);

            TestMemFn testFn = test->TestMemFn;

            // Clear the buffer so all of the test have similar startup conditions
            commandList->ClearRenderTargetView(m_deviceResources->GetRenderTargetView(), DirectX::Colors::Black, 0, nullptr);
            commandList->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

            (this->*testFn)(*test, commandList);

            test->TotalTimeResult += float(m_gpuTimer->GetElapsedMS(test->Id));
            ++(test->RunCount);
        }
    }
}

void Sample::ClearTestResults()
{
    for (auto it = m_testList.begin(); it != m_testList.end(); ++it)
    {
        Test& test = *it;

        test.RunCount = 0;
        test.TotalTimeResult = 0.0f;
        test.AverageTimeResult = 0.0f;
    }
}

Sample::Test* Sample::ComputeAverageTestTimeAndWinner(int& enabledTestCount)
{
    Test* winnerTest = nullptr;
    float bestAvgTime = FLT_MAX;
    enabledTestCount = 0;

    for (auto it = m_testList.begin(); it != m_testList.end(); ++it)
    {
        Test& test = *it;

        if (test.Enabled && test.RunCount > 0)
        {
            test.AverageTimeResult = test.TotalTimeResult / (float)test.RunCount;
            if (test.AverageTimeResult < bestAvgTime)
            {
                bestAvgTime = test.AverageTimeResult;
                winnerTest = &test;
            }
            ++enabledTestCount;
        }
    }

    return winnerTest;
}

void Sample::ToggleTest(unsigned testIdx)
{
    assert(testIdx < m_testList.size());

    m_testList[testIdx].Enabled = !m_testList[testIdx].Enabled;
}

void Sample::CreateRandomTestSequence(TestSequence& testSequence)
{
    TestSequence temp;
    size_t testCount = m_testList.size();

    temp.reserve(testCount);
    // Push all elements into temp test sequence
    for (auto it = m_testList.begin(); it != m_testList.end(); ++it)
    {
        temp.push_back(&(*it));
    }

    // Clear previous test sequence
    testSequence.clear();
    testSequence.reserve(testCount);

    for (size_t i = 0; i < testCount; ++i)
    {
        size_t randIdx = rand() % temp.size();
        testSequence.push_back(temp[randIdx]);

        // Remove the element
        temp[randIdx] = temp.back();
        temp.pop_back();
    }
}

void Sample::CreateForwardTestSequence(TestSequence& testSequence)
{
    testSequence.clear();
    testSequence.reserve(m_testList.size());

    for (auto it = m_testList.begin(); it != m_testList.end(); ++it)
    {
        testSequence.push_back(&(*it));
    }
}

void Sample::CreateReverseTestSequence(TestSequence& testSequence)
{
    testSequence.clear();
    testSequence.reserve(m_testList.size());

    for (auto it = m_testList.rbegin(); it != m_testList.rend(); ++it)
    {
        testSequence.push_back(&(*it));
    }
}

#pragma region VS Tests
void Sample::TestVSNativeQuads(Test& test, ID3D12GraphicsCommandList* commandList)
{
    commandList->SetPipelineState(m_nativeQuadPSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_QUADLIST);

    m_gpuTimer->Start(commandList, test.Id);
    for (int i = 0; i < c_runsPerTest; ++i)
    {
        // multiplying the number of verts by 4 because each 4 verts will make a quad
        commandList->DrawInstanced(c_numParticles * 4, 1, 0, 0);
    }
    m_gpuTimer->Stop(commandList, test.Id);
}

void Sample::TestVSNativeInstancedQuads(Test& test, ID3D12GraphicsCommandList* commandList)
{
    commandList->SetPipelineState(m_nativeQuadInstPSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_QUADLIST);

    m_gpuTimer->Start(commandList, test.Id);
    for (int i = 0; i < c_runsPerTest; ++i)
    {
        commandList->DrawInstanced(4, c_numParticles, 0, 0);
    }
    m_gpuTimer->Stop(commandList, test.Id);
}

void Sample::TestVSTriangleStripInstancedQuads(Test& test, ID3D12GraphicsCommandList* commandList)
{
    commandList->SetPipelineState(m_triStripInstQuadPSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    m_gpuTimer->Start(commandList, test.Id);
    for (int i = 0; i < c_runsPerTest; ++i)
    {
        // multiplying the number of verts by 4 because each 4 verts will make a quad
        commandList->DrawInstanced(4, c_numParticles, 0, 0);
    }
    m_gpuTimer->Stop(commandList, test.Id);
}

void Sample::TestVSRectListQuads(Test& test, ID3D12GraphicsCommandList* commandList)
{
    commandList->SetPipelineState(m_rectListQuadPSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_RECTLIST);

    m_gpuTimer->Start(commandList, test.Id);
    for (int i = 0; i < c_runsPerTest; ++i)
    {
        // multiplying the number of verts by 3 because each 3 verts will make a quad
        commandList->DrawInstanced(c_numParticles * 3, 1, 0, 0);
    }
    m_gpuTimer->Stop(commandList, test.Id);
}

void Sample::TestVSTriangles(Test& test, ID3D12GraphicsCommandList* commandList)
{
    commandList->SetPipelineState(m_triPSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_gpuTimer->Start(commandList, test.Id);
    for (int i = 0; i < c_runsPerTest; ++i)
    {
        // multiplying the number of verts by 3 because each 3 verts will make a triangle
        commandList->DrawInstanced(c_numParticles * 3, 1, 0, 0);
    }
    m_gpuTimer->Stop(commandList, test.Id);
}

void Sample::TestVSQuads(Test& test, ID3D12GraphicsCommandList* commandList)
{
    commandList->SetPipelineState(m_quadPSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_gpuTimer->Start(commandList, test.Id);
    for (int i = 0; i < c_runsPerTest; ++i)
    {
        // multiplying the number of verts by 6 because each 6 verts will make two triangles
        commandList->DrawInstanced(c_numParticles * 6, 1, 0, 0);
    }
    m_gpuTimer->Stop(commandList, test.Id);
}

void Sample::TestVSInstancedTriangles(Test& test, ID3D12GraphicsCommandList* commandList)
{
    commandList->SetPipelineState(m_triInstPSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_gpuTimer->Start(commandList, test.Id);
    for (int i = 0; i < c_runsPerTest; ++i)
    {
        commandList->DrawInstanced(3, c_numParticles, 0, 0);
    }
    m_gpuTimer->Stop(commandList, test.Id);
}

void Sample::TestVSInstancedQuads(Test& test, ID3D12GraphicsCommandList* commandList)
{
    commandList->SetPipelineState(m_quadInstPSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_gpuTimer->Start(commandList, test.Id);
    for (int i = 0; i < c_runsPerTest; ++i)
    {
        commandList->DrawInstanced(6, c_numParticles, 0, 0);
    }
    m_gpuTimer->Stop(commandList, test.Id);
}
#pragma endregion

#pragma region GS Tests
#pragma warning( push )
#pragma warning( disable : 4127 )   // conditional expression is constant
template<bool OnChip>
void Sample::TestGSTriangles(Test& test, ID3D12GraphicsCommandList* commandList)
{
    commandList->SetPipelineState(OnChip ? m_triGeoOnChipPSO.Get() : m_triGeoOffChipPSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

    m_gpuTimer->Start(commandList, test.Id);
    for (int i = 0; i < c_runsPerTest; ++i)
    {
        commandList->DrawInstanced(c_numParticles, 1, 0, 0);
    }
    m_gpuTimer->Stop(commandList, test.Id);
}

template<bool OnChip>
void Sample::TestGSQuads(Test& test, ID3D12GraphicsCommandList* commandList)
{
    commandList->SetPipelineState(OnChip ? m_quadGeoOnChipPSO.Get() : m_quadGeoOffChipPSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

    m_gpuTimer->Start(commandList, test.Id);
    for (int i = 0; i < c_runsPerTest; ++i)
    {
        commandList->DrawInstanced(c_numParticles, 1, 0, 0);
    }
    m_gpuTimer->Stop(commandList, test.Id);
}

template<bool OnChip>
void Sample::TestGSOnlyTriangles(Test& test, ID3D12GraphicsCommandList* commandList)
{
    commandList->SetPipelineState(OnChip ? m_onlyTriGeoOnChipPSO.Get() : m_onlyTriGeoOffChipPSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

    m_gpuTimer->Start(commandList, test.Id);
    for (int i = 0; i < c_runsPerTest; ++i)
    {
        commandList->DrawInstanced(c_numParticles, 1, 0, 0);
    }
    m_gpuTimer->Stop(commandList, test.Id);
}

template<bool OnChip>
void Sample::TestGSOnlyQuads(Test& test, ID3D12GraphicsCommandList* commandList)
{
    commandList->SetPipelineState(OnChip ? m_onlyQuadGeoOnChipPSO.Get() : m_onlyQuadGeoOffChipPSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

    m_gpuTimer->Start(commandList, test.Id);
    for (int i = 0; i < c_runsPerTest; ++i)
    {
        commandList->DrawInstanced(c_numParticles, 1, 0, 0);
    }

    m_gpuTimer->Stop(commandList, test.Id);
}
#pragma warning( pop )
#pragma endregion

#pragma region DS Tests
void Sample::TestDSTriangles(Test& test, ID3D12GraphicsCommandList* commandList)
{
    commandList->SetPipelineState(m_triDomainPSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);

    m_gpuTimer->Start(commandList, test.Id);
    for (int i = 0; i < c_runsPerTest; ++i)
    {
        commandList->DrawInstanced(c_numParticles, 1, 0, 0);
    }

    m_gpuTimer->Stop(commandList, test.Id);
}

void Sample::TestDSQuads(Test& test, ID3D12GraphicsCommandList* commandList)
{
    commandList->SetPipelineState(m_quadDomainPSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);

    m_gpuTimer->Start(commandList, test.Id);
    for (int i = 0; i < c_runsPerTest; ++i)
    {
        commandList->DrawInstanced(c_numParticles, 1, 0, 0);
    }

    m_gpuTimer->Stop(commandList, test.Id);
}
#pragma endregion

#ifdef _GAMING_XBOX_SCARLETT
#pragma region MSTests

void Sample::TestMSTriangles(Test& test, ID3D12GraphicsCommandList* commandList)
{
    ID3D12GraphicsCommandList6* cmdList = static_cast<ID3D12GraphicsCommandList6*>(commandList);

    commandList->SetPipelineState(m_msPSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    uint32_t totalVertCount = (c_numParticles * 4);
    uint32_t groupCount = (totalVertCount + 31) / 32;

    m_gpuTimer->Start(commandList, test.Id);
    for (int i = 0; i < c_runsPerTest; ++i)
    {
        cmdList->DispatchMesh(groupCount, 1, 1);
    }

    m_gpuTimer->Stop(commandList, test.Id);
}
#pragma endregion
#endif
