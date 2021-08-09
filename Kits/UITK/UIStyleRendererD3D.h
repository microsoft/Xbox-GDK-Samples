//--------------------------------------------------------------------------------------
// File: UIStyleRendererD3D.h
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
#include "SimpleMath.h"
#include "SpriteBatch.h"
#include "PrimitiveBatch.h"
#include "VertexTypes.h"
#include "Effects.h"
#include "Texture.h"
#include "DirectXHelpers.h"
#include "SpriteFont.h"

#include "UILog.h"
#include "UIStyleRenderer.h"
#include "UISpriteFontRendererD3D.h"
#include "UIFreeTypeFontRendererD3D.h"

#include <map>

NAMESPACE_ATG_UITK_BEGIN

using TexturePtr = std::unique_ptr<DX::Texture>;
using FontPtr = std::unique_ptr<DirectX::SpriteFont>;

class D3DResourcesProvider
{
public:
    virtual ~D3DResourcesProvider() = default;
    virtual ID3D12Device* GetD3DDevice() = 0;
    virtual ID3D12CommandQueue* GetCommandQueue() const = 0;
    virtual ID3D12GraphicsCommandList* GetCommandList() const = 0;
};

class UIStyleRendererD3D final : public UIStyleRenderer
{
    DECLARE_CLASS_LOG();
public:
    UIStyleRendererD3D(D3DResourcesProvider& d3dResourcesProvider) : UIStyleRendererD3D(d3dResourcesProvider, c_descriptorPileSize) {};
    UIStyleRendererD3D(D3DResourcesProvider& d3dResourcesProvider, const size_t descriptorPilesize);
    ~UIStyleRendererD3D();

    void SetupRender() override;
    void FinalizeRender() override;
    Rectangle GetWindowRectangle() override;
    void SetWindowSize(int w, int h) override;
    void ClearCaches() override {}

    FontHandle CacheFont(FontType fontType, const std::string& fontFilePath, size_t fontSize) override;
    int GetScaledFontHeight(FontHandle) override;
    int GetUnscaledFontHeight(FontHandle) override;
    Rectangle GetScaledFontTextSize(FontHandle, const std::string& text) override;
    Rectangle GetUnscaledFontTextSize(FontHandle, const std::string& text) override;
    void DrawTextString(FontHandle fontHandle, const TextString& string) override;
    void DrawTextStrings(        
        FontHandle fontHandle,
        const std::vector<TextString>& strings) override;

    void RenderGrid(Vector2 origin, Vector2 size, Vector2 gridSize) override;
    void ModifyTextureFromData(
        TextureHandle textureHandle,
        const uint8_t* wicData,
        size_t wicDataSize) override;
    TextureHandle CacheTextureFromData(const uint8_t* wicData, size_t wicDataSize) override;
    TextureHandle CacheTexture(const std::string& textureFilePath) override;
    Rectangle GetTextureRect(TextureHandle textureHandle) override;
    void DrawTexturedQuads(TextureHandle textureHandle, const std::vector<TexturedQuad>& quads) override;

    size_t IntersectScissorRectangle(const Rectangle& rectInPixels) override;
    size_t PushScissorRectangle(const Rectangle& rectInPixels) override;
    void PopScissorRectangle(size_t index) override;

    size_t PushTintColor(const Color& tintColor) override
    {
        auto index = m_colorStack.size();
        m_colorStack.emplace_back(tintColor);
        return index;
    }

    void PopTintColor(size_t index) override
    {
        // note an error if the index is not legitimate!
        // the index *should* be equal to size() - 1
        assert(m_colorStack.size() > 0 && index == m_colorStack.size() - 1);
        while (index < m_colorStack.size())
        {
            m_colorStack.pop_back();
        }
    }

    Color GetCurrentColor() const override
    {
        if (m_colorStack.size() > 0)
        {
            return m_colorStack.back();
        }
        else
        {
            return c_defaultColor;
        }
    }

    size_t PushFontTextScale(float textScale) override
    {
        auto index = m_fontTextScaleStack.size();
        m_fontTextScaleStack.emplace_back(textScale);
        RefreshCurrentFontTextScale();
        return index;
    }

    void PopFontTextScale(size_t index) override
    {
        // note an error if the index is not legitimate!
        // the index *should* be equal to size() - 1
        assert(m_fontTextScaleStack.size() > 0 && index == m_fontTextScaleStack.size() - 1);
        while (index < m_fontTextScaleStack.size())
        {
            m_fontTextScaleStack.pop_back();
        }
        RefreshCurrentFontTextScale();
    }

    float GetCurrentFontTextScale() const override
    {
        return m_currentFontTextScale;
    }

    bool IsValidTextureHandle(TextureHandle th) override { return th < m_textures.size(); }
    bool IsValidFontHandle(FontHandle fh) override
    {
        return fh < m_fontHandleInfos.size();
    }

    void SetRotation(UIRotation rotation) override
    {
        m_spriteBatch->SetRotation((DXGI_MODE_ROTATION)rotation);
    }

    ID3D12DescriptorHeap* GetDescriptorHeap() const { return m_resourceDescriptors->Heap(); }

private:
    D3DResourcesProvider&                       m_d3dResourcesProvider;
    D3D12_VIEWPORT                              m_d3dViewport;

    struct CachedTexture
    {
        CachedTexture(size_t descriptorIndex, DX::Texture* texturePtr, uint32_t textureHandle) :
            m_descriptorIndex(descriptorIndex),
            m_texturePtr(texturePtr),
            m_textureHandle(textureHandle) {}

        void Clear()
        {
            m_descriptorIndex = c_invalidDescriptor;
            m_texturePtr.reset();
            m_textureHandle = c_invalidTextureHandle;
        }

        size_t      m_descriptorIndex;
        TexturePtr  m_texturePtr;
        uint32_t    m_textureHandle;
    };

    struct FontHandleInfo
    {
        FontHandleInfo(FontType fontType, UIFontRenderer& fontRenderer, FontHandle localFontHandle) :
            m_fontType(fontType),
            m_fontRenderer(fontRenderer),
            m_localFontHandle(localFontHandle) {}

        void Clear()
        {
            m_fontType = FontType::Sprite;            
            m_localFontHandle = c_invalidFontHandle;
        }

        FontType        m_fontType;
        UIFontRenderer& m_fontRenderer;
        FontHandle      m_localFontHandle;
    };

    std::map<ID, TextureHandle>                 m_cachedTextureHandles;
    std::vector<CachedTexture>                  m_textures;
    CachedTexture                               m_defaultTexture;

#ifdef UITK_ENABLE_FREETYPE
    std::unique_ptr<UIFreeTypeFontRendererD3D>  m_freeTypeFontRenderer;
#endif
    std::unique_ptr<UISpriteFontRendererD3D>    m_spriteFontRenderer;
    std::map<ID, FontHandle>                    m_cachedFontHandles;
    std::vector<FontHandleInfo>                 m_fontHandleInfos;

    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_primitiveBatch;
    std::unique_ptr<DirectX::BasicEffect>       m_lineEffect;

    std::vector<Color>                          m_colorStack;
    std::vector<Rectangle>                      m_scissorRectStack;
    std::vector<float>                          m_fontTextScaleStack;
    float                                       m_currentFontTextScale;    

    Microsoft::WRL::ComPtr<ID3D12Fence>         m_fence;
    UINT64                                      m_fenceValue;
    Microsoft::WRL::Wrappers::Event             m_fenceEvent;

    std::mutex                                  m_textureLock;

    /// note: this must be configured according to the needs of the
    /// application that is using this renderer

#ifndef UITK_DESC_PILE_SIZE
#define UITK_DESC_PILE_SIZE 64
#endif//UITK_DESC_PILE_SIZE

    static constexpr size_t c_descriptorPileSize = UITK_DESC_PILE_SIZE;
    static constexpr Color  c_defaultColor = Color(1.0f, 1.0f, 1.0f, 1.0f);

    static constexpr size_t c_invalidTextureHandle = 0xFFFFFFFF;
    static constexpr size_t c_invalidFontHandle = 0xFFFFFFFF;
    static constexpr size_t c_invalidDescriptor = 0xFFFFFFFF;

private:

    void RefreshCurrentFontTextScale()
    {       
        m_currentFontTextScale = 1.0f;
        for (auto scale : m_fontTextScaleStack)
        {
            m_currentFontTextScale *= scale;
        }
    }

    float GetCurrentFontTextScale()
    {
        return m_currentFontTextScale;
    }

    void FlushSpriteBatch();
    void WaitForGpuOperationsCompletion();
};

NAMESPACE_ATG_UITK_END
