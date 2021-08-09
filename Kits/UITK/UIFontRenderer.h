//--------------------------------------------------------------------------------------
// File: UIFontRenderer.h
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

NAMESPACE_ATG_UITK_BEGIN

class UIFontRenderer
{
public:
    virtual ~UIFontRenderer() = default;
    virtual void FinalizeFontRender() = 0;
    virtual FontHandle CacheFont(const std::string& fontFilePath, size_t fontSize) = 0;    
    virtual float GetFontHeight(FontHandle) = 0;    
    virtual Vector2 GetFontTextSize(FontHandle, const std::string& text) = 0;
    virtual void DrawTextString(FontHandle fontHandle, const std::string& text, Vector2 coordinates, float scale, Color color) = 0;

protected:
    static constexpr size_t c_invalidDescriptor = 0xFFFFFFFF;
};

NAMESPACE_ATG_UITK_END

