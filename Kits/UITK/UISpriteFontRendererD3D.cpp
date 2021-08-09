//--------------------------------------------------------------------------------------
// File: UISpriteFontRendererD3D.cpp
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
#include "UISpriteFontRendererD3D.h"

NAMESPACE_ATG_UITK_BEGIN

INITIALIZE_CLASS_LOG_DEBUG(UISpriteFontRendererD3D);

_Use_decl_annotations_
UISpriteFontRendererD3D::UISpriteFontRendererD3D(ID3D12Device* d3dDevice,
    ID3D12CommandQueue* commandQueue,    
    DirectX::DescriptorPile& resourceDescriptors,
    DirectX::SpriteBatch& spriteBatch ) :
    m_d3dDevice(d3dDevice),
    m_commandQueue(commandQueue),    
    m_resourceDescriptors(resourceDescriptors),
    m_spriteBatch(spriteBatch)
{
}

UISpriteFontRendererD3D::~UISpriteFontRendererD3D()
{
    for (auto cachedFontIndex = size_t(0); cachedFontIndex < m_fonts.size(); cachedFontIndex++)
    {
        m_fonts[cachedFontIndex].Clear();
    }
    m_fonts.clear();
}

/*virtual*/ void UISpriteFontRendererD3D::FinalizeFontRender()
{
    // Nothing to do here
}

/*virtual*/ FontHandle UISpriteFontRendererD3D::CacheFont(const std::string& fontFilePath, size_t)
{
    UILOG_TRACE("Caching font. %s", fontFilePath.c_str());
    auto descriptorHandle = m_resourceDescriptors.Allocate();    
    DirectX::ResourceUploadBatch resourceUploadBatch(m_d3dDevice);
    resourceUploadBatch.Begin();

    auto fontPtr = new DirectX::SpriteFont(
        m_d3dDevice,
        resourceUploadBatch,
        DX::Utf8ToWide(fontFilePath).c_str(),
        m_resourceDescriptors.GetCpuHandle(descriptorHandle),
        m_resourceDescriptors.GetGpuHandle(descriptorHandle));

    auto uploadResourcesFinished = resourceUploadBatch.End(m_commandQueue);
    uploadResourcesFinished.wait();

    uint32_t newFontHandle = static_cast<uint32_t>(m_fonts.size());

    fontPtr->SetDefaultCharacter(L'*');

    m_fonts.emplace_back(CachedSpriteFont(descriptorHandle, fontPtr));    
    UILOG_DEBUG("Font cached. %s", fontFilePath.c_str());

    return newFontHandle;
}


/*virtual*/ float UISpriteFontRendererD3D::GetFontHeight(FontHandle fontHandle)
{
    if (size_t(fontHandle) >= m_fonts.size())
    {
        throw std::invalid_argument("Invalid font handle");
    }

    CachedSpriteFont& cachedFont = m_fonts[fontHandle];
    auto drawHeight = cachedFont.m_fontPtr->GetLineSpacing();

    return drawHeight;
}

/*virtual*/ Vector2 UISpriteFontRendererD3D::GetFontTextSize(FontHandle fontHandle, const std::string& text)
{
    if (size_t(fontHandle) >= m_fonts.size())
    {
        throw std::invalid_argument("Invalid font handle");
    }

    CachedSpriteFont& cachedFont = m_fonts[fontHandle];
    Vector2 textSize = cachedFont.m_fontPtr->MeasureString(text.c_str(), false);

    return textSize;
}

/*virtual*/ void UISpriteFontRendererD3D::DrawTextString(FontHandle fontHandle, const std::string& text, Vector2 coordinates, float scale, Color color)
{
    CachedSpriteFont& cachedFont = m_fonts[fontHandle];
    cachedFont.m_fontPtr->DrawString(
        /* _In_ SpriteBatch* spriteBatch */&m_spriteBatch,
        /* _In_z_ char const* text */text.c_str(),
        /* XMFLOAT2 const& position */coordinates,
        /* FXMVECTOR color */color,
        /* float rotation */0.0f,
        /* XMFLOAT2 const& origin */DirectX::XMFLOAT2(0, 0),
        /* XMFLOAT2 const& scale */DirectX::XMFLOAT2(scale, scale),
        /* SpriteEffects effects */DirectX::SpriteEffects_None,
        /* float layerDepth */0.0f
    );
}

NAMESPACE_ATG_UITK_END

