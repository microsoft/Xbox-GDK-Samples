//--------------------------------------------------------------------------------------
// D3D12Util.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "precomp.hpp"
#include "D3D12Util.h"
#include "util.hpp"
#include <nmmintrin.h>

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |       \
                ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24))
#endif /* defined(MAKEFOURCC) */

//--------------------------------------------------------------------------------------
// DDS file structure definitions
//
// See DDS.h in the 'Texconv' sample and the 'DirectXTex' library
//--------------------------------------------------------------------------------------
#pragma pack(push,1)

const uint32_t DDS_MAGIC = 0x20534444; // "DDS "

struct DDS_PIXELFORMAT
{
    uint32_t    size;
    uint32_t    flags;
    uint32_t    fourCC;
    uint32_t    RGBBitCount;
    uint32_t    RBitMask;
    uint32_t    GBitMask;
    uint32_t    BBitMask;
    uint32_t    ABitMask;
};

#define DDS_FOURCC      0x00000004  // DDPF_FOURCC
#define DDS_RGB         0x00000040  // DDPF_RGB
#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDS_ALPHA       0x00000002  // DDPF_ALPHA

#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH

#define DDS_HEIGHT 0x00000002 // DDSD_HEIGHT
#define DDS_WIDTH  0x00000004 // DDSD_WIDTH

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES (DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
                               DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
                               DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ)

#define DDS_CUBEMAP 0x00000200 // DDSCAPS2_CUBEMAP

enum DDS_MISC_FLAGS2
{
    DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7L,
};

struct DDS_HEADER
{
    uint32_t        size;
    uint32_t        flags;
    uint32_t        height;
    uint32_t        width;
    uint32_t        pitchOrLinearSize;
    uint32_t        depth; // only if DDS_HEADER_FLAGS_VOLUME is set in flags
    uint32_t        mipMapCount;
    uint32_t        reserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t        caps;
    uint32_t        caps2;
    uint32_t        caps3;
    uint32_t        caps4;
    uint32_t        reserved2;
};

struct DDS_HEADER_DXT10
{
    DXGI_FORMAT     dxgiFormat;
    uint32_t        resourceDimension;
    uint32_t        miscFlag; // see D3D11_RESOURCE_MISC_FLAG
    uint32_t        arraySize;
    uint32_t        miscFlags2;
};

#pragma pack(pop)

//--------------------------------------------------------------------------------------
#define ISBITMASK(r,g,b,a) (ddpf.RBitMask == r && ddpf.GBitMask == g && ddpf.BBitMask == b && ddpf.ABitMask == a)

static DXGI_FORMAT GetDXGIFormat(const DDS_PIXELFORMAT& ddpf)
{
    if (ddpf.flags & DDS_RGB)
    {
        // Note that sRGB formats are written using the "DX10" extended header

        switch (ddpf.RGBBitCount)
        {
        case 32:
            if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
            {
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            }

            if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
            {
                return DXGI_FORMAT_B8G8R8A8_UNORM;
            }

            if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
            {
                return DXGI_FORMAT_B8G8R8X8_UNORM;
            }

            // No DXGI format maps to ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0x00000000) aka D3DFMT_X8B8G8R8

            // Note that many common DDS reader/writers (including D3DX) swap the
            // the RED/BLUE masks for 10:10:10:2 formats. We assumme
            // below that the 'backwards' header mask is being used since it is most
            // likely written by D3DX. The more robust solution is to use the 'DX10'
            // header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

            // For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
            if (ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
            {
                return DXGI_FORMAT_R10G10B10A2_UNORM;
            }

            // No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10

            if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
            {
                return DXGI_FORMAT_R16G16_UNORM;
            }

            if (ISBITMASK(0xffffffff, 0x00000000, 0x00000000, 0x00000000))
            {
                // Only 32-bit color channel format in D3D9 was R32F
                return DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114
            }
            break;

        case 24:
            // No 24bpp DXGI formats aka D3DFMT_R8G8B8
            break;

        case 16:
            if (ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x8000))
            {
                return DXGI_FORMAT_B5G5R5A1_UNORM;
            }
            if (ISBITMASK(0xf800, 0x07e0, 0x001f, 0x0000))
            {
                return DXGI_FORMAT_B5G6R5_UNORM;
            }

            // No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0x0000) aka D3DFMT_X1R5G5B5

            if (ISBITMASK(0x0f00, 0x00f0, 0x000f, 0xf000))
            {
                return DXGI_FORMAT_B4G4R4A4_UNORM;
            }

            // No DXGI format maps to ISBITMASK(0x0f00,0x00f0,0x000f,0x0000) aka D3DFMT_X4R4G4B4

            // No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
            break;
        }
    }
    else if (ddpf.flags & DDS_LUMINANCE)
    {
        if (8 == ddpf.RGBBitCount)
        {
            if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x00000000))
            {
                return DXGI_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension
            }

            // No DXGI format maps to ISBITMASK(0x0f,0x00,0x00,0xf0) aka D3DFMT_A4L4
        }

        if (16 == ddpf.RGBBitCount)
        {
            if (ISBITMASK(0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
            {
                return DXGI_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension
            }
            if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
            {
                return DXGI_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
            }
        }
    }
    else if (ddpf.flags & DDS_ALPHA)
    {
        if (8 == ddpf.RGBBitCount)
        {
            return DXGI_FORMAT_A8_UNORM;
        }
    }
    else if (ddpf.flags & DDS_FOURCC)
    {
        if (MAKEFOURCC('D', 'X', 'T', '1') == ddpf.fourCC)
        {
            return DXGI_FORMAT_BC1_UNORM;
        }
        if (MAKEFOURCC('D', 'X', 'T', '3') == ddpf.fourCC)
        {
            return DXGI_FORMAT_BC2_UNORM;
        }
        if (MAKEFOURCC('D', 'X', 'T', '5') == ddpf.fourCC)
        {
            return DXGI_FORMAT_BC3_UNORM;
        }

        // While pre-mulitplied alpha isn't directly supported by the DXGI formats,
        // they are basically the same as these BC formats so they can be mapped
        if (MAKEFOURCC('D', 'X', 'T', '2') == ddpf.fourCC)
        {
            return DXGI_FORMAT_BC2_UNORM;
        }
        if (MAKEFOURCC('D', 'X', 'T', '4') == ddpf.fourCC)
        {
            return DXGI_FORMAT_BC3_UNORM;
        }

        if (MAKEFOURCC('A', 'T', 'I', '1') == ddpf.fourCC)
        {
            return DXGI_FORMAT_BC4_UNORM;
        }
        if (MAKEFOURCC('B', 'C', '4', 'U') == ddpf.fourCC)
        {
            return DXGI_FORMAT_BC4_UNORM;
        }
        if (MAKEFOURCC('B', 'C', '4', 'S') == ddpf.fourCC)
        {
            return DXGI_FORMAT_BC4_SNORM;
        }

        if (MAKEFOURCC('A', 'T', 'I', '2') == ddpf.fourCC)
        {
            return DXGI_FORMAT_BC5_UNORM;
        }
        if (MAKEFOURCC('B', 'C', '5', 'U') == ddpf.fourCC)
        {
            return DXGI_FORMAT_BC5_UNORM;
        }
        if (MAKEFOURCC('B', 'C', '5', 'S') == ddpf.fourCC)
        {
            return DXGI_FORMAT_BC5_SNORM;
        }

        // BC6H and BC7 are written using the "DX10" extended header

        if (MAKEFOURCC('R', 'G', 'B', 'G') == ddpf.fourCC)
        {
            return DXGI_FORMAT_R8G8_B8G8_UNORM;
        }
        if (MAKEFOURCC('G', 'R', 'G', 'B') == ddpf.fourCC)
        {
            return DXGI_FORMAT_G8R8_G8B8_UNORM;
        }

        if (MAKEFOURCC('Y', 'U', 'Y', '2') == ddpf.fourCC)
        {
            return DXGI_FORMAT_YUY2;
        }

        // Check for D3DFORMAT enums being set here
        switch (ddpf.fourCC)
        {
        case 36: // D3DFMT_A16B16G16R16
            return DXGI_FORMAT_R16G16B16A16_UNORM;

        case 110: // D3DFMT_Q16W16V16U16
            return DXGI_FORMAT_R16G16B16A16_SNORM;

        case 111: // D3DFMT_R16F
            return DXGI_FORMAT_R16_FLOAT;

        case 112: // D3DFMT_G16R16F
            return DXGI_FORMAT_R16G16_FLOAT;

        case 113: // D3DFMT_A16B16G16R16F
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case 114: // D3DFMT_R32F
            return DXGI_FORMAT_R32_FLOAT;

        case 115: // D3DFMT_G32R32F
            return DXGI_FORMAT_R32G32_FLOAT;

        case 116: // D3DFMT_A32B32G32R32F
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
    }

    return DXGI_FORMAT_UNKNOWN;
}



//--------------------------------------------------------------------------------------
// Determines if the format is block compressed
//--------------------------------------------------------------------------------------
static bool IsCompressed(_In_ DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return true;

    default:
        return false;
    }
}


//--------------------------------------------------------------------------------------
// Return the BPP for a particular format
//--------------------------------------------------------------------------------------
UINT32 BitsPerPixel(_In_ DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 96;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
        return 64;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
    case DXGI_FORMAT_YUY2:
        return 32;

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
        return 24;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_A8P8:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        return 16;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_NV11:
        return 12;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
        return 8;

    case DXGI_FORMAT_R1_UNORM:
        return 1;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 4;

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
        return 8;

#if defined(_XBOX_ONE) && defined(_TITLE)

    case DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT:
    case DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT:
        return 32;

    case DXGI_FORMAT_D16_UNORM_S8_UINT:
    case DXGI_FORMAT_R16_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X16_TYPELESS_G8_UINT:
        return 24;

#endif // _XBOX_ONE && _TITLE

    default:
        return 0;
    }
}

//-------------------------------------------------------------------------------------------------
// Calculates the 32-bit CRC for the specified dword-aligned buffer.
inline UINT32 ComputeCrc32(
    const void* pBuffer,
    UINT32 byteCount
    )
{
    assert(byteCount % 4 == 0 && (UINT64)pBuffer % 4 == 0);

    UINT32 byteRemainder = byteCount;
    UINT64 crc32 = 0xffffffff;

    const UINT64* pSource64 = (const UINT64*)pBuffer;
    while (byteRemainder >= 32)
    {
        crc32 = _mm_crc32_u64(crc32, pSource64[0]);
        crc32 = _mm_crc32_u64(crc32, pSource64[1]);
        crc32 = _mm_crc32_u64(crc32, pSource64[2]);
        crc32 = _mm_crc32_u64(crc32, pSource64[3]);
        pSource64 += 4;
        byteRemainder -= 32;
    }

    const UINT32* pSource32 = (const UINT32*)pSource64;
    while (byteRemainder >= 4)
    {
        crc32 = (UINT64)_mm_crc32_u32((UINT32)crc32, *pSource32);
        pSource32++;
        byteRemainder -= 4;
    }

    if (byteRemainder > 0)
    {
        // Mask out the extra bytes in the last dword by simply shifting them away:

        UINT32 bytesToIgnore = 4 - byteRemainder;
        crc32 = (UINT64)_mm_crc32_u32((UINT32)crc32, *pSource32 << (bytesToIgnore * 8));
    }

    return (UINT32)crc32;
}

void ResourceBarrier(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After, UINT32 SubresourceIndex)
{
    D3D12_RESOURCE_BARRIER barrierDesc = {};
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierDesc.Transition.pResource = pResource;
    barrierDesc.Transition.Subresource = SubresourceIndex;
    barrierDesc.Transition.StateBefore = Before;
    barrierDesc.Transition.StateAfter = After;
    pCmdList->ResourceBarrier(1, &barrierDesc);
}

#if defined(_XBOX_ONE) && defined(_TITLE)
void ResourceBarrier(ID3D12XboxDmaCommandList* pCmdList, ID3D12Resource* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After, UINT32 SubresourceIndex)
{
    D3D12_RESOURCE_BARRIER barrierDesc = {};
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierDesc.Transition.pResource = pResource;
    barrierDesc.Transition.Subresource = SubresourceIndex;
    barrierDesc.Transition.StateBefore = Before;
    barrierDesc.Transition.StateAfter = After;
    pCmdList->ResourceBarrier(1, &barrierDesc);
}
#endif

bool IsBlockCompressedFormat(DXGI_FORMAT Format)
{
    switch (Format)
    {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return true;
    default:
        return false;
    }
}

bool IsCommonStatePromotionEnabled()
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    return (D3D12XboxGetProcessDebugFlags() & D3D12XBOX_PROCESS_DEBUG_FLAG_ENABLE_COMMON_STATE_PROMOTION) != 0;
#else
    return true;
#endif
}

std::size_t PSOCache::PSODescHashFunction::operator() (const D3D12_GRAPHICS_PIPELINE_STATE_DESC& PSODesc) const
{
    return ComputeCrc32(&PSODesc, sizeof(PSODesc));
}

bool PSOCache::PSODescEqualFunction::operator () (const D3D12_GRAPHICS_PIPELINE_STATE_DESC& PSODescA, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& PSODescB) const
{
    return memcmp(&PSODescA, &PSODescB, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC)) == 0;
}

void* PSOCache::DuplicateMemory(const void* pSrc, size_t SizeBytes)
{
    void* pDupe = malloc(SizeBytes);
    if (pDupe == nullptr)
    {
        return nullptr;
    }
    memcpy(pDupe, pSrc, SizeBytes);
    m_Allocations.push_back(pDupe);
    return pDupe;
}

void PSOCache::Initialize(ID3D12Device* pd3dDevice, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& PSOTemplate)
{
    if (!m_Map.empty())
    { 
        Terminate();
    }
    m_PSOTemplate = PSOTemplate;
    m_pd3dDevice = pd3dDevice;
    m_pd3dDevice->AddRef();
}

void PSOCache::Terminate()
{
    {
        auto iter = m_Map.begin();
        auto end = m_Map.end();
        while (iter != end)
        {
            ID3D12PipelineState* pPSO = iter->second;
            pPSO->Release();
            ++iter;
        }
        m_Map.clear();
    }

    {
        auto iter = m_Allocations.begin();
        auto end = m_Allocations.end();
        while (iter != end)
        {
            void* pMem = *iter;
            free(pMem);
            ++iter;
        }
        m_Allocations.clear();
    }

    SAFE_RELEASE(m_pd3dDevice);
}

ID3D12PipelineState* PSOCache::FindPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& PSODesc)
{
    auto iter = m_Map.find(PSODesc);
    if (iter != m_Map.end())
    {
        return iter->second;
    }
    return nullptr;
}

ID3D12PipelineState* PSOCache::FindOrCreatePSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& PSODesc)
{
    ID3D12PipelineState* pPSO = FindPSO(PSODesc);
    if (pPSO != nullptr)
    {
        return pPSO;
    }

    HRESULT hr = m_pd3dDevice->CreateGraphicsPipelineState(&PSODesc, __uuidof(*pPSO), (void**)&pPSO);
    if (SUCCEEDED(hr))
    {
        m_Map[PSODesc] = pPSO;
        return pPSO;
    }

    return nullptr;
}

HRESULT D3D12DynamicBuffer::Create(ID3D12Device* pd3dDevice, ID3D12Fence* pGpuFence, UINT64* pCpuFence, UINT32 ByteWidth, UINT32 VertexStrideBytes, UINT32 InitialRenameCount)
{
    m_CurrentBufferIndex = 0;
    m_ByteWidth = NextMultiple( ByteWidth, 256U );
    m_VertexStrideBytes = VertexStrideBytes;
    m_pd3dDevice = pd3dDevice;
    m_pd3dDevice->AddRef();
    m_pGpuFence = pGpuFence;
    m_pGpuFence->AddRef();
    m_pCpuFence = pCpuFence;

    InitialRenameCount = std::min(256U, std::max(1U, InitialRenameCount));

    for (UINT32 i = 0; i < InitialRenameCount; ++i)
    {
        UINT32 Index = FindOrCreateRenameBuffer(nullptr, true);
        if (Index == -1)
        {
            return E_FAIL;
        }
    }
    return S_OK;
}

void D3D12DynamicBuffer::Terminate()
{
    UINT32 BufferCount = (UINT32)m_RenameBuffers.size();
    for (UINT32 i = 0; i < BufferCount; ++i)
    {
        RenameBuffer& RB = m_RenameBuffers[i];
        SAFE_RELEASE(RB.pBuffer);
    }
    m_RenameBuffers.clear();
    SAFE_RELEASE(m_pGpuFence);
    SAFE_RELEASE(m_pd3dDevice);
}

HRESULT D3D12DynamicBuffer::MapDiscard(ID3D12CommandQueue* pd3dCmdQueue, D3D12_MAPPED_SUBRESOURCE* pMapData)
{
    UINT32 RenameIndex = FindOrCreateRenameBuffer(pd3dCmdQueue, false);
    if (RenameIndex == -1)
    {
        return E_OUTOFMEMORY;
    }
    m_CurrentBufferIndex = RenameIndex;

    return MapNoOverwrite(pd3dCmdQueue, pMapData);
}

HRESULT D3D12DynamicBuffer::MapNoOverwrite(ID3D12CommandQueue* pd3dCmdQueue, D3D12_MAPPED_SUBRESOURCE* pMapData)
{
    assert(m_CurrentBufferIndex < m_RenameBuffers.size());
    assert(pMapData != nullptr);
    RenameBuffer& RB = m_RenameBuffers[m_CurrentBufferIndex];
    RB.WriteFence = *m_pCpuFence;

    HRESULT hr = RB.pBuffer->Map(0, nullptr, &pMapData->pData);
    pMapData->RowPitch = m_ByteWidth;
    pMapData->DepthPitch = 0;
    return hr;
}

HRESULT D3D12DynamicBuffer::Unmap(ID3D12CommandQueue* pd3dCmdQueue)
{
    assert(m_CurrentBufferIndex < m_RenameBuffers.size());
    RenameBuffer& RB = m_RenameBuffers[m_CurrentBufferIndex];
    RB.pBuffer->Unmap(0, nullptr);
    return S_OK;
}

HRESULT D3D12DynamicBuffer::CopyDiscard(ID3D12CommandQueue* pd3dCmdQueue, const void* pSrc, SIZE_T SrcSizeBytes)
{
    D3D12_MAPPED_SUBRESOURCE MapData = {};
    HRESULT hr = MapDiscard(pd3dCmdQueue, &MapData);
    if (FAILED(hr))
    {
        return hr;
    }
    memcpy(MapData.pData, pSrc, SrcSizeBytes);
    return Unmap(pd3dCmdQueue);
}

UINT32 D3D12DynamicBuffer::FindOrCreateRenameBuffer(ID3D12CommandQueue* pCmdQueue, bool ForceCreate)
{
    UINT32 FoundIndex = -1;
    if (!ForceCreate)
    {
        assert(pCmdQueue != nullptr);

        // find an unused rename buffer
        const UINT64 CompletedFence = m_pGpuFence->GetCompletedValue();
        if (CompletedFence == 0)
        {
            return 0;
        }

        const UINT32 BufferCount = (UINT32)m_RenameBuffers.size();
        for (UINT32 i = 0; i < BufferCount; ++i)
        {
            const UINT32 OffsetIndex = (i + m_CurrentBufferIndex) % BufferCount;
            const RenameBuffer& RB = m_RenameBuffers[OffsetIndex];
            if (RB.WriteFence <= CompletedFence)
            {
                return OffsetIndex;
            }
        }
        ForceCreate = true;
    }

    if (ForceCreate)
    {
        RenameBuffer NewBuffer = {};
        NewBuffer.WriteFence = 0;

        D3D12_HEAP_PROPERTIES HeapProperties = {};
        HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProperties.VisibleNodeMask = D3D12XBOX_NODE_MASK;
        HeapProperties.CreationNodeMask = D3D12XBOX_NODE_MASK;

        D3D12_RESOURCE_DESC BufferDesc = {};
        BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        BufferDesc.Alignment = 0;
        BufferDesc.Width = m_ByteWidth;
        BufferDesc.Height = 1;
        BufferDesc.DepthOrArraySize = 1;
        BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        BufferDesc.MipLevels = 1;
        BufferDesc.SampleDesc.Count = 1;
        BufferDesc.SampleDesc.Quality = 0;
        BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = m_pd3dDevice->CreateCommittedResource(&HeapProperties,
                                                D3D12_HEAP_FLAG_NONE,
                                                &BufferDesc,
                                                D3D12_RESOURCE_STATE_GENERIC_READ,
                                                nullptr,
                                                __uuidof(ID3D12Resource),
                                                (void**)&NewBuffer.pBuffer);
        if (FAILED(hr))
        {
            SAFE_RELEASE(NewBuffer.pBuffer);
            return -1;
        }

        FoundIndex = (UINT32)m_RenameBuffers.size();
        m_RenameBuffers.push_back(NewBuffer);
    }

    return FoundIndex;
}

void D3D12DynamicBuffer::GetCBDesc(D3D12_CONSTANT_BUFFER_VIEW_DESC* pCBDesc) const
{
    ID3D12Resource* pBuffer = GetBuffer();
    pCBDesc->BufferLocation = pBuffer->GetGPUVirtualAddress();
    pCBDesc->SizeInBytes = m_ByteWidth;
}

void D3D12DynamicBuffer::GetSBDesc(D3D12_SHADER_RESOURCE_VIEW_DESC* pSBDesc) const
{
    pSBDesc->Format = DXGI_FORMAT_UNKNOWN;
    pSBDesc->ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    pSBDesc->Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    pSBDesc->Buffer.FirstElement = 0;
    pSBDesc->Buffer.NumElements = 1;
    pSBDesc->Buffer.StructureByteStride = m_ByteWidth;
    pSBDesc->Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
}

void D3D12DynamicBuffer::GetVBDesc(D3D12_VERTEX_BUFFER_VIEW* pVBDesc) const
{
    ID3D12Resource* pBuffer = GetBuffer();
    pVBDesc->BufferLocation = pBuffer->GetGPUVirtualAddress();
    pVBDesc->StrideInBytes = m_VertexStrideBytes;
    pVBDesc->SizeInBytes = m_ByteWidth;
}

void D3D12DynamicBuffer::GetIBDesc(D3D12_INDEX_BUFFER_VIEW* pIBDesc, DXGI_FORMAT IndexFormat) const
{
    assert(IndexFormat == DXGI_FORMAT_R16_UINT || IndexFormat == DXGI_FORMAT_R32_UINT);
    ID3D12Resource* pBuffer = GetBuffer();
    pIBDesc->BufferLocation = pBuffer->GetGPUVirtualAddress();
    pIBDesc->Format = IndexFormat;
    pIBDesc->SizeInBytes = m_ByteWidth;
}

void D3D12DynamicBuffer::CreateCBView(D3D12_CPU_DESCRIPTOR_HANDLE DestHandle)
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC CBDesc = {};
    GetCBDesc(&CBDesc);
    m_pd3dDevice->CreateConstantBufferView(&CBDesc, DestHandle);
}

void D3D12DynamicBuffer::CreateSBView(D3D12_CPU_DESCRIPTOR_HANDLE DestHandle)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    GetSBDesc(&SRVDesc);
    ID3D12Resource* pBuffer = GetBuffer();
    m_pd3dDevice->CreateShaderResourceView(pBuffer, &SRVDesc, DestHandle);
}

void D3D12DynamicBuffer::SetIB(ID3D12GraphicsCommandList* pCmdList, DXGI_FORMAT IndexFormat)
{
    D3D12_INDEX_BUFFER_VIEW IBView = {};
    GetIBDesc(&IBView, IndexFormat);
    pCmdList->IASetIndexBuffer(&IBView);
}

void D3D12DynamicBuffer::SetVB(ID3D12GraphicsCommandList* pCmdList, UINT32 StartSlot)
{
    D3D12_VERTEX_BUFFER_VIEW VBView = {};
    GetVBDesc(&VBView);
    pCmdList->IASetVertexBuffers(StartSlot, 1, &VBView);
}

void D3D12DynamicBuffer::SetGraphicsRootCB(ID3D12GraphicsCommandList* pCmdList, UINT32 RootParameterIndex)
{
    ID3D12Resource* pBuffer = GetBuffer();
    pCmdList->SetGraphicsRootConstantBufferView(RootParameterIndex, pBuffer->GetGPUVirtualAddress());
}

void D3D12DynamicBuffer::SetComputeRootCB(ID3D12GraphicsCommandList* pCmdList, UINT32 RootParameterIndex)
{
    ID3D12Resource* pBuffer = GetBuffer();
    pCmdList->SetComputeRootConstantBufferView(RootParameterIndex, pBuffer->GetGPUVirtualAddress());
}

HRESULT DescriptorHeapSetManager::Initialize(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pQueue, ID3D12Fence* pGpuFence, UINT64* pCpuFence, D3D12_DESCRIPTOR_HEAP_TYPE HeapType, ID3D12DescriptorHeap* pHeap, UINT32 HeapOffset, UINT32 HeapStride, UINT32 MaxHandleCount)
{
    const UINT32 SetCount = MaxHandleCount / HeapStride;
    if (SetCount == 0)
    {
        return E_INVALIDARG;
    }

    m_pd3dDevice = pd3dDevice;
    m_pd3dDevice->AddRef();

    m_pQueue = pQueue;
    m_pQueue->AddRef();

    m_pGpuFence = pGpuFence;
    m_pGpuFence->AddRef();
    m_pCpuFence = pCpuFence;

    m_pHeap = pHeap;
    m_pHeap->AddRef();
    m_HeapDesc = m_pHeap->GetDesc();

    m_Type = HeapType;

    m_HandleSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(m_HeapDesc.Type);

    m_BaseHeapOffset = HeapOffset;
    m_HeapStride = HeapStride;

    for (UINT32 i = 0; i < SetCount; ++i)
    {
        HandleSet HS = {};
        HS.Fence = 0;
        HS.HeapOffset = HeapOffset + HeapStride * i;
        m_Sets.push_back(HS);
    }
    m_LastSetIndex = SetCount - 1;

    return S_OK;
}

void DescriptorHeapSetManager::Terminate()
{
    SAFE_RELEASE(m_pHeap);
    SAFE_RELEASE(m_pQueue);
    SAFE_RELEASE(m_pGpuFence);
    SAFE_RELEASE(m_pd3dDevice);
}

DescriptorHeapSetManager::ManagedBuffer* DescriptorHeapSetManager::AddBuffer()
{
    const UINT32 Size = (UINT32)m_Buffers.size();
    if (Size >= m_HeapStride)
    {
        return nullptr;
    }

    ManagedBuffer MB = {};
    m_Buffers.push_back(MB);
    return &m_Buffers[Size];
}

bool DescriptorHeapSetManager::AddDynamicCB(D3D12DynamicBuffer* pBuffer)
{
    if (m_Type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        return false;
    }
    ManagedBuffer* pMB = AddBuffer();
    if (pMB == nullptr)
    {
        return false;
    }
    pMB->DynamicBuffer = true;
    pMB->pResource = pBuffer;
    return true;
}

bool DescriptorHeapSetManager::AddEmptySlot()
{
    ManagedBuffer* pMB = AddBuffer();
    if (pMB == nullptr)
    {
        return false;
    }
    pMB->DynamicBuffer = false;
    pMB->pResource = nullptr;
    return true;
}

bool DescriptorHeapSetManager::AddStaticDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* pHandle)
{
    ManagedBuffer* pMB = AddBuffer();
    if (pMB == nullptr)
    {
        return false;
    }
    pMB->DynamicBuffer = false;

    const UINT32 IndexWithinSet = (UINT32)m_Buffers.size() - 1;
    const UINT32 HeapIndex = m_BaseHeapOffset + IndexWithinSet;
    pMB->OriginalHeapIndex = HeapIndex;

    CD3DX12_CPU_DESCRIPTOR_HANDLE Handle;
    Handle = m_pHeap->GetCPUDescriptorHandleForHeapStart();
    Handle.Offset(HeapIndex, m_HandleSize);
    *pHandle = Handle;
    return true;
}

bool DescriptorHeapSetManager::AddStaticSRV(ID3D12Resource* pResource, D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc)
{
    if (m_Type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        return false;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE Handle;
    bool Result = AddStaticDescriptor(&Handle);
    if (!Result)
    {
        return false;
    }
    m_pd3dDevice->CreateShaderResourceView(pResource, &SRVDesc, Handle);
    return true;
}

void DescriptorHeapSetManager::FinalizeDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE* pGpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE* pCpuHandle, bool DynamicBuffersAsStructuredBuffers)
{
    const UINT64 CurrentFence = *m_pCpuFence;

    HandleSet* pSet = FindUnusedSet();
    assert(pSet != nullptr);
    pSet->Fence = CurrentFence;

    const UINT32 BufferCount = (UINT32)m_Buffers.size();
    for (UINT32 i = 0; i < BufferCount; ++i)
    {
        UINT32 HeapIndex = pSet->HeapOffset + i;
        PrepareDescriptor(m_Buffers[i], HeapIndex, DynamicBuffersAsStructuredBuffers);
    }

    if (pGpuHandle != nullptr)
    {
        CD3DX12_GPU_DESCRIPTOR_HANDLE GpuHandle;
        GpuHandle = m_pHeap->GetGPUDescriptorHandleForHeapStart();
        GpuHandle.Offset(pSet->HeapOffset, m_HandleSize);
        *pGpuHandle = GpuHandle;
    }

    if (pCpuHandle != nullptr)
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle;
        CpuHandle = m_pHeap->GetCPUDescriptorHandleForHeapStart();
        CpuHandle.Offset(pSet->HeapOffset, m_HandleSize);
        *pCpuHandle = CpuHandle;
    }
}

void DescriptorHeapSetManager::PrepareDescriptor(const ManagedBuffer& MB, UINT32 HeapIndex, bool DynamicBuffersAsStructuredBuffers) const
{
    if (MB.pResource == nullptr)
    {
        return;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE Handle;
    Handle = m_pHeap->GetCPUDescriptorHandleForHeapStart();
    Handle.Offset(HeapIndex, m_HandleSize);

    if (MB.DynamicBuffer)
    {
        D3D12DynamicBuffer* pDynamic = (D3D12DynamicBuffer*)MB.pResource;
        switch (m_Type)
        {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            if (DynamicBuffersAsStructuredBuffers)
            {
                pDynamic->CreateSBView(Handle);
            }
            else
            {
                pDynamic->CreateCBView(Handle);
            }
            break;
        }
    }
    else
    {
        if (MB.OriginalHeapIndex != HeapIndex)
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE SrcHandle;
            SrcHandle = m_pHeap->GetCPUDescriptorHandleForHeapStart();
            SrcHandle.Offset(MB.OriginalHeapIndex, m_HandleSize);
            if (m_Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
            {
                m_pd3dDevice->CopyDescriptorsSimple(1, Handle, SrcHandle, m_HeapDesc.Type);
            }
            else
            {
                CopyMemory((void*)Handle.ptr, (void*)SrcHandle.ptr, m_HandleSize);
            }
        }
    }
}

DescriptorHeapSetManager::HandleSet* DescriptorHeapSetManager::FindUnusedSet()
{
    const UINT32 SetCount = (UINT32)m_Sets.size();
    UINT64 LowestFence = -1;
    while (true)
    {
        for (UINT32 i = 0; i < SetCount; ++i)
        {
            UINT32 SetIndex = (i + m_LastSetIndex) % SetCount;
            HandleSet& HS = m_Sets[SetIndex];
            if (HS.Fence <= m_pGpuFence->GetCompletedValue())
            {
                m_LastSetIndex = SetIndex;
                return &HS;
            }
            else
            {
                LowestFence = std::min(LowestFence, HS.Fence);
            }
        }
        UINT64 AdvanceFence = InterlockedIncrement64((LONG64*)m_pCpuFence);
        m_pQueue->Signal(m_pGpuFence, AdvanceFence);

        static bool s_GpuHangDetected = false;
        UINT32 Timeout = 500;
        while (m_pGpuFence->GetCompletedValue() <= LowestFence && Timeout > 0 && !s_GpuHangDetected)
        {
            Sleep(1);
            --Timeout;
        }

        if (Timeout == 0 || s_GpuHangDetected)
        {
            if (!s_GpuHangDetected)
            {
                DebugSpew("GPU is probably hung!\n");
                s_GpuHangDetected = true;
            }
            return &m_Sets[0];
        }
    }
}

HRESULT CpuGpuHeap::Initialize(ID3D12Device* pd3dDevice, ID3D12Fence* pGpuFence, UINT64* pCpuFence, SIZE_T HeapSizeBytesPerSlab, bool Readback, UINT32 MaxSlabCount)
{
    if (Readback)
    {
        return E_NOTIMPL;
    }

    Terminate();

    m_pd3dDevice = pd3dDevice;
    m_pd3dDevice->AddRef();

    m_pGpuFence = pGpuFence;
    m_pGpuFence->AddRef();
    m_pCpuFence = pCpuFence;

    m_MaxSlabCount = std::max(1U, MaxSlabCount);
    m_Readback = Readback;
    m_DefaultSlabSizeBytes = HeapSizeBytesPerSlab;
    m_CurrentSlabIndex = 0;

    const UINT32 StartSlabCount = std::min(2U, m_MaxSlabCount);
    for (UINT32 i = 0; i < StartSlabCount; ++i)
    {
        HRESULT hr = CreateNewSlab(nullptr, 0);
        if (FAILED(hr))
        {
            Terminate();
            return hr;
        }
    }

    m_hBlockEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    return S_OK;
}

void CpuGpuHeap::Terminate()
{
    UINT32 SlabCount = (UINT32)m_Slabs.size();
    for (UINT32 i = 0; i < SlabCount; ++i)
    {
        HeapSlab& HS = m_Slabs[i];
        HS.pHeap->Unmap(0, nullptr);
        SAFE_RELEASE(HS.pHeap);
    }
    m_Slabs.clear();

    CloseHandle(m_hBlockEvent);
    SAFE_RELEASE(m_pGpuFence);
    SAFE_RELEASE(m_pd3dDevice);
}

HRESULT CpuGpuHeap::CreateNewSlab(HeapSlab** ppSlab, SIZE_T RequestedSizeBytes)
{
    HeapSlab NewSlab = {};

    D3D12_HEAP_PROPERTIES HeapProperties = {};
    HeapProperties.Type = m_Readback ? D3D12_HEAP_TYPE_READBACK : D3D12_HEAP_TYPE_UPLOAD;
    HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProperties.VisibleNodeMask = D3D12XBOX_NODE_MASK;
    HeapProperties.CreationNodeMask = D3D12XBOX_NODE_MASK;

    D3D12_RESOURCE_DESC BufferDesc = {};
    BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    BufferDesc.Alignment = 0;
    BufferDesc.Width = std::max(m_DefaultSlabSizeBytes, RequestedSizeBytes);
    BufferDesc.Height = 1;
    BufferDesc.DepthOrArraySize = 1;
    BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    BufferDesc.MipLevels = 1;
    BufferDesc.SampleDesc.Count = 1;
    BufferDesc.SampleDesc.Quality = 0;
    BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    BufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

    D3D12_RESOURCE_STATES InitialUsage = m_Readback ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;

    HRESULT hr = m_pd3dDevice->CreateCommittedResource(&HeapProperties,
                                                       D3D12_HEAP_FLAG_NONE,
                                                       &BufferDesc,
                                                       InitialUsage,
                                                       nullptr,
                                                       __uuidof(ID3D12Resource),
                                                       (void**)&NewSlab.pHeap);
    if (FAILED(hr))
    {
        return hr;
    }

    NewSlab.pHeap->Map(0, nullptr, (void**)&NewSlab.pBase);
    NewSlab.CurrentOffset = 0;
    NewSlab.Fence = 0;
    NewSlab.SizeBytes = BufferDesc.Width;

    const UINT32 SlabIndex = (UINT32)m_Slabs.size();
    m_Slabs.push_back(NewSlab);

    if (m_strName[0] != L'\0')
    {
        WCHAR strSlabName[64];
        swprintf_s(strSlabName, L"%s Slab %u", m_strName, SlabIndex);
        NewSlab.pHeap->SetName(strSlabName);
    }

    if (ppSlab != nullptr)
    {
        *ppSlab = &m_Slabs[SlabIndex];
    }

    return S_OK;
}

HRESULT CpuGpuHeap::FindAvailableSlab(HeapSlab** ppSlab, SIZE_T SizeBytes, SIZE_T AlignmentBytes)
{
    const UINT64 CurrentFence = *m_pCpuFence;
    const UINT64 LastCompletedFence = m_pGpuFence->GetCompletedValue();
    const UINT32 SlabCount = (UINT32)m_Slabs.size();
    for (UINT32 i = 0; i < SlabCount; ++i)
    {
        UINT32 Index = ( m_CurrentSlabIndex + i ) % SlabCount;
        HeapSlab& S = m_Slabs[Index];

        if (S.Fence <= LastCompletedFence)
        {
            // Recycle this slab:
            S.Fence = 0;
            S.CurrentOffset = 0;
        }
        if (S.Fence == CurrentFence || S.Fence == 0)
        {
            // Slab is available; try to fit allocation:
            const SIZE_T EndPos = NextMultiple(S.CurrentOffset, AlignmentBytes) + SizeBytes;
            if (EndPos <= S.SizeBytes)
            {
                m_CurrentSlabIndex = Index;
                *ppSlab = &S;
                return S_OK;
            }
        }
    }
    return E_OUTOFMEMORY;
}

HRESULT CpuGpuHeap::Allocate(SIZE_T SizeBytes, SIZE_T AlignmentBytes, HeapSlab** ppSlab, SIZE_T* pOffsetWithinSlab)
{
    HeapSlab* pSlab = nullptr;
    HRESULT hr = FindAvailableSlab(&pSlab, SizeBytes, AlignmentBytes);

    // If we couldn't find an available slab, try to create a new slab if we haven't hit the slab limit:
    if (FAILED(hr) && (UINT32)m_Slabs.size() < m_MaxSlabCount)
    {
        hr = CreateNewSlab(&pSlab, SizeBytes);
        if (SUCCEEDED(hr))
        { 
            m_CurrentSlabIndex = (UINT32)m_Slabs.size() - 1;
        }
    }

    // If we couldn't create a new slab, then wait for a slab to be available:
    if (FAILED(hr))
    {
        UINT64 LowestFence = -1;
        UINT32 SlabIndex = 0;
        const UINT32 SlabCount = (UINT32)m_Slabs.size();
        for (UINT32 i = 0; i < SlabCount; ++i)
        {
            const HeapSlab& S = m_Slabs[i];
            if (S.Fence < LowestFence)
            {
                LowestFence = S.Fence;
                SlabIndex = i;
            }
        }
        m_pGpuFence->SetEventOnCompletion(LowestFence, m_hBlockEvent);
        WaitForSingleObject(m_hBlockEvent, INFINITE);
        m_CurrentSlabIndex = SlabIndex;
        hr = FindAvailableSlab(&pSlab, SizeBytes, AlignmentBytes);
    }

    if (SUCCEEDED(hr))
    {
        assert(pSlab != nullptr);
        pSlab->Fence = *m_pCpuFence;

        SIZE_T ReturnOffset = NextMultiple(pSlab->CurrentOffset, AlignmentBytes);
        SIZE_T NewOffset = ReturnOffset + SizeBytes;
        assert(NewOffset <= pSlab->SizeBytes);
        pSlab->CurrentOffset = NewOffset;

        *ppSlab = pSlab;
        *pOffsetWithinSlab = ReturnOffset;
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

HRESULT CpuGpuHeap::AllocateBufferInHeap(SIZE_T SizeBytes, BYTE** ppData, D3D12_GPU_VIRTUAL_ADDRESS* pVA, UINT32 AlignmentBytes)
{
    HeapSlab* pSlab = nullptr;
    SIZE_T DestOffset = 0;
    HRESULT hr = Allocate(SizeBytes, AlignmentBytes, &pSlab, &DestOffset);
    if (FAILED(hr))
    {
        return hr;
    }

    assert(pSlab != nullptr && pSlab->pBase != nullptr);
    *ppData = pSlab->pBase + DestOffset;
    *pVA = pSlab->pHeap->GetGPUVirtualAddress() + DestOffset;
    return S_OK;
}

HRESULT CpuGpuHeap::CopyBufferDataToHeap(const BYTE* pData, SIZE_T SizeBytes, ID3D12Resource** ppHeap, SIZE_T& DestOffsetWithinHeap)
{
    HeapSlab* pSlab = nullptr;
    HRESULT hr = Allocate(SizeBytes, 16, &pSlab, &DestOffsetWithinHeap);
    if (FAILED(hr))
    {
        return hr;
    }

    assert(pSlab != nullptr && pSlab->pBase != nullptr);
    *ppHeap = pSlab->pHeap;
    BYTE* pDstRow = pSlab->pBase + DestOffsetWithinHeap;
    const BYTE* pSrcRow = pData;
    memcpy(pDstRow, pSrcRow, SizeBytes);

    return S_OK;
}

HRESULT CpuGpuHeap::CopyBufferDataToDefaultBuffer(const BYTE* pData, SIZE_T SizeBytes, ID3D12GraphicsCommandList* pCommandList, ID3D12Resource* pDefaultResource)
{
    SIZE_T DestOffsetWithinHeap = 0;
    ID3D12Resource* pHeapResource = nullptr;
    HRESULT hr = CopyBufferDataToHeap(pData, SizeBytes, &pHeapResource, DestOffsetWithinHeap);
    if (FAILED(hr))
    {
        return hr;
    }

    assert(pHeapResource != nullptr);

    pCommandList->CopyBufferRegion(pDefaultResource,
                                   0,
                                   pHeapResource,
                                   DestOffsetWithinHeap,
                                   SizeBytes);

    return S_OK;
}

#if defined(_XBOX_ONE) && defined(_TITLE)
HRESULT CpuGpuHeap::CopyBufferDataToDefaultBuffer(const BYTE* pData, SIZE_T SizeBytes, ID3D12XboxDmaCommandList* pCommandList, ID3D12Resource* pDefaultResource)
{
    SIZE_T DestOffsetWithinHeap = 0;
    ID3D12Resource* pHeapResource = nullptr;
    HRESULT hr = CopyBufferDataToHeap(pData, SizeBytes, &pHeapResource, DestOffsetWithinHeap);
    if (FAILED(hr))
    {
        return hr;
    }

    assert(pHeapResource != nullptr);

    pCommandList->CopyBufferRegion(pDefaultResource,
        0,
        pHeapResource,
        DestOffsetWithinHeap,
        SizeBytes);

    return S_OK;
}
#endif

HRESULT CpuGpuHeap::CopyTextureSubresourceToHeap(const BYTE* pData, UINT32 DataPitchBytes, const D3D12_SUBRESOURCE_FOOTPRINT& SubresourceDesc, ID3D12Resource** ppHeap, SIZE_T& DestOffsetWithinHeap)
{
    const UINT32 RowPitchBytes = SubresourceDesc.RowPitch;
    if (DataPitchBytes == 0)
    {
        DataPitchBytes = RowPitchBytes;
    }
    assert (DataPitchBytes <= RowPitchBytes);
    UINT32 HeightRows = SubresourceDesc.Height;
    if (IsBlockCompressedFormat(SubresourceDesc.Format))
    {
        HeightRows = NextMultiple(HeightRows, 4U) >> 2;
    }
    if (SubresourceDesc.Depth > 0)
    {
        HeightRows *= SubresourceDesc.Depth;
    }
    const SIZE_T LinearSizeBytes = RowPitchBytes * HeightRows;
    HeapSlab* pSlab = nullptr;
//    HRESULT hr = Allocate(LinearSizeBytes, D3D12_TEXTURE_OFFSET_ALIGNMENT, &pSlab, &DestOffsetWithinHeap);
    HRESULT hr = Allocate(LinearSizeBytes, 8192, &pSlab, &DestOffsetWithinHeap);
    if (FAILED(hr))
    {
        return hr;
    }

    assert(pSlab != nullptr && pSlab->pBase != nullptr);
    *ppHeap = pSlab->pHeap;
    BYTE* pDstRow = pSlab->pBase + DestOffsetWithinHeap;
    const BYTE* pSrcRow = pData;

    if (RowPitchBytes == DataPitchBytes)
    {
        memcpy(pDstRow, pSrcRow, DataPitchBytes * HeightRows);
    }
    else
    {
        for (UINT32 i = 0; i < HeightRows; ++i)
        {
            memcpy(pDstRow, pSrcRow, DataPitchBytes);
            pDstRow += RowPitchBytes;
            pSrcRow += DataPitchBytes;
        }
    }

    return S_OK;
}

HRESULT CpuGpuHeap::CopyTextureSubresourceToDefaultTexture(const BYTE* pData, UINT32 DataPitchBytes, const D3D12_SUBRESOURCE_FOOTPRINT& SubresourceDesc, ID3D12GraphicsCommandList* pCommandList, ID3D12Resource* pDefaultResource, UINT32 DestSubresource, UINT32 DestX, UINT32 DestY, UINT32 DestZ, const D3D12_BOX* pSrcBox)
{
    SIZE_T DestOffsetWithinHeap = 0;
    ID3D12Resource* pHeapResource = nullptr;
    HRESULT hr = CopyTextureSubresourceToHeap(pData, DataPitchBytes, SubresourceDesc, &pHeapResource, DestOffsetWithinHeap);
    if (FAILED(hr))
    {
        return hr;
    }

    assert(pHeapResource != nullptr);

    D3D12_TEXTURE_COPY_LOCATION Src;

    Src.pResource = pHeapResource;
    Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    Src.PlacedFootprint.Offset = DestOffsetWithinHeap;
    Src.PlacedFootprint.Footprint = SubresourceDesc;

    assert((Src.PlacedFootprint.Footprint.RowPitch % D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT) == 0);

    D3D12_TEXTURE_COPY_LOCATION Dst;

    Dst.pResource = pDefaultResource;
    Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    Dst.SubresourceIndex = DestSubresource;

    pCommandList->CopyTextureRegion(&Dst,
                                    DestX, DestY, DestZ,
                                    &Src,
                                    pSrcBox);

    return S_OK;
}

#if defined(_XBOX_ONE) && defined(_TITLE)
HRESULT CpuGpuHeap::CopyTextureSubresourceToDefaultTexture(const BYTE* pData, UINT32 DataPitchBytes, const D3D12_SUBRESOURCE_FOOTPRINT& SubresourceDesc, ID3D12XboxDmaCommandList* pCommandList, ID3D12Resource* pDefaultResource, UINT32 DestSubresource, UINT32 DestX, UINT32 DestY, UINT32 DestZ, const D3D12_BOX* pSrcBox)
{
    SIZE_T DestOffsetWithinHeap = 0;
    ID3D12Resource* pHeapResource = nullptr;
    HRESULT hr = CopyTextureSubresourceToHeap(pData, DataPitchBytes, SubresourceDesc, &pHeapResource, DestOffsetWithinHeap);
    if (FAILED(hr))
    {
        return hr;
    }

    assert(pHeapResource != nullptr);

    D3D12_TEXTURE_COPY_LOCATION Src;

    Src.pResource = pHeapResource;
    Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    Src.PlacedFootprint.Offset = DestOffsetWithinHeap;
    Src.PlacedFootprint.Footprint = SubresourceDesc;

    assert((Src.PlacedFootprint.Footprint.RowPitch % D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT) == 0);

    D3D12_TEXTURE_COPY_LOCATION Dst;

    Dst.pResource = pDefaultResource;
    Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    Dst.SubresourceIndex = DestSubresource;

    pCommandList->CopyTextureRegion(&Dst,
        DestX, DestY, DestZ,
        &Src,
        pSrcBox);

    return S_OK;
}
#endif

HRESULT DDSLoader12::Initialize(ID3D12Device* pd3dDevice, ID3D12CommandAllocator* pCmdAlloc, CpuGpuHeap* pUploadHeap)
{
    Terminate();

    m_pd3dDevice = pd3dDevice;
    m_pd3dDevice->AddRef();

    m_pCmdAlloc = pCmdAlloc;
    m_pCmdAlloc->AddRef();

    m_pUploadHeap = pUploadHeap;

    return S_OK;
}

void DDSLoader12::Terminate()
{
    assert(m_pCmdList == nullptr);

    m_pUploadHeap = nullptr;
    SAFE_RELEASE(m_pCmdAlloc);
    SAFE_RELEASE(m_pd3dDevice);
}

void DDSLoader12::BeginLoading(ID3D12GraphicsCommandList* pExistingCmdList)
{
    if (pExistingCmdList != nullptr)
    {
        m_pCmdList = pExistingCmdList;
        m_pCmdList->AddRef();
    }
    else
    {
        HRESULT hr = m_pd3dDevice->CreateCommandList(D3D12XBOX_NODE_MASK, 
                                                     D3D12_COMMAND_LIST_TYPE_DIRECT, 
                                                     m_pCmdAlloc, 
                                                     nullptr, 
                                                     __uuidof(ID3D12GraphicsCommandList), 
                                                     (void**)&m_pCmdList);
        assert(SUCCEEDED(hr));
    }
}

void DDSLoader12::FinishLoading(ID3D12CommandQueue* pQueue)
{
    assert(m_pCmdList != nullptr);

    m_pCmdList->ResourceBarrier((UINT)m_FinalDescs.size(), &m_FinalDescs.front());
    m_FinalDescs.clear();

    if (pQueue != nullptr)
    {
        m_pCmdList->Close();
        pQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&m_pCmdList);
    }
    SAFE_RELEASE(m_pCmdList);
}

//--------------------------------------------------------------------------------------
// Return the BPE for a particular format
//--------------------------------------------------------------------------------------
static UINT32 BitsPerElement(_In_ DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 8;
        break;

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
        return 16;
        break;

    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_YUY2:
        return 4;
        break;

    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
        return 8;
        break;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
        return 2;
        break;

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
        return 4;
        break;

#if defined(_XBOX_ONE) && defined(_TITLE)

    case DXGI_FORMAT_D16_UNORM_S8_UINT:
    case DXGI_FORMAT_R16_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X16_TYPELESS_G8_UINT:
        return 4;
        break;

#endif
    }

    return 1;
}

//--------------------------------------------------------------------------------------
// Get surface information for a particular format
//--------------------------------------------------------------------------------------
void GetSurfaceInfo(
    _In_ size_t width,
    _In_ size_t height,
    _In_ DXGI_FORMAT fmt,
    _Out_opt_ size_t* outNumBytes,
    _Out_opt_ size_t* outRowBytes,
    _Out_opt_ size_t* outNumRows)
{
    size_t numBytes = 0;
    size_t rowBytes = 0;
    size_t numRows = 0;

    bool packed = false;
    bool planar = false;
    size_t bpe = BitsPerElement(fmt);

    if (IsCompressed(fmt))
    {
        size_t numBlocksWide = 0;
        if (width > 0)
        {
            numBlocksWide = std::max<size_t >(1, (width + 3) / 4);
        }
        size_t numBlocksHigh = 0;
        if (height > 0)
        {
            numBlocksHigh = std::max< size_t >(1, (height + 3) / 4);
        }
        rowBytes = numBlocksWide * bpe;
        numRows = numBlocksHigh;
        numBytes = rowBytes * numBlocksHigh;
    }
    else if (packed)
    {
        rowBytes = ((width + 1) >> 1) * bpe;
        numRows = height;
        numBytes = rowBytes * height;
    }
    else if (fmt == DXGI_FORMAT_NV11)
    {
        rowBytes = ((width + 3) >> 2) * 4;
        numRows = height * 2; // Direct3D makes this simplifying assumption, although it is larger than the 4:1:1 data
        numBytes = rowBytes * numRows;
    }
    else if (planar)
    {
        rowBytes = ((width + 1) >> 1) * bpe;
        numBytes = (rowBytes * height) + ((rowBytes * height + 1) >> 1);
        numRows = height + ((height + 1) >> 1);
    }
    else
    {
        size_t bpp = BitsPerPixel(fmt);
        rowBytes = (width * bpp + 7) / 8; // round up to nearest byte
        numRows = height;
        numBytes = rowBytes * height;
    }

    if (outNumBytes)
    {
        *outNumBytes = numBytes;
    }
    if (outRowBytes)
    {
        *outRowBytes = rowBytes;
    }
    if (outNumRows)
    {
        *outNumRows = numRows;
    }
}

_Use_decl_annotations_
static HRESULT FillInitData(
    UINT width,
    UINT height,
    UINT depth,
    UINT16 mipCount,
    UINT16 arraySize,
    DXGI_FORMAT format,
    size_t maxsize,
    size_t bitSize,
    const uint8_t* bitData,
    UINT16* pSkipMip,
    D3D12_SUBRESOURCE_DATA* initData)
{
    if (!bitData || !initData)
    {
        return E_POINTER;
    }

    *pSkipMip = 0;
    size_t numBytes = 0;
    size_t rowBytes = 0;
    const uint8_t* pSrcBits = bitData;
    const uint8_t* pEndBits = bitData + bitSize;

    UINT minW = 1;
    UINT minH = 1;

    // ToDo: hack, fix it
    if (IsCompressed(format))
    {
        minW = minH = 4;
    }

    size_t index = 0;
    for (size_t j = 0; j < arraySize; j++)
    {
        for (size_t i = 0; i < mipCount; i++)
        {
            UINT w = std::max<UINT>(width >> i, minW);
            UINT h = std::max<UINT>(height >> i, minH);
            UINT d = std::max<UINT>(depth >> i, 1u);

            GetSurfaceInfo(w, h, format, &numBytes, &rowBytes, nullptr);

            if ((mipCount <= 1) || !maxsize || (w <= maxsize && h <= maxsize && d <= maxsize))
            {
                assert(index < static_cast<size_t>(mipCount * arraySize));
                _Analysis_assume_(index < static_cast<size_t>(mipCount * arraySize));
                initData[index].pData = reinterpret_cast< const void* >(pSrcBits);
                initData[index].RowPitch = static_cast< UINT >(rowBytes);
                initData[index].SlicePitch = static_cast< UINT >(numBytes);
                ++index;
            }
            else if (!j)
            {
                // Count number of skipped mipmaps (first item only)
                ++(*pSkipMip);
            }

            if (pSrcBits + (numBytes * d) > pEndBits)
            {
                return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
            }

            pSrcBits += numBytes * d;
        }
    }

    return (index > 0) ? S_OK : E_FAIL;
}


//--------------------------------------------------------------------------------------
#if defined( _XBOX_ONE ) && defined( _TITLE )
#define TEXTURE_DATA_PITCH_ALIGNMENT D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT
#else
#define TEXTURE_DATA_PITCH_ALIGNMENT D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
#endif

static HRESULT CreateD3DResources(
    _In_ ID3D12Device* pDevice,
    _In_ ID3D12GraphicsCommandList* pCmdList,
    _In_ D3D12_RESOURCE_DIMENSION resDim,
    _In_ UINT width,
    _In_ UINT height,
    _In_ UINT depth,
    _In_ UINT16 mipCount,
    _In_ UINT16 arraySize,
    _In_ DXGI_FORMAT format,
    _In_ bool isCubeMap,
    _In_ D3D12_RESOURCE_FLAGS miscFlags,
    _In_ CpuGpuHeap* pUploadHeap,
    _In_reads_opt_(mipCount * arraySize) D3D12_SUBRESOURCE_DATA* initData,
    _Outptr_opt_ D3D12_CPU_DESCRIPTOR_HANDLE descHandle,
    _Outptr_opt_ ID3D12Resource** ppTexture)
{
    HRESULT hr = S_OK;

    if (!pDevice || ppTexture == nullptr)
    {
        return E_POINTER;
    }

    ID3D12Resource* pTex = nullptr;
    const D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC texDesc;

    switch (resDim)
    {
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        return E_NOTIMPL;
        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        texDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, arraySize, mipCount, 1, 0, miscFlags);
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        texDesc = CD3DX12_RESOURCE_DESC::Tex3D(format, width, height, static_cast<UINT16>(depth), mipCount, miscFlags);
        break;
    }

    hr = pDevice->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        __uuidof(ID3D12Resource),
        reinterpret_cast<void**>(&pTex));
    if (FAILED(hr))
    {
        return hr;
    }

    D3D12_SUBRESOURCE_FOOTPRINT pitchSrcDesc = CD3DX12_SUBRESOURCE_FOOTPRINT(format, width, height, depth, 0);
    UINT subresourceIndex = 0;
    UINT minW = 1;
    UINT minH = 1;

    // ToDo: hack, fix it
    if (IsCompressed(format))
    {
        minW = minH = 4;
    }

    for (UINT arr = 0; arr < arraySize; ++arr)
    {
        for (UINT i = 0; i < mipCount; ++i)
        {
            pitchSrcDesc.RowPitch = static_cast<UINT>(initData[subresourceIndex].RowPitch);
            pitchSrcDesc.Width = std::max<UINT>(width >> i, minW);
            pitchSrcDesc.Height = std::max<UINT>(height >> i, minH);
            pitchSrcDesc.Depth = std::max<UINT>(depth >> i, 1u);
            if (IsCompressed(format))
            {
                pitchSrcDesc.Width = static_cast<UINT32>(NextMultiple<UINT>(pitchSrcDesc.Width, 4));
                pitchSrcDesc.Height = static_cast<UINT32>(NextMultiple<UINT>(pitchSrcDesc.Height, 4));
            }

            D3D12_SUBRESOURCE_FOOTPRINT pitchDestDesc = pitchSrcDesc;
            pitchDestDesc.RowPitch = static_cast<UINT32>(NextMultiple<UINT>(pitchDestDesc.RowPitch, TEXTURE_DATA_PITCH_ALIGNMENT));

            hr = pUploadHeap->CopyTextureSubresourceToDefaultTexture(
                reinterpret_cast<const BYTE*>(initData[subresourceIndex].pData), pitchSrcDesc.RowPitch, pitchDestDesc, pCmdList, pTex, subresourceIndex);
            if (FAILED(hr))
            {
                return hr;
            }

            subresourceIndex++;
        }
    }


    ResourceBarrier(pCmdList, pTex, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    D3D12_SHADER_RESOURCE_VIEW_DESC descSRV = {};
    descSRV.Format = format;
    descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (isCubeMap)
    {
        descSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        descSRV.TextureCube.MipLevels = mipCount;
    }
    else if (arraySize > 1)
    {
        descSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        descSRV.Texture2DArray.ArraySize = arraySize;
        descSRV.Texture2DArray.MipLevels = mipCount;
    }
    else if (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        descSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        descSRV.Texture3D.MipLevels = mipCount;
    }
    else
    {
        descSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        descSRV.Texture2D.MipLevels = mipCount;
    }

    if (descHandle.ptr != 0)
    {
        pDevice->CreateShaderResourceView(pTex, &descSRV, descHandle);
    }

    if (ppTexture != nullptr)
    {
        *ppTexture = pTex;
    }
    else
    {
        SAFE_RELEASE(pTex);
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DDSLoader12::LoadDDSFile(
    const WCHAR* strFileName,
    D3D12_CPU_DESCRIPTOR_HANDLE DescHandle,
    ID3D12Resource** ppTexture,
    D3D12_RESOURCE_FLAGS miscFlags)
{
    BYTE* pDDSFile = nullptr;
    UINT32 DDSSizeBytes = 0;
    if (FAILED(LoadFile(strFileName, (void**)&pDDSFile, &DDSSizeBytes)))
    {
        return E_FAIL;
    }

    HRESULT hr = LoadDDSFromMemory(pDDSFile, DDSSizeBytes, DescHandle, ppTexture, miscFlags);

    UnloadFile(pDDSFile);

    return hr;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DDSLoader12::LoadDDSFromMemory(
    const uint8_t* ddsData, size_t ddsDataSize,
    D3D12_CPU_DESCRIPTOR_HANDLE DescHandle,
    ID3D12Resource** ppTexture,
    D3D12_RESOURCE_FLAGS miscFlags)
{
    assert(m_pCmdList != nullptr);

    DWORD Magic = *(DWORD*)ddsData;
    if (Magic != ' SDD')
    {
        return E_FAIL;
    }

    const DDS_HEADER* pHeader = reinterpret_cast<const DDS_HEADER*>(ddsData + 4);

    // Check for DX10 extension
    bool bDXT10Header = false;
    if ((pHeader->ddspf.flags & DDS_FOURCC) &&
        (pHeader->ddspf.fourCC == MAKEFOURCC('D', 'X', '1', '0')))
    {
        // Must be long enough for both headers and magic value
        if (ddsDataSize < (sizeof(DDS_HEADER) + sizeof(uint32_t) + sizeof(DDS_HEADER_DXT10)))
        {
            return E_FAIL;
        }

        bDXT10Header = true;
    }

    // setup the pointers in the process request
    ptrdiff_t offset = sizeof(uint32_t) + sizeof(DDS_HEADER)
        + (bDXT10Header ? sizeof(DDS_HEADER_DXT10) : 0);
    const BYTE* pBits = ddsData + offset;
    size_t bitSize = ddsDataSize - offset;

    const UINT32 width = pHeader->width;
    UINT32 height = pHeader->height;
    UINT32 depth = pHeader->depth;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

    D3D12_RESOURCE_DIMENSION resDim = D3D12_RESOURCE_DIMENSION_UNKNOWN;
    UINT16 arraySize = 1;
    bool isCubeMap = false;

    UINT16 mipCount = static_cast<UINT16>(pHeader->mipMapCount);

    if (mipCount == 0)
    {
        mipCount = 1;
    }

    // Check for DX10 extension
    if (bDXT10Header)
    {
        const DDS_HEADER_DXT10* pHeader10 = reinterpret_cast<const DDS_HEADER_DXT10*>(ddsData + sizeof(uint32_t) + sizeof(DDS_HEADER));

        arraySize = static_cast< UINT16 >(pHeader10->arraySize);
        if (arraySize == 0)
        {
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        switch (pHeader10->dxgiFormat)
        {
        case DXGI_FORMAT_AI44:
        case DXGI_FORMAT_IA44:
        case DXGI_FORMAT_P8:
        case DXGI_FORMAT_A8P8:
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

        default:
            if (BitsPerPixel(pHeader10->dxgiFormat) == 0)
            {
                return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }
        }

        format = pHeader10->dxgiFormat;
        const UINT D3D11_RESOURCE_MISC_TEXTURECUBE = 0x4L;
        switch (pHeader10->resourceDimension)
        {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            // D3DX writes 1D textures with a fixed Height of 1
            if ((pHeader->flags & DDS_HEIGHT) && height != 1)
            {
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }
            height = depth = 1;
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (pHeader10->miscFlag & D3D11_RESOURCE_MISC_TEXTURECUBE)
            {
                arraySize *= 6;
                isCubeMap = true;
            }
            depth = 1;
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            if (!(pHeader->flags & DDS_HEADER_FLAGS_VOLUME))
            {
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            if (arraySize > 1)
            {
                return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }
            break;

        default:
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }

        resDim = static_cast< D3D12_RESOURCE_DIMENSION >(pHeader10->resourceDimension);
    }
    else
    {
        format = GetDXGIFormat(pHeader->ddspf);
        if (format == DXGI_FORMAT_UNKNOWN)
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }

        if (pHeader->flags & DDS_HEADER_FLAGS_VOLUME)
        {
            resDim = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        }
        else
        {
            if (pHeader->caps2 & DDS_CUBEMAP)
            {
                // We require all six faces to be defined
                if ((pHeader->caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
                {
                    return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                }

                arraySize = 6;
                isCubeMap = true;
            }

            depth = 1;
            resDim = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

            // Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
        }

        assert(BitsPerPixel(format) != 0);
    }

    switch (resDim)
    {
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (isCubeMap)
        {
            // This is the right bound because we set arraySize to (NumCubes * 6) above
            if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
                (width > D3D12_REQ_TEXTURECUBE_DIMENSION) ||
                (height > D3D12_REQ_TEXTURECUBE_DIMENSION))
            {
                return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }
        }
        else if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
            (width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) ||
            (height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION))
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
        break;
    default:
        // ToDo
        break;
    }

    // Create the texture
    std::unique_ptr<D3D12_SUBRESOURCE_DATA[]> initData(new (std::nothrow) D3D12_SUBRESOURCE_DATA[mipCount * arraySize]);
    if (!initData)
    {
        return E_OUTOFMEMORY;
    }

    UINT16 skipMip = 0;
    size_t maxSize = 0;

    HRESULT hr = FillInitData(width, height, depth, mipCount, arraySize, format, maxSize,
        bitSize, pBits, &skipMip, initData.get());
    if (FAILED(hr))
    {
        return hr;
    }

    hr = CreateD3DResources(m_pd3dDevice, m_pCmdList, resDim, width, height, depth, mipCount - skipMip,
        arraySize, format, isCubeMap, miscFlags, m_pUploadHeap, initData.get(), DescHandle, ppTexture);

    return hr;
}

/*HRESULT DDSLoader12::LoadDDSFile(const WCHAR* strFileName, D3D12_CPU_DESCRIPTOR_HANDLE DescHandle, ID3D12Resource** ppTexture)
{
    assert(m_pCmdList != nullptr);

    BYTE* pDDSFile = nullptr;
    UINT32 DDSSizeBytes = 0;
    HRESULT hr = LoadFile(strFileName, (void**)&pDDSFile, &DDSSizeBytes);
    if (FAILED(hr))
    {
        return hr;
    }

    struct DDS_PIXELFORMAT {
        DWORD dwSize;
        DWORD dwFlags;
        DWORD dwFourCC;
        DWORD dwRGBBitCount;
        DWORD dwRBitMask;
        DWORD dwGBitMask;
        DWORD dwBBitMask;
        DWORD dwABitMask;
    };

    struct DDS_HEADER {
        DWORD           dwSize;
        DWORD           dwFlags;
        DWORD           dwHeight;
        DWORD           dwWidth;
        DWORD           dwPitchOrLinearSize;
        DWORD           dwDepth;
        DWORD           dwMipMapCount;
        DWORD           dwReserved1[11];
        DDS_PIXELFORMAT ddspf;
        DWORD           dwCaps;
        DWORD           dwCaps2;
        DWORD           dwCaps3;
        DWORD           dwCaps4;
        DWORD           dwReserved2;
    };

    struct DDS_HEADER_DXT10 {
        DXGI_FORMAT              dxgiFormat;
        D3D12_RESOURCE_DIMENSION resourceDimension;
        UINT                     miscFlag;
        UINT                     arraySize;
        UINT                     miscFlags2;
    };

    DWORD Magic = *(DWORD*)pDDSFile;
    if (Magic != ' SDD')
    {
        UnloadFile(pDDSFile);
        return E_FAIL;
    }

    const DDS_HEADER* pHeader = (const DDS_HEADER*)(pDDSFile + 4);
    const BYTE* pBits = pDDSFile + sizeof(DDS_HEADER) + 4;

    const UINT32 Width = pHeader->dwWidth;
    const UINT32 Height = pHeader->dwHeight;
    UINT32 RowPitchBytes = pHeader->dwPitchOrLinearSize;
    DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    if (pHeader->ddspf.dwFourCC == 'DX10')
    {
        const DDS_HEADER_DXT10* pHeader10 = (const DDS_HEADER_DXT10*)(pDDSFile + 4 + sizeof(DDS_HEADER));
        Format = pHeader10->dxgiFormat;
        pBits += sizeof(DDS_HEADER_DXT10);
    }
    else if (pHeader->ddspf.dwFourCC == '1TXD')
    {
        Format = DXGI_FORMAT_BC1_UNORM;
        RowPitchBytes = max((Width >> 2), 1) * 8;
    }

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
    TexDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;

    ID3D12Resource* pTex2D = nullptr;
    hr = CreateDefaultResource(m_pd3dDevice, 
                               &TexDesc, 
                               IsCommonStatePromotionEnabled() ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_COPY_DEST,
                               (void**)&pTex2D);
    if (FAILED(hr))
    {
        UnloadFile(pDDSFile);
        return hr;
    }

    WCHAR strResourceName[128];
    swprintf_s(strResourceName, L"DDS File %s", strFileName);
    pTex2D->SetName(strResourceName);

    D3D12_SUBRESOURCE_FOOTPRINT PitchDesc = {};
    PitchDesc.Width = (UINT32)TexDesc.Width;
    PitchDesc.Height = TexDesc.Height;
    PitchDesc.Depth = 1;
    PitchDesc.Format = TexDesc.Format;
    PitchDesc.RowPitch = NextMultiple(RowPitchBytes, (UINT32)D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT);

    m_pUploadHeap->CopyTextureSubresourceToDefaultTexture(pBits, RowPitchBytes, PitchDesc, m_pCmdList, pTex2D, 0);

    UnloadFile(pDDSFile);

    D3D12_RESOURCE_BARRIER barrierDesc = {};
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierDesc.Transition.pResource = pTex2D;
    barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    m_FinalDescs.push_back(barrierDesc);

    m_pd3dDevice->CreateShaderResourceView(pTex2D, nullptr, DescHandle);

    if (ppTexture != nullptr)
    {
        *ppTexture = pTex2D;
    }
    else
    {
        SAFE_RELEASE(pTex2D);
    }

    return hr;
}*/

HRESULT CreateDefaultBuffer(ID3D12Device* pd3dDevice, 
                            CpuGpuHeap* pUploadHeap, 
                            ID3D12GraphicsCommandList* pCmdList, 
                            SIZE_T SizeBytes, 
                            const void* pData, 
                            ID3D12Resource** ppBuffer,
                            D3D12_RESOURCE_FLAGS flags)
{
    assert(pd3dDevice != nullptr && pUploadHeap != nullptr && pCmdList != nullptr);

    if (SizeBytes == 0)
    {
        return E_INVALIDARG;
    }

    D3D12_RESOURCE_STATES InitialUsage = D3D12_RESOURCE_STATE_COMMON;
    
    D3D12_RESOURCE_DESC BufferDesc = {};
    BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    BufferDesc.Width = SizeBytes;
    BufferDesc.Height = 1;
    BufferDesc.DepthOrArraySize = 1;
    BufferDesc.MipLevels = 1;
    BufferDesc.SampleDesc.Count = 1;
    BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    BufferDesc.Flags = flags;
    ID3D12Resource* pBuffer = nullptr;
    HRESULT hr = CreateDefaultResource(pd3dDevice, &BufferDesc, InitialUsage, (void**)&pBuffer);
    if (FAILED(hr))
    {
        return hr;
    }

    if (pData != nullptr)
    {
        const bool IsCSP = IsCommonStatePromotionEnabled();

        if (!IsCSP)
        {
            ResourceBarrier(pCmdList, pBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
        }
        hr = pUploadHeap->CopyBufferDataToDefaultBuffer((const BYTE*)pData, SizeBytes, pCmdList, pBuffer);
        if (FAILED(hr))
        {
            return hr;
        }
        if (!IsCSP)
        {
            ResourceBarrier(pCmdList, pBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
        }
    }

    *ppBuffer = pBuffer;

    return S_OK;
}

HRESULT CreateDefaultIndexBuffer(ID3D12Device* pd3dDevice, 
                                 CpuGpuHeap* pUploadHeap, 
                                 ID3D12GraphicsCommandList* pCmdList, 
                                 SIZE_T SizeBytes, 
                                 DXGI_FORMAT IBFormat, 
                                 const void* pData, 
                                 ID3D12Resource** ppBuffer, 
                                 D3D12_INDEX_BUFFER_VIEW* pIBView)
{
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
#if defined(_XBOX_ONE) && defined(_TITLE)
    flags |= D3D12XBOX_RESOURCE_FLAG_PREFER_INDEX_BUFFER;
#endif

    HRESULT hr = CreateDefaultBuffer(pd3dDevice, pUploadHeap, pCmdList, SizeBytes, pData, ppBuffer, flags);
    if (FAILED(hr))
    {
        return hr;
    }
    pIBView->BufferLocation = (*ppBuffer)->GetGPUVirtualAddress();
    pIBView->Format = IBFormat;
    pIBView->SizeInBytes = (UINT32)SizeBytes;
    return S_OK;
}

HRESULT CreateDefaultVertexBuffer(ID3D12Device* pd3dDevice, 
                                  CpuGpuHeap* pUploadHeap, 
                                  ID3D12GraphicsCommandList* pCmdList, 
                                  SIZE_T SizeBytes, 
                                  SIZE_T StrideBytes, 
                                  const void* pData, 
                                  ID3D12Resource** ppBuffer, 
                                  D3D12_VERTEX_BUFFER_VIEW* pVBView)
{
    HRESULT hr = CreateDefaultBuffer(pd3dDevice, pUploadHeap, pCmdList, SizeBytes, pData, ppBuffer);
    if (FAILED(hr))
    {
        return hr;
    }
    pVBView->BufferLocation = (*ppBuffer)->GetGPUVirtualAddress();
    pVBView->SizeInBytes = (UINT32)SizeBytes;
    pVBView->StrideInBytes = (UINT32)StrideBytes;
    return S_OK;
}

HRESULT CreateDefaultConstantBuffer(ID3D12Device* pd3dDevice, 
                                    CpuGpuHeap* pUploadHeap, 
                                    ID3D12GraphicsCommandList* pCmdList, 
                                    SIZE_T SizeBytes, 
                                    const void* pData, 
                                    ID3D12Resource** ppBuffer, 
                                    D3D12_CPU_DESCRIPTOR_HANDLE hCBHandle)
{
    HRESULT hr = CreateDefaultBuffer(pd3dDevice, pUploadHeap, pCmdList, SizeBytes, pData, ppBuffer);
    if (FAILED(hr))
    {
        return hr;
    }
    D3D12_CONSTANT_BUFFER_VIEW_DESC CBDesc = {};
    CBDesc.BufferLocation = (*ppBuffer)->GetGPUVirtualAddress();
    CBDesc.SizeInBytes = (UINT32)SizeBytes;
    pd3dDevice->CreateConstantBufferView(&CBDesc, hCBHandle);
    return S_OK;
}

//-------------------------------------------------------------------------------------------------

HRESULT CreateStagingBufferForTexture(ID3D12Device* pd3dDevice, const D3D12_RESOURCE_DESC* pTextureDesc, bool Readback, ID3D12Resource** ppBuffer, D3D12_TEXTURE_COPY_LOCATION* pStagingCopyLocation, D3D12_RANGE* pEntireRange)
{
    const UINT32 SliceCount = pTextureDesc->DepthOrArraySize;

    D3D12_RESOURCE_DESC StagingDesc = *pTextureDesc;
    StagingDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    StagingDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    StagingDesc.MipLevels = 1;
    StagingDesc.DepthOrArraySize = 1;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT CopyLocation = {};
    UINT64 TotalBytes = 0;
    pd3dDevice->GetCopyableFootprints(&StagingDesc, 0, 1, 0, &CopyLocation, nullptr, nullptr, &TotalBytes);
    if (SliceCount > 1)
    {
        UINT64 SliceSizeBytes = CopyLocation.Footprint.RowPitch * CopyLocation.Footprint.Height;
        if (SliceSizeBytes > TotalBytes)
        {
            TotalBytes = SliceSizeBytes;
        }
        TotalBytes *= SliceCount;
    }

    D3D12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(TotalBytes);
    D3D12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(Readback ? D3D12_HEAP_TYPE_READBACK : D3D12_HEAP_TYPE_UPLOAD);
    HRESULT hr = pd3dDevice->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &BufferDesc, Readback ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)ppBuffer);

    if (pStagingCopyLocation != nullptr)
    {
        pStagingCopyLocation->Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        pStagingCopyLocation->pResource = *ppBuffer;
        CopyLocation.Offset = 0;
        pStagingCopyLocation->PlacedFootprint = CopyLocation;
    }

    if (pEntireRange != nullptr)
    {
        pEntireRange->Begin = 0;
        pEntireRange->End = TotalBytes;
    }

    return hr;
}

//-------------------------------------------------------------------------------------------------

HRESULT LinearBufferAllocator::Initialize(ID3D12Device* pd3dDevice, SIZE_T SizeBytes)
{
    D3D12_RESOURCE_DESC BufferDesc = {};
    BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    BufferDesc.Width = SizeBytes;
    BufferDesc.Height = 1;
    BufferDesc.DepthOrArraySize = 1;
    BufferDesc.MipLevels = 1;
    BufferDesc.SampleDesc.Count = 1;
    BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_HEAP_PROPERTIES HeapProperties = {};
    HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProperties.CreationNodeMask = D3D12XBOX_NODE_MASK;
    HeapProperties.VisibleNodeMask = D3D12XBOX_NODE_MASK;

    HRESULT hr = pd3dDevice->CreateCommittedResource(&HeapProperties,
                                                     D3D12_HEAP_FLAG_NONE,
                                                     &BufferDesc,
                                                     D3D12_RESOURCE_STATE_GENERIC_READ,
                                                     nullptr,
                                                     __uuidof(ID3D12Resource),
                                                     (void**)&m_pResource);

    if (FAILED(hr))
    {
        return hr;
    }

    hr = m_pResource->Map(0, nullptr, (void**)&m_pBase);
    if (FAILED(hr))
    {
        SAFE_RELEASE(m_pResource);
        return hr;
    }

    m_BaseAddress = m_pResource->GetGPUVirtualAddress();
    m_SizeBytes = SizeBytes;
    Reset();

    return S_OK;
}

//-------------------------------------------------------------------------------------------------

HRESULT CreateRootSignature(ID3D12Device* pd3dDevice, 
                            const D3D12_ROOT_SIGNATURE_DESC* pDesc, 
                            ID3D12RootSignature** ppRootSignature)
{
    ID3DBlob* pRSBlob = nullptr;
    ID3DBlob* pErrorBlob = nullptr;

    HRESULT hr = D3D12SerializeRootSignature(pDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pRSBlob, &pErrorBlob);
    if (FAILED(hr))
    {
        assert(pErrorBlob != nullptr);
        DebugSpew("Root signature serialize failed: %s\n", pErrorBlob->GetBufferPointer());
        SAFE_RELEASE(pErrorBlob);
        return hr;
    }

    assert(pErrorBlob == nullptr && pRSBlob != nullptr);

    hr = pd3dDevice->CreateRootSignature(D3D12XBOX_NODE_MASK, 
                                         pRSBlob->GetBufferPointer(), 
                                         pRSBlob->GetBufferSize(),
                                         __uuidof(ID3D12RootSignature),
                                         (void**)ppRootSignature);

    SAFE_RELEASE(pRSBlob);

    return hr;
}

void GpuIntervalTimer::Initialize(UINT32 MaxIntervals)
{
    m_MaxIntervalCount = MaxIntervals;
    UINT64 TimestampCount = (UINT64)(MaxIntervals + 1) * m_TimestampBufferDepth;
    
#if defined(_XBOX_ONE) && defined(_TITLE) && !XGS_BUILD
    m_pRawTimestamps = (UINT64*)VirtualAlloc(nullptr, TimestampCount * sizeof(UINT64), MEM_GRAPHICS | MEM_LARGE_PAGES | MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE | PAGE_GPU_COHERENT);
#else
    m_pRawTimestamps = nullptr;
#endif
    m_pDeltaTimestamps = new UINT64[TimestampCount];
    ZeroMemory(m_pDeltaTimestamps, TimestampCount * sizeof(UINT64));

    m_FrameIndex = 0;
    m_CurrentIntervalIndex = 0;
}

void GpuIntervalTimer::Terminate()
{
    VirtualFree(m_pRawTimestamps, 0, MEM_RELEASE);
    m_pRawTimestamps = nullptr;
    delete[] m_pDeltaTimestamps;
    m_pDeltaTimestamps = nullptr;
}

void GpuIntervalTimer::AddInterval(const WCHAR* strName)
{
    assert(GetIntervalCount() < m_MaxIntervalCount);

    TimingInterval TI = {};
    wcscpy_s(TI.strName, strName);
    m_Intervals.push_back(TI);
}

void GpuIntervalTimer::NextFrame()
{
    ++m_FrameIndex;

    assert(m_pRawTimestamps != nullptr);

    const UINT32 IntervalCount = GetIntervalCount();
    assert(m_CurrentIntervalIndex == 0 || m_CurrentIntervalIndex == IntervalCount + 1);

    for (UINT32 i = 1; i <= IntervalCount; ++i)
    {
        m_pDeltaTimestamps[GetTimestampIndex(i - 1)] = m_pRawTimestamps[GetTimestampIndex(i)] - m_pRawTimestamps[GetTimestampIndex(i - 1)];
        UINT64 Sum = 0;
        const UINT32 BaseIndex = m_TimestampBufferDepth * (i - 1);
        for (UINT32 j = 0; j < m_TimestampBufferDepth; ++j)
        {
            Sum += m_pDeltaTimestamps[BaseIndex + j];
        }
        m_Intervals[i - 1].AverageTicks = (Sum / m_TimestampBufferDepth);
    }

    m_CurrentIntervalIndex = 0;
}

void GpuIntervalTimer::MarkTimestamp(ID3D12GraphicsCommandList* pCmdList)
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    pCmdList->Write64BitValueBottomOfPipeX(GetTimestampAddress(m_CurrentIntervalIndex), 0, D3D12XBOX_FLUSH_NONE, D3D12XBOX_WRITE_VALUE_BOP_FLAG_SYSTEM_TIMESTAMP);
#endif
    ++m_CurrentIntervalIndex;
}

void GpuIntervalTimer::GetInterval(UINT32 Index, const WCHAR** ppName, FLOAT* pTimeUsec) const
{
    if (Index >= GetIntervalCount())
    {
        *ppName = nullptr;
        *pTimeUsec = 0;
        return;
    }

    const TimingInterval& TI = m_Intervals[Index];
    *ppName = TI.strName;
    *pTimeUsec = (FLOAT)((DOUBLE)TI.AverageTicks * 1e-2);
}
