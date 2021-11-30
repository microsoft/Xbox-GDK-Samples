//--------------------------------------------------------------------------------------
// GlyphCache.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <cstddef>

#include "ATGColors.h"

namespace DirectX {
    class SpriteBatch;
    class DescriptorHeap;
    enum SpriteEffects : uint32_t;
}

namespace ATG
{

    enum class UnicodeRange
    {
        // Any character that does not fall within below ranges
        Other,

        // Symbols such as '!', ':', '?', etc
        LatinSymbols,

        // Latin alpha and numeric symbols, such as 'A', '1', ' ', etc.
        LatinAlphaNumberic,

        // Supplemental and extended Latin characters, such as accented Latin letters
        LatinSupplementalExtended,
        
        // Icons
        PrivateUseArea,

        // Language-specific ranges
        Korean,
        Japanese,
        Chinese,
        Russian,
        Bengali,
        Thai,
        Greek,
        Hindi,
        Arabic,
        Hebrew
    };

    // The glyph cache provides functionality to cache Unicode glyphs to texture atlases based on loaded fonts using the FreeType2 open-source library.
    // In addition, this class can render Unicode strings and provide measuring and string conversions between UTF8 and UTF32.
    // The GlyphCache is thread-safe, however it's not efficient when used simultaneously on multiple threads. The safety is provided via naive locking of methods.
    class GlyphCache
    {
    public:

        // The maximum number of cached glyphs is based on the maximum textures this cache can have.
        // Once those textures fill up, old glyphs will be emptied and overwritten based on usage.
        // Using one texture causes the most efficient string rendering, but can churn glyph upload/cache if a lot of different glyphs are used.
        // Using multiple textures causes a more efficient cache with less glyph upload/churn, but less efficient individual string draws.
        // A shaped string is the final render representation of glyph positions, glyph reordering, etc. A single input string may have multiple
        // internal shaped strings based on language changes inside of the string.
        GlyphCache(size_t maxTextures, LONG textureDimension, size_t maxCachedShapedStrings, _In_ ID3D12Device* d3dDevice);

        GlyphCache(GlyphCache&&) noexcept;
        GlyphCache& operator= (GlyphCache&&) noexcept;

        GlyphCache(GlyphCache const&) = delete;
        GlyphCache& operator= (GlyphCache const&) = delete;

        virtual ~GlyphCache();

        // Initialize graphics resources
        void CreateDeviceDependentResources(_In_ ID3D12CommandQueue* commandQueue, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat);
        void CreateWindowSizeDependentResources(D3D12_VIEWPORT viewport);
       
        // Render() performs 2 actions:
        // - Uploads any pending glyph data to atlas cache textures
        // - Creates a new SpriteBatch and submits draws for any strings that were queued with EnqueueDrawText().
        // This can be called multiple times per frame if custom UI element ordering is needed. Strings are always rendered in the order that EnqueueDrawText() is called.
        // frameAdvance specified if this render method calls RenderFrameAdvance() afterwards. See comments for RenderFrameAdvance().
        void Render(_In_ ID3D12GraphicsCommandList* commandList, bool frameAdvance = true);
        
        // Since Render() can be called multiple times per frame, this needs to be called once per frame (only!) to properly release internally-used graphics resources for uploads.
        // Simply call this once per frame once all desired Render(commandList, false) calls for the frame have been completed.
        // Alternatively, the final Render() method can use frameAdvance=true.
        void RenderFrameAdvance();

        // Load a font to be used with the glyph cache. The preferred ranges specify which unicode ranges this font should be preferred for. The
        // priority determines whether this font is used over other fonts when finding a font to use for a specific unicode character.
        // Higher priority means this font will be considered first over lower priority fonts.
        bool LoadFont(const std::string& fontPath, const std::vector<UnicodeRange>& preferredRanges, int priority, int faceIndex = 0);

        // Attempts to automatically load Noto fonts for the UnicodeRanges specified above. These fonts must be added as a project reference before this method will work.
        bool LoadNotoFonts(const std::string& optSubDir = "");

        // Precaches the glyphs present within the string at the specified size for optimization purposes. This action is performed automatically if needed by the EnqueueDrawText method.
        void PrecacheGlyphs(const char* utf8String, int fontSize);

        // Queues the specified string to draw with the next Render() call. The glyphs within are precached as needed. Strings do not persist, so this method should be called whenever
        // a draw is desired. Draw location uses screen pixel coordinates with the origin being the top-left of screen and top-left of glyph run.
        // NOTE: Provides no support for wrapping/clipping/newlines. Instead, a consumer of this cache can implement it themselves using the conversion and
        // measurement methods below.
        void XM_CALLCONV EnqueueDrawText(const char* utf8String, int fontSize, float xPos, float yPos, DirectX::FXMVECTOR color = ATG::Colors::White);

        // Measures how tall and wide a particular string would be if it were to be drawn. Measurement information for a glyph is only known after it is
        // rendered, so this method caches all glyphs within the string.
        // Note: the draw height includes drawing above or below "standard" characters, such as diacritics. It's best to use custom heights for layout based on
        // draw size instead of the measured height since the measured height can change a lot based on the string.        
        void MeasureText(const char* utf8String, int fontSize, int* outDrawWidth, int* outDrawHeight);

        // Empties the cache of all pre-rendered glyphs. This might be useful if fonts are loaded in dynamically instead of all at the start.
        // For example, a glyph might be missing when initially cached, but be present after loading a new font. The cache will still use the old
        // missing glyph unless it is cleared.
        void ClearCache();
       
    public:

        // Returns the number of maximum textures specified in the constructor.
        size_t GetTextureCount();


        // Create textures. When using external SpriteBatch with external DescriptorHeap this method can be used instead of CreateDeviceDependentResources() which will use internal SpriteBatch and DescriptorHeap.
        // The number of textures this function will put into the external DescriptorHeap can be retrieved through GetTextureCount() method.
        void CreateTextures(_In_ ID3D12CommandQueue* commandQueue, _In_ DirectX::DescriptorHeap* descriptorHeap, size_t descriptorHeapOffset);

        // SpriteFont style draw. When using external SpriteBatch with external DescriptorHeap this method can be used instead of EnqueueDrawText() and Render() which will use internal SpriteBatch and DescriptorHeap.
       // The external DescriptorHeap must have been used for CreateTexutres() prior to this method.
       // Unlike Render(), this method requires to Begin() and End() the spritebatch before and after this method.
       // Please make sure to call RenderFrameAdvance() after finishing calling this method multiple times.
       // Currently, "rotation", "origin", "effects" and "layerDepth" are not being considered.
        void XM_CALLCONV DrawString(
            _In_ ID3D12GraphicsCommandList* commandList,
            _In_ DirectX::SpriteBatch* spriteBatch,
            _In_z_ char const* utf8String,
            DirectX::XMFLOAT2 const& position,
            DirectX::FXMVECTOR color,
            float rotation,
            DirectX::XMFLOAT2 const& origin,
            float fontSize,
            DirectX::SpriteEffects effects,
            float layerDepth
        );


    protected:

        // Private implementation
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };

}
