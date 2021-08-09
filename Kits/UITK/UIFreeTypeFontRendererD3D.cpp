//--------------------------------------------------------------------------------------
// File: UIFreeTypeFontRendererD3D.cpp
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

#include "pch.h"
#include "UIFreeTypeFontRendererD3D.h"

#ifdef UITK_ENABLE_FREETYPE

NAMESPACE_ATG_UITK_BEGIN

INITIALIZE_CLASS_LOG_DEBUG(UIFreeTypeFontRendererD3D);

_Use_decl_annotations_
UIFreeTypeFontRendererD3D::UIFreeTypeFontRendererD3D(ID3D12Device* d3dDevice,
    ID3D12CommandQueue* commandQueue,
    ID3D12GraphicsCommandList* commandList,
    DirectX::DescriptorPile& resourceDescriptors,
    DirectX::SpriteBatch& spriteBatch) :
    m_d3dDevice(d3dDevice),
    m_commandQueue(commandQueue),
    m_commandList(commandList),
    m_resourceDescriptors(resourceDescriptors),
    m_spriteBatch(spriteBatch)
{
    // Setup GlyphCache
    const int maxTextureNum = 4;
    m_glyphCache = std::make_unique<ATG::GlyphCache>(maxTextureNum, 4096, 256, m_d3dDevice);

    size_t start, end;
    m_resourceDescriptors.AllocateRange((size_t)maxTextureNum, start, end);
    m_glyphCache->CreateTextures(m_commandQueue, &m_resourceDescriptors, start);

    // Noto fonts are in Media/Fonts/OFL folder.
    // Make sure that the fonts will be copied under the Assets/Fontos/ folder in the loose directory during the build.
    m_glyphCache->LoadNotoFonts("Assets/Fonts");    
}

UIFreeTypeFontRendererD3D::~UIFreeTypeFontRendererD3D()
{
    for (auto cachedFontIndex = size_t(0); cachedFontIndex < m_fonts.size(); cachedFontIndex++)
    {
        m_fonts[cachedFontIndex].Clear();
    }
    m_fonts.clear();
    m_glyphCache->ClearCache();
}

/*virtual*/ void UIFreeTypeFontRendererD3D::FinalizeFontRender()
{
    m_glyphCache->RenderFrameAdvance();
}

/*virtual*/ FontHandle UIFreeTypeFontRendererD3D::CacheFont(const std::string&, size_t fontSize)
{    
    uint32_t newFontHandle = static_cast<uint32_t>(m_fonts.size());

    // Only caching fontSize here. Actual font caching is done in GlyphCache class
    m_fonts.emplace_back(CachedFreeTypeFont(fontSize));    
    return newFontHandle;
}

/*virtual*/ float UIFreeTypeFontRendererD3D::GetFontHeight(FontHandle fontHandle)
{
    if (size_t(fontHandle) >= m_fonts.size())
    {
        throw std::invalid_argument("Invalid font handle");
    }

    CachedFreeTypeFont& cachedFont = m_fonts[fontHandle];
    return cachedFont.m_fontSize + 2.0f;
}

/*virtual*/ Vector2 UIFreeTypeFontRendererD3D::GetFontTextSize(FontHandle fontHandle, const std::string& text)
{
    if (size_t(fontHandle) >= m_fonts.size())
    {
        throw std::invalid_argument("Invalid font handle");
    }

    int textWidth = 0;
    int textHeight = 0;
    CachedFreeTypeFont& cachedFont = m_fonts[fontHandle];

    m_glyphCache->MeasureText(text.c_str(), (int)cachedFont.m_fontSize, &textWidth, &textHeight);
    
    return Vector2(        
        float(textWidth),
        float(textHeight));
}

/*virtual*/ void UIFreeTypeFontRendererD3D::DrawTextString(FontHandle fontHandle, const std::string& text, Vector2 coordinates, float scale, Color color)
{
    CachedFreeTypeFont& cachedFont = m_fonts[fontHandle];    
    float glyphFontSize = static_cast<float>(cachedFont.m_fontSize) * scale;
   
    m_glyphCache->DrawString(
        m_commandList,
        /* _In_ SpriteBatch* spriteBatch */&m_spriteBatch,
        /* _In_z_ char const* text */ text.c_str(),
        /* XMFLOAT2 const& position */ coordinates,
        /* FXMVECTOR color */color,
        /* float rotation */0.0f,
        /* XMFLOAT2 const& origin */DirectX::XMFLOAT2(0, 0),
        /* float fontSize */glyphFontSize,
        /* SpriteEffects effects */DirectX::SpriteEffects_None,
        /* float layerDepth */0.0f
    );
}

NAMESPACE_ATG_UITK_END

#endif
