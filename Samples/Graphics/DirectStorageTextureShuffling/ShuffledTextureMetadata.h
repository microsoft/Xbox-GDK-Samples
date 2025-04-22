//--------------------------------------------------------------------------------------
// ShuffledTextureMetadata.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifndef SHUFFLED_TEXTURE_METADATA_H
#define SHUFFLED_TEXTURE_METADATA_H

namespace ATG {
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
                (static_cast<uint32_t>(static_cast<uint8_t>(ch0)) \
                | (static_cast<uint32_t>(static_cast<uint8_t>(ch1)) << 8) \
                | (static_cast<uint32_t>(static_cast<uint8_t>(ch2)) << 16) \
                | (static_cast<uint32_t>(static_cast<uint8_t>(ch3)) << 24))
#endif

    const int32_t fourCC = MAKEFOURCC('S', 'T', 'M', 'D');
}

struct ShuffledTextureMetadata
{
    uint32_t    reserved;
    uint32_t    width;
    uint32_t    height;
    uint32_t    mipCount;
    DXGI_FORMAT format;
    D3D12_TEXTURE_LAYOUT layout;
    UINT32      swizzleMode;
    uint32_t    loadSize;
    uint32_t    uncompressedSize;
    uint32_t    shuffledComrpessedSize;
    uint32_t    unshuffledComrpessedSize;
	uint32_t	pad[5];
};

#endif // SHUFFLED_TEXTURE_METADATA_H
