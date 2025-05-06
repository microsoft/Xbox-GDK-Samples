//--------------------------------------------------------------------------------------
// ShuffleTextures.cpp
//
// Command line tool to create shuffled textures
//
// Usage: ShuffleTextures.exe <InputFileForBC1> <InputFileForBC3> <InputFileForBC4> <InputFileForBC5>
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define _GAMING_XBOX_SCARLETT

#include <zlib.h>
#include <xg_xs.h>
#include <DirectXTexXbox.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include "..\\ShuffledTextureMetadata.h"

#pragma warning(default : 4061)

HRESULT LoadInputImage(DirectX::ScratchImage& image, const wchar_t* pFilePath);
HRESULT EncodeToBCFormat(DirectX::ScratchImage& srcImage, DirectX::ScratchImage& bcImage, const DXGI_FORMAT bcFormat);
HRESULT ShuffleData(const Xbox::XboxImage& srcImage, Xbox::XboxImage& dstImage, XG_FORMAT format, UINT32& swizzleMode);
HRESULT CompressWithZlib(Xbox::XboxImage& srcImage, std::vector<Bytef>& compressedData, uint32_t& size);
HRESULT WriteTextureDataToDisk(std::wstring& outputPath, Xbox::XboxImage& image, const uint8_t* pCompressedData, UINT32 dstorageSwizzleMode,
                               const uint32_t shuffledCompressedSize, const uint32_t unshuffledCompressedSize);

const int numFormats = 4;
DXGI_FORMAT bcFormats[numFormats] = { DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC4_UNORM, DXGI_FORMAT_BC5_UNORM };
XG_FORMAT   xgFormats[numFormats] = { XG_FORMAT_BC1_UNORM, XG_FORMAT_BC3_UNORM, XG_FORMAT_BC4_UNORM, XG_FORMAT_BC5_UNORM };

std::wstring dataFolder = L"../Textures/";

int main(int argc, char* argv[])
{
    if (argc < 5)
    {
        wprintf(L"Usage: ShuffleTextures.exe <InputFileForBC1> <InputFileForBC3> <InputFileForBC4> <InputFileForBC5>\n");
        return 1;
    }

    std::wstring inputFilePath[numFormats];
    char* pInputFileName = nullptr;
    wchar_t* pWideInputFileName = nullptr;

    DirectX::ScratchImage   inputImage;
    DirectX::ScratchImage   bcImage;
    Xbox::XboxImage         bcXboxImage;
    Xbox::XboxImage         shuffledImage;
    std::vector<Bytef>      compressedData;

    uint32_t unshuffledCompressedSize = 0;
	uint32_t shuffledCompressedSize = 0;
    uint32_t dstorageSwizzleMode = 0;
    int bcIndex = 0;
	
    for (int i = 0; i < numFormats; i++)
    {
        pInputFileName = argv[i + 1];
        auto length = strlen(pInputFileName);
        pWideInputFileName = new wchar_t[length + 1];
        mbstowcs_s(&length, pWideInputFileName, length + 1, pInputFileName, length + 1);
        inputFilePath[i] = dataFolder + pWideInputFileName;
        delete[] pWideInputFileName;
    }

    // Initialize COM library to facilitate WIC usage
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        wprintf(L"Failed to initialize COM.\n");
        return 1;
    }

    for (int i = 0; i < numFormats; i++)
    {
        wprintf(L"\n");

        auto hr = LoadInputImage(inputImage, inputFilePath[i].c_str());
        if (FAILED(hr))
        {
            wprintf(L"Failed to load image.\n");
            return 1;
        }

        hr = EncodeToBCFormat(inputImage, bcImage, bcFormats[i]);
        if (FAILED(hr))
        {
            wprintf(L"Failed to convert image to BC%d-compressed format.\n", bcIndex);
            return 1;
        }

        hr = Xbox::Tile(bcImage.GetImages(), bcImage.GetImageCount(), bcImage.GetMetadata(), bcXboxImage, Xbox::XboxTileMode::XG_SWIZZLE_MODE_64KB_S);
        if (FAILED(hr))
        {
            wprintf(L"Failed to get tile format: %x\n", hr);
            return 1;
        }

        hr = ShuffleData(bcXboxImage, shuffledImage, xgFormats[bcIndex], dstorageSwizzleMode);
        if (FAILED(hr))
        {
            wprintf(L"Failed to shuffle image data.\n");
            return 1;
        }

        // Just for stats, determine the size of unshuffled compressed data, and shuffled compressed data
        {
            hr = CompressWithZlib(bcXboxImage, compressedData, unshuffledCompressedSize);
            if (FAILED(hr))
            {
                wprintf(L"Failed to compress image data with Zlib.\n");
                return 1;
            }
        }

        hr = CompressWithZlib(shuffledImage, compressedData, shuffledCompressedSize);
        if (FAILED(hr))
        {
            wprintf(L"Failed to compress image data with Zlib.\n");
            return 1;
        }

        WriteTextureDataToDisk(dataFolder, shuffledImage, compressedData.data(), dstorageSwizzleMode, shuffledCompressedSize, unshuffledCompressedSize);

        // Show some stats
        wprintf(L"\nStats:\n");
        wprintf(L"    Original size:              %d bytes\n", static_cast<uint32_t>(inputImage.GetPixelsSize()));
        wprintf(L"    BCn size:                   %d bytes\n", static_cast<uint32_t>(bcImage.GetPixelsSize()));
        wprintf(L"    Unshuffled compressed size: %d bytes\n", unshuffledCompressedSize);
        wprintf(L"    Shuffled compressed size:   %d bytes\n", shuffledCompressedSize);
        bcIndex++;
    }

    return 0;
}

HRESULT LoadInputImage(DirectX::ScratchImage& image, const wchar_t* pFilePath)
{
    wprintf(L"Loading: %s", pFilePath);
    HRESULT hr = DirectX::LoadFromWICFile(pFilePath, DirectX::WIC_FLAGS_NONE, nullptr, image);
    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}

HRESULT EncodeToBCFormat(DirectX::ScratchImage& srcImage, DirectX::ScratchImage& bcImage, const DXGI_FORMAT bcFormat)
{
    wprintf(L"\nEncoding BCn ...");

    HRESULT hr = DirectX::Compress(srcImage.GetImages(), srcImage.GetImageCount(), srcImage.GetMetadata(), bcFormat, DirectX::TEX_COMPRESS_SRGB_OUT, 0.5f, bcImage);
    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}

HRESULT ShuffleData(const Xbox::XboxImage& srcImage, Xbox::XboxImage& dstImage, XG_FORMAT format, UINT32& swizzleMode)
{
    wprintf(L"\nShuffling texture data ...");

    if (srcImage.GetPointer() == dstImage.GetPointer())
    {
        return E_POINTER;
    }

    dstImage.Initialize(srcImage.GetMetadata(), srcImage.GetTileMode(), srcImage.GetSize(), srcImage.GetAlignment());

    HRESULT hr = XGShuffleTextureBufferForDirectStorage(srcImage.GetPointer(), dstImage.GetPointer(), dstImage.GetSize(), format, &swizzleMode);
    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}

HRESULT CompressWithZlib(Xbox::XboxImage& srcImage, std::vector<Bytef>& compressedData, uint32_t& size)
{
    wprintf(L"\nCompressing with Zlib ...");

    uLong uncompressedSize = static_cast<uLong>(srcImage.GetSize());
    uLong compressedSize = compressBound(uncompressedSize);
    compressedData.resize(compressedSize);

    int result = compress(compressedData.data(), &compressedSize, srcImage.GetPointer(), uncompressedSize);

    if (result != Z_OK)
    {
        wprintf(L"Failed to compress data with zlib");
        return E_FAIL;
    }

    compressedData.resize(compressedSize);
    size = static_cast<uint32_t>(compressedSize);

    return S_OK;
}

HRESULT WriteTextureDataToDisk(std::wstring& outputPath, Xbox::XboxImage& image, const uint8_t* pCompressedData, UINT32 dstorageSwizzleMode,
                               const uint32_t shuffledCompressedSize, const uint32_t unshuffledCompressedSize)
{
    auto textureMetadata = image.GetMetadata();

    ShuffledTextureMetadata shuffledTextureMetadata{};

    shuffledTextureMetadata.reserved = ATG::fourCC;
    shuffledTextureMetadata.width = static_cast<uint32_t>(textureMetadata.width);
    shuffledTextureMetadata.height = static_cast<uint32_t>(textureMetadata.height);
    shuffledTextureMetadata.mipCount = static_cast<uint32_t>(textureMetadata.mipLevels);
    shuffledTextureMetadata.layout = static_cast<D3D12_TEXTURE_LAYOUT>(image.GetTileMode() | 0x100);
    shuffledTextureMetadata.format = textureMetadata.format;
    shuffledTextureMetadata.swizzleMode = dstorageSwizzleMode;
    shuffledTextureMetadata.loadSize = shuffledCompressedSize;
    shuffledTextureMetadata.uncompressedSize = static_cast<uint32_t>(image.GetSize());
    shuffledTextureMetadata.shuffledComrpessedSize = shuffledCompressedSize;
    shuffledTextureMetadata.unshuffledComrpessedSize = unshuffledCompressedSize;

    unsigned int bcEncoding = (textureMetadata.format - 71) / 3 + 1;
    std::wstring outputFilePath = outputPath + L"BC" + std::to_wstring(bcEncoding) + L"_" + L"shuffled_compressed.bin";

    std::ofstream outFile(outputFilePath, std::ios::binary);
    if (!outFile)
    {
        std::cerr << "Failed to open output file." << std::endl;
        return E_FAIL;
    }

    outFile.write(reinterpret_cast<const char*>(&shuffledTextureMetadata), sizeof(ShuffledTextureMetadata));
    outFile.write(reinterpret_cast<const char*>(pCompressedData), shuffledCompressedSize);

    std::wcout << L"\nTexture data saved: " << outputFilePath << std::endl;

    outFile.close();

    return S_OK;
}
