//--------------------------------------------------------------------------------------
// DirectStorageTextureShuffling.cpp
//
// DirectStorage on Xbox Series X|S provides several special swizzle modes that are compatible with the
// memory decompression unit (MDU). These modes are designed to provide additional file size savings for
// compressed texture data when used in conjunction with BCPACK or ZLib based compression.
//
// This sample demonstrates how to use these DirectStorage swizzle modes together with shuffling (deinterleaving)
// the data of BC1, BC3, BC4 and BC5 textures in specific ways to improve ZLib compression.The sample consists of
// an offline tool to prepare the texture data, and runtime code with compute shaders to unshuffle the texture data.
// We've seen up to 10% file size savings using this method.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DirectStorageTextureShuffling.h"

#include "ATGColors.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;

namespace
{
    static constexpr uint32_t c_numTextures = 4;

    static std::wstring c_textureID[c_numTextures]  = { L"BC1", L"BC3", L"BC4", L"BC5" };

    static std::wstring c_inputFiles[c_numTextures] = { L".\\Assets\\Textures\\BC1_shuffled_compressed.bin",
                                                        L".\\Assets\\Textures\\BC3_shuffled_compressed.bin",
                                                        L".\\Assets\\Textures\\BC4_shuffled_compressed.bin",
                                                        L".\\Assets\\Textures\\BC5_shuffled_compressed.bin" };

    enum Descriptors
    {
        FontSRV,
        BC1TextureSRV,
        BC3TextureSRV,
        BC4TextureSRV,
        BC5TextureSRV,
        Count
    };

    uint32_t RoundTo64K(uint32_t size) { return (size + 65536 - 1) & ~(65536 - 1); }
}

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_unshuffleTextureFence(0),
    m_unshuffleTextureFenceEvent(nullptr),
    m_unshuffleTextureFenceValue(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN, 2, DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);
    m_deviceResources->SetClearColor(ATG::Colors::Background);

    m_textureMetadata.resize(c_numTextures);
    m_unshufflePSOs.resize(c_numTextures);
    m_textureMemory.resize(c_numTextures);
    m_textures.resize(c_numTextures);
    m_textureDataBuffers.resize(c_numTextures);
    m_textureUploadSizes.resize(c_numTextures);
    m_dsFiles.resize(c_numTextures);
}

// Initialize DirectStorage
void Sample::InitDirectStorage()
{
    DX::ThrowIfFailed(::DStorageGetFactory(__uuidof(IDStorageFactoryX1), reinterpret_cast<void**>(m_dsFactory.ReleaseAndGetAddressOf())));

    DSTORAGE_QUEUE_DESC queueDesc = {};
    queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
    queueDesc.Capacity = DSTORAGE_MIN_QUEUE_CAPACITY;
    queueDesc.Priority = DSTORAGE_PRIORITY_REALTIME;
    queueDesc.Name = "m_dsQueue";

    DX::ThrowIfFailed(m_dsFactory->CreateQueue(&queueDesc, __uuidof(IDStorageQueueX1), reinterpret_cast<void**>(m_dsQueue.ReleaseAndGetAddressOf())));

    m_dsFactory->SetDebugFlags(DSTORAGE_DEBUG_BREAK_ON_ERROR | DSTORAGE_DEBUG_SHOW_ERRORS | DSTORAGE_DEBUG_RECORD_OBJECT_NAMES);
}

// Load metadata from shuffled texture files
void Sample::LoadTextureMetadata()
{
    for (uint32_t i = 0; i < c_numTextures; i++)
    {
        DX::ThrowIfFailed(m_dsFactory->OpenFile(c_inputFiles[i].c_str(), IID_PPV_ARGS(m_dsFiles[i].ReleaseAndGetAddressOf())));

        // Texture metadata is not compressed
        DSTORAGE_REQUEST_OPTIONS requestOptions = {};
        requestOptions.ZlibDecompress = 0;
        requestOptions.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        requestOptions.BcpackMode = DSTORAGE_BCPACK_MODE_NONE;
        requestOptions.SwizzleMode = DSTORAGE_SWIZZLE_MODE_NONE;

        DSTORAGE_REQUEST request = {};
        request.Options = requestOptions;
        request.File = m_dsFiles[i].Get();
        request.FileOffset = 0;
        request.SourceSize = sizeof(ShuffledTextureMetadata);
        request.Destination = &m_textureMetadata[i];
        request.DestinationSize = sizeof(ShuffledTextureMetadata);

        m_dsQueue->EnqueueRequest(&request);
    }

    // Submit the DS requests
    auto textureMetadataLoadedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    m_dsQueue->EnqueueSetEvent(textureMetadataLoadedEvent);
    m_dsQueue->Submit();

    // Wait for DS work to complete
    WaitForSingleObject(textureMetadataLoadedEvent, INFINITE);
}

// Load shuffled texture data
void Sample::LoadTextureData()
{
    // This is a simple sample showing the end-to-end loading and processing of the texture data, so we load the shuffled texture
    // data every frame. Therefore, wait here until the GPU is done processing the shuffled texture data from the previous frame
    if (m_unshuffleTextureFence->GetCompletedValue() < m_unshuffleTextureFenceValue)
    {
        m_unshuffleTextureFence->SetEventOnCompletion(m_unshuffleTextureFenceValue, m_unshuffleTextureFenceEvent);
        PIXBeginEvent(PIX_COLOR_DEFAULT, L"Wait for GPU to finish with shuffled data");
        WaitForSingleObject(m_unshuffleTextureFenceEvent, INFINITE);
        PIXEndEvent();
    }
    m_unshuffleTextureFenceValue++;

    for (uint32_t i = 0; i < c_numTextures; i++)
    {
        PIXScopedEvent(PIX_COLOR_DEFAULT, L"Enqueue Load %s", c_textureID[i].c_str());

        assert(m_textureMemory[i]);
        assert(m_dsFiles[i].Get());

        const ShuffledTextureMetadata& textureMetadata = m_textureMetadata[i];

        // Texture data is pre-swizzled, compressed and shuffled
        DSTORAGE_REQUEST_OPTIONS requestOptions = {};
        requestOptions.ZlibDecompress = 1;
        requestOptions.BcpackMode = DSTORAGE_BCPACK_MODE_NONE;
        requestOptions.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        requestOptions.SwizzleMode = static_cast<DSTORAGE_SWIZZLE_MODE>(textureMetadata.swizzleMode);

        DSTORAGE_REQUEST DSRequest = {};
        DSRequest.Options = requestOptions;
        DSRequest.Destination = reinterpret_cast<void*>(m_textureMemory[i]);
        DSRequest.DestinationSize = m_textureUploadSizes[i];
        DSRequest.File = m_dsFiles[i].Get();
        DSRequest.FileOffset = sizeof(ShuffledTextureMetadata);
        DSRequest.SourceSize = textureMetadata.loadSize;

        m_dsQueue->EnqueueRequest(&DSRequest);
    }

    // Ask DS to signal the D3D fence when requests are done
    m_dsQueue->EnqueueSignal(m_unshuffleTextureFence.Get(), m_unshuffleTextureFenceValue);
    m_dsQueue->Submit();
}

// Unshuffle texture data using compute shaders
void Sample::UnshuffleTextureData()
{
    DX::ThrowIfFailed(m_unshuffleCmdAllocator->Reset());
    DX::ThrowIfFailed(m_unshuffleCmdList->Reset(m_unshuffleCmdAllocator.Get(), m_unshufflePSOs[0].Get()));

    m_unshuffleCmdList->SetComputeRootSignature(m_unshuffleRootSignature.Get());

    for (uint32_t i = 0; i < c_numTextures; i++)
    {
        PIXBeginEvent(m_unshuffleCmdList.Get(), PIX_COLOR_DEFAULT, L"Unshuffle %s", c_textureID[i].c_str());

        const ShuffledTextureMetadata& textureMetadata = m_textureMetadata[i];

        auto bytesInTextureBuffer = m_textureDataBuffers[i]->GetDesc().Width;
        uint32_t bytesPerThreadGroup = (textureMetadata.swizzleMode == DSTORAGE_SWIZZLE_MODE_4K_4K_8K) ? 16384U : 32768U;
        uint32_t numThreadGroups = static_cast<uint32_t>(bytesInTextureBuffer / bytesPerThreadGroup);

        m_unshuffleCmdList->SetPipelineState(m_unshufflePSOs[i].Get());
        m_unshuffleCmdList->SetComputeRootUnorderedAccessView(0, m_textureDataBuffers[i]->GetGPUVirtualAddress());
        m_unshuffleCmdList->Dispatch(numThreadGroups, 1, 1);

        PIXEndEvent(m_unshuffleCmdList.Get());
    }

    // Flush the caches
    m_unshuffleCmdList->FlushPipelineX(D3D12XBOX_FLUSH_TOP_COMPUTE_MASK | D3D12XBOX_FLUSH_BOP_COMPUTE_MASK, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);

    DX::ThrowIfFailed(m_unshuffleCmdList->Close());

    // Add a Wait() command on the GPU's Compute pipeline which will be satisfied once DirectStorage finish reading and decompressing unshuffled texture data
    // This will ensure that the command list isn't executed until the texture data is ready
    m_unshuffleCmdQueue->Wait(m_unshuffleTextureFence.Get(), m_unshuffleTextureFenceValue);
    m_unshuffleTextureFenceValue++;

    // Execute the command list
    ID3D12CommandList* ppCommandLists[] = { m_unshuffleCmdList.Get() };
    m_unshuffleCmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Signal that the GPU's Compute pipeline is done unshuffling the texture data
    m_unshuffleCmdQueue->Signal(m_unshuffleTextureFence.Get(), m_unshuffleTextureFenceValue);
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    InitDirectStorage();
    LoadTextureMetadata();

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

    // This is a simple sample showing the end-to-end loading and processing of the texture data, so we load and unshuffle every frame
    LoadTextureData();
    UnshuffleTextureData();

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

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

    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Render");

    auto commandList = m_deviceResources->GetCommandList();
	auto commandQueue = m_deviceResources->GetCommandQueue();

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    Clear();

    ID3D12DescriptorHeap* ppHeaps[] = { m_srvUavHeap->Heap() };
    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    commandList->SetGraphicsRootSignature(m_simpleRootSignature.Get());
    commandList->SetPipelineState(m_simplePSO.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    commandList->IASetVertexBuffers(0, 1, &m_quadVertexBufferView);

    // Offset and scale to place four quads, one in each corner on the screen
    const float positionInfo[c_numTextures][4] = { { -0.5f, 0.5f, 0.5f, 0.5f }, { 0.5f, 0.5f, 0.5f, 0.5f }, { -0.5f, -0.5f, 0.5f, 0.5f }, { 0.5f, -0.5f, 0.5f, 0.5f } };

    // Render unshuffed textures
    for (uint32_t i = 0; i < c_numTextures; i++)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render unshuffled %s texture", c_textureID[i].c_str());
        commandList->SetGraphicsRoot32BitConstants(1, 4, &positionInfo[i], 0);
        commandList->SetGraphicsRootDescriptorTable(0, m_srvUavHeap->GetGpuHandle(i + Descriptors::BC1TextureSRV));
        commandList->DrawInstanced(4, 1, 0, 0);
        PIXEndEvent(commandList);
    }

    // Render text
    {
        wchar_t buffer[256];

        const float textScale = 1.5f;
        auto lineSpacing = m_font->GetLineSpacing() * 1.5f;
        XMFLOAT2 textPos = XMFLOAT2(40.0f, 40.0f);
        auto viewport = m_deviceResources->GetScreenViewport();

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"UI");
        m_spriteBatch->Begin(commandList);

        m_font->DrawString(m_spriteBatch.get(), L"DirectStorage Texture Shuffling", textPos, DirectX::Colors::White, 0.0f, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(2.0f, 2.0f));

        // BC1 info
        {
            textPos = XMFLOAT2(40.0f, (viewport.Height / 2.0f) - lineSpacing * 3);
            m_font->DrawString(m_spriteBatch.get(), L"BC1", textPos, DirectX::Colors::White, 0.0f, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(textScale, textScale));

            textPos = XMFLOAT2(40.0f, (viewport.Height / 2.0f) - lineSpacing * 2);
            swprintf_s(buffer, std::size(buffer), L"%d KB Compressed", m_textureMetadata[0].unshuffledComrpessedSize / 1024);
            m_font->DrawString(m_spriteBatch.get(), buffer, textPos, DirectX::Colors::White, 0.0f, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(textScale, textScale));

            textPos = XMFLOAT2(40.0f, (viewport.Height / 2.0f) - lineSpacing * 1);
            swprintf_s(buffer, std::size(buffer), L"%d KB Shuffled Compressed", m_textureMetadata[0].shuffledComrpessedSize / 1024);
            m_font->DrawString(m_spriteBatch.get(), buffer, textPos, DirectX::Colors::White, 0.0f, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(textScale, textScale));
        }

        // BC3 info
        {
            textPos = XMFLOAT2((viewport.Width / 2.0f) + 40.0f, (viewport.Height / 2.0f) - lineSpacing * 3);
            m_font->DrawString(m_spriteBatch.get(), L"BC3", textPos, DirectX::Colors::White, 0.0f, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(textScale, textScale));

            textPos = XMFLOAT2((viewport.Width / 2.0f) + 40.0f, (viewport.Height / 2.0f) - lineSpacing * 2);
            swprintf_s(buffer, std::size(buffer), L"%d KB Compressed", m_textureMetadata[1].unshuffledComrpessedSize / 1024);
            m_font->DrawString(m_spriteBatch.get(), buffer, textPos, DirectX::Colors::White, 0.0f, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(textScale, textScale));

            textPos = XMFLOAT2((viewport.Width / 2.0f) + 40.0f, (viewport.Height / 2.0f) - lineSpacing * 1);
            swprintf_s(buffer, std::size(buffer), L"%d KB Shuffled Compressed", m_textureMetadata[1].shuffledComrpessedSize / 1024);
            m_font->DrawString(m_spriteBatch.get(), buffer, textPos, DirectX::Colors::White, 0.0f, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(textScale, textScale));
        }

        // BC4 info
        {
            textPos = XMFLOAT2(40.0f, viewport.Height - lineSpacing * 3);
            m_font->DrawString(m_spriteBatch.get(), L"BC4", textPos, DirectX::Colors::White, 0.0f, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(textScale, textScale));

            textPos = XMFLOAT2(40.0f, viewport.Height - lineSpacing * 2);
            swprintf_s(buffer, std::size(buffer), L"%d KB Compressed", m_textureMetadata[2].unshuffledComrpessedSize / 1024);
            m_font->DrawString(m_spriteBatch.get(), buffer, textPos, DirectX::Colors::White, 0.0f, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(textScale, textScale));

            textPos = XMFLOAT2(40.0f, viewport.Height - lineSpacing * 1);
            swprintf_s(buffer, std::size(buffer), L"%d KB Shuffled Compressed", m_textureMetadata[2].shuffledComrpessedSize / 1024);
            m_font->DrawString(m_spriteBatch.get(), buffer, textPos, DirectX::Colors::White, 0.0f, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(textScale, textScale));
        }

        // BC5 info
        {
            textPos = XMFLOAT2((viewport.Width / 2.0f) + 40.0f, viewport.Height - lineSpacing * 3);
            m_font->DrawString(m_spriteBatch.get(), L"BC5", textPos, DirectX::Colors::White, 0.0f, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(textScale, textScale));

            textPos = XMFLOAT2((viewport.Width / 2.0f) + 40.0f, viewport.Height - lineSpacing * 2);
            swprintf_s(buffer, std::size(buffer), L"%d KB Compressed", m_textureMetadata[3].unshuffledComrpessedSize / 1024);
            m_font->DrawString(m_spriteBatch.get(), buffer, textPos, DirectX::Colors::White, 0.0f, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(textScale, textScale));

            textPos = XMFLOAT2((viewport.Width / 2.0f) + 40.0f, viewport.Height - lineSpacing * 1);
            swprintf_s(buffer, std::size(buffer), L"%d KB Shuffled Compressed", m_textureMetadata[3].shuffledComrpessedSize / 1024);
            m_font->DrawString(m_spriteBatch.get(), buffer, textPos, DirectX::Colors::White, 0.0f, XMFLOAT2(0.0f, 0.0f), XMFLOAT2(textScale, textScale));
        }

        m_spriteBatch->End();
        PIXEndEvent(commandList);
    }

    PIXEndEvent(commandList);

    // Add a Wait() command on the GPU's graphics pipeline to ensure that the GPU's compute pipeline has finished unshuffling the texture data
    commandQueue->Wait(m_unshuffleTextureFence.Get(), m_unshuffleTextureFenceValue);
    m_unshuffleTextureFenceValue++;

    // Signal that the GPU is done with the texture data
    commandQueue->Signal(m_unshuffleTextureFence.Get(), m_unshuffleTextureFenceValue);

    PIXEndEvent();

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present(m_unshuffleTextureFence.Get(), m_unshuffleTextureFenceValue);
    m_graphicsMemory->Commit(commandQueue);
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    const auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    const auto viewport = m_deviceResources->GetScreenViewport();
    const auto scissorRect = m_deviceResources->GetScissorRect();
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

    // Simple quad rendering
    {
        static const D3D12_INPUT_ELEMENT_DESC s_inputElementDesc[2] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        auto vertexShaderBlob = DX::ReadData(L"SimpleVS.cso");
        auto pixelShaderBlob = DX::ReadData(L"SimplePS.cso");

        // Create root signature
        DX::ThrowIfFailed(device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(), IID_GRAPHICS_PPV_ARGS(m_simpleRootSignature.ReleaseAndGetAddressOf())));

        // Create PSO for final rendering
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_simpleRootSignature.Get();
        psoDesc.InputLayout = { s_inputElementDesc, _countof(s_inputElementDesc) };
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12XBOX_PRIMITIVE_TOPOLOGY_TYPE_QUAD;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_simplePSO.ReleaseAndGetAddressOf())));

        // Create vertex buffer.
        {
            struct Vertex
            {
                XMFLOAT3 position;
                XMFLOAT2 uv;
            };

            // Define the geoemtry for a rectangle.
            static const Vertex s_vertexData[4] =
            {
                { { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
                { { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
                { { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } }
            };

            // Note: using upload heaps to transfer static data like vert buffers is not 
            // recommended. Every time the GPU needs it, the upload heap will be marshalled 
            // over. Please read up on Default Heap usage. An upload heap is used here for 
            // code simplicity and because there are very few verts to actually transfer.
            const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
            auto const resDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(s_vertexData));

            DX::ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                nullptr, IID_GRAPHICS_PPV_ARGS(m_quadVertexBuffer.ReleaseAndGetAddressOf())));

            UINT8* pVertexDataBegin;
            CD3DX12_RANGE readRange(0, 0);

            DX::ThrowIfFailed(m_quadVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
            memcpy(pVertexDataBegin, s_vertexData, sizeof(s_vertexData));
            m_quadVertexBuffer->Unmap(0, nullptr);

            m_quadVertexBufferView.BufferLocation = m_quadVertexBuffer->GetGPUVirtualAddress();
            m_quadVertexBufferView.StrideInBytes = sizeof(Vertex);
            m_quadVertexBufferView.SizeInBytes = sizeof(s_vertexData);
        }
    }

    {
        auto resourceUpload = ResourceUploadBatch(device);
        resourceUpload.Begin();

        RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
        auto spritePSD = SpriteBatchPipelineStateDescription(rtState, &CommonStates::AlphaBlend);
        m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

        auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        finished.wait();
    }

    m_srvUavHeap = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    // Create root signatures, PSOs, UAVs, SRVs, and virtual memory allocations for each texture
    for (uint32_t i = 0; i < c_numTextures; i++)
    {
        const ShuffledTextureMetadata& textureMetadata = m_textureMetadata[i];

        // Create RS and PSO for Direct Storage fix-up stage
        const wchar_t* csoName = nullptr;
        switch (static_cast<DSTORAGE_SWIZZLE_MODE>(textureMetadata.swizzleMode))
        {
        case DSTORAGE_SWIZZLE_MODE_4K_4K_8K:
            csoName = L"UnshuffleBC1.cso";
            break;

        case DSTORAGE_SWIZZLE_MODE_4K_12K_4K_4K_8K:
            csoName = L"UnshuffleBC3.cso";
            break;

        case DSTORAGE_SWIZZLE_MODE_4K_4K_24K:
            csoName = L"UnshuffleBC4.cso";
            break;

        case DSTORAGE_SWIZZLE_MODE_4K_12K_4K_12K:
            csoName = L"UnshuffleBC5.cso";
            break;

        default:
            DX::ThrowIfFailed(E_FAIL);
        }

        if (csoName != nullptr)
        {
            auto const csBlob = DX::ReadData(csoName);

            if (!m_unshuffleRootSignature)
            {
                DX::ThrowIfFailed(device->CreateRootSignature(0, csBlob.data(), csBlob.size(), IID_GRAPHICS_PPV_ARGS(m_unshuffleRootSignature.ReleaseAndGetAddressOf())));
            }

            D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
            descComputePSO.pRootSignature = m_unshuffleRootSignature.Get();
            descComputePSO.CS = { csBlob.data(), csBlob.size() };
            DX::ThrowIfFailed(device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_unshufflePSOs[i].ReleaseAndGetAddressOf())));
        }

        m_textureUploadSizes[i] = RoundTo64K(textureMetadata.uncompressedSize);

        // Create backing memory for texture data loaded via DirectStorage
        {
            m_textureMemory[i] = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(XMemVirtualAlloc(NULL, m_textureUploadSizes[i], MEM_COMMIT | MEM_RESERVE | MEM_64K_PAGES,
                                                                                              XMEM_GRAPHICS, PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GRAPHICS_READWRITE));

            if (!m_textureMemory[i])
                throw std::exception("Virtual Memory not allocated for texture");
        }

        // Create texture resources
        {
            auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_textureUploadSizes[i], D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

            DX::ThrowIfFailed(device->CreatePlacedResourceX(m_textureMemory[i], &bufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_GRAPHICS_PPV_ARGS(m_textureDataBuffers[i].ReleaseAndGetAddressOf())));

            D3D12_RESOURCE_DESC textureDesc = {};
            textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            textureDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            textureDesc.Width = textureMetadata.width;
            textureDesc.Height = textureMetadata.height;
            textureDesc.DepthOrArraySize = 1;
            textureDesc.MipLevels = static_cast<UINT16>(textureMetadata.mipCount);
            textureDesc.Format = textureMetadata.format;
            textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Layout = textureMetadata.layout;

            DX::ThrowIfFailed(device->CreatePlacedResourceX(m_textureMemory[i], &textureDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, NULL, IID_GRAPHICS_PPV_ARGS(m_textures[i].ReleaseAndGetAddressOf())));

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = textureMetadata.format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MipLevels = textureMetadata.mipCount;

            device->CreateShaderResourceView(m_textures[i].Get(), &srvDesc, m_srvUavHeap->GetCpuHandle(i + Descriptors::BC1TextureSRV));
        }
    }

    // Create synchronization objects to handle communication between DirectStorage and Graphics workloads
    {       
        m_unshuffleTextureFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_unshuffleTextureFenceEvent == nullptr)
        {
            DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        DX::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(m_unshuffleTextureFence.ReleaseAndGetAddressOf())));
        m_unshuffleTextureFence->SetEventOnCompletion(m_unshuffleTextureFenceValue, m_unshuffleTextureFenceEvent);
    }

    // We use an async compute queue to unshuffle the texture data
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

        DX::ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_GRAPHICS_PPV_ARGS(m_unshuffleCmdQueue.ReleaseAndGetAddressOf())));
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_GRAPHICS_PPV_ARGS(m_unshuffleCmdAllocator.ReleaseAndGetAddressOf())));
        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_unshuffleCmdAllocator.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_unshuffleCmdList.ReleaseAndGetAddressOf())));
        m_unshuffleCmdList->Close();

        m_unshuffleCmdQueue->SetName(L"Unshuffle Command Queue");
        m_unshuffleCmdAllocator->SetName(L"Unshuffle Command Allocator");
        m_unshuffleCmdList->SetName(L"Unshuffle Command List");
    }

    m_deviceResources->WaitForGpu();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    const auto size = m_deviceResources->GetOutputSize();

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_font = std::make_unique<SpriteFont>(device, resourceUpload, (size.bottom > 1080) ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
                                            m_srvUavHeap->GetCpuHandle(Descriptors::FontSRV), m_srvUavHeap->GetGpuHandle(Descriptors::FontSRV));

    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    finished.wait();

    m_spriteBatch->SetViewport(m_deviceResources->GetScreenViewport());
}
#pragma endregion
