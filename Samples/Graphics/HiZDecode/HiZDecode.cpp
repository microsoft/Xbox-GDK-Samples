//--------------------------------------------------------------------------------------
// HiZDecode.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "HiZDecode.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

/** NOTE:
 *  On Scarlett HTile re-summarization together with forced Depth decompression is not supported
 *  As a result, separate HTile re-summarization happens either on Scarlett or on Xbox One only
 *  when forced Depth decompression is not needed, otherwise HTile re-summarization on Xbox One
 *  happens later together forced Depth decompression
 */
#if 1 //_GAMING_XBOX_SCARLETT
#define DISABLE_FORCE_DECOMPRESS_WITH_RESUMMARIZATION 1
#else
#define DISABLE_FORCE_DECOMPRESS_WITH_RESUMMARIZATION 0
#endif

#define DISABLE_VALIDATION_ERRORS_ON_COPY_SRC_DST_BARRIER 1

#define TEXTURE_BUILDER_ASSERT(expr) assert(expr)

struct TextureResourceBuilder
{
    TextureResourceBuilder() :
        m_desc{},
        m_res(nullptr),
        m_viewFormat{},
        m_viewCompMapping(0),
        m_mipFirstLevel(0),
        m_mipLevelCount(0),
        m_arrayFirstSlice(0),
        m_arraySliceCount(0),
        m_layout(0)
    {}

    ~TextureResourceBuilder()
    {
    }

    inline void initAsTex2dArrayWithMips(uint16_t w, uint16_t h, uint16_t arraySlices, uint16_t mipLevels, DXGI_FORMAT format);

    inline void initAsTex2dArray(uint16_t w, uint16_t h, uint16_t arraySlices, DXGI_FORMAT format);

    inline void initAsTex2dWithMips(uint16_t w, uint16_t h, uint16_t mipLevels, DXGI_FORMAT format);

    inline void initAsTex2d(uint16_t w, uint16_t h, DXGI_FORMAT format);

    inline void setMipLevelViewRange(uint16_t mipFirstLevel, uint16_t mipLevelCount);

    inline void setArraySliceViewRange(uint16_t arrayFirstSlice, uint16_t arraySliceCount);

    inline void setFormat(DXGI_FORMAT format);

    inline void setLayout(uint32_t layout);

    inline ID3D12Resource * createDefaultHeapResource(ID3D12Device *device, D3D12_RESOURCE_STATES state);

    inline ID3D12Resource * createPlacedResourceX(ID3D12Device *device, D3D12_GPU_VIRTUAL_ADDRESS address, D3D12_RESOURCE_STATES state);

    inline void createSrv(ID3D12Device *device, D3D12_SHADER_RESOURCE_VIEW_DESC *srvDesc, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle) const;

    inline void createUav(ID3D12Device *device, D3D12_UNORDERED_ACCESS_VIEW_DESC *srvDesc, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle) const;

    inline void create2dSrv(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle) const;

    inline void create2dArraySrv(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle) const;

    inline void create2dUav(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle) const;

    inline void create2dArrayUav(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle) const;

    D3D12_RESOURCE_DESC m_desc;
    ID3D12Resource     *m_res;

    DXGI_FORMAT         m_viewFormat;
    uint32_t            m_viewCompMapping;
    uint16_t            m_mipFirstLevel;
    uint16_t            m_mipLevelCount;
    uint16_t            m_arrayFirstSlice;
    uint16_t            m_arraySliceCount;
    uint32_t            m_layout;
};


void TextureResourceBuilder::initAsTex2dArrayWithMips(uint16_t w, uint16_t h, uint16_t arraySlices, uint16_t mipLevels, DXGI_FORMAT format)
{
    m_desc.Dimension            = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    m_desc.Alignment            = 0u;
    m_desc.Width                = w;
    m_desc.Height               = h;
    m_desc.DepthOrArraySize     = arraySlices;
    m_desc.MipLevels            = mipLevels;
    m_desc.Format               = format;
    m_desc.SampleDesc.Count     = 1u;
    m_desc.SampleDesc.Quality   = 0u;
    m_desc.Layout               = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    m_desc.Flags                = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    m_viewFormat        = format;
    m_viewCompMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_mipFirstLevel     = 0u;
    m_mipLevelCount     = mipLevels;
    m_arrayFirstSlice   = 0u;
    m_arraySliceCount   = arraySlices;
}

void TextureResourceBuilder::initAsTex2dArray(uint16_t w, uint16_t h, uint16_t arraySlices, DXGI_FORMAT format)
{
    initAsTex2dArrayWithMips(w, h, arraySlices, 1u, format);
}

void TextureResourceBuilder::initAsTex2dWithMips(uint16_t w, uint16_t h, uint16_t mipLevels, DXGI_FORMAT format)
{
    initAsTex2dArrayWithMips(w, h, 1u, mipLevels, format);
}

void TextureResourceBuilder::initAsTex2d(uint16_t w, uint16_t h, DXGI_FORMAT format)
{
    initAsTex2dArray(w, h, 1u, format);
}

void TextureResourceBuilder::setMipLevelViewRange(uint16_t mipFirstLevel, uint16_t mipLevelCount)
{
    TEXTURE_BUILDER_ASSERT(mipFirstLevel < m_desc.MipLevels);
    TEXTURE_BUILDER_ASSERT(mipFirstLevel + mipLevelCount <= m_desc.MipLevels);
    m_mipFirstLevel = std::min<uint16_t>(mipFirstLevel, (uint16_t)m_desc.MipLevels - 1u);
    m_mipLevelCount = std::min(mipLevelCount, (uint16_t)(m_desc.MipLevels - m_mipFirstLevel));
}

void TextureResourceBuilder::setArraySliceViewRange(uint16_t arrayFirstSlice, uint16_t arraySliceCount)
{
    TEXTURE_BUILDER_ASSERT(arrayFirstSlice < m_desc.DepthOrArraySize);
    TEXTURE_BUILDER_ASSERT(arrayFirstSlice + arraySliceCount <= m_desc.DepthOrArraySize);
    m_arrayFirstSlice = std::min<uint16_t>(arrayFirstSlice, (uint16_t)m_desc.DepthOrArraySize - 1u);
    m_arraySliceCount = std::min(arraySliceCount, (uint16_t)(m_desc.DepthOrArraySize - m_arrayFirstSlice));
}

void TextureResourceBuilder::setFormat(DXGI_FORMAT format)
{
    m_desc.Format = m_viewFormat = format;
}

void TextureResourceBuilder::setLayout(uint32_t layout)
{
    m_desc.Layout = static_cast<D3D12_TEXTURE_LAYOUT>(layout);
}

ID3D12Resource * TextureResourceBuilder::createDefaultHeapResource(ID3D12Device *device, D3D12_RESOURCE_STATES state)
{
    ID3D12Resource *res = 0;
    D3D12_HEAP_PROPERTIES heapProps =
    {
        /* Type                  */ D3D12_HEAP_TYPE_DEFAULT,
        /* CPUPageProperty       */ D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        /* MemoryPoolPreference  */ D3D12_MEMORY_POOL_UNKNOWN,
        /* CreationNodeMask      */ 0x1u,
        /* VisibleNodeMask       */ 0x1u,
    };

    if (S_OK == device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &m_desc, state, 0, IID_ID3D12Resource, (void **)&res))
    {
        m_res = res;
    }
    return res;
}

ID3D12Resource * TextureResourceBuilder::createPlacedResourceX(ID3D12Device *device, D3D12_GPU_VIRTUAL_ADDRESS address, D3D12_RESOURCE_STATES state)
{
    ID3D12Resource* res = 0;
    if (S_OK == device->CreatePlacedResourceX(address, &m_desc, state, 0, IID_ID3D12Resource, (void **)&res))
    {
        m_res = res;
    }
    return res;
}

void TextureResourceBuilder::createSrv(ID3D12Device *device, D3D12_SHADER_RESOURCE_VIEW_DESC *srvDesc, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle) const
{
    srvDesc->Format                     = m_viewFormat;
    srvDesc->Shader4ComponentMapping    = m_viewCompMapping;
    device->CreateShaderResourceView(m_res, srvDesc, destCpuHandle);
}

void TextureResourceBuilder::createUav(ID3D12Device *device, D3D12_UNORDERED_ACCESS_VIEW_DESC *uavDesc, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle) const
{
    uavDesc->Format = m_viewFormat;
    device->CreateUnorderedAccessView(m_res, 0, uavDesc, destCpuHandle);
}

void TextureResourceBuilder::create2dSrv(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle) const
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.ViewDimension                  = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip      = m_mipFirstLevel;
    srvDesc.Texture2D.MipLevels            = m_mipLevelCount;
    srvDesc.Texture2D.PlaneSlice           = 0u;
    srvDesc.Texture2D.ResourceMinLODClamp  = 0.0f;
    createSrv(device, &srvDesc, destCpuHandle);
}

void TextureResourceBuilder::create2dArraySrv(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle) const
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.ViewDimension                       = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip      = m_mipFirstLevel;
    srvDesc.Texture2DArray.MipLevels            = m_mipLevelCount;
    srvDesc.Texture2DArray.FirstArraySlice      = m_arrayFirstSlice;
    srvDesc.Texture2DArray.ArraySize            = m_arraySliceCount;
    srvDesc.Texture2DArray.PlaneSlice           = 0u;
    srvDesc.Texture2DArray.ResourceMinLODClamp  = 0.0f;
    createSrv(device, &srvDesc, destCpuHandle);
}

void TextureResourceBuilder::create2dUav(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle) const
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.ViewDimension           = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice      = m_mipFirstLevel;
    uavDesc.Texture2D.PlaneSlice    = 0u;
    createUav(device, &uavDesc, destCpuHandle);
}

void TextureResourceBuilder::create2dArrayUav(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle) const
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.ViewDimension                   = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
    uavDesc.Texture2DArray.MipSlice         = m_mipFirstLevel;
    uavDesc.Texture2DArray.FirstArraySlice  = m_arrayFirstSlice;
    uavDesc.Texture2DArray.ArraySize        = m_arraySliceCount;
    uavDesc.Texture2DArray.PlaneSlice       = 0u;
    createUav(device, &uavDesc, destCpuHandle);
}

namespace
{
    void SetThumbnailViewportAndScissors(ID3D12GraphicsCommandList* cmdList, D3D12_VIEWPORT const* vp)
    {
        D3D12_RECT scissorRect =
        {
            long(vp->TopLeftX),
            long(vp->TopLeftY),
            long(vp->TopLeftX + vp->Width),
            long(vp->TopLeftY + vp->Height)
        };

        cmdList->RSSetViewports(1, vp);
        cmdList->RSSetScissorRects(1, &scissorRect);
    }

    const wchar_t* const s_modelPaths[] =
    {
        L"scanner.sdkmesh",
        L"occcity.sdkmesh",
        L"column.sdkmesh",
    };

    // Barebones definition of a scene.
    struct ObjectDefinition
    {
        Matrix  world;
        size_t  modelIndex;
        uint32_t stencilRef;
    };

    constexpr uint32_t kStencilRef0 = 0u;
    constexpr uint32_t kStencilRef1 = 127u;
    constexpr uint32_t kStencilRef2 = 255u;

    const ObjectDefinition s_sceneDefinition[] =
    {
        { XMMatrixIdentity(), 0, kStencilRef2 },
        { XMMatrixRotationY(XM_2PI * (1.0f / 6.0f)), 0, kStencilRef2 },
        { XMMatrixRotationY(XM_2PI * (2.0f / 6.0f)), 0, kStencilRef2 },
        { XMMatrixRotationY(XM_2PI * (3.0f / 6.0f)), 0, kStencilRef2 },
        { XMMatrixRotationY(XM_2PI * (4.0f / 6.0f)), 0, kStencilRef2 },
        { XMMatrixRotationY(XM_2PI * (5.0f / 6.0f)), 0, kStencilRef2 },
        { XMMatrixIdentity(), 1, kStencilRef0 },
        { XMMatrixIdentity(), 2, kStencilRef1 },
    };

    const float c_defaultPhi = XM_2PI / 6.0f;
    const float c_defaultRadius = 3.3f;

    const float c_cameraNear = 1.0f;
    const float c_cameraFar = 10.0f;

    const uint32_t  c_hTileTileWidth = 8;
    const uint32_t  c_hTileTileHeight = 8;

    const uint32_t  c_threadGroupDecodeHtileX = 8;
    const uint32_t  c_threadGroupDecodeHtileY = 8;

    static_assert(static_cast<int>(DXGI_FORMAT_D32_FLOAT) == static_cast<int>(XG_FORMAT_D32_FLOAT), "DXGI_FORMAT should match XG_FORMAT");
    static_assert(static_cast<int>(DXGI_FORMAT_D32_FLOAT_S8X24_UINT) == static_cast<int>(XG_FORMAT_D32_FLOAT_S8X24_UINT), "DXGI_FORMAT should match XG_FORMAT");

    __declspec(align(16)) struct ConstantBufferHTileParams
    {
        uint32_t    m_htileInfo;
        uint8_t     padding[12];
    };

    static_assert((sizeof(ConstantBufferHTileParams) % 16) == 0, "CB size not padded correctly");

    __declspec(align(16)) struct ConstantBufferOverlay
    {
        XMVECTOR    m_dims;
        float       m_farNearRatio;
        uint8_t     padding[12];
    };

    static_assert((sizeof(ConstantBufferOverlay) % 16) == 0, "CB size not padded correctly");
}

Sample::Sample() noexcept(false)
    : m_frame(0),
    m_theta(0.0f),
    m_phi(c_defaultPhi),
    m_radius(c_defaultRadius),
    m_depthTextureAddress(nullptr),
    m_depthTextureAddresses{},
    m_widthHtile(0),
    m_heightHtile(0),
    m_vpThumbnailHiZ{},
    m_vpThumbnailHiS{},
    m_vpThumbnailExpandedZ{},
    m_vpThumbnailExpandedS{},
#ifdef _GAMING_XBOX_SCARLETT
    m_hwVersion(D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_ANACONDA),
#else
    m_hwVersion(D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X),
    m_esram(false),
#endif
    m_stencil(true),
    m_resummarize(true),
    m_depthCompression(true),
    m_decompress(true),
    m_reset(false)
{
    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN, 2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD | DX::DeviceResources::c_ReverseDepth);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();

    auto device = m_deviceResources->GetD3DDevice();
    D3D12XBOX_GPU_HARDWARE_CONFIGURATION hwConfig;
    device->GetGpuHardwareConfigurationX(&hwConfig);
    m_hwVersion = hwConfig.HardwareVersion;
#ifdef _GAMING_XBOX_XBOXONE
    m_esram = m_hwVersion <= D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_S;
#endif

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
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        using State = GamePad::ButtonStateTracker::ButtonState;

        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.a == State::PRESSED)
        {
            m_stencil = !m_stencil;
            m_reset = true;
        }

        if (m_gamePadButtons.b == State::PRESSED)
        {
            m_resummarize = !m_resummarize;
        }

        if (m_gamePadButtons.y == State::PRESSED)
        {
            m_depthCompression = !m_depthCompression;
            m_reset = true;
        }

#ifdef _GAMING_XBOX_XBOXONE
        if (m_hwVersion <= D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_S  && m_gamePadButtons.x == State::PRESSED)
        {
            m_esram = !m_esram;
            m_reset = true;
        }
#endif

        if (m_gamePadButtons.dpadLeft == State::PRESSED
            || m_gamePadButtons.dpadRight == State::PRESSED)
        {
            m_decompress = pad.IsDPadRightPressed();
        }

        if (pad.IsRightStickPressed())
        {
            m_theta = 0.f;
            m_phi = c_defaultPhi;
            m_radius = c_defaultRadius;
        }
        else
        {
            m_theta += pad.thumbSticks.rightX * XM_PI * elapsedTime;
            m_phi -= pad.thumbSticks.rightY * XM_PI * elapsedTime;
            m_radius -= pad.thumbSticks.leftY * 5.f * elapsedTime;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    // Limit to avoid looking directly up or down
    m_phi = std::max(1e-2f, std::min(XM_PIDIV2, m_phi));
    m_radius = std::max(1.f, std::min(10.f, m_radius));

    if (m_theta > XM_PI)
    {
        m_theta -= XM_PI * 2.f;
    }
    else if (m_theta < -XM_PI)
    {
        m_theta += XM_PI * 2.f;
    }

    XMVECTOR lookFrom = XMVectorSet(
        m_radius * sinf(m_phi) * cosf(m_theta),
        m_radius * cosf(m_phi),
        m_radius * sinf(m_phi) * sinf(m_theta),
        0);

    m_view = XMMatrixLookAtLH(lookFrom, g_XMZero, g_XMIdentityR1);

    PIXEndEvent();
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

    // Put thumbnails into UAV state at the beginning of the frame
    {
        D3D12_RESOURCE_BARRIER barriers[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_HiZTexture.Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_HiSTexture.Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS),

            CD3DX12_RESOURCE_BARRIER::Transition(
                m_ZDecompressedTexture.Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_SDecompressedTexture.Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        commandList->ResourceBarrier(_countof(barriers), barriers);
    }

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");
    m_gpuTimer.BeginFrame(commandList);

    // Clear
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    Clear(); // Doesn't clear depth/stencil since we have custom behavior here

    // D3D12.X requires the title to choose hi stencil state.
    // The choice should ideally be made prior to clear of stencil.
    D3D12XBOX_HISTENCIL_CONTROL hiStencilControl = {};

    // We choose for State 0 to record "Is stencil equal to 0?"
    hiStencilControl.State0.Enabled = 1;
    hiStencilControl.State0.CompareFunction = D3D12XBOX_HISTENCIL_COMPARE_FUNCTION_EQUAL;
    hiStencilControl.State0.CompareValue = kStencilRef0;
    hiStencilControl.State0.CompareMask = 0xff;

    // We choose for State 1 to record "Is stencil equal to 1?"
    hiStencilControl.State1.Enabled = 1;
    hiStencilControl.State1.CompareFunction = D3D12XBOX_HISTENCIL_COMPARE_FUNCTION_EQUAL;
    hiStencilControl.State1.CompareValue = kStencilRef1;
    hiStencilControl.State1.CompareMask = 0xff;

    commandList->SetHiStencilStateX(m_depthTexture.Get(), &hiStencilControl);

    // Allow or disallow compression, on a resource which was created with compression support
    auto compressionFlags = D3D12XBOX_COMPRESSION_STATE_FLAG_NONE;
    auto preserveFlags = D3D12_RESOURCE_STATE_COMMON;   // This is the "NONE" state
    if (m_depthCompression)
    {
        compressionFlags = D3D12XBOX_COMPRESSION_STATE_FLAG_ENABLE_DEPTH;
        if (m_stencil)
        {
            compressionFlags |= D3D12XBOX_COMPRESSION_STATE_FLAG_ENABLE_STENCIL;
        }
    }
    else
    {
        preserveFlags |= D3D12XBOX_RESOURCE_STATE_PRESERVE_EXPANDED_DEPTH;
        if (m_stencil)
        {
            preserveFlags |= D3D12XBOX_RESOURCE_STATE_PRESERVE_EXPANDED_STENCIL;
        }
    }
    commandList->SetResourceCompressionStateX(m_depthTexture.Get(), compressionFlags, preserveFlags);

    // Clear the views.
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto const dsvDescriptor = m_dsvDescriptorHeap->GetFirstCpuHandle();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);

    commandList->ClearDepthStencilView(m_dsvDescriptorHeap->GetFirstCpuHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);

    PIXEndEvent(commandList);

    // Set descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { m_srvPile->Heap(), m_commonStates->Heap() };
    commandList->SetDescriptorHeaps(UINT(_countof(heaps)), heaps);

    // Draw the scene.
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render scene");

    for (auto& it : m_scene)
    {
        Model::UpdateEffectMatrices(it.effects, it.world, m_view, m_proj);
    }

    for (auto& it : m_scene)
    {
        commandList->OMSetStencilRef(it.stencilRef);

        it.model->DrawOpaque(commandList, it.effects.begin());
    }

    PIXEndEvent(commandList);

    m_gpuTimer.Start(commandList, 0);

    struct DepthOpHelper
    {
        static void ReSummarizeHTileInplace(ID3D12GraphicsCommandList *cmdList, ID3D12Resource *resource)
        {
            cmdList->CopyResourceX(resource, resource, D3D12XBOX_COPY_FLAG_RESUMMARIZE_HTILE);
        }

        static void ForceDecompressDepthInplace(ID3D12GraphicsCommandList *cmdList, ID3D12Resource *resource, bool stencil)
        {
            D3D12XBOX_COPY_FLAGS flags = D3D12XBOX_COPY_FLAG_FORCE_DECOMPRESS_EXPAND_DEPTH;
            if (stencil)
            {
                flags |= D3D12XBOX_COPY_FLAG_FORCE_DECOMPRESS_EXPAND_STENCIL;
            }
            cmdList->CopyResourceX(resource, resource, flags);
        }

        static void ForceDecompressDepthAndReSummarizeHTileInplace(ID3D12GraphicsCommandList *cmdList, ID3D12Resource *resource, bool stencil)
        {
            D3D12XBOX_COPY_FLAGS flags = D3D12XBOX_COPY_FLAG_RESUMMARIZE_HTILE | D3D12XBOX_COPY_FLAG_FORCE_DECOMPRESS_EXPAND_DEPTH;
            if (stencil)
            {
                flags |= D3D12XBOX_COPY_FLAG_FORCE_DECOMPRESS_EXPAND_STENCIL;
            }
            cmdList->CopyResourceX(resource, resource, flags);
        }

        static void Transition(ID3D12GraphicsCommandList *cmdList, ID3D12Resource *resource, D3D12_RESOURCE_STATES prevState, D3D12_RESOURCE_STATES nextState)
        {
            D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                resource,
                prevState,
                nextState
            );
            if (prevState != nextState)
            {
                cmdList->ResourceBarrier(1u, &barrier);
            }
        }
    };

    D3D12_RESOURCE_STATES preserveCompressedDepthFlags = D3D12XBOX_RESOURCE_STATE_PRESERVE_COMPRESSED_DEPTH;
    D3D12_RESOURCE_STATES preserveExpandedDepthFlags = D3D12XBOX_RESOURCE_STATE_PRESERVE_EXPANDED_DEPTH;

    if (m_stencil)
    {
        preserveCompressedDepthFlags |= D3D12XBOX_RESOURCE_STATE_PRESERVE_COMPRESSED_STENCIL;
        preserveExpandedDepthFlags |= D3D12XBOX_RESOURCE_STATE_PRESERVE_EXPANDED_STENCIL;
    }
    D3D12_RESOURCE_STATES lastState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    D3D12_RESOURCE_STATES nextState = D3D12_RESOURCE_STATE_COMMON;

    bool needsForceDecompress = m_depthCompression && m_decompress;

    /** NOTE:
     *  Manual decompression is activated when the Depth buffer is actually compressed
     */
    if (m_depthCompression)
    {
        nextState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        nextState |= preserveCompressedDepthFlags;

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"No decompression before Compressed Z/S Decode");
        DepthOpHelper::Transition(commandList, m_depthTexture.Get(), lastState, nextState);
        PIXEndEvent(commandList);

        commandList->SetComputeRootSignature(m_decompressRootSig.Get());
        commandList->SetComputeRootConstantBufferView(0, m_decodeCB.GpuAddress());
        commandList->SetComputeRootDescriptorTable(1, m_srvPile->GetGpuHandle(Descriptors::HTileSRV));
        commandList->SetComputeRootDescriptorTable(2, m_srvPile->GetGpuHandle(Descriptors::ZDecompressedUAV));

        static_assert(Descriptors::ZCompressedSRV == Descriptors::HTileSRV + 1, "HTile and ZCompressed Srvs must be contigous");
        static_assert(Descriptors::SCompressedSRV == Descriptors::HTileSRV + 2, "HTile and SCompressed Srvs must be contigous");
        static_assert(Descriptors::SDecompressedUAV == Descriptors::ZDecompressedUAV + 1, "SDecompressed and ZDecompressed Uavs must be contigous");


        ID3D12PipelineState* decodePSO = m_stencil ? m_decompressDepthStencilPSO.Get() : m_decompressDepthPSO.Get();

        commandList->SetPipelineState(decodePSO);

        auto const size = m_deviceResources->GetOutputSize();

        uint32_t numThreadgroupsX = AlignUp(static_cast<uint32_t>(size.right), 8u) / 8u;
        uint32_t numThreadgroupsY = AlignUp(static_cast<uint32_t>(size.bottom), 8u) / 8u;

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Compressed Z/S Decode");
        commandList->Dispatch(numThreadgroupsX, numThreadgroupsY, 1);
        PIXEndEvent(commandList);

        lastState = nextState;
    }

#if DISABLE_FORCE_DECOMPRESS_WITH_RESUMMARIZATION
    if (m_resummarize)
#else
    if (m_resummarize && !needsForceDecompress)
#endif
    {
        nextState = D3D12_RESOURCE_STATE_COPY_DEST;

        /** NOTE:
         *  When Depth Compression enabled for Depth buffer, the barrier should preserve compressed state
         *  to make sure HTile of compressed Depth buffer resummarized
         */
        if (m_depthCompression)
        {
            nextState |= preserveCompressedDepthFlags;
        }

#if _GAMING_XBOX_XBOXONE
        commandList->FlushPipelineX(D3D12XBOX_FLUSH_BOP_CS_PARTIAL, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
#endif

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"No decompression before HTile Re-Summarization");
        DepthOpHelper::Transition(commandList, m_depthTexture.Get(), lastState, nextState);
        PIXEndEvent(commandList);

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"No decompression during HTile Re-Summarization");
        DepthOpHelper::ReSummarizeHTileInplace(commandList, m_depthTexture.Get());
        PIXEndEvent(commandList);

        lastState = nextState;
    }

    if (needsForceDecompress)
    {
        nextState = D3D12_RESOURCE_STATE_COPY_DEST;
        nextState |= preserveCompressedDepthFlags;

#if DISABLE_FORCE_DECOMPRESS_WITH_RESUMMARIZATION
        bool needForcedDecompressWithHTileReSummarization = false;
#else
        bool needForcedDecompressWithHTileReSummarization = m_resummarize;
#endif
        if (needForcedDecompressWithHTileReSummarization)
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Compressed Depth Buffer, No decompression before Force Depth Expand and HTile Re-Summarization");
            DepthOpHelper::Transition(commandList, m_depthTexture.Get(), lastState, nextState);
            PIXEndEvent(commandList);

            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Compressed Depth Buffer, Decompression during Force Depth Expand and HTile Re-Summarization");
            DepthOpHelper::ForceDecompressDepthAndReSummarizeHTileInplace(commandList, m_depthTexture.Get(), m_stencil);
            PIXEndEvent(commandList);
        }
        else
        {
            if (lastState != nextState)
            {
#if _GAMING_XBOX_XBOXONE
                if (!m_resummarize)
                    commandList->FlushPipelineX(D3D12XBOX_FLUSH_BOP_CS_PARTIAL, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
#endif

                PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Compressed Depth Buffer, No decompression before Force Depth Expand");
                DepthOpHelper::Transition(commandList, m_depthTexture.Get(), lastState, nextState);
                PIXEndEvent(commandList);
            }

            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Compressed Depth Buffer, Decompression during Force Depth Expand");
            DepthOpHelper::ForceDecompressDepthInplace(commandList, m_depthTexture.Get(), m_stencil);
            PIXEndEvent(commandList);
        }

        lastState = D3D12_RESOURCE_STATE_COPY_DEST | preserveExpandedDepthFlags;
        nextState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Force Decompressed Depth Buffer, No decompression before HTile Decode");
        DepthOpHelper::Transition(commandList, m_depthTexture.Get(), lastState, nextState);
        PIXEndEvent(commandList);
    }
    else if (m_depthCompression)
    {
        nextState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | preserveCompressedDepthFlags;

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Compressed Depth Buffer, No decompression before HTile Decode");
        DepthOpHelper::Transition(commandList, m_depthTexture.Get(), lastState, nextState);
        PIXEndEvent(commandList);
    }
    else
    {
        nextState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Expanded Depth Buffer, No decompression before HTile Decode");
        DepthOpHelper::Transition(commandList, m_depthTexture.Get(), lastState, nextState);
        PIXEndEvent(commandList);
    }
    lastState = nextState;
    {
        commandList->SetComputeRootSignature(m_decodeRootSig.Get());
        commandList->SetComputeRootConstantBufferView(e_rootParameterCB, m_decodeCB.GpuAddress());
        commandList->SetComputeRootDescriptorTable(e_rootParameterHTile, m_srvPile->GetGpuHandle(Descriptors::HTileSRV));
        commandList->SetComputeRootDescriptorTable(e_rootParameterHiZ_HiS, m_srvPile->GetGpuHandle(Descriptors::HiZTextureUAV));
        static_assert(Descriptors::HiSTextureUAV == Descriptors::HiZTextureUAV + 1, "Descriptors must be continuous");

        ID3D12PipelineState* decodePSO = m_stencil ? m_decodePSOStencil.Get() : m_decodePSO.Get();

        commandList->SetPipelineState(decodePSO);

        uint32_t widthInThreadGroups = AlignUp(m_widthHtile, c_threadGroupDecodeHtileX) / c_threadGroupDecodeHtileX;
        uint32_t heightInThreadGroups = AlignUp(m_heightHtile, c_threadGroupDecodeHtileY) / c_threadGroupDecodeHtileY;

        // Decode htile.
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"HTile Decode");
        commandList->Dispatch(widthInThreadGroups, heightInThreadGroups, 1);
        PIXEndEvent(commandList);
    }

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Barriers after HTile Decode");
    {
        D3D12_RESOURCE_BARRIER barriers[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_HiZTexture.Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

            CD3DX12_RESOURCE_BARRIER::Transition(
                m_HiSTexture.Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

            CD3DX12_RESOURCE_BARRIER::Transition(
                m_ZDecompressedTexture.Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_SDecompressedTexture.Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

            CD3DX12_RESOURCE_BARRIER::Transition(
                m_depthTexture.Get(),
                lastState,
                D3D12_RESOURCE_STATE_DEPTH_WRITE)
        };
        commandList->ResourceBarrier(_countof(barriers), barriers);
    }
    PIXEndEvent(commandList);

    m_gpuTimer.Stop(commandList, 0);

    // Render thumbnails.
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Thumbnails");

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    SetThumbnailViewportAndScissors(commandList, &m_vpThumbnailHiZ);
    m_fullScreenQuad->Draw(commandList, m_hizPSO.Get(), m_srvPile->GetGpuHandle(Descriptors::HiZTextureSRV), m_overlayCB.GpuAddress());

    if (m_depthCompression)
    {
        SetThumbnailViewportAndScissors(commandList, &m_vpThumbnailExpandedZ);
        m_fullScreenQuad->Draw(commandList, m_overlayPSO.Get(), m_srvPile->GetGpuHandle(Descriptors::ZDecompressedSRV), m_overlayCB.GpuAddress());
    }

    if (m_stencil)
    {
        SetThumbnailViewportAndScissors(commandList, &m_vpThumbnailHiS);
        m_fullScreenQuad->Draw(commandList, m_hisPSO.Get(), m_srvPile->GetGpuHandle(Descriptors::HiSTextureSRV), m_overlayCB.GpuAddress());

        if (m_depthCompression)
        {
            SetThumbnailViewportAndScissors(commandList, &m_vpThumbnailExpandedS);
            m_fullScreenQuad->Draw(commandList, m_overlayPSO.Get(), m_srvPile->GetGpuHandle(Descriptors::SDecompressedSRV), m_overlayCB.GpuAddress());
        }
    }

    PIXEndEvent(commandList);

    auto const vp = m_deviceResources->GetScreenViewport();
    commandList->RSSetViewports(1, &vp);

    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetScissorRects(1, &scissorRect);

    DrawHUD(commandList);

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

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

void Sample::DrawHUD(ID3D12GraphicsCommandList* commandList)
{
    m_hudBatch->Begin(commandList);

    auto const size = m_deviceResources->GetOutputSize();

    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));

    wchar_t textBuffer[128] = {};
    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));

    float spacing = m_smallFont->GetLineSpacing();;

    swprintf_s(textBuffer, L"[A] Stencil = %ls", m_stencil ? L"TRUE" : L"FALSE");
    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(),
        textBuffer, textPos, ATG::ColorsLinear::Blue);
    textPos.y += spacing;

    swprintf_s(textBuffer, L"[B] Resummarize = %ls", m_resummarize ? L"TRUE" : L"FALSE");
    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(),
        textBuffer, textPos, ATG::ColorsLinear::Blue);
    textPos.y += spacing;

    swprintf_s(textBuffer, L"[Y] Compression = %ls", m_depthCompression ? L"TRUE" : L"FALSE");
    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(),
        textBuffer, textPos, ATG::ColorsLinear::Blue);
    textPos.y += spacing;

#ifdef _GAMING_XBOX_XBOXONE
    if (m_hwVersion <= D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_S)
    {
        swprintf_s(textBuffer, L"[X] ESRAM = %ls", m_esram ? L"TRUE" : L"FALSE");
        DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(),
            textBuffer, textPos, ATG::ColorsLinear::Blue);
        textPos.y += spacing;
    }
#endif

    swprintf_s(textBuffer, L"[DPad] Decompress = %ls", m_decompress ? L"TRUE" : L"FALSE");
    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(),
        textBuffer, textPos, ATG::ColorsLinear::Blue);

    textPos.y += 1.5f * spacing;

    // Timing
    float decodeTime = m_gpuTimer.GetAverageMS(0);
    swprintf_s(textBuffer, L"Decode (+Resummarize +Decompress) GPU time: %3.3f ms", decodeTime);
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, ATG::ColorsLinear::Blue);

    // Draw thumbnail titles
    textPos.x = m_vpThumbnailHiZ.TopLeftX;
    textPos.y = m_vpThumbnailHiZ.TopLeftY - spacing;
    m_smallFont->DrawString(m_hudBatch.get(), L"Hi Z (darker -> nearer, greener -> wider range)", textPos, ATG::ColorsLinear::Green);

    textPos.x = m_vpThumbnailHiS.TopLeftX;
    textPos.y = m_vpThumbnailHiS.TopLeftY - spacing;
    m_smallFont->DrawString(m_hudBatch.get(), L"Hi S (red -> state 0, green -> state 1)", textPos, ATG::ColorsLinear::Orange);

    if (m_depthCompression)
    {
        textPos.x = m_vpThumbnailExpandedZ.TopLeftX;
        textPos.y = m_vpThumbnailExpandedZ.TopLeftY - spacing;
        m_smallFont->DrawString(m_hudBatch.get(), L"CS Decompressed Depth", textPos, ATG::ColorsLinear::Green);

        if (m_stencil)
        {
            textPos.x = m_vpThumbnailExpandedS.TopLeftX;
            textPos.y = m_vpThumbnailExpandedS.TopLeftY - spacing;
            m_smallFont->DrawString(m_hudBatch.get(), L"CS Decompressed Stencil", textPos, ATG::ColorsLinear::Orange);
        }
    }
    // Draw legend
    textPos.x = float(safe.left);
    textPos.y = float(safe.bottom) - 2.0f * m_smallFont->GetLineSpacing();
    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(),
        L"[LThumb] Toward/Away   [RThumb]: Orbit Camera   [View] Exit", textPos, ATG::ColorsLinear::DarkGrey);

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

#if DISABLE_VALIDATION_ERRORS_ON_COPY_SRC_DST_BARRIER
    D3D12XBOX_DEBUG_FILTER_FLAGS debugFilterFlags = D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_FAILURE
                                                  | D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_BREAKS
                                                  | D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_OUTPUT;

    #if _GAMING_XBOX_SCARLETT
        device->SetDebugErrorFilterX(0x8173BF71, debugFilterFlags);
    #else
        device->SetDebugErrorFilterX(0x4ff3b9d6, debugFilterFlags);
    #endif
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    m_commonStates = std::make_unique<DirectX::CommonStates>(device);

    m_srvPile = std::make_unique<DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        128,
        Descriptors::Count);

    m_dsvDescriptorHeap = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        1);

    m_fullScreenQuad = std::make_unique<DX::FullScreenQuad>();
    m_fullScreenQuad->Initialize(device);

    //--- Load HTile Decoding Compute shaders
    {
        auto blob = DX::ReadData(L"HiZDecodeCS.cso");

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

        blob = DX::ReadData(L"HiZDecodeCS_Stencil.cso");

        descComputePSO.CS.pShaderBytecode = blob.data();
        descComputePSO.CS.BytecodeLength = blob.size();

        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_decodePSOStencil.ReleaseAndGetAddressOf())));

        m_decodePSOStencil->SetName(L"Decode PSO Stencil");
    }

    {
        auto blob = DX::ReadData(L"DecompressDepthCS.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, blob.data(), blob.size(),
                IID_GRAPHICS_PPV_ARGS(m_decompressRootSig.ReleaseAndGetAddressOf())));

        m_decompressRootSig->SetName(L"Decompress RS");

        D3D12_COMPUTE_PIPELINE_STATE_DESC decompressCsPso = {};
        decompressCsPso.pRootSignature = m_decompressRootSig.Get();

        decompressCsPso.CS = { blob.data(), blob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&decompressCsPso, IID_GRAPHICS_PPV_ARGS(m_decompressDepthPSO.ReleaseAndGetAddressOf())));

        m_decompressDepthPSO->SetName(L"Decompress Depth PSO");

        blob = DX::ReadData(L"DecompressDepthStencilCS.cso");

        decompressCsPso.CS = { blob.data(), blob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&decompressCsPso, IID_GRAPHICS_PPV_ARGS(m_decompressDepthStencilPSO.ReleaseAndGetAddressOf())));

        m_decompressDepthStencilPSO->SetName(L"Decompress Depth Stencil PSO");
    }

    //--- Load Scene
    m_models.resize(_countof(s_modelPaths));
    for (size_t i = 0; i < m_models.size(); ++i)
    {
        m_models[i] = Model::CreateFromSDKMESH(device, s_modelPaths[i]);
    }

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Optimize meshes for rendering
    for (size_t i = 0; i < m_models.size(); ++i)
    {
        m_models[i]->LoadStaticBuffers(device, resourceUpload);
    }

    // Upload textures to GPU.
    m_textureFactory = std::make_unique<EffectTextureFactory>(device, resourceUpload, m_srvPile->Heap());

    auto texOffsets = std::vector<size_t>(m_models.size());
    for (size_t i = 0; i < m_models.size(); ++i)
    {
        size_t junk;
        m_srvPile->AllocateRange(m_models[i]->textureNames.size(), texOffsets[i], junk);

        m_models[i]->LoadTextures(*m_textureFactory, int(texOffsets[i]));
    }

    {
        RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), DXGI_FORMAT_UNKNOWN);

        SpriteBatchPipelineStateDescription psd(rtState, &CommonStates::AlphaBlend);

        m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, psd);
    }

    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    finished.wait();

    {
        m_scene.resize(_countof(s_sceneDefinition));
        for (size_t i = 0; i < m_scene.size(); i++)
        {
            size_t index = s_sceneDefinition[i].modelIndex;

            assert(index < m_models.size());

            m_scene[i].txtOffset = texOffsets[index];
            m_scene[i].world = s_sceneDefinition[i].world;
            m_scene[i].model = m_models[index].get();
            m_scene[i].stencilRef = s_sceneDefinition[i].stencilRef;
        }
    }

    m_gpuTimer.RestoreDevice(device, m_deviceResources->GetCommandQueue());
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    const auto size = m_deviceResources->GetOutputSize();

    // Set hud sprite viewport
    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());

    // Set camera parameters.
    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), c_cameraFar, c_cameraNear);

    // Font resources
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
            (size.bottom > 1080) ? L"SegoeUI_24.spritefont" : L"SegoeUI_18.spritefont",
            m_srvPile->GetCpuHandle(Descriptors::TextFont),
            m_srvPile->GetGpuHandle(Descriptors::TextFont));

        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            (size.bottom > 1080) ? L"XboxOneControllerLegend.spritefont" : L"XboxOneControllerLegendSmall.spritefont",
            m_srvPile->GetCpuHandle(Descriptors::ControllerFont),
            m_srvPile->GetGpuHandle(Descriptors::ControllerFont));

        auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        finished.wait();
    }

    // Set up thumbnail viewports
    m_vpThumbnailHiZ.Width = m_vpThumbnailHiS.Width = float(size.right) / 4.f;
    m_vpThumbnailHiZ.Height = m_vpThumbnailHiS.Height = float(size.bottom) / 4.f;
    m_vpThumbnailHiZ.TopLeftX = 0.93f * float(size.right) - m_vpThumbnailHiZ.Width;
    m_vpThumbnailHiZ.TopLeftY = 0.10f * float(size.bottom);

    m_vpThumbnailHiS.TopLeftX = 0.93f * float(size.right) - m_vpThumbnailHiS.Width;
    m_vpThumbnailHiS.TopLeftY = 0.40f * float(size.bottom);

    m_vpThumbnailHiZ.MinDepth = m_vpThumbnailHiS.MinDepth = 0.f;
    m_vpThumbnailHiZ.MaxDepth = m_vpThumbnailHiS.MaxDepth = 1.f;

    m_vpThumbnailExpandedZ.Width = m_vpThumbnailExpandedS.Width = float(size.right) / 4.f;
    m_vpThumbnailExpandedZ.Height = m_vpThumbnailExpandedS.Height = float(size.bottom) / 4.f;

    m_vpThumbnailExpandedZ.TopLeftX = 0.91f * float(size.right) - 2.0f * float(m_vpThumbnailExpandedZ.Width);
    m_vpThumbnailExpandedZ.TopLeftY = 0.10f * float(size.bottom);

    m_vpThumbnailExpandedS.TopLeftX = 0.91f * float(size.right) - 2.0f * float(m_vpThumbnailExpandedS.Width);
    m_vpThumbnailExpandedS.TopLeftY = 0.40f * float(size.bottom);

    m_vpThumbnailExpandedZ.MinDepth = m_vpThumbnailExpandedS.MinDepth = 0.f;
    m_vpThumbnailExpandedZ.MaxDepth = m_vpThumbnailExpandedS.MaxDepth = 1.f;

    InitializeDepthResources();
}
#pragma endregion

void Sample::InitializeDepthResources()
{
    if (m_depthTextureAddress)
    {
        m_deviceResources->WaitForGpu();

        m_depthTexture.Reset();
        m_hTileBuffer.Reset();
        m_HiZTexture.Reset();
        m_HiSTexture.Reset();

        VirtualFree(m_depthTextureAddress, 0, MEM_RELEASE);

        m_depthTextureAddress = nullptr;
    }

    //--- Create depth buffer --------------------------------------------------------------
    auto const size = m_deviceResources->GetOutputSize();

    auto const width = static_cast<uint32_t>(size.right);
    auto const height = static_cast<uint32_t>(size.bottom);

    m_widthHtile = AlignUp(width, c_hTileTileWidth) / c_hTileTileWidth;
    m_heightHtile = AlignUp(height, c_hTileTileHeight) / c_hTileTileHeight;

    // sort out depth/stencil types (no point using D24S8 on XBox One, always prefer D32S8)
    auto const formatDepth= m_stencil ? DXGI_FORMAT_D32_FLOAT_S8X24_UINT : DXGI_FORMAT_D32_FLOAT;

    // retrieve tile modes
#ifdef _GAMING_XBOX_SCARLETT
    XG_SWIZZLE_MODE tileModeDepth = XG_SWIZZLE_MODE_INVALID;
    XGComputeOptimalDepthStencilSwizzleMode(static_cast<XG_FORMAT>(formatDepth), width, height, 1, 1, TRUE, FALSE, FALSE, &tileModeDepth);
#else
    XG_TILE_MODE tileModeDepth = XG_TILE_MODE_INVALID, tileModeStencil = XG_TILE_MODE_INVALID;
    XGComputeOptimalDepthStencilTileModes(static_cast<XG_FORMAT>(formatDepth), width, height, 1, 1, TRUE, FALSE, FALSE, &tileModeDepth, &tileModeStencil);
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
        static_cast<XG_FORMAT>(formatDepth),
        { 1U, 0U, },
        static_cast<XG_TEXTURE_LAYOUT>(0x100 | tileModeDepth),
        XG12_RESOURCE_MISC_ALLOW_DEPTH_STENCIL,
    };

    ComPtr<XGTextureAddressComputer> computer;
    DX::ThrowIfFailed(XGCreateTextureComputer(&depthBufferXGDesc, computer.GetAddressOf()));

    XG_RESOURCE_LAYOUT depthBufferLayout = {};
    DX::ThrowIfFailed(computer->GetResourceLayout(&depthBufferLayout));

    // Reserve virtual address
    DWORD allocType = MEM_2MB_PAGES | MEM_RESERVE | MEM_COMMIT;
#ifdef _GAMING_XBOX_XBOXONE
    if (m_esram)
    {
        // Not using ESRAM
        allocType &= ~MEM_COMMIT;
    }
#endif

    m_depthTextureAddress = XMemVirtualAlloc(nullptr,
        depthBufferLayout.SizeBytes,
        allocType,
        XMEM_GRAPHICS, PAGE_READWRITE | PAGE_GRAPHICS_READWRITE | PAGE_WRITECOMBINE);

    if (!m_depthTextureAddress)
    {
        throw std::bad_alloc();
    }

#ifdef _GAMING_XBOX_XBOXONE
    if (m_esram)
    {
        // Map ESRAM pages
        const uint32_t c_pageSize = 2 * 1024 * 1024;
        const uint32_t c_ESRAMSize = 32 * 1024 * 1024;

        uint32_t numESRAMPages = static_cast<uint32_t>(AlignUp(depthBufferLayout.SizeBytes, c_pageSize) / c_pageSize);
        uint32_t ESRAMPageList[c_ESRAMSize / c_pageSize];
        for (uint32_t i = 0; i < _countof(ESRAMPageList); ++i)
        {
            ESRAMPageList[i] = i;
        }

        DX::ThrowIfFailed(D3DMapEsramMemory(D3D11_MAP_ESRAM_2MB_PAGES,
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

        case XG_PLANE_USAGE_UNUSED:
        case XG_PLANE_USAGE_DEFAULT:
        case XG_PLANE_USAGE_COLOR_MASK:
        case XG_PLANE_USAGE_FRAGMENT_MASK:
        case XG_PLANE_USAGE_LUMA:
        case XG_PLANE_USAGE_CHROMA:
        case XG_PLANE_USAGE_DELTA_COLOR_COMPRESSION:
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

    auto device = m_deviceResources->GetD3DDevice();
    ConstantBufferHTileParams cbData = {};
    {
        // Generate an HTile descriptor for the compute decompression system
        uint32_t TileCountX = paddedWidthElements / c_hTileTileWidth;
        uint32_t TileCountY = paddedHeightElements / c_hTileTileHeight;
#ifdef _GAMING_XBOX_SCARLETT
        const uint32_t IsHTileLinear = 0u;
        const uint32_t PipeCount = m_hwVersion == D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART ? 8u : 32u;
        const uint32_t MacroTileWidth = m_hwVersion == D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART ? 64u : 128u;
        const uint32_t MacroTileHeight = m_hwVersion == D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART ? 64u : 128u;
#else
        const uint32_t IsHTileLinear = width * height < 0x200000 ? 1u : 0u;
        const uint32_t PipeCount = m_hwVersion >= D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X ? 8u : 4u;
        const uint32_t MacroTileWidth = 64u;
        const uint32_t MacroTileHeight = 8u * PipeCount;
#endif
        if (!IsHTileLinear)
        {
            TileCountX = AlignUp(TileCountX, MacroTileWidth);
            TileCountY = AlignUp(TileCountY, MacroTileHeight);
            assert(htileSizeBytes == TileCountX * TileCountY * 4);
        }
        const uint32_t HTileInfo = IsHTileLinear << 31 | PipeCount << 24 | TileCountX | TileCountY << 12;
        cbData.m_htileInfo = HTileInfo;
    }
    m_decodeCB = m_graphicsMemory->AllocateConstant(cbData);

    ConstantBufferOverlay cbData2 = {};
    cbData2.m_dims = XMVectorSet(float(width) / (float)c_hTileTileWidth, float(height) / (float)c_hTileTileHeight, 0.0f, 0.0f);
    cbData2.m_farNearRatio = c_cameraFar / c_cameraNear;
    m_overlayCB = m_graphicsMemory->AllocateConstant(cbData2);

    // Creat resources
    D3D12_CLEAR_VALUE clearValue = { formatDepth, { { 0.0f, 0} } };

    auto depthBufferD3D12Desc = reinterpret_cast<D3D12_RESOURCE_DESC *>(&depthBufferXGDesc);

    /** NOTE: Re-summarization of Depth buffers with enabled EXPLCLEAR is not supported */
    depthBufferD3D12Desc->Flags |= D3D12XBOX_RESOURCE_FLAG_DENY_DEPTH_COMPRESSION_EXPCLEAR;

    DX::ThrowIfFailed(device->CreatePlacedResourceX(
        *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(&m_depthTextureAddress),
        depthBufferD3D12Desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_GRAPHICS_PPV_ARGS(m_depthTexture.GetAddressOf())));

    m_depthTexture->SetName(L"Depth texture");

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = formatDepth;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    device->CreateDepthStencilView(m_depthTexture.Get(), &dsvDesc, m_dsvDescriptorHeap->GetFirstCpuHandle());

    // Create a texture which holds min/max depth starting from htile dimensions
    CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    TextureResourceBuilder builder;
    builder.initAsTex2d((uint16_t)m_widthHtile, (uint16_t)m_heightHtile, DXGI_FORMAT_R16G16_UINT);

    m_HiZTexture.Attach(builder.createDefaultHeapResource(device, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    m_HiZTexture->SetName(L"m_HiZTexture");
    builder.create2dSrv(device, m_srvPile->GetCpuHandle(Descriptors::HiZTextureSRV));
    builder.create2dUav(device, m_srvPile->GetCpuHandle(Descriptors::HiZTextureUAV));

    builder.setFormat(DXGI_FORMAT_R8G8_UINT);
    m_HiSTexture.Attach(builder.createDefaultHeapResource(device, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    m_HiSTexture->SetName(L"m_HiSTexture");
    builder.create2dSrv(device, m_srvPile->GetCpuHandle(Descriptors::HiSTextureSRV));
    builder.create2dUav(device, m_srvPile->GetCpuHandle(Descriptors::HiSTextureUAV));

    builder.initAsTex2d((uint16_t)width, (uint16_t)height, DXGI_FORMAT_R32_FLOAT);
    m_ZDecompressedTexture.Attach(builder.createDefaultHeapResource(device, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    m_ZDecompressedTexture->SetName(L"m_ZDecompressedTexture");
    builder.create2dSrv(device, m_srvPile->GetCpuHandle(Descriptors::ZDecompressedSRV));
    builder.create2dUav(device, m_srvPile->GetCpuHandle(Descriptors::ZDecompressedUAV));

    builder.setFormat(DXGI_FORMAT_R8_TYPELESS);
    m_SDecompressedTexture.Attach(builder.createDefaultHeapResource(device, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    m_SDecompressedTexture->SetName(L"m_SDecompressedTexture");

    builder.setFormat(DXGI_FORMAT_R8_UNORM);
    builder.create2dSrv(device, m_srvPile->GetCpuHandle(Descriptors::SDecompressedSRV));

    builder.setFormat(DXGI_FORMAT_R8_UINT);
    builder.create2dUav(device, m_srvPile->GetCpuHandle(Descriptors::SDecompressedUAV));

    // Create a texture which can hold htile information (encoded)
    // This must be a placement allocation because it aliases the actual htile data.
    {
        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(
            htileSizeBytes,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            htileAlignmentInBytes
        );

        DX::ThrowIfFailed(
            device->CreatePlacedResourceX(
                *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(&m_depthTextureAddresses.DepthStencilTarget.HTile),
                &desc,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_hTileBuffer.GetAddressOf())));

        m_hTileBuffer->SetName(L"HTile buffer");

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                  = DXGI_FORMAT_R32_TYPELESS;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.NumElements      = htileSizeBytes / 4;
        srvDesc.Buffer.Flags            = D3D12_BUFFER_SRV_FLAG_RAW;
        device->CreateShaderResourceView(m_hTileBuffer.Get(), &srvDesc, m_srvPile->GetCpuHandle(Descriptors::HTileSRV));
    }

    {
        builder.initAsTex2d((uint16_t)width, (uint16_t)height, DXGI_FORMAT_R32_FLOAT);
        builder.setLayout(static_cast<XG_TEXTURE_LAYOUT>(0x100 | tileModeDepth));

        m_ZCompressedTexture.Attach(builder.createPlacedResourceX(device,
            m_depthTextureAddresses.DepthStencilTarget.DepthSamples,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
        m_ZCompressedTexture->SetName(L"m_ZCompressedTexture");
        builder.create2dSrv(device, m_srvPile->GetCpuHandle(Descriptors::ZCompressedSRV));

        builder.setFormat(DXGI_FORMAT_R8_UINT);

        m_SCompressedTexture.Attach(builder.createPlacedResourceX(device,
            m_depthTextureAddresses.DepthStencilTarget.StencilSamples,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
        m_SCompressedTexture->SetName(L"m_SCompressedTexture");
        builder.create2dSrv(device, m_srvPile->GetCpuHandle(Descriptors::SCompressedSRV));
    }

    //-- Create PSOs that rely on DSV format
    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), formatDepth);

    {
        EffectPipelineStateDescription overlay(nullptr,
            CommonStates::Opaque,
            CommonStates::DepthNone,
            CommonStates::CullNone,
            rtState);

        auto rootSig = m_fullScreenQuad->GetRootSignature();

        auto blobvs = DX::ReadData(L"FullScreenQuadVS.cso");
        D3D12_SHADER_BYTECODE vs = { blobvs.data(), blobvs.size() };

        auto blob = DX::ReadData(L"FullScreenQuadPS.cso");
        D3D12_SHADER_BYTECODE ps = { blob.data(), blob.size() };

        overlay.CreatePipelineState(device, rootSig, vs, ps, m_overlayPSO.ReleaseAndGetAddressOf());
        m_overlayPSO->SetName(L"Overlay PSO");

        blob = DX::ReadData(L"HiZDecodeDepth.cso");
        ps = { blob.data(), blob.size() };

        overlay.CreatePipelineState(device, rootSig, vs, ps, m_hizPSO.ReleaseAndGetAddressOf());
        m_hizPSO->SetName(L"HiZ PSO");

        blob = DX::ReadData(L"HiZDecodeStencil.cso");
        ps = { blob.data(), blob.size() };

        overlay.CreatePipelineState(device, rootSig, vs, ps, m_hisPSO.ReleaseAndGetAddressOf());
        m_hisPSO->SetName(L"HiS PSO");
    }

    {
        // Using reverse z
        auto testAndWrite = CD3DX12_DEPTH_STENCIL_DESC(
            TRUE, D3D12_DEPTH_WRITE_MASK_ALL, D3D12_COMPARISON_FUNC_GREATER_EQUAL, TRUE, 0xff, 0xff,
            D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_REPLACE, D3D12_COMPARISON_FUNC_ALWAYS,
            D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_REPLACE, D3D12_COMPARISON_FUNC_ALWAYS);

        EffectPipelineStateDescription psd(
            nullptr,
            CommonStates::Opaque,
            testAndWrite,
            CommonStates::CullCounterClockwise,
            rtState,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

        auto effectFactory = EffectFactory(m_srvPile->Heap(), m_commonStates->Heap());

        for (size_t i = 0; i < m_scene.size(); i++)
        {
            m_scene[i].effects = m_scene[i].model->CreateEffects(effectFactory, psd, psd, int(m_scene[i].txtOffset));

            std::for_each(
                m_scene[i].effects.begin(),
                m_scene[i].effects.end(),
                [&](std::shared_ptr<IEffect>& e)
            {
                static_cast<BasicEffect*>(e.get())->SetEmissiveColor(Colors::White);
            });
        }
    }

    m_reset = false;
}
