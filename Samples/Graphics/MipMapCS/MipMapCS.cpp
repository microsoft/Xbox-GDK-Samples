//--------------------------------------------------------------------------------------
// MipMapCS.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MipMapCS.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h"
#include "ReadData.h"

#include "Shaders\Shared.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    struct ConstantBufferQuad
    {
        float oneOverZoom;
        float offsetX;
        float offsetY;
        float textureWidth;
        float textureHeight;
        uint32_t mipLevel;
        uint32_t pad[2];
    };

    static_assert((sizeof(ConstantBufferQuad) % 16) == 0, "CB size not padded correctly");

    enum CSRootParameters : uint32_t
    {
        CSRootParameterCB = 0,
        CSRootParameterSRV,
        CSRootParameterUAV,
        CSRootParameterAtomicUAV
    };

    struct ConstantBufferGenerateMips
    {
        float       invTexDimsWidth;
        float       invTexDimsHeight;
        uint32_t    srcSlice;
        uint32_t    numMips;
    };

    static_assert((sizeof(ConstantBufferGenerateMips) % 16) == 0, "CB size not padded correctly");

    struct SpdConstants
    {
        uint32_t mips;
        uint32_t numWorkGroups;
        float invInputSize[2];
    };

    static_assert((sizeof(SpdConstants) % 16) == 0, "CB size not padded correctly");

    constexpr float c_zoomSpeed = 0.5f;
    constexpr float c_panSpeed = 0.5f;

    const DXGI_FORMAT c_textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_allLevels(true),
    m_multiPass(true),
    m_fp16(false),
    m_mipLevel(0),
    m_numLevels(0),
    m_topMip(0),
    m_texWidth(0),
    m_texHeight(0),
    m_zoom(1.f),
    m_offsetX(0),
    m_offsetY(0)

{
    // Renders only 2D, so no need for a depth buffer.
    // Use gamma-correct rendering.
    m_deviceResources =
        std::make_unique<DX::DeviceResources>(
            DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
            DXGI_FORMAT_UNKNOWN,
            2,
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

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        using ButtonState = GamePad::ButtonStateTracker::ButtonState;

        if (m_gamePadButtons.a == ButtonState::PRESSED)
        {
            m_allLevels = !m_allLevels;
        }
        else if (m_gamePadButtons.y == ButtonState::PRESSED)
        {
            m_multiPass = !m_multiPass;
        }
#ifdef _GAMING_XBOX_SCARLETT
        else if (m_gamePadButtons.x == ButtonState::PRESSED)
        {
            m_fp16 = !m_fp16;
        }
#endif
        else if (!m_allLevels)
        {
            if (m_gamePadButtons.dpadDown == ButtonState::PRESSED)
            {
                m_mipLevel = std::min(m_mipLevel + 1, m_numLevels);
            }
            else if (m_gamePadButtons.dpadUp == ButtonState::PRESSED)
            {
                m_mipLevel = m_mipLevel > 0 ? m_mipLevel - 1 : 0;
            }

            // Update camera
            {
                // Change zoom
                m_zoom *= (1.f + elapsedTime * c_zoomSpeed * pad.triggers.right);
                m_zoom *= (1.f - elapsedTime * c_zoomSpeed * pad.triggers.left);
                m_zoom = std::max(m_zoom, 1.f);

                float oneOverZoom = 1.0f / m_zoom;

                // Change offset
                m_offsetX += elapsedTime * c_panSpeed * oneOverZoom * pad.thumbSticks.rightX;
                m_offsetX = std::max(m_offsetX, 0.f);
                m_offsetX = std::min(m_offsetX, 1.f - oneOverZoom);
                m_offsetY -= elapsedTime * c_panSpeed * oneOverZoom * pad.thumbSticks.rightY;
                m_offsetY = std::max(m_offsetY, 0.f);
                m_offsetY = std::min(m_offsetY, 1.f - oneOverZoom);
            }

            // Reset camera
            if (pad.IsRightStickPressed())
            {
                m_zoom = 1.f;
                m_offsetX = 0;
                m_offsetY = 0;
            }
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

    // Set the descriptor heaps
    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        m_resourceDescriptors->Heap()
    };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(descriptorHeaps)), descriptorHeaps);

    // First frame initialization
    if (!m_frame)
    {
        CD3DX12_TEXTURE_COPY_LOCATION src(m_sourceTexture.Get(), 0);
        CD3DX12_TEXTURE_COPY_LOCATION dst(m_texture.Get(), 0);
        commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

        D3D12_RESOURCE_BARRIER barriers[D3D12_REQ_MIP_LEVELS] = {};

        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0);

        uint32_t count = 1;

        for (; count <= m_numLevels; ++count)
        {
            barriers[count] = CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, count);
        }

        commandList->ResourceBarrier(count, barriers);
    }

    // Generate mips
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Generate mips");

        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 0);
        commandList->ResourceBarrier(1, &barrier);

        if (m_multiPass)
        {
            commandList->SetComputeRootSignature(m_csRootSig.Get());
            commandList->SetComputeRootDescriptorTable(CSRootParameterSRV, m_resourceDescriptors->GetGpuHandle(Texture));

            for (uint32_t level = 0; level < uint32_t(m_numLevels);)
            {
                uint32_t srcWidth = std::max<uint32_t>(m_texWidth >> level, 1u);
                uint32_t srcHeight = std::max<uint32_t>(m_texHeight >> level, 1u);

                uint32_t destWidth = srcWidth >> 1;
                uint32_t destHeight = srcHeight >> 1;

                // Check if the height or width of the next generated mip is odd
                // For odd dimensions, create a new dispatch so that undersampling doesn't occur
                // If offsets have to be calculated while swizzling threads for odd width/height,
                // we will have to read thread values from neighboring threadgroups as well,
                // which will lead to sync barriers which we want to avoid and so we have a new dispatch
                // for odd sized mips.
                // But if the first generated mip has one of the dimensions as 1, then the number of mips
                // generated in the dispatch just depends on the other dimension
                // eg: 16x3 -> 8x1 -> 4x1 -> 2x1 -> 1x1 in a single dispatch

                DWORD numMipsInDispatch;
            	if (0 == _BitScanForward(&numMipsInDispatch, ((destWidth == 1) ? 0u : destWidth) | ((destHeight == 1) ? 0u : destHeight)))
            	{
                	numMipsInDispatch = 0;
            	}
                // The minimum value for width and height should be 1.
                // The value of 0 prior to this makes sure that the number
                // of mips generated in a dispatch do not depend on this dimension
                if (destWidth == 0) destWidth = 1;
                if (destWidth == 0) destWidth = 1;

                // One constant buffer per dispatch is required
                // In cases where the width or height of mip being generated is odd, it is sent as a separate dispatch
                // So in the worst case, each mip will have a separate dispatch
                ConstantBufferGenerateMips cb = {};
                cb.invTexDimsWidth = 1.0f / float(destWidth);
                cb.invTexDimsHeight = 1.0f / float(destHeight);
                cb.srcSlice = level;

                numMipsInDispatch = std::min<DWORD>(numMipsInDispatch + 1, MIPS_IN_ONE_SHADER);
                numMipsInDispatch = std::min<DWORD>(m_numLevels - level, numMipsInDispatch);
                cb.numMips = numMipsInDispatch;

                commandList->SetComputeRoot32BitConstants(CSRootParameterCB, 4, (void*)&cb, 0);

                commandList->SetComputeRootDescriptorTable(CSRootParameterUAV, m_resourceDescriptors->GetGpuHandle(UAV_MipBase + level));

                // 0th bit is set if Width is Odd
                // 1st bit is set if height is Odd
                uint32_t oddDimsYX = (srcWidth & 1) | ((srcHeight & 1) << 1);
                commandList->SetPipelineState(m_fp16 ? m_generateMipsFp16[oddDimsYX].Get() : m_generateMips[oddDimsYX].Get());

                UINT threadGroupX = (destWidth + 7) / 8;
                UINT threadGroupY = (destHeight + 7) / 8;
                commandList->Dispatch(threadGroupX, threadGroupY, 1);

                barrier = CD3DX12_RESOURCE_BARRIER::UAV(m_texture.Get());
                commandList->ResourceBarrier(1, &barrier);

                level += numMipsInDispatch;
            }
        }
        else
        {
            commandList->SetComputeRootSignature(m_spdRootSig.Get());
            commandList->SetComputeRootDescriptorTable(CSRootParameterSRV, m_resourceDescriptors->GetGpuHandle(Texture));
            commandList->SetComputeRootDescriptorTable(CSRootParameterUAV, m_resourceDescriptors->GetGpuHandle(UAV_MipBase));
            commandList->SetComputeRootUnorderedAccessView(CSRootParameterAtomicUAV, m_atomicBuffer->GetGPUVirtualAddress());

            uint32_t threadGroupX = (m_texWidth + 63) >> 6;
            uint32_t threadGroupY = (m_texHeight + 63) >> 6;

            SpdConstants cb = {};
            cb.mips = uint32_t(m_numLevels);
            cb.numWorkGroups = threadGroupX * threadGroupY;
            cb.invInputSize[0] = 1.0f / float(m_texWidth);
            cb.invInputSize[1] = 1.0f / float(m_texHeight);
            commandList->SetComputeRoot32BitConstants(CSRootParameterCB, 4, (void*)&cb, 0);

            commandList->SetPipelineState(m_ffxSpd[m_fp16 ? FfxSpdFp16 : FfxSpdFp32].Get());
            commandList->Dispatch(threadGroupX, threadGroupY, 1);

            barrier = CD3DX12_RESOURCE_BARRIER::UAV(m_texture.Get());
            commandList->ResourceBarrier(1, &barrier);
        }

        PIXEndEvent(commandList);
    }

    // View image
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            0);
        commandList->ResourceBarrier(1, &barrier);
    }

    auto const size = m_deviceResources->GetOutputSize();
    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(1920, 1080);
    const uint32_t topMipWidth = std::max<uint32_t>(m_texWidth >> m_topMip, 1u);
    const uint32_t topMipHeight = std::max<uint32_t>(m_texHeight >> m_topMip, 1u);

    if (m_allLevels)
    {
        ConstantBufferQuad cameraSettings = {};
        cameraSettings.oneOverZoom = 1.f;

        // display top mip on left and all other mips on right stacked vertically
		const float spacing = 8.0f;
        float x = (size.right - size.left - 1.5f * topMipWidth - spacing) * 0.5f;
        float y = (size.bottom - size.top - topMipHeight) * 0.5f;
        for (uint32_t level = m_topMip; level <= m_numLevels; ++level)
        {
            uint32_t width = std::max<uint32_t>(m_texWidth >> level, 1u);
            uint32_t height = std::max<uint32_t>(m_texHeight >> level, 1u);

            cameraSettings.textureWidth = (static_cast<float>(m_texWidth) / std::powf(2.f, static_cast<float>(level)));
            cameraSettings.textureHeight = (static_cast<float>(m_texHeight) / std::powf(2.f, static_cast<float>(level)));
            cameraSettings.mipLevel = level;
            auto cbQuadOrig = m_graphicsMemory->AllocateConstant(cameraSettings);

            D3D12_VIEWPORT viewPort;
            viewPort.TopLeftX = x;
            viewPort.TopLeftY = y;
            viewPort.Width = float(width);
            viewPort.Height = float(height);
            viewPort.MinDepth = 0;
            viewPort.MaxDepth = 1;
            commandList->RSSetViewports(1, &viewPort);

            RECT rct = { long(viewPort.TopLeftX), long(viewPort.TopLeftY), long(viewPort.TopLeftX + viewPort.Width), long(viewPort.TopLeftY + viewPort.Height) };
            commandList->RSSetScissorRects(1, &rct);

            m_fullScreenQuad->Draw(commandList, m_quadPSO.Get(),
                m_resourceDescriptors->GetGpuHandle(Descriptors::Texture), cbQuadOrig.GpuAddress());

            if (level == m_topMip)
            {
                x += float(width) + spacing;
            }
            else
            {
                y += float(height);
            }
        }
    }
    else
    {
        ConstantBufferQuad cameraSettings = {};
        cameraSettings.oneOverZoom = 1.f / m_zoom;
        cameraSettings.offsetX = m_offsetX;
        cameraSettings.offsetY = m_offsetY;
        cameraSettings.textureWidth = (static_cast<float>(m_texWidth) / std::powf(2.f, static_cast<float>(m_mipLevel)));
        cameraSettings.textureHeight = (static_cast<float>(m_texHeight) / std::powf(2.f, static_cast<float>(m_mipLevel)));
        cameraSettings.mipLevel = uint32_t(m_mipLevel);
        auto cbQuadOrig = m_graphicsMemory->AllocateConstant(cameraSettings);

        {
            D3D12_VIEWPORT viewPort;
            viewPort.TopLeftX = (size.right - size.left - topMipWidth) * 0.5f;
            viewPort.TopLeftY = (size.bottom - size.top - topMipHeight) * 0.5f;
            viewPort.Width = float(topMipWidth);
            viewPort.Height = float(topMipHeight);
            viewPort.MinDepth = 0;
            viewPort.MaxDepth = 1;
            commandList->RSSetViewports(1, &viewPort);

            RECT rct = { long(viewPort.TopLeftX), long(viewPort.TopLeftY), long(viewPort.TopLeftX + viewPort.Width), long(viewPort.TopLeftY + viewPort.Height) };
            commandList->RSSetScissorRects(1, &rct);

            m_fullScreenQuad->Draw(commandList, m_quadPSO.Get(),
                m_resourceDescriptors->GetGpuHandle(Descriptors::Texture), cbQuadOrig.GpuAddress());
        }
    }

    // HUD
    {
        auto vp = m_deviceResources->GetScreenViewport();
        commandList->RSSetViewports(1, &vp);

        auto rct = m_deviceResources->GetScissorRect();
        commandList->RSSetScissorRects(1, &rct);
    }

	const float horizontalAlign = 300.0f;
    XMFLOAT2 pos(float(safe.right) - horizontalAlign, float(safe.bottom) - 7.0f * m_font->GetLineSpacing());

    const float ysize = m_font->GetLineSpacing();

    m_batch->Begin(commandList);

    m_font->DrawString(m_batch.get(), L"MipMapCS", pos, ATG::Colors::LightGrey);

    pos.y += ysize;

    if (m_allLevels)
    {
        wchar_t buff[128] = {};
        swprintf_s(buff, L"Mip Levels %u - %u", m_topMip, m_numLevels);
        m_font->DrawString(m_batch.get(), buff, pos, ATG::Colors::LightGrey);
        pos.y += ysize;

        DX::DrawControllerString(m_batch.get(), m_font.get(), m_colorCtrlFont.get(), L"[A] Display single level", pos, ATG::Colors::LightGrey);
        pos.y += ysize;

        DX::DrawControllerString(m_batch.get(), m_font.get(), m_colorCtrlFont.get(), m_multiPass ? L"[Y] Use FFX SPD downsample" : L"[Y] Use multi-pass downsample", pos, ATG::Colors::LightGrey);
        pos.y += ysize;

#ifdef _GAMING_XBOX_SCARLETT
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_colorCtrlFont.get(), m_fp16 ? L"[X] Use FP32 shader" : L"[X] Use FP16 shader", pos, ATG::Colors::LightGrey);
        pos.y += ysize;
#endif
    }
    else
    {
        wchar_t buff[128] = {};
        swprintf_s(buff, L"[DPad] Up/Down Mip Level %u", m_mipLevel);
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_colorCtrlFont.get(), buff, pos, ATG::Colors::LightGrey);
        pos.y += ysize;

        DX::DrawControllerString(m_batch.get(), m_font.get(), m_colorCtrlFont.get(), L"[A] Show all levels", pos, ATG::Colors::LightGrey);
        pos.y += ysize;

        DX::DrawControllerString(m_batch.get(), m_font.get(), m_colorCtrlFont.get(), m_multiPass ? L"[Y] Use FFX SPD downsample" : L"[Y] Use multi-pass downsample", pos, ATG::Colors::LightGrey);
        pos.y += ysize;

#ifdef _GAMING_XBOX_SCARLETT
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_colorCtrlFont.get(), m_fp16 ? L"[X] Use FP32 shader" : L"[X] Use FP16 shader", pos, ATG::Colors::LightGrey);
        pos.y += ysize;
#endif
    }

    DX::DrawControllerString(m_batch.get(),
        m_font.get(), m_ctrlFont.get(),
        L"[View] Exit  [RThumb] Pan  [RT]/[LT] Zoom",
        XMFLOAT2(float(safe.right) - horizontalAlign,
            float(safe.bottom) - m_font->GetLineSpacing()),
        ATG::ColorsLinear::LightGrey);

    m_batch->End();

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
    // Use linear clear color for gamma-correct rendering.
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);

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

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    // Create compute shader objects.
    {
        auto csBlob = DX::ReadData(L"CSGenerateMips.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, csBlob.data(), csBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_csRootSig.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = m_csRootSig.Get();
        desc.CS.pShaderBytecode = csBlob.data();
        desc.CS.BytecodeLength = csBlob.size();
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_generateMips[CSGenerateMips].ReleaseAndGetAddressOf()))
        );

        m_generateMips[CSGenerateMips]->SetName(L"Generate Mips");

        csBlob = DX::ReadData(L"CSGenerateMipsOddX.cso");
        desc.CS.pShaderBytecode = csBlob.data();
        desc.CS.BytecodeLength = csBlob.size();
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_generateMips[CSGenerateMips_OddX].ReleaseAndGetAddressOf()))
        );

        m_generateMips[CSGenerateMips_OddX]->SetName(L"Generate Mips (Odd X)");

        csBlob = DX::ReadData(L"CSGenerateMipsOddY.cso");
        desc.CS.pShaderBytecode = csBlob.data();
        desc.CS.BytecodeLength = csBlob.size();
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_generateMips[CSGenerateMips_OddY].ReleaseAndGetAddressOf()))
        );

        m_generateMips[CSGenerateMips_OddY]->SetName(L"Generate Mips (Odd Y)");

        csBlob = DX::ReadData(L"CSGenerateMipsOddXY.cso");
        desc.CS.pShaderBytecode = csBlob.data();
        desc.CS.BytecodeLength = csBlob.size();
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_generateMips[CSGenerateMips_OddXY].ReleaseAndGetAddressOf()))
        );

        m_generateMips[CSGenerateMips_OddXY]->SetName(L"Generate Mips (Odd XY)");

#ifdef _GAMING_XBOX_SCARLETT
        csBlob = DX::ReadData(L"CSGenerateMips_fp16.cso");

        desc.CS.pShaderBytecode = csBlob.data();
        desc.CS.BytecodeLength = csBlob.size();
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_generateMipsFp16[CSGenerateMips].ReleaseAndGetAddressOf()))
        );

        m_generateMipsFp16[CSGenerateMips]->SetName(L"Generate Mips FP16");

        csBlob = DX::ReadData(L"CSGenerateMipsOddX_fp16.cso");
        desc.CS.pShaderBytecode = csBlob.data();
        desc.CS.BytecodeLength = csBlob.size();
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_generateMipsFp16[CSGenerateMips_OddX].ReleaseAndGetAddressOf()))
        );

        m_generateMipsFp16[CSGenerateMips_OddX]->SetName(L"Generate Mips (Odd X)");

        csBlob = DX::ReadData(L"CSGenerateMipsOddY_fp16.cso");
        desc.CS.pShaderBytecode = csBlob.data();
        desc.CS.BytecodeLength = csBlob.size();
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_generateMipsFp16[CSGenerateMips_OddY].ReleaseAndGetAddressOf()))
        );

        m_generateMipsFp16[CSGenerateMips_OddY]->SetName(L"Generate Mips (Odd Y) FP16");

        csBlob = DX::ReadData(L"CSGenerateMipsOddXY_fp16.cso");
        desc.CS.pShaderBytecode = csBlob.data();
        desc.CS.BytecodeLength = csBlob.size();
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_generateMipsFp16[CSGenerateMips_OddXY].ReleaseAndGetAddressOf()))
        );

        m_generateMipsFp16[CSGenerateMips_OddXY]->SetName(L"Generate Mips (Odd XY) FP16");
#endif

        // FFX SPD shaders
        csBlob = DX::ReadData(L"ffx_spd_fp32.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, csBlob.data(), csBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_spdRootSig.ReleaseAndGetAddressOf())));
        desc.pRootSignature = m_spdRootSig.Get();
        
        desc.CS.pShaderBytecode = csBlob.data();
        desc.CS.BytecodeLength = csBlob.size();
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_ffxSpd[FfxSpdFp32].ReleaseAndGetAddressOf()))
        );

        m_ffxSpd[FfxSpdFp32]->SetName(L"FFX SPD fp32");

#ifdef _GAMING_XBOX_SCARLETT
        csBlob = DX::ReadData(L"ffx_spd_fp16.cso");
        desc.CS.pShaderBytecode = csBlob.data();
        desc.CS.BytecodeLength = csBlob.size();
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_ffxSpd[FfxSpdFp16].ReleaseAndGetAddressOf()))
        );

        m_ffxSpd[FfxSpdFp16]->SetName(L"FFX SPD fp16");
#endif
    }

    // Create UI objects.
    m_fullScreenQuad = std::make_unique<DX::FullScreenQuad>();
    m_fullScreenQuad->Initialize(device);

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);
        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    {
        auto pixelShaderBlob = DX::ReadData(L"QuadWithCamera.cso");
        auto vertexShaderBlob = DX::ReadData(L"FullScreenQuadVS.cso");

        EffectPipelineStateDescription pd(
            nullptr,
            CommonStates::Opaque,
            CommonStates::DepthNone,
            CommonStates::CullNone,
            rtState);

        D3D12_SHADER_BYTECODE vertexShader = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        D3D12_SHADER_BYTECODE pixelShader = { pixelShaderBlob.data(), pixelShaderBlob.size() };

        pd.CreatePipelineState(
            device,
            m_fullScreenQuad->GetRootSignature(),
            vertexShader,
            pixelShader,
            m_quadPSO.ReleaseAndGetAddressOf());
    }

    wchar_t buff[MAX_PATH] = {};
    DX::FindMediaFile(buff, MAX_PATH, L"Insects.png");

    DX::ThrowIfFailed(
        CreateWICTextureFromFileEx(device, upload, buff,
            0, D3D12_RESOURCE_FLAG_NONE, WIC_LOADER_FORCE_RGBA32, m_sourceTexture.ReleaseAndGetAddressOf()));

    upload.Transition(m_sourceTexture.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    auto texDesc = m_sourceTexture->GetDesc();
    m_texWidth = uint32_t(texDesc.Width);
    m_texHeight = texDesc.Height;

    // Compute number of mips
    DWORD levels = 0;
    _BitScanReverse(&levels, DWORD(texDesc.Width) | texDesc.Height);
    m_numLevels = levels;

    // Create resources for generate mips
    {
        auto desc = CD3DX12_RESOURCE_DESC::Tex2D(c_textureFormat, texDesc.Width, texDesc.Height,
            1, uint16_t(m_numLevels + 1), 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        DX::ThrowIfFailed(
            device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                IID_GRAPHICS_PPV_ARGS(m_texture.ReleaseAndGetAddressOf()))
        );

        m_texture->SetName(L"Main Texture");

        // Create UAVs for generate mips compute shaders
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = { c_textureFormat, D3D12_UAV_DIMENSION_TEXTURE2D, {} };

        uint32_t index = 1;
        for (; index <= m_numLevels; ++index)
        {
            uavDesc.Texture2D.MipSlice = index;

            device->CreateUnorderedAccessView(m_texture.Get(), nullptr, &uavDesc,
                m_resourceDescriptors->GetCpuHandle(Descriptors::UAV_MipBase + index - 1));
        }

        // Create psuedo descriptors for validated build
        uint32_t alignedMips = m_numLevels + MIPS_IN_ONE_SHADER - 1;
        for (uint32_t j = index; j <= alignedMips; ++j)
        {
            uavDesc.Texture2D.MipSlice = static_cast<UINT>(index - 1);

            device->CreateUnorderedAccessView(m_texture.Get(), nullptr, &uavDesc,
                m_resourceDescriptors->GetCpuHandle(Descriptors::UAV_MipBase + j - 1));
        }
    }

    // create resource for atomic buffer
    {
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        auto descAtomicBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &descAtomicBuffer,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_atomicBuffer.ReleaseAndGetAddressOf())));

        DX::ThrowIfFailed(m_atomicBuffer->SetName(L"Atomic Buffer"));
    }

    CreateShaderResourceView(device, m_texture.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Texture));

    m_deviceResources->WaitForGpu();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const size = m_deviceResources->GetOutputSize();
    auto device = m_deviceResources->GetD3DDevice();

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    wchar_t buff[MAX_PATH] = {};
    DX::FindMediaFile(buff, MAX_PATH, L"SegoeUI_18.spritefont");
    m_font = std::make_unique<SpriteFont>(device, resourceUpload,
        buff,
        m_resourceDescriptors->GetCpuHandle(Descriptors::TextFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::TextFont));

    DX::FindMediaFile(buff, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        buff,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    DX::FindMediaFile(buff, MAX_PATH, L"XboxOneControllerSmall.spritefont");
    m_colorCtrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        buff,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ColorControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ColorControllerFont));

    // Wait until assets have been uploaded to the GPU.
    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    D3D12_VIEWPORT hudViewport = { 0, 0, 1920, 1080 };
    m_batch->SetViewport(hudViewport);

    // Find top mip to display for current window size
    const uint32_t windowWidth = static_cast<uint32_t>(size.right - size.left);
    const uint32_t windowHeight = static_cast<uint32_t>(size.bottom - size.top);
    for (uint32_t level = 0; level <= m_numLevels; ++level)
    {
        uint32_t srcWidth = std::max<uint32_t>(m_texWidth >> level, 1u);
        uint32_t srcHeight = std::max<uint32_t>(m_texHeight >> level, 1u);

        if (srcWidth < windowWidth && srcHeight < windowHeight)
        {
            m_topMip = level;
            break;
        }
    }

}
#pragma endregion
