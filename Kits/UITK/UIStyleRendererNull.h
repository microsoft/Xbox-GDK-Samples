//--------------------------------------------------------------------------------------
// File: UIStyleRendererNull.h
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

#include "UIStyleManager.h"

NAMESPACE_ATG_UITK_BEGIN

using Rectangle = DirectX::SimpleMath::Rectangle;

class UIStyleRendererNull : public UIStyleRenderer
{
    // intentionally do nothing
    void SetupRender() override {}

    // intentionally do nothing
    void FinalizeRender() override {}

    // intentionally do nothing
    void SetWindowSize(int, int) override {}

    // intentionally do nothing
    Rectangle GetWindowRectangle() override { return Rectangle(); }

    // intentionally do nothing
    void ClearCaches() override {}

    // intentionally do nothing
    void ModifyTextureFromData(TextureHandle, const uint8_t*, size_t) override {}

    // intentionally do nothing
    TextureHandle CacheTextureFromData(const uint8_t*, size_t) override
    {
        // intentionally return nothing;
        return 0xFFFFFFFF;
    }

    // intentionally do nothing
    TextureHandle CacheTexture(const std::string&) override
    {
        // intentionally return nothing
        return 0;
    }

    // intentionally do nothing
    int GetScaledFontHeight(FontHandle) override
    {
        // intentionally return nothing
        return 0;
    }

    // intentionally do nothing
    int GetUnscaledFontHeight(FontHandle) override
    {
        // intentionally return nothing
        return 0;
    }

    // intentionally do nothing
    Rectangle GetTextureRect(TextureHandle) override
    {
        // intentionally return nothing
        return Rectangle();
    }

    // intentionally do nothing
    void DrawTexturedQuads(
        TextureHandle,
        const std::vector<TexturedQuad>&) override {}

    // intentionally do nothing
    FontHandle CacheFont(FontType, const std::string&, size_t) override
    {
        // intentionally return nothing
        return 0;
    }

    // intentionally do nothing
    Rectangle GetScaledFontTextSize(FontHandle, const std::string&) override
    {
        // intentionally return nothing
        return Rectangle();
    }

    // intentionally do nothing
    Rectangle GetUnscaledFontTextSize(FontHandle, const std::string&) override
    {
        // intentionally return nothing
        return Rectangle();
    }

    // intentionally do nothing
    void DrawTextString(        
        FontHandle,
        const TextString&) override {}

    // intentionally do nothing
    void DrawTextStrings(
        FontHandle,
        const std::vector<TextString>&) override {}
    
    // intentionally do nothing
    void RenderGrid(Vector2, Vector2, Vector2) override {}

    // intentionally do nothing
    size_t IntersectScissorRectangle(const Rectangle&) override
    {
        return 0;
    }

    // intentionally do nothing
    size_t PushScissorRectangle(const Rectangle&) override
    {
        // intentionally return nothing
        return 0;
    }

    // intentionally do nothing
    void PopScissorRectangle(size_t) override {}

    // intentionally do nothing
    size_t PushTintColor(const Color&) override
    {
        // intentionally return nothing
        return 0;
    }

    // intentionally do nothing
    void PopTintColor(size_t) override {}

    // intentionally do nothing
    Color GetCurrentColor() const override
    {
        // intentionally return a default value
        return Vector4::One;
    }

    // intentionally do nothing
    size_t PushFontTextScale(float) override
    {
        // intentionally return nothing
        return 0;
    }

    // intentionally do nothing
    void PopFontTextScale(size_t) override {}

    // intentionally do nothing
    float GetCurrentFontTextScale() const override
    {
        // intentionally return a default
        return 1.0f;
    }

    // intentionally do nothing
    void SetRotation(UIRotation) override {}
};

NAMESPACE_ATG_UITK_END
