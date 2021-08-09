//--------------------------------------------------------------------------------------
// File: UISpriteFontRendererD3D.h
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
#include "Texture.h"
#include "DirectXHelpers.h"
#include "SpriteFont.h"

#include "UILog.h"
#include "UIFontRenderer.h"

using FontPtr = std::unique_ptr<DirectX::SpriteFont>;

NAMESPACE_ATG_UITK_BEGIN

class UISpriteFontRendererD3D final : public UIFontRenderer
{
    DECLARE_CLASS_LOG();
public:
    UISpriteFontRendererD3D(_In_ ID3D12Device* d3dDevice, _In_ ID3D12CommandQueue* commandQueue, DirectX::DescriptorPile& resourceDescriptors, DirectX::SpriteBatch& spriteBatch);
    ~UISpriteFontRendererD3D();

    void FinalizeFontRender() override;
    FontHandle CacheFont(const std::string& fontFilePath, size_t fontSize) override;    
    float GetFontHeight(FontHandle) override;    
    Vector2 GetFontTextSize(FontHandle, const std::string& text) override;
    void DrawTextString(FontHandle fontHandle, const std::string& text, Vector2 coordinates, float scale, Color color) override;
    
private:
    struct CachedSpriteFont
    {
        CachedSpriteFont(size_t descriptorIndex, DirectX::SpriteFont* fontPtr) :
            m_descriptorIndex(descriptorIndex),
            m_fontPtr(fontPtr)
        {}

        void Clear()
        {
            m_descriptorIndex = c_invalidDescriptor;
            m_fontPtr.reset();            
        }


        size_t      m_descriptorIndex;
        FontPtr     m_fontPtr;        
    };
    
    std::vector<CachedSpriteFont>               m_fonts;

    ID3D12Device*                               m_d3dDevice;
    ID3D12CommandQueue*                         m_commandQueue;    
    DirectX::DescriptorPile&                    m_resourceDescriptors;
    DirectX::SpriteBatch&                       m_spriteBatch;
};

NAMESPACE_ATG_UITK_END
