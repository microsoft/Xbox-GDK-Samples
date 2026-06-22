//--------------------------------------------------------------------------------------
// CMaskDecode.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "CMaskDecode.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const XMVECTORF32 c_eye = { { { -20.f, 10.f, -20.f, 0.f } } };
    constexpr float c_pitch = -0.432353f;
    constexpr float c_yaw = -0.988740f;

    constexpr DXGI_FORMAT   c_formatColor = DXGI_FORMAT_R8G8B8A8_UNORM;
    constexpr DXGI_FORMAT   c_formatDepth = DXGI_FORMAT_D32_FLOAT;
    constexpr DXGI_FORMAT   c_formatTextureDecodedCmask = DXGI_FORMAT_R8_TYPELESS;
    constexpr DXGI_FORMAT   c_formatUAVDecodedCmask = DXGI_FORMAT_R8_UINT;
    constexpr DXGI_FORMAT   c_formatSRVDecodedCmask = DXGI_FORMAT_R8_UINT;

    constexpr uint32_t      c_cmaskTileWidth = 8;
    constexpr uint32_t      c_cmaskTileHeight = 8;
    constexpr uint32_t      c_threadGroupDecodeCmaskX = 8;
    constexpr uint32_t      c_threadGroupDecodeCmaskY = 8;

    __declspec(align(16)) struct ConstantBufferCmaskParams
    {
        uint32_t m_cmaskInfo;
        uint8_t padding[12];
    };

    static_assert((sizeof(ConstantBufferCmaskParams) % 16) == 0, "CB size not padded correctly");

    __declspec(align(16)) struct ConstantBufferOverlay
    {
        uint32_t SubTileLayoutMode;
        uint32_t padding[3];
    };

    static_assert((sizeof(ConstantBufferOverlay) % 16) == 0, "CB size not padded correctly");
}

#ifdef NDEBUG

#define ATG_SAMPLE_ASSERT_MSG_RETURN(cond, rcode, format, ...) (void)0

#else

#define ATG_SAMPLE_ASSERT_MSG_RETURN(cond, rcode, format, ...)\
    do {                                                                                \
        if (!(cond))                                                                    \
        {                                                                               \
            wchar_t msgBuffer[256];                                                     \
            _snwprintf_s(msgBuffer, 256, _CRT_WIDE(format), __VA_ARGS__);               \
            _wassert(msgBuffer, _CRT_WIDE(__FILE__), static_cast<unsigned>(__LINE__));  \
            return (rcode);                                                             \
        }                                                                               \
    __pragma(warning(push))                                                             \
    __pragma(warning(disable : 4127))                                                   \
    } while (0)                                                                         \
    __pragma(warning(pop))

#endif

typedef enum CmaskSubtileLayout
{
    kCmaskSubtileLayout2x2          = 0x0u,
    kCmaskSubtileLayout2x2Rotated   = 0x1u,
    kCmaskSubtileLayout4x1          = 0x2u,
    kCmaskSubtileLayout1x4          = 0x3u,
    kCmaskSubtileLayout2x1          = 0x4u,
    kCmaskSubtileLayout1x2          = 0x5u,
    kCmaskSubtileLayoutUnknown      = 0xffffffffu,
} CmaskSubtileLayout;

#ifdef _GAMING_XBOX_SCARLETT

static CmaskSubtileLayout computeCmaskSubtileLayout(XG_SWIZZLE_MODE swizzleMode, uint32_t formatBytesPerElement, bool msaaEnabledSurface)
{
    ATG_SAMPLE_ASSERT_MSG_RETURN(formatBytesPerElement > 0u && formatBytesPerElement <= 16u && (formatBytesPerElement & (formatBytesPerElement - 1u)) == 0, kCmaskSubtileLayoutUnknown, "formatBytesPerElement (%u) should be a power of 2, > 0 and <= 16 ", formatBytesPerElement);

__pragma(warning(push))
__pragma(warning(disable : 4061))
    switch (swizzleMode)
    {
        case XG_SWIZZLE_MODE_256B_S:
        case XG_SWIZZLE_MODE_4KB_S:
        case XG_SWIZZLE_MODE_64KB_S:
        case XG_SWIZZLE_MODE_VAR_S:
        case XG_SWIZZLE_MODE_64KB_S_T:
        case XG_SWIZZLE_MODE_4KB_S_X:
        case XG_SWIZZLE_MODE_64KB_S_X:
        case XG_SWIZZLE_MODE_VAR_S_X:
        {
            ATG_SAMPLE_ASSERT_MSG_RETURN(!msaaEnabledSurface, kCmaskSubtileLayoutUnknown, "XG_SWIZZLE_MODE_*_S modes must not be chosen with Msaa");
            if (formatBytesPerElement == 1u || formatBytesPerElement == 2u)
                return kCmaskSubtileLayout1x4;
            else if (formatBytesPerElement == 8u)
                return kCmaskSubtileLayout2x2;
            else /* 4 or 16 */
                return kCmaskSubtileLayout2x2Rotated;
        }
        break;

        case XG_SWIZZLE_MODE_256B_D:
        case XG_SWIZZLE_MODE_4KB_D:
        case XG_SWIZZLE_MODE_64KB_D:
        case XG_SWIZZLE_MODE_VAR_D:
        case XG_SWIZZLE_MODE_64KB_D_T:
        case XG_SWIZZLE_MODE_4KB_D_X:
        case XG_SWIZZLE_MODE_64KB_D_X:
        case XG_SWIZZLE_MODE_VAR_D_X:
        {
            ATG_SAMPLE_ASSERT_MSG_RETURN(!msaaEnabledSurface, kCmaskSubtileLayoutUnknown, "XG_SWIZZLE_MODE_*_D modes must not be chosen with Msaa");
            if (formatBytesPerElement == 1u || formatBytesPerElement == 2u || formatBytesPerElement == 8u)
                return kCmaskSubtileLayout1x4;
            else if (formatBytesPerElement == 4u)
                return kCmaskSubtileLayout2x2;
            else /* 16 */
                return kCmaskSubtileLayout2x2Rotated;
        }
        break;

        case XG_SWIZZLE_MODE_256B_R:
        case XG_SWIZZLE_MODE_4KB_R:
        case XG_SWIZZLE_MODE_64KB_R:
        case XG_SWIZZLE_MODE_VAR_R:
        case XG_SWIZZLE_MODE_64KB_R_T:
        case XG_SWIZZLE_MODE_4KB_R_X:
        case XG_SWIZZLE_MODE_64KB_R_X:
        case XG_SWIZZLE_MODE_VAR_R_X:
        {
            if (msaaEnabledSurface)
            {
                return kCmaskSubtileLayout1x2;
            }
            else
            {
                if (formatBytesPerElement == 1u || formatBytesPerElement == 2u || formatBytesPerElement == 8u)
                    return kCmaskSubtileLayout1x4;
                else /* 4 or 16 */
                    return kCmaskSubtileLayout2x2;
            }
        }
        break;

        case XG_SWIZZLE_MODE_4KB_Z:
        case XG_SWIZZLE_MODE_64KB_Z:
        case XG_SWIZZLE_MODE_VAR_Z:
        case XG_SWIZZLE_MODE_64KB_Z_T:
        case XG_SWIZZLE_MODE_4KB_Z_X:
        case XG_SWIZZLE_MODE_64KB_Z_X:
        case XG_SWIZZLE_MODE_VAR_Z_X:
        {
            ATG_SAMPLE_ASSERT_MSG_RETURN(!msaaEnabledSurface, kCmaskSubtileLayoutUnknown, "XG_SWIZZLE_MODE_*_Z modes must not be chosen with Msaa");
            return kCmaskSubtileLayout2x2;
        }
        break;

        default:
        {
            return kCmaskSubtileLayoutUnknown;
        }
        break;
    }
__pragma(warning(pop))
}

#else

static CmaskSubtileLayout computeCmaskSubtileLayout(XG_TILE_MODE tileMode, uint32_t formatBytesPerElement, bool msaaEnabledSurface)
{
    ATG_SAMPLE_ASSERT_MSG_RETURN(formatBytesPerElement > 0u && formatBytesPerElement <= 16u && (formatBytesPerElement & (formatBytesPerElement - 1u)) == 0, kCmaskSubtileLayoutUnknown, "formatBytesPerElement (%u) should be a power of 2, > 0 and <= 16 ", formatBytesPerElement);

__pragma(warning(push))
__pragma(warning(disable : 4061))
    switch(tileMode)
    {
        case XG_TILE_MODE_DISPLAY:
        case XG_TILE_MODE_2D_DISPLAY:
        case XG_TILE_MODE_TILED_DISPLAY:
        case XG_TILE_MODE_TILED_2D_DISPLAY:
        {
            if (msaaEnabledSurface)
                return kCmaskSubtileLayout1x2;
            else
                return kCmaskSubtileLayout1x4;
        }
        break;

        case XG_TILE_MODE_1D_THIN:
        case XG_TILE_MODE_2D_THIN:
        case XG_TILE_MODE_3D_THIN:
        case XG_TILE_MODE_TILED_1D_THIN:
        case XG_TILE_MODE_TILED_2D_THIN:
        case XG_TILE_MODE_TILED_3D_THIN:
        {
            if (msaaEnabledSurface)
                return kCmaskSubtileLayout1x2;
            else
                return kCmaskSubtileLayout2x2;
        }
        break;

        case XG_TILE_MODE_RESERVED_27:
        case XG_TILE_MODE_RESERVED_28:
        case XG_TILE_MODE_RESERVED_29:
        case XG_TILE_MODE_RESERVED_30:
        {
            if (msaaEnabledSurface)
            {
                if (formatBytesPerElement < 8u)
                    return kCmaskSubtileLayout2x1;
                else if (formatBytesPerElement == 8u)
                    return kCmaskSubtileLayout1x2;
                else
                    return kCmaskSubtileLayoutUnknown;
            }
            else
            {
                if (formatBytesPerElement < 8u)
                    return kCmaskSubtileLayout4x1;
                else if (formatBytesPerElement == 8u)
                    return kCmaskSubtileLayout2x2;
                else
                    return kCmaskSubtileLayoutUnknown;
            }
        }
        break;

        default:
        {
            return kCmaskSubtileLayoutUnknown;
        }
        break;
    };
__pragma(warning(pop))
}

#endif

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_pitch(c_pitch),
    m_yaw(c_yaw),
#ifdef _GAMING_XBOX_SCARLETT
    m_hwVersion(D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_ANACONDA),
#else
    m_hwVersion(D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X),
#endif
    m_msaa(false),
    m_subTile(false),
    m_reset(false),
    m_colorTextureState(D3D12_RESOURCE_STATE_COMMON),
    m_widthCmask(0),
    m_heightCmask(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_formatColor, c_formatDepth,
        2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
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

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_msaa = !m_msaa;
            m_reset = true;
        }

        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_subTile = !m_subTile;
        }

        if (pad.IsLeftStickPressed())
        {
            m_pitch = c_pitch;
            m_yaw = c_yaw;
        }
        else
        {
            m_yaw += pad.thumbSticks.leftX * 0.1f;
            m_pitch -= pad.thumbSticks.leftY * 0.1f;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    // Limit to avoid looking directly up or down
    const float limit = XM_PI / 2.0f - 0.01f;
    m_pitch = std::max(-limit, std::min(+limit, m_pitch));

    if (m_yaw > XM_PI)
    {
        m_yaw -= XM_PI * 2.f;
    }
    else if (m_yaw < -XM_PI)
    {
        m_yaw += XM_PI * 2.f;
    }

    XMVECTOR lookAt = XMVectorSet(
        sinf(m_yaw),
        cosf(m_yaw),
        m_pitch,
        0);

    m_view = XMMatrixLookToLH(c_eye, lookAt, g_XMNegIdentityR2);

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
        InitializeColorResources();
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare(D3D12_RESOURCE_STATE_RENDER_TARGET);
    auto commandList = m_deviceResources->GetCommandList();
    {
        D3D12_RESOURCE_STATES colorNewState = D3D12_RESOURCE_STATE_RENDER_TARGET;
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_colorTexture.Get(),
            m_colorTextureState,
            colorNewState);
        commandList->ResourceBarrier(1, &barrier);
        m_colorTextureState = colorNewState;
    }
    Clear();

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    RenderScene(commandList);

    // Decode cmask
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Decode cmask");

    // Do not perform a decompress when transitioning the color target as we need the CMask information
    D3D12_RESOURCE_STATES colorNewState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12XBOX_RESOURCE_STATE_PRESERVE_COMPRESSED_COLOR | D3D12XBOX_RESOURCE_STATE_PRESERVE_INDIRECT_COLOR_CLEAR;
    if (m_msaa)
        colorNewState |= D3D12XBOX_RESOURCE_STATE_PRESERVE_SCATTERED_COLOR_FMASK;

    {
        D3D12_RESOURCE_BARRIER barriers[2]
        {
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_colorTexture.Get(),
                m_colorTextureState,
                colorNewState),
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_cmaskDecodedTexture.Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };

        commandList->ResourceBarrier(_countof(barriers), barriers);
        m_colorTextureState = colorNewState;

        commandList->SetComputeRootSignature(m_decodeRootSig.Get());
        commandList->SetComputeRootConstantBufferView(e_rootParameterCB, m_decodeCB.GpuAddress());
        commandList->SetComputeRootDescriptorTable(e_rootParameterSRV, m_resourceDescriptors->GetGpuHandle(Descriptors::CmaskEncoded));
        commandList->SetComputeRootDescriptorTable(e_rootParameterUAV, m_resourceDescriptors->GetGpuHandle(Descriptors::CmaskDecodedUAV));

        commandList->SetPipelineState(m_decodePSO.Get());

        uint32_t widthInThreadGroups = AlignUp(m_widthCmask, c_threadGroupDecodeCmaskX) / c_threadGroupDecodeCmaskX;
        uint32_t heightInThreadGroups = AlignUp(m_heightCmask, c_threadGroupDecodeCmaskY) / c_threadGroupDecodeCmaskY;
        commandList->Dispatch(widthInThreadGroups, heightInThreadGroups, 1);

#if 1  // Set to zero to switch to a barrier and explore Command Disasssmebly view

        // To make sure Fast Clear Elimination triggered by the next barrier doesn't overwrite CMask buffer while
        // the Dispatch above decodes it, insert a partial flush here.
        commandList->FlushPipelineX(D3D12XBOX_FLUSH_BOP_CS_PARTIAL, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
#else

        // Alternatively, aliasing barrier could be used instead of the partial flush. It's more expensive,
        // but doesn't require Xbox-specific FlushPipelineX
        D3D12_RESOURCE_BARRIER barrier[] = { CD3DX12_RESOURCE_BARRIER::Aliasing(0, m_colorTexture.Get()) };
        commandList->ResourceBarrier(1, barrier);
#endif
    }

    PIXEndEvent(commandList);

    // Copy to back buffer
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Copy/resolve to back buffer");

    auto backBuffer = m_deviceResources->GetRenderTarget();
    colorNewState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    D3D12_RESOURCE_BARRIER barriers[2] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(
            m_colorTexture.Get(),
            m_colorTextureState,
            colorNewState),
        CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer,
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET)
    };

    commandList->ResourceBarrier(2, barriers);
    m_colorTextureState = colorNewState;

    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    m_fullScreenQuad->Draw(commandList, m_overlayReplacePSO.Get(), m_resourceDescriptors->GetGpuHandle(Descriptors::SceneTex), m_overlayCB.GpuAddress());

    PIXEndEvent(commandList);
    // Render overlay
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_cmaskDecodedTexture.Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        commandList->ResourceBarrier(1, &barrier);

        ID3D12PipelineState* overlayPSO = m_subTile ? m_overlaySubTilePSO.Get() : m_overlayNoSubTilePSO.Get();

        m_fullScreenQuad->Draw(commandList, overlayPSO, m_resourceDescriptors->GetGpuHandle(Descriptors::CmaskDecoded), m_overlayCB.GpuAddress());
    }

    PIXEndEvent(commandList);

    // Render UI
    RenderUI(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::RenderScene(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render scene");

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        m_modelResources->Heap(),
        m_states->Heap()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    if (m_msaa)
    {
        Model::UpdateEffectMatrices(m_modelMSAA, SimpleMath::Matrix::Identity, m_view, m_proj);

        m_model->Draw(commandList, m_modelMSAA.cbegin());
    }
    else
    {
        Model::UpdateEffectMatrices(m_modelNormal, SimpleMath::Matrix::Identity, m_view, m_proj);

        m_model->Draw(commandList, m_modelNormal.cbegin());
    }

    PIXEndEvent(commandList);
}

void Sample::RenderUI(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render UI");

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        m_resourceDescriptors->Heap()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    auto size = m_deviceResources->GetOutputSize();
    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));

    m_batch->Begin(commandList);

    float y = float(safe.top);

    m_smallFont->DrawString(m_batch.get(), L"CMaskDecode",
        XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    y += m_smallFont->GetLineSpacing() * 1.5f;

    m_smallFont->DrawString(m_batch.get(), m_msaa ? L"2X MSAA" : L"No MSAA", XMFLOAT2(float(safe.left), y), m_msaa ? ATG::Colors::OffWhite : ATG::Colors::LightGrey);

    y += m_smallFont->GetLineSpacing();

    m_smallFont->DrawString(m_batch.get(), m_subTile ? L"SubTile" : L"No SubTile", XMFLOAT2(float(safe.left), y), m_subTile ? ATG::Colors::OffWhite : ATG::Colors::LightGrey);

    m_smallFont->DrawString(m_batch.get(), L"Fast clear Overlay (green = clear, red = non-clear)",
        XMFLOAT2(float(safe.left), float(safe.bottom) - m_smallFont->GetLineSpacing() * 2.5f), ATG::Colors::Blue);

    const wchar_t* legendStr = L"[View] Exit   [LThumb] Rotate   [A] Toggle MSAA   [B] Toggle SubTile";

    DX::DrawControllerString(m_batch.get(),
        m_smallFont.get(), m_ctrlFont.get(),
        legendStr,
        XMFLOAT2(float(safe.left),
            float(safe.bottom) - m_smallFont->GetLineSpacing()),
        ATG::Colors::LightGrey);

    m_batch->End();

    PIXEndEvent(commandList);
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto const rtvDescriptor = m_rtvDescriptorHeap->GetFirstCpuHandle();
    auto const dsvDescriptor = m_dsvDescriptorHeap->GetFirstCpuHandle();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
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

    D3D12XBOX_GPU_HARDWARE_CONFIGURATION hwConfig = {};
    device->GetGpuHardwareConfigurationX(&hwConfig);
    m_hwVersion = hwConfig.HardwareVersion;

    m_states = std::make_unique<CommonStates>(device);

    m_model = Model::CreateFromSDKMESH(device, L"FPSRoom.sdkmesh");

    m_fullScreenQuad = std::make_unique<DX::FullScreenQuad>();
    m_fullScreenQuad->Initialize(device);

    m_rtvDescriptorHeap = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        1);

    m_dsvDescriptorHeap = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        1);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    {
        auto blob = DX::ReadData(L"CMaskDecodeCS.cso");

        // Xbox One best practice is to use HLSL-based root signatures to support shader precompilation.

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, blob.data(), blob.size(),
                IID_GRAPHICS_PPV_ARGS(m_decodeRootSig.ReleaseAndGetAddressOf())));

        m_decodeRootSig->SetName(L"Decode RS");

        D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
        descComputePSO.pRootSignature = m_decodeRootSig.Get();
        descComputePSO.CS.pShaderBytecode = blob.data();
        descComputePSO.CS.BytecodeLength = blob.size();

        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_decodePSO.ReleaseAndGetAddressOf())));

        m_decodePSO->SetName(L"Decode PSO");
    }

    const RenderTargetState rtStateUI(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pd(rtStateUI, &CommonStates::AlphaBlend);

    {
        const EffectPipelineStateDescription overlay(nullptr,
            CommonStates::Additive,
            CommonStates::DepthNone,
            CommonStates::CullCounterClockwise,
            rtStateUI);

        auto rootSig = m_fullScreenQuad->GetRootSignature();

        auto blobvs = DX::ReadData(L"FullScreenQuadVS.cso");
        const D3D12_SHADER_BYTECODE vs = { blobvs.data(), blobvs.size() };

        auto blob = DX::ReadData(L"CMaskDecodePS.cso");
        D3D12_SHADER_BYTECODE ps = { blob.data(), blob.size() };

        overlay.CreatePipelineState(device, rootSig, vs, ps, m_overlayNoSubTilePSO.GetAddressOf());

        blob = DX::ReadData(L"CMaskDecode_Subtile.cso");
        ps = { blob.data(), blob.size() };

        overlay.CreatePipelineState(device, rootSig, vs, ps, m_overlaySubTilePSO.GetAddressOf());

        const EffectPipelineStateDescription overlayReplace(nullptr,
            CommonStates::Opaque,
            CommonStates::DepthNone,
            CommonStates::CullCounterClockwise,
            rtStateUI);

        blob = DX::ReadData(L"FullScreenQuadPS.cso");
        ps = { blob.data(), blob.size() };
        overlayReplace.CreatePipelineState(device, rootSig, vs, ps, m_overlayReplacePSO.GetAddressOf());
    }

    ResourceUploadBatch upload(device);
    upload.Begin();

    m_modelResources = m_model->LoadTextures(device, upload);

    m_fxFactory = std::make_unique<EffectFactory>(m_modelResources->Heap(), m_states->Heap());

    m_model->LoadStaticBuffers(device, upload);

    m_batch = std::make_unique<SpriteBatch>(device, upload, pd);

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    m_deviceResources->WaitForGpu();
    finish.wait();

    RenderTargetState rtState(c_formatColor, c_formatDepth);

    EffectPipelineStateDescription pdOpaque(
        nullptr,
        CommonStates::Opaque,
        CommonStates::DepthDefault,
        CommonStates::CullCounterClockwise,
        rtState);

    EffectPipelineStateDescription pdAlpha(
        nullptr,
        CommonStates::AlphaBlend,
        CommonStates::DepthDefault,
        CommonStates::CullCounterClockwise,
        rtState);

    m_modelNormal = m_model->CreateEffects(*m_fxFactory, pdOpaque, pdAlpha);

    rtState.sampleDesc.Count = 2;
    pdOpaque.renderTargetState = rtState;
    pdAlpha.renderTargetState = rtState;

    m_modelMSAA = m_model->CreateEffects(*m_fxFactory, pdOpaque, pdAlpha);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    m_batch->SetViewport(m_deviceResources->GetScreenViewport());

    auto const size = m_deviceResources->GetOutputSize();

    auto device = m_deviceResources->GetD3DDevice();

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
        (size.bottom > 1080) ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::TextFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::TextFont));

    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        (size.bottom > 1080) ? L"XboxOneControllerLegend.spritefont" : L"XboxOneControllerLegendSmall.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    // Wait until assets have been uploaded to the GPU.
    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), 0.1f, 10000.f);

    InitializeColorResources();
}

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#pragma warning(disable : 4061)

void Sample::InitializeColorResources()
{
    if (m_colorTexture != nullptr)
    {
        m_deviceResources->WaitForGpu();

        m_colorTexture.Reset();
        m_depthTexture.Reset();
        m_cmaskDecodedTexture.Reset();
    }

    //--- Create color buffer --------------------------------------------------------------
    auto const size = m_deviceResources->GetOutputSize();

    uint32_t width = static_cast<uint32_t>(size.right);
    uint32_t height = static_cast<uint32_t>(size.bottom);

    // Create descriptor for the color texture,
    // Note that unless D3D12XBOX_RESOURCE_FLAG_DENY_COLOR_COMPRESSION_DATA specified, CMASK will be always allocated
    D3D12_RESOURCE_DESC colorTextureDesc =
    {
        /*.Dimension        */  D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        /*.Alignment        */  0u,
        /*.Width            */  width,
        /*.Height           */  height,
        /*.DepthOrArraySize */  1u,
        /*.MipLevels        */  1u,
        /*.Format           */  c_formatColor,
        /*.SampleDesc       */  { m_msaa ? 2u : 1u, 0u, },
        /*.Layout           */  D3D12_TEXTURE_LAYOUT_UNKNOWN,
        /*.Flags            */  D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
    };
    auto device = m_deviceResources->GetD3DDevice();
    const CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_CLEAR_VALUE colorTextureClearValue;
    {
        colorTextureClearValue.Format = c_formatColor;
        colorTextureClearValue.Color[0] = ATG::Colors::Background.f[0];
        colorTextureClearValue.Color[1] = ATG::Colors::Background.f[1];
        colorTextureClearValue.Color[2] = ATG::Colors::Background.f[2];
        colorTextureClearValue.Color[3] = ATG::Colors::Background.f[3];
    }

    m_colorTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    // For simplicity, use CreateCommittedResource which internally allocates memory
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &colorTextureDesc,
        m_colorTextureState,
        &colorTextureClearValue,
        IID_GRAPHICS_PPV_ARGS(m_colorTexture.GetAddressOf())
    ));
    m_colorTexture->SetName(L"Color texture");

    // Create texture computer to double-check CMASK surface was allocated and query its location and size
    ComPtr<XGTextureAddressComputer> computer;
    DX::ThrowIfFailed(XGCreateTextureComputer(reinterpret_cast<XG_RESOURCE_DESC*>(&colorTextureDesc), computer.GetAddressOf()));

    XG_RESOURCE_LAYOUT colorTextureLayout;
    DX::ThrowIfFailed(computer->GetResourceLayout(&colorTextureLayout));

    // Iterate over existing planes of the resource and
    // get CMASK size in bytes and its offset relative to the base address of the resource
    uint32_t cmaskBaseRelativeByteOffset = 0u;
    uint32_t cmaskSizeInBytes = 0u;

    auto cmaskLayout = kCmaskSubtileLayoutUnknown;
    for (uint32_t i = 0; i < colorTextureLayout.Planes; ++i)
    {
        const XG_PLANE_LAYOUT & plane = colorTextureLayout.Plane[i];
        if (plane.Usage == XG_PLANE_USAGE_COLOR_MASK)
        {
            cmaskBaseRelativeByteOffset = (uint32_t)plane.BaseOffsetBytes;
            cmaskSizeInBytes = (uint32_t)plane.SizeBytes;
        }
        if (plane.Usage == XG_PLANE_USAGE_DEFAULT)
        {
            assert(0 == (plane.MipLayout[0].PaddedWidthElements % c_cmaskTileWidth));
            assert(0 == (plane.MipLayout[0].PaddedHeightElements % c_cmaskTileHeight));

            m_widthCmask = plane.MipLayout[0].PaddedWidthElements / c_cmaskTileWidth;
            m_heightCmask = plane.MipLayout[0].PaddedHeightElements / c_cmaskTileHeight;

#ifdef _GAMING_XBOX_SCARLETT
            cmaskLayout = computeCmaskSubtileLayout(plane.MipLayout[0].SwizzleMode, plane.BytesPerElement, m_msaa);
#else
            cmaskLayout = computeCmaskSubtileLayout(plane.MipLayout[0].TileMode, plane.BytesPerElement, m_msaa);
#endif
        }
    }
    assert(cmaskBaseRelativeByteOffset != 0u);
    assert(cmaskSizeInBytes != 0u);
    assert(cmaskLayout != kCmaskSubtileLayoutUnknown);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = c_formatColor;
    rtvDesc.ViewDimension = m_msaa ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;

    device->CreateRenderTargetView(m_colorTexture.Get(), &rtvDesc, m_rtvDescriptorHeap->GetFirstCpuHandle());

    D3D12_SHADER_RESOURCE_VIEW_DESC colorTextureSrvDesc = {};
    colorTextureSrvDesc.Format                  = c_formatColor;
    colorTextureSrvDesc.ViewDimension           = m_msaa ? D3D12_SRV_DIMENSION_TEXTURE2DMS : D3D12_SRV_DIMENSION_TEXTURE2D;
    colorTextureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (!m_msaa)
    {
        colorTextureSrvDesc.Texture2D.MostDetailedMip       = 0u;
        colorTextureSrvDesc.Texture2D.MipLevels             = 1u;
        colorTextureSrvDesc.Texture2D.PlaneSlice            = 0u;
        colorTextureSrvDesc.Texture2D.ResourceMinLODClamp   = 0.0;
    }
    device->CreateShaderResourceView(m_colorTexture.Get(), &colorTextureSrvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::SceneTex));

    //--- Create depth stencil view matching the render target -----------------------------
    const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        c_formatDepth,
        width,
        height,
        1, // Use a single array entry.
        1,  // Use a single mipmap level.
        m_msaa ? 2U : 1U,
        0U
    );
    depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = c_formatDepth;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthOptimizedClearValue,
        IID_GRAPHICS_PPV_ARGS(m_depthTexture.GetAddressOf())
    ));

    m_depthTexture->SetName(L"Depth stencil");

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = c_formatDepth;
    dsvDesc.ViewDimension = m_msaa ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;

    device->CreateDepthStencilView(m_depthTexture.Get(), &dsvDesc, m_dsvDescriptorHeap->GetFirstCpuHandle());

    //--- Set up Cmask buffers -------------------------------------------------------------
    ConstantBufferCmaskParams cbData = {};
    {
#ifdef _GAMING_XBOX_SCARLETT
        const uint32_t isCMaskLinear = 0u;
        const uint32_t pipeCount = m_hwVersion == D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART ? 8u : 32u;
        const uint32_t macroTileWidth = 128u; // 1024 pixels
        const uint32_t macroTileHeight = m_hwVersion == D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART ? 64u : 128u;
        m_widthCmask = AlignUp(m_widthCmask, macroTileWidth);
        m_heightCmask = AlignUp(m_heightCmask, macroTileHeight);
#else
        const uint32_t isCMaskLinear = 1u;
        const uint32_t pipeCount = m_hwVersion >= D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X ? 8u : 4u;
#endif
        cbData.m_cmaskInfo = isCMaskLinear << 31 | pipeCount << 24 | m_widthCmask | m_heightCmask << 12;
    }
    m_decodeCB = m_graphicsMemory->AllocateConstant(cbData);

    ConstantBufferOverlay cbData2 = {};
    cbData2.SubTileLayoutMode = cmaskLayout;
    m_overlayCB = m_graphicsMemory->AllocateConstant(cbData2);

    // Create a texture which holds fast clear information based on cmask
    D3D12_RESOURCE_DESC descCmaskDecodedTexture = CD3DX12_RESOURCE_DESC::Tex2D(
        c_formatTextureDecodedCmask,
        m_widthCmask,
        m_heightCmask,
        1, // Use a single array entry.
        1  // Use a single mipmap level.
    );
    descCmaskDecodedTexture.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &descCmaskDecodedTexture,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_cmaskDecodedTexture.GetAddressOf())));

    m_cmaskDecodedTexture->SetName(L"Cmask Decoded");

    D3D12_SHADER_RESOURCE_VIEW_DESC descCmaskDecodedSRV =
    {
        c_formatSRVDecodedCmask, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, {}
    };
    descCmaskDecodedSRV.Texture2D.MipLevels = 1;

    device->CreateShaderResourceView(m_cmaskDecodedTexture.Get(), &descCmaskDecodedSRV, m_resourceDescriptors->GetCpuHandle(Descriptors::CmaskDecoded));

    D3D12_UNORDERED_ACCESS_VIEW_DESC descCmaskDecodedUAV =
    {
        c_formatUAVDecodedCmask, D3D12_UAV_DIMENSION_TEXTURE2D, {}
    };

    device->CreateUnorderedAccessView(m_cmaskDecodedTexture.Get(), nullptr, &descCmaskDecodedUAV, m_resourceDescriptors->GetCpuHandle(Descriptors::CmaskDecodedUAV));

    // Create a texture which can hold cmask information (encoded)
    // This must be a placement allocation because it aliases the actual cmask data.
    D3D12_RESOURCE_DESC descCmaskEncodedBuffer = CD3DX12_RESOURCE_DESC::Buffer(cmaskSizeInBytes);

    D3D12XBOX_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                      = DXGI_FORMAT_R32_TYPELESS;
    srvDesc.ViewDimension               = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping     = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement         = 0;
    srvDesc.Buffer.NumElements          = cmaskSizeInBytes / 4;
    srvDesc.Buffer.StructureByteStride  = 0;
    srvDesc.Buffer.Flags                = D3D12_BUFFER_SRV_FLAG_RAW;
    srvDesc.ResourceLocation            = m_colorTexture->GetGPUVirtualAddress() + cmaskBaseRelativeByteOffset;
#ifndef _GAMING_XBOX_SCARLETT
    srvDesc.DataFormat                  = D3D12XBOX_DATA_FORMAT_32;
    srvDesc.NumberFormat                = D3D12XBOX_NUMBER_FORMAT_UINT;
#else
    srvDesc.ImageFormat                 = D3D12XBOX_IMAGE_FORMAT_32_UINT;
#endif
    srvDesc.MemoryType                  = 0;
    srvDesc.TextureWarnLevelOfDetail    = 0;
    srvDesc.TexturePerfModulation       = 0;

    device->CreatePlacedRawShaderResourceViewX(&descCmaskEncodedBuffer, &srvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::CmaskEncoded));

    m_reset = false;
}
#pragma endregion
