//--------------------------------------------------------------------------------------
// TextureGen.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "precomp.hpp"
#include "TextureGen.h"
#include "JobQueue.h"

#define SAFE_TO_INCLUDE
#include <DirectXTex.h>

#include "ToolDefines.h"
#include "TiledResourceImage.h"
using namespace DirectX;
using namespace DirectX::PackedVector;

#include "TextureFiles.h"
#include "TextureStreamingUtils.h"

LONG g_RequestedTextureCount = 0;
volatile LONG g_TextureCount = 0;
volatile LONG64 g_TextureSizeBytes = 0;
ID3D12Device* g_pd3dDevice = nullptr;

struct TilingLayout
{
    UINT32 TotalSizeTiles;
    D3D12_TILE_SHAPE TileShape;
    D3D12_PACKED_MIP_INFO PackedMipInfo;
    UINT32 NumSubresources;
    D3D12_SUBRESOURCE_TILING* pSubresourceTilings;
};

bool CreateTilingLayout(DXGI_FORMAT Format, UINT32 Width, UINT32 Height, UINT32 MipCount, UINT32 SliceCount, TilingLayout* pTilingLayout)
{
    D3D12_RESOURCE_DESC TexDesc = CD3DX12_RESOURCE_DESC::Tex2D(Format, Width, Height, (UINT16)SliceCount, (UINT16)MipCount, 1, 0, D3D12_RESOURCE_FLAG_NONE, D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE);

    ID3D12Resource* pResource = nullptr;
    assert(g_pd3dDevice != nullptr);
    HRESULT hr = g_pd3dDevice->CreateReservedResource(&TexDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, __uuidof(*pResource), (void**)&pResource);
    if (FAILED(hr))
    {
        return false;
    }

    pTilingLayout->NumSubresources = MipCount * SliceCount;
    pTilingLayout->pSubresourceTilings = new D3D12_SUBRESOURCE_TILING[pTilingLayout->NumSubresources];

    g_pd3dDevice->GetResourceTiling(
        pResource,
        &pTilingLayout->TotalSizeTiles,
        &pTilingLayout->PackedMipInfo,
        &pTilingLayout->TileShape,
        &pTilingLayout->NumSubresources,
        0,
        pTilingLayout->pSubresourceTilings);

    pResource->Release();

    return true;
}

void FreeTilingLayout(TilingLayout* pTilingLayout)
{
    delete[] pTilingLayout->pSubresourceTilings;
    ZeroMemory(pTilingLayout, sizeof(*pTilingLayout));
}

struct GenerateTextureJobDesc
{
    WCHAR strDiffuseFileName[MAX_PATH];
    WCHAR strNormalFileName[MAX_PATH];
    WCHAR strSpecularFileName[MAX_PATH];
    WCHAR strDataDiffuseFileName[MAX_PATH];
    WCHAR strDataNormalFileName[MAX_PATH];
    WCHAR strDataSpecularFileName[MAX_PATH];
    UINT32 TextureSize;
    UINT32 PatternVariant;
};

void GenTexturesPattern0(const GenerateTextureJobDesc* pDesc, JobQueueThreadData* pThreadData, DirectX::ScratchImage& Diffuse, DirectX::ScratchImage& Normal, DirectX::ScratchImage& Specular)
{
    RandomNumberGenerator& RNG = pThreadData->RNG;

    const Image* pDiffusePlane = Diffuse.GetImage(0, 0, 0);
    const Image* pSpecularPlane = Specular.GetImage(0, 0, 0);

    ZeroMemory(pDiffusePlane->pixels, pDiffusePlane->slicePitch);
    ZeroMemory(pSpecularPlane->pixels, pSpecularPlane->slicePitch);

    const UINT32 Width = pDiffusePlane->width;
    const UINT32 Height = pDiffusePlane->height;

    DirectX::ScratchImage HeightMap;
    HeightMap.Initialize2D(DXGI_FORMAT_R8_UNORM, Width, Height, 1, 1);
    const Image* pHeightPlane = HeightMap.GetImage(0, 0, 0);
    ZeroMemory(pHeightPlane->pixels, pHeightPlane->slicePitch);

    UINT32 RectSize = (Width * 3) / 4;

    const FLOAT BaseHue = RNG.NextFloat();
    //DebugSpew("hue: %0.3f\n", BaseHue);

    for (UINT32 i = 0; i < 100; ++i)
    {
        const UINT32 RectX = RNG.NextInt(0, Width - 1);
        const UINT32 RectY = RNG.NextInt(0, Height - 1);
        const UINT32 EndX = RectX + RectSize;
        const UINT32 EndY = RectY + RectSize;

        const FLOAT Hue = std::max(0.0f, std::min(0.999f, BaseHue + RNG.NextFloat(-0.05f, 0.05f)));
        const FLOAT Saturation = RNG.NextFloat(0.9f, 1.0f);
        XMVECTOR vColor = XMColorHSVToRGB(XMVectorSet(Hue, Saturation, 1, 1));
        XMCOLOR DiffuseColor;
        XMStoreColor(&DiffuseColor, XMVectorSwizzle<2, 1, 0, 3>(vColor));
        XMCOLOR SpecularColor;
        SpecularColor.r = SpecularColor.g = SpecularColor.b = (BYTE)RNG.NextInt(0, 255);
        SpecularColor.a = 255;

        for (UINT32 ay = RectY; ay < EndY; ++ay)
        {
            const UINT32 y = ay % Height;

            UINT32* pDiffuseRow = (UINT32*)(pDiffusePlane->pixels + pDiffusePlane->rowPitch * y);
            UINT32* pSpecularRow = (UINT32*)(pSpecularPlane->pixels + pSpecularPlane->rowPitch * y);
            BYTE* pHeightRow = pHeightPlane->pixels + pHeightPlane->rowPitch * y;

            for (UINT32 ax = RectX; ax < EndX; ++ax)
            {
                const UINT32 x = ax % Width;
                pHeightRow[x] = i + 1;
                pDiffuseRow[x] = DiffuseColor.c;
                pSpecularRow[x] = SpecularColor.c;
            }
        }

        RectSize = (RectSize * 95) / 100;
    }

    const UINT32 DotCount = (Width * Height) >> 3;
    for (UINT32 i = 0; i < DotCount; ++i)
    {
        const UINT32 PointX = RNG.NextInt(0, Width - 1);
        const UINT32 PointY = RNG.NextInt(0, Height - 1);

        UINT32* pDiffusePixel = (UINT32*)(pDiffusePlane->pixels + pDiffusePlane->rowPitch * PointY) + PointX;
        UINT32* pSpecularPixel = (UINT32*)(pSpecularPlane->pixels + pSpecularPlane->rowPitch * PointY) + PointX;
        BYTE* pHeightPixel = pHeightPlane->pixels + (pHeightPlane->rowPitch * PointY) + PointX;

        XMVECTOR Color = XMLoadUByteN4((XMUBYTEN4*)pDiffusePixel);
        Color = XMVectorSaturate(Color * 1.5f);
        XMStoreUByteN4((XMUBYTEN4*)pDiffusePixel, Color);
        XMStoreColor((XMCOLOR*)pSpecularPixel, XMVectorSet(1, 1, 1, 1));
        *pHeightPixel += 100;
    }

    DirectX::ComputeNormalMap(HeightMap.GetImages(), HeightMap.GetImageCount(), HeightMap.GetMetadata(), CNMAP_CHANNEL_RED, 2.0f, DXGI_FORMAT_R8G8B8A8_UNORM, Normal);
}

inline UINT32 GetElementSizeShift(DXGI_FORMAT Format)
{
    switch (Format)
    {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 2;
    default:
        return 0;
    }
}

inline UINT32 GetElementSizeBytes(DXGI_FORMAT Format)
{
    const UINT32 ElementSizeShift = GetElementSizeShift(Format);
    const UINT32 TexelCount = 1U << (ElementSizeShift * 2);
    return (TexelCount * DirectX::BitsPerPixel(Format)) >> 3;
}

inline void CopyBCRectangle(_In_ const DirectX::Image& srcImage, _In_ const DirectX::Rect& srcRect, _In_ const DirectX::Image& dstImage, _In_ DWORD filter, _In_ size_t xOffset, _In_ size_t yOffset)
{
    static const UINT32 BCBlockShift = 2;
    static const UINT32 BCBlockMask = (1U << BCBlockShift) - 1;

    assert(srcRect.x + srcRect.w <= srcImage.width);
    assert(srcRect.y + srcRect.h <= srcImage.height);
    assert(xOffset + srcRect.w <= dstImage.width);
    assert(yOffset + srcRect.h <= dstImage.height);
    assert((xOffset & BCBlockMask) == 0 && (yOffset & BCBlockMask) == 0);
    assert((srcRect.x & BCBlockMask) == 0 && (srcRect.y & BCBlockMask) == 0 && (srcRect.w & BCBlockMask) == 0 && (srcRect.h & BCBlockMask) == 0);

    static const UINT32 BitsToBytesShift = 3;

    const UINT32 BitsPerPixel = DirectX::BitsPerPixel(srcImage.format);
    assert(BitsPerPixel == DirectX::BitsPerPixel(dstImage.format));

    const UINT32 SrcTopRows = (srcRect.y >> BCBlockShift);
    const UINT32 RectHeightRows = srcRect.h >> BCBlockShift;
    const UINT32 SrcLeftOffset = ((srcRect.x << BCBlockShift) * BitsPerPixel) >> BitsToBytesShift;
    const UINT32 RectWidthBytes = ((srcRect.w << BCBlockShift) * BitsPerPixel) >> BitsToBytesShift;
    assert(RectWidthBytes <= dstImage.rowPitch);
    assert(RectWidthBytes <= srcImage.rowPitch);

    const BYTE* pSrcRow = srcImage.pixels + SrcTopRows * srcImage.rowPitch;
    pSrcRow += SrcLeftOffset;

    const UINT32 DestTopRows = (yOffset >> BCBlockShift);
    const UINT32 DestLeftOffset = ((xOffset << BCBlockShift) * BitsPerPixel) >> BitsToBytesShift;
    BYTE* pDestRow = dstImage.pixels + DestTopRows * dstImage.rowPitch;
    pDestRow += DestLeftOffset;

    for (UINT32 RowIndex = 0; RowIndex < RectHeightRows; ++RowIndex)
    {
        memcpy(pDestRow, pSrcRow, RectWidthBytes);
        pDestRow += dstImage.rowPitch;
        pSrcRow += srcImage.rowPitch;
    }
}

HRESULT SaveTiledResourceImage(const DirectX::ScratchImage& CompressedImage, const WCHAR* strFileName, const WCHAR* strDataFileName)
{
    static const bool SaveDebugDDSFiles = false;

    HANDLE hFile = CreateFile2(strFileName, GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return E_FAIL;
    }

    HANDLE hDataFile = CreateFile2(strDataFileName, GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, nullptr);
    if (hDataFile == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        return E_FAIL;
    }

    const DirectX::TexMetadata& Metadata = CompressedImage.GetMetadata();
    const bool IsBlockCompressed = DirectX::IsCompressed(Metadata.format);

    TilingLayout TL = {};
    bool Success = CreateTilingLayout(Metadata.format, Metadata.width, Metadata.height, Metadata.mipLevels, Metadata.arraySize, &TL);
    if (!Success)
    {
        return E_OUTOFMEMORY;
    }

    DirectX::ScratchImage SingleTileImage;
    HRESULT hr = SingleTileImage.Initialize2D(Metadata.format, TL.TileShape.WidthInTexels, TL.TileShape.HeightInTexels, 1, 1);
    if (FAILED(hr)) return hr;

    if (Success)
    {
        // Fill in file header with essential info:

        TiledResourceImage::Header FileHeader = {};
        FileHeader.MagicVersion = TiledResourceImage::CURRENT_VERSION;
        FileHeader.MipLevelCount = Metadata.mipLevels;
        FileHeader.SliceCount = Metadata.arraySize;
        FileHeader.DXGIFormat = Metadata.format;

        // TODO: Xbox native swizzle flag
        FileHeader.Flags = 0;

        assert(TL.PackedMipInfo.NumStandardMips < ARRAYSIZE(FileHeader.MipLevels));
        assert((TL.PackedMipInfo.NumStandardMips + TL.PackedMipInfo.NumPackedMips) == FileHeader.MipLevelCount);

        // Fill in the mip levels in the header with the non-packed mips:

        UINT32 FileOffsetTiles = 0;
        for (UINT32 MipIndex = 0; MipIndex < TL.PackedMipInfo.NumStandardMips; ++MipIndex)
        {
            TiledResourceImage::MipLevel& DestMip = FileHeader.MipLevels[MipIndex];
            DestMip.FileOffsetTiles = FileOffsetTiles;
            DestMip.WidthTilesM1 = TL.pSubresourceTilings[MipIndex].WidthInTiles - 1;
            DestMip.HeightTilesM1 = TL.pSubresourceTilings[MipIndex].HeightInTiles - 1;
            DestMip.DepthTilesM1 = TL.pSubresourceTilings[MipIndex].DepthInTiles - 1;

            FileOffsetTiles += FileHeader.SliceCount * (TL.pSubresourceTilings[MipIndex].WidthInTiles * TL.pSubresourceTilings[MipIndex].HeightInTiles * TL.pSubresourceTilings[MipIndex].DepthInTiles);
        }

        // Fill in the remainder of the mip level header structures with packed mip location and size:

        FileHeader.MipTail.ElementSizeShift = GetElementSizeShift((DXGI_FORMAT)FileHeader.DXGIFormat);
        FileHeader.MipTail.ElementByteShift = FloorLog2(GetElementSizeBytes((DXGI_FORMAT)FileHeader.DXGIFormat));
        if (TL.PackedMipInfo.NumPackedMips > 0)
        {
            const UINT32 FirstPackedMipIndex = TL.PackedMipInfo.NumStandardMips;
            assert(FirstPackedMipIndex < ARRAYSIZE(FileHeader.MipLevels));
            for (UINT32 MipIndex = FirstPackedMipIndex; MipIndex < ARRAYSIZE(FileHeader.MipLevels); ++MipIndex)
            {
                TiledResourceImage::MipLevel& DestMip = FileHeader.MipLevels[MipIndex];
                DestMip.FileOffsetTiles = FileOffsetTiles;
                DestMip.WidthTilesM1 = TL.PackedMipInfo.NumTilesForPackedMips - 1;
                DestMip.HeightTilesM1 = 0;
                DestMip.DepthTilesM1 = 0;
                DestMip.Flags = TiledResourceImage::PackedMipLevel;
            }

            // Fill in the packed mip tail info with details of the packed mip tail:

            const DirectX::Image* pFirstPackedMipImage = CompressedImage.GetImage(FirstPackedMipIndex, 0, 0);
            FileHeader.MipTail.FirstRowPitchBytes = pFirstPackedMipImage->rowPitch;
            FileHeader.MipTail.FirstHeightRows = std::max(1, (int)(pFirstPackedMipImage->height >> FileHeader.MipTail.ElementSizeShift));
            FileHeader.MipTail.FirstMipIndex = FirstPackedMipIndex;
        }
        else
        {
            FileHeader.MipTail.FirstMipIndex = 0;
        }

        // Write the header:

        DWORD BytesWritten = 0;
        WriteFile(hFile, &FileHeader, (DWORD)sizeof(FileHeader), &BytesWritten, nullptr);

        const DirectX::Image* pTileImage = SingleTileImage.GetImages();
        assert(pTileImage->slicePitch == TiledResourceImage::TILE_SIZE_BYTES);

        DirectX::Rect CopyRect;
        CopyRect.w = TL.TileShape.WidthInTexels;
        CopyRect.h = TL.TileShape.HeightInTexels;
        for (UINT32 MipIndex = 0; MipIndex < TL.PackedMipInfo.NumStandardMips; ++MipIndex)
        {
            const D3D12_SUBRESOURCE_TILING& MipTiling = TL.pSubresourceTilings[MipIndex];

            for (UINT32 SliceIndex = 0; SliceIndex < Metadata.arraySize; ++SliceIndex)
            {
                const DirectX::Image* pSliceImage = CompressedImage.GetImage(MipIndex, 0, SliceIndex);
                //if (SaveDebugDDSFiles)
                //{
                //    WCHAR strDebugFilename[MAX_PATH];
                //    swprintf_s(strDebugFilename, L"%s_s%um%u.dds", strFileName, SliceIndex, MipIndex);
                //    hr = DirectX::SaveToDDSFile(*pSliceImage, DDS_FLAGS_NONE, strDebugFilename);
                //    assert(SUCCEEDED(hr));
                //}

                for (UINT32 TileY = 0; TileY < MipTiling.HeightInTiles; ++TileY)
                {
                    CopyRect.y = CopyRect.h * TileY;
                    for (UINT32 TileX = 0; TileX < MipTiling.WidthInTiles; ++TileX)
                    {
                        CopyRect.x = CopyRect.w * TileX;
                        if (IsBlockCompressed)
                        {
                            CopyBCRectangle(*pSliceImage, CopyRect, *pTileImage, 0, 0, 0);
                        }
                        else
                        {
                            DirectX::CopyRectangle(*pSliceImage, CopyRect, *pTileImage, 0, 0, 0);
                        }

                        WriteFile(hDataFile, pTileImage->pixels, (DWORD)TiledResourceImage::TILE_SIZE_BYTES, &BytesWritten, nullptr);
                        //if (SaveDebugDDSFiles)
                        //{
                        //    WCHAR strDebugFilename[MAX_PATH];
                        //    swprintf_s(strDebugFilename, L"%s_s%um%ux%uy%u.dds", strFileName, SliceIndex, MipIndex, TileX, TileY);
                        //    hr = DirectX::SaveToDDSFile(pTileImage, 1, SingleTileImage.GetMetadata(), DDS_FLAGS_NONE, strDebugFilename);
                        //    assert(SUCCEEDED(hr));
                        //}
                    }
                }
            }
        }

        UINT32 PackedMipSizeBytes = 0;
        for (UINT32 MipIndex = TL.PackedMipInfo.NumStandardMips; MipIndex < FileHeader.MipLevelCount; ++MipIndex)
        {
            const UINT32 ElementSizeTexels = 1U << FileHeader.MipTail.ElementSizeShift;
            const UINT32 TailMipIndex = MipIndex - TL.PackedMipInfo.NumStandardMips;
            const UINT32 HeightRows = std::max(1U, FileHeader.MipTail.FirstHeightRows >> TailMipIndex);
            const UINT32 PitchBytes = std::max(1U << FileHeader.MipTail.ElementByteShift, FileHeader.MipTail.FirstRowPitchBytes >> TailMipIndex);

            for (UINT32 SliceIndex = 0; SliceIndex < Metadata.arraySize; ++SliceIndex)
            {
                const DirectX::Image* pSliceImage = CompressedImage.GetImage(MipIndex, 0, SliceIndex);
                assert(pSliceImage->rowPitch >= PitchBytes);
                const UINT32 ImageHeightRows = (pSliceImage->height + (ElementSizeTexels - 1)) >> FileHeader.MipTail.ElementSizeShift;
                assert(ImageHeightRows == HeightRows);

                if (pSliceImage->rowPitch == PitchBytes)
                {
                    UINT32 ByteCount = pSliceImage->rowPitch * HeightRows;
                    WriteFile(hDataFile, pSliceImage->pixels, ByteCount, &BytesWritten, nullptr);
                    PackedMipSizeBytes += ByteCount;
                }
                else
                {
                    const BYTE* pSrc = pSliceImage->pixels;
                    for (UINT32 RowIndex = 0; RowIndex < HeightRows; ++RowIndex)
                    {
                        WriteFile(hDataFile, pSrc, PitchBytes, &BytesWritten, nullptr);
                        PackedMipSizeBytes += PitchBytes;
                        pSrc += pSliceImage->rowPitch;
                    }
                }
            }
        }

        assert(PackedMipSizeBytes <= (TL.PackedMipInfo.NumTilesForPackedMips * TiledResourceImage::TILE_SIZE_BYTES));

        // Pad the packed mip tail to a multiple of the tile size, for efficient aligned loading:
        const UINT32 PaddingSize = (TL.PackedMipInfo.NumTilesForPackedMips * TiledResourceImage::TILE_SIZE_BYTES) - PackedMipSizeBytes;
        if (PaddingSize > 0)
        {
            BYTE* pPadding = new BYTE[PaddingSize];
            assert(pPadding != nullptr);
            ZeroMemory(pPadding, PaddingSize);
            WriteFile(hDataFile, pPadding, PaddingSize, nullptr, nullptr);
            delete[] pPadding;
        }
    }

    CloseHandle(hFile);
    CloseHandle(hDataFile);
    SingleTileImage.Release();
    FreeTilingLayout(&TL);

    return S_OK;
}

UINT32 CreateTextureWorker(const GenerateTextureJobDesc* pDesc, void* pData0, void* pData1, JobQueueThreadData* pThreadData)
{
    DirectX::ScratchImage ImgDiffuseBase;
    HRESULT hr = ImgDiffuseBase.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, pDesc->TextureSize, pDesc->TextureSize, 1, 1);
    if (FAILED(hr)) { DebugSpew("ERROR: Diffuse initialize HRESULT 0x%x\n", static_cast<unsigned int>(hr)); return 1; }

    DirectX::ScratchImage ImgNormalBase;
    hr = ImgNormalBase.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, pDesc->TextureSize, pDesc->TextureSize, 1, 1);
    if (FAILED(hr)) { DebugSpew("ERROR: Normal initialize HRESULT 0x%x\n", static_cast<unsigned int>(hr)); return 1; }

    DirectX::ScratchImage ImgSpecularBase;
    hr = ImgSpecularBase.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, pDesc->TextureSize, pDesc->TextureSize, 1, 1);
    if (FAILED(hr)) { DebugSpew("ERROR: Specular initialize HRESULT 0x%x\n", static_cast<unsigned int>(hr)); return 1; }

    switch (pDesc->PatternVariant)
    {
    case 0:
    default:
        GenTexturesPattern0(pDesc, pThreadData, ImgDiffuseBase, ImgNormalBase, ImgSpecularBase);
        break;
    }

    LONG64 SizeBytes = 0;

    DirectX::ScratchImage ImgDiffuseMips;
    hr = DirectX::GenerateMipMaps(ImgDiffuseBase.GetImages(), ImgDiffuseBase.GetImageCount(), ImgDiffuseBase.GetMetadata(), TEX_FILTER_DEFAULT, 0, ImgDiffuseMips);
    if (FAILED(hr)) { DebugSpew("ERROR: Diffuse mips HRESULT 0x%x\n", static_cast<unsigned int>(hr)); return 1; }
    ImgDiffuseBase.Release();

    DirectX::ScratchImage ImgDiffuseBC;
    hr = DirectX::Compress(ImgDiffuseMips.GetImages(), ImgDiffuseMips.GetImageCount(), ImgDiffuseMips.GetMetadata(), DXGI_FORMAT_BC1_UNORM, TEX_COMPRESS_DEFAULT, 0.5f, ImgDiffuseBC);
    if (FAILED(hr)) { DebugSpew("ERROR: Diffuse compress HRESULT 0x%x\n", static_cast<unsigned int>(hr)); return 1; }
    ImgDiffuseMips.Release();
    //DirectX::SaveToDDSFile(ImgDiffuseBC.GetImages(), ImgDiffuseBC.GetImageCount(), ImgDiffuseBC.GetMetadata(), DDS_FLAGS_NONE, pDesc->strDiffuseFileName);
    hr = SaveTiledResourceImage(ImgDiffuseBC, pDesc->strDiffuseFileName, pDesc->strDataDiffuseFileName);
    if (FAILED(hr)) { DebugSpew("ERROR: Diffuse save HRESULT 0x%x\n", static_cast<unsigned int>(hr)); return 1; }
    SizeBytes += ImgDiffuseBC.GetPixelsSize();
    ImgDiffuseBC.Release();

    DirectX::ScratchImage ImgNormalMips;
    hr = DirectX::GenerateMipMaps(ImgNormalBase.GetImages(), ImgNormalBase.GetImageCount(), ImgNormalBase.GetMetadata(), TEX_FILTER_DEFAULT, 0, ImgNormalMips);
    if (FAILED(hr)) { DebugSpew("ERROR: Normal mips HRESULT 0x%x\n", static_cast<unsigned int>(hr)); return 1; }
    ImgNormalBase.Release();

    DirectX::ScratchImage ImgNormalBC;
    hr = DirectX::Compress(ImgNormalMips.GetImages(), ImgNormalMips.GetImageCount(), ImgNormalMips.GetMetadata(), DXGI_FORMAT_BC3_UNORM, TEX_COMPRESS_UNIFORM, 0.5f, ImgNormalBC);
    if (FAILED(hr)) { DebugSpew("ERROR: Normal compress HRESULT 0x%x\n", static_cast<unsigned int>(hr)); return 1; }
    ImgNormalMips.Release();
    //DirectX::SaveToDDSFile(ImgNormalBC.GetImages(), ImgNormalBC.GetImageCount(), ImgNormalBC.GetMetadata(), DDS_FLAGS_NONE, pDesc->strNormalFileName);
    hr = SaveTiledResourceImage(ImgNormalBC, pDesc->strNormalFileName, pDesc->strDataNormalFileName);
    if (FAILED(hr)) { DebugSpew("ERROR: Normal save HRESULT 0x%x\n", static_cast<unsigned int>(hr)); return 1; }
    SizeBytes += ImgNormalBC.GetPixelsSize();
    ImgNormalBC.Release();

    DirectX::ScratchImage ImgSpecularMips;
    hr = DirectX::GenerateMipMaps(ImgSpecularBase.GetImages(), ImgSpecularBase.GetImageCount(), ImgSpecularBase.GetMetadata(), TEX_FILTER_DEFAULT, 0, ImgSpecularMips);
    if (FAILED(hr)) { DebugSpew("ERROR: Specular mips HRESULT 0x%x\n", static_cast<unsigned int>(hr)); return 1; }
    ImgSpecularBase.Release();

    DirectX::ScratchImage ImgSpecularBC;
    hr = DirectX::Compress(ImgSpecularMips.GetImages(), ImgSpecularMips.GetImageCount(), ImgSpecularMips.GetMetadata(), DXGI_FORMAT_BC1_UNORM, TEX_COMPRESS_DEFAULT, 0.5f, ImgSpecularBC);
    if (FAILED(hr)) { DebugSpew("ERROR: Specular compress HRESULT 0x%x\n", static_cast<unsigned int>(hr)); return 1; }
    ImgSpecularMips.Release();
    //DirectX::SaveToDDSFile(ImgSpecularBC.GetImages(), ImgSpecularBC.GetImageCount(), ImgSpecularBC.GetMetadata(), DDS_FLAGS_NONE, pDesc->strSpecularFileName);
    hr = SaveTiledResourceImage(ImgSpecularBC, pDesc->strSpecularFileName, pDesc->strDataSpecularFileName);
    if (FAILED(hr)) { DebugSpew("ERROR: Specular save HRESULT 0x%x\n", static_cast<unsigned int>(hr)); return 1; }
    SizeBytes += ImgSpecularBC.GetPixelsSize();
    ImgSpecularBC.Release();

    delete pDesc;
    InterlockedIncrement(&g_TextureCount);
    InterlockedAdd64(&g_TextureSizeBytes, SizeBytes);
    return 0;
}

bool GenerateTextures(UINT32 Count, UINT32 Dimension, ID3D12Device* pd3dDevice)
{
    g_RequestedTextureCount = Count;
    g_TextureCount = 0;
    g_TextureSizeBytes = 0;
    g_pd3dDevice = pd3dDevice;

    bool Success = CreateDirectoryW(GetTextureBasePath(), nullptr);

    GenerateTextureJobDesc Desc = {};
    Desc.TextureSize = Dimension;

    bool CreatedTextures = false;

    for (UINT32 i = 0; i < Count; ++i)
    {
        CreateTextureFilename(Desc.strDiffuseFileName, ARRAYSIZE(Desc.strDiffuseFileName), i, TextureSuffix_Diffuse, false);
        CreateTextureFilename(Desc.strNormalFileName, ARRAYSIZE(Desc.strNormalFileName), i, TextureSuffix_Normal, false);
        CreateTextureFilename(Desc.strSpecularFileName, ARRAYSIZE(Desc.strSpecularFileName), i, TextureSuffix_Specular, false);

        CreateTextureFilename(Desc.strDataDiffuseFileName, ARRAYSIZE(Desc.strDataDiffuseFileName), i, TextureSuffix_Diffuse, true);
        CreateTextureFilename(Desc.strDataNormalFileName, ARRAYSIZE(Desc.strDataNormalFileName), i, TextureSuffix_Normal, true);
        CreateTextureFilename(Desc.strDataSpecularFileName, ARRAYSIZE(Desc.strDataSpecularFileName), i, TextureSuffix_Specular, true);

        GenerateTextureJobDesc* pDesc = new GenerateTextureJobDesc();
        memcpy(pDesc, &Desc, sizeof(*pDesc));
        g_JobQueue.AddJob((JobWorkerFunction)CreateTextureWorker, pDesc, nullptr, nullptr);
        CreatedTextures = true;
    }

    return CreatedTextures;
}

bool IsTextureGenerationComplete(UINT32* pCompleted, UINT32* pTotal, UINT64* pBytes)
{
    if (pCompleted != nullptr)
    {
        *pCompleted = g_TextureCount;
    }
    if (pTotal != nullptr)
    {
        *pTotal = g_RequestedTextureCount;
    }
    if (pBytes != nullptr)
    {
        *pBytes = g_TextureSizeBytes;
    }
    return g_TextureCount >= g_RequestedTextureCount;
}
