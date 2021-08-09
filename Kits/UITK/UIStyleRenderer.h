#pragma once
#include "UIStyle.h"

NAMESPACE_ATG_UITK_BEGIN

struct TextString // text, coordinates
{
    std::string text;
    Vector2 coordinates;
};

class UIStyleRenderer
{
public:
    virtual ~UIStyleRenderer() = default;
    virtual void SetupRender() = 0;
    virtual void FinalizeRender() = 0;
    virtual void SetWindowSize(int w, int h) = 0;
    virtual Rectangle GetWindowRectangle() = 0;
    virtual void ClearCaches() = 0;

    virtual void ModifyTextureFromData(
        TextureHandle textureHandle,
        const uint8_t* wicData,
        size_t wicDataSize) = 0;
    virtual TextureHandle CacheTextureFromData(const uint8_t* wicData, size_t wicDataSize) = 0;
    virtual TextureHandle CacheTexture(const std::string& textureFilePath) = 0;
    virtual Rectangle GetTextureRect(TextureHandle textureHandle) = 0;

    virtual void DrawTexturedQuads(
        TextureHandle textureHandle,
        const std::vector<TexturedQuad>& quads) = 0;

    virtual FontHandle CacheFont(FontType fontType, const std::string& fontFilePath, size_t fontSize) = 0;
    virtual int GetScaledFontHeight(FontHandle) = 0;
    virtual int GetUnscaledFontHeight(FontHandle) = 0;
    virtual Rectangle GetScaledFontTextSize(FontHandle, const std::string& text) = 0;
    virtual Rectangle GetUnscaledFontTextSize(FontHandle, const std::string& text) = 0;

    virtual void DrawTextString(FontHandle fontHandle, const TextString& string) = 0;
    virtual void DrawTextStrings(        
        FontHandle fontHandle,
        const std::vector<TextString>& strings) = 0;
   
    virtual void RenderGrid(Vector2 origin, Vector2 size, Vector2 gridSize) = 0;

    virtual size_t IntersectScissorRectangle(const Rectangle& rectInPixels) = 0;
    virtual size_t PushScissorRectangle(const Rectangle& rectInPixels) = 0;
    virtual void PopScissorRectangle(size_t index) = 0;

    virtual size_t PushTintColor(const Color& tintColor) = 0;
    virtual void PopTintColor(size_t index) = 0;
    virtual Color GetCurrentColor() const = 0;

    virtual size_t PushFontTextScale(float textScale) = 0;
    virtual void PopFontTextScale(size_t index) = 0;
    virtual float GetCurrentFontTextScale() const = 0;

    virtual bool IsValidTextureHandle(TextureHandle) { return false; }
    virtual bool IsValidFontHandle(FontHandle) { return false; }

    virtual void SetRotation(UIRotation rotation) = 0;
};

using UIStyleRendererPtr = std::unique_ptr<UIStyleRenderer>;

NAMESPACE_ATG_UITK_END
