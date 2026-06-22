//--------------------------------------------------------------------------------------
// texfill.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "precomp.hpp"
#include "texfill.hpp"
#include "util.hpp"

#include <directxmath.h>
using namespace DirectX;
#include <directxpackedvector.h>
using namespace DirectX::PackedVector;

inline VOID EncodeBC1Texel(VOID* pPixel, XMVECTOR vColor)
{
    static const XMVECTOR vScale565 = { 31.0f, 63.0f, 31.0f, 0 };

    // fill in 565 color anchors with the same color:
    BYTE* pDest = (BYTE*)pPixel;
    XMStoreU565((XMU565*)pDest, XMVectorSwizzle(vColor, 2, 1, 0, 3) * vScale565);
    XMStoreU565((XMU565*)(pDest + 2), XMVectorSwizzle(vColor, 2, 1, 0, 3) * vScale565);

    // set all 16 2-bit indices to 0:
    ZeroMemory(pDest + 4, 4);
}

inline VOID EncodeBC2Texel(VOID* pPixel, XMVECTOR Color)
{
    static const XMVECTOR vScale4444 = { 15.0f, 15.0f, 15.0f, 15.0f };

    BYTE* pDest = (BYTE*)pPixel;

    // store the alpha in all 16 4-bit alpha indices:
    XMVECTOR Alpha = XMVectorSplatW(Color) * vScale4444;
    XMStoreUNibble4((XMUNIBBLE4*)pDest, Alpha);
    XMStoreUNibble4((XMUNIBBLE4*)(pDest + 2), Alpha);
    XMStoreUNibble4((XMUNIBBLE4*)(pDest + 4), Alpha);
    XMStoreUNibble4((XMUNIBBLE4*)(pDest + 6), Alpha);

    // store the color as a BC1 block:
    EncodeBC1Texel(pDest + 8, Color);
}

inline VOID EncodeBC4Texel(VOID* pPixel, XMVECTOR Color)
{
    BYTE* pDest = (BYTE*)pPixel;

    XMUBYTEN4 Temp;
    XMStoreUByteN4(&Temp, Color);

    // fill in both byte alpha anchors:
    pDest[0] = Temp.x;
    pDest[1] = Temp.x;

    // fill in 6 bytes of zeros for the 16 3-bit indices:
    ZeroMemory(pDest + 2, 6);
}

inline VOID EncodeBC3Texel(VOID* pPixel, XMVECTOR Color)
{
    BYTE* pDest = (BYTE*)pPixel;

    // store the alpha as a BC4 block:
    EncodeBC4Texel(pDest, XMVectorSplatW(Color));

    // store the color as a BC1 block:
    EncodeBC1Texel(pDest + 8, Color);
}

inline VOID EncodeBC5Texel(VOID* pPixel, XMVECTOR Color)
{
    BYTE* pDest = (BYTE*)pPixel;

    // store the red as a BC4 block:
    EncodeBC4Texel(pDest, XMVectorSplatX(Color));

    // store the green as a BC4 block:
    EncodeBC4Texel(pDest + 8, XMVectorSplatY(Color));
}

/*
Specification:

a. Both formats do not support INF or NAN

b. 6e4 10-10-10-2 MDR:
The raster back-end must support output to a packed 32-bit floating point render target color format which includes 30 bits of color and 2 bits of alpha. The 10-bit color channels shall be formatted using an unsigned 6e4 (exponent bias of 7, support for denormals) floating point representation while the 2-bit alpha channel shall be formatted using an unsigned (normalized) integer representation. A packed 32-bit floating point texture format with equivalent composition shall be provided with support as a Resolve destination and in sampler fetch conversions and filtering. This format must support signed blending operations using relatively high precision source alpha and should run at full rate with 4x AA fragment processing. The following conversion code explains the details of this format.
int exp = (value & 0x03c0) >> 6;
float mant = (value & 0x003f) * 2^-6;
float out = (exp == 0) ? mant*2^-6 : (1 + mant)*2^exp-7;
The smallest nonzero value is value == 0x0001 --> exp == 0, mant == 1/64 --> out == 1/4096, and the largest value is value == 0x03ff --> exp == 15, mant == 63/64 --> out == 508.

c. 7e3 10-10-10-2 MDR:
The raster back-end must support output to a packed 32-bit floating point render target color format which includes 30 bits of color and 2 bits of alpha. The 10 bit color channels shall be formatted using an unsigned 7e3 (with support for denormals) floating point representation while the 2-bit alpha channel shall be formatted using an unsigned (normalized) integer representation. A packed 32 bit floating point texture format with equivalent composition shall be provided with support as a Resolve destination and in sampler fetch conversions and filtering. This format must support signed blending operations using relatively high precision source alpha and should
run at full rate with 4x AA fragment processing. The following conversion code explains the details of this format.
int exp = (value & 0x0380) >> 7;
float mant = (value & 0x007f) * 2^-7;
float out = (exp == 0) ? mant*2^-2 : (1 + mant)*2^exp-3;
The smallest nonzero value is value == 0x0001 --> exp == 0, mant == 1/128 --> out == 1/512, and the largest value is value == 0x03ff --> exp == 7, mant == 127/128 --> out == 31.875.

*/

#pragma warning(push)
// XMDEC4 is an Xbox 360-specific GPU type and is marked deprecated in the public headers

inline VOID Encode7e3Texel(VOID* pPixel, XMVECTOR Color)
{
    // This is not correct at all, but it will put some sort of bits in the right places:

    XMStoreDec4((XMDEC4*)pPixel, Color);
}

inline VOID Encode6e4Texel(VOID* pPixel, XMVECTOR Color)
{
    // This is not correct at all, but it will put some sort of bits in the right places:

    XMStoreDec4((XMDEC4*)pPixel, Color);
}

#pragma warning(pop)

inline VOID SetPixel(VOID* pPixel, const FLOAT ColorRGBA[4], DXGI_FORMAT Format, UINT XPos, UINT RowPitchBytes)
{
    XMVECTOR vColor = XMLoadFloat4((const XMFLOAT4*)ColorRGBA);
    static const XMVECTOR vScale565 = { 31.0f, 63.0f, 31.0f, 0 };
    static const XMVECTOR vScale4444 = { 15.0f, 15.0f, 15.0f, 15.0f };
    static const XMVECTOR vScale5551 = { 31.0f, 31.0f, 31.0f, 1.0f };

    switch (Format)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        XMStoreUByteN4((XMUBYTEN4*)pPixel, vColor);
        break;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        XMStoreUByteN4((XMUBYTEN4*)pPixel, XMVectorSwizzle(vColor, 2, 1, 0, 3));
        break;
    case DXGI_FORMAT_R8G8B8A8_SNORM:
        XMStoreByteN4((XMBYTEN4*)pPixel, vColor);
        break;
    case DXGI_FORMAT_B5G6R5_UNORM:
        XMStoreU565((XMU565*)pPixel, XMVectorSwizzle(vColor, 2, 1, 0, 3) * vScale565);
        break;
    case DXGI_FORMAT_B5G5R5A1_UNORM:
        XMStoreU555((XMU555*)pPixel, XMVectorSwizzle(vColor, 2, 1, 0, 3) * vScale5551);
        break;
    case DXGI_FORMAT_R8G8_SNORM:
    {
        XMBYTEN4 Temp;
        vColor += XMVector3Dot(vColor, XMVectorSet(0, 0, 0.25f, 0));
        vColor = XMVectorSaturate(vColor);
        XMStoreByteN4(&Temp, vColor);
        memcpy(pPixel, &Temp, 2);
        break;
    }
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_NV11:
    {
        XMUBYTEN4 Temp;
        vColor = XMVectorSaturate(XMVector3Dot(vColor, XMVectorSet(1, 0.25f, 0.25f, 0)));
        XMStoreUByteN4(&Temp, vColor);
        memcpy(pPixel, &Temp, 1);
        break;
    }
    case DXGI_FORMAT_R8_UINT:
    {
        XMUBYTE4 Temp;
        vColor = XMVectorSaturate(XMVector3Dot(vColor, XMVectorSet(1, 0.25f, 0.25f, 0)));
        vColor *= 255.0f;
        XMStoreUByte4(&Temp, vColor);
        memcpy(pPixel, &Temp, 1);
        break;
    }
    case DXGI_FORMAT_B4G4R4A4_UNORM:
    {
        XMStoreUNibble4((XMUNIBBLE4*)pPixel, XMVectorSwizzle(vColor, 2, 1, 0, 3) * vScale4444);
        break;
    }
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    {
        XMStoreUShortN4((XMUSHORTN4*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    {
        XMStoreShortN4((XMSHORTN4*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    {
        XMStoreFloat4((XMFLOAT4*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R32G32B32_FLOAT:
    {
        XMStoreFloat3((XMFLOAT3*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R32G32_FLOAT:
    {
        XMStoreFloat2((XMFLOAT2*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    {
        XMStoreHalf4((XMHALF4*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R16G16_FLOAT:
    {
        XMStoreHalf2((XMHALF2*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    {
        vColor = XMVector3Dot(vColor, XMVectorSet(1, 0.25f, 0.25f, 0));
        XMStoreFloat((FLOAT*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    {
        XMStoreUDecN4((XMUDECN4*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R16G16_UNORM:
    {
        XMStoreUShortN2((XMUSHORTN2*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R16G16_SNORM:
    {
        XMStoreShortN2((XMSHORTN2*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R32_FLOAT:
    {
        vColor = XMVector3Dot(vColor, XMVectorSet(1, 0.25f, 0.25f, 0));
        XMStoreFloat((FLOAT*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    {
        UINT Value = (UINT)(XMVectorGetX(vColor) * 16777215.0f);
        *(UINT*)pPixel = Value << 8;
        break;
    }
    case DXGI_FORMAT_R8G8_UNORM:
    {
        XMStoreUByteN2((XMUBYTEN2*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R16_FLOAT:
    {
        vColor = XMVector3Dot(vColor, XMVectorSet(1, 0.25f, 0.25f, 0));
        XMHALF2 Half2;
        XMStoreHalf2(&Half2, vColor);
        *(HALF*)pPixel = Half2.x;
        break;
    }
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
    {
        vColor = XMVectorSaturate(XMVector3Dot(vColor, XMVectorSet(1, 0.25f, 0.25f, 0)));
        XMUSHORTN2 UShortN2;
        XMStoreUShortN2(&UShortN2, vColor);
        *(USHORT*)pPixel = UShortN2.x;
        break;
    }
    case DXGI_FORMAT_R16_SNORM:
    {
        vColor = XMVectorSaturate(XMVector3Dot(vColor, XMVectorSet(1, 0.25f, 0.25f, 0)));
        XMSHORTN2 ShortN2;
        XMStoreShortN2(&ShortN2, vColor);
        *(SHORT*)pPixel = ShortN2.x;
        break;
    }
    case DXGI_FORMAT_R8_SNORM:
    {
        XMBYTEN4 Temp;
        vColor = XMVectorSaturate(XMVector3Dot(vColor, XMVectorSet(1, 0.25f, 0.25f, 0)));
        XMStoreByteN4(&Temp, vColor);
        memcpy(pPixel, &Temp, 1);
        break;
    }
    case DXGI_FORMAT_A8_UNORM:
    {
        XMUBYTEN4 Temp;
        vColor = XMVectorSaturate(XMVector3Dot(vColor, XMVectorSet(1, 0.25f, 0.25f, 0)));
        XMStoreUByteN4(&Temp, vColor);
        // fill in the alpha channel with the red channel so we see something...
        *(BYTE*)pPixel = Temp.x;
        break;
    }
    case DXGI_FORMAT_R11G11B10_FLOAT:
    {
        XMStoreFloat3PK((XMFLOAT3PK*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    {
        XMStoreFloat3SE((XMFLOAT3SE*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    {
        XMUBYTEN4 Temp;
        if (XPos % 2 == 0)
        {
            vColor = XMVectorSwizzle(XMVectorSaturate(vColor), 0, 1, 0, 1);
        }
        else
        {
            vColor = XMVectorSwizzle(XMVectorSaturate(vColor), 2, 1, 2, 1);
        }
        XMStoreUByteN4(&Temp, vColor);
        memcpy(pPixel, &Temp, 2);
        break;
    }
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    {
        XMUBYTEN4 Temp;
        if (XPos % 2 == 0)
        {
            vColor = XMVectorSwizzle(XMVectorSaturate(vColor), 1, 0, 1, 0);
        }
        else
        {
            vColor = XMVectorSwizzle(XMVectorSaturate(vColor), 1, 2, 1, 2);
        }
        XMStoreUByteN4(&Temp, vColor);
        memcpy(pPixel, &Temp, 2);
        break;
    }

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    {
        EncodeBC1Texel(pPixel, vColor);
        break;
    }

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    {
        EncodeBC2Texel(pPixel, vColor);
        break;
    }

    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    {
        EncodeBC3Texel(pPixel, vColor);
        break;
    }

    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    {
        EncodeBC4Texel(pPixel, vColor);
        break;
    }

    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    {
        EncodeBC5Texel(pPixel, vColor);
        break;
    }

    case DXGI_FORMAT_AYUV:
    {
        XMVECTOR vYUV = XMColorRGBToYUV(vColor);
        XMStoreUByteN4((XMUBYTEN4*)pPixel, vYUV);
        break;
    }

    case DXGI_FORMAT_Y410:
    {
        XMVECTOR vYUV = XMColorRGBToYUV(vColor);
        XMStoreUDecN4((XMUDECN4*)pPixel, vYUV);
        break;
    }

    case DXGI_FORMAT_Y416:
    {
        XMVECTOR vYUV = XMColorRGBToYUV(vColor);
        XMStoreUShortN4((XMUSHORTN4*)pPixel, vYUV);
        break;
    }

    case DXGI_FORMAT_YUY2:
    {
        XMVECTOR vYUV = XMColorRGBToYUV(vColor);
        XMStoreUByteN4((XMUBYTEN4*)pPixel, XMVectorSwizzle(vYUV, 0, 1, 0, 2));
        break;
    }

    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
    {
        XMVECTOR vYUV = XMColorRGBToYUV(vColor);
        XMStoreUShortN4((XMUSHORTN4*)pPixel, XMVectorSwizzle(vYUV, 0, 1, 0, 2));
        break;
    }

    case DXGI_FORMAT_R1_UNORM:
    {
        BYTE Value = (ColorRGBA[0] >= 0.5f) ? 1 : 0;
        UINT32 Shift = XPos % 8;
        Value <<= Shift;
        BYTE Mask = 1 << Shift;
        BYTE CurrentPixel = *(BYTE*)pPixel;
        CurrentPixel = (CurrentPixel & ~Mask) | Value;
        *(BYTE*)pPixel = CurrentPixel;
        break;
    }

#if defined(_XBOX_ONE) && defined(_TITLE)
    case DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT:
    {
        Encode7e3Texel(pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT:
    {
        Encode6e4Texel(pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM:
    {
        XMStoreXDecN4((XMXDECN4*)pPixel, vColor);
        break;
    }
    case DXGI_FORMAT_R4G4_UNORM:
    {
        XMUNIBBLE4 Nibble4;
        XMStoreUNibble4(&Nibble4, vColor * vScale4444);
        *(BYTE*)pPixel = *(BYTE*)&Nibble4;
        break;
    }
#endif

    // below formats are not supported as textures:

    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:

    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    default:
    {
        static DXGI_FORMAT LastFormat = DXGI_FORMAT_UNKNOWN;
        if (Format != LastFormat)
        {
            LastFormat = Format;
            DebugSpew("WARNING: Could not create pixels for format %S\n", g_strFormatNames[Format]);
        }
        break;
    }
    }
}

static const BYTE g_DigitGlyphs[165] =
{
    0, 1, 0,
    1, 0, 1,
    1, 0, 1,
    1, 0, 1,
    0, 1, 0,

    0, 1, 0,
    1, 1, 0,
    0, 1, 0,
    0, 1, 0,
    1, 1, 1,

    1, 1, 0,
    0, 0, 1,
    0, 1, 0,
    1, 0, 0,
    1, 1, 1,

    1, 1, 0,
    0, 0, 1,
    0, 1, 0,
    0, 0, 1,
    1, 1, 0,

    1, 0, 1,
    1, 0, 1,
    1, 1, 1,
    0, 0, 1,
    0, 0, 1,

    1, 1, 1,
    1, 0, 0,
    1, 1, 0,
    0, 0, 1,
    1, 1, 0,

    0, 1, 1,
    1, 0, 0,
    1, 1, 0,
    1, 0, 1,
    1, 1, 0,

    1, 1, 1,
    0, 0, 1,
    0, 1, 0,
    0, 1, 0,
    0, 1, 0,

    0, 1, 0,
    1, 0, 1,
    0, 1, 0,
    1, 0, 1,
    0, 1, 0,

    0, 1, 0,
    1, 0, 1,
    0, 1, 1,
    0, 0, 1,
    1, 1, 0,

    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
};
static const UINT g_GlyphWidth = 3;
static const UINT g_GlyphHeight = 5;

inline BYTE GetDigit(UINT X, UINT Y, UINT OffsetX, UINT OffsetY, const CHAR* strString, UINT Digits)
{
    const UINT GlyphWidth = 3;
    const UINT GlyphHeight = 5;
    const UINT GlyphSpacing = 1;

    if (X < OffsetX || Y < OffsetY)
    {
        return 0;
    }

    X -= OffsetX;
    Y -= OffsetY;

    if (Y >= GlyphHeight)
    {
        return 0;
    }

    UINT DigitIndex = X / (GlyphWidth + GlyphSpacing);
    if (DigitIndex >= Digits)
    {
        return 0;
    }

    CHAR DigitChar = strString[DigitIndex];
    if (DigitChar < '0' || DigitChar > '9')
    {
        return 0;
    }
    DigitChar -= '0';

    UINT DigitX = X % (GlyphWidth + GlyphSpacing);
    if (DigitX >= GlyphWidth)
    {
        return 0;
    }

    UINT DigitOffset = (DigitChar * (GlyphWidth * GlyphHeight)) + (Y * GlyphWidth) + DigitX;

    return g_DigitGlyphs[DigitOffset];
}

VOID FillTextureBits(VOID* pImageBits, UINT RowPitchBytes, UINT Width, UINT Height, DXGI_FORMAT Format, UINT Pattern, UINT MipLevel, UINT MarkColor)
{
    UINT bpp = GetBytesPerElement(Format);

    if (RowPitchBytes == 0)
    {
        RowPitchBytes = Width * bpp;
    }

    UINT pixelbpp = bpp;

    static const XMVECTOR MipColors[] = {
        { 1, 0, 0, 1 },
        { 1, 0.5f, 0, 1 },
        { 1, 1, 0, 1 },
        { 0, 1, 0, 1 },
        { 0, 1, 1, 1 },
        { 0, 0, 1, 1 },
        { 0.5f, 0, 1, 1 },
        { 0.5f, 0.5f, 0.5f, 1 },
        { 1, 1, 1, 1 },
        { 0, 0, 0, 1 },
        { 0.5f, 0.5f, 1, 1 },
    };

    BYTE* pRow = (BYTE*)pImageBits;

    CHAR strDigits[10] = "";
    if (MarkColor > 0)
    {
        _itoa_s(MarkColor - 1, strDigits, 10);
    }
    UINT DigitSize = (UINT32)strlen(strDigits);
    UINT32 IncrementXInterval = 1;
    if (Format == DXGI_FORMAT_R1_UNORM)
    {
        IncrementXInterval = 8;
    }

    FLOAT Color[4] = { 0, 0, 0, 1 };
    for (UINT y = 0; y < Height; ++y)
    {
        BYTE* pCurrentPixel = pRow;

        for (UINT x = 0; x < Width; ++x)
        {
            switch (Pattern)
            {
            case FP_Gradient:
                Color[0] = (FLOAT)x / (FLOAT)Width;
                Color[1] = (FLOAT)y / (FLOAT)Height;
                Color[2] = (FLOAT)(x + y) / (FLOAT)(Width + Height);
                break;
            case FP_MipLevelColorGrid:
            {
                UINT ColorIndex = MipLevel % ARRAYSIZE(MipColors);
                XMVECTOR vColor = MipColors[ColorIndex];
                if (x % 5 == 0 || y % 5 == 0)
                {
                    vColor *= 0.5f;
                }
                XMStoreFloat4((XMFLOAT4*)Color, vColor);
                break;
            }
            case FP_OddStripes:
            {
                if (x % 5 == 0)
                {
                    Color[0] = 1.0f;
                }
                else
                {
                    Color[0] = 0.0f;
                }
                if (y % 7 == 0)
                {
                    Color[1] = 1.0f;
                }
                else
                {
                    Color[1] = 0.0f;
                }
                if ((x + y) % 9 == 0)
                {
                    Color[2] = 1.0f;
                }
                else
                {
                    Color[2] = 0.0f;
                }
                break;
            }
            case FP_GradientMod256:
                Color[0] = (FLOAT)(x % 256) / 255.0f;
                Color[1] = (FLOAT)(y % 256) / 255.0f;
                Color[2] = (FLOAT)MipLevel / 255.0f;
                break;
            case FP_Checker8x8:
            {
                UINT XCoord = (x * 8) / Width;
                UINT YCoord = (y * 8) / Height;
                UINT BlackWhite = (XCoord + YCoord) % 2;
                FLOAT PixelColor = BlackWhite ? 1.0f : 0.0f;
                Color[0] = Color[1] = Color[2] = PixelColor;
                break;
            }
            case FP_HorizStripes4:
            {
                UINT BlackWhite = (y / 4) % 2;
                FLOAT PixelColor = BlackWhite ? 1.0f : 0.0f;
                Color[0] = Color[1] = Color[2] = PixelColor;
                break;
            }
            case FP_VertStripes4:
            {
                UINT BlackWhite = (x / 4) % 2;
                FLOAT PixelColor = BlackWhite ? 1.0f : 0.0f;
                Color[0] = Color[1] = Color[2] = PixelColor;
                break;
            }
            case FP_SolidMipLevelColor:
            {
                UINT ColorIndex = MipLevel % ARRAYSIZE(MipColors);
                XMVECTOR vColor = MipColors[ColorIndex];
                XMStoreFloat4((XMFLOAT4*)Color, vColor);
                break;
            }
            case FP_SolidColor:
            {
                XMVECTOR vColorBGRA = XMLoadUByteN4((const XMUBYTEN4*)&MipLevel);
                XMStoreFloat4((XMFLOAT4*)Color, XMVectorSwizzle<2, 1, 0, 3>(vColorBGRA));
                break;
            }
            default:
                Color[0] = 1.0f;
                Color[1] = 0.0f;
                Color[2] = 1.0f;
                break;
            }

            const UINT MarkSize = 10;
            if (MarkColor != 0 && x <= MarkSize && y <= MarkSize)
            {
                BYTE DigitPixel = GetDigit(x, y, 2, 2, strDigits, DigitSize);

                if (x == MarkSize || y == MarkSize || DigitPixel != 0)
                {
                    XMStoreFloat4((XMFLOAT4*)Color, XMVectorZero());
                }
                else
                {
                    UINT ColorIndex = (MarkColor - 1) % ARRAYSIZE(MipColors);
                    XMStoreFloat4((XMFLOAT4*)Color, MipColors[ColorIndex]);
                }
            }

            SetPixel(pCurrentPixel, Color, Format, x, RowPitchBytes);
            if (((x + 1) % IncrementXInterval) == 0)
            {
                pCurrentPixel += pixelbpp;
            }
        }

        pRow += RowPitchBytes;
    }
}

void WriteTextToBits(void* pImageBits, UINT RowPitchBytes, UINT Width, UINT Height, DXGI_FORMAT Format, const CHAR* strText, UINT X, UINT Y, const FLOAT TextColor[4], const FLOAT BGColor[4])
{
    assert(strText != nullptr);

    UINT GlyphWidth = g_GlyphWidth;

    UINT bpp = GetBytesPerElement(Format);
    if (RowPitchBytes == 0)
    {
        RowPitchBytes = Width * bpp;
    }

    UINT32 IncrementXInterval = 1;
    if (Format == DXGI_FORMAT_R1_UNORM)
    {
        IncrementXInterval = 8;
    }

    const CHAR* pChar = strText;
    while (*pChar != '\0')
    {
        INT CharValue = *pChar - '0';
        if (CharValue < 0 || CharValue > 9)
        {
            CharValue = 10;
        }

        const UINT GlyphBaseIndex = CharValue * (g_GlyphHeight * g_GlyphWidth);

        for (UINT YCell = 0; YCell < g_GlyphHeight; ++YCell)
        {
            if (Y + YCell >= Height)
            {
                continue;
            }

            BYTE* pRow = (BYTE*)pImageBits + (RowPitchBytes * (Y + YCell)) + (bpp * X);
            for (UINT XCell = 0; XCell < GlyphWidth; ++XCell)
            {
                if (X + XCell >= Width)
                {
                    break;
                }

                bool LookupPixel = true;
                UINT LookupX = XCell;

                if (GlyphWidth > g_GlyphWidth)
                {
                    if (XCell == 0)
                    {
                        LookupPixel = false;
                    }
                    else
                    {
                        LookupX = XCell - 1;
                    }
                }

                BYTE Pixel = 0;
                if (LookupPixel)
                {
                    const UINT PixelIndex = GlyphBaseIndex + (YCell * g_GlyphWidth) + LookupX;
                    assert(PixelIndex < ARRAYSIZE(g_DigitGlyphs));
                    Pixel = g_DigitGlyphs[PixelIndex];
                }

                SetPixel(pRow, (Pixel != 0) ? TextColor : BGColor, Format, X + XCell, RowPitchBytes);
                if (((XCell + 1) % IncrementXInterval) == 0)
                {
                    pRow += bpp;
                }
            }
        }

        X += GlyphWidth;
        GlyphWidth = g_GlyphWidth + 1;
        ++pChar;
    }
}

UINT GetBytesPerElement(DXGI_FORMAT Format)
{
    switch (Format)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 16;
    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 12;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
        return 8;
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
        return 8;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
        return 8;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
        return 8;
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
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
#if LNM
    case DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT:
    case DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT:
    case DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM:
#endif
        return 4;
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
    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
        return 2;
    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
#if LNM
    case DXGI_FORMAT_R4G4_UNORM:
#endif
        return 1;
    case DXGI_FORMAT_R1_UNORM:
        return 1;
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        return 4;
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
        return 2;
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 8;
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
    case DXGI_FORMAT_A8P8:
        return 2;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_YUY2:
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
        return 16;
        // Planar formats:
    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_NV11:
        return 1;
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
        return 1;
    default:
        return 0;
    }
}

BOOL IsPlanarFormat(DXGI_FORMAT Format)
{
    switch (Format)
    {
    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_NV11:
        return TRUE;
    }

    return FALSE;
}

BOOL IsCompressedFormat(DXGI_FORMAT Format)
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
        return TRUE;
    }
    return FALSE;
}

DXGI_FORMAT GetSRViewFormat(DXGI_FORMAT TextureFormat, UINT PlaneIndex /*= 0 */)
{
    switch (TextureFormat)
    {
    case DXGI_FORMAT_AYUV:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_Y410:
        return DXGI_FORMAT_R10G10B10A2_UNORM;
    case DXGI_FORMAT_Y416:
        return DXGI_FORMAT_R16G16B16A16_UNORM;
    case DXGI_FORMAT_NV12:
        return PlaneIndex == 0 ? DXGI_FORMAT_R8_UNORM : DXGI_FORMAT_R8G8_UNORM;
    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
        return PlaneIndex == 0 ? DXGI_FORMAT_R16_UNORM : DXGI_FORMAT_R16G16_UNORM;
    case DXGI_FORMAT_YUY2:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_Y210:
        return DXGI_FORMAT_R16G16B16A16_UNORM;
    case DXGI_FORMAT_Y216:
        return DXGI_FORMAT_R16G16B16A16_UNORM;
    case DXGI_FORMAT_NV11:
        return PlaneIndex == 0 ? DXGI_FORMAT_R8_UNORM : DXGI_FORMAT_R8G8_UNORM;
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
        return DXGI_FORMAT_R8_UNORM;
    case DXGI_FORMAT_A8P8:
        return DXGI_FORMAT_R8G8_UNORM;
    case DXGI_FORMAT_420_OPAQUE:
        return DXGI_FORMAT_UNKNOWN;
    case DXGI_FORMAT_D32_FLOAT:
        return DXGI_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_D16_UNORM:
        return DXGI_FORMAT_R16_UNORM;
    default:
        return TextureFormat;
    }
}

DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT Format)
{
    switch (Format)
    {
    case DXGI_FORMAT_UNKNOWN:
        return DXGI_FORMAT_UNKNOWN;
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return DXGI_FORMAT_R32G32B32A32_TYPELESS;
    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return DXGI_FORMAT_R32G32B32_TYPELESS;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
        return DXGI_FORMAT_R16G16B16A16_TYPELESS;
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
        return DXGI_FORMAT_R32G32_TYPELESS;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return DXGI_FORMAT_R32G8X24_TYPELESS;
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
        return DXGI_FORMAT_R10G10B10A2_TYPELESS;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
        return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
        return DXGI_FORMAT_R16G16_TYPELESS;
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
        return DXGI_FORMAT_R32_TYPELESS;
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return DXGI_FORMAT_R24G8_TYPELESS;
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
        return DXGI_FORMAT_R8G8_TYPELESS;
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
        return DXGI_FORMAT_R16_TYPELESS;
    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
        return DXGI_FORMAT_R8_TYPELESS;
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        return DXGI_FORMAT_BC1_TYPELESS;
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
        return DXGI_FORMAT_BC2_TYPELESS;
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        return DXGI_FORMAT_BC3_TYPELESS;
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return DXGI_FORMAT_BC4_TYPELESS;
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
        return DXGI_FORMAT_BC5_TYPELESS;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8A8_TYPELESS;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8X8_TYPELESS;
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
        return DXGI_FORMAT_BC6H_TYPELESS;
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return DXGI_FORMAT_BC7_TYPELESS;
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_R1_UNORM:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_YUY2:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
    case DXGI_FORMAT_NV11:
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
    case DXGI_FORMAT_A8P8:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
    case DXGI_FORMAT_R11G11B10_FLOAT:
#if LNM
    case DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT:
    case DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT:
#endif
    default:
        return Format;
    }
}

BOOL CompareMemory(D3D12_MAPPED_SUBRESOURCE* pTextureData, const VOID* pCompareData, UINT CompareDataPitchBytes, UINT RowCount)
{
    BYTE* pTexData = (BYTE*)pTextureData->pData;
    const BYTE* pRefData = (BYTE*)pCompareData;

    for (UINT i = 0; i < RowCount; ++i)
    {
        INT Result = memcmp(pTexData, pRefData, pTextureData->RowPitch);
        if (Result != 0)
        {
            return FALSE;
        }
        pTexData += pTextureData->RowPitch;
        pRefData += CompareDataPitchBytes;
    }
    return TRUE;
}
