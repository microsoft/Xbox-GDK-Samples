//--------------------------------------------------------------------------------------
// File: UIStyleRendererD3D.cpp
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

#include "UILog.h"
#include "UIStyleRendererD3D.h"

#include "CommonStates.h"
#include "StringUtil.h"
#include "UIMath.h"

NAMESPACE_ATG_UITK_BEGIN

using namespace DirectX;

INITIALIZE_CLASS_LOG_DEBUG(UIStyleRendererD3D);

#ifdef __clang__
// Suppress false-positive warning
#pragma clang diagnostic ignored "-Wunused-const-variable"
#endif

namespace
{
    constexpr float c_fullyTransparentEpsilon = 1.0f / 255.0f;
    constexpr UINT64 c_InitialFenceValue = 0x0000000010011001;
    constexpr uint32_t c_invalidFontHandle = 0x7FFF0000;

    static UINT64 AvailableFenceValue = c_InitialFenceValue;
}

/*public:*/

UIStyleRendererD3D::UIStyleRendererD3D(D3DResourcesProvider& d3dResourcesProvider, const size_t descriptorPileSize, int w, int h) :
    m_d3dResourcesProvider(d3dResourcesProvider),
    m_defaultTexture(0, nullptr, uint32_t(-1)),
    m_currentFontTextScale(1.0f),
    m_fenceValue(++AvailableFenceValue)
{
    UILOG_TRACE("Initializing D3D Renderer.");

    auto device = m_d3dResourcesProvider.GetD3DDevice();

    device->CreateFence(
        m_fenceValue,
        D3D12_FENCE_FLAG_NONE,
        IID_GRAPHICS_PPV_ARGS(m_fence.ReleaseAndGetAddressOf()));
    m_fence->SetName(L"UIStyleRendererD3D");
    m_fenceEvent.Attach(CreateEventEx(
        nullptr,
        nullptr,
        0,
        EVENT_MODIFY_STATE | SYNCHRONIZE));

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(
        device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        descriptorPileSize);

    DirectX::ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    RenderTargetState rtState(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT);
    SpriteBatchPipelineStateDescription pd(
        rtState,
        &CommonStates::NonPremultiplied,
        nullptr,
        nullptr,
        nullptr);

    m_spriteBatch = std::make_unique<SpriteBatch>(
        device,
        resourceUpload,
        pd);

    m_primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(device);

    EffectPipelineStateDescription epd(
        &VertexPositionColor::InputLayout,
        CommonStates::Opaque,
        CommonStates::DepthNone,
        CommonStates::CullNone,
        rtState,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

    m_lineEffect = std::make_unique<BasicEffect>(device,
        EffectFlags::VertexColor,
        epd);

    SetWindowSize(w, h);

    m_lineEffect->SetProjection(XMMatrixOrthographicOffCenterRH(0,
        m_d3dViewport.Width, m_d3dViewport.Height, 0, 0, 1));

    auto descriptorHandle = m_resourceDescriptors->Allocate();
    m_defaultTexture.m_descriptorIndex = descriptorHandle;
    m_defaultTexture.m_texturePtr = DX::Texture::CreateDefaultTexture(
        device,
        resourceUpload,
        m_resourceDescriptors->GetCpuHandle(descriptorHandle));

    auto commandQueue = m_d3dResourcesProvider.GetCommandQueue();
    auto uploadResourcesFinished = resourceUpload.End(commandQueue);
    uploadResourcesFinished.wait();

    m_spriteFontRenderer = std::make_unique<UISpriteFontRendererD3D>(m_d3dResourcesProvider.GetD3DDevice(), m_d3dResourcesProvider.GetCommandQueue(), *m_resourceDescriptors, *m_spriteBatch);
#ifdef UITK_ENABLE_FREETYPE
    m_freeTypeFontRenderer = std::make_unique<UIFreeTypeFontRendererD3D>(m_d3dResourcesProvider.GetD3DDevice(), m_d3dResourcesProvider.GetCommandQueue(), m_d3dResourcesProvider.GetCommandList(), *m_resourceDescriptors, *m_spriteBatch);
#endif

    UILOG_DEBUG("D3D Renderer initialized.");
}

UIStyleRendererD3D::~UIStyleRendererD3D()
{
    WaitForGpuOperationsCompletion();

    m_defaultTexture.Clear();

    m_cachedTextureHandles.clear();

    for (auto cachedTextureIndex = size_t(0); cachedTextureIndex < m_textures.size(); cachedTextureIndex++)
    {
        m_textures[cachedTextureIndex].Clear();
    }
    m_textures.clear();

    m_cachedFontHandles.clear();

    for (auto cachedFontHandleIndex = size_t(0); cachedFontHandleIndex < m_fontHandleInfos.size(); cachedFontHandleIndex++)
    {
        m_fontHandleInfos[cachedFontHandleIndex].Clear();
    }
    m_fontHandleInfos.clear();

    m_lineEffect.reset();
    m_primitiveBatch.reset();
    m_spriteBatch.reset();

    m_resourceDescriptors.reset();

    m_colorStack.clear();
    m_scissorRectStack.clear();

    m_spriteFontRenderer.reset();
#ifdef UITK_ENABLE_FREETYPE
    m_freeTypeFontRenderer.reset();
#endif
}

/*virtual*/ void UIStyleRendererD3D::SetupRender()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_PreRender");
    auto commandList = m_d3dResourcesProvider.GetCommandList();
    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);
    m_spriteBatch->Begin(commandList);
}

void UIStyleRendererD3D::FinalizeRender()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_FinalizeRender");
    m_spriteBatch->End();
    m_spriteFontRenderer->FinalizeFontRender();
#ifdef UITK_ENABLE_FREETYPE
    m_freeTypeFontRenderer->FinalizeFontRender();
#endif
}

/*virtual*/ void UIStyleRendererD3D::SetWindowSize(int w, int h)
{
    m_d3dViewport.TopLeftX = 0.0f;
    m_d3dViewport.TopLeftY = 0.0f;
    m_d3dViewport.Width = static_cast<float>(w);
    m_d3dViewport.Height = static_cast<float>(h);
    m_d3dViewport.MinDepth = D3D12_MIN_DEPTH;
    m_d3dViewport.MaxDepth = D3D12_MAX_DEPTH;

    m_spriteBatch->SetViewport(m_d3dViewport);
    m_lineEffect->SetProjection(XMMatrixOrthographicOffCenterRH(0,
        m_d3dViewport.Width, m_d3dViewport.Height, 0, 0, 1));
}

/*virtual*/ Rectangle UIStyleRendererD3D::GetWindowRectangle()
{
    return Rectangle(0, 0, static_cast<int>(m_d3dViewport.Width), static_cast<int>(m_d3dViewport.Height));
}

/*virtual*/ void UIStyleRendererD3D::ModifyTextureFromData(
    TextureHandle textureHandle,
    const uint8_t* wicData,
    size_t wicDataSize)
{
    UILOG_SCOPE("ModifyTextureFromData");
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_ModifyTextureFromData");

    // bad parameter checking
    if (!IsValidTextureHandle(textureHandle))
    {
        UILOG_ERROR("UIStyleRendererD3D::ModifyTextureFromData() passed in an invalid texture handle.");
        return;
    }

    if (!wicData || !wicDataSize)
    {
        UILOG_ERROR("UIStyleRendererD3D::ModifyTextureFromData() passed in empty texture data.");
        return;
    }

    UILOG_DEBUG("Modifying texture handle (%d) from data of size. %d", textureHandle, wicDataSize);

    // TODO: double check with the graphics team to see if from a performance standpoint
    // and also from an example standpoint, that this is the best and most efficient way
    // to modify an existing resource bound to the same resource handle at runtime...
    WaitForGpuOperationsCompletion();

    CachedTexture& cachedTexture = m_textures[textureHandle];
    auto descriptorHandle = cachedTexture.m_descriptorIndex;
    cachedTexture.Clear();

    auto device = m_d3dResourcesProvider.GetD3DDevice();
    auto commandQueue = m_d3dResourcesProvider.GetCommandQueue();

    DirectX::ResourceUploadBatch resourceUploadBatch(device);
    resourceUploadBatch.Begin();

    auto texturePtr = new DX::Texture(
        device,
        resourceUploadBatch,
        m_resourceDescriptors->GetCpuHandle(descriptorHandle),
        wicData,
        wicDataSize);

    auto uploadResourcesFinished = resourceUploadBatch.End(commandQueue);
    uploadResourcesFinished.wait();

    m_textures[textureHandle] = CachedTexture(descriptorHandle, texturePtr, textureHandle);

    UILOG_DEBUG("Texture handle (%d) re-cached of size. %d", textureHandle, wicDataSize);
}

/*virtual*/ TextureHandle UIStyleRendererD3D::CacheTextureFromData(const uint8_t* wicData, size_t wicDataSize)
{
    UILOG_SCOPE("CacheTextureFromData");
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_CacheTextureFromData");

    // bad parameter checking
    if (!wicData || !wicDataSize)
    {
        UILOG_ERROR("UIStyleRendererD3D::CacheTextureFromData() passed in empty texture data.");
        return c_invalidFontHandle;
    }

    UILOG_TRACE("Caching texture from data of size. %d", wicDataSize);

    auto device = m_d3dResourcesProvider.GetD3DDevice();
    auto commandQueue = m_d3dResourcesProvider.GetCommandQueue();

    auto descriptorHandle = m_resourceDescriptors->Allocate();

    DirectX::ResourceUploadBatch resourceUploadBatch(device);
    resourceUploadBatch.Begin();

    auto texturePtr = new DX::Texture(
        device,
        resourceUploadBatch,
        m_resourceDescriptors->GetCpuHandle(descriptorHandle),
        wicData,
        wicDataSize);

    auto uploadResourcesFinished = resourceUploadBatch.End(commandQueue);
    uploadResourcesFinished.wait();

    uint32_t newTextureHandle = static_cast<uint32_t>(m_textures.size());

    m_textures.emplace_back(CachedTexture(descriptorHandle, texturePtr, newTextureHandle));

    UILOG_DEBUG("Texture cached of size. %d", wicDataSize);

    return newTextureHandle;
}

/*virtual*/ TextureHandle UIStyleRendererD3D::CacheTexture(const std::string& textureFilePath)
{
    UILOG_SCOPE("CacheTexture");
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_CacheTexture");
    ID textureFileId = ID(textureFilePath);

    // bad parameter checking
    if (!textureFileId)
    {
        return c_invalidFontHandle;
    }

    const auto& iter = m_cachedTextureHandles.find(textureFileId);

    if (iter != m_cachedTextureHandles.end())
    {
        return iter->second;
    }
    else
    {
        PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_CacheTexture_Add");
        UILOG_TRACE("Caching texture. %s", textureFilePath.c_str());

        auto device = m_d3dResourcesProvider.GetD3DDevice();
        auto commandQueue = m_d3dResourcesProvider.GetCommandQueue();

        auto descriptorHandle = m_resourceDescriptors->Allocate();

        DirectX::ResourceUploadBatch resourceUploadBatch(device);
        resourceUploadBatch.Begin();

        auto texturePtr = new DX::Texture(
            device,
            resourceUploadBatch,
            m_resourceDescriptors->GetCpuHandle(descriptorHandle),
            DX::Utf8ToWide(textureFilePath).c_str());

        auto uploadResourcesFinished = resourceUploadBatch.End(commandQueue);
        uploadResourcesFinished.wait();

        uint32_t newTextureHandle = static_cast<uint32_t>(m_textures.size());

        m_textures.emplace_back(CachedTexture(descriptorHandle, texturePtr, newTextureHandle));

        m_cachedTextureHandles[textureFileId] = newTextureHandle;

        UILOG_DEBUG("Texture cached. %s", textureFilePath.c_str());

        return newTextureHandle;
    }
}

/*virtual*/ Rectangle UIStyleRendererD3D::GetTextureRect(TextureHandle textureHandle)
{
    if (size_t(textureHandle) >= m_textures.size())
    {
        return Rectangle();
    }

    CachedTexture& cachedTexture = m_textures[textureHandle];
    auto w = cachedTexture.m_texturePtr->Width();
    auto h = cachedTexture.m_texturePtr->Height();

    return Rectangle(0, 0, w, h);
}

/*virtual*/ void UIStyleRendererD3D::DrawTexturedQuads(
    TextureHandle textureHandle,
    const std::vector<TexturedQuad>& quads)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_DrawTexturedQuads");
    auto currentColor = GetCurrentColor();

    if (currentColor.A() < c_fullyTransparentEpsilon)
    {
        return;
    }

    // bad parameter checking
    CachedTexture& cachedTexture = IsValidTextureHandle(textureHandle) ?
        m_textures[textureHandle] :
        m_defaultTexture;

    uint32_t w = static_cast<uint32_t>(cachedTexture.m_texturePtr->Width());
    uint32_t h = static_cast<uint32_t>(cachedTexture.m_texturePtr->Height());

    for (const auto& quad : quads)
    {
        const auto& sourceTextureRectangle = quad.first;
        const auto& destinationScreenRectangle = quad.second;

        RECT textureRectangle = Rectangle(sourceTextureRectangle);
        RECT screenRectangle = Rectangle(destinationScreenRectangle);

        m_spriteBatch->Draw(
            /*D3D12_GPU_DESCRIPTOR_HANDLE*/m_resourceDescriptors->GetGpuHandle(cachedTexture.m_descriptorIndex),
            /*XMUINT2 const&*/XMUINT2(w, h),
            /*RECT const& destinationRectangle*/screenRectangle,
            /*_In_opt_ RECT const* sourceRectangle*/&textureRectangle,
            /*FXMVECTOR color*/currentColor,
            /*float rotation*/0,
            /*XMFLOAT2 const& origin*/XMFLOAT2(0, 0),
            /*SpriteEffects effects*/SpriteEffects_None,
            /*float layerDepth*/0
        );
    }
}

/*virtual*/ FontHandle UIStyleRendererD3D::CacheFont(FontType fontType, const std::string& fontFilePath, size_t fontSize)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_CacheFont");
    UILOG_SCOPE("CacheFont");
    ID fontFileId;

    switch (fontType)
    {
    case FontType::Sprite:
        if (!Util::FileExists(fontFilePath))
        {
            UILOG_ERROR("UIStyleRendererD3D::CacheFont() passed in a non-existent font file path: ");
            UILOG_ERROR(fontFilePath.c_str());
            return c_invalidFontHandle;
        }
        fontFileId = ID(fontFilePath);
        break;
    case FontType::FreeType:
#ifdef UITK_ENABLE_FREETYPE
        // In case of FreeType, currently NotoFonts will be used in every case
        // Therefore, only fontSize information is considered
        fontFileId = ID(std::to_string(fontSize));
#else
        throw std::invalid_argument("FreeType is not enabled. To use TrueType or OpenType, please #define UITK_ENABLE_FREETYPE in pch.h and include GlyphCache in your project.");
#endif
        break;
    default:
        throw std::logic_error("FontType is not implemented");
        return 0;
    }

    // bad parameter checking
    if (!fontFileId)
    {
        UILOG_ERROR("UIStyleRendererD3D::CacheFont() passed in empty font file path.");
        return c_invalidFontHandle;
    }

    const auto& iter = m_cachedFontHandles.find(fontFileId);

    if (iter != m_cachedFontHandles.end())
    {
        return iter->second;
    }

    uint32_t newFontHandle = static_cast<uint32_t>(m_fontHandleInfos.size());
    m_cachedFontHandles[fontFileId] = newFontHandle;

    FontHandle localFontHandle = 0;

    switch(fontType)
    {
    case FontType::Sprite:
        localFontHandle = m_spriteFontRenderer->CacheFont(fontFilePath, fontSize);
        m_fontHandleInfos.emplace_back(FontHandleInfo(fontType, *m_spriteFontRenderer, localFontHandle));
        break;
    case FontType::FreeType:
#ifdef UITK_ENABLE_FREETYPE
        localFontHandle = m_freeTypeFontRenderer->CacheFont(fontFilePath, fontSize);
        m_fontHandleInfos.emplace_back(FontHandleInfo(fontType, *m_freeTypeFontRenderer, localFontHandle));
#endif
        break;
    default:
        throw std::logic_error("FontType is not implemented");
        return 0;
    }

    return newFontHandle;
}

/*virtual*/ int UIStyleRendererD3D::GetScaledFontHeight(FontHandle fontHandle)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_GetScaledFontHeight");

    // we purposefully throw an exception so that when running in the debugger,
    // one can see where the invalid font handle is coming from
    if (size_t(fontHandle) >= m_fontHandleInfos.size())
    {
        throw std::invalid_argument("Invalid font handle");
    }

    FontHandleInfo& fontHandleInfo = m_fontHandleInfos[fontHandle];
    UIFontRenderer& fontRenderer = fontHandleInfo.m_fontRenderer;
    FontHandle localFontHandle = fontHandleInfo.m_localFontHandle;
    auto drawHeight = int(fontRenderer.GetFontHeight(localFontHandle) * GetCurrentFontTextScale());

    return drawHeight;
}

/*virtual*/ int UIStyleRendererD3D::GetUnscaledFontHeight(FontHandle fontHandle)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_GetUnscaledFontHeight");

    // we purposefully throw an exception so that when running in the debugger,
    // one can see where the invalid font handle is coming from
    if (size_t(fontHandle) >= m_fontHandleInfos.size())
    {
        throw std::invalid_argument("Invalid font handle");
    }

    FontHandleInfo& fontHandleInfo = m_fontHandleInfos[fontHandle];
    UIFontRenderer& fontRenderer = fontHandleInfo.m_fontRenderer;
    FontHandle localFontHandle = fontHandleInfo.m_localFontHandle;
    auto drawHeight = int(fontRenderer.GetFontHeight(localFontHandle));

    return drawHeight;
}

/*virtual*/ Rectangle UIStyleRendererD3D::GetScaledFontTextSize(FontHandle fontHandle, const std::string& text)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_GetScaledFontTextSize");

    // we purposefully throw an exception so that when running in the debugger,
    // one can see where the invalid font handle is coming from
    if (size_t(fontHandle) >= m_fontHandleInfos.size())
    {
        throw std::invalid_argument("Invalid font handle");
    }

    FontHandleInfo& fontHandleInfo = m_fontHandleInfos[fontHandle];
    UIFontRenderer& fontRenderer = fontHandleInfo.m_fontRenderer;
    FontHandle localFontHandle = fontHandleInfo.m_localFontHandle;
    Vector2 drawBounds = fontRenderer.GetFontTextSize(localFontHandle, text);

    return Rectangle(
        0, 0,
        long(drawBounds.x * GetCurrentFontTextScale()),
        long(drawBounds.y * GetCurrentFontTextScale()));
}

/*virtual*/ Rectangle UIStyleRendererD3D::GetUnscaledFontTextSize(FontHandle fontHandle, const std::string& text)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_GetUnscaledFontTextSize");

    // we purposefully throw an exception so that when running in the debugger,
    // one can see where the invalid font handle is coming from
    if (size_t(fontHandle) >= m_fontHandleInfos.size())
    {
        throw std::invalid_argument("Invalid font handle");
    }

    FontHandleInfo& fontHandleInfo = m_fontHandleInfos[fontHandle];
    UIFontRenderer& fontRenderer = fontHandleInfo.m_fontRenderer;
    FontHandle localFontHandle = fontHandleInfo.m_localFontHandle;
    Vector2 drawBounds = fontRenderer.GetFontTextSize(localFontHandle, text);

    return Rectangle(
        0, 0,
        long(drawBounds.x),
        long(drawBounds.y));
}

void UIStyleRendererD3D::DrawTextString(FontHandle fontHandle, const TextString& string)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_DrawTextString");
    auto currentColor = GetCurrentColor();

    if (currentColor.A() < c_fullyTransparentEpsilon)
    {
        return;
    }

    if (size_t(fontHandle) >= m_fontHandleInfos.size())
    {
        UILOG_WARN("Invalid font handle (%d) passed to UIStyleRendererD3D::DrawTextString()", fontHandle);
        return;
    }

    FontHandleInfo& fontHandleInfo = m_fontHandleInfos[fontHandle];
    UIFontRenderer& fontRenderer = fontHandleInfo.m_fontRenderer;
    FontHandle localFontHandle = fontHandleInfo.m_localFontHandle;

    fontRenderer.DrawTextString(localFontHandle, string.text, string.coordinates, GetCurrentFontTextScale(), currentColor);
}

/*virtual*/ void UIStyleRendererD3D::DrawTextStrings(
    FontHandle fontHandle,
    const std::vector<TextString>& strings)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_DrawTextStrings");
    auto currentColor = GetCurrentColor();

    if (currentColor.A() < c_fullyTransparentEpsilon)
    {
        return;
    }

    if (size_t(fontHandle) >= m_fontHandleInfos.size())
    {
        UILOG_WARN("Invalid font handle (%d) passed to UIStyleRendererD3D::DrawTextStrings()", fontHandle);
        return;
    }

    FontHandleInfo& fontHandleInfo = m_fontHandleInfos[fontHandle];
    UIFontRenderer& fontRenderer = fontHandleInfo.m_fontRenderer;
    FontHandle localFontHandle = fontHandleInfo.m_localFontHandle;

    for (const auto& string : strings)
    {
        fontRenderer.DrawTextString(localFontHandle, string.text, string.coordinates, GetCurrentFontTextScale(), currentColor);
    }
}

/*virtual*/ void UIStyleRendererD3D::RenderGrid(Vector2 origin, Vector2 size, Vector2 gridSize)
{
    auto commandList = m_d3dResourcesProvider.GetCommandList();
    auto color = GetCurrentColor();
    m_lineEffect->SetColorAndAlpha(color);
    m_lineEffect->Apply(commandList);
    m_primitiveBatch->Begin(commandList);

    float offsetWidthX = origin.x + size.x;
    float offsetHeightY = origin.y + size.y;

    for (float f = origin.x; f < offsetWidthX; f += gridSize.x)
    {
        DirectX::VertexPositionColor v1({ f, origin.y, 0.f }, color);
        DirectX::VertexPositionColor v2({ f, offsetHeightY, 0.f }, color);
        m_primitiveBatch->DrawLine(v1, v2);
    }

    for (float f = origin.y; f < offsetHeightY; f += gridSize.y)
    {
        DirectX::VertexPositionColor v1({ origin.x, f, 0.f }, color);
        DirectX::VertexPositionColor v2({ offsetWidthX, f, 0.f }, color);
        m_primitiveBatch->DrawLine(v1, v2);
    }

    m_primitiveBatch->End();
}

// TODO (scmatlof): Move to an enum value optional parameter in PushScissorRectangle
/*virtual*/ size_t UIStyleRendererD3D::IntersectScissorRectangle(const Rectangle& rectangle)
{
    if (m_scissorRectStack.size() > 0)
    {
        auto activeRectangle = m_scissorRectStack.back();
        auto intersection = Rectangle::Intersect(activeRectangle, rectangle);
        return PushScissorRectangle(intersection);
    }
    else
    {
        return PushScissorRectangle(rectangle);
    }
}

/*virtual*/ size_t UIStyleRendererD3D::PushScissorRectangle(const Rectangle& rectangle)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_PushScissorRectangle");
    auto index = m_scissorRectStack.size();
    m_scissorRectStack.emplace_back(rectangle);

    RECT scissorRect = Rectangle(rectangle);

    auto commandList = m_d3dResourcesProvider.GetCommandList();
    FlushSpriteBatch();
    commandList->RSSetScissorRects(1, &scissorRect);

    return index;
}

/*virtual*/ void UIStyleRendererD3D::PopScissorRectangle(size_t index)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_PopScissorRectangle");
    // note an error if the index is not legitimate!
    // the index *should* be equal to size() - 1
    assert(m_scissorRectStack.size() > 0 && index == m_scissorRectStack.size() - 1);

    auto commandList = m_d3dResourcesProvider.GetCommandList();
    FlushSpriteBatch();

    while (index < m_scissorRectStack.size())
    {
        m_scissorRectStack.pop_back();
    }

    if (m_scissorRectStack.size() > 0)
    {
        RECT scissorRect = Rectangle(m_scissorRectStack.back());
        commandList->RSSetScissorRects(1, &scissorRect);
    }
    else
    {
        RECT scissorRect = GetWindowRectangle();
        commandList->RSSetScissorRects(1, &scissorRect);
    }
}

void UIStyleRendererD3D::FlushSpriteBatch()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_FlushSpriteBatch");
    auto commandList = m_d3dResourcesProvider.GetCommandList();
    m_spriteBatch->End();
    m_spriteBatch->Begin(commandList);
}

void UIStyleRendererD3D::WaitForGpuOperationsCompletion()
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"UIStyleRendererD3D_WaitForGpu");
    auto commandQueue = m_d3dResourcesProvider.GetCommandQueue();

    // Schedule a Signal command in the GPU queue.
    auto fenceValue = m_fence->GetCompletedValue() + 1;
    if (SUCCEEDED(commandQueue->Signal(m_fence.Get(), fenceValue)))
    {
        if (SUCCEEDED(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent.Get())))
        {
            WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);
        }
    }
}

NAMESPACE_ATG_UITK_END
