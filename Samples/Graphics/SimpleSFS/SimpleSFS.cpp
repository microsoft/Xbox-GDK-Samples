//--------------------------------------------------------------------------------------
// SimpleSFS.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleSFS.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace SimpleMath;
using DirectX::Colors::White;
using Microsoft::WRL::ComPtr;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

namespace
{
    struct Vertex
    {
        XMFLOAT3 Pos;
        XMFLOAT2 TexCoord;
    };

    // On Scarlett, feedback values are encoded as 5.3 fixed point. When resolving the feedback map to be consumed on the CPU, you can
    // use format R8_UNORM to get the clamped BYTE values with no fractional portion, or use R8_TYPELESS to get the 5.3 fixed point value
    // These are convenient helper functions to convert the feedback values
    constexpr BYTE g_FeedbackValueFractionalLODShift = 3;
    inline BYTE MipValueToFeedbackValue(BYTE mip) { return BYTE(mip << g_FeedbackValueFractionalLODShift); }
    inline BYTE FeedbackValueToMipValue(BYTE feedbackValue) { return BYTE(feedbackValue >> g_FeedbackValueFractionalLODShift); }

    // Decode the 5.3 fixed point value to a floating point value with fractional portion
    float DecodeFeedbackMapValue(BYTE feedbackValue)
    {
        constexpr BYTE Mask = (1 << g_FeedbackValueFractionalLODShift);
        float mipValue = float(feedbackValue >> g_FeedbackValueFractionalLODShift);
        mipValue += (feedbackValue & (Mask - 1)) / float(Mask);
        return mipValue;
    }

    static const D3D12_TILE_REGION_SIZE g_OneTile = { 1, FALSE, 1, 1, 1 };
    static const D3D12_TILE_RANGE_FLAGS g_ClearTileMappingFlag = D3D12_TILE_RANGE_FLAG_NULL;

    static const ULONGLONG g_PlacementAttributes = MAKE_XALLOC_ATTRIBUTES(0, 0, XALLOC_MEMTYPE_GRAPHICS_WRITECOMBINE, XALLOC_PAGESIZE_64KB, XALLOC_ALIGNMENT_64K, 0);

    DSTORAGE_BCPACK_MODE GetBCPackMode(DXGI_FORMAT dxgiFormat)
    {
        switch (dxgiFormat)
        {
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            return DSTORAGE_BCPACK_MODE_BC1;

        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return DSTORAGE_BCPACK_MODE_BC3;

        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            return DSTORAGE_BCPACK_MODE_BC4;

        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
            return DSTORAGE_BCPACK_MODE_BC5;

        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
            return DSTORAGE_BCPACK_MODE_BC6H;

        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return DSTORAGE_BCPACK_MODE_BC7;

        default:
            throw std::exception("Unknown DXGI format for BCPack");
        }
    }
}

Sample::Sample() noexcept(false)
    : m_frame(0)
    , m_vertexBufferView{}
    , m_indexBufferView{}
    , m_pMappedConstantBufferData(nullptr)
    , m_constantBufferDataGpuAddr(0)
    , m_worldMatrix{}
    , m_viewMatrix{}
    , m_projectionMatrix{}
    , m_feedbackMapWidth(0)
    , m_feedbackMapHeight(0)
    , m_pFeedbackMapData(nullptr)
    , m_pMinMipMapData(nullptr)
    , m_minMipMapPitch(0)
    , m_minMipMapSizeBytes(0)
    , m_memoryPoolBaseAddress(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_D32_FLOAT,
        2,
        DX::DeviceResources::c_Enable4K_UHD  | DX::DeviceResources::c_EnableQHD);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);

    for (int i = 0; i < g_NumTilesInMemoryPool; i++)
    {
        m_availableTilesInMemoryPool.push(i);
    }
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    if (m_pMinMipMapData)
    {
        XMemFree((void*)m_pMinMipMapData, g_PlacementAttributes);
        m_pMinMipMapData = nullptr;
    }

    if (m_pFeedbackMapData)
    {
        m_feedbackMapStagingTexture.Unmap();
        m_pFeedbackMapData = nullptr;
    }

    if (m_pMappedConstantBufferData)
    {
        m_constantBuffer->Unmap(0, nullptr);
        m_pMappedConstantBufferData = nullptr;
    }

    m_dsStatus.Reset();
    m_dsQueue.Reset();
    m_dsFactory.Reset();
    m_dsFile.Reset();
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

    // Initialize matrices
    XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&m_viewMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&m_projectionMatrix, XMMatrixIdentity());

    // DirectStorage
    {
        // Create the DirectStorage factory
        DX::ThrowIfFailed(DStorageGetFactory(__uuidof(IDStorageFactoryX), (void **)(m_dsFactory.ReleaseAndGetAddressOf())));

        // Create a queue that will manage read requests being routed to the hardware
        DSTORAGE_QUEUE_DESC queueDesc = {};
        queueDesc.Priority = DSTORAGE_PRIORITY_HIGH;		// Use high priority for texture streaming
        queueDesc.Capacity = DSTORAGE_MIN_QUEUE_CAPACITY;	// How many outstanding requests can the queue hold
        queueDesc.Name = "High Pri Streaming Queue";
        DX::ThrowIfFailed(m_dsFactory->CreateQueue(&queueDesc, __uuidof(IDStorageQueueX), (void **)(m_dsQueue.ReleaseAndGetAddressOf())));

        // Ceate a status array for notification of read completion. In this case we only need one slot in the array
        // It's not valid to check a slot for completion until after that slot has been enqueued, the default is signaled as complete   
        DX::ThrowIfFailed(m_dsFactory->CreateStatusArray(1, "Status Array", __uuidof(IDStorageStatusArrayX), (void **)(m_dsStatus.ReleaseAndGetAddressOf())));
    }

    // Open the compressed texture file
    HRESULT hr = m_dsFactory->OpenFile(m_tiledTextureFileName.c_str(), __uuidof(IDStorageFileX), (void**)(m_dsFile.ReleaseAndGetAddressOf()));
    if (!SUCCEEDED(hr))
    {
        throw std::exception("Couldn't open compressed texture file");
    }

    // Reset streaming
    ResetStreaming();
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

    auto pad = m_gamePad->GetState(0);
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

    // Process all submitted streaming requests
    ProcessSubmittedStreamingRequests();
    AgeOutTiles();

    // The streaming system updated the MinMip map, so flush before rendering
    auto commandList = m_deviceResources->GetCommandList();
    commandList->FlushPipelineX(D3D12XBOX_FLUSH_TOP_TEXTURE_L1_INVALIDATE | D3D12XBOX_FLUSH_TOP_TEXTURE_L2_INVALIDATE, m_minMipMapResource->GetGPUVirtualAddress(), m_minMipMapSizeBytes);

    // Clear the render targets
    Clear();

    // Clear the feedback map before rendering the scene
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"ClearFeedbackMap");
    if (m_feedbackMapResource)
    {
        // Don't clear feedback map to 0, since that would mean that mip 0 was requested during scene rendering
        static const UINT s_clearValue[4] = { (UINT)-1, (UINT)-1, (UINT)-1, (UINT)-1 };
        auto gpuHandle = m_resourceDescriptorHeap->GetGpuHandle(static_cast<int>(ResourceDescriptors::FeedbackMapUAV));
        auto cpuHandle = m_resourceDescriptorHeap->GetCpuHandle(static_cast<int>(ResourceDescriptors::FeedbackMapUAV));
        commandList->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, m_feedbackMapResource.Get(), s_clearValue, 0, nullptr);
    }
    PIXEndEvent(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // Setup for rendering, including a binding to the feedback map
    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptorHeap->Heap(), m_samplerDescriptorHeap->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetGraphicsRootDescriptorTable(1, m_resourceDescriptorHeap->GetGpuHandle(static_cast<int>(ResourceDescriptors::TiledTextureSRV)));
    commandList->SetGraphicsRootDescriptorTable(2, m_resourceDescriptorHeap->GetGpuHandle(static_cast<int>(ResourceDescriptors::MinMipMapSRV)));
    commandList->SetGraphicsRootDescriptorTable(3, m_resourceDescriptorHeap->GetGpuHandle(static_cast<int>(ResourceDescriptors::FeedbackMapUAV)));
    commandList->SetGraphicsRootDescriptorTable(4, m_samplerDescriptorHeap->GetGpuHandle(static_cast<int>(SamplerDescriptors::MinMipMapSampler)));
    commandList->SetPipelineState(m_PSO.Get());

    // Set the per-frame shader constants
    ConstantBuffer sceneParameters = {};
    sceneParameters.WorldMatrix = XMMatrixTranspose(XMLoadFloat4x4(&m_worldMatrix));
    sceneParameters.ViewMatrix = XMMatrixTranspose(m_camera->GetView());
    sceneParameters.ProjectionMatrix = XMMatrixTranspose(m_camera->GetProjection());
    memcpy(m_pMappedConstantBufferData, &sceneParameters, sizeof(ConstantBuffer));
    commandList->SetGraphicsRootConstantBufferView(0, m_constantBufferDataGpuAddr);

    // Set up the input assembler
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->IASetIndexBuffer(&m_indexBufferView);

    // Render the quad
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

    PIXEndEvent(commandList);

    // A simple sample, so process texture streaming synchronously after scene rendering
    GenerateNewStreamingRequests();
    ProcessNewStreamingRequests();

    // Render UI showing MinMip and feedback values
    RenderUI();
    
    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Visualize which tiles in a mip level is resident, a visualization of the MinMip map
void Sample::VisualizeMip(UINT mip, LONG x, LONG y, LONG size)
{
    const auto textureSize = GetTextureSize(m_debugTexture.Get());
    const auto debugTextureHandle = m_resourceDescriptorHeap->GetGpuHandle(static_cast<int>(ResourceDescriptors::DebugTexture));

    // Black background
    constexpr LONG border = 10;
    RECT pos = { x - border, y - border, x + size + border, y + size + border };
    m_spriteBatch->Draw(debugTextureHandle, textureSize, pos, Colors::Black);

    // The whole texture
    pos = { x, y, x + size, y + size };
    m_spriteBatch->Draw(debugTextureHandle, textureSize, pos, Colors::White);

    // Transparent black blocks on the tiles that are not resident
    const XMVECTORF32 transparentBlack = { { { 0.0f, 0.0f, 0.0f, 0.75f } } };
    const auto widthInTiles = m_tiledTexture.TilingInfo[mip].WidthInTiles;
    const auto heightInTiles = m_tiledTexture.TilingInfo[mip].HeightInTiles;
    const auto widthInPixels = 1 + size / widthInTiles;
    const auto heightInPixels = 1 + size / heightInTiles;

    for (UINT j = 0; j < heightInTiles; j++)
    {
        for (UINT i = 0; i < widthInTiles; i++)
        {
            if (m_tiledTexture.GetTile(i, j, mip)->Status != Tile::TileStatus::Loaded)
            {
                pos = { static_cast<LONG>(x - 1 + i * widthInPixels), static_cast<LONG>(y - 1 + j * heightInPixels),
                        static_cast<LONG>(x + 1 + (i + 1) * widthInPixels), static_cast<LONG>(y + 1 + (j + 1) * heightInPixels) };
                m_spriteBatch->Draw(debugTextureHandle, textureSize, pos, transparentBlack);
            }
        }
    }
}

// Render the UI
void Sample::RenderUI()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderUI");

    // Render UI at 1080p
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920;
    viewportUI.Height = 1080;
    m_fontBatch->SetViewport(viewportUI);
    m_spriteBatch->SetViewport(viewportUI);

    const float startX = 50.0f;
    const float startY = 40.0f;
    Vector2 fontPos(startX, startY);

    m_fontBatch->Begin(commandList);
    m_textFont->DrawString(m_fontBatch.get(), L"SimpleSFS Sample", fontPos, White, 0.0f, g_XMZero, 1.0f);
  
    wchar_t strText[2048] = {};

    float x = 50.0f;
    float y = 150.0f;
    float spaceX = 100.0f;
    float spaceY = 30.0f;
    float scale = 0.5f;

    // Stats about the tiled texture
    {
        UINT numCommittedTiles = 0;
        for (Tile& tile : m_tiledTexture.Tiles)
        {
            numCommittedTiles += (tile.Status == Tile::TileStatus::Loaded);
        }

        const float reserved = m_tiledTexture.NumTiles * D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES / 1024.0f / 1024.0f;
        const float committed = numCommittedTiles * D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES / 1024.0f / 1024.0f;
        const float memoryPoolSize = float(g_NumTilesInMemoryPool) * D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES / 1024.0f / 1024.0f;
        const float percentageUsed = numCommittedTiles * 100.0f / float(g_NumTilesInMemoryPool);

        fontPos.x = x;
        fontPos.y = y;
        m_textFont->DrawString(m_fontBatch.get(), L"Memory Pool", fontPos, White, 0.0f, g_XMZero, scale); fontPos.y += spaceY;

        swprintf_s(strText, L"   Num Tiles: %d", g_NumTilesInMemoryPool);
        m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, White, 0.0f, g_XMZero, scale); fontPos.y += spaceY;

        swprintf_s(strText, L"  Total size: %3.1f MBytes", memoryPoolSize);
        m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, White, 0.0f, g_XMZero, scale); fontPos.y += spaceY;

        swprintf_s(strText, L" Memory used: %3.1f MBytes, %3.0f%%", committed, percentageUsed);

        // Add a warning about memory being filled up above 90%
        if (percentageUsed > 90.0f)
        {
            m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, Colors::Yellow, 0.0f, g_XMZero, scale * 1.25f); fontPos.y += spaceY;
        }
        else
        {
            m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, White, 0.0f, g_XMZero, scale); fontPos.y += spaceY;
        }

        fontPos.y += spaceY;
        m_textFont->DrawString(m_fontBatch.get(), L"Tiled Texture", fontPos, White, 0.0f, g_XMZero, scale); fontPos.y += spaceY;

        swprintf_s(strText, L" Num Tiles: %d, tile size = %d x %d", m_tiledTexture.NumTiles, m_tiledTexture.TileShape.WidthInTexels, m_tiledTexture.TileShape.HeightInTexels);
        m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, White, 0.0f, g_XMZero, scale); fontPos.y += spaceY;

        swprintf_s(strText, L"  Reserved: %3.1f MBytes", reserved);
        m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, White, 0.0f, g_XMZero, scale); fontPos.y += spaceY;

        swprintf_s(strText, L" Committed: %3.1f MBytes, %3.1f%% of texture is resident", committed, committed * 100.0f / reserved);
        m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, White, 0.0f, g_XMZero, scale); fontPos.y += spaceY;
    }

    // The UI only has enough space to show two grids of 8x8 values, so if there are more values, just don't show any of them
    if (m_feedbackMapWidth <= 8 || m_feedbackMapHeight <= 8)
    {
        y = 450.0f;
        fontPos.x = x;
        fontPos.y = y;
        m_textFont->DrawString(m_fontBatch.get(), L"MinMip Map values - Mips currently resident", fontPos, White, 0.0f, g_XMZero, scale);
        fontPos.y += spaceY;

        assert(m_pMinMipMapData);
        auto pRow = m_pMinMipMapData;

        // Show the MinMip map values
        for (size_t j = 0; j < m_feedbackMapHeight; j++)
        {
            fontPos.x = x;

            for (size_t i = 0; i < m_feedbackMapWidth; i++)
            {
                swprintf_s(strText, L"%3d", FeedbackValueToMipValue(pRow[i]));
                m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, White, 0.0f, g_XMZero, scale);
                fontPos.x += spaceX;
            }

            pRow += m_minMipMapPitch;
            fontPos.y += spaceY;
        }

        fontPos.x = x;
        fontPos.y += spaceY;
        m_textFont->DrawString(m_fontBatch.get(), L"Feedback Map values - Mips requested during scene rendering", fontPos, White, 0.0f, g_XMZero, scale);
        fontPos.y += spaceY;

        // Show the feedback map values
        assert(m_pFeedbackMapData);
        pRow = m_pFeedbackMapData;

        for (size_t j = 0; j < m_feedbackMapHeight; j++)
        {
            fontPos.x = x;

            for (size_t i = 0; i < m_feedbackMapWidth; i++)
            {
                auto feedbackValue = pRow[i];
                swprintf_s(strText, L"%1.3f", DecodeFeedbackMapValue(feedbackValue));
                m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, White, 0.0f, g_XMZero, scale);
                fontPos.x += spaceX;
            }

            fontPos.y += spaceY;
            pRow += m_feedbackMapStagingTexture.CopyLocation.PlacedFootprint.Footprint.RowPitch;
        }
    }

    fontPos.x = x;
    fontPos.y = viewportUI.Height - 45;
    DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[LThumb] - Move camera", fontPos, White, 0.65f);

    // Legend for mip tile visualization
    LONG size = 250;
    LONG left = 1920 - size - 50;
    LONG top = 600;

    fontPos.x = static_cast<float>(left);
    fontPos.y = static_cast<float>(top - 40);
    m_textFont->DrawString(m_fontBatch.get(), L"Tiles in Mip 0", fontPos, White, 0.0f, g_XMZero, scale);
    fontPos.y = static_cast<float>(top + size + 40);
    m_textFont->DrawString(m_fontBatch.get(), L"Tiles in Mips 1-4", fontPos, White, 0.0f, g_XMZero, scale);

    m_fontBatch->End();

    // Visualize the tiles in each mip level
    m_spriteBatch->Begin(commandList);
    VisualizeMip(0, left, top, size + 9); top += size + 25 + 10 + 45; size /= 2;
    VisualizeMip(1, left, top, size); left += size + 20; size /= 2;
    VisualizeMip(2, left, top, size); left += size + 20; size /= 2;
    VisualizeMip(3, left, top, size); top += size + 16; size /= 2;
    VisualizeMip(4, left, top, size);
    m_spriteBatch->End();


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

#pragma region Streaming

// Generate streaming requests from the feedback map values
void Sample::GenerateNewStreamingRequests()
{
    if (m_feedbackMapResource)
    {
        auto commandList = m_deviceResources->GetCommandList();
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"GenerateNewStreamingRequests");

        // Copy feedback values to the staging buffer
        TransitionResource(commandList, m_feedbackMapResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        CD3DX12_TEXTURE_COPY_LOCATION SrcLocation(m_feedbackMapResource.Get(), 0);
        commandList->CopyTextureRegion(&m_feedbackMapStagingTexture.CopyLocation, 0, 0, 0, &SrcLocation, nullptr);
        TransitionResource(commandList, m_feedbackMapResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        // Read the values on the CPU
        assert(m_pFeedbackMapData);
        auto pRow = m_pFeedbackMapData;

        // Generate a streaming request for each value in the feedback map
        for (UINT y = 0; y < m_feedbackMapHeight; y++)
        {
            for (UINT x = 0; x < m_feedbackMapWidth; x++)
            {
                auto feedbackValue = pRow[x];
                GenerateStreamingRequest(x, y, feedbackValue);
            }

            pRow += m_feedbackMapStagingTexture.CopyLocation.PlacedFootprint.Footprint.RowPitch;
        }

        // Sort the requests, prioritizing based on the fractional portion of the 5.3 fixed point feedback map value.
        // E.g. a requested tile with feedback value of 2.125 could be interpreted as having a higher streaming priority
        // than a requested tile with feedback value 2.875, since it's closer to possibly requesting the next higher detailed mip 1
        std::sort(m_newStreamingRequests.begin(), m_newStreamingRequests.end(), StreamingRequest::Compare);

        PIXEndEvent(commandList);
    }
}

void Sample::GenerateStreamingRequest(UINT feedbackMapX, UINT feedbackMapY, BYTE encodedRequestedMip)
{
    if (m_newStreamingRequests.size() >= size_t(g_MaxNumStreamingRequests))
    {
        return;
    }

    // Find current resident mip from the MinMip map
    assert(m_pMinMipMapData);
    auto pRow = m_pMinMipMapData;
    pRow += (feedbackMapY * m_minMipMapPitch);
    auto residentMip = FeedbackValueToMipValue(pRow[feedbackMapX]);

    // Use the fractional portion for the feedback map to decide which mip level is more important to stream.
    // In this simple case, if the fractional portion is below 0.5, we use the lower mip value, otherwise the
    // higher mip value. E.g. if the feedback value is 0.2 we stream in mip 0, but if it's 0.8, we stream in mip 1
    auto decodedFeedbackValue = DecodeFeedbackMapValue(encodedRequestedMip);
    auto requestedMip = static_cast<UINT>(decodedFeedbackValue + 0.5f);

    // Check if higher detailed mip is already resident. If so, we don't need to generate a
    // streaming request, but we do need to update the tile's time stamp for correct aging out
    if (requestedMip >= residentMip)
    {
        // (feedbackMapX, feedbackMapY) is the texel position in the feedback map, so convert it to tile coordinates
        auto tileX = feedbackMapX >> requestedMip;
        auto tileY = feedbackMapY >> requestedMip;

        // Get the tile for the mip requested
        auto pTile = m_tiledTexture.GetTile(tileX, tileY, requestedMip);
        if (pTile == nullptr)
        {
            return;
        }

        // Update when we last saw this tile so that we can prioritize which tiles to age out
        pTile->SetTimeLastSeen(m_timer.GetTotalTicks());

        return;
    }

    // Check that we don't skip loading a mip, e.g. if mip 4 is resident, but we request 0, we also need the mips inbetween.
    // In this simple case we simply load the next higher detail mip, not all of them, e.g. load mip 3.
    auto mipDiff = residentMip - requestedMip;
    if (mipDiff > 1)
    {
        auto mipTail = m_tiledTexture.PackedMipInfo.NumStandardMips;
        requestedMip = std::min(static_cast<BYTE>(residentMip - 1), mipTail);
    }

    // (feedbackMapX, feedbackMapY) is the texel position in the feedback map, so convert it to tile coordinates for the specific mip
    auto tileX = feedbackMapX >> requestedMip;
    auto tileY = feedbackMapY >> requestedMip;

    // Get the tile for the mip requested
    auto pTile = m_tiledTexture.GetTile(tileX, tileY, requestedMip);
    if (pTile == nullptr)
    {
        return;
    }

    // Update when we last saw this tile so that we can prioritize which tiles to age out
    pTile->SetTimeLastSeen(m_timer.GetTotalTicks());

    // Check if the requested tile has already been loaded
    if (pTile->Status != Tile::TileStatus::Invalid)
    {
        return;
    }

    // Create a streaming request for this tile. The tile holds info from where it needs to be
    // streamed from disk and where it needs to be mapped to memory
    StreamingRequest streamingRequest;
    streamingRequest.pTile = pTile;
    streamingRequest.FeedbackValue = encodedRequestedMip;
    pTile->Status = Tile::TileStatus::Requested;

    m_newStreamingRequests.push_back(streamingRequest);
}

// Process all new streaming requests
void Sample::ProcessNewStreamingRequests()
{
    if (m_newStreamingRequests.empty())
    {
        return;
    }

    auto cmdList = m_deviceResources->GetCommandList();

    wchar_t strText[1024] = {};
    swprintf_s(strText, L"ProcessNewStreamingRequests: Num requests %zu", m_newStreamingRequests.size());
    PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, strText);

    auto cmdQueue = m_deviceResources->GetCommandQueue();
    auto pTiledTexture = m_tiledTexture.D3dResource.Get();
    auto textureDesc = pTiledTexture->GetDesc();

    while (!m_newStreamingRequests.empty())
    {
        // Check if there is space in the memory pool
        if (m_availableTilesInMemoryPool.empty())
        {
            break;
        }

        auto streamingRequest = m_newStreamingRequests.back();
        m_newStreamingRequests.pop_back();

        auto pTile = streamingRequest.pTile;
        if (pTile == nullptr)
        {
            continue;
        }

        // Check that the tile is still requested, maybe it's already loaded
        if (pTile->Status != Tile::TileStatus::Requested)
        {
            continue;
        }

        // Get space in the memory pool for this tile
        pTile->IndexInMemoryPool = GetAvailableTileInMemoryPool();
        assert(pTile->IndexInMemoryPool >= 0);

        // Update tile mappings
        {
            PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, L"UpdateTileMappings");

            const D3D12_TILE_RANGE_FLAGS rangeFlag = D3D12_TILE_RANGE_FLAG_NONE;
            UINT rangeTileCount = 1;
            UINT indexInMemoryPool = static_cast<UINT>(pTile->IndexInMemoryPool);

            // Check if this is the miptail tile
            const auto mipTail = m_tiledTexture.PackedMipInfo.NumStandardMips;
            if (streamingRequest.pTile->Coord.Subresource >= mipTail)
            {
                streamingRequest.pTile->Coord = CD3DX12_TILED_RESOURCE_COORDINATE(0, 0, 0, mipTail);
            }

            // Update the tile mappings
            cmdQueue->UpdateTileMappings(pTiledTexture, 1, &streamingRequest.pTile->Coord, &g_OneTile, m_tileMemoryPool.Get(), 1, &rangeFlag, &indexInMemoryPool, &rangeTileCount, D3D12_TILE_MAPPING_FLAG_NONE);

            PIXEndEvent(cmdList);
        }

        // Enqueue DirectStorage requests
        {
            PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, L"Enqueue DirectStorage requests");

            // Find the memory address to stream into
            streamingRequest.StreamingMemoryAddress = m_memoryPoolBaseAddress + pTile->IndexInMemoryPool * D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;

            DSTORAGE_REQUEST_OPTIONS requestOptions = {};
            requestOptions.ZlibDecompress = pTile->ZlibCompressed;
            requestOptions.BcpackMode = pTile->BCPacked ? GetBCPackMode((DXGI_FORMAT)textureDesc.Format) : DSTORAGE_BCPACK_MODE_NONE;
            requestOptions.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;

            // Enqueue a DirectStorage request for streaming this tile
            DSTORAGE_REQUEST request = {};
            request.Options = requestOptions;
            request.File = m_dsFile.Get();
            request.Destination = reinterpret_cast<void*>(streamingRequest.StreamingMemoryAddress);
            request.FileOffset = pTile->OffsetInFile;
            request.SourceSize = pTile->LoadSize;
            request.DestinationSize = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
            request.IntermediateSize = UINT32(pTile->ZlibCompressed && pTile->BCPacked ? pTile->InflatedSize : 0);
            m_dsQueue->EnqueueRequest(&request);

            // Enqueued to be loaded, but not submitted yet
            streamingRequest.pTile->Status = Tile::TileStatus::Loading;
            m_enqueuedStreamingRequests.push_back(streamingRequest);

            PIXEndEvent(cmdList);
        }
    }

    // Submit the batch of DirectStorage requests
    if (!m_enqueuedStreamingRequests.empty())
    {
        PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, L"Submit DirectStorage requests");

        // Enqueue one status notification per batch as opposed to per read request
        m_dsQueue->EnqueueStatus(m_dsStatus.Get(), 0);

        // Submit the entire queue to the hardware
        m_dsQueue->Submit();

        PIXEndEvent(cmdList);
    }

    PIXEndEvent(cmdList);
}

// Process all submitted streaming requests
void Sample::ProcessSubmittedStreamingRequests()
{
    if (m_enqueuedStreamingRequests.empty())
    {
        return;
    }

    auto cmdList = m_deviceResources->GetCommandList();

    wchar_t strText[1024] = {};
    swprintf_s(strText, L"ProcessSubmittedStreamingRequests: Num requests %zu", m_enqueuedStreamingRequests.size());
    PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, strText);

    // Wait for all requests to complete. Note that DirectStorage was not designed to be used in a syncrhonous blocking manner like in this simple sample
    // Refer to the best practices white paper here: https://forums.xboxlive.com/articles/99727/directstorage-best-practices.html
    {
        PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, L"Wait for DirectStorage requests to complete");

        // Wait for the direct storage batch to complete. We only use one batch in this sample
        while (!m_dsStatus->IsComplete(0))
        {
            SwitchToThread();
        }

        if (FAILED(m_dsStatus->GetHResult(0)))
        {
            throw std::exception("Direct Storage failed");
        }

        PIXEndEvent(cmdList);
    }

    {
        PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, L"Update MinMip Map");

        while (!m_enqueuedStreamingRequests.empty())
        {
            auto streamingRequest = m_enqueuedStreamingRequests.back();
            m_enqueuedStreamingRequests.pop_back();

            auto pTile = streamingRequest.pTile;
            if (pTile == nullptr)
            {
                continue;
            }

            // The tile has now been loaded
            streamingRequest.pTile->Status = Tile::TileStatus::Loaded;

            // Flush
            cmdList->FlushPipelineX(D3D12XBOX_FLUSH_TOP_TEXTURE_L1_INVALIDATE | D3D12XBOX_FLUSH_TOP_TEXTURE_L2_INVALIDATE, streamingRequest.StreamingMemoryAddress, D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES);

            // Update MinMip map with the mip value that is now in memory
            assert(m_pMinMipMapData);
            auto pRow = m_pMinMipMapData;

            // Update all the pixels of the tile region
            auto mip = streamingRequest.pTile->Coord.Subresource;
            auto startX = streamingRequest.pTile->Coord.X << mip;
            auto startY = streamingRequest.pTile->Coord.Y << mip;
            auto stopX = (streamingRequest.pTile->Coord.X + 1) << mip;
            auto stopY = (streamingRequest.pTile->Coord.Y + 1) << mip;

            pRow += (startY * m_minMipMapPitch);
            for (auto j = startY; j < stopY; j++)
            {
                for (auto i = startX; i < stopX; i++)
                {
                    pRow[i] = std::min(pRow[i], MipValueToFeedbackValue(static_cast<BYTE>(mip)));
                }
                pRow += m_minMipMapPitch;
            }
        }

        PIXEndEvent(cmdList);
    }

    PIXEndEvent(cmdList);
}

// Unmap a tile if it's loaded and old enough
void Sample::UnmapTile(Tile* pTile, ID3D12CommandQueue* cmdQueue, ID3D12Resource* pTiledTexture)
{
    assert(pTile);
    if ((pTile->Status != Tile::TileStatus::Loaded))
    {
        return;
    }

    auto elapsedTimeSinceLaunched = m_timer.GetTotalTicks();
    auto tileAge = m_timer.TicksToSeconds(elapsedTimeSinceLaunched - pTile->TimeLastSeen);

    if (tileAge < g_TileAgeOutInSeconds)
    {
        return;
    }

    // Unmap the tile
    cmdQueue->UpdateTileMappings(pTiledTexture, 1, &pTile->Coord, &g_OneTile, nullptr, 1, &g_ClearTileMappingFlag, nullptr, nullptr, D3D12_TILE_MAPPING_FLAG_NONE);

    // Make the slot in the memory pool available again
    assert(pTile->IndexInMemoryPool >= 0);
    m_availableTilesInMemoryPool.push(pTile->IndexInMemoryPool);

    // Reset the tile
    pTile->Status = Tile::TileStatus::Invalid;
    pTile->IndexInMemoryPool = -1;
    pTile->TimeLastSeen = 0;

    // Update MinMip map with the mip value that is now in memory
    assert(m_pMinMipMapData);
    auto pRow = m_pMinMipMapData;

    // Update all the pixels of the tile region
    auto mip = pTile->Coord.Subresource;
    auto startX = pTile->Coord.X << mip;
    auto startY = pTile->Coord.Y << mip;
    auto stopX = (pTile->Coord.X + 1) << mip;
    auto stopY = (pTile->Coord.Y + 1) << mip;

    pRow += (startY * m_minMipMapPitch);
    for (auto j = startY; j < stopY; j++)
    {
        for (auto i = startX; i < stopX; i++)
        {
            pRow[i] = MipValueToFeedbackValue(static_cast<BYTE>(mip + 1));
        }
        pRow += m_minMipMapPitch;
    }
}

// Age out tiles
void Sample::AgeOutTiles()
{
    auto cmdQueue = m_deviceResources->GetCommandQueue();
    auto pTiledTexture = m_tiledTexture.D3dResource.Get();

    for (UINT mip = 0; mip < m_tiledTexture.PackedMipInfo.NumStandardMips; mip++)
    {
        for (UINT y = 0; y < m_tiledTexture.TilingInfo[mip].HeightInTiles; y++)
        {
            for (UINT x = 0; x < m_tiledTexture.TilingInfo[mip].WidthInTiles; x++)
            {
                auto pTile = m_tiledTexture.GetTile(x, y, mip);
                UnmapTile(pTile, cmdQueue, pTiledTexture);
            }
        }
    }
}

// Reset all streaming
void Sample::ResetStreaming()
{
    auto cmdQueue = m_deviceResources->GetCommandQueue();
    auto pTiledTexture = m_tiledTexture.D3dResource.Get();

    // Reset all tiles
    for (Tile& tile : m_tiledTexture.Tiles)
    {
        tile.Status = Tile::TileStatus::Invalid;
        tile.IndexInMemoryPool = -1;
        tile.TimeLastSeen = 0;
    }

    // Clear all tile mappings
    cmdQueue->UpdateTileMappings(pTiledTexture, 1, nullptr, nullptr, nullptr, 1, &g_ClearTileMappingFlag, nullptr, nullptr, D3D12_TILE_MAPPING_FLAG_NONE);

    // Clear the queue and set all the tiles in the pool as available again
    m_availableTilesInMemoryPool = std::queue<int>();
    for (int i = 0; i < g_NumTilesInMemoryPool; i++)
    {
        m_availableTilesInMemoryPool.push(i);
    }

    // Reset the MinMip map
    {
        assert(m_pMinMipMapData);
        auto pRow = m_pMinMipMapData;

        for (size_t j = 0; j < m_feedbackMapHeight; j++)
        {
            for (size_t i = 0; i < m_feedbackMapWidth; i++)
            {
                pRow[i] = g_InvalidFeedbackValue;
            }

            pRow += m_minMipMapPitch;
        }
    }

    // Stream in the mip tail
    auto mipTail = m_tiledTexture.PackedMipInfo.NumPackedMips;
    GenerateStreamingRequest(0, 0, MipValueToFeedbackValue(mipTail));
}

// Find an available slot in the tile memory pool
int Sample::GetAvailableTileInMemoryPool()
{
    if (m_availableTilesInMemoryPool.empty())
    {
        return -1;
    }

    auto tile = m_availableTilesInMemoryPool.front();
    m_availableTilesInMemoryPool.pop();
    return tile;
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

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Create the vertex buffer
    {
        static const Vertex s_vertices[] =
        {
            { XMFLOAT3(-1.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
            { XMFLOAT3(1.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
            { XMFLOAT3(1.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
            { XMFLOAT3(-1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
        };

        DX::ThrowIfFailed(
            CreateStaticBuffer(device, resourceUpload, s_vertices, std::size(s_vertices), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                m_vertexBuffer.ReleaseAndGetAddressOf()));

        // Initialize the vertex buffer view
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = sizeof(s_vertices);
    }

    // Create the index buffer
    {
        static const uint16_t s_indices[] =
        {
            0, 1, 3,
            3, 1, 2,           
        };

        DX::ThrowIfFailed(
            CreateStaticBuffer(device, resourceUpload, s_indices, std::size(s_indices), D3D12_RESOURCE_STATE_INDEX_BUFFER,
                m_indexBuffer.ReleaseAndGetAddressOf()));

        // Initialize the index buffer view.
        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        m_indexBufferView.SizeInBytes = sizeof(s_indices);
    }

    // Create the constant buffer memory and map the CPU and GPU addresses
    {
        const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const D3D12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ConstantBuffer));

        DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_GRAPHICS_PPV_ARGS(m_constantBuffer.ReleaseAndGetAddressOf())));

        DX::ThrowIfFailed(m_constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_pMappedConstantBufferData)));
        m_constantBufferDataGpuAddr = m_constantBuffer->GetGPUVirtualAddress();
    }

    // Load shader blobs
    auto vertexShaderBlob = DX::ReadData(L"VertexShader.cso");
    auto pixelShaderBlob = DX::ReadData(L"PixelShader.cso");

    // Create a root signature. Xbox One best practice is to use HLSL-based root signatures to support shader precompilation.
    DX::ThrowIfFailed(device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(), IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

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

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    m_resourceDescriptorHeap = std::make_unique<DescriptorHeap>(device, static_cast<UINT>(ResourceDescriptors::Count));
    m_samplerDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, static_cast<UINT>(SamplerDescriptors::Count));

    // Load fonts
    {
        // Fonts
        RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
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

        {
            SpriteBatchPipelineStateDescription desc(rtState, &CommonStates::AlphaBlend);
            m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, desc);

            // Load the untiled version of the tiled texture for debug visualizations
            DX::ThrowIfFailed(CreateDDSTextureFromFile(device, resourceUpload, m_debugTextureFileName.c_str(), m_debugTexture.ReleaseAndGetAddressOf()));
            device->CreateShaderResourceView(m_debugTexture.Get(), nullptr, m_resourceDescriptorHeap->GetCpuHandle(static_cast<int>(ResourceDescriptors::DebugTexture)));
        }
    }

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());

    // Wait for the upload thread to terminate
    uploadResourcesFinished.wait();

    auto commandList = m_deviceResources->GetCommandList();
    commandList->Reset(m_deviceResources->GetCommandAllocator(), nullptr);

    // Create the memory pool for resident tiles
    {
        const UINT heapSize = g_NumTilesInMemoryPool * D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
        CD3DX12_HEAP_DESC heapDesc(heapSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES, D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES);
        device->CreateHeap(&heapDesc, __uuidof(m_tileMemoryPool), (void**)&m_tileMemoryPool);
        m_tileMemoryPool->SetName(L"Memory Pool for Tiles");
    }

    // Create the tiled texture from the compressed xbtc file created by the xbtc.exe tool in the GDK
    // This sample's texture was created using the following command line:
    // "xbtc.exe ToyRobot_BaseColor.dds -o ToyRobot_BaseColor.xbtc --streamsize 64KB -e BC1"
    if (!m_tiledTexture.Create(device, m_tiledTextureFileName))
    {
        throw std::exception("Couldn't create tiled texture");
    }

    // Get the base address of the heap holding the tiles, so that we can stream directly into graphics memory
    {
        ID3D12Heap1* pHeap1 = nullptr;
        m_tileMemoryPool->QueryInterface(__uuidof(pHeap1), (void**)&pHeap1);
        m_memoryPoolBaseAddress = pHeap1->GetGpuAddressX();

        if (!m_memoryPoolBaseAddress)
        {
            throw std::exception("Invalid base address for tile memory pool");
        }
    }

    auto pTiledTextureResource = m_tiledTexture.D3dResource.Get();
    device->CreateShaderResourceView(pTiledTextureResource, nullptr, m_resourceDescriptorHeap->GetCpuHandle(static_cast<int>(ResourceDescriptors::TiledTextureSRV)));

    const auto width = pTiledTextureResource->GetDesc().Width;
    const auto height = pTiledTextureResource->GetDesc().Height;
    const auto tileWidthTexels = m_tiledTexture.TileShape.WidthInTexels;
    const auto tileHeightTexels = m_tiledTexture.TileShape.HeightInTexels;

    // Create feedback map
    {
        // We create a "MinMip" feedback map, so use the format FEEDBACK_MIN_MIP_OPAQUE
        D3D12_RESOURCE_DESC feedbackMapDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        D3D12_RESOURCE_DESC1 feedbackMapDesc1;
        memcpy(&feedbackMapDesc1, &feedbackMapDesc, sizeof(feedbackMapDesc));
        feedbackMapDesc1.SamplerFeedbackMipRegion.Width = tileWidthTexels;
        feedbackMapDesc1.SamplerFeedbackMipRegion.Height = tileHeightTexels;
        feedbackMapDesc1.SamplerFeedbackMipRegion.Depth = 1;
        feedbackMapDesc1.Alignment = static_cast<UINT64>(D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT);

        // Create the feedback map texture
        CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_DEFAULT);
        device->CreateCommittedResource2(&HeapProps, D3D12_HEAP_FLAG_NONE, &feedbackMapDesc1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, nullptr, __uuidof(m_feedbackMapResource), (void**)&m_feedbackMapResource);
        m_feedbackMapResource->SetName(L"FeedbackMap");

        // Use this specific API to create a feedback map UAV and pair it with its tiled texture
        D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
        UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        device->CreateSamplerFeedbackUnorderedAccessView(pTiledTextureResource, m_feedbackMapResource.Get(), m_resourceDescriptorHeap->GetCpuHandle(static_cast<int>(ResourceDescriptors::FeedbackMapUAV)));

        // Create a readback texture for the streaming system to access feedback values on the CPU
        {
            // The resolve texture needs to be the logical size of the feedback map
            m_feedbackMapWidth = static_cast<UINT>(width / tileWidthTexels);
            m_feedbackMapHeight = height / tileHeightTexels;

            // The values in the feedback map are 8-bit, so use FORMAT_R8
            // NOTE: Using R8_UINT returns regular feedback map values. Using R8_TYPELESS returns the Scarlett specific 5.3 fix point value
            auto resolveDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_TYPELESS, m_feedbackMapWidth, m_feedbackMapHeight);
            resolveDesc.MipLevels = 1;  // Has to be 1 mip, since we're using a MinMip feedback map, not a RegionUsed feedback map

            CreateStagingBufferForTexture(device, &resolveDesc, true, m_feedbackMapStagingTexture.StagingBuffer, &m_feedbackMapStagingTexture.CopyLocation, &m_feedbackMapStagingTexture.CopyRange);
            m_feedbackMapStagingTexture.Desc = resolveDesc;
            m_feedbackMapStagingTexture.StagingBuffer.Get()->SetName(L"FeedbackMap Readback Texture");

            m_pFeedbackMapData = static_cast<BYTE*>(m_feedbackMapStagingTexture.Map());
            assert(m_pFeedbackMapData);
        }
    }

    // Create MinMip map to keep track tile residency
    {
        D3D12_RESOURCE_DESC minMipMapDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UNORM, m_feedbackMapWidth, m_feedbackMapHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        minMipMapDesc.Layout = D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_LINEAR;
        minMipMapDesc.Alignment = static_cast<UINT64>(D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT);

        D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(D3D12XBOX_NODE_MASK, 1, &minMipMapDesc);

        // Alloc memory and clear to 0xff, not 0, indicating no mips are currently in memory
        m_minMipMapSizeBytes = static_cast<UINT32>(allocInfo.SizeInBytes);
        m_pMinMipMapData = (BYTE*)XMemAlloc(m_minMipMapSizeBytes, g_PlacementAttributes);
        if (!m_pMinMipMapData)
        {
            throw std::bad_alloc();
        }
        memset(m_pMinMipMapData, g_InvalidFeedbackValue, m_minMipMapSizeBytes);

        device->CreatePlacedResourceX((D3D12_GPU_VIRTUAL_ADDRESS)m_pMinMipMapData, &minMipMapDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, __uuidof(ID3D12Resource), (void**)&m_minMipMapResource);
        m_minMipMapResource->SetName(L"MinMipMap");

        // Specific API for a MinMip UAV
        device->CreateMinMipShaderResourceViewX(pTiledTextureResource, nullptr, m_minMipMapResource.Get(), nullptr, m_resourceDescriptorHeap->GetCpuHandle(static_cast<int>(ResourceDescriptors::MinMipMapSRV)));

        XG_RESOURCE_LAYOUT layout = {};
        XGTextureAddressComputer* pTexComputer = nullptr;
        XGCreateTextureComputer((XG_RESOURCE_DESC*)&minMipMapDesc, &pTexComputer);
        pTexComputer->GetResourceLayout(&layout);
        m_minMipMapPitch = layout.Plane[0].MipLayout->PitchBytes;

        // Scarlett specific sampler for the MinMip map
        D3D12XBOX_SAMPLER_DESC SamplerDesc = {};
        SamplerDesc.FilterMag = D3D12XBOX_TEXTURE_XY_FILTER_BILINEAR;
        SamplerDesc.FilterMin = D3D12XBOX_TEXTURE_XY_FILTER_BILINEAR;
        SamplerDesc.FilterMip = D3D12XBOX_TEXTURE_MIP_FILTER_LINEAR;
        SamplerDesc.FilterZ = D3D12XBOX_TEXTURE_Z_FILTER_LINEAR;
        SamplerDesc.FilterMode = D3D12XBOX_TEXTURE_FILTER_MODE_LERP;
        SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        SamplerDesc.MipLODBias = 0;
        SamplerDesc.MaxAnisotropy = 16;
        SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        SamplerDesc.BorderColor[0] = 0.0f;
        SamplerDesc.BorderColor[1] = 0.0f;
        SamplerDesc.BorderColor[2] = 0.0f;
        SamplerDesc.BorderColor[3] = 0.0f;
        SamplerDesc.MinLOD = 0.0f;
        SamplerDesc.MaxLOD = 9999.0f;

        // MinMip map specific fields
        SamplerDesc.MinMipSlopeU = D3D12XBOX_MINMIP_FILTER_SLOPE_5;
        SamplerDesc.MinMipSlopeVOrW = D3D12XBOX_MINMIP_FILTER_SLOPE_5;
        SamplerDesc.MinMipOffsetU = D3D12XBOX_MINMIP_FILTER_OFFSET_1_32;
        SamplerDesc.MinMipOffsetVOrW = D3D12XBOX_MINMIP_FILTER_OFFSET_1_32;
        SamplerDesc.Flags |= D3D12XBOX_SAMPLER_FLAG_ENABLE_MINMIP_FILTER;

        device->CreateSamplerX(&SamplerDesc, m_samplerDescriptorHeap->GetCpuHandle(static_cast<int>(SamplerDescriptors::MinMipMapSampler)));
    }

    DX::ThrowIfFailed(commandList->Close());
    m_deviceResources->GetCommandQueue()->ExecuteCommandLists(1, CommandListCast(&commandList));

    // Wait until assets have been uploaded to the GPU.
    m_deviceResources->WaitForGpu();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Initialize the projection matrix
    auto size = m_deviceResources->GetOutputSize();
    XMMATRIX projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), 0.001f, 1000.0f);
    XMStoreFloat4x4(&m_projectionMatrix, projection);

    // Initialize camera
    m_camera = std::make_unique<DX::FlyCamera>();
    m_camera->SetWindow(static_cast<int>(m_deviceResources->GetScreenViewport().Width), static_cast<int>(m_deviceResources->GetScreenViewport().Height));
    const XMVECTORF32 startPos = { 0.0f, 0.2f, 2.0f, 1.0f };
    m_camera->SetPosition(startPos);
    m_camera->SetSensitivity(1.0f, 1.0f, 1.0f, 0.0f);
    m_camera->SetFlags(DX::FlyCamera::c_FlagsDisableRotation);
    m_camera->SetRotation(Quaternion::CreateFromAxisAngle(Vector3::Right, -XM_PI / 9.0f));

    BoundingBox bbox(XMFLOAT3(0.0f, 0.2f, 0.5f), XMFLOAT3(5.0f, 0.0f, 5.0f));
    m_camera->SetBoundingBox(bbox);
}

void Sample::CreateStagingBufferForTexture(ID3D12Device* pd3dDevice, const D3D12_RESOURCE_DESC* pTextureDesc, bool readback, ComPtr<ID3D12Resource>& buffer, D3D12_TEXTURE_COPY_LOCATION* pStagingCopyLocation, D3D12_RANGE* pEntireRange)
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

    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(totalBytes);
    D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(readback ? D3D12_HEAP_TYPE_READBACK : D3D12_HEAP_TYPE_UPLOAD);
    DX::ThrowIfFailed(pd3dDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                    readback ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)buffer.ReleaseAndGetAddressOf()));

    if (pStagingCopyLocation != nullptr)
    {
        pStagingCopyLocation->Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        pStagingCopyLocation->pResource = buffer.Get();
        copyLocation.Offset = 0;
        pStagingCopyLocation->PlacedFootprint = copyLocation;
    }

    if (pEntireRange != nullptr)
    {
        pEntireRange->Begin = 0;
        pEntireRange->End = totalBytes;
    }
}

StagingTexture::StagingTexture() : pLockedBits(nullptr)
{
    CopyLocation = {};
    CopyRange = {};
    Desc = {};
}

void* StagingTexture::Map()
{
    if (StagingBuffer != nullptr)
    {
        HRESULT hr = StagingBuffer->Map(0, nullptr, &pLockedBits);
        return SUCCEEDED(hr) ? pLockedBits : nullptr;
    }
    return pLockedBits;
}

void StagingTexture::Unmap()
{
    if (StagingBuffer != nullptr)
    {
        StagingBuffer->Unmap(0, nullptr);
        pLockedBits = nullptr;
    }
}

#pragma endregion
