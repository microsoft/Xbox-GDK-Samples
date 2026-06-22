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
#pragma once

#include <windows.h>
#if defined(_XBOX_ONE) && defined(_TITLE)
#define D3D12XBOX_NO_COMPAT_TYPEDEFS 1
#include <d3d12_xs.h>
#else
#include <d3d12.h>
#endif
#include <directxmath.h>
#include <directxpackedvector.h>

#include "D3D12Util.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

#ifdef DrawText
#undef DrawText
#endif

struct UIVERTEX
{
    XMFLOAT2    Position;
    XMUBYTEN4   Color;
    XMFLOAT2    TexCoord;
};

struct SpriteInitParams12
{
    ID3D12Device* pd3dDevice12;
    ID3D12CommandQueue* pd3dCmdQueue;
    ID3D12CommandAllocator* pd3dCmdAllocatorDirect;
    ID3D12Fence* pGpuFence;
    UINT64* pCpuFence;
    CpuGpuHeap* pUploadHeap;
};

class UISprite12
{
protected:
    XMVECTOR                    m_vQuadOffset;

    ID3D12GraphicsCommandList*  m_pd3dCmdList;
    ID3D12CommandQueue*         m_pd3dCmdQueue;
    ID3D12Fence*                m_pGpuFence;
    UINT64*                     m_pCpuFence;
    DXGI_FORMAT                 m_RenderTargetFormat;
    D3D12_VIEWPORT              m_Viewport;

    ID3D12Resource*             m_pFontTexture;
    D3D12_CPU_DESCRIPTOR_HANDLE m_FontSRVCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE m_FontSRVGpu;

    DescriptorHeapWrapper       m_CBSRVHeap;
    DescriptorHeapWrapper       m_SamplerHeap;
    D3D12_BLEND_DESC            m_AlphaBlendDesc;
    D3D12_BLEND_DESC            m_SolidBlendDesc;

    D3D12_SHADER_BYTECODE       m_PSText;
    D3D12_SHADER_BYTECODE       m_PSModulate;
    D3D12_SHADER_BYTECODE       m_PSModulateCube;
    D3D12_SHADER_BYTECODE       m_PSModulateArray;
    D3D12_SHADER_BYTECODE       m_PSModulateVolume;
    D3D12_SHADER_BYTECODE       m_PSModulate1D;
    D3D12_SHADER_BYTECODE       m_PSModulateInt;

    ID3D12RootSignature*        m_pRootSignature;

    PSOCache                    m_PSOCache;

    D3D12DynamicBuffer          m_VB;
    D3D12DynamicBuffer          m_IB;
    D3D12DynamicBuffer          m_ConstantBuffer;

    UIVERTEX*                   m_pLockedVBData;
    WORD*                       m_pLockedIBData;
    UINT                        m_QuadCount;
    UINT                        m_StartQuadIndex;
    UINT                        m_MaxQuadCount;
    XMFLOAT4X4                  m_CurrentWVPMatrix;
    UINT32                      m_SceneQuadCount;

    XMFLOAT4                    m_AlternateUVector;

public:
    HRESULT Initialize( const SpriteInitParams12* pInitParams, UINT MaxQuadCount = 200 );
    VOID Terminate();

    XMVECTOR GetOffset() const { return m_vQuadOffset; }
    VOID SetOffset( XMVECTOR vOffset ) { m_vQuadOffset = vOffset; }

    HRESULT BeginScene( ID3D12GraphicsCommandList* pd3dCmdList, ID3D12CommandQueue* pd3dCmdQueue, DXGI_FORMAT RenderTargetFormat, D3D12_CPU_DESCRIPTOR_HANDLE RTHandle, const D3D12_VIEWPORT& Viewport );
    VOID SetDescriptorHeap(ID3D12DescriptorHeap* pSRVHeap);
    VOID DrawRect( INT X, INT Y, UINT Width, UINT Height, const FLOAT ColorRGBA[4] );
    VOID DrawRectOutline( INT X, INT Y, UINT Width, UINT Height, UINT LineWidth, const FLOAT ColorRGBA[4] );
    VOID DrawRoundRect( INT X, INT Y, UINT Width, UINT Height, UINT EdgeThickness, const FLOAT ColorRGBA[4], UINT Style = 0 );
    VOID DrawGlyph( INT X, INT Y, UINT Width, UINT Height, const FLOAT ColorRGBA[4], UINT GlyphIndex );
    VOID DrawEllipse( INT X, INT Y, UINT Width, UINT Height, const FLOAT ColorRGBA[4] );
    VOID DrawText( INT X, INT Y, FLOAT Size, const WCHAR* strText, const FLOAT ColorRGBA[4] );
    VOID DrawText3D(const XMFLOAT3& Position, const XMMATRIX& matViewProjection, const D3D12_VIEWPORT& Viewport, FLOAT Size, const WCHAR* strText, const FLOAT ColorRGBA[4]);
    VOID DrawCenteredText( INT CenterX, INT Y, FLOAT Size, const WCHAR* strText, const FLOAT ColorRGBA[4] );
    VOID DrawTexturedQuad( INT X, INT Y, UINT Width, UINT Height, D3D12_GPU_DESCRIPTOR_HANDLE SRView, D3D12_SRV_DIMENSION SRVDimension, const FLOAT ModulateColorRGBA[4], bool AlphaBlend, FLOAT CubeIndex, const FLOAT UVRect[4], bool PointSample, FLOAT SelectedLOD = -1, const D3D12_SHADER_BYTECODE* pAlternatePixelShader = nullptr );
    VOID DrawTexturedQuad( INT X, INT Y, UINT Width, UINT Height, D3D12_GPU_DESCRIPTOR_HANDLE SRView, D3D12_SRV_DIMENSION SRVDimension, const FLOAT ModulateColorRGBA[4], bool AlphaBlend = true, FLOAT CubeIndex = 0, FLOAT SelectedLOD = -1 )
    {
        const FLOAT UVWhole[4] = { 0, 0, 1, 1 };
        DrawTexturedQuad( X, Y, Width, Height, SRView, SRVDimension, ModulateColorRGBA, AlphaBlend, CubeIndex, UVWhole, FALSE, SelectedLOD, nullptr );
    }
    VOID DrawIntTexturedQuad( INT X, INT Y, UINT Width, UINT Height, D3D12_GPU_DESCRIPTOR_HANDLE SRView, UINT TexWidth, UINT TexHeight, const FLOAT ModulateColorRGBA[4], FLOAT IntSize, const FLOAT UVRect[4] );
    VOID DrawIntTexturedQuad( INT X, INT Y, UINT Width, UINT Height, D3D12_GPU_DESCRIPTOR_HANDLE SRView, UINT TexWidth, UINT TexHeight, const FLOAT ModulateColorRGBA[4], FLOAT IntSize )
    {
        const FLOAT UVWhole[4] = { 0, 0, 1, 1 };
        DrawIntTexturedQuad( X, Y, Width, Height, SRView, TexWidth, TexHeight, ModulateColorRGBA, IntSize, UVWhole );
    }
    VOID EndScene();

    VOID Flush() { FlushQuads( TRUE ); StartQuads( m_PSText, true ); }

    FLOAT GetTextAspectRatio() const { return 0.6f; }

    void SetAlternateUVector(XMFLOAT4 vec) { m_AlternateUVector = vec; }

    ID3D12Resource* GetFontTexture() { return m_pFontTexture; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetFontGpuSRV() { return m_FontSRVGpu; }

protected:
    VOID AddQuad( INT X, INT Y, UINT Width, UINT Height, CXMVECTOR vTexCoord, CXMVECTOR vTexCoordSize, const FLOAT ColorRGBA[4] );
    VOID AddQuad( CXMVECTOR vUpperLeft, CXMVECTOR vSize, FXMVECTOR vTexCoord, FXMVECTOR vTexCoordSize, const FLOAT ColorRGBA[4] );
    VOID StartQuads( const D3D12_SHADER_BYTECODE& PSBytecode, bool AlphaBlend );
    VOID FlushQuads( BOOL FlushQuadCount );
    VOID SetFontTexture();
};
