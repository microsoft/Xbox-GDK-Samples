//--------------------------------------------------------------------------------------
// File: UIFreeTypeFontRendererD3D.h
//
// Authored by: ATG
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------
#pragma once

#include "DescriptorHeap.h"
#include "DirectXHelpers.h"
#include "SpriteBatch.h"

#include "UILog.h"
#include "UIFontRenderer.h"

// To use TrueType or OpenType, please #define UITK_ENABLE_FREETYPE in pch.h and include GlyphCache in your project
#ifdef UITK_ENABLE_FREETYPE
#include "UnicodeRendering\GlyphCache.h"

NAMESPACE_ATG_UITK_BEGIN

class UIFreeTypeFontRendererD3D final : public UIFontRenderer
{
    DECLARE_CLASS_LOG();

public:
    UIFreeTypeFontRendererD3D(_In_ ID3D12Device* d3dDevice, _In_ ID3D12CommandQueue* commandQueue, _In_ ID3D12GraphicsCommandList* commandList, DirectX::DescriptorPile& resourceDescriptors, DirectX::SpriteBatch& spriteBatch);
    ~UIFreeTypeFontRendererD3D();

    void FinalizeFontRender() override;
    FontHandle CacheFont(const std::string& fontFilePath, size_t fontSize) override;    
    float GetFontHeight(FontHandle) override;    
    Vector2 GetFontTextSize(FontHandle, const std::string& text) override;
    void DrawTextString(FontHandle fontHandle, const std::string& text, Vector2 coordinates, float scale, Color color) override;
    
private:

    struct CachedFreeTypeFont
    {
        CachedFreeTypeFont(size_t fontSize) :
            m_fontSize(fontSize) {}

        void Clear() { m_fontSize = 0; }

        size_t      m_fontSize;
    };
    
    std::vector<CachedFreeTypeFont>             m_fonts;

    std::unique_ptr<ATG::GlyphCache>			m_glyphCache;
    ID3D12Device*                               m_d3dDevice;
    ID3D12CommandQueue*                         m_commandQueue;
    ID3D12GraphicsCommandList*                  m_commandList;
    DirectX::DescriptorPile&                    m_resourceDescriptors;
    DirectX::SpriteBatch&                       m_spriteBatch;
};

NAMESPACE_ATG_UITK_END

#endif
