//--------------------------------------------------------------------------------------
// SimpleBezier.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleBezier.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

#pragma region Globals
// Global variables
namespace
{
    // Legend descriptors
    enum Descriptors
    {
        Font1,
        CtrlFont1,
        Count,
    };

    // Help menu text
    const wchar_t* c_sampleTitle = L"Simple Bezier Sample";
    const wchar_t* c_sampleDescription = L"Demonstrates how to create hull and domain shaders to draw a\ntessellated Bezier surface representing a Mobius strip.";
    const ATG::HelpButtonAssignment c_helpButtons[] = {
        { ATG::HelpID::MENU_BUTTON,      L"Show/Hide Help" },
        { ATG::HelpID::VIEW_BUTTON,      L"Exit" },
        { ATG::HelpID::LEFT_STICK,       L"Rotate Camera" },
        { ATG::HelpID::LEFT_TRIGGER,     L"Decrease Subdivisions" },
        { ATG::HelpID::RIGHT_TRIGGER,    L"Increase Subdivisions" },
        { ATG::HelpID::Y_BUTTON,         L"Toggle Wireframe" },
        { ATG::HelpID::A_BUTTON,         L"Fractional Partitioning (Even)" },
        { ATG::HelpID::B_BUTTON,         L"Fractional Partitioning (Odd)" },
        { ATG::HelpID::X_BUTTON,         L"Integer Partitioning" },
    };

    // Min and max divisions of the patch per side for the slider control.
    constexpr float c_minDivs = 4.0f;
    constexpr float c_maxDivs = 16.0f;
    // Startup subdivisions per side.
    constexpr float c_defaultSubdivs = 8.0f;
    // Camera's rotation angle per step.
    constexpr float c_rotationAnglePerStep = XM_2PI / 360.0f;

    // Initial camera setup
    const XMVECTORF32 c_cameraEye = { 0.0f, 0.45f, 2.7f, 0.0f };
    const XMVECTORF32 c_cameraAt = { 0.0f, 0.0f, 0.0f, 0.0f };
    const XMVECTORF32 c_cameraUp = { 0.0f, 1.0f, 0.0f, 0.0f };

    // Draw the mesh with shaded triangles at start.
    const bool c_defaultWireframeRendering = false;

    // Simple Bezier patch for a Mobius strip.
    // 4 patches with 16 control points each.
    const XMFLOAT3 c_mobiusStrip[64] = {
        { 1.0f, -0.5f, 0.0f },
        { 1.0f, -0.5f, 0.5f },
        { 0.5f, -0.3536f, 1.354f },
        { 0.0f, -0.3536f, 1.354f },
        { 1.0f, -0.1667f, 0.0f },
        { 1.0f, -0.1667f, 0.5f },
        { 0.5f, -0.1179f, 1.118f },
        { 0.0f, -0.1179f, 1.118f },
        { 1.0f, 0.1667f, 0.0f },
        { 1.0f, 0.1667f, 0.5f },
        { 0.5f, 0.1179f, 0.8821f },
        { 0.0f, 0.1179f, 0.8821f },
        { 1.0f, 0.5f, 0.0f },
        { 1.0f, 0.5f, 0.5f },
        { 0.5f, 0.3536f, 0.6464f },
        { 0.0f, 0.3536f, 0.6464f },
        { 0.0f, -0.3536f, 1.354f },
        { -0.5f, -0.3536f, 1.354f },
        { -1.5f, 0.0f, 0.5f },
        { -1.5f, 0.0f, 0.0f },
        { 0.0f, -0.1179f, 1.118f },
        { -0.5f, -0.1179f, 1.118f },
        { -1.167f, 0.0f, 0.5f },
        { -1.167f, 0.0f, 0.0f },
        { 0.0f, 0.1179f, 0.8821f },
        { -0.5f, 0.1179f, 0.8821f },
        { -0.8333f, 0.0f, 0.5f },
        { -0.8333f, 0.0f, 0.0f },
        { 0.0f, 0.3536f, 0.6464f },
        { -0.5f, 0.3536f, 0.6464f },
        { -0.5f, 0.0f, 0.5f },
        { -0.5f, 0.0f, 0.0f },
        { -1.5f, 0.0f, 0.0f },
        { -1.5f, 0.0f, -0.5f },
        { -0.5f, 0.3536f, -1.354f },
        { 0.0f, 0.3536f, -1.354f },
        { -1.167f, 0.0f, 0.0f },
        { -1.167f, 0.0f, -0.5f },
        { -0.5f, 0.1179f, -1.118f },
        { 0.0f, 0.1179f, -1.118f },
        { -0.8333f, 0.0f, 0.0f },
        { -0.8333f, 0.0f, -0.5f },
        { -0.5f, -0.1179f, -0.8821f },
        { 0.0f, -0.1179f, -0.8821f },
        { -0.5f, 0.0f, 0.0f },
        { -0.5f, 0.0f, -0.5f },
        { -0.5f, -0.3536f, -0.6464f },
        { 0.0f, -0.3536f, -0.6464f },
        { 0.0f, 0.3536f, -1.354f },
        { 0.5f, 0.3536f, -1.354f },
        { 1.0f, 0.5f, -0.5f },
        { 1.0f, 0.5f, 0.0f },
        { 0.0f, 0.1179f, -1.118f },
        { 0.5f, 0.1179f, -1.118f },
        { 1.0f, 0.1667f, -0.5f },
        { 1.0f, 0.1667f, 0.0f },
        { 0.0f, -0.1179f, -0.8821f },
        { 0.5f, -0.1179f, -0.8821f },
        { 1.0f, -0.1667f, -0.5f },
        { 1.0f, -0.1667f, 0.0f },
        { 0.0f, -0.3536f, -0.6464f },
        { 0.5f, -0.3536f, -0.6464f },
        { 1.0f, -0.5f, -0.5f },
        { 1.0f, -0.5f, 0.0f },
    };
}
#pragma endregion

Sample::Sample() noexcept(false)
    : m_frame(0)
    , m_controlPointVBView{}
    , m_mappedConstantData(nullptr)
    , m_subdivs(c_defaultSubdivs)
    , m_drawWires(c_defaultWireframeRendering)
    , m_partitionMode(PartitionMode::PartitionInteger)
    , m_showHelp(false)
{
    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD
        | DX::DeviceResources::c_TessellationShaders);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);

    m_help = std::make_unique<ATG::Help>(c_sampleTitle, c_sampleDescription, c_helpButtons, _countof(c_helpButtons), true);
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

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.menu == GamePad::ButtonStateTracker::PRESSED)
        {
            m_showHelp = !m_showHelp;
        }
        else if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            m_drawWires = !m_drawWires;
        }
        else if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            m_partitionMode = PartitionMode::PartitionInteger;
        }
        else if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_partitionMode = PartitionMode::PartitionFractionalEven;
        }
        else if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            if (m_showHelp)
            {
                m_showHelp = false;
            }
            else
            {
                m_partitionMode = PartitionMode::PartitionFractionalOdd;
            }
        }

        if (pad.IsLeftTriggerPressed())
        {
            m_subdivs = std::max(m_subdivs - 0.1f, c_minDivs);
        }

        if (pad.IsRightTriggerPressed())
        {
            m_subdivs = std::min(m_subdivs + 0.1f, c_maxDivs);
        }

        if (pad.thumbSticks.leftX != 0.0f)
        {
            float rotationAxisY = -pad.thumbSticks.leftX * c_rotationAnglePerStep;

            XMVECTOR eye = XMLoadFloat3(&m_cameraEye);
            eye = XMVector3Transform(eye, XMMatrixRotationY(rotationAxisY));
            XMMATRIX view = XMMatrixLookAtLH(eye, c_cameraAt, c_cameraUp);
            XMStoreFloat4x4(&m_viewMatrix, view);
            XMStoreFloat3(&m_cameraEye, eye);
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
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    if (m_showHelp)
    {
        m_help->Render(commandList);
    }
    else
    {
        //Set appropriate pipeline state.
        commandList->SetPipelineState(m_PSOs[m_drawWires ? 1 : 0][(int)m_partitionMode].Get());

        // Set root signature and descriptor heaps.
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap() };
        commandList->SetDescriptorHeaps(_countof(heaps), heaps);

        // Calculate world-view-projection matrix.
        XMMATRIX view = XMLoadFloat4x4(&m_viewMatrix);
        XMMATRIX projection = XMLoadFloat4x4(&m_projectionMatrix);
        XMMATRIX viewProjectionMatrix = XMMatrixMultiply(view, projection);

        // Update per-frame variables.
        if (m_mappedConstantData != nullptr)
        {
            XMStoreFloat4x4(&m_mappedConstantData->viewProjectionMatrix, viewProjectionMatrix);
            m_mappedConstantData->cameraWorldPos = m_cameraEye;
            m_mappedConstantData->tessellationFactor = (float)m_subdivs;
        }

        // Finalize dynamic constant buffer into descriptor heap.
        commandList->SetGraphicsRootDescriptorTable(c_rootParameterCB, m_resourceDescriptors->GetGpuHandle(c_rootParameterCB));

        commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST);
        commandList->IASetVertexBuffers(0, 1, &m_controlPointVBView);

        // Draw the mesh
        commandList->DrawInstanced(_countof(c_mobiusStrip), 1, 0, 0);

        // Draw the legend
        ID3D12DescriptorHeap* fontHeaps[] = { m_fontDescriptors->Heap() };
        commandList->SetDescriptorHeaps(_countof(fontHeaps), fontHeaps);

        auto const size = m_deviceResources->GetOutputSize();
        auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));

        m_batch->Begin(commandList);

        wchar_t str[64] = {};
        swprintf_s(str, L"Subdivisions: %.2f   Partition Mode: %ls", m_subdivs,
            m_partitionMode == PartitionMode::PartitionInteger ? L"Integer" :
            (m_partitionMode == PartitionMode::PartitionFractionalEven ? L"Fractional Even" : L"Fractional Odd"));
        m_smallFont->DrawString(m_batch.get(), str,
            XMFLOAT2(float(safe.left), float(safe.top)), ATG::Colors::LightGrey);

        DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(),
            L"[LThumb] Rotate   [RT][LT] Increase/decrease subdivisions   [A][B][X] Change partition mode   [Y] Toggle wireframe   [View] Exit   [Menu] Help",
            XMFLOAT2(float(safe.left), float(safe.bottom) - m_smallFont->GetLineSpacing()),
            ATG::Colors::LightGrey);

        m_batch->End();
    }

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

    // Use linear clear color for gamma-correct rendering.
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);

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

    CreateShaders();

    // Initialize the world and view matrices.
    XMMATRIX world = XMMatrixIdentity();
    XMMATRIX view = XMMatrixLookAtLH(c_cameraEye, c_cameraAt, c_cameraUp);
    XMStoreFloat4x4(&m_worldMatrix, world);
    XMStoreFloat4x4(&m_viewMatrix, view);
    XMStoreFloat3(&m_cameraEye, c_cameraEye);

    // UI resources
    m_fontDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
        m_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::AlphaBlend);

    ResourceUploadBatch uploadBatch(device);
    uploadBatch.Begin();

    m_batch = std::make_unique<SpriteBatch>(device, uploadBatch, pd);

    m_help->RestoreDevice(device, uploadBatch, rtState);

    auto finish = uploadBatch.End(m_deviceResources->GetCommandQueue());
    finish.wait();
}

// Creates and initializes shaders and their data.
void Sample::CreateShaders()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Create root signature.
    auto const vertexShaderBlob = DX::ReadData(L"BezierVS.cso");

    // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.

    DX::ThrowIfFailed(
        device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

    // Create our vertex input layout.
    const D3D12_INPUT_ELEMENT_DESC c_inputElementDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // Load shaders.
    std::vector<uint8_t> hullShaderBlobs[c_numHullShaders];
    hullShaderBlobs[(int)PartitionMode::PartitionInteger] = DX::ReadData(L"BezierHS_int.cso");
    hullShaderBlobs[(int)PartitionMode::PartitionFractionalEven] = DX::ReadData(L"BezierHS_fracEven.cso");
    hullShaderBlobs[(int)PartitionMode::PartitionFractionalOdd] = DX::ReadData(L"BezierHS_fracOdd.cso");

    auto const domainShaderBlob = DX::ReadData(L"BezierDS.cso");

    std::vector<uint8_t> pixelShaderBlobs[c_numPixelShaders];
    pixelShaderBlobs[0] = DX::ReadData(L"BezierPS.cso");
    pixelShaderBlobs[1] = DX::ReadData(L"SolidColorPS.cso");

    // Create solid and wireframe rasterizer state objects.
    D3D12_RASTERIZER_DESC RasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    RasterDesc.CullMode = D3D12_CULL_MODE_NONE;
    RasterDesc.DepthClipEnable = TRUE;

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout.pInputElementDescs = c_inputElementDesc;
    psoDesc.InputLayout.NumElements = _countof(c_inputElementDesc);
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
    psoDesc.DS = { domainShaderBlob.data(), domainShaderBlob.size() };
    psoDesc.RasterizerState = RasterDesc;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
    psoDesc.SampleDesc.Count = 1;

    // Enumerate PSOs.
    D3D12_FILL_MODE fillModes[] = { D3D12_FILL_MODE_SOLID, D3D12_FILL_MODE_WIREFRAME };
    for (size_t i = 0; i < c_numPixelShaders; i++)
    {
        psoDesc.RasterizerState.FillMode = fillModes[i];
        psoDesc.PS = { pixelShaderBlobs[i].data(), pixelShaderBlobs[i].size() };

        for (uint8_t j = 0; j < c_numHullShaders; j++)
        {
            psoDesc.HS = { hullShaderBlobs[j].data(), hullShaderBlobs[j].size() };

            DX::ThrowIfFailed(
                device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_PSOs[i][j].ReleaseAndGetAddressOf())));
        }
    }

    {
        // Create constant buffer.
        const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        const D3D12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ConstantBuffer));
        DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
            &constantBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_GRAPHICS_PPV_ARGS(m_cbPerFrame.GetAddressOf())));
        DX::ThrowIfFailed(m_cbPerFrame->SetName(L"Per Frame CB"));

        // Map it to a CPU variable. Leave the mapping active for per-frame updates.
        DX::ThrowIfFailed(m_cbPerFrame->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedConstantData)));

        // Create constant buffer view.
        constexpr uint32_t c_cbCount = 1;
        m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, c_cbCount);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = m_cbPerFrame->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = (sizeof(ConstantBuffer) + 255) & ~255;    // CB size is required to be 256-byte aligned.
        device->CreateConstantBufferView(&cbvDesc, m_resourceDescriptors->GetCpuHandle(c_rootParameterCB));

        // Create vertex buffer containing a mesh's control points.
        // Note: Using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are few verts to actually transfer.        
        const D3D12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(c_mobiusStrip));
        DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
            &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_GRAPHICS_PPV_ARGS(m_controlPointVB.GetAddressOf())));
        DX::ThrowIfFailed(m_controlPointVB->SetName(L"Control Point VB"));

        // Copy the MobiusStrip data to the vertex buffer.
        uint8_t* dataBegin;
        CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
        DX::ThrowIfFailed(m_controlPointVB->Map(
            0, &readRange, reinterpret_cast<void**>(&dataBegin)));
        memcpy(dataBegin, c_mobiusStrip, sizeof(c_mobiusStrip));
        m_controlPointVB->Unmap(0, nullptr);

        // Initialize vertex buffer view.
        ZeroMemory(&m_controlPointVBView, sizeof(m_controlPointVBView));
        m_controlPointVBView.BufferLocation = m_controlPointVB->GetGPUVirtualAddress();
        m_controlPointVBView.StrideInBytes = sizeof(XMFLOAT3);
        m_controlPointVBView.SizeInBytes = sizeof(c_mobiusStrip);
    }

    // Wait until assets have been uploaded to the GPU.
    m_deviceResources->WaitForGpu();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const size = m_deviceResources->GetOutputSize();
    auto device = m_deviceResources->GetD3DDevice();

    XMMATRIX projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), 0.01f, 100.0f);
    XMStoreFloat4x4(&m_projectionMatrix, projection);

    // UI
    m_batch->SetViewport(m_deviceResources->GetScreenViewport());

    m_help->SetWindow(size);

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
        (size.bottom > 1080) ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
        m_fontDescriptors->GetCpuHandle(Descriptors::Font1),
        m_fontDescriptors->GetGpuHandle(Descriptors::Font1));

    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        (size.bottom > 1080)
        ? L"XboxOneControllerLegend.spritefont" : L"XboxOneControllerLegendSmall.spritefont",
        m_fontDescriptors->GetCpuHandle(Descriptors::CtrlFont1),
        m_fontDescriptors->GetGpuHandle(Descriptors::CtrlFont1));

    // Wait until assets have been uploaded to the GPU.
    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
}
#pragma endregion
