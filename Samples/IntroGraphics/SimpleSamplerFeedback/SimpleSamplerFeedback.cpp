//--------------------------------------------------------------------------------------
// SimpleSamplerFeedback.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "SimpleSamplerFeedback.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;
using DirectX::Colors::White;
using Microsoft::WRL::ComPtr;

namespace
{
    struct Vertex
    {
        XMFLOAT3 pos;
        XMFLOAT2 texCoord;
    };
}

Sample::Sample() noexcept(false)
    : m_frame(0)
    , m_mappedConstantBufferData(nullptr)
    , m_constantBufferDataGpuAddr(0)
    , m_feedbackMapStagingTexture{}
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
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

    m_worldMatrix = XMMatrixIdentity();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame++);

    m_deviceResources->WaitForOrigin();

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);
        m_camera->Update(elapsedTime, pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
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

    // Clear the render targets
    Clear();

    // Clear the feedback map before rendering the scene
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"ClearFeedbackMap");
    if (m_feedbackMapResource)
    {
        // Don't clear feedback map to 0, since that would mean that mip 0 was requested during scene rendering
        static const UINT s_clearValue[4] = { (UINT)-1, (UINT)-1, (UINT)-1, (UINT)-1 };
        auto const gpuHandle = m_resourceDescriptorHeap->GetGpuHandle(static_cast<int>(ResourceDescriptors::FeedbackMapUAV));
        auto const cpuHandle = m_resourceDescriptorHeap->GetCpuHandle(static_cast<int>(ResourceDescriptors::FeedbackMapUAV));
        commandList->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, m_feedbackMapResource.Get(), s_clearValue, 0, nullptr);
    }
    PIXEndEvent(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // Setup for rendering, including a binding to the feedback map
    auto heap = m_resourceDescriptorHeap->Heap();
    commandList->SetDescriptorHeaps(1, &heap);
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetGraphicsRootDescriptorTable(1, m_resourceDescriptorHeap->GetGpuHandle(static_cast<int>(ResourceDescriptors::DiffuseTextureSRV)));
    commandList->SetGraphicsRootDescriptorTable(2, m_resourceDescriptorHeap->GetGpuHandle(static_cast<int>(ResourceDescriptors::FeedbackMapUAV)));
    commandList->SetPipelineState(m_PSO.Get());

    // Set the per-frame shader constants
    ConstantBuffer sceneParameters = {};
    sceneParameters.worldMatrix = XMMatrixTranspose(m_worldMatrix);
    sceneParameters.viewMatrix = XMMatrixTranspose(m_camera->GetView());
    sceneParameters.projectionMatrix = XMMatrixTranspose(m_camera->GetProjection());
    memcpy(m_mappedConstantBufferData, &sceneParameters, sizeof(ConstantBuffer));
    commandList->SetGraphicsRootConstantBufferView(0, m_constantBufferDataGpuAddr);

    // Set up the input assembler
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->IASetIndexBuffer(&m_indexBufferView);

    // Render the quad
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

    PIXEndEvent(commandList);

    RenderUI();
    
    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Read back the feedback map value
float Sample::ReadBackFeedbackMapValue()
{
    float minRequestedMip = -1;

    if (m_feedbackMapResource)
    {
        auto commandList = m_deviceResources->GetCommandList();
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"ReadBackFeedbackMapValue");

        // Copy feedback values to the staging buffer
        TransitionResource(commandList, m_feedbackMapResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        const CD3DX12_TEXTURE_COPY_LOCATION SrcLocation(m_feedbackMapResource.Get(), 0);
        commandList->CopyTextureRegion(&m_feedbackMapStagingTexture.CopyLocation, 0, 0, 0, &SrcLocation, nullptr);
        TransitionResource(commandList, m_feedbackMapResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        // Read the value on the CPU. For this simple sample, there is only 1 value
        auto pRow = static_cast<BYTE*>(m_feedbackMapStagingTexture.Map());
        BYTE feedbackValue = pRow[0];
        m_feedbackMapStagingTexture.Unmap();

        // On Xbox Series X|S, this is a 5.3 fixed point value
        constexpr BYTE FractionalShift = 3;
        constexpr BYTE Mask = (1 << FractionalShift);
        minRequestedMip = float(feedbackValue >> FractionalShift);
        minRequestedMip += (feedbackValue & (Mask - 1)) / float(Mask);

        PIXEndEvent(commandList);
    }    

    return minRequestedMip;
}

// Render the UI
void Sample::RenderUI()
{
    // Get the feedback map value
    float minRequestedMip = ReadBackFeedbackMapValue();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderUI");

    // Render UI at 1080p
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920;
    viewportUI.Height = 1080;
    m_fontBatch->SetViewport(viewportUI);

    constexpr float startX = 50.0f;
    constexpr float startY = 40.0f;
    const float fontScale = 0.75f;
    Vector2 fontPos(startX, startY);

    m_fontBatch->Begin(commandList);
    m_textFont->DrawString(m_fontBatch.get(), L"SimpleSamplerFeedback Sample", fontPos, White, 0.0f, g_XMZero, 1.0f);

    // Show the feedback map value
    wchar_t strText[2048] = {};
    swprintf_s(strText, minRequestedMip < 0 ? L"No mip requested" : L"Minimum requested mip level: %1.3f", minRequestedMip);
    fontPos.y += 100;
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, White, 0.0f, g_XMZero, fontScale);

    fontPos.y = viewportUI.Height - 60;
    DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[LThumb] - Move camera", fontPos, White, 0.65f);

    m_fontBatch->End();

    PIXEndEvent(commandList);
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

    // Input element descriptor
    static const D3D12_INPUT_ELEMENT_DESC s_inputElementDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // Create the vertex buffer
    {
        static const Vertex vertices[] =
        {
            { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },
            { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
            { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
            { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
        };

        // Note: using upload heaps to transfer static data like vert buffers is not recommended. Every time the GPU needs it, the upload heap will be marshalled
        // over. Please read up on Default Heap usage. An upload heap is used here for code simplicity and because there are very few verts to actually transfer
        const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices));
        DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_GRAPHICS_PPV_ARGS(m_vertexBuffer.ReleaseAndGetAddressOf())));

        // Copy the data to the vertex buffer
        UINT8* pData = nullptr;
        CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU
        DX::ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData)));
        memcpy(pData, vertices, sizeof(vertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = sizeof(vertices);
    }

    // Create the index buffer
    {
        static const uint16_t indices[] =
        {
            3,1,0,
            2,1,3,
        };

        // See note above
        const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices));
        DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_GRAPHICS_PPV_ARGS(m_indexBuffer.ReleaseAndGetAddressOf())));

        // Copy the data to the index buffer.
        UINT8* pData;
        CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
        DX::ThrowIfFailed(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData)));
        memcpy(pData, indices, sizeof(indices));
        m_indexBuffer->Unmap(0, nullptr);

        // Initialize the index buffer view.
        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        m_indexBufferView.SizeInBytes = sizeof(indices);
    }

    // Create the constant buffer memory and map the CPU and GPU addresses
    {
        const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const D3D12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ConstantBuffer));

        DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_GRAPHICS_PPV_ARGS(m_constantBuffer.ReleaseAndGetAddressOf())));

        DX::ThrowIfFailed(m_constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedConstantBufferData)));
        m_constantBufferDataGpuAddr = m_constantBuffer->GetGPUVirtualAddress();
    }

    // Load shader blobs
    auto const vertexShaderBlob = DX::ReadData(L"VertexShader.cso");
    auto const pixelShaderBlob = DX::ReadData(L"PixelShader.cso");

    // Create a root signature. Xbox One best practice is to use HLSL-based root signatures to support shader precompilation.
    DX::ThrowIfFailed(device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

    // Load textures and fonts
    {
        m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
        m_resourceDescriptorHeap = std::make_unique<DescriptorHeap>(device, static_cast<UINT>(ResourceDescriptors::Count));

        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        // Textures
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, resourceUpload, L"Clouds.dds", m_diffuseTexture.ReleaseAndGetAddressOf(), false));
        device->CreateShaderResourceView(m_diffuseTexture.Get(), nullptr, m_resourceDescriptorHeap->GetCpuHandle(static_cast<int>(ResourceDescriptors::DiffuseTextureSRV)));
        m_diffuseTexture->SetName(L"Diffuse");

        // Fonts
        const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
            m_deviceResources->GetDepthBufferFormat());
        {
            SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::AlphaBlend);
            m_fontBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

            auto cpuDescHandleText = m_resourceDescriptorHeap->GetCpuHandle(static_cast<int>(ResourceDescriptors::TextFont));
            auto gpuDescHandleText = m_resourceDescriptorHeap->GetGpuHandle(static_cast<int>(ResourceDescriptors::TextFont));
            m_textFont = std::make_unique<SpriteFont>(device, resourceUpload, L"Courier_36.spritefont", cpuDescHandleText, gpuDescHandleText);

            auto cpuDescHandleController = m_resourceDescriptorHeap->GetCpuHandle(static_cast<int>(ResourceDescriptors::ControllerFont));
            auto gpuDescHandleController = m_resourceDescriptorHeap->GetGpuHandle(static_cast<int>(ResourceDescriptors::ControllerFont));
            m_controllerFont = std::make_unique<SpriteFont>(device, resourceUpload, L"XboxOneControllerLegendSmall.spritefont", cpuDescHandleController, gpuDescHandleController);
        }

        auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());

        // Wait for the upload thread to terminate
        uploadResourcesFinished.wait();
    }

    // Create feedback map
    {
        // Feedback map creation needs to know the size of its paired texture, e.g. D3D12_RESOURCE_DESC1.Width = diffuseTexture.Width
        // To calculate the logical size of the feedback map, extra info is needed in D3D12_RESOURCE_DESC1.SamplerFeedbackMipRegion
        // These values are used as a divisor between the feedback map and its paired texture. E.g. the logical width of the feedback map
        // is calculated as D3D12_RESOURCE_DESC1.Width / D3D12_RESOURCE_DESC1.SamplerFeedbackMipRegion.Width

        // NOTE: For this very simple sample we just want to create a 1x1 sized feedback map, i.e. one feedback map value for the entire paired texture.
        // To do that we need to ensure that this division results in 1, e.g. D3D12_RESOURCE_DESC1.SamplerFeedbackMipRegion.Width = D3D12_RESOURCE_DESC1.Width

        const auto diffuseTextureWidth = m_diffuseTexture->GetDesc().Width;
        const auto diffuseTextureHeight = m_diffuseTexture->GetDesc().Height;
        const auto diffuseTextureDepth = m_diffuseTexture->GetDesc().DepthOrArraySize;
        const auto feedbackMapWidth = diffuseTextureWidth;      // See note above
        const auto feedbackMapHeight = diffuseTextureHeight;    // See note above

        // We create a MinMip feedback map, so use the format DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE
        const D3D12_RESOURCE_DESC feedbackMapDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE, diffuseTextureWidth, diffuseTextureHeight, diffuseTextureDepth, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        D3D12_RESOURCE_DESC1 feedbackMapDesc1;
        memcpy(&feedbackMapDesc1, &feedbackMapDesc, sizeof(feedbackMapDesc));
        feedbackMapDesc1.SamplerFeedbackMipRegion.Width = (UINT)feedbackMapWidth;
        feedbackMapDesc1.SamplerFeedbackMipRegion.Height = feedbackMapHeight;
        feedbackMapDesc1.SamplerFeedbackMipRegion.Depth = 1;
        feedbackMapDesc1.Alignment = (UINT64)D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;

        // Create the feedback map texture
        const CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_DEFAULT);
        device->CreateCommittedResource2(&HeapProps, D3D12_HEAP_FLAG_NONE, &feedbackMapDesc1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, nullptr, __uuidof(m_feedbackMapResource), (void**)&m_feedbackMapResource);
        m_feedbackMapResource->SetName(L"FeedbackMap");

        // Create the UAV and pair the feedback map to its texture
        D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
        UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        device->CreateSamplerFeedbackUnorderedAccessView(m_diffuseTexture.Get(), m_feedbackMapResource.Get(), m_resourceDescriptorHeap->GetCpuHandle(static_cast<int>(ResourceDescriptors::FeedbackMapUAV)));

        // Create the resolve texture
        {
            // The resolve texture needs to be the logical size of the feedback map
            const auto resolveWidth = diffuseTextureWidth / feedbackMapWidth;
            const auto resolveHeight = diffuseTextureHeight / feedbackMapHeight;

            // The values in the feedback map are 8-bit, so use FORMAT_R8
            // NOTE: Using R8_UINT returns regular feedback map values. Using R8_TYPELESS returns the Scarlett specific 5.3 fix point value
            auto resolveDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_TYPELESS, resolveWidth, resolveHeight);
            resolveDesc.MipLevels = 1;  // Has to be 1 mip

            CreateStagingBufferForTexture(device, &resolveDesc, true, &m_feedbackMapStagingTexture.pStagingBuffer, &m_feedbackMapStagingTexture.CopyLocation, &m_feedbackMapStagingTexture.CopyRange);
            m_feedbackMapStagingTexture.Desc = resolveDesc;
            m_feedbackMapStagingTexture.pStagingBuffer->SetName(L"FeedbackMap Staging Readback");
        }     
    }

    // Create the PSO
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { s_inputElementDesc, _countof(s_inputElementDesc) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_PSO.ReleaseAndGetAddressOf())));
    }  

    // Wait until assets have been uploaded to the GPU.
    m_deviceResources->WaitForGpu();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Initialize camera
    m_camera = std::make_unique<DX::FlyCamera>();
    m_camera->SetWindow(static_cast<int>(m_deviceResources->GetScreenViewport().Width), static_cast<int>(m_deviceResources->GetScreenViewport().Height));
    static const XMVECTORF32 s_camPosition = { 0.0f, 0.0f, 10.0f, 1.0f };
    m_camera->SetPosition(s_camPosition);
    m_camera->SetSensitivity(100.0f, 100.0f, 100.0f, 0.0f);
    m_camera->SetFlags(DX::FlyCamera::c_FlagsDisableRotation);

    const BoundingBox bbox(XMFLOAT3(0.0f, 0.0f, 100.0f), XMFLOAT3(0.0f, 0.0f, 100.0f));
    m_camera->SetBoundingBox(bbox);
}

HRESULT Sample::CreateStagingBufferForTexture(ID3D12Device* pd3dDevice, const D3D12_RESOURCE_DESC* pTextureDesc, bool readback, ID3D12Resource** ppBuffer, D3D12_TEXTURE_COPY_LOCATION* pStagingCopyLocation, D3D12_RANGE* pEntireRange)
{
    const UINT32 sliceCount = pTextureDesc->DepthOrArraySize;

    D3D12_RESOURCE_DESC stagingDesc = *pTextureDesc;
    stagingDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    stagingDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    stagingDesc.MipLevels = 1;
    stagingDesc.DepthOrArraySize = 1;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT copyLocation = {};
    UINT64 totalBytes = 0;
    pd3dDevice->GetCopyableFootprints(&stagingDesc, 0, 1, 0, &copyLocation, nullptr, nullptr, &totalBytes);
    if (sliceCount > 1)
    {
        UINT64 sliceSizeBytes = copyLocation.Footprint.RowPitch * copyLocation.Footprint.Height;
        if (sliceSizeBytes > totalBytes)
        {
            totalBytes = sliceSizeBytes;
        }
        totalBytes *= sliceCount;
    }

    const D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(totalBytes);
    const D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(readback ? D3D12_HEAP_TYPE_READBACK : D3D12_HEAP_TYPE_UPLOAD);
    HRESULT hr = pd3dDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, readback ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)ppBuffer);

    if (pStagingCopyLocation != nullptr)
    {
        pStagingCopyLocation->Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        pStagingCopyLocation->pResource = *ppBuffer;
        copyLocation.Offset = 0;
        pStagingCopyLocation->PlacedFootprint = copyLocation;
    }

    if (pEntireRange != nullptr)
    {
        pEntireRange->Begin = 0;
        pEntireRange->End = totalBytes;
    }

    return hr;
}

#pragma endregion
