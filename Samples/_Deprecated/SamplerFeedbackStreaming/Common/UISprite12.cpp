//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

#include "precomp.hpp"
#include "UISprite12.h"
#include "util.hpp"
#include "ui2-r8.h"

#include "UISprite11.vfxhpp"
#include "UISprite11text.pfxhpp"
#include "UISprite11modulate.pfxhpp"
#include "UISprite11modulatecube.pfxhpp"
#include "UISprite11modulatearray.pfxhpp"
#include "UISprite11modulatevolume.pfxhpp"
#include "UISprite11modulate1d.pfxhpp"
#include "UISprite11modulateint.pfxhpp"

struct CONSTANTBUFFER
{
    XMFLOAT4X4      m_WorldViewProjection;
    XMFLOAT4        m_UVector;
    XMFLOAT4        m_VVector;
    XMFLOAT4        m_WVector;
    XMFLOAT4        m_LODSelection;
};

static const FLOAT g_BlendFactor[4] = { 1, 1, 1, 1 };

HRESULT LoadRawDDS( const SpriteInitParams12* pInitParams, const void* pDDSFile, DXGI_FORMAT Format, UINT32 Width, UINT32 Height, UINT32 RowPitchBytes, D3D12_CPU_DESCRIPTOR_HANDLE Handle, ID3D12Resource** ppResource )
{
    ID3D12Device* pd3dDevice = pInitParams->pd3dDevice12;

    D3D12_RESOURCE_DESC TexDesc = {};
    TexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    TexDesc.Alignment = 0;
    TexDesc.Width = Width;
    TexDesc.Height = Height;
    TexDesc.DepthOrArraySize = 1;
    TexDesc.Format = Format;
    TexDesc.MipLevels = 1;
    TexDesc.SampleDesc.Count = 1;
    TexDesc.SampleDesc.Quality = 0;
    TexDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    TexDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource* pTex2D = nullptr;
    HRESULT hr = CreateDefaultResource(pd3dDevice, 
                                       &TexDesc, 
                                       D3D12_RESOURCE_STATE_COMMON, 
                                       (void**)&pTex2D);
    if (FAILED(hr))
    {
        return hr;
    }

    pd3dDevice->CreateShaderResourceView(pTex2D, nullptr, Handle);

    ID3D12GraphicsCommandList* pCmdList = nullptr;
    hr = pd3dDevice->CreateCommandList(D3D12XBOX_NODE_MASK, 
                                       D3D12_COMMAND_LIST_TYPE_DIRECT, 
                                       pInitParams->pd3dCmdAllocatorDirect, 
                                       nullptr, 
                                       __uuidof(ID3D12GraphicsCommandList), 
                                       (void**)&pCmdList);
    assert(SUCCEEDED(hr));

    ResourceBarrier(pCmdList, pTex2D, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

    D3D12_SUBRESOURCE_FOOTPRINT PitchDesc = {};
    PitchDesc.Width = (UINT32)TexDesc.Width;
    PitchDesc.Height = TexDesc.Height;
    PitchDesc.Depth = 1;
    PitchDesc.Format = TexDesc.Format;
    PitchDesc.RowPitch = NextMultiple(RowPitchBytes, (UINT32)D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT);

    const BYTE* pInitData = (const BYTE*)pDDSFile + 128;

    pInitParams->pUploadHeap->CopyTextureSubresourceToDefaultTexture(pInitData, RowPitchBytes, PitchDesc, pCmdList, pTex2D, 0);

    ResourceBarrier(pCmdList, pTex2D, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    pCmdList->Close();
    pInitParams->pd3dCmdQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&pCmdList);
    SAFE_RELEASE(pCmdList);

    *ppResource = pTex2D;

    return hr;
}

HRESULT UISprite12::Initialize( const SpriteInitParams12* pInitParams, UINT MaxQuadCount )
{
    ID3D12Device* pd3dDevice12 = pInitParams->pd3dDevice12;
    HRESULT hr = S_OK;

    m_pGpuFence = pInitParams->pGpuFence;
    m_pCpuFence = pInitParams->pCpuFence;
    m_pd3dCmdList = nullptr;
    m_pd3dCmdQueue = nullptr;
    m_RenderTargetFormat = DXGI_FORMAT_UNKNOWN;
    m_MaxQuadCount = MaxQuadCount;
    m_QuadCount = 0;
    m_StartQuadIndex = 0;
    m_pLockedVBData = NULL;
    m_pLockedIBData = NULL;
    m_vQuadOffset = XMVectorZero();
    ZeroMemory(&m_AlternateUVector, sizeof(m_AlternateUVector));

    m_CBSRVHeap.Initialize(pd3dDevice12, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, true);
    ((ID3D12DescriptorHeap*)m_CBSRVHeap)->SetName(L"UISprite12 CB-SRV");

    m_FontSRVCpu = m_CBSRVHeap.hCPU(0);
    m_FontSRVGpu = m_CBSRVHeap.hGPU(0);

    m_SamplerHeap.Initialize(pd3dDevice12, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2, true);
    ((ID3D12DescriptorHeap*)m_SamplerHeap)->SetName(L"UISprite12 Samplers");

    m_PSText.pShaderBytecode = UISprite11text_pfxhpp;
    m_PSText.BytecodeLength = sizeof(UISprite11text_pfxhpp);

    m_PSModulate.pShaderBytecode = UISprite11modulate_pfxhpp;
    m_PSModulate.BytecodeLength = sizeof(UISprite11modulate_pfxhpp);

    m_PSModulateCube.pShaderBytecode = UISprite11modulatecube_pfxhpp;
    m_PSModulateCube.BytecodeLength = sizeof(UISprite11modulatecube_pfxhpp);

    m_PSModulateArray.pShaderBytecode = UISprite11modulatearray_pfxhpp;
    m_PSModulateArray.BytecodeLength = sizeof(UISprite11modulatearray_pfxhpp);

    m_PSModulateVolume.pShaderBytecode = UISprite11modulatevolume_pfxhpp;
    m_PSModulateVolume.BytecodeLength = sizeof(UISprite11modulatevolume_pfxhpp);

    m_PSModulate1D.pShaderBytecode = UISprite11modulate1D_pfxhpp;
    m_PSModulate1D.BytecodeLength = sizeof(UISprite11modulate1D_pfxhpp);

    m_PSModulateInt.pShaderBytecode = UISprite11modulateint_pfxhpp;
    m_PSModulateInt.BytecodeLength = sizeof(UISprite11modulateint_pfxhpp);

    // Root table layout
    {
        CD3DX12_DESCRIPTOR_RANGE DescRange[3];
        DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0); // t0
        DescRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); // s0
        DescRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); // u0

        CD3DX12_ROOT_PARAMETER RTSlot[4];
        RTSlot[0].InitAsDescriptorTable(1, &DescRange[0], D3D12_SHADER_VISIBILITY_PIXEL); // t0
        RTSlot[1].InitAsDescriptorTable(1, &DescRange[1], D3D12_SHADER_VISIBILITY_PIXEL); // s0
        RTSlot[2].InitAsConstantBufferView(0); // b0
        RTSlot[3].InitAsDescriptorTable(1, &DescRange[2], D3D12_SHADER_VISIBILITY_PIXEL); // u0

        CD3DX12_ROOT_SIGNATURE_DESC RTLayout(ARRAYSIZE(RTSlot), RTSlot);
        RTLayout.Flags =  D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        ID3DBlob* pSerializedLayout = nullptr;
        hr = D3D12SerializeRootSignature(&RTLayout, D3D_ROOT_SIGNATURE_VERSION_1, &pSerializedLayout, NULL);

        hr = pd3dDevice12->CreateRootSignature(
            D3D12XBOX_NODE_MASK,
            pSerializedLayout->GetBufferPointer(),
            pSerializedLayout->GetBufferSize(),
            __uuidof(ID3D12RootSignature),
            (void**)&m_pRootSignature);

        SAFE_RELEASE(pSerializedLayout);

        if (FAILED(hr))
        {
            return hr;
        }
    }

    const D3D12_INPUT_ELEMENT_DESC Layout[] =
    {
        { "POSITION",   0,  DXGI_FORMAT_R32G32_FLOAT,           0,  0,      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,    0 },
        { "COLOR",      0,  DXGI_FORMAT_R8G8B8A8_UNORM,         0,  8,      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,    0 },
        { "TEXCOORD",   0,  DXGI_FORMAT_R32G32_FLOAT,           0,  12,     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,    0 },
    };

    D3D12_BLEND_DESC BlendDesc;
    ZeroMemory( &BlendDesc, sizeof(BlendDesc) );
    BlendDesc.RenderTarget[0].BlendEnable = TRUE;
    BlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    BlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    BlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
    BlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    BlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    m_AlphaBlendDesc = BlendDesc;

    BlendDesc.RenderTarget[0].BlendEnable = FALSE;
    m_SolidBlendDesc = BlendDesc;

    D3D12_SAMPLER_DESC SamplerDesc;
    SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    SamplerDesc.MipLODBias = 0;
    SamplerDesc.MaxAnisotropy = 16;
    SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    SamplerDesc.BorderColor[0] = 0.0f;
    SamplerDesc.BorderColor[1] = 0.0f;
    SamplerDesc.BorderColor[2] = 0.0f;
    SamplerDesc.BorderColor[3] = 0.0f;
    SamplerDesc.MinLOD = 0.0f;
    SamplerDesc.MaxLOD = 9999.0f;
    pd3dDevice12->CreateSampler(&SamplerDesc, m_SamplerHeap.hCPU(0));

    SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    pd3dDevice12->CreateSampler(&SamplerDesc, m_SamplerHeap.hCPU(1));

    D3D12_DEPTH_STENCIL_DESC DSDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    DSDesc.DepthEnable = FALSE;
    DSDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    DSDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    DSDesc.StencilEnable = FALSE;
    DSDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    DSDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

    D3D12_RASTERIZER_DESC RasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    RasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    RasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    RasterizerDesc.FrontCounterClockwise = FALSE;
    RasterizerDesc.DepthClipEnable = TRUE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
    PSODesc.VS.pShaderBytecode = UISprite11_vfxhpp;
    PSODesc.VS.BytecodeLength = sizeof(UISprite11_vfxhpp);
    PSODesc.RasterizerState = RasterizerDesc;
    PSODesc.DepthStencilState = DSDesc;
    PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    PSODesc.InputLayout.pInputElementDescs = (const D3D12_INPUT_ELEMENT_DESC*)m_PSOCache.DuplicateMemory( Layout, sizeof(Layout) );
    PSODesc.InputLayout.NumElements = ARRAYSIZE(Layout);
    PSODesc.SampleDesc.Count = 1;
    PSODesc.SampleMask = -1;
    PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    PSODesc.pRootSignature = m_pRootSignature;
    m_PSOCache.Initialize(pd3dDevice12, PSODesc);

    // On GFX9 in EmptyGS the m_VB and m_IB hang the GPU because they get wedged with fence waits.
    // As we have known holes in the GFX9 memory system setup, I'm increasing the number of buffers
    // such that waits are never inside the same frame IN THE SAMPLES. This help to fix the gpu wedge.
    static const UINT numFrames = 5;

    UINT32 ByteWidth = m_MaxQuadCount * 4 * sizeof(UIVERTEX);
    m_VB.Create(pd3dDevice12, m_pGpuFence, m_pCpuFence, ByteWidth, sizeof(UIVERTEX), numFrames);

    ByteWidth = m_MaxQuadCount * 6 * sizeof(WORD);
    m_IB.Create(pd3dDevice12, m_pGpuFence, m_pCpuFence, ByteWidth, 0, numFrames);

    ByteWidth = sizeof(CONSTANTBUFFER);
    m_ConstantBuffer.Create(pd3dDevice12, m_pGpuFence, m_pCpuFence, ByteWidth, 0, numFrames);

    hr = LoadRawDDS( pInitParams, ui2_dds, DXGI_FORMAT_R8_UNORM, 256, 256, 256, m_FontSRVCpu, &m_pFontTexture );
    if( FAILED(hr) )
    {
        return hr;
    }

    return S_OK;
}

VOID UISprite12::Terminate()
{
    SAFE_RELEASE(m_pRootSignature);
    SAFE_RELEASE(m_pd3dCmdQueue);
    SAFE_RELEASE(m_pFontTexture);
    m_VB.Terminate();
    m_IB.Terminate();
    m_ConstantBuffer.Terminate();
    m_CBSRVHeap.Terminate();
    m_SamplerHeap.Terminate();
    m_PSOCache.Terminate();
}

VOID UISprite12::SetDescriptorHeap(ID3D12DescriptorHeap* pSRVHeap)
{
    assert(m_pd3dCmdList != nullptr);
    ID3D12DescriptorHeap* pHeaps[] = { pSRVHeap, m_SamplerHeap };
    m_pd3dCmdList->SetDescriptorHeaps(ARRAYSIZE(pHeaps), pHeaps);
}

HRESULT UISprite12::BeginScene(ID3D12GraphicsCommandList* pd3dCmdList, ID3D12CommandQueue* pd3dCmdQueue, DXGI_FORMAT RenderTargetFormat, D3D12_CPU_DESCRIPTOR_HANDLE RTHandle, const D3D12_VIEWPORT& Viewport)
{
    if( m_pd3dCmdList != nullptr || pd3dCmdList == nullptr || m_pd3dCmdQueue != nullptr || pd3dCmdQueue == nullptr )
    {
        return E_FAIL;
    }

    m_pd3dCmdList = pd3dCmdList;
    m_pd3dCmdList->AddRef();
    m_pd3dCmdQueue = pd3dCmdQueue;
    m_pd3dCmdQueue->AddRef();
    m_RenderTargetFormat = RenderTargetFormat;
    m_Viewport = Viewport;

    D3D12_RECT ScissorRect = { 0, 0, (LONG)Viewport.Width, (LONG)Viewport.Height };
    m_pd3dCmdList->RSSetScissorRects(1, &ScissorRect);
    m_pd3dCmdList->RSSetViewports(1, &Viewport);

    // Set descriptor heap and root signature
    SetDescriptorHeap(m_CBSRVHeap);
    m_pd3dCmdList->SetGraphicsRootSignature(m_pRootSignature);

    m_pd3dCmdList->OMSetRenderTargets(1, &RTHandle, TRUE, nullptr);

    // Set up shader constants
    D3D12_MAPPED_SUBRESOURCE MappedCB;
    HRESULT hr = m_ConstantBuffer.MapDiscard(m_pd3dCmdQueue, &MappedCB);
    ASSERT( SUCCEEDED(hr) );
    CONSTANTBUFFER* pCB = (CONSTANTBUFFER*)MappedCB.pData;
    XMMATRIX Scaling = XMMatrixScaling( 2.0f / (FLOAT)Viewport.Width, -2.0f / (FLOAT)Viewport.Height, 0 );
    Scaling *= XMMatrixTranslation( -1.0f, 1.0f, 0 );
    XMStoreFloat4x4( &pCB->m_WorldViewProjection, Scaling );
    XMStoreFloat4x4(&m_CurrentWVPMatrix, Scaling);
    m_ConstantBuffer.Unmap(m_pd3dCmdQueue);

    m_SceneQuadCount = 0;
    StartQuads(m_PSText, true);

    m_ConstantBuffer.SetGraphicsRootCB(pd3dCmdList, 2);

    SetFontTexture();

    return S_OK;
}

VOID UISprite12::EndScene()
{
    FlushQuads( TRUE );

    SAFE_RELEASE(m_pd3dCmdList);
    SAFE_RELEASE(m_pd3dCmdQueue);
}

void UISprite12::SetFontTexture()
{
    SetDescriptorHeap(m_CBSRVHeap);
    m_pd3dCmdList->SetGraphicsRootDescriptorTable(0, m_FontSRVGpu);
    m_pd3dCmdList->SetGraphicsRootDescriptorTable(1, m_SamplerHeap.hGPU(0));
}

VOID UISprite12::DrawGlyph( INT X, INT Y, UINT Width, UINT Height, const FLOAT ColorRGBA[4], UINT GlyphIndex )
{
    const XMVECTOR GlyphRectSizeOffsets[] =
    {
        { 100.0f / 256.0f, 100.0f / 256.0f, 0, 0 }, { 0.0f / 256.0f, 156.0f / 256.0f, 0, 0 },
    };

    if( GlyphIndex > ( ARRAYSIZE(GlyphRectSizeOffsets) / 2 ) )
    {
        GlyphIndex = 0;
    }

    const UINT SizeIndex = GlyphIndex * 2;
    const UINT OffsetIndex = SizeIndex + 1;

    AddQuad( XMVectorSet( (FLOAT)X, (FLOAT)Y, 0, 0 ), XMVectorSet( (FLOAT)Width, (FLOAT)Height, 0, 0 ), GlyphRectSizeOffsets[OffsetIndex], GlyphRectSizeOffsets[SizeIndex], ColorRGBA );
}

VOID UISprite12::DrawEllipse( INT X, INT Y, UINT Width, UINT Height, const FLOAT ColorRGBA[4] )
{
    DrawGlyph( X, Y, Width, Height, ColorRGBA, 0 );
}

VOID UISprite12::DrawTexturedQuad( INT X, INT Y, UINT Width, UINT Height, D3D12_GPU_DESCRIPTOR_HANDLE SRView, D3D12_SRV_DIMENSION SRVDimension, const FLOAT ModulateColorRGBA[4], bool AlphaBlend, FLOAT CubeIndex, const FLOAT UVRect[4], bool PointSample, FLOAT SelectedLOD, const D3D12_SHADER_BYTECODE* pAlternatePixelShader )
{
    FlushQuads(FALSE);

    bool SetUAV = false;
    
    if (pAlternatePixelShader != nullptr)
    {
        StartQuads(*pAlternatePixelShader, AlphaBlend);
        D3D12_MAPPED_SUBRESOURCE MappedData;
        m_ConstantBuffer.MapDiscard(m_pd3dCmdQueue, &MappedData);
        CONSTANTBUFFER* pCB = (CONSTANTBUFFER*)MappedData.pData;
        pCB->m_WorldViewProjection = m_CurrentWVPMatrix;
        pCB->m_UVector = m_AlternateUVector;
        pCB->m_LODSelection.x = SelectedLOD;
        m_ConstantBuffer.Unmap(m_pd3dCmdQueue);
        m_ConstantBuffer.SetGraphicsRootCB(m_pd3dCmdList, 2);

        if (SRVDimension == D3D12_SRV_DIMENSION_BUFFER)
        {
            SetUAV = true;
        }
    }
    else if( SRVDimension == D3D12_SRV_DIMENSION_TEXTURECUBE )
    {
        StartQuads( m_PSModulateCube, AlphaBlend );
        D3D12_MAPPED_SUBRESOURCE MappedData;
        m_ConstantBuffer.MapDiscard(m_pd3dCmdQueue, &MappedData);
        CONSTANTBUFFER* pCB = (CONSTANTBUFFER*)MappedData.pData;
        pCB->m_WorldViewProjection = m_CurrentWVPMatrix;
        static const XMVECTOR CubeVectors[] = 
        {
            // +x face
            { 0, 0, -1, 0 },
            { 0, 1, 0, 0 },
            { 1, 0, 0, 0 },
            // -x face
            { 0, 0, 1, 0 },
            { 0, 1, 0, 0 },
            { -1, 0, 0, 0 },
            // +y face
            { 1, 0, 0, 0 },
            { 0, 0, -1, 0 },
            { 0, 1, 0, 0 },
            // -y face
            { 1, 0, 0, 0 },
            { 0, 0, 1, 0 },
            { 0, -1, 0, 0 },
            // +z face
            { 1, 0, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 0, 1, 0 },
            // -z face
            { -1, 0, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 0, -1, 0 },
        };
        assert( CubeIndex < ( ARRAYSIZE(CubeVectors) / 3 ) );
        memcpy(&pCB->m_UVector, &CubeVectors[(UINT)CubeIndex * 3], 3 * sizeof(XMVECTOR));
        pCB->m_LODSelection.x = SelectedLOD;
        m_ConstantBuffer.Unmap(m_pd3dCmdQueue);
        m_ConstantBuffer.SetGraphicsRootCB(m_pd3dCmdList, 2);
    }
    else if( SRVDimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY )
    {
        StartQuads(m_PSModulateArray, AlphaBlend);
        D3D12_MAPPED_SUBRESOURCE MappedData;
        m_ConstantBuffer.MapDiscard(m_pd3dCmdQueue, &MappedData);
        CONSTANTBUFFER* pCB = (CONSTANTBUFFER*)MappedData.pData;
        pCB->m_WorldViewProjection = m_CurrentWVPMatrix;
        pCB->m_UVector.x = CubeIndex;
        pCB->m_LODSelection.x = SelectedLOD;
        m_ConstantBuffer.Unmap(m_pd3dCmdQueue);
        m_ConstantBuffer.SetGraphicsRootCB(m_pd3dCmdList, 2);
    }
    else if( SRVDimension == D3D12_SRV_DIMENSION_TEXTURE1DARRAY || SRVDimension == D3D12_SRV_DIMENSION_TEXTURE1D )
    {
        StartQuads( m_PSModulate1D, AlphaBlend );
        D3D12_MAPPED_SUBRESOURCE MappedData;
        m_ConstantBuffer.MapDiscard(m_pd3dCmdQueue, &MappedData);
        CONSTANTBUFFER* pCB = (CONSTANTBUFFER*)MappedData.pData;
        pCB->m_WorldViewProjection = m_CurrentWVPMatrix;
        pCB->m_UVector.x = CubeIndex;
        pCB->m_LODSelection.x = SelectedLOD;
        m_ConstantBuffer.Unmap(m_pd3dCmdQueue);
        m_ConstantBuffer.SetGraphicsRootCB(m_pd3dCmdList, 2);
    }
    else if( SRVDimension == D3D12_SRV_DIMENSION_TEXTURE3D )
    {
        StartQuads( m_PSModulateVolume, AlphaBlend );
        D3D12_MAPPED_SUBRESOURCE MappedData;
        m_ConstantBuffer.MapDiscard(m_pd3dCmdQueue, &MappedData);
        CONSTANTBUFFER* pCB = (CONSTANTBUFFER*)MappedData.pData;
        pCB->m_WorldViewProjection = m_CurrentWVPMatrix;
        pCB->m_UVector.x = CubeIndex;
        pCB->m_LODSelection.x = SelectedLOD;
        m_ConstantBuffer.Unmap(m_pd3dCmdQueue);
        m_ConstantBuffer.SetGraphicsRootCB(m_pd3dCmdList, 2);
    }
    else
    {
        StartQuads( m_PSModulate, AlphaBlend );
        D3D12_MAPPED_SUBRESOURCE MappedData;
        m_ConstantBuffer.MapDiscard(m_pd3dCmdQueue, &MappedData);
        CONSTANTBUFFER* pCB = (CONSTANTBUFFER*)MappedData.pData;
        pCB->m_WorldViewProjection = m_CurrentWVPMatrix;
        pCB->m_LODSelection.x = SelectedLOD;
        m_ConstantBuffer.Unmap(m_pd3dCmdQueue);
        m_ConstantBuffer.SetGraphicsRootCB(m_pd3dCmdList, 2);
    }

    if (SetUAV)
    {
        m_pd3dCmdList->SetGraphicsRootDescriptorTable(3, SRView);
    }
    else
    {
        m_pd3dCmdList->SetGraphicsRootDescriptorTable(0, SRView);
    }

    if (PointSample)
    {
        m_pd3dCmdList->SetGraphicsRootDescriptorTable(1, m_SamplerHeap.hGPU(1));
    }

    XMVECTOR uvwh = XMLoadFloat4( (const XMFLOAT4*)UVRect );
    XMVECTOR uvStart = XMVectorSelect( XMVectorZero(), uvwh, g_XMSelect1100 );
    XMVECTOR uvSize = XMVectorSelect( XMVectorZero(), XMVectorSwizzle( uvwh, 2, 3, 0, 1 ), g_XMSelect1100 );
    AddQuad( XMVectorSet( (FLOAT)X, (FLOAT)Y, 0, 0 ), XMVectorSet( (FLOAT)Width, (FLOAT)Height, 0, 0 ), uvStart, uvSize, ModulateColorRGBA );
    FlushQuads(FALSE);

    StartQuads(m_PSText, true);

    SetFontTexture();
}

VOID UISprite12::DrawIntTexturedQuad( INT X, INT Y, UINT Width, UINT Height, D3D12_GPU_DESCRIPTOR_HANDLE SRView, UINT TexWidth, UINT TexHeight, const FLOAT ModulateColorRGBA[4], FLOAT IntSize, const FLOAT UVRect[4] )
{
    FlushQuads(FALSE);

    StartQuads(m_PSModulateInt, FALSE);

    m_pd3dCmdList->SetGraphicsRootDescriptorTable(0, SRView);

    XMVECTOR uvwh = XMLoadFloat4( (const XMFLOAT4*)UVRect );
    XMVECTOR uvStart = XMVectorSelect( XMVectorZero(), uvwh, g_XMSelect1100 );
    XMVECTOR uvSize = XMVectorSelect( XMVectorZero(), XMVectorSwizzle( uvwh, 2, 3, 0, 1 ), g_XMSelect1100 );
    uvSize *= XMVectorSet((FLOAT)TexWidth, (FLOAT)TexHeight, 1, 1);
    FLOAT ModColor[4];
    ModColor[0] = ModulateColorRGBA[0] / IntSize;
    ModColor[1] = ModulateColorRGBA[1] / IntSize;
    ModColor[2] = ModulateColorRGBA[2] / IntSize;
    ModColor[3] = ModulateColorRGBA[3] / IntSize;
    AddQuad( XMVectorSet( (FLOAT)X, (FLOAT)Y, 0, 0 ), XMVectorSet( (FLOAT)Width, (FLOAT)Height, 0, 0 ), uvStart, uvSize, ModColor );
    FlushQuads(FALSE);

    StartQuads(m_PSText, true);

    SetFontTexture();
}

VOID UISprite12::DrawRect( INT X, INT Y, UINT Width, UINT Height, const FLOAT ColorRGBA[4] )
{
    AddQuad( X, Y, Width, Height, XMVectorSet( 50.0f / 256.0f, 206.0f / 256.0f, 0, 0 ), XMVectorZero(), ColorRGBA );
}

VOID UISprite12::DrawRectOutline( INT X, INT Y, UINT Width, UINT Height, UINT LineWidth, const FLOAT ColorRGBA[4] )
{
    UINT XLineWidth = std::min( LineWidth, Width / 2 );
    UINT YLineWidth = std::min( LineWidth, Height / 2 );

    const XMVECTOR vUV = XMVectorSet( 50.0f / 256.0f, 206.0f / 256.0f, 0, 0 );
    AddQuad( X, Y, Width, YLineWidth, vUV, XMVectorZero(), ColorRGBA );
    AddQuad( X, Y + Height - YLineWidth, Width, YLineWidth, vUV, XMVectorZero(), ColorRGBA );
    AddQuad( X, Y, XLineWidth, Height, vUV, XMVectorZero(), ColorRGBA );
    AddQuad( X + Width - XLineWidth, Y, XLineWidth, Height, vUV, XMVectorZero(), ColorRGBA );
}

VOID UISprite12::DrawRoundRect( INT X, INT Y, UINT Width, UINT Height, UINT EdgeThickness, const FLOAT ColorRGBA[4], UINT Style )
{
    XMVECTOR RectSize;
    XMVECTOR RectOffset;

    switch( Style )
    {
    case 1:
        RectSize = XMVectorSet( 50.0f / 256.0f, 50.0f / 256.0f, 0, 0 );
        RectOffset = XMVectorSet( 100.0f / 256.0f, 156.0f / 256.0f, 0, 0 );
        break;
    case 0:
    default:
        RectSize = XMVectorSet( 100.0f / 256.0f, 100.0f / 256.0f, 0, 0 );
        RectOffset = XMVectorSet( 0.0f / 256.0f, 156.0f / 256.0f, 0, 0 );
        break;
    }

    const XMVECTOR Half = { 0.5f, 0.5f, 0, 0 };
    const XMVECTOR VHalf = { 0.0f, 0.5f, 0, 0 };
    const XMVECTOR HHalf = { 0.5f, 0.0f, 0, 0 };

    AddQuad( X, Y, EdgeThickness, EdgeThickness, RectOffset, RectSize * Half, ColorRGBA );
    AddQuad( X + EdgeThickness, Y, Width - EdgeThickness * 2, EdgeThickness, RectOffset + RectSize * HHalf, RectSize * VHalf, ColorRGBA );
    AddQuad( X + Width - EdgeThickness, Y, EdgeThickness, EdgeThickness, RectOffset + RectSize * HHalf, RectSize * Half, ColorRGBA );

    AddQuad( X, Y + EdgeThickness, EdgeThickness, Height - EdgeThickness * 2, RectOffset + RectSize * VHalf, RectSize * HHalf, ColorRGBA );
    AddQuad( X + EdgeThickness, Y + EdgeThickness, Width - EdgeThickness * 2, Height - EdgeThickness * 2, RectOffset + RectSize * Half, XMVectorZero(), ColorRGBA );
    AddQuad( X + Width - EdgeThickness, Y + EdgeThickness, EdgeThickness, Height - EdgeThickness * 2, RectOffset + RectSize * Half, RectSize * HHalf, ColorRGBA );

    AddQuad( X, Y + Height - EdgeThickness, EdgeThickness, EdgeThickness, RectOffset + RectSize * VHalf, RectSize * Half, ColorRGBA );
    AddQuad( X + EdgeThickness, Y + Height - EdgeThickness, Width - EdgeThickness * 2, EdgeThickness, RectOffset + RectSize * Half, RectSize * VHalf, ColorRGBA );
    AddQuad( X + Width - EdgeThickness, Y + Height - EdgeThickness, EdgeThickness, EdgeThickness, RectOffset + RectSize * Half, RectSize * Half, ColorRGBA );
}

VOID UISprite12::DrawText( INT X, INT Y, FLOAT Size, const WCHAR* strText, const FLOAT ColorRGBA[4] )
{
    static const WCHAR* strCharTable = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()-=[]\\;',./_+{}|:\"<>?`~";
    const INT TableColumns = 16;
    const INT TableRows = 6;
    const FLOAT TableVSize = 156.0f / 256.0f;
    const FLOAT TableVOffset = 0.5f / 256.0f;
    const XMVECTOR vTexCoordSize = { 1.0f / (FLOAT)TableColumns, TableVSize / (FLOAT)TableRows, 0, 0 };

    const FLOAT fCharAspect = GetTextAspectRatio();
    const XMVECTOR vCharSize = XMVectorSet( Size * fCharAspect, Size, 0, 0 );
    const XMVECTOR vCharSpacing = vCharSize * XMVectorSet( 1, 0, 0, 0 );
    XMVECTOR vCursor = XMVectorSet( (FLOAT)X, (FLOAT)Y, 0, 0 );

    const WCHAR* strCursor = strText;
    while( *strCursor != L'\0' )
    {
        WCHAR CurrentChar = *strCursor;
        switch( CurrentChar )
        {
        case L' ':
            vCursor += vCharSpacing;
            break;
        case L'\n':
            vCursor += vCharSize;
            vCursor = XMVectorSetX( vCursor, (FLOAT)X );
            break;
        default:
            {
                const WCHAR* strFound = wcschr( strCharTable, CurrentChar );
                if( strFound != NULL )
                {
                    size_t Index = strFound - strCharTable;
                    FLOAT fRow = (FLOAT)( Index / TableColumns ) / (FLOAT)TableRows;
                    FLOAT fColumn = (FLOAT)( Index % TableColumns ) / (FLOAT)TableColumns;
                    XMVECTOR vTexCoord = XMVectorSet( fColumn, fRow * TableVSize + TableVOffset, 0, 0 );
                    AddQuad( vCursor, vCharSize, vTexCoord, vTexCoordSize, ColorRGBA );
                }
                vCursor += vCharSpacing;
                break;
            }
        }
        ++strCursor;
    }
}

void UISprite12::DrawCenteredText( INT CenterX, INT Y, FLOAT Size, const WCHAR* strText, const FLOAT ColorRGBA[4] )
{
    INT StringWidth = (INT)(GetTextAspectRatio() * Size * wcslen(strText));
    INT Offset = StringWidth / 2;
    DrawText(CenterX - Offset, Y, Size, strText, ColorRGBA);
}

void UISprite12::DrawText3D(const XMFLOAT3& Position, const XMMATRIX& matViewProjection, const D3D12_VIEWPORT& Viewport, FLOAT Size, const WCHAR* strText, const FLOAT ColorRGBA[4])
{
    const XMVECTOR Offset = GetOffset();
    SetOffset(g_XMZero);

    const XMVECTOR Pos = XMLoadFloat3(&Position);
    XMVECTOR ViewPos = XMVector3TransformCoord(Pos, matViewProjection);
    ViewPos *= XMVectorReciprocal(XMVectorSplatW(ViewPos));
    const FLOAT ZPos = XMVectorGetZ(ViewPos);
    if (ZPos <= 0 || ZPos > 1)
    {
        return;
    }
    ViewPos *= XMVectorSet(Viewport.Width * 0.5f, Viewport.Height * -0.5f, 1, 1);
    ViewPos += XMVectorSet(Viewport.Width * 0.5f, Viewport.Height * 0.5f, 0, 0);

    INT X = (INT)XMVectorGetX(ViewPos);
    INT Y = (INT)XMVectorGetY(ViewPos);

    DrawText(X, Y, Size, strText, ColorRGBA);

    SetOffset(Offset);
}

VOID UISprite12::AddQuad( INT X, INT Y, UINT Width, UINT Height, CXMVECTOR vTexCoord, CXMVECTOR vTexCoordSize, const FLOAT ColorRGBA[4] )
{
    AddQuad( XMVectorSet( (FLOAT)X, (FLOAT)Y, 0, 0 ), XMVectorSet( (FLOAT)Width, (FLOAT)Height, 0, 0 ), vTexCoord, vTexCoordSize, ColorRGBA );
}

VOID UISprite12::AddQuad( CXMVECTOR vUpperLeft, CXMVECTOR vSize, FXMVECTOR vTexCoord, FXMVECTOR vTexCoordSize, const FLOAT ColorRGBA[4] )
{
    if( ( m_QuadCount + m_StartQuadIndex ) >= m_MaxQuadCount )
    {
        Flush();
    }
    ASSERT( m_pLockedVBData != NULL && m_pLockedIBData != NULL );

    const XMVECTOR Vec10 = { 1.0f, 0, 0, 0 };
    const XMVECTOR Vec01 = { 0, 1.0f, 0, 0 };

    const INT BaseVertex = ( m_QuadCount + m_StartQuadIndex ) * 4;
    UIVERTEX* pCurrentQuad = m_pLockedVBData + BaseVertex;

    XMVECTOR vColor = XMLoadFloat4( (const XMFLOAT4*)ColorRGBA );
    //vColor = XMVectorSwizzle( vColor, 3, 2, 1, 0 );

    const XMVECTOR vOffsetUpperLeft = m_vQuadOffset + vUpperLeft;

    XMStoreFloat2( &pCurrentQuad[0].Position, vOffsetUpperLeft );
    XMStoreUByteN4( &pCurrentQuad[0].Color, vColor );
    XMStoreFloat2( &pCurrentQuad[0].TexCoord, vTexCoord );

    XMStoreFloat2( &pCurrentQuad[1].Position, vOffsetUpperLeft + vSize * Vec10 );
    XMStoreUByteN4( &pCurrentQuad[1].Color, vColor );
    XMStoreFloat2( &pCurrentQuad[1].TexCoord, vTexCoord + vTexCoordSize * Vec10 );

    XMStoreFloat2( &pCurrentQuad[2].Position, vOffsetUpperLeft + vSize * Vec01 );
    XMStoreUByteN4( &pCurrentQuad[2].Color, vColor );
    XMStoreFloat2( &pCurrentQuad[2].TexCoord, vTexCoord + vTexCoordSize * Vec01 );

    XMStoreFloat2( &pCurrentQuad[3].Position, vOffsetUpperLeft + vSize );
    XMStoreUByteN4( &pCurrentQuad[3].Color, vColor );
    XMStoreFloat2( &pCurrentQuad[3].TexCoord, vTexCoord + vTexCoordSize );

    const INT BaseIndex = ( m_QuadCount + m_StartQuadIndex ) * 6;
    WORD* pCurrentQuadIndices = m_pLockedIBData + BaseIndex;
    pCurrentQuadIndices[0] = 0 + (WORD)BaseVertex;
    pCurrentQuadIndices[1] = 1 + (WORD)BaseVertex;
    pCurrentQuadIndices[2] = 2 + (WORD)BaseVertex;
    pCurrentQuadIndices[3] = 2 + (WORD)BaseVertex;
    pCurrentQuadIndices[4] = 1 + (WORD)BaseVertex;
    pCurrentQuadIndices[5] = 3 + (WORD)BaseVertex;

    ++m_QuadCount;
}

VOID UISprite12::StartQuads(const D3D12_SHADER_BYTECODE& PSByteCode, bool AlphaBlend)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = m_PSOCache.GetPSOTemplate();
    PSODesc.PS = PSByteCode;
    PSODesc.BlendState = AlphaBlend ? m_AlphaBlendDesc : m_SolidBlendDesc;
    PSODesc.NumRenderTargets = 1;
    PSODesc.RTVFormats[0] = m_RenderTargetFormat;
    ID3D12PipelineState* pPSO = m_PSOCache.FindOrCreatePSO(PSODesc);
    ASSERT(pPSO != nullptr);
    m_pd3dCmdList->SetPipelineState(pPSO);
    m_pd3dCmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ASSERT( m_pLockedVBData == NULL && m_pLockedIBData == NULL );
    ASSERT( m_QuadCount == 0 );

    if( m_StartQuadIndex == 0 )
    {
        D3D12_MAPPED_SUBRESOURCE MappedVB;
        HRESULT hr = m_VB.MapDiscard(m_pd3dCmdQueue, &MappedVB);
        ASSERT(SUCCEEDED(hr));
        m_pLockedVBData = (UIVERTEX*)MappedVB.pData;

        D3D12_MAPPED_SUBRESOURCE MappedIB;
        hr = m_IB.MapDiscard(m_pd3dCmdQueue, &MappedIB);
        ASSERT(SUCCEEDED(hr));
        m_pLockedIBData = (WORD*)MappedIB.pData;
    }
    else
    {
        D3D12_MAPPED_SUBRESOURCE MappedVB;
        HRESULT hr = m_VB.MapNoOverwrite(m_pd3dCmdQueue, &MappedVB);
        ASSERT(SUCCEEDED(hr));
        m_pLockedVBData = (UIVERTEX*)MappedVB.pData;

        D3D12_MAPPED_SUBRESOURCE MappedIB;
        hr = m_IB.MapNoOverwrite(m_pd3dCmdQueue, &MappedIB);
        ASSERT(SUCCEEDED(hr));
        m_pLockedIBData = (WORD*)MappedIB.pData;
    }
}

VOID UISprite12::FlushQuads( BOOL FlushQuadCount )
{
    if( m_pLockedVBData != NULL )
    {
        m_VB.Unmap(m_pd3dCmdQueue);
        m_pLockedVBData = NULL;
    }

    if( m_pLockedIBData != NULL )
    {
        m_IB.Unmap(m_pd3dCmdQueue);
        m_pLockedIBData = NULL;
    }

    if( m_QuadCount == 0 )
    {
        return;
    }

    m_IB.SetIB(m_pd3dCmdList, DXGI_FORMAT_R16_UINT);
    m_VB.SetVB(m_pd3dCmdList, 0);

    m_pd3dCmdList->DrawIndexedInstanced(m_QuadCount * 6, 1, m_StartQuadIndex * 6, 0, 0);

    m_SceneQuadCount += m_QuadCount;

    if( FlushQuadCount )
    {
        m_StartQuadIndex = 0;
        m_QuadCount = 0;
    }
    else
    {
        m_StartQuadIndex += m_QuadCount;
        if( m_StartQuadIndex >= m_MaxQuadCount )
        {
            m_StartQuadIndex = 0;
        }
        m_QuadCount = 0;
    }
}
