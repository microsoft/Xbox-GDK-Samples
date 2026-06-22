//--------------------------------------------------------------------------------------
// TextureInfo.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "TextureInfo.h"

// DirectXTex includes
#include "DDS.h"

using namespace DirectX;

namespace
{
    void CalculateTextureLayout(TextureInfo& textureInfo)
    {
        XG_TEXTURE2D_DESC tex2DDesc = {};
        tex2DDesc.Width                 = textureInfo.width;
        tex2DDesc.Height                = textureInfo.height;
        tex2DDesc.MipLevels             = textureInfo.mipCount;
        tex2DDesc.ArraySize             = 1;
        tex2DDesc.Format                = static_cast<XG_FORMAT>(textureInfo.format);
        tex2DDesc.SampleDesc.Count      = 1;
        tex2DDesc.SampleDesc.Quality    = 0;
        tex2DDesc.Usage                 = XG_USAGE_DEFAULT;
        tex2DDesc.BindFlags             = XG_BIND_SHADER_RESOURCE;
        tex2DDesc.CPUAccessFlags        = 0;
        tex2DDesc.MiscFlags             = 0;
        // This looks odd, but makes sense. On Xbox texture layouts are directly
        // related to the GPU's native tiling modes. See the definition of
        // D3D12_TEXTURE_LAYOUT in d3d12_x.h.
        tex2DDesc.SwizzleMode           = static_cast<XG_SWIZZLE_MODE>(textureInfo.textureLayout & ~0x100);
        tex2DDesc.Pitch                 = 0;
        DX::ThrowIfFailed(XGComputeTexture2DLayout(&tex2DDesc, &textureInfo.layout));
    }

    XG_SWIZZLE_MODE GetXGSwizzleModeFromTileMode(XBTC_TILING_MODE tileMode)
    {
        switch (tileMode)
        {
        case XBTC_TILING_SCARLETT_LINEAR: return XG_SWIZZLE_MODE_LINEAR;
        case XBTC_TILING_SCARLETT_256B: return XG_SWIZZLE_MODE_256B_D;
        case XBTC_TILING_SCARLETT_4KB: return XG_SWIZZLE_MODE_4KB_D;
        case XBTC_TILING_SCARLETT_64KB: return XG_SWIZZLE_MODE_64KB_D;
        case XBTC_TILING_STANDARD_SWIZZLE: return XG_SWIZZLE_MODE_64KB_S;
        default:
            throw std::exception("Unsupported XBTC tiling mode");
        }
    }

#pragma region XDDS support
    //--------------------------------------------------------------------------------------
    // DDS file structure definitions
    //
    // See DirectXTexXboxDDS.cpp in the Xbox version of DirectXTex library
    //--------------------------------------------------------------------------------------
#pragma pack(push,1)

    struct DDS_HEADER_XBOX
        // Must match structure in XboxDDSTextureLoader module
    {
        DXGI_FORMAT dxgiFormat;
        uint32_t    resourceDimension;
        uint32_t    miscFlag;           // see DDS_RESOURCE_MISC_FLAG
        uint32_t    arraySize;
        uint32_t    miscFlags2;         // see DDS_MISC_FLAGS2
        uint32_t    tileMode;           // see XG_TILE_MODE / XG_SWIZZLE_MODE
        uint32_t    baseAlignment;
        uint32_t    dataSize;
        uint32_t    xdkVer;             // matching _XDK_VER / _GXDK_VER
    };

#pragma pack(pop)

    constexpr uint32_t XBOX_TILEMODE_SCARLETT = 0x1000000;

    static_assert(sizeof(DDS_HEADER_XBOX) == 36, "DDS XBOX Header size mismatch");

    TextureInfo LoadDDSTextureInfo(const char* ddsFilePath)
    {
        struct RequiredHeader
        {
            uint32_t            Magic;
            DDS_HEADER          DDSHeader;
            DDS_HEADER_XBOX     XboxHeader;
        };

        std::ifstream fileStream;
        fileStream.open(ddsFilePath, std::ios_base::in | std::ios_base::binary);
        if (!fileStream.is_open())
        {
            throw std::exception("Failed to open DDS file");
        }

        // Load DDS header
        RequiredHeader fileHeader = {};
        fileStream.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
        if ((fileHeader.Magic != DDS_MAGIC) ||
            !(fileHeader.DDSHeader.ddspf.flags & DDS_FOURCC) ||
            !(fileHeader.DDSHeader.ddspf.fourCC == MAKEFOURCC('X', 'B', 'O', 'X')))
        {
            throw std::exception("Encountered invalid Xbox DDS file");
        }

        if (fileHeader.XboxHeader.tileMode == uint32_t(-1))
        {
            throw std::exception("Encountered invalid Xbox DDS file");
        }
        else if (!(fileHeader.XboxHeader.tileMode & XBOX_TILEMODE_SCARLETT))
        {
            throw std::exception("Encountered non-Scarlett Xbox DDS file");
        }

        if ((static_cast<D3D12_RESOURCE_DIMENSION>(fileHeader.XboxHeader.resourceDimension) != D3D12_RESOURCE_DIMENSION_TEXTURE2D) ||
            (fileHeader.XboxHeader.arraySize > 1) ||
            (fileHeader.XboxHeader.miscFlag))
        {
            throw std::exception("Encountered unsupported Xbox DDS file");
        }

        auto swizzleMode = static_cast<XG_SWIZZLE_MODE>(fileHeader.XboxHeader.tileMode & ~XBOX_TILEMODE_SCARLETT);

        TextureInfo rv = {};
        rv.filePath             = ddsFilePath;
        rv.containerType        = ContainerType::XDDS;
        rv.format               = fileHeader.XboxHeader.dxgiFormat;
        rv.width                = fileHeader.DDSHeader.width;
        rv.height               = fileHeader.DDSHeader.height;
        rv.mipCount             = fileHeader.DDSHeader.mipMapCount;
        rv.streamableMipCount   = rv.mipCount;
        rv.textureLayout        = static_cast<D3D12_TEXTURE_LAYOUT>(swizzleMode | 0x100);

        uint64_t dataOffset = static_cast<uint64_t>(fileStream.tellg());
        fileStream.seekg(0, std::ios::end);
        uint64_t dataSize = static_cast<uint64_t>(fileStream.tellg()) - dataOffset;
        fileStream.close();

        CalculateTextureLayout(rv);

        if (dataSize != rv.layout.SizeBytes)
        {
            throw std::exception("Malformed Xbox DDS file");
        }

        // For XBox DDS files the mip data is laid out exactly like calculated by
        // XGComputeTexture2DLayout in the CalculateTextureLayout function
        for (uint32_t mipIndex = 0; mipIndex < fileHeader.DDSHeader.mipMapCount; mipIndex++)
        {
            const auto& layoutMip = rv.layout.Plane[0].MipLayout[mipIndex];
            auto& fileMip = rv.mips[mipIndex];
            fileMip.offset      = dataOffset + layoutMip.OffsetBytes;
            fileMip.loadSize    = static_cast<uint32_t>(layoutMip.SizeBytes);
            fileMip.deflated    = false;
            fileMip.bcPacked    = false;
        }

        return rv;
    }
#pragma endregion

    std::vector<TextureInfo> LoadXBTCTextureInfo(const char* bcpackFilePath)
    {
        std::ifstream fileStream;
        fileStream.open(bcpackFilePath, std::ios_base::in | std::ios_base::binary);
        if (!fileStream.is_open())
        {
            throw std::exception("Failed to open XBTC file");
        }

        XBTC_ARCHIVE_HEADER fileHeader = {};
        fileStream.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
        if (fileHeader.Identifier != XBTC_ARCHIVE_IDENTIFIER)
        {
            throw std::exception("Unknown archive identifier for XBTC file");
        }
        if (fileHeader.Version != XBTC_ARCHIVE_VERSION)
        {
            throw std::exception("Unknown version identifier for XBTC file");
        }

        // Read texture headers, index information and stream headers
        size_t headerDataSize = fileHeader.DataOffset - sizeof(fileHeader);
        std::vector<char> headerData;
        headerData.resize(headerDataSize);
        fileStream.read(headerData.data(), headerDataSize);

        auto textureInfos = reinterpret_cast<const XBTC_TEXTURE_INFO*>(headerData.data());
        auto offsetsAndIndices = reinterpret_cast<const XBTC_OFFSET_AND_INDEX*>(textureInfos + fileHeader.TextureCount);
        auto streamHeaders = reinterpret_cast<const XBTC_STREAM_INFO*>(offsetsAndIndices + fileHeader.TextureCount);

        std::vector<TextureInfo> bcpackTextures;
        bcpackTextures.resize(fileHeader.TextureCount);
        uint32_t streamBaseIndex = 0;
        for (uint32_t i = 0; i < fileHeader.TextureCount; i++)
        {
            const auto& texInfo = textureInfos[i];

            if (texInfo.Dimension != XBTC_TEX_DIMENSION_2D)
            {
                throw std::exception("Sample only supports 2D textures");
            }

            XG_SWIZZLE_MODE xgSwizzleMode = GetXGSwizzleModeFromTileMode(static_cast<XBTC_TILING_MODE>(texInfo.TileMode));

            TextureInfo& rv = bcpackTextures[i];
            rv.filePath             = bcpackFilePath;
            rv.textureIndex         = i;
            rv.containerType        = ContainerType::XBTC;
            rv.format               = static_cast<DXGI_FORMAT>(texInfo.Format);
            rv.width                = static_cast<uint32_t>(texInfo.Width + 1);
            rv.height               = static_cast<uint32_t>(texInfo.Height + 1);
            rv.mipCount             = texInfo.MipCount;
            rv.streamableMipCount   = texInfo.MipStreams;
            rv.textureLayout        = static_cast<D3D12_TEXTURE_LAYOUT>(xgSwizzleMode | 0x100);        // See D3D12_TEXTURE_LAYOUT definition in d3d12_xs.h for correlation to XG_SWIZZLE_MODE

            CalculateTextureLayout(rv);

            const auto& offsetAndIndex = offsetsAndIndices[i];
            for (uint32_t mipIndex = 0; mipIndex <= rv.streamableMipCount; mipIndex++)
            {
                // The order of mip data in the XBTC file is reverse of the order in the XG layout
                const auto& streamHeader = streamHeaders[streamBaseIndex + (rv.streamableMipCount - mipIndex)];
                auto& fileMip = rv.mips[mipIndex];

                fileMip.offset          = fileHeader.DataOffset + offsetAndIndex.BaseOffset + streamHeader.RelativeOffset;
                fileMip.loadSize        = streamHeader.CompressedSize;
                fileMip.inflatedSize    = streamHeader.InflatedSize;
                fileMip.deflated        = (streamHeader.CompressedSize != streamHeader.InflatedSize);
                fileMip.bcPacked        = (streamHeader.UncompressedSize != streamHeader.InflatedSize);
            }

            streamBaseIndex = offsetAndIndex.NextIndex;
        }

        return bcpackTextures;
    }
}

std::vector<TextureInfo> FindTexturesInDirectory(const char* dir)
{
    std::vector<TextureInfo> rv;

    WIN32_FIND_DATAA findData = {};

    std::string testDataDir(dir);

    HANDLE hFile = ::FindFirstFileA((testDataDir + "\\*").c_str(), &findData);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        throw std::exception("Failed to find any texture data");
    }

    do
    {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            continue;
        }
        const char* fileExt = strrchr(findData.cFileName, '.');
        if (!fileExt)
        {
            continue;
        }
        fileExt++;
        if (!::_stricmp(fileExt, "dds"))
        {
            rv.push_back(LoadDDSTextureInfo((testDataDir + "\\" + findData.cFileName).c_str()));            
        }
        else if (!::_stricmp(fileExt, "xbtc"))
        {
            std::vector<TextureInfo> bcpackTextures = LoadXBTCTextureInfo((testDataDir + "\\" + findData.cFileName).c_str());
            rv.insert(rv.end(), bcpackTextures.begin(), bcpackTextures.end());
        }
    } while (::FindNextFileA(hFile, &findData));
    ::FindClose(hFile);

    return rv;
}
