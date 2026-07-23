//--------------------------------------------------------------------------------------
// File: StringTextureAtlas.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "StringShaper.h"
#include "StringUtil.h"

using namespace DirectX;

// NOTE: D3D12_TEXTURE_DATA_PITCH_ALIGNMENT isn't in the Xbox headers?
#ifdef _GAMING_XBOX
size_t d3d12TextureDataPitchAlignment = D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT;
#else
size_t d3d12TextureDataPitchAlignment = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
#endif

namespace TextRenderer
{

#pragma warning(push)
#pragma warning(disable: 26812)
    std::unique_ptr<ShapedString> StringShaper::ShapeUTF8String(const std::string& utf8String, const int32_t fontSize)
    {
        // Get the max height above and below the baseline for all fonts with the current fontsize.
        auto [maxAscent, maxDescent] = GetGlobalAscentDescent(fontSize);

        // Grab the first UTF8 character from the string
        uint32_t bytesInUtf8 = DX::DetermineUtf8CharBytesFromFirstByte(utf8String[0]);
        UnicodeRange prevStringUnicodeRange = GetUnicodeRange(utf8String.c_str(), bytesInUtf8);
        size_t index = (size_t)bytesInUtf8;
        size_t textLength = (size_t)bytesInUtf8;
        size_t itemOffset = 0;

        // An hbBuffers entry[i] represents shaping information for a string within a SINGLE unicode range.
        std::vector<hb_buffer_t*> hbBuffers;

        // A faces entry[i] represents the font face used to render the corresponding hbBuffers entry[i] with.
        std::vector<Face*> faces;

        // Populate the faces and hbBuffers buffers.
        while (index < utf8String.size())
        {
            // Process multibyte utf8 string.
            bytesInUtf8 = DX::DetermineUtf8CharBytesFromFirstByte(utf8String[index]);
            UnicodeRange curStringUnicodeRange = GetUnicodeRange(utf8String.c_str() + index, bytesInUtf8);

            // Harfbuzz is only able to shape a string 1 font file at a time, so strings with
            // multiple languages/fonts are split by their unicode range.
            // If a string only has 1 language, this block is never executed, and only 1
            // harfbuzz buffer and Face is added to their corresponding vectors.
            if (curStringUnicodeRange != prevStringUnicodeRange)
            {
                m_rangeToFaceMap[prevStringUnicodeRange]->SetFontSize(fontSize);
                faces.push_back(m_rangeToFaceMap[prevStringUnicodeRange]);
                hbBuffers.push_back(hb_buffer_create());
                hb_buffer_set_direction(hbBuffers.back(), prevStringUnicodeRange.GetHBDirection());
                hb_buffer_set_script(hbBuffers.back(), prevStringUnicodeRange.GetHBScript());
                hb_buffer_set_language(hbBuffers.back(), hb_language_from_string(prevStringUnicodeRange.GetLanguageCode().c_str(), -1));
                hb_buffer_add_utf8(hbBuffers.back(), utf8String.c_str(), (int32_t)textLength, (uint32_t)itemOffset, (int32_t)textLength);
                hb_shape(m_rangeToFaceMap[prevStringUnicodeRange]->m_hbFont, hbBuffers.back(), NULL, 0);

                prevStringUnicodeRange = curStringUnicodeRange;
                itemOffset += textLength;
                textLength = 0;
            }
            index += bytesInUtf8;
            textLength += bytesInUtf8;
        }

        m_rangeToFaceMap[prevStringUnicodeRange]->SetFontSize(fontSize);
        faces.push_back(m_rangeToFaceMap[prevStringUnicodeRange]);
        hbBuffers.push_back(hb_buffer_create());
        hb_buffer_set_direction(hbBuffers.back(), prevStringUnicodeRange.GetHBDirection());
        hb_buffer_set_script(hbBuffers.back(), prevStringUnicodeRange.GetHBScript());
        hb_buffer_set_language(hbBuffers.back(), hb_language_from_string(prevStringUnicodeRange.GetLanguageCode().c_str(), -1));
        hb_buffer_add_utf8(hbBuffers.back(), utf8String.c_str(), (int32_t)textLength, (uint32_t)itemOffset, (int32_t)textLength);
        hb_shape(m_rangeToFaceMap[prevStringUnicodeRange]->m_hbFont, hbBuffers.back(), NULL, 0);

        uint32_t drawWidth = 0;
        uint32_t drawHeight = 0;
        MeasureShapedString(faces, hbBuffers, &drawWidth, nullptr);

        // Add padding of 1px around our shaped string
        drawWidth += 2;
        drawHeight = maxAscent + maxDescent + 2;

        uint32_t bufferSize = AlignUp(drawWidth, d3d12TextureDataPitchAlignment) * drawHeight;
        auto outputBuffer = std::make_unique<uint8_t[]>(bufferSize);

        GenerateTextureBitmap(maxAscent, faces, hbBuffers, drawWidth, outputBuffer.get(), bufferSize);

        std::vector< hb_buffer_t* >::iterator hbBufferIter = hbBuffers.begin();
        for (; hbBufferIter != hbBuffers.end(); ++hbBufferIter)
        {
            // Destroy the buffer once were done processing the shaped string.
            hb_buffer_destroy(*hbBufferIter);
        }

        auto toReturn = std::make_unique<ShapedString>(std::move(outputBuffer), drawWidth, drawHeight, bufferSize);
        return std::move(toReturn);
    }
#pragma warning(pop)

    void StringShaper::MapFontToUnicodeRange(const std::string& font, const std::vector<UnicodeRange>& ranges)
    {
        auto face = std::make_unique<Face>(font);
        for (UnicodeRange range : ranges)
        {
            m_rangeToFaceMap[range] = face.get();
            m_supportedRanges.insert(range);
        }

        m_faces.push_back(std::move(face));

        // Clear the ascent descent map as a new font has been mapped.
        m_maxFontAscentDescentMap.clear();
    }

    UnicodeRange StringShaper::GetUnicodeRange(const char* utf8, const uint32_t& size) const
    {
        uint32_t character = DX::Utf8ToUtf32Character(utf8, (int32_t)size);

        // If there is a font loaded that supports glyphs that are outside general language unicode ranges. We want to make sure
        // that glyphs that have a langauge are returned as that languages range and not matched to other.
        bool unicodeRangeOtherFontLoaded = false;

        // Check all the supported unicode ranges provided by the provided font files.
        std::set<UnicodeRange>::iterator rangeIter = m_supportedRanges.begin();
        for (; rangeIter != m_supportedRanges.end(); ++rangeIter)
        {
            if (rangeIter->IsCharacterInRange(character))
            {
                if ((*rangeIter) == UnicodeRangeInfo::Other)
                {
                    unicodeRangeOtherFontLoaded = true;
                }
                else
                {
                    return *rangeIter;
                }
            }
        }

        // If you use a font labled mapped OTHER, ALL codepoints that didn't have their codepoint mapped to a font will be labled as other, even codepoints that don't
        // have a corresponding glyph in any of the provided fonts. 
        if (unicodeRangeOtherFontLoaded)
        {
            return UnicodeRangeInfo::Other;
        }

        throw std::exception("Passed in character not supported.");
    }

    // The height of the string will be measured using the max ascent and descent of the font faces present in the string. This may cause some alignment issues, so pass in
    // a nullptr for the outDrawHeight if you are using a global height across all font faces for a specific font size.
    void StringShaper::MeasureShapedString(const std::vector<Face*>& facesInShapedString, const std::vector<hb_buffer_t*>& hbBuffers, uint32_t* outDrawWidth, uint32_t* outDrawHeight) const
    {
        std::vector<Face*>::const_iterator faceIter = facesInShapedString.begin();
        std::vector< hb_buffer_t*>::const_iterator hbBufferIter = hbBuffers.begin();
        uint32_t drawWidth = 0;
        uint32_t maxAscent = 0;
        uint32_t maxDescent = 0;

        for (; faceIter != facesInShapedString.end() && hbBufferIter != hbBuffers.end(); ++faceIter, ++hbBufferIter)
        {
            Face* face = *faceIter;
            hb_glyph_position_t* hbGlyphPositions;
            uint32_t glyphCount;
            hbGlyphPositions = hb_buffer_get_glyph_positions(*hbBufferIter, &glyphCount);

            // The following pixel heights calculation is from here
            // https://freetype.org/freetype2/docs/tutorial/step2.html#:~:text=/*%20compute%20floating%20point%20scale%20factors%20*/
            uint32_t currentFontMaxAscent = uint32_t(face->m_ftFace->ascender * (face->m_ftFace->size->metrics.y_scale / 65536.0)) / FT_SIZENORMALIZER;
            uint32_t currentFontMaxDescent = uint32_t(abs(face->m_ftFace->descender * (face->m_ftFace->size->metrics.y_scale / 65536.0))) / FT_SIZENORMALIZER;

            if (maxAscent < currentFontMaxAscent)
            {
                maxAscent = currentFontMaxAscent;
            }
            if (maxDescent < currentFontMaxDescent)
            {
                maxDescent = currentFontMaxDescent;
            }

            for (uint32_t i = 0; i < glyphCount; i++)
            {
                FT_Error error = FT_Load_Glyph(face->m_ftFace, i, FT_LOAD_NO_BITMAP);
                if (error != FT_Err_Ok)
                {
                    throw std::exception("Failed FT_Load_Glyph within MultiFont MeasureShapedString");
                }
                error = FT_Render_Glyph(face->m_ftFace->glyph, FT_RENDER_MODE_NORMAL);
                if (error != FT_Err_Ok)
                {
                    throw std::exception("Failed FT_Render_Glyph within MultiFont MeasureShapedString");
                }
                drawWidth += hbGlyphPositions[i].x_advance + (hbGlyphPositions[i].x_offset);
            }
        }

        if (outDrawWidth)
        {
            (*outDrawWidth) = drawWidth / FT_SIZENORMALIZER;
        }
        if (outDrawHeight)
        {
            (*outDrawHeight) = (maxAscent + maxDescent);
        }
    }

    void StringShaper::GenerateTextureBitmap(
        const uint32_t& maxAscent,
        const std::vector<Face*>& facesInShapedString,
        const std::vector<hb_buffer_t*>& hbBuffers,
        const uint32_t width,
        uint8_t* outTextureBuffer,
        const uint32_t bufferSize)
    {
        // The width of our shaped string is aligned to d3d12TextureDataPitchAlignment. If the width is longer, we want to make sure that
        // we pad out our rows by that amount.
        uint64_t d3d12AlignmentMultiple = AlignUp(width, d3d12TextureDataPitchAlignment);
        std::vector<Face*>::const_iterator faceIter = facesInShapedString.begin();
        std::vector< hb_buffer_t* >::const_iterator hbBufferIter = hbBuffers.begin();

        // 0 the whole ShapedString texture
        memset(outTextureBuffer, 0, bufferSize);

        // To track where to draw the glyphs to our shaped string texture, we keep track of the glyph advances.
        // Start off at 1 so that we have the padding 1PX around our Shaped string.
        uint32_t currentX = 1;
        uint32_t currentY = 1;

        for (; faceIter != facesInShapedString.end() && hbBufferIter != hbBuffers.end(); ++faceIter, ++hbBufferIter)
        {
            hb_glyph_info_t* hbGlyphInfos;
            hb_glyph_position_t* hbGlyphPositions;
            uint32_t glyphCount;
            hbGlyphInfos = hb_buffer_get_glyph_infos(*hbBufferIter, &glyphCount);
            hbGlyphPositions = hb_buffer_get_glyph_positions(*hbBufferIter, &glyphCount);

            Face* face = *faceIter;
            for (uint32_t i = 0; i < glyphCount; i++)
            {
                // After calling hb_shape, the hb_buffer is converted to a shaped string. The glyphinfos codepoint
                // actually becomes a glyph index for the glyph that was shaped. This is because there is a case
                // where multiple codepoints potentially form a single ligature, which is a glyph made up of 1+ codepoints.
                // https://harfbuzz.github.io/harfbuzz-hb-buffer.html#hb-glyph-info-t
                const uint32_t glyphIndex = hbGlyphInfos[i].codepoint;

                auto glyph = face->m_ftFace->glyph;
                FT_Error error = FT_Load_Glyph(face->m_ftFace, glyphIndex, FT_LOAD_NO_BITMAP);
                if (error != FT_Err_Ok)
                {
                    throw std::exception("Failed FT_Load_Glyph within MultiFont GenerateTexture.");
                }
                error = FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);
                if (error != FT_Err_Ok)
                {
                    throw std::exception("Failed FT_Render_Glyph within MultiFont GenerateTexture.");
                }

                // Spaces and invisible glyphs cant be rendered, so just use HB shaping information to skip to the next glyph.
                if (glyph->bitmap.width > 0 && glyph->bitmap.rows > 0)
                {
                    // This is the distance between the top of our current glyph and the top of the TALLEST possible glyph given our fonts.
                    uint32_t heightDiff = maxAscent - (glyph->metrics.horiBearingY / FT_SIZENORMALIZER);

                    // Insert the glyph into the buffer at position x, y.
                    InsertGlyphIntoBuffer(glyph, d3d12AlignmentMultiple, currentX, currentY + heightDiff, outTextureBuffer);
                }

                // Move the x position to the next Glyph
                currentX += (hbGlyphPositions[i].x_advance / FT_SIZENORMALIZER);
            }
        }
    }

    /*
    *                             Representation of a SHAPED STRING "q"
    * 
    * FT = value obtained from freetype rendered glyph
    * HB = value obtained from harfbuzz shaping
    * 
    *  OutTextureBuffer Start                                      
    *               \          |<-- FTbitmap.width -->| <---     alignmentPadding     --->|
    *                *---------+----------------------+-----------------------------------+---------- GlobalMaxAscent across ALL fonts with the current fontsize
    *                |         |                      |            ^                      |
    *                |---------|----------------------|------------|----------------------+---------- MaxAscent of current font
    *                |         |                      |         heightDiff                |
    *                |         |                      |            |                      |
    *                |         |                      |            v                      |
    *                |---------|----qqqqqqqqq   qqqqq-|-----------------------------------|
    *                |         |   q:::::::::qqq::::q |            ^                      |
    *                |         |  q:::::::::::::::::q |            |                      |
    *                |         |q::::::qqqqq::::::qq  |            |                      |
    *   FTbearingX   |<------->|q:::::q     q:::::q   |            |                      |
    *                |         |q:::::q     q:::::q   |            |                      |
    *                |         |q:::::q     q:::::q   |         FTbearingY                |
    *                |         |q::::::q    q:::::q   |            |                      |
    *                |         |q:::::::qqqqq:::::q   |            |                      | 
    *                |         | q::::::::::::::::q   |            |                      | 
    *                |         |  qq::::::::::::::q   |            v                      |
    *     baseline --*---------|----qqqqqqqq::::::q---|-----------------------------------| 
    *              / |         |            q:::::q   |                                   |
    * glyph origin   |         |            q:::::q   |                                   |  
    *                |         |           q:::::::q  |                                   |
    *                |         |           q:::::::q  |                                   |
    *                |         |           q:::::::q  |                                   |
    *                |         |           qqqqqqqqq  |                                   |
    *                |         |                      |                                   |
    *   HBx_advance  |<--------+--------------------->|                                   |
    *                |         |                      |                                   |
    *                |---------|----------------------|-----------------------------------+-------- MaxDescent of currentFont
    *                |         |                      |                                   |
    *                |         |                      |                                   |
    *                |--------------------------------------------------------------------*-------- GlobalMaxDescent across ALL fonts with the current fontsize
    *                                                                                       \
    *                |<------------------        bufferWidth         -------------------->|   OutTextureBuffer end
    *
    * The general flow of inserting a glyph into the ShapedString buffer is as follows.
    * 1. Skip the first y rows of our ShapedString buffer
    * 2. On the new row, skip a total of x_advance bytes in the current row
    * 3. Copy a row of our bitmap into the current row of the ShapedString buffer.
    * 4. Skip the remaining alignmentPadding bytes to get to the next row of our ShapedString buffer.
    * 5. Repeat steps 2 -> 4 until the bitmap is completely written.
    * 6. Repeatsteps 1 -> 5 to complete writing ALL fretype bitmaps to our shaped string.
    */
    void StringShaper::InsertGlyphIntoBuffer(const FT_GlyphSlot& glyph, const uint64_t bufferWidth, uint32_t x, uint32_t y, uint8_t* outTextureBuffer) const
    {
        int64_t bearingX = (glyph->metrics.horiBearingX / FT_SIZENORMALIZER);

        // The amount of padding bytes needed to go from one row of the ShapedString buffer to the next
        size_t alignmentPadding = bufferWidth - ((glyph->bitmap.width + bearingX) % bufferWidth);

        uint8_t* bufferLocPtr = outTextureBuffer;

        // Immediatly skip the first few rows for our y offset.
        bufferLocPtr += y * size_t(bufferWidth);

        // Shift our buffer to the right by our current bearingX.
        bufferLocPtr += x + bearingX;

        // Draw every glyph to our texture
        for (uint32_t row = 0; row < glyph->bitmap.rows; ++row)
        {
            // Grab a row of our bitmap to copy over to our atlas.
            // Negative character pitch is not supported. 
            const uint8_t* bitmapSource = glyph->bitmap.buffer + (size_t(row) * size_t(glyph->bitmap.width));

            memcpy(bufferLocPtr, bitmapSource, glyph->bitmap.width);

            // Move the bufferPtr to the next row of the ShapedString.
            bufferLocPtr += bearingX;
            bufferLocPtr += glyph->bitmap.width;
            bufferLocPtr += alignmentPadding;
        }
    }

    std::pair<uint32_t,uint32_t> StringShaper::GetGlobalAscentDescent(int32_t fontSize)
    {
        std::unordered_map<int32_t, std::pair<uint32_t, uint32_t>>::iterator maxAscentIter = m_maxFontAscentDescentMap.find(fontSize);

        // outMaxAscent, outMaxDescent
        std::pair<uint32_t, uint32_t> toReturn;

        uint32_t maxAscent = 0;
        uint32_t maxDescent = 0;

        if (maxAscentIter == m_maxFontAscentDescentMap.end())
        {
            //std::vector<Face*>::iterator faceIter = m_faces.begin();
            for (auto& faceIter: m_faces)
            {
                // Update the FT_library font sizes
                int32_t prevSize = faceIter->GetFontSize();
                if (prevSize != fontSize)
                {
                    faceIter->SetFontSize(fontSize);
                }

                // The following pixel heights calculation is from here
                // https://freetype.org/freetype2/docs/tutorial/step2.html#:~:text=/*%20compute%20floating%20point%20scale%20factors%20*/
                uint32_t currentFontMaxAscent = uint32_t((faceIter->m_ftFace->ascender * (faceIter->m_ftFace->size->metrics.y_scale / 65536.0)) / FT_SIZENORMALIZER);
                uint32_t currentFontMaxDescent = uint32_t((abs(faceIter->m_ftFace->descender * (faceIter->m_ftFace->size->metrics.y_scale / 65536.0))) / FT_SIZENORMALIZER);

                if (maxAscent < currentFontMaxAscent)
                {
                    maxAscent = currentFontMaxAscent;
                }
                if (maxDescent < currentFontMaxDescent)
                {
                    maxDescent = currentFontMaxDescent;
                }
                faceIter->SetFontSize(prevSize);
            }

            // Insert an ascent descent pair for a specific font size.
            m_maxFontAscentDescentMap.insert(std::pair<uint32_t, std::pair<uint32_t, uint32_t>>(fontSize, std::pair<uint32_t, uint32_t>(maxAscent, maxDescent)));
            toReturn.first = maxAscent;
            toReturn.second = maxDescent;
        }
        else
        {
            toReturn.first = maxAscentIter->second.first;
            toReturn.second = maxAscentIter->second.second;
        }
        return toReturn;
    }

    Face::Face(const std::string& m_fontName) :
        m_fontName(m_fontName)
    {
        // Set up freetype library
        FT_Error error = FT_Init_FreeType(&m_ftLibrary);

        // Load the font file into freetype glyph rendering and harfbuzz text shaping libraries.
        error = FT_New_Face(m_ftLibrary, m_fontName.c_str(), FACEINDEX, &m_ftFace);
        if (error != FT_Err_Ok)
        {
            throw std::exception("Freetype failed to load library");
        }

        m_hbBlob = hb_blob_create_from_file(m_fontName.c_str());
        if (!m_hbBlob)
        {
            throw std::exception("Harfbuzz failed to create hbblob from font");
        }

        m_hbFace = hb_face_create(m_hbBlob, (uint32_t)FACEINDEX);
        m_hbFont = hb_font_create(m_hbFace);
        m_fontSize = invalidFontSize;
    };

    Face::~Face()
    {
        hb_font_destroy(m_hbFont);
        hb_face_destroy(m_hbFace);
        hb_blob_destroy(m_hbBlob);
        FT_Done_Face(m_ftFace);
        FT_Done_FreeType(m_ftLibrary);
    }

    void Face::SetFontSize(const int32_t& fontSize)
    {
        FT_Set_Char_Size(m_ftFace, 0, fontSize * FT_SIZENORMALIZER, 0, 0);
        hb_font_set_scale(m_hbFont, fontSize * FT_SIZENORMALIZER, fontSize * FT_SIZENORMALIZER);
        m_fontSize = fontSize;
    }

    int32_t Face::GetFontSize() const
    {
        return m_fontSize;
    }
}
