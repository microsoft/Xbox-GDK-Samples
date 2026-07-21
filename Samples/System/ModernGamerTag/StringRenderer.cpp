//--------------------------------------------------------------------------------------
// File: StringRenderer.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "StringRenderer.h"
#include "StringTextureAtlas.h"

using namespace DirectX;

namespace TextRenderer
{
    void StringRenderer::CreateDeviceDependentResources(ID3D12CommandQueue* commandQueue, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat)
    {
        // Initialize sprite batch
        RenderTargetState rtState(backBufferFormat, depthBufferFormat);
        SpriteBatchPipelineStateDescription spritePsoDesc(rtState, &CommonStates::AlphaBlend);
        ResourceUploadBatch upload(m_d3dDevice);
        upload.Begin();
        m_spriteBatch = std::make_unique<SpriteBatch>(m_d3dDevice, upload, spritePsoDesc);
        upload.End(commandQueue);
    }

    void StringRenderer::CreateWindowSizeDependentResources(D3D12_VIEWPORT viewport)
    {
        m_spriteBatch->SetViewport(viewport);
    }

    void StringRenderer::DrawString(const char* utf8String, int32_t fontSize, float x, float y)
    {
        std::string stringCopy = utf8String;
        DrawString(stringCopy, fontSize, x, y);
    }

    void StringRenderer::DrawString(std::string utf8String, int32_t fontSize, float x, float y)
    {
        // Don't draw empty strings.
        if (utf8String.size() == 0)
        {
            return;
        }
        // When drawing the string, we first check if it's in the atlas. If not, add it.
        if (m_stringTextureAtlas->GetStringAtlasItem(utf8String, fontSize) == nullptr)
        {
            std::unique_ptr<ShapedString> shapedString = m_stringShaper->ShapeUTF8String(utf8String, fontSize);

            m_stringTextureAtlas->AddShapedStringToAtlas(std::move(shapedString), utf8String, fontSize);
        }

        // Queue string to be rendered to screen. 
        m_pendingStrings.push_back({ utf8String, fontSize, x, y });
    }
    void StringRenderer::RemoveString(const char* utf8String, int32_t fontSize)
    {
        m_stringTextureAtlas->Remove(utf8String, fontSize);
    }

    HRESULT StringRenderer::Render(ID3D12GraphicsCommandList* commandList)
    {
        // For each string in m_pendingStrings queue,
        //      Get the source rect from the texture atlas
        //      Render the source rect to the screen with the positions from the queue of tobe-renderedstrings.
        // Then clear the queue

        // Set spritebatch for drawing to the screen.
        m_spriteBatch->Begin(commandList);
        auto pendingStringIter = m_pendingStrings.begin();
        for (;pendingStringIter != m_pendingStrings.end(); ++pendingStringIter)
        {
            const PendingString pendingDrawString = *pendingStringIter;
            const StringTextureAtlasItem* item = m_stringTextureAtlas->GetStringAtlasItem(pendingDrawString.m_utf8String, pendingDrawString.m_fontSize);

            if (item && item->IsCommited())
            {
                XMFLOAT2 position(pendingDrawString.m_x, pendingDrawString.m_y);
                XMFLOAT2 offset(0, 0);

                // Add 1 to the rect left and top as the glyphs in the atlas have a padding of 1 on all sides.
                // Doing this allows the glyph to be drawn correctly.
                RECT drawtextureRect = item->m_textureRect;
                drawtextureRect.left += 1;
                drawtextureRect.top += 1;
                DirectX::FXMVECTOR color = DirectX::Colors::White;
                m_spriteBatch->Draw(
                    /*SRV*/ m_stringTextureAtlas->m_textureDescriptorHeap->GetGpuHandle(0),
                    /*TextureSize*/XMUINT2(m_stringTextureAtlas->m_dimension, m_stringTextureAtlas->m_dimension),
                    /*Position*/XMLoadFloat2(&position),
                    /*SourceRect*/&drawtextureRect,
                    /*Color*/color,
                    /*Rotation*/0.0f,
                    /*Origin*/XMLoadFloat2(&offset),
                    /*Scale*/1.0f,
                    /*Effects*/SpriteEffects_None,
                    /*LayerDepth*/0
                );
            }
        }
        m_spriteBatch->End();
        m_pendingStrings.clear();
        return S_OK;
    }

    HRESULT StringRenderer::QueuePendingAtlasUpdates(ID3D12GraphicsCommandList* commandList)
    {
        // Render all uncommited textures to the texture atlas.
        m_stringTextureAtlas->CommitUpdates(commandList);
        return S_OK;
    }
}
