//--------------------------------------------------------------------------------------
// File: StringRenderer.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DescriptorHeap.h"
#include "StringTextureAtlas.h"
#include "StringShaper.h"

#include "SpriteBatch.h"
#include "ResourceUploadBatch.h"

namespace TextRenderer
{
    constexpr uint32_t                                      DEPTH = 1;
    constexpr uint32_t                                      MIP_COUNT = 1;
    constexpr uint32_t                                      ARRAY_SIZE = 1;
    constexpr D3D12_RESOURCE_DIMENSION                      RESOURCE_DIMENSION = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    constexpr DXGI_FORMAT                                   FORMAT = DXGI_FORMAT_R8_UNORM;
    constexpr D3D12_RESOURCE_FLAGS                          RESOURCE_FLAGS = D3D12_RESOURCE_FLAG_NONE;

    // Represents a string that needs to be rendered to the screen at a certain position.
    struct PendingString
    {
    public:
        std::string                                         m_utf8String;
        int32_t                                             m_fontSize;

        // Positions to render String to the screen
        float                                               m_x;
        float                                               m_y;
    };

    class StringRenderer
    {
    public:
        StringRenderer(StringShaper* stringShaper, StringTextureAtlas* textureAtlas, ID3D12Device* device) :
            m_d3dDevice(device),
            m_stringShaper(stringShaper),
            m_stringTextureAtlas(textureAtlas)
        {
        }

        // Creates a pending string to be drawn to the screen. All strings need be cached to the atlas by
        // calling QueuePendingUpdates. A string can't be rendered to the screen until it has been cached to the atlas.
        void DrawString(const char* utf8String, int32_t fontSize, float x, float y);
        void DrawString(std::string utf8String, int32_t fontSize, float x, float y);

        // Caches/Renders all uncached strings to the texture atlas.
        HRESULT QueuePendingAtlasUpdates(ID3D12GraphicsCommandList* commandList);

        // Renders all CACHED pending strings to the screen.
        HRESULT Render(ID3D12GraphicsCommandList* commandList);

        // Removes a string from the texture atlas. There is currently NO eviction policy, this is left as an exercise
        // to the reader. One option is to create an eviction policy in the StringTextureAtlas.
        void RemoveString(const char* utf8String, int32_t fontSize);

        // DX12 functions
        void CreateDeviceDependentResources(ID3D12CommandQueue* commandQueue, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat);
        void CreateWindowSizeDependentResources(D3D12_VIEWPORT viewport);

    private:
        // DX12 fields
        ID3D12Device*                                       m_d3dDevice;
        std::unique_ptr<DirectX::DX12::SpriteBatch>         m_spriteBatch;
        StringShaper*                                       m_stringShaper;
        StringTextureAtlas*                                 m_stringTextureAtlas;

        // A list of strings that will need to be drawn to the screen.
        std::vector<PendingString>                          m_pendingStrings;
    };
}

