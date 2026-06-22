//--------------------------------------------------------------------------------------
// TextureInfo.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <xg_xs.h>

enum class ContainerType
{
    XDDS,
    XBTC
};

struct MipOffsetSize
{
    uint64_t            offset;
    uint32_t            loadSize;
    uint32_t            inflatedSize;               // If both deflated and BCPacked this is the intermediate size after inflate, but before BCPack decompression
    bool                deflated;
    bool                bcPacked;
};

struct TextureInfo
{
    std::string             filePath;               // Relative to dir passed into FindTexturesInDirectory
    uint32_t                textureIndex{0};           // Index of texture in file (XBTC containers support multiple textures)
    ContainerType           containerType{ContainerType::XDDS};
    DXGI_FORMAT             format{};
    uint32_t                width{0};
    uint32_t                height{0};
    uint32_t                mipCount{0};
    uint32_t                streamableMipCount{0};
    MipOffsetSize           mips[D3D12_REQ_MIP_LEVELS]{};
    D3D12_TEXTURE_LAYOUT    textureLayout{};
    XG_RESOURCE_LAYOUT      layout{};
};

// Function scans the directory (non-recursively) for DDS and BCPack files and builds TextureInfo
// objects. The dir parameter should not contain a trailing \ character.
std::vector<TextureInfo> FindTexturesInDirectory(const char* dir);
