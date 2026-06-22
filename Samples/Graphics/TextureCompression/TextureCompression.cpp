//--------------------------------------------------------------------------------------
// TextureCompression.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// This sample illustrates how to take advantage of the BCPack texture and zlib
// decompression hardware in the Scarlett platform. These decompression features can be
// used to decrease streaming bandwidth requirements at no additional CPU cost since the
// decompression is done by the MDU hardware.
//
// The sample contains a set of the test resources which have been compressed using
// the XBTC tool from the GDK. When the sample runs, it will stream (using DirectStorage)
// mip-levels from the test resources and display them on screen.
//
// DirectStorage is Microsoft's new low-level API for streaming data off the NVMe drive
// found in the Scarlett platform. The API allows streaming requests be LZ inflated and/
// or BCPack decompressed in real-time using the platform's hardware.
//
// IMPORTANT:
// This sample reaquires the February 2020 GDK preview (or later).
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "TextureCompression.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadData.h"
#include "StringUtil.h"

extern void ExitSample() noexcept;

using namespace DirectX;

namespace
{
    static constexpr size_t kSRV_Count = 128;
    static constexpr size_t kSampler_Count = 128;

    enum DescriptorHeapIndex : size_t
    {
        SRV_Font,
        SRV_CtrlFont,
        // The SRV pool only really needs two slots (the current texture and the next),
        // but since the CPU is ahead of the GPU we add a few extra slots to accommodate
        // for the swap chain length. See the PlaceTexture function for the index
        // allocator.
        SRV_Texture_Start,
        SRV_Texture_End = SRV_Texture_Start + 10
    };

    enum SamplerDescriptorHeapIndex : size_t
    {
        Texture
    };

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

    const char* GetDXGIFormatString(DXGI_FORMAT dxgiFormat)
    {
        switch (dxgiFormat)
        {
        case DXGI_FORMAT_BC1_UNORM:         return "BC1_UNORM";
        case DXGI_FORMAT_BC1_UNORM_SRGB:    return "BC1_UNORM_SRGB";
        case DXGI_FORMAT_BC3_UNORM:         return "BC3_UNORM";
        case DXGI_FORMAT_BC3_UNORM_SRGB:    return "BC3_UNORM_SRGB";
        case DXGI_FORMAT_BC4_UNORM:         return "BC4_UNORM";
        case DXGI_FORMAT_BC4_SNORM:         return "BC4_SNORM";
        case DXGI_FORMAT_BC5_UNORM:         return "BC5_UNORM";
        case DXGI_FORMAT_BC5_SNORM:         return "BC5_SNORM";
        case DXGI_FORMAT_BC6H_UF16:         return "BC6H_UF16";
        case DXGI_FORMAT_BC6H_SF16:         return "BC6H_SF16";
        case DXGI_FORMAT_BC7_UNORM:         return "BC7_UNORM";
        case DXGI_FORMAT_BC7_UNORM_SRGB:    return "BC7_UNORM_SRGB";
        default:
            throw std::exception("Unknown DXGI format");
        }
    }

    const char* GetTextureLayoutString(D3D12_TEXTURE_LAYOUT layout)
    {
        switch (layout)
        {
        case D3D12_TEXTURE_LAYOUT_ROW_MAJOR:                    return "ROW_MAJOR";
        case D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE:       return "64KB_UNDEFINED";
        case D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE:        return "64KB_STANDARD";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_LINEAR:      return "SWIZZLE_MODE_LINEAR";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_256B_S:      return "SWIZZLE_MODE_256B_S";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_256B_D:      return "SWIZZLE_MODE_256B_D";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_256B_R:      return "SWIZZLE_MODE_256B_R";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_4KB_Z:       return "SWIZZLE_MODE_4KB_Z";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_4KB_S:       return "SWIZZLE_MODE_4KB_S";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_4KB_D:       return "SWIZZLE_MODE_4KB_D";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_4KB_R:       return "SWIZZLE_MODE_4KB_R";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_64KB_Z:      return "SWIZZLE_MODE_64KB_Z";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_64KB_S:      return "SWIZZLE_MODE_64KB_S";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_64KB_D:      return "SWIZZLE_MODE_64KB_D";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_64KB_R:      return "SWIZZLE_MODE_64KB_R";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_VAR_Z:       return "SWIZZLE_MODE_VAR_Z";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_VAR_S:       return "SWIZZLE_MODE_VAR_S";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_VAR_D:       return "SWIZZLE_MODE_VAR_D";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_VAR_R:       return "SWIZZLE_MODE_VAR_R";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_64KB_Z_T:    return "SWIZZLE_MODE_64KB_Z_T";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_64KB_S_T:    return "SWIZZLE_MODE_64KB_S_T";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_64KB_D_T:    return "SWIZZLE_MODE_64KB_D_T";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_64KB_R_T:    return "SWIZZLE_MODE_64KB_R_T";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_4KB_Z_X:     return "SWIZZLE_MODE_4KB_Z_X";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_4KB_S_X:     return "SWIZZLE_MODE_4KB_S_X";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_4KB_D_X:     return "SWIZZLE_MODE_4KB_D_X";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_4KB_R_X:     return "SWIZZLE_MODE_4KB_R_X";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_64KB_Z_X:    return "SWIZZLE_MODE_64KB_Z_X";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_64KB_S_X:    return "SWIZZLE_MODE_64KB_S_X";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_64KB_D_X:    return "SWIZZLE_MODE_64KB_D_X";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_64KB_R_X:    return "SWIZZLE_MODE_64KB_R_X";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_VAR_Z_X:     return "SWIZZLE_MODE_VAR_Z_X";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_VAR_S_X:     return "SWIZZLE_MODE_VAR_S_X";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_VAR_D_X:     return "SWIZZLE_MODE_VAR_D_X";
        case D3D12XBOX_TEXTURE_LAYOUT_SWIZZLE_MODE_VAR_R_X:     return "SWIZZLE_MODE_VAR_R_X";
        default:
            throw std::exception("Unknown texture layout");
        }
    }

} // End unnamed namespace

Sample::Sample() noexcept(false)
    : m_frame(0)
    , m_nextSRVIndex(0)
    , m_cacheFlushRequired(false)
    , m_texture(nullptr)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
}

Sample::~Sample()
{
    // TODO: handle potential pending texture request

    // TODO: shutdown DStorage

    if (m_texture)
    {
        m_retiredTextures.push_back(m_texture);
    }

    m_deviceResources->WaitForGpu();
    PruneRetiredTextures(m_deviceResources->GetRetiredFenceValue());
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    // Initialize DirectStorage
    DX::ThrowIfFailed(::DStorageGetFactory(__uuidof(IDStorageFactoryX), reinterpret_cast<void**>(m_DSFactory.ReleaseAndGetAddressOf())));

    DSTORAGE_QUEUE_DESC queueDesc = {};
    queueDesc.Capacity  = DSTORAGE_MIN_QUEUE_CAPACITY;
    queueDesc.Priority  = DSTORAGE_PRIORITY_NORMAL;
    queueDesc.Name      = "NormalQueue";
    DX::ThrowIfFailed(m_DSFactory->CreateQueue(&queueDesc, __uuidof(IDStorageQueueX), reinterpret_cast<void**>(m_DSSQueue.ReleaseAndGetAddressOf())));
    DX::ThrowIfFailed(m_DSFactory->CreateStatusArray(10, "NormalQueueStatus", __uuidof(IDStorageStatusArrayX), reinterpret_cast<void**>(m_DSStatusArray.ReleaseAndGetAddressOf())));;

    // Find available textures
    m_textureInfos = FindTexturesInDirectory("Assets\\TestData");
    if (!m_textureInfos.size())
    {
        throw std::runtime_error("No sample textures found");
    }

    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // Request top mip from first texture
    auto texture = AllocateTexture(0);
    m_streamRequest = RequestTexture(texture, 0);
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const&/* timer*/)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    // Update pad and check for exit
    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
            return;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    if (!m_streamRequest.valid())
    {
        uint32_t mipIndex = m_texture->renderMipIndex;
        uint32_t textureIndex = m_texture->infoIndex;
        const auto& textureInfo = m_textureInfos[textureIndex];
        if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
        {
            if (mipIndex < (textureInfo.mipCount - 1))
            {
                mipIndex = mipIndex + 1;
            }
        }
        if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
        {
            if (mipIndex > 0)
            {
                mipIndex = mipIndex - 1;
            }
        }
        if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
        {
            if (textureIndex < (m_textureInfos.size() - 1))
            {
                textureIndex = textureIndex + 1;
            }
        }
        if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
        {
            if (textureIndex > 0)
            {
                textureIndex = textureIndex - 1;
            }
        }

        if (textureIndex != m_texture->infoIndex)
        {
            mipIndex = 0;
            auto texture = AllocateTexture(textureIndex);
            m_streamRequest = RequestTexture(texture, mipIndex);
        }
        else if (mipIndex != m_texture->renderMipIndex)
        {
            if (mipIndex >= textureInfo.streamableMipCount)
            {
                // BCPack: the mip index part of a compressed tail
                if (m_texture->renderMipIndex >= textureInfo.streamableMipCount)
                {
                    // The full compressed tail mip chain is already loaded - just change render index
                    m_texture->renderMipIndex = mipIndex;
                    return;
                }
            }

            // Load new mip level
            m_streamRequest = RequestTexture(m_texture, std::min(mipIndex, textureInfo.streamableMipCount));
        }
    }
    else
    {
        // Wait for pending load request
        if (m_streamRequest.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            auto texture = m_streamRequest.get();

            // Did we load a new texture?
            if (texture != m_texture)
            {
                // Loaded mip of new texture - retire old texture
                if (m_texture)
                {
                    m_retiredTextures.push_back(m_texture);
                }

                // Create D3D resource and SRV
                PlaceTexture(texture);

                // Make new texture current
                m_texture = texture;
            }

            m_texture->renderMipIndex = m_texture->streamMipIndex;
            m_cacheFlushRequired = true;
        }
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

    // Draw texture
    if (m_texture)
    {
        const auto& textureInfo = m_textureInfos[m_texture->infoIndex];

        if (m_cacheFlushRequired)
        {
            D3D12_GPU_VIRTUAL_ADDRESS flushAdr = 0;
            size_t flushSize = 0;
            if (textureInfo.streamableMipCount &&
                (m_texture->renderMipIndex >= textureInfo.streamableMipCount))
            {
                // Compressed tail mip loaded - flush all contained levels
                //
                // On Scarlett the chain is reversed in memory so highest level
                // is at the beginning of the resource memory
                flushAdr = m_texture->memory;
                const auto& lastRegMipLayout = textureInfo.layout.Plane[0].MipLayout[textureInfo.streamableMipCount - 1];
                flushSize = lastRegMipLayout.OffsetBytes;               // The tail may not occupy all of the space, but this is the max
            }
            else
            {
                // Regular mip level
                const auto& mipLayout = textureInfo.layout.Plane[0].MipLayout[m_texture->renderMipIndex];
                flushAdr = m_texture->memory + mipLayout.OffsetBytes,
                flushSize = mipLayout.SizeBytes;
            }

            // We just streamed into this mip - make sure GPU caches are invalidated
            commandList->FlushPipelineX(
                D3D12XBOX_FLUSH_TOP_TEXTURE_L1_INVALIDATE | D3D12XBOX_FLUSH_TOP_TEXTURE_L2_INVALIDATE,
                flushAdr,
                flushSize);
            m_cacheFlushRequired = false;
        }

        // Draw texture quad
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_QUADLIST);
        commandList->SetGraphicsRootSignature(m_simpleRootSignature.Get());
        commandList->SetPipelineState(m_simplePSO.Get());
        ID3D12DescriptorHeap* heaps[] =
        {
            m_srvHeap->Heap(),
            m_samplerHeap->Heap()
        };
        commandList->SetDescriptorHeaps(_countof(heaps), heaps);
        struct SimpleRootConstants
        {
            float       quadScale[2];
            uint32_t    mipLevel;
        };
        SimpleRootConstants constants;

        auto viewport = m_deviceResources->GetScreenViewport();
        float viewAspect = viewport.Width / viewport.Height;
        float texAspect = static_cast<float>(textureInfo.width) / static_cast<float>(textureInfo.height);

        constants.quadScale[0] = 0.4f * texAspect;
        constants.quadScale[1] = 0.4f * viewAspect;
        constants.mipLevel = m_texture->renderMipIndex;
        commandList->SetGraphicsRoot32BitConstants(0, 3, &constants, 0);
        commandList->SetGraphicsRootDescriptorTable(1, m_srvHeap->GetGpuHandle(DescriptorHeapIndex::SRV_Texture_Start + m_texture->srvIndex));
        commandList->SetGraphicsRootDescriptorTable(2, m_samplerHeap->GetGpuHandle(SamplerDescriptorHeapIndex::Texture));
        commandList->DrawInstanced(4, 1, 0, 0);

        m_texture->fenceValue = m_deviceResources->GetCurrentFenceValue();
    }

    // Draw the HUD
    {
        ScopedPixEvent hud(commandList, PIX_COLOR_DEFAULT, L"HUD");

        const auto outputSize = m_deviceResources->GetOutputSize();
        auto safeSize = DirectX::SimpleMath::Viewport::ComputeTitleSafeArea(UINT(outputSize.right), UINT(outputSize.bottom));
        XMFLOAT2 textPos = XMFLOAT2(float(safeSize.left), float(safeSize.top));
        XMVECTOR textColor = ATG::Colors::Green;
        wchar_t textBuffer[512] = {};

        m_hudBatch->Begin(commandList);

        m_font->DrawString(m_hudBatch.get(), L"TextureCompression", textPos, textColor);

        if (m_texture)
        {
            textPos.y += m_font->GetLineSpacing();

            const auto& textureInfo = m_textureInfos[m_texture->infoIndex];
            const auto& mipLayoutInfo = textureInfo.layout.Plane[0].MipLayout[m_texture->renderMipIndex];
            uint64_t mipMemSize;
            uint32_t streamIndex;
            if (!textureInfo.streamableMipCount || (m_texture->renderMipIndex < textureInfo.streamableMipCount))
            {
                // Regular mip level
                mipMemSize = mipLayoutInfo.SizeBytes;
                streamIndex = m_texture->renderMipIndex;
            }
            else
            {
                // Part of packed mip tail
                const auto& lastRegularMipLayoutInfo = textureInfo.layout.Plane[0].MipLayout[textureInfo.streamableMipCount - 1];
                mipMemSize = lastRegularMipLayoutInfo.OffsetBytes; // The packed tail contains all mips up to the first regular mip (this offset is tile size aligned)
                streamIndex = textureInfo.streamableMipCount;
            }

            char compressionTypeString[128] = {};
            const auto& fileMipInfo = textureInfo.mips[streamIndex];
            if (!fileMipInfo.deflated && !fileMipInfo.bcPacked)
            {
                strcat_s(compressionTypeString, sizeof(compressionTypeString), "none");
            }
            else
            {
                if (fileMipInfo.deflated)
                {
                    strcat_s(compressionTypeString, sizeof(compressionTypeString), "deflate");
                }
                if (fileMipInfo.bcPacked)
                {
                    strcat_s(compressionTypeString, sizeof(compressionTypeString), fileMipInfo.deflated ? ", bcpack" : "bcpack");
                }
            }

            size_t fileNameIndex = textureInfo.filePath.rfind("\\") + 1;

            swprintf_s(textBuffer, _countof(textBuffer),
                L"\nTexture: %hs[%d]\n"
                L"Format: %hs\n"
                L"Layout: %hs\n\n"
                L"Mip level: %d\n"
                L"Mip dimensions: %d X %d blocks\n\n"
                L"Stream id: %d (%hs)\n"
                L"Stream mem size: %lld bytes\n"
                L"Stream load size: %d bytes\n"
                L"Compression type: %hs\n"
                L"Compression ratio: %5.2f",
                textureInfo.filePath.c_str() + fileNameIndex, textureInfo.textureIndex,
                GetDXGIFormatString(textureInfo.format),
                GetTextureLayoutString(textureInfo.textureLayout),
                m_texture->renderMipIndex,              
                mipLayoutInfo.WidthElements, mipLayoutInfo.HeightElements,
                streamIndex, streamIndex < textureInfo.streamableMipCount ? "regular" : "packed tail",
                mipMemSize,
                fileMipInfo.loadSize,
                compressionTypeString,
                static_cast<float>(mipMemSize) / static_cast<float>(fileMipInfo.loadSize));
            m_font->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
        }

        swprintf_s(textBuffer, _countof(textBuffer), L"[DPad] Switch texture/ mip-level");
        textPos.y = float(safeSize.bottom - m_font->GetLineSpacing() * 1);
        DX::DrawControllerString(m_hudBatch.get(), m_font.get(), m_ctrlFont.get(), textBuffer, textPos, textColor);

        if (m_streamRequest.valid())
        {
            float flashTextColor = static_cast<float>(((sin(m_timer.GetTotalSeconds() * 6.0) + 1.0) / 2.0) * 0.5 + 1.0);
            swprintf_s(textBuffer, _countof(textBuffer), L"Loading");
            auto textSize = m_font->MeasureString(textBuffer);
            textPos.x = safeSize.right - XMVectorGetX(textSize);
            m_font->DrawString(m_hudBatch.get(), textBuffer, textPos, XMVectorScale(textColor, flashTextColor));
        }

        m_hudBatch->End();
    }

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());

    PruneRetiredTextures(m_deviceResources->GetRetiredFenceValue());

    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the back buffer
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
    // TODO: handle potential pending texture request
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

    {
        auto vertexShaderBlob = DX::ReadData(L"SimpleVS.cso");
        auto pixelShaderBlob = DX::ReadData(L"SimplePS.cso");

        // Create root signature
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_simpleRootSignature.ReleaseAndGetAddressOf())));

        // Create the pipeline state
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature                  = m_simpleRootSignature.Get();
        psoDesc.VS                              = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS                              = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState                 = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState.RenderTarget[0].BlendEnable              = TRUE;
        psoDesc.BlendState.RenderTarget[0].LogicOpEnable            = FALSE;
        psoDesc.BlendState.RenderTarget[0].BlendOp                  = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha             = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlend                 = D3D12_BLEND_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha            = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].DestBlend                = D3D12_BLEND_INV_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha           = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask    = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.DepthStencilState.DepthEnable   = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DSVFormat                       = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask                      = UINT_MAX;
        psoDesc.PrimitiveTopologyType           = D3D12XBOX_PRIMITIVE_TOPOLOGY_TYPE_QUAD;
        psoDesc.NumRenderTargets                = 1;
        psoDesc.RTVFormats[0]                   = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count                = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_simplePSO.ReleaseAndGetAddressOf())));
    }

    {
        auto resourceUpload = ResourceUploadBatch(device);
        resourceUpload.Begin();

        RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
        auto spritePSD = SpriteBatchPipelineStateDescription(rtState, &CommonStates::AlphaBlend);
        m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

        auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        finished.wait();
    }

    m_srvHeap = std::make_unique<DescriptorHeap>(device, kSRV_Count);

    m_samplerHeap = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        kSampler_Count);

    {
        D3D12_SAMPLER_DESC desc = {};
        desc.Filter         = D3D12_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.AddressV       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.AddressW       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.MipLODBias     = 0.0f;
        desc.MaxAnisotropy  = 1;
        desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.MinLOD         = 0.0f;
        desc.MaxLOD         = std::numeric_limits<float>::max();
        device->CreateSampler(&desc, m_samplerHeap->GetCpuHandle(SamplerDescriptorHeapIndex::Texture));
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    const auto size = m_deviceResources->GetOutputSize();
    // Load font and upload to GPU
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        m_font = std::make_unique<SpriteFont>(
            device,
            resourceUpload,
            (size.bottom > 1080) ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
            m_srvHeap->GetCpuHandle(DescriptorHeapIndex::SRV_Font),
            m_srvHeap->GetGpuHandle(DescriptorHeapIndex::SRV_Font));

        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            (size.bottom > 1080) ? L"XboxOneControllerLegend.spritefont" : L"XboxOneControllerLegendSmall.spritefont",
            m_srvHeap->GetCpuHandle(DescriptorHeapIndex::SRV_CtrlFont),
            m_srvHeap->GetGpuHandle(DescriptorHeapIndex::SRV_CtrlFont));

        auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        finished.wait();
    }

    // Set hud sprite viewport
    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());
}

#pragma endregion

#pragma region BCPack texture sample

Sample::Texture* Sample::AllocateTexture(uint32_t textureIndex) const
{
    const TextureInfo& textureInfo = m_textureInfos[textureIndex];

    // Allocate gfx memory for texture data. This is done using the XMemVirtualAlloc function
    // to get write-combined memory on the CPU side.
    //
    // DirectStorage also supports loading straight to physical memory which can allow a title
    // more explicit control of page allocation and the mapping process (see
    // XMemAllocPhysicalPages and XMemMapPhysicalPages)
    PVOID allocRV = ::XMemVirtualAlloc(
        NULL,
        textureInfo.layout.SizeBytes,
        MEM_COMMIT | MEM_RESERVE | MEM_64K_PAGES,
        XMEM_GRAPHICS,
        PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GRAPHICS_READONLY);

    Texture* texture = new Texture;
    assert(texture);
    texture->infoIndex = textureIndex;
    texture->memory = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(allocRV);
    return texture;
}

std::future<Sample::Texture*> Sample::RequestTexture(Texture* texture, uint32_t streamMipIndex) const
{
    assert(streamMipIndex <= m_textureInfos[texture->infoIndex].streamableMipCount);

    return std::async(std::launch::async,
        [this, texture, streamMipIndex]() -> Texture*
    {
        const TextureInfo& textureInfo = m_textureInfos[texture->infoIndex];
        const auto& fileMip = textureInfo.mips[streamMipIndex];

        void* loadAddress = nullptr;
        uint64_t destSize = 0;
        if (textureInfo.containerType == ContainerType::XDDS)
        {
            const auto& layoutMip = textureInfo.layout.Plane[0].MipLayout[streamMipIndex];
            loadAddress = reinterpret_cast<void*>(texture->memory + layoutMip.OffsetBytes);
            destSize = static_cast<uint64_t>(fileMip.loadSize);
        }
        else
        {
            if (textureInfo.streamableMipCount && (streamMipIndex == textureInfo.streamableMipCount))
            {
                // The streamed data is a packed mip tail. On Scarlett mip data is organized in reverse order
                // compared to Xbox One (that is: highest mip level at the base address of the resource)
                const auto& lastRegMipLayout = textureInfo.layout.Plane[0].MipLayout[textureInfo.streamableMipCount - 1];
                loadAddress = reinterpret_cast<void*>(texture->memory);
                destSize = lastRegMipLayout.OffsetBytes;        // The tail may not occupy all of the space, but this is the max
            }
            else
            {
                // Regular mip level
                const auto& mipLayout = textureInfo.layout.Plane[0].MipLayout[streamMipIndex];
                loadAddress = reinterpret_cast<void*>(texture->memory + mipLayout.OffsetBytes);
                destSize    = mipLayout.SizeBytes;
            }
        }

        // Open texture file
        Microsoft::WRL::ComPtr<IDStorageFileX> DSFile;
        m_DSFactory->OpenFile(
            DX::Utf8ToWide(textureInfo.filePath).c_str(),
            IID_PPV_ARGS(DSFile.ReleaseAndGetAddressOf()));

        // Request mip data from stream
        DSTORAGE_REQUEST_OPTIONS DSRequestOptions = {};
        DSRequestOptions.ZlibDecompress = fileMip.deflated;
        DSRequestOptions.BcpackMode     = fileMip.bcPacked ? GetBCPackMode(textureInfo.format) : DSTORAGE_BCPACK_MODE_NONE;
        DSRequestOptions.SourceType     = DSTORAGE_REQUEST_SOURCE_FILE;
        DSTORAGE_REQUEST DSRequest = {};
        DSRequest.Options               = DSRequestOptions;
        DSRequest.Destination           = loadAddress;
        DSRequest.DestinationSize       = static_cast<uint32_t>(destSize);
        DSRequest.File                  = DSFile.Get();
        DSRequest.FileOffset            = fileMip.offset;
        DSRequest.SourceSize            = fileMip.loadSize;
        DSRequest.IntermediateSize      = fileMip.deflated && fileMip.bcPacked ? fileMip.inflatedSize : 0;
        DSRequest.CancellationTag       = 1;
        m_DSSQueue->EnqueueRequest(&DSRequest);

        // Submit request and wait for result to come in. This is NOT how you get best performance out of
        // DirectStorage (you will want to keep many requests in-flight at all times and only sync as
        // infrequently as possible), but does the job for simplicity of the sample.
        m_DSSQueue->EnqueueStatus(m_DSStatusArray.Get(), 0);
        m_DSSQueue->Submit();
        while (!m_DSStatusArray->IsComplete(0))
        {
            ::Sleep(1);
        }

        HRESULT statusCode = m_DSStatusArray->GetHResult(0);
        if (statusCode != S_OK)
        {
            throw std::exception("Direct Storage failed");
        }

        DSFile->Close();

        texture->streamMipIndex = streamMipIndex;
        return texture;
    });
}

void Sample::PlaceTexture(Texture* texture)
{
    assert(texture);

    auto d3dDevice = m_deviceResources->GetD3DDevice();
    const TextureInfo& textureInfo = m_textureInfos[texture->infoIndex];
    {
        // Create new texture resource pointing to the pre-allocated gfx memory
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        desc.Width              = textureInfo.width;
        desc.Height             = textureInfo.height;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = static_cast<UINT16>(textureInfo.mipCount);
        desc.Format             = textureInfo.format;
        desc.Flags              = D3D12_RESOURCE_FLAG_NONE;
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout             = textureInfo.textureLayout;
        DX::ThrowIfFailed(d3dDevice->CreatePlacedResourceX(
            texture->memory,
            &desc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            NULL,
            IID_GRAPHICS_PPV_ARGS(texture->resource.ReleaseAndGetAddressOf())));
    }

    {
        // Allocate SRV slot
        texture->srvIndex = m_nextSRVIndex;
        static constexpr size_t SRVRange = DescriptorHeapIndex::SRV_Texture_End - DescriptorHeapIndex::SRV_Texture_Start;
        m_nextSRVIndex = (m_nextSRVIndex + 1) % SRVRange;

        // Create new texture SRV
        auto cpuSRV = m_srvHeap->GetCpuHandle(DescriptorHeapIndex::SRV_Texture_Start + texture->srvIndex);

        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format                         = textureInfo.format;
        desc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;
        desc.Shader4ComponentMapping        = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Texture2D.MostDetailedMip      = 0;
        desc.Texture2D.MipLevels            = textureInfo.mipCount;
        desc.Texture2D.PlaneSlice           = 0;
        desc.Texture2D.ResourceMinLODClamp  = 0;
        d3dDevice->CreateShaderResourceView(texture->resource.Get(), &desc, cpuSRV);
    }

    texture->fenceValue = 0;
}

void Sample::PruneRetiredTextures(UINT64 lastRetiredFence)
{
    // Retire old textures
    m_retiredTextures.remove_if(
        [this, lastRetiredFence](Texture* texture)
        {
            if (texture->fenceValue <= lastRetiredFence)
            {
                // Free texture memory
                ::VirtualFree(
                    reinterpret_cast<LPVOID>(texture->memory),
                    0,
                    MEM_RELEASE);

                delete texture;
                return true;
            }
            else
            {
                return false;
            }
        }
    );
}

#pragma endregion
