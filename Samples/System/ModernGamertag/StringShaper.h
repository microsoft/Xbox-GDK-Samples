//--------------------------------------------------------------------------------------
// File: StringShaper.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// FreeType2 is required for glyph rendering
#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftbitmap.h>

// HarfBuzz is required for text shaping
#include <hb.h>
#include <set>
#include <map>

#include "UnicodeRange.h"

namespace TextRenderer
{
    class Face;
    class ShapedString;

    // Freetype uses fractional pixels on the scale of 1/64. So multiplying our width and height by 64 gets a regular pixel size.
    // Harfbuzz doesn't use fractional pixels, but using the scaling with harfbuzz makes it easier to work with both Freetype and harfbuzz simultaneously.
    constexpr uint8_t FT_SIZENORMALIZER = 64;

    // Currently only support font files with 1 font face. So only the first typeface in a font file is used.
    constexpr uint32_t FACEINDEX = 0;

    constexpr int32_t invalidFontSize = -1;

    class StringShaper
    {
    public:
        std::unique_ptr<ShapedString> ShapeUTF8String(const std::string& utf8String, const int32_t fontSize);

        // Maps a font to a unicode range. All fonts that are going to be supported
        // have to be mapped to a unicode range BEFORE a StringRender drawstring() is called.
        void MapFontToUnicodeRange(const std::string& font, const std::vector<UnicodeRange>& ranges);
        UnicodeRange GetUnicodeRange(const char* utf8, const uint32_t& size) const;
        void InsertGlyphIntoBuffer(const FT_GlyphSlot& glyph, const uint64_t d3d12AlignmentMultiple, uint32_t x, uint32_t y, uint8_t* outTextureBuffer) const;
        void MeasureShapedString(
            const std::vector<Face*>& facesInShapedString,
            const std::vector<hb_buffer_t*>& hbBuffers,
            uint32_t* outDrawWidth,
            uint32_t* outDrawHeight) const;

    private:
        // Generate the actual bitmap data of a string. Can be made up of multiple Harfbuzz ShapedStrings
        void GenerateTextureBitmap(
            const uint32_t& maxAscent,
            const std::vector<Face*>& facesInShapedString,
            const std::vector<hb_buffer_t*>& hbBuffers,
            const uint32_t width,
            uint8_t* outTextureBuffer,
            const uint32_t bufferSize);

        // Returns the max pixels that should occur below and above the baseline of all provided fonts of a specific font size.
        // This valeu is only calculated once PER font size.
        std::pair<uint32_t, uint32_t> GetGlobalAscentDescent(int32_t fontSize);

        // Maps a specific unicode range to a font.
        // If there is a range that is in multiple fonts. The first font supporting that range is used.
        std::map<UnicodeRange, Face*>                                           m_rangeToFaceMap;
        std::set<UnicodeRange>                                                  m_supportedRanges;

        // The key is font size, and the value is a pair of the max Ascent and Descent of the specific font size.
        // Used to align the yoffset/Ybearing of text across multiple fonts of the SAME FONT SIZE.
        std::unordered_map<int32_t, std::pair<uint32_t, uint32_t>>              m_maxFontAscentDescentMap;
        std::vector<std::unique_ptr<Face>>                                      m_faces;
    };

    // Represents a font and manages Freetype and Harfbuzz structures related to a font face.
    // A single FreeType font face is active per font file, so features such as size have to be updated
    // if there is a request to use a different font saze.
    class Face
    {
    public:
        Face(const std::string& fontName);
        ~Face();

        int32_t GetFontSize() const;
        void SetFontSize(const int32_t& fontSize);

        const std::string           m_fontName;
        FT_Library                  m_ftLibrary;
        FT_Face                     m_ftFace;
        hb_blob_t*                  m_hbBlob;
        hb_face_t*                  m_hbFace;
        hb_font_t*                  m_hbFont;

    private:
        int32_t                     m_fontSize;
    };

    // A class for holding metadata related to the final representation of a string.
    class ShapedString
    {
    public:
        ShapedString(std::unique_ptr<uint8_t[]> data, uint32_t width, uint32_t height, uint32_t size) :
            m_data(std::move(data)),
            m_width(width),
            m_height(height),
            m_size(size)
        {
        }
        // Contains the bitmap information of 1+ harfbuzz shapedstrings(multi language) combined.
        std::unique_ptr<uint8_t[]>   m_data;

        // The width and height of the string to be drawn to the texture atlas and screen
        uint32_t                    m_width;
        uint32_t                    m_height;

        // Size of m_data in bytes 
        uint32_t                    m_size;
    };

    class ShapedStringKey
    {
    public:
        ShapedStringKey(std::string utf8String, int32_t fontSize) :
            m_utf8String(utf8String),
            m_fontSize(fontSize)
        {
        }
        bool operator==(const ShapedStringKey& other) const
        {
            return m_utf8String == other.m_utf8String &&
                m_fontSize == other.m_fontSize;
        }

        bool operator<(const ShapedStringKey& other) const
        {
            if (m_utf8String != other.m_utf8String)
            {
                return m_utf8String < other.m_utf8String;
            }
            else
            {
                return m_fontSize < other.m_fontSize;
            }
        }

        std::string                 m_utf8String;
        int32_t                     m_fontSize;
    };
}
