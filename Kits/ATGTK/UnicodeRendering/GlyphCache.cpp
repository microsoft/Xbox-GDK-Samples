//--------------------------------------------------------------------------------------
// GlyphCache.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "GlyphCache.h"
#include "StringUtil.h"

// FreeType2 is required for glyph rendering
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H

// HarfBuzz is required for text shaping
#include <hb.h>

#include <assert.h>

#include <algorithm>
#include <exception>
#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

// DirectXTK12 dependencies
#include "CommonStates.h"
#include "DescriptorHeap.h"
#include "DirectXHelpers.h"
#include "GraphicsMemory.h"
#include "ResourceUploadBatch.h"
#include "SimpleMath.h"
#include "SpriteBatch.h"

using namespace ATG;
using namespace DirectX;

namespace
{
    static UnicodeRange GetRangeForUTF32Character(char32_t character)
    {
        // This table is taken from the GDK documentation page: "UTF-8 character ranges for modern gamertags".
        // This represents the set of characters that should be supported by loaded fonts.

        if ((character >= 0x0021 && character <= 0x0026) ||
            (character >= 0x0028 && character <= 0x002F) ||
            (character >= 0x003A && character <= 0x0040) ||
            (character >= 0x005B && character <= 0x0060) ||
            (character >= 0x007B && character <= 0x007E) ||
            (character >= 0x00A0 && character <= 0x00BF))
        {
            return UnicodeRange::LatinSymbols;
        }
        else if ((character == 0x0020) ||
            (character == 0x0027) ||
            (character >= 0x0030 && character <= 0x0039) ||
            (character >= 0x0041 && character <= 0x005A) ||
            (character >= 0x0061 && character <= 0x007A))
        {
            return UnicodeRange::LatinAlphaNumberic;
        }
        else if ((character >= 0x00C0 && character <= 0x00F6) ||
            (character >= 0x00F8 && character <= 0x00FF) ||
            (character >= 0x0100 && character <= 0x017F))
        {
            return UnicodeRange::LatinSupplementalExtended;
        }
        else if ((character >= 0x1100 && character <= 0x1112) ||
            (character >= 0x1161 && character <= 0x1175) ||
            (character >= 0x11A8 && character <= 0x11C2) ||
            (character >= 0xAC00 && character <= 0xD7A3))
        {
            return UnicodeRange::Korean;
        }
        else if ((character >= 0x3041 && character <= 0x3096) ||
            (character >= 0x30A1 && character <= 0x30FA))
        {
            return UnicodeRange::Japanese;
        }
        else if ((character >= 0x4E00 && character <= 0x9FFF))
        {
            return UnicodeRange::Chinese;
        }
        else if ((character >= 0x0400 && character <= 0x045F))
        {
            return UnicodeRange::Russian;
        }
        else if ((character >= 0x0985 && character <= 0x09B9))
        {
            return UnicodeRange::Bengali;
        }
        else if ((character >= 0x0E01 && character <= 0x0E3A) ||
            (character >= 0x0E40 && character <= 0x0E4E))
        {
            return UnicodeRange::Thai;
        }
        else if ((character >= 0x0390 && character <= 0x03CE))
        {
            return UnicodeRange::Greek;
        }
        else if ((character >= 0x0900 && character <= 0x094F) ||
            (character >= 0x0966 && character <= 0x096F) ||
            (character >= 0x0671 && character <= 0x06D3) ||
            (character >= 0x06F0 && character <= 0x06F9))
        {
            return UnicodeRange::Hindi;
        }
        else if ((character >= 0x0620 && character <= 0x064A) ||
            (character >= 0x0660 && character <= 0x0669))
        {
            return UnicodeRange::Arabic;
        }
        else if ((character >= 0x05D0 && character <= 0x05EA))
        {
            return UnicodeRange::Hebrew;
        }
        else if ((character >= 0xE000 && character <= 0xF8FF))
        {
            return UnicodeRange::PrivateUseArea;
        }
        else
        {
            return UnicodeRange::Other;
        }
    }

    static const char* GetLanguageCodeForUnicodeRange(UnicodeRange range)
    {
        switch (range)
        {
        case UnicodeRange::Korean:
            return "ko";
        case UnicodeRange::Japanese:
            return "ja";
        case UnicodeRange::Chinese:
            return "zh";
        case UnicodeRange::Russian:
            return "ru";
        case UnicodeRange::Bengali:
            return "bn";
        case UnicodeRange::Thai:
            return "th";
        case UnicodeRange::Greek:
            return "el";
        case UnicodeRange::Hindi:
            return "hi";
        case UnicodeRange::Arabic:
            return "ar";
        case UnicodeRange::Hebrew:
            return "he";
        case UnicodeRange::Other:
        case UnicodeRange::LatinSymbols:
        case UnicodeRange::LatinAlphaNumberic:
        case UnicodeRange::LatinSupplementalExtended:
        case UnicodeRange::PrivateUseArea:
        default:
            return "en";
        }
    }

    // Suppress analysis warning "enum vs enum class usage" from 3rd-party libraries
#pragma warning(push)
#pragma warning(disable: 26812)

    static hb_script_t GetHBScriptForUnicodeRange(UnicodeRange range)
    {
        switch (range)
        {
        case UnicodeRange::Korean:
            return HB_SCRIPT_UNKNOWN;
        case UnicodeRange::Japanese:
            return HB_SCRIPT_UNKNOWN;
        case UnicodeRange::Chinese:
            return HB_SCRIPT_UNKNOWN;
        case UnicodeRange::Russian:
            return HB_SCRIPT_UNKNOWN;
        case UnicodeRange::Bengali:
            return HB_SCRIPT_BENGALI;
        case UnicodeRange::Thai:
            return HB_SCRIPT_THAI;
        case UnicodeRange::Greek:
            return HB_SCRIPT_GREEK;
        case UnicodeRange::Hindi:
            return HB_SCRIPT_DEVANAGARI;
        case UnicodeRange::Arabic:
            return HB_SCRIPT_ARABIC;
        case UnicodeRange::Hebrew:
            return HB_SCRIPT_HEBREW;
        case UnicodeRange::PrivateUseArea:
        case UnicodeRange::Other:
        case UnicodeRange::LatinSymbols:
        case UnicodeRange::LatinAlphaNumberic:
        case UnicodeRange::LatinSupplementalExtended:
        default:
            return HB_SCRIPT_UNKNOWN;
        }
    }

    static hb_direction_t GetHBDirectionForUnicodeRange(UnicodeRange range)
    {
        switch (range)
        {
        case UnicodeRange::Arabic:
        case UnicodeRange::Hebrew:
            return HB_DIRECTION_RTL;
        case UnicodeRange::Korean:
        case UnicodeRange::Japanese:
        case UnicodeRange::Chinese:
        case UnicodeRange::Russian:
        case UnicodeRange::Bengali:
        case UnicodeRange::Thai:
        case UnicodeRange::Greek:
        case UnicodeRange::Hindi:
        case UnicodeRange::PrivateUseArea:
        case UnicodeRange::Other:
        case UnicodeRange::LatinSymbols:
        case UnicodeRange::LatinAlphaNumberic:
        case UnicodeRange::LatinSupplementalExtended:
        default:
            return HB_DIRECTION_LTR;
        }
    }

#pragma warning(pop)

}

// Internal GlyphCache implementation class.
class GlyphCache::Impl
{
public:

    struct Face;

    struct GlyphKey
    {
        bool operator==(const GlyphKey& other) const
        {
            return m_face == other.m_face &&
                m_glyphIndex == other.m_glyphIndex &&
                m_size == other.m_size;
        }

        bool operator<(const GlyphKey& other) const
        {
            if (m_face != other.m_face)
            {
                return m_face < other.m_face;
            }
            else if (m_glyphIndex != other.m_glyphIndex)
            {
                return m_glyphIndex < other.m_glyphIndex;
            }
            else
            {
                return m_size < other.m_size;
            }
        }

        std::shared_ptr<Face>   m_face;
        FT_UInt                 m_glyphIndex;
        int                     m_size;
    };

    struct ShapedStringKey
    {
        bool operator==(const ShapedStringKey& other) const
        {
            return m_fontSize == other.m_fontSize &&
                m_string == other.m_string;
        }

        bool operator<(const ShapedStringKey& other) const
        {
            if (m_fontSize != other.m_fontSize)
            {
                return m_fontSize < other.m_fontSize;
            }
            else
            {
                return m_string < other.m_string;
            }
        }

        std::u32string  m_string;
        int             m_fontSize;
    };


    struct ShapedString
    {
        ShapedString(std::shared_ptr<Face> face, hb_font_t* hbFont, const std::u32string& str, hb_script_t hbScript, const char* language)
            : m_face(face)
        {
            m_hbBuffer = hb_buffer_create();
            hb_buffer_add_utf32(m_hbBuffer, reinterpret_cast<const uint32_t*>(str.c_str()), -1, 0, -1);
            hb_buffer_set_direction(m_hbBuffer, GetHBDirectionForUnicodeRange(GetRangeForUTF32Character(str[0])));
            hb_buffer_set_script(m_hbBuffer, hbScript);
            hb_buffer_set_language(m_hbBuffer, hb_language_from_string(language, -1));

            hb_shape(hbFont, m_hbBuffer, NULL, 0);
            m_hbGlyphInfos = hb_buffer_get_glyph_infos(m_hbBuffer, &m_glyphCount);
            m_hbGlyphPositions = hb_buffer_get_glyph_positions(m_hbBuffer, &m_glyphCount);
        }

        ShapedString(const ShapedString&) = delete;
        ShapedString(const ShapedString&&) = delete;
        ShapedString& operator=(const ShapedString&) = delete;
        ShapedString& operator=(const ShapedString&&) = delete;

        ~ShapedString()
        {
            hb_buffer_destroy(m_hbBuffer);
        }

        unsigned int            m_glyphCount;
        hb_buffer_t*            m_hbBuffer;
        hb_glyph_info_t*        m_hbGlyphInfos;
        hb_glyph_position_t*    m_hbGlyphPositions;
        std::shared_ptr<Face>   m_face;
    };

    struct ShapedStringCacheInfo
    {
        std::shared_ptr<ShapedString>       m_shapedString;
        std::list<ShapedStringKey>::iterator m_usageFrequencyIter;
    };

    struct Face : std::enable_shared_from_this<Face>
    {
        Face(FT_Face face, hb_blob_t* hbBlob, int faceIndex, int priority, LPCRITICAL_SECTION lpCritSection)
            : m_ftFace(face)
            , m_hbBlob(hbBlob)
            , m_priority(priority)
            , m_lastSize(0)
            , m_lpCritSection(lpCritSection)
        {
            m_hbFace = hb_face_create(hbBlob, (unsigned int)faceIndex);
            m_hbFont = hb_font_create(m_hbFace);
        }

        ~Face()
        {
            EnterCriticalSection(m_lpCritSection);
            FT_Done_Face(m_ftFace);
            LeaveCriticalSection(m_lpCritSection);

            hb_font_destroy(m_hbFont);
            hb_face_destroy(m_hbFace);
            hb_blob_destroy(m_hbBlob);
        }

        void SetCharSize(int size);
        std::shared_ptr<ShapedString> GetShapedString(const std::u32string& str, int size);

        FT_Face             m_ftFace;
        hb_blob_t*          m_hbBlob;
        hb_face_t*          m_hbFace;
        hb_font_t*          m_hbFont;
        int                 m_priority;
        int                 m_lastSize;
        LPCRITICAL_SECTION  m_lpCritSection;
    };

    struct TextureCachePartitionRow;

    struct TextureCachePartition : std::enable_shared_from_this<TextureCachePartition>
    {
        TextureCachePartition(RECT rect, std::weak_ptr<TextureCachePartitionRow> parent)
            : m_rect(rect)
            , m_glyphRect{0, 0, 0, 0}
            , m_parent(parent)
            , m_inUse(false)
        {
            ResetToUnused();
        }

        void ResetToUnused();

        void MarkInUse(int width, int height);
        void ClearInUse();
        bool IsInUse() const { return m_inUse; }
        int GetWidth() const { return (m_rect.right - m_rect.left) + 1; }
        int GetHeight() const { return (m_rect.bottom - m_rect.top) + 1; }
        void SetNewRect(RECT newRect);
        RECT GetRect() const { return m_rect; }
        bool CouldInsertGlyph(int width, int height) const;
        void TryCombiningWithNeighbors();

        RECT                                    m_rect;
        RECT                                    m_glyphRect;
        std::weak_ptr<TextureCachePartitionRow> m_parent;
        bool                                    m_inUse;
    };

    struct TextureCacheInfo;

    struct TextureCachePartitionRow : std::enable_shared_from_this<TextureCachePartitionRow>
    {
        TextureCachePartitionRow(RECT rect, std::weak_ptr<TextureCacheInfo> parent)
            : m_rect(rect)
            , m_maxChildHeight(0)
            , m_parent(parent)
        {
        }

        void ResetToUnused();

        void TryCombiningChildren(std::shared_ptr<TextureCachePartition> hint);
        std::shared_ptr<TextureCachePartition> TryAddingChildGlyph(int width, int height);

        bool CouldFitGlyph(int width, int height) const;
        void SetMaxChildHeight(int maxHeight) { m_maxChildHeight = maxHeight; }
        int GetMaxChildHeight() const { return m_maxChildHeight; }
        int GetWidth() const { return (m_rect.right - m_rect.left) + 1; }
        int GetHeight() const { return (m_rect.bottom - m_rect.top) + 1; }
        void SetNewRect(RECT newRect);
        RECT GetRect() const { return m_rect; }

        RECT                                                m_rect;
        int                                                 m_maxChildHeight;
        std::weak_ptr<TextureCacheInfo>                     m_parent;
        std::list<std::shared_ptr<TextureCachePartition>>   m_children;
    };

    struct TextureCacheInfo : std::enable_shared_from_this<TextureCacheInfo>
    {
        TextureCacheInfo(LONG dimension, int textureIndex)
            : m_textureSize(RECT{ 0, 0, dimension - 1, dimension - 1 })
            , m_textureIndex(textureIndex)
        {
        }

        void TryCombiningChildren(std::shared_ptr<TextureCachePartitionRow> hint);
        std::shared_ptr<TextureCachePartition> TryInsertNewPartition(int width, int height);
        void ResetToUnused();
        int GetWidth() const { return (m_textureSize.right - m_textureSize.left) + 1; }
        int GetHeight() const { return (m_textureSize.bottom - m_textureSize.top) + 1; }

        RECT                                                    m_textureSize;
        std::list<std::shared_ptr<TextureCachePartitionRow>>    m_partitionRows;
        int                                                     m_textureIndex;
    };

    struct GlyphCacheInfo
    {
        GlyphCacheInfo()
            : m_metrics{}
        {
        }

        std::list<GlyphKey>::iterator           m_usageFrequencyIter;
        std::shared_ptr<TextureCacheInfo>       m_textureCacheInfo;
        std::shared_ptr<TextureCachePartition>  m_textureCachePartition;
        FT_Glyph_Metrics                        m_metrics;
        std::shared_ptr<Face>                   m_face;
    };

    struct PendingUpload
    {
        PendingUpload(FT_Library& ftLibrary, const FT_Bitmap& originalBitmap, GlyphKey glyph)
            : m_ftLibrary(ftLibrary)
            , m_bitmap{}
            , m_glyph(glyph)
        {
            FT_Bitmap_Copy(m_ftLibrary, &originalBitmap, &m_bitmap);
        }

        ~PendingUpload()
        {
            FT_Bitmap_Done(m_ftLibrary, &m_bitmap);
            m_uploadMemoryHandle.Reset();
        }

        FT_Library&         m_ftLibrary;
        FT_Bitmap           m_bitmap;
        GlyphKey            m_glyph;
        GraphicsResource    m_uploadMemoryHandle;
    };

    struct PendingDraw
    {
        PendingDraw(const std::list<std::shared_ptr<ShapedString>>& shapedStrings, int size, float x, float y, const DirectX::FXMVECTOR color)
            : m_color(color)
            , m_shapedStrings(shapedStrings)
            , m_size(size)
            , m_x(x)
            , m_y(y)
        {
        }

        ~PendingDraw() = default;
       
        DirectX::SimpleMath::Color                  m_color;
        std::list<std::shared_ptr<ShapedString>>    m_shapedStrings;
        int                                         m_size;
        float                                       m_x;
        float                                       m_y;
    };

public:

    Impl(size_t maxTextures, LONG textureDimension, size_t maxCachedShapedStrings, _In_ ID3D12Device* d3dDevice);
    ~Impl();

    void Lock();
    void Unlock();

    size_t GetTextureCount();
    void CreateTextures(_In_ ID3D12CommandQueue* commandQueue, _In_ DescriptorHeap* descriptorHeap, size_t descriptorHeapOffset);
    void CreateDeviceDependentResources(_In_ ID3D12CommandQueue* commandQueue, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat);    
    void CreateWindowSizeDependentResources(D3D12_VIEWPORT viewport);

    void Render(_In_ ID3D12GraphicsCommandList* commandList);
    void RenderFrameAdvance();

    void RenderPendingUploads(_In_ ID3D12GraphicsCommandList* commandList);
    void RenderPendingDraws(_In_ ID3D12GraphicsCommandList* commandList);
    void DrawTextureBatch(_In_ SpriteBatch* spriteBatch);
    void ClearCacheTextures(_In_ ID3D12GraphicsCommandList* commandList);

    void XM_CALLCONV DrawString(_In_ ID3D12GraphicsCommandList* commandList,
        _In_ SpriteBatch* spriteBatch,
        _In_z_ char const* utf8String,
        DirectX::XMFLOAT2 const& position,
        DirectX::FXMVECTOR color,
        float rotation,
        DirectX::XMFLOAT2 const& origin,
        float fontSize,
        DirectX::SpriteEffects effects,
        float layerDepth );
    bool LoadFont(const std::string& fontPath, const std::vector<UnicodeRange>& preferredRanges, int priority, int faceIndex);

    std::list<std::shared_ptr<ShapedString>> CalculateShapedStrings(const char* str, int size);
    std::shared_ptr<ShapedString> GetShapedString(std::shared_ptr<Face> face, std::u32string& str, int size);

    void PrecacheGlyphs(const char* str, int size);
    void PrecacheGlyphs(const std::list<std::shared_ptr<ShapedString>>& shapedStrings, int size);

    std::shared_ptr<Face> GetPreferredFace(char32_t unicodeCodepoint, UnicodeRange currentStringRange);

    bool IsGlyphCached(const GlyphKey& glyph) const;
    void UpdateGlyphUsageFrequency(const GlyphKey& glyph);

    void ClearCache();

    void XM_CALLCONV DrawText(const char* str, int size, float x, float y, DirectX::FXMVECTOR color = Colors::White);

    void MeasureText(const char* str, int size, int* outDrawWidth, int* outDrawHeight);
   
protected:

    void RenderGlyphToCache(const GlyphKey& glyph, bool useMissingGlyphSymbol = false);
    void ClearGlyphFromCache(const GlyphKey& glyph);

    void BeginTextureBatch(_In_ ID3D12GraphicsCommandList* commandList);
    void EndTextureBatch();

    std::shared_ptr<TextureCachePartition> TryInsertNewPartition(int width, int height, int* outTextureIndex);
    bool EjectOldGlyph();

protected:

    // Basic
    FT_Library                                                  m_ftLibrary;
    CRITICAL_SECTION                                            m_ftCritSection;
    CRITICAL_SECTION                                            m_threadSafetyLock;
    ID3D12Device*                                               m_d3dDevice;

    // Fonts/Faces
    std::list<std::shared_ptr<Face>>                            m_allFaces;
    std::map<UnicodeRange, std::list<std::shared_ptr<Face>>>    m_preferredFaces;
    std::map<UnicodeRange, std::list<std::shared_ptr<Face>>>    m_fallbackFaces;

    // Glyphs
    std::map<GlyphKey, GlyphCacheInfo>                          m_glyphCacheInfo;
    std::map<GlyphKey, GlyphCacheInfo>                          m_glyphCacheInfo_NonPrintable;
    std::list<GlyphKey>                                         m_glyphUsageFrequency;

    // Shaped Strings
    std::map<ShapedStringKey, ShapedStringCacheInfo>            m_shapedStringCache;
    std::list<ShapedStringKey>                                  m_shapedStringUsageFrequency;
    size_t                                                      m_maxCachedShapedStrings;

    // Textures
    std::vector<std::shared_ptr<TextureCacheInfo>>              m_textureCacheInfos;
    std::unique_ptr<DirectX::DescriptorHeap>                    m_textureDescriptorHeap;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>         m_textureResources;
    std::unique_ptr<DirectX::SpriteBatch>                       m_textureBatch;
    bool                                                        m_clearCacheTextures;    

    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>                    m_GpuDescriptorHandles;

    // Pending Actions
    std::vector<std::shared_ptr<PendingUpload>>                 m_pendingUploads;
    std::vector<std::shared_ptr<PendingDraw>>                   m_pendingDraws;
    std::vector<std::shared_ptr<PendingUpload>>                 m_delayedFreeUploads;
    std::vector<std::shared_ptr<PendingUpload>>                 m_currentUploads;
};

void GlyphCache::Impl::Face::SetCharSize(int size)
{
    if (m_lastSize != size && size > 0)
    {
        FT_Error error = FT_Set_Char_Size(m_ftFace, 0, size * 64, 0, 0);
        if (error == FT_Err_Ok)
        {
            m_lastSize = size;
        }

        hb_font_set_scale(m_hbFont, size * 64, size * 64);
    }
}

std::shared_ptr<GlyphCache::Impl::ShapedString> GlyphCache::Impl::Face::GetShapedString(const std::u32string& str, int size)
{
    if (str.size() == 0)
    {
        return {};
    }

    SetCharSize(size);
    UnicodeRange assumedRange = GetRangeForUTF32Character(str[0]);
    return std::make_shared<ShapedString>(shared_from_this(), m_hbFont, str,
        GetHBScriptForUnicodeRange(assumedRange),
        GetLanguageCodeForUnicodeRange(assumedRange));
}

void GlyphCache::Impl::TextureCachePartition::ResetToUnused()
{
    ClearInUse();
}

void GlyphCache::Impl::TextureCachePartition::MarkInUse(int width, int height)
{
    assert(!m_inUse);

    m_inUse = true;
    m_glyphRect = m_rect;
    m_glyphRect.right = m_glyphRect.left + (width - 1);
    m_glyphRect.bottom = m_glyphRect.top + (height - 1);
}

void GlyphCache::Impl::TextureCachePartition::ClearInUse()
{
    m_inUse = false;
    m_glyphRect = { 0, 0, 0, 0 };
}

bool GlyphCache::Impl::TextureCachePartition::CouldInsertGlyph(int width, int height) const
{
    return m_inUse == false &&
        width <= GetWidth() &&
        height <= GetHeight();
}

void GlyphCache::Impl::TextureCachePartition::SetNewRect(RECT newRect)
{
    assert(!m_inUse);

    m_rect = newRect;
}

void GlyphCache::Impl::TextureCachePartition::TryCombiningWithNeighbors()
{
    auto parentPtr = m_parent.lock();
    if (parentPtr)
    {
        parentPtr->TryCombiningChildren(shared_from_this());
    }
}

void GlyphCache::Impl::TextureCachePartitionRow::ResetToUnused()
{
    m_children.clear();
    m_children.push_back(std::make_shared<GlyphCache::Impl::TextureCachePartition>(m_rect, shared_from_this()));
    m_children.back()->ResetToUnused();
}

void GlyphCache::Impl::TextureCachePartitionRow::TryCombiningChildren(std::shared_ptr<TextureCachePartition> /*hint*/)
{
    auto iter = m_children.begin();
    while (iter != m_children.end())
    {
        auto lastElementIter = iter;
        ++iter;
        if (iter == m_children.end())
        {
            break;
        }

        if (lastElementIter->get()->IsInUse() && iter->get()->IsInUse())
        {
            // Combine into later iter
            RECT combinedRect = lastElementIter->get()->GetRect();
            combinedRect.right = iter->get()->GetRect().right;
            iter->get()->SetNewRect(combinedRect);

            // Remove old iter
            m_children.erase(lastElementIter);
        }
    }

    // Tell parent to try combining its children too
    if (m_children.size() == 1 && !m_children.front()->IsInUse())
    {
        SetMaxChildHeight(0);
        m_parent.lock()->TryCombiningChildren(shared_from_this());
    }
}

std::shared_ptr<GlyphCache::Impl::TextureCachePartition> GlyphCache::Impl::TextureCachePartitionRow::TryAddingChildGlyph(int width, int height)
{
    // Tried to add a new child glyph to the row. If it succeeds, then a pointer to the new glyph is added and children are split/updated
    // as needed. Updates internal data as well.
    auto partitionIter = m_children.begin();
    while (partitionIter != m_children.end())
    {
        auto oldPartition = *partitionIter;

        if (oldPartition->CouldInsertGlyph(width, height))
        {
            bool needsSplit = oldPartition->GetWidth() > width;
            if (needsSplit)
            {
                RECT oldRect = oldPartition->GetRect();

                // Create new partition to go before the old partition
                RECT newRect = oldRect;
                newRect.right = newRect.left + (width - 1);
                auto newPartition = std::make_shared<GlyphCache::Impl::TextureCachePartition>(newRect, shared_from_this());
                newPartition->ResetToUnused();
                m_children.insert(partitionIter, newPartition);

                // Update the old partition
                RECT updatedOldRect = oldRect;
                updatedOldRect.left = newRect.right + 1;
                oldPartition->SetNewRect(updatedOldRect);

                // Use the new rect and return it
                newPartition->MarkInUse(width, height);
                return newPartition;
            }
            else
            {
                oldPartition->MarkInUse(width, height);
                return oldPartition;
            }
        }

        ++partitionIter;
    }

    return {};
}

bool GlyphCache::Impl::TextureCachePartitionRow::CouldFitGlyph(int width, int height) const
{
    return width <= ((m_rect.right - m_rect.left) + 1) &&
        height <= ((m_rect.bottom - m_rect.top) + 1);
}

void GlyphCache::Impl::TextureCachePartitionRow::SetNewRect(RECT newRect)
{
    assert(m_children.size() == 1 && !m_children.front()->m_inUse);

    m_rect = newRect;
    m_children.front()->SetNewRect(newRect);
}

void GlyphCache::Impl::TextureCacheInfo::TryCombiningChildren(std::shared_ptr<TextureCachePartitionRow> /*hint*/)
{
    auto iter = m_partitionRows.begin();
    while (iter != m_partitionRows.end())
    {
        auto lastElementIter = iter;
        ++iter;
        if (iter == m_partitionRows.end())
        {
            break;
        }

        if (lastElementIter->get()->GetMaxChildHeight() == 0 && iter->get()->GetMaxChildHeight() == 0)
        {
            // Combine into later iter
            RECT combinedRect = lastElementIter->get()->GetRect();
            combinedRect.bottom = iter->get()->GetRect().bottom;
            iter->get()->SetNewRect(combinedRect);

            // Remove old iter
            m_partitionRows.erase(lastElementIter);
        }
    }
}

std::shared_ptr<GlyphCache::Impl::TextureCachePartition> GlyphCache::Impl::TextureCacheInfo::TryInsertNewPartition(int width, int height)
{
    assert(width > 0 && height > 0);

    auto rowIterator = m_partitionRows.begin();
    while (rowIterator != m_partitionRows.end())
    {
        // Check if it's possible for the row to fit the glyph
        if (rowIterator->get()->CouldFitGlyph(width, height))
        {
            auto oldRow = *rowIterator;

            LONG potentialNewRowHeight = AlignUp(height, 4);

            bool unassignedMaxHeight = rowIterator->get()->GetMaxChildHeight() == 0;
            bool rowTallerThanGlyph = rowIterator->get()->GetHeight() > height;
            bool needsSplit = unassignedMaxHeight &&
                rowTallerThanGlyph &&
                potentialNewRowHeight < rowIterator->get()->GetHeight();

            if (needsSplit)
            {
                RECT originalRowRect = rowIterator->get()->GetRect();

                // Split row into multiple
                // New row it the top
                // Old row moves to the bottom
                RECT newRowRect = originalRowRect;
                newRowRect.bottom = newRowRect.top + (potentialNewRowHeight - 1);
                std::shared_ptr<GlyphCache::Impl::TextureCachePartitionRow> newRow =
                    std::make_shared<GlyphCache::Impl::TextureCachePartitionRow>(newRowRect, shared_from_this());
                newRow->ResetToUnused();
                m_partitionRows.insert(rowIterator, newRow);

                // Fixup old row's max height in the rect
                RECT oldRowFixedRect = originalRowRect;
                oldRowFixedRect.top = newRowRect.bottom + 1;
                oldRow->SetNewRect(oldRowFixedRect);

                // Place glyph within new row
                // This should not fail
                newRow->SetMaxChildHeight(height);
                return newRow->TryAddingChildGlyph(width, height);
            }
            else
            {
                if (unassignedMaxHeight)
                {
                    oldRow->SetMaxChildHeight(height);
                }

                // Check if there is an open spot in the row
                auto newChild = oldRow->TryAddingChildGlyph(width, height);
                if (newChild)
                {
                    return newChild;
                }
                }
            }

        ++rowIterator;
        }

    return {};
    }

void GlyphCache::Impl::TextureCacheInfo::ResetToUnused()
{
    m_partitionRows.clear();
    m_partitionRows.push_back(std::make_shared<TextureCachePartitionRow>(m_textureSize, shared_from_this()));
    m_partitionRows.back()->ResetToUnused();
}

_Use_decl_annotations_
GlyphCache::Impl::Impl(size_t maxTextures, LONG textureDimension, size_t maxCachedShapedStrings, ID3D12Device* d3dDevice)
    : m_ftLibrary(nullptr)
    , m_d3dDevice(d3dDevice)
    , m_maxCachedShapedStrings(maxCachedShapedStrings)
#ifdef _DEBUG
    , m_clearCacheTextures(true)
#else
    , m_clearCacheTextures(false)
#endif
{
    if (maxTextures == 0)
    {
        throw std::exception("GlyphCache requires at least 1 texture to be usable");
    }

    FT_Error error = FT_Init_FreeType(&m_ftLibrary);
    if (error != FT_Err_Ok)
    {
        char buf[128] = { 0 };
        sprintf_s(buf, 128, "Failed to initialize FreeType2 with code [%d]", error);
        throw std::exception(buf);
    }

    InitializeCriticalSection(&m_ftCritSection);
    InitializeCriticalSection(&m_threadSafetyLock);

    m_textureCacheInfos.reserve(maxTextures);
    for (size_t textureIndex = 0; textureIndex < maxTextures; ++textureIndex)
    {
        m_textureCacheInfos.push_back(std::make_shared<TextureCacheInfo>(textureDimension, static_cast<int>(textureIndex)));
        m_textureCacheInfos[textureIndex]->ResetToUnused();
    }
}

GlyphCache::Impl::~Impl()
{
    ClearCache();

    m_delayedFreeUploads.clear();
    m_currentUploads.clear();

    m_allFaces.clear();
    m_preferredFaces.clear();
    m_fallbackFaces.clear();
    
    if (m_ftLibrary)
    {
        FT_Done_FreeType(m_ftLibrary);
    }

    DeleteCriticalSection(&m_threadSafetyLock);
    DeleteCriticalSection(&m_ftCritSection);
}

void GlyphCache::Impl::Lock()
{
    EnterCriticalSection(&m_threadSafetyLock);
}

void GlyphCache::Impl::Unlock()
{
    LeaveCriticalSection(&m_threadSafetyLock);
}

HRESULT CreateTextureResource(
    ID3D12Device* d3dDevice,
    D3D12_RESOURCE_DIMENSION resDim,
    size_t width,
    size_t height,
    size_t depth,
    size_t mipCount,
    size_t arraySize,
    DXGI_FORMAT format,
    D3D12_RESOURCE_FLAGS resFlags,
    D3D12_RESOURCE_STATES initialState,
    ID3D12Resource** outTexture) noexcept
{
    if (!d3dDevice)
    {
        return E_POINTER;
    }

    HRESULT hr = E_FAIL;

    D3D12_RESOURCE_DESC desc = {};
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.MipLevels = static_cast<UINT16>(mipCount);
    desc.DepthOrArraySize = (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE3D) ? static_cast<UINT16>(depth) : static_cast<UINT16>(arraySize);
    desc.Format = format;
    desc.Flags = resFlags;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Dimension = resDim;

    CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    hr = d3dDevice->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        initialState,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(outTexture));
    if (SUCCEEDED(hr))
    {
        assert(outTexture != nullptr && *outTexture != nullptr);
        _Analysis_assume_(outTexture != nullptr && *outTexture != nullptr);

        SetDebugObjectName(*outTexture, L"GlyphCacheTexture");
    }

    return hr;
}

size_t GlyphCache::Impl::GetTextureCount()
{
    return m_textureCacheInfos.size();
}

_Use_decl_annotations_
void GlyphCache::Impl::CreateTextures(ID3D12CommandQueue* commandQueue, DescriptorHeap* descriptorHeap, size_t descriptorHeapOffset)
{
    ResourceUploadBatch upload(m_d3dDevice);

    upload.Begin();
   
    // Create textures for texture cache
    RECT dimensionRect = m_textureCacheInfos[0]->m_textureSize;
    m_textureResources.resize(m_textureCacheInfos.size());
    for (size_t index = 0; index < m_textureCacheInfos.size(); ++index)
    {
        HRESULT result = CreateTextureResource(m_d3dDevice,
            D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            size_t(dimensionRect.right) - size_t(dimensionRect.left) + 1,
            size_t(dimensionRect.bottom) - size_t(dimensionRect.top) + 1,
            1, 1, 1, DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            m_textureResources[index].ReleaseAndGetAddressOf());
        if (FAILED(result))
        {
            throw std::exception("Failed to create all texture resources for GlyphCache");
        }
    }

    upload.End(commandQueue);

    // SRVs
    for (size_t index = 0; index < m_textureCacheInfos.size(); ++index)
    {
        const auto desc = m_textureResources[index].Get()->GetDesc();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 0, 0, 0);
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = (!desc.MipLevels) ? UINT(-1) : desc.MipLevels;

        m_d3dDevice->CreateShaderResourceView(m_textureResources[index].Get(), &srvDesc, descriptorHeap->GetCpuHandle(descriptorHeapOffset + index));
        m_GpuDescriptorHandles.push_back(descriptorHeap->GetGpuHandle(descriptorHeapOffset + index));
    }
}

_Use_decl_annotations_
void GlyphCache::Impl::CreateDeviceDependentResources(ID3D12CommandQueue* commandQueue, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat)
{
    // Init texture descriptor heap
    m_textureDescriptorHeap = std::make_unique<DescriptorHeap>(m_d3dDevice,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, m_textureCacheInfos.size());

    RenderTargetState rtState(backBufferFormat, depthBufferFormat);
    SpriteBatchPipelineStateDescription spritePsoDesc(rtState, &CommonStates::AlphaBlend);
    ResourceUploadBatch upload(m_d3dDevice);

    upload.Begin();

    // Init texture batch
    m_textureBatch = std::make_unique<SpriteBatch>(m_d3dDevice, upload, spritePsoDesc);

    upload.End(commandQueue);

    CreateTextures(commandQueue, m_textureDescriptorHeap.get(), 0);    
}

void GlyphCache::Impl::CreateWindowSizeDependentResources(D3D12_VIEWPORT viewport)
{
    m_textureBatch->SetViewport(viewport);
}

_Use_decl_annotations_
void GlyphCache::Impl::Render(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"GlyphCache Render");

    if (m_clearCacheTextures)
    {
        ClearCacheTextures(commandList);
        m_clearCacheTextures = false;
    }

    RenderPendingUploads(commandList);
    RenderPendingDraws(commandList);

    PIXEndEvent(commandList);
}

void GlyphCache::Impl::RenderFrameAdvance()
{
    m_delayedFreeUploads.clear();
    std::swap(m_delayedFreeUploads, m_currentUploads);
}

_Use_decl_annotations_
void GlyphCache::Impl::RenderPendingUploads(ID3D12GraphicsCommandList* commandList)
{
    if (m_pendingUploads.size() == 0)
    {
        return;
    }

    for (std::shared_ptr<PendingUpload> pendingUpload : m_pendingUploads)
    {
        // Non-printable characters don't queue an upload, so don't bother checking that map
        auto glyphCacheInfoIter = m_glyphCacheInfo.find(pendingUpload->m_glyph);
        if (glyphCacheInfoIter == m_glyphCacheInfo.end())
        {
            throw std::exception("Failed to find glyph cache info for a pending upload. "
                "The cache likely churned through more glyphs than it can cache in a frame. "
                "Increase the glyph cache size to resolve.");
        }
        auto& glyphCacheInfo = glyphCacheInfoIter->second;
        auto texturePartition = glyphCacheInfo.m_textureCachePartition;

        // NOTE: D3D12_TEXTURE_DATA_PITCH_ALIGNMENT isn't in the Xbox headers?
#ifdef _GAMING_XBOX_SCARLETT
        size_t d3d12TextureDataPitchAlignment = D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT;
#elif (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
        size_t d3d12TextureDataPitchAlignment = D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT;
#else
        size_t d3d12TextureDataPitchAlignment = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
#endif

        // Describe Upload Buffer
        D3D12_SUBRESOURCE_FOOTPRINT uploadDesc;
        uploadDesc.Format = DXGI_FORMAT_R8_UNORM;
        uploadDesc.Width = (UINT)texturePartition->GetWidth();
        uploadDesc.Height = (UINT)texturePartition->GetHeight();
        uploadDesc.Depth = 1;
        uploadDesc.RowPitch = (UINT)AlignUp(uploadDesc.Width, d3d12TextureDataPitchAlignment);

        // Allocate upload buffer
        GraphicsMemory& graphicsMemory = GraphicsMemory::Get(m_d3dDevice);
        pendingUpload->m_uploadMemoryHandle = graphicsMemory.Allocate(size_t(uploadDesc.Height) * size_t(uploadDesc.RowPitch), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

        // Describe placed texture
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTexture2D;
        placedTexture2D.Offset = pendingUpload->m_uploadMemoryHandle.ResourceOffset();
        placedTexture2D.Footprint = uploadDesc;

        // Fill buffer
        // Since source comes as a single color, this has to manually fill out RGBA values
        // The actual glyph is 2 pixels less in each dimension.
        // Fill out the 1-pixel border when copying data
        for (UINT row = 0; row < uploadDesc.Height; ++row)
        {
            UINT8* rowWrite = reinterpret_cast<UINT8*>(pendingUpload->m_uploadMemoryHandle.Memory()) + (size_t(row) * size_t(uploadDesc.RowPitch));
            if (row == 0 || row == uploadDesc.Height - 1)
            {
                memset(rowWrite, 0, uploadDesc.Width);
            }
            else
            {
                for (unsigned int col = 0; col < pendingUpload->m_bitmap.width + 2; ++col)
                {
                    if (col == 0 || col == (pendingUpload->m_bitmap.width + 1))
                    {
                        *(rowWrite + col) = 0;
                    }
                    else if((row - 1) < pendingUpload->m_bitmap.rows)
                    {
                        const unsigned char* bitmapSource = pendingUpload->m_bitmap.buffer + (size_t(row - 1) * size_t(pendingUpload->m_bitmap.pitch));
                        *(rowWrite + col) = bitmapSource[col - 1];
                    }
                    else
                    {
                        *(rowWrite + col) = 0;
                    }
                }
            }
        }

        // Transition target to writable
        int targetTextureIndex = glyphCacheInfo.m_textureCacheInfo->m_textureIndex;
        CD3DX12_RESOURCE_BARRIER barrierToCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(m_textureResources[(size_t)targetTextureIndex].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->ResourceBarrier(1, &barrierToCopyDest);

        // Copy data from buffer to texture
        auto texturePartitionInfo = glyphCacheInfo.m_textureCachePartition;
        CD3DX12_TEXTURE_COPY_LOCATION destCopyLoc = CD3DX12_TEXTURE_COPY_LOCATION(m_textureResources[(size_t)targetTextureIndex].Get(), 0);
        CD3DX12_TEXTURE_COPY_LOCATION sourceCopyLoc = CD3DX12_TEXTURE_COPY_LOCATION(pendingUpload->m_uploadMemoryHandle.Resource(), placedTexture2D);
        commandList->CopyTextureRegion(
            &destCopyLoc,
            (UINT)texturePartitionInfo->GetRect().left, (UINT)texturePartitionInfo->GetRect().top, 0,
            &sourceCopyLoc,
            nullptr
        );

        // Transition target to usable
        CD3DX12_RESOURCE_BARRIER barrierToResource = CD3DX12_RESOURCE_BARRIER::Transition(m_textureResources[(size_t)targetTextureIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &barrierToResource);
    }

    // Don't free until next frame so that rendering can perform its uploads
    m_currentUploads.insert(m_currentUploads.end(), m_pendingUploads.begin(), m_pendingUploads.end());
    m_pendingUploads.clear();
}

_Use_decl_annotations_
void GlyphCache::Impl::RenderPendingDraws(ID3D12GraphicsCommandList* commandList)
{
    if (m_pendingDraws.size() == 0)
    {
        return;
    }
    
    BeginTextureBatch(commandList);    
    DrawTextureBatch(m_textureBatch.get());
    EndTextureBatch();    
    m_pendingDraws.clear();
}

_Use_decl_annotations_
void GlyphCache::Impl::DrawTextureBatch(SpriteBatch* spriteBatch)
{
    for (std::shared_ptr<PendingDraw> pendingDraw : m_pendingDraws)
    {
        float currentX = pendingDraw->m_x;
        float currentY = pendingDraw->m_y;
        for (auto shapedString : pendingDraw->m_shapedStrings)
        {
            for(unsigned int glyphIndex = 0; glyphIndex < shapedString->m_glyphCount; ++glyphIndex)
            {
                GlyphKey glyph = { shapedString->m_face, shapedString->m_hbGlyphInfos[glyphIndex].codepoint, pendingDraw->m_size };

                // Non-printable
                auto glyphCacheInfoIter = m_glyphCacheInfo_NonPrintable.find(glyph);
                if (glyphCacheInfoIter != m_glyphCacheInfo_NonPrintable.end())
                {
                    currentX += float(shapedString->m_hbGlyphPositions[glyphIndex].x_advance) / 64.0f;
                    continue;
                }

                // Printable
                glyphCacheInfoIter = m_glyphCacheInfo.find(glyph);
                if (glyphCacheInfoIter == m_glyphCacheInfo.end())
                {
                    throw std::exception("Failed to find cached glyph for a string draw. "
                        "This likely means that there were too many draws this frame to fit all glyphs in the cache. "
                        "Increase the glyph cache size or reduce the number of draws to resolve.");
                }
                auto& glyphCacheInfo = glyphCacheInfoIter->second;
                auto textureCacheInfo = glyphCacheInfo.m_textureCacheInfo;
                auto texturePartitionInfo = glyphCacheInfo.m_textureCachePartition;

                XMFLOAT2 position(currentX, currentY);
                XMFLOAT2 offset(-float(glyphCacheInfo.m_metrics.horiBearingX / 64), float((glyphCacheInfo.m_metrics.horiBearingY / 64) - pendingDraw->m_size));
                offset.x += float(shapedString->m_hbGlyphPositions[glyphIndex].x_offset) / 64.0f;
                offset.y += float(shapedString->m_hbGlyphPositions[glyphIndex].y_offset) / 64.0f;

                RECT drawGlyphRect = texturePartitionInfo->m_glyphRect;
                drawGlyphRect.left += 1;
                drawGlyphRect.top += 1;
                spriteBatch->Draw(
                    /*SRV*/m_GpuDescriptorHandles[(size_t)textureCacheInfo->m_textureIndex],
                    /*TextureSize*/XMUINT2((uint32_t)textureCacheInfo->GetWidth(), (uint32_t)textureCacheInfo->GetHeight()),
                    /*Position*/XMLoadFloat2(&position),
                    /*SourceRect*/&drawGlyphRect,
                    /*Color*/pendingDraw->m_color,
                    /*Rotation*/0.0f,
                    /*Origin*/XMLoadFloat2(&offset),
                    /*Scale*/1.0f,
                    /*Effects*/SpriteEffects_None,
                    /*LayerDepth*/0
                );

                currentX += float(shapedString->m_hbGlyphPositions[glyphIndex].x_advance / 64);
            }
        }
    }
}

_Use_decl_annotations_
void GlyphCache::Impl::ClearCacheTextures(ID3D12GraphicsCommandList* commandList)
{
    // NOTE: D3D12_TEXTURE_DATA_PITCH_ALIGNMENT isn't in the Xbox headers?
#ifdef _GAMING_XBOX_SCARLETT
    size_t d3d12TextureDataPitchAlignment = D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT;
#elif (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
    size_t d3d12TextureDataPitchAlignment = D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT;
#else
    size_t d3d12TextureDataPitchAlignment = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
#endif

    // Describe Upload Buffer
    RECT dimensionRect = m_textureCacheInfos[0]->m_textureSize;
    D3D12_SUBRESOURCE_FOOTPRINT uploadDesc;
    uploadDesc.Format = DXGI_FORMAT_R8_UNORM;
    uploadDesc.Width = UINT(dimensionRect.right - dimensionRect.left) + 1;
    uploadDesc.Height = UINT(dimensionRect.bottom - dimensionRect.top) + 1;
    uploadDesc.Depth = 1;
    uploadDesc.RowPitch = (UINT)AlignUp(uploadDesc.Width, d3d12TextureDataPitchAlignment);

    // Allocate upload buffer
    GraphicsMemory& graphicsMemory = GraphicsMemory::Get(m_d3dDevice);
    auto uploadMemoryHandle = graphicsMemory.Allocate(size_t(uploadDesc.Height) * size_t(uploadDesc.RowPitch), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

    // Describe placed texture
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTexture2D;
    placedTexture2D.Offset = uploadMemoryHandle.ResourceOffset();
    placedTexture2D.Footprint = uploadDesc;

    // Fill buffer
    memset(uploadMemoryHandle.Memory(), 0, size_t(uploadDesc.Width) * size_t(uploadDesc.RowPitch));

    for (size_t index = 0; index < m_textureCacheInfos.size(); ++index)
    {
        // Transition target to writable
        CD3DX12_RESOURCE_BARRIER barrierToCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(m_textureResources[index].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->ResourceBarrier(1, &barrierToCopyDest);

        // Copy data from buffer to texture
        CD3DX12_TEXTURE_COPY_LOCATION destCopyLoc = CD3DX12_TEXTURE_COPY_LOCATION(m_textureResources[index].Get(), 0);
        CD3DX12_TEXTURE_COPY_LOCATION sourceCopyLoc = CD3DX12_TEXTURE_COPY_LOCATION(uploadMemoryHandle.Resource(), placedTexture2D);
        commandList->CopyTextureRegion(
            &destCopyLoc,
            0, 0, 0,
            &sourceCopyLoc,
            nullptr
        );

        // Transition target to usable
        CD3DX12_RESOURCE_BARRIER barrierToResource = CD3DX12_RESOURCE_BARRIER::Transition(m_textureResources[index].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &barrierToResource);
    }
}

_Use_decl_annotations_
void XM_CALLCONV GlyphCache::Impl::DrawString(ID3D12GraphicsCommandList* commandList,
    DirectX::SpriteBatch* spriteBatch,
    _In_z_ char const* utf8String,
    DirectX::XMFLOAT2 const& position,
    DirectX::FXMVECTOR color,
    float,
    DirectX::XMFLOAT2 const&,
    float fontSize,
    DirectX::SpriteEffects,
    float)
{
    DrawText(utf8String, static_cast<int>(fontSize), position.x, position.y, color);

    if (m_clearCacheTextures)
    {
        ClearCacheTextures(commandList);
        m_clearCacheTextures = false;
    }

    RenderPendingUploads(commandList);

    if (m_pendingDraws.size() == 0)
    {
        return;
    }
    DrawTextureBatch(spriteBatch);
    m_pendingDraws.clear();
}

static std::vector<UnicodeRange> s_allUnicodeRanges =
{
    UnicodeRange::Other,
    UnicodeRange::LatinSymbols,
    UnicodeRange::LatinAlphaNumberic,
    UnicodeRange::LatinSupplementalExtended,
    UnicodeRange::PrivateUseArea,
    UnicodeRange::Korean,
    UnicodeRange::Japanese,
    UnicodeRange::Chinese,
    UnicodeRange::Russian,
    UnicodeRange::Bengali,
    UnicodeRange::Thai,
    UnicodeRange::Greek,
    UnicodeRange::Hindi,
    UnicodeRange::Arabic,
    UnicodeRange::Hebrew
};

bool GlyphCache::Impl::LoadFont(const std::string& fontPath, const std::vector<UnicodeRange>& preferredRanges, int priority, int faceIndex)
{
    FT_Face newFace;
    EnterCriticalSection(&m_ftCritSection);
    FT_Error error = FT_New_Face(m_ftLibrary, fontPath.c_str(), faceIndex, &newFace);
    LeaveCriticalSection(&m_ftCritSection);
    if (error != FT_Err_Ok)
    {
        char buf[512] = { 0 };
        sprintf_s(buf, 512, "Failed to open font [%s] faceIndex [%d] with error code [%d]\n", fontPath.c_str(), faceIndex, error);
        OutputDebugStringA(buf);

        return false;
    }

    // Store new face, sorted by priority
    hb_blob_t* hbBlob = hb_blob_create_from_file(fontPath.c_str());
    if (!hbBlob)
    {
        char buf[512] = { 0 };
        sprintf_s(buf, 512, "Failed to open font [%s] faceIndex [%d] with error code [%d]\n", fontPath.c_str(), faceIndex, error);
        OutputDebugStringA(buf);

        return false;
    }
    std::shared_ptr<Face> face = std::make_shared<Face>(newFace, hbBlob, faceIndex, priority, &m_ftCritSection);
    {
        auto iter = m_allFaces.begin();
        while (iter != m_allFaces.end())
        {
            if (iter->get()->m_priority < priority)
            {
                break;
            }

            ++iter;
        }
        m_allFaces.insert(iter, face);
    }

    // Sort into prioritized map as well
    for (UnicodeRange range : preferredRanges)
    {
        auto& faces = m_preferredFaces[range];
        auto iter = faces.begin();
        while (iter != faces.end())
        {
            if (iter->get()->m_priority < priority)
            {
                break;
            }

            ++iter;
        }
        faces.insert(iter, face);
    }

    // Finally, create another list to go through for fallback glyph purposes (optimization)
    for (size_t index = 0; index < s_allUnicodeRanges.size(); ++index)
    {
        if(std::find(preferredRanges.begin(), preferredRanges.end(), s_allUnicodeRanges[index]) != preferredRanges.end())
        {
            continue;
        }

        auto& faces = m_fallbackFaces[s_allUnicodeRanges[index]];
        auto iter = faces.begin();
        while (iter != faces.end())
        {
            if (iter->get()->m_priority < priority)
            {
                break;
            }

            ++iter;
        }
        faces.insert(iter, face);
    }

    return true;
}

std::list<std::shared_ptr<GlyphCache::Impl::ShapedString>> GlyphCache::Impl::CalculateShapedStrings(const char* str, int size)
{
    if (str == nullptr || strlen(str) == 0)
    {
        return {};
    }

    // Split string by ranges
    std::u32string utf32String = DX::Utf8ToUtf32(str);
    std::list<std::shared_ptr<ShapedString>> shapedStrings;
    UnicodeRange prevRange = GetRangeForUTF32Character(utf32String[0]);
    std::shared_ptr<Face> prevPreferredFace = GetPreferredFace(utf32String[0], prevRange);
    std::u32string buildingString(utf32String.begin(), utf32String.begin() + 1);
    for (size_t index = 1; index < utf32String.length(); ++index)
    {
        UnicodeRange curRange = GetRangeForUTF32Character(utf32String[index]);
        std::shared_ptr<Face> curPreferredFace = GetPreferredFace(utf32String[index], prevRange);
        if (curPreferredFace != prevPreferredFace)
        {
            std::shared_ptr<ShapedString> shapedStr = GetShapedString(prevPreferredFace, buildingString, size);
            shapedStrings.push_back(shapedStr);

            buildingString.clear();
        }

        prevRange = curRange;
        prevPreferredFace = curPreferredFace;
        buildingString.push_back(utf32String[index]);
    }
    {
        std::shared_ptr<ShapedString> shapedStr = GetShapedString(prevPreferredFace, buildingString, size);
        shapedStrings.push_back(shapedStr);
    }

    return shapedStrings;
}

std::shared_ptr<GlyphCache::Impl::ShapedString> GlyphCache::Impl::GetShapedString(std::shared_ptr<Face> face, std::u32string& str, int size)
{
    ShapedStringKey shapedStringKey = { str, size };

    // Look in cache first
    auto iter = m_shapedStringCache.find(shapedStringKey);
    if (iter != m_shapedStringCache.end())
    {
        // Bring usage to front
        m_shapedStringUsageFrequency.erase(iter->second.m_usageFrequencyIter);
        m_shapedStringUsageFrequency.push_front(shapedStringKey);
        iter->second.m_usageFrequencyIter = m_shapedStringUsageFrequency.begin();

        return iter->second.m_shapedString;
    }

    // Remove old cache value if needed
    if (m_shapedStringUsageFrequency.size() == m_maxCachedShapedStrings)
    {
        m_shapedStringCache.erase(m_shapedStringUsageFrequency.back());
        m_shapedStringUsageFrequency.pop_back();
    }

    // Create a new shaped string
    auto shapedString = face->GetShapedString(str, size);

    // Cache it
    m_shapedStringUsageFrequency.push_front(shapedStringKey);
    ShapedStringCacheInfo cacheInfo;
    cacheInfo.m_shapedString = shapedString;
    cacheInfo.m_usageFrequencyIter = m_shapedStringUsageFrequency.begin();
    m_shapedStringCache[shapedStringKey] = cacheInfo;

    return shapedString;
}

void GlyphCache::Impl::PrecacheGlyphs(const char* str, int size)
{
    if (m_allFaces.size() == 0)
    {
        throw std::exception("Cannot precache glyphs for a string if no fonts have been loaded.");
    }

    auto shapedStrings = CalculateShapedStrings(str, size);
    PrecacheGlyphs(shapedStrings, size);
}

void GlyphCache::Impl::PrecacheGlyphs(const std::list<std::shared_ptr<ShapedString>>& shapedStrings, int size)
{
    for (std::shared_ptr<ShapedString> shapedString : shapedStrings)
    {
        for (unsigned int glyphIndex = 0; glyphIndex < shapedString->m_glyphCount; ++glyphIndex)
        {
            GlyphKey glyph = { shapedString->m_face, shapedString->m_hbGlyphInfos[glyphIndex].codepoint, size };
            if (!IsGlyphCached(glyph))
            {
                RenderGlyphToCache(glyph);
            }
        }
    }
}


std::shared_ptr<GlyphCache::Impl::Face> GlyphCache::Impl::GetPreferredFace(char32_t unicodeCodepoint, UnicodeRange currentStringRange)
{
    UnicodeRange preferredRange = GetRangeForUTF32Character(unicodeCodepoint);
    if (preferredRange == UnicodeRange::LatinSymbols || preferredRange == UnicodeRange::LatinSupplementalExtended || unicodeCodepoint == U' ')
    {
        preferredRange = currentStringRange;
    }
    std::list<std::shared_ptr<Face>>& preferredFaces = preferredRange == UnicodeRange::Other ? m_allFaces : m_preferredFaces[preferredRange];
    std::list<std::shared_ptr<Face>>& fallbackFaces = m_fallbackFaces[preferredRange];

    for (auto face : preferredFaces)
    {
        if (FT_Get_Char_Index(face->m_ftFace, unicodeCodepoint) != 0)
        {
            return face;
        }
    }
    for (auto face : fallbackFaces)
    {
        if (FT_Get_Char_Index(face->m_ftFace, unicodeCodepoint) != 0)
        {
            return face;
        }
    }

    return m_allFaces.front();
}

bool GlyphCache::Impl::IsGlyphCached(const GlyphKey& glyph) const
{
    return
        (m_glyphCacheInfo.find(glyph) != m_glyphCacheInfo.end()) ||
        (m_glyphCacheInfo_NonPrintable.find(glyph) != m_glyphCacheInfo_NonPrintable.end());
}

void GlyphCache::Impl::UpdateGlyphUsageFrequency(const GlyphKey& glyph)
{
    // Only have to update for printable characters.
    // Non-printable characters can be cached indefinitely
    auto iter = m_glyphCacheInfo.find(glyph);
    if (iter != m_glyphCacheInfo.end())
    {
        m_glyphUsageFrequency.erase(iter->second.m_usageFrequencyIter);
        m_glyphUsageFrequency.push_front(glyph);
        iter->second.m_usageFrequencyIter = m_glyphUsageFrequency.begin();
    }
}

void GlyphCache::Impl::ClearCache()
{
    // Clearing the cache simply gets rid of all meta data so that the textures can be used anew.
    // No actual rendering needs to be done.
    m_glyphCacheInfo.clear();
    m_glyphCacheInfo_NonPrintable.clear();
    m_glyphUsageFrequency.clear();
    m_shapedStringCache.clear();
    m_shapedStringUsageFrequency.clear();
    for (auto& textureCacheInfo : m_textureCacheInfos)
    {
        textureCacheInfo->ResetToUnused();
    }

    // Clear any pending uploads and draws too, since those depend upon cached data.
    m_pendingUploads.clear();
    m_pendingDraws.clear();
}

void XM_CALLCONV GlyphCache::Impl::DrawText(const char* str, int size, float x, float y, FXMVECTOR color)
{
    if (str == nullptr || strlen(str) == 0)
    {
        return;
    }

    auto shapedStrings = CalculateShapedStrings(str, size);
    PrecacheGlyphs(shapedStrings, size);

    std::shared_ptr<PendingDraw> newDraw = std::make_shared<PendingDraw>(shapedStrings, size, x, y, color);
    m_pendingDraws.push_back(newDraw);
}

void GlyphCache::Impl::MeasureText(const char* str, int size, int* outDrawWidth, int* outDrawHeight)
{
    if (str == nullptr || strlen(str) == 0)
    {
        if (outDrawWidth)
        {
            (*outDrawWidth) = 0;
        }
        if (outDrawHeight)
        {
            (*outDrawHeight) = 0;
        }
        return;
    }

    auto shapedStrings = CalculateShapedStrings(str, size);
    PrecacheGlyphs(shapedStrings, size);

    int drawWidth = 0;
    int drawHeight = 0;
    for (auto shapedString : shapedStrings)
    {
        for (unsigned int index = 0; index < shapedString->m_glyphCount; ++index)
        {
            GlyphKey glyph = { shapedString->m_face, shapedString->m_hbGlyphInfos[index].codepoint, size };

            auto iter = m_glyphCacheInfo.find(glyph);
            if (iter == m_glyphCacheInfo.end())
            {
                iter = m_glyphCacheInfo_NonPrintable.find(glyph);
                if (iter == m_glyphCacheInfo_NonPrintable.end())
                {
                    throw std::exception("MeasureText failed to find a glyph in the cache. "
                        "This is likely due to the cache being too small. "
                        "Please increase the number of textures and/or increase texture size.");
                }
            }

            drawWidth += shapedString->m_hbGlyphPositions[index].x_advance;

            int glyphDrawHeight = 0;
            if (iter->second.m_metrics.horiBearingY >= iter->second.m_metrics.height)
            {
                glyphDrawHeight = iter->second.m_metrics.horiBearingY;
            }
            else
            {
                glyphDrawHeight = iter->second.m_metrics.height + (iter->second.m_metrics.height - iter->second.m_metrics.horiBearingY);
            }

            drawHeight = glyphDrawHeight > drawHeight ? glyphDrawHeight : drawHeight;
        }
    }

    if (outDrawWidth)
    {
        (*outDrawWidth) = drawWidth / 64;
    }
    if (outDrawHeight)
    {
        (*outDrawHeight) = drawHeight / 64;
    }
}

void GlyphCache::Impl::RenderGlyphToCache(const GlyphKey& glyph, bool useMissingGlyphSymbol)
{
    glyph.m_face->SetCharSize(glyph.m_size);

    // Load glyph
    // Don't load bitmap from the font directly so we can always get anti-aliased 8bpp images from the following render.
    FT_Error error = FT_Load_Glyph(glyph.m_face->m_ftFace, useMissingGlyphSymbol ? 0 : glyph.m_glyphIndex, FT_LOAD_NO_BITMAP);
    if (error != FT_Err_Ok)
    {
        char buf[256] = { 0 };
        sprintf_s(buf, 256, "FT_Load_Glyph failed. GlyphIndex:%d FamilyName:%s Size:%d Error:%d\n",
            useMissingGlyphSymbol ? 0 : glyph.m_glyphIndex,
            glyph.m_face->m_ftFace->family_name ? glyph.m_face->m_ftFace->family_name : "",
            glyph.m_size,
            error);
        OutputDebugStringA(buf);

        if (glyph.m_glyphIndex != 0 && !useMissingGlyphSymbol)
        {
            RenderGlyphToCache(glyph, true);
        }

        return;
    }

    // CPU render glyph to bitmap buffer
    error = FT_Render_Glyph(glyph.m_face->m_ftFace->glyph, FT_RENDER_MODE_NORMAL);
    if (error != FT_Err_Ok)
    {
        char buf[256] = { 0 };
        sprintf_s(buf, 256, "FT_Load_Glyph failed. GlyphIndex:%d FamilyName:%s Size:%d Error:%d\n",
            useMissingGlyphSymbol ? 0 : glyph.m_glyphIndex,
            glyph.m_face->m_ftFace->family_name ? glyph.m_face->m_ftFace->family_name : "",
            glyph.m_size,
            error);
        OutputDebugStringA(buf);

        if (glyph.m_glyphIndex != 0 && !useMissingGlyphSymbol)
        {
            RenderGlyphToCache(glyph, true);
        }

        return;
    }

    bool printable = !(glyph.m_face->m_ftFace->glyph->bitmap.width == 0 && glyph.m_face->m_ftFace->glyph->bitmap.rows == 0);

    // Get a new partition from a texture
    // Increase width and height by 2 pixel for a border
    if (printable)
    {
        int textureIndex = 0;
        std::shared_ptr<TextureCachePartition> newPartition =
            TryInsertNewPartition(static_cast<int>(glyph.m_face->m_ftFace->glyph->bitmap.width + 2), static_cast<int>(glyph.m_face->m_ftFace->glyph->bitmap.rows + 2), &textureIndex);
        if (!newPartition)
        {
            char buf[256] = { 0 };
            sprintf_s(buf, 256, "Failed to insert glyph into cache. GlyphIndex:%d FamilyName:%s Size:%d\nLikely cause is size being too large for any single texture. Is the size a sane value?",
                useMissingGlyphSymbol ? 0 : glyph.m_glyphIndex,
                glyph.m_face->m_ftFace->family_name ? glyph.m_face->m_ftFace->family_name : "",
                glyph.m_size);
            throw std::exception(buf);
        }

        // Update glyph cache info
        m_glyphUsageFrequency.push_front(glyph);
        GlyphCacheInfo newCacheInfo;
        newCacheInfo.m_usageFrequencyIter = m_glyphUsageFrequency.begin();
        newCacheInfo.m_textureCacheInfo = m_textureCacheInfos[(size_t)textureIndex];
        newCacheInfo.m_textureCachePartition = newPartition;
        newCacheInfo.m_metrics = glyph.m_face->m_ftFace->glyph->metrics;
        newCacheInfo.m_face = glyph.m_face;
        m_glyphCacheInfo[glyph] = newCacheInfo;

        //Setup upload to the texture for the next render call
        std::shared_ptr<PendingUpload> newUpload = std::make_shared<PendingUpload>(m_ftLibrary, glyph.m_face->m_ftFace->glyph->bitmap, glyph);
        m_pendingUploads.push_back(newUpload);
    }
    else
    {
        // Update glyph cache info for non-printable character
        GlyphCacheInfo newCacheInfo;
        newCacheInfo.m_metrics = glyph.m_face->m_ftFace->glyph->metrics;
        m_glyphCacheInfo_NonPrintable[glyph] = newCacheInfo;
    }
}

void GlyphCache::Impl::ClearGlyphFromCache(const GlyphKey& glyph)
{
    auto glyphCacheInfoIter = m_glyphCacheInfo.find(glyph);
    auto nonPrintableGlyphCacheInfoIter = m_glyphCacheInfo_NonPrintable.find(glyph);

    if (glyphCacheInfoIter != m_glyphCacheInfo.end())
    {
        // Remove usage entry
        m_glyphUsageFrequency.erase(glyphCacheInfoIter->second.m_usageFrequencyIter);

        // Remove texture partition entry
        glyphCacheInfoIter->second.m_textureCachePartition->ClearInUse();
        glyphCacheInfoIter->second.m_textureCachePartition->TryCombiningWithNeighbors();

        // Remove info entry
        m_glyphCacheInfo.erase(glyphCacheInfoIter);
    }
    else if (nonPrintableGlyphCacheInfoIter != m_glyphCacheInfo_NonPrintable.end())
    {
        // Remove info entry
        m_glyphCacheInfo_NonPrintable.erase(nonPrintableGlyphCacheInfoIter);
    }
}

_Use_decl_annotations_
void GlyphCache::Impl::BeginTextureBatch(ID3D12GraphicsCommandList* commandList)
{
    auto heap = m_textureDescriptorHeap->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_textureBatch->Begin(commandList);
}

void GlyphCache::Impl::EndTextureBatch()
{
    m_textureBatch->End();
}

std::shared_ptr<GlyphCache::Impl::TextureCachePartition> GlyphCache::Impl::TryInsertNewPartition(int width, int height, int* outTextureIndex)
{
    do
    {
        for (size_t index = 0; index < m_textureCacheInfos.size(); ++index)
        {
            auto newPartition = m_textureCacheInfos[index]->TryInsertNewPartition(width, height);
            if (newPartition)
            {
                if (outTextureIndex)
                {
                    (*outTextureIndex) = static_cast<int>(index);
                }
                return newPartition;
            }
        }


    } while (EjectOldGlyph());

    return {};
}

bool GlyphCache::Impl::EjectOldGlyph()
{
    if (m_glyphUsageFrequency.size() == 0)
    {
        return false;
    }

    ClearGlyphFromCache(m_glyphUsageFrequency.back());

    return true;
}

_Use_decl_annotations_
GlyphCache::GlyphCache(size_t maxTextures, LONG textureDimension, size_t maxCachedShapedStrings, ID3D12Device* d3dDevice)
    : pImpl(std::make_unique<Impl>(maxTextures, textureDimension, maxCachedShapedStrings, d3dDevice))
{
}

GlyphCache::GlyphCache(GlyphCache&&) noexcept = default;
GlyphCache& GlyphCache::operator= (GlyphCache&&) noexcept = default;
GlyphCache::~GlyphCache() = default;


_Use_decl_annotations_
void GlyphCache::CreateDeviceDependentResources(ID3D12CommandQueue* commandQueue, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat)
{
    pImpl->Lock();
    pImpl->CreateDeviceDependentResources(commandQueue, backBufferFormat, depthBufferFormat);
    pImpl->Unlock();
}

void GlyphCache::CreateWindowSizeDependentResources(D3D12_VIEWPORT viewport)
{
    pImpl->Lock();
    pImpl->CreateWindowSizeDependentResources(viewport);
    pImpl->Unlock();
}

_Use_decl_annotations_
void GlyphCache::Render(ID3D12GraphicsCommandList* commandList, bool frameAdvance)
{
    pImpl->Lock();
    pImpl->Render(commandList);
    pImpl->Unlock();

    if (frameAdvance)
    {
        RenderFrameAdvance();
    }
}

void GlyphCache::RenderFrameAdvance()
{
    pImpl->Lock();
    pImpl->RenderFrameAdvance();
    pImpl->Unlock();
}

bool GlyphCache::LoadFont(const std::string& fontPath, const std::vector<UnicodeRange>& preferredRanges, int priority, int faceIndex)
{
    pImpl->Lock();
    bool result = pImpl->LoadFont(fontPath, preferredRanges, priority, faceIndex);
    pImpl->Unlock();
    return result;
}

bool GlyphCache::LoadNotoFonts(const std::string& optSubDir)
{
    std::string path = ".";
    if (optSubDir.size() != 0)
    {
        path = optSubDir;
    }

    bool overallResult = true;
    overallResult = LoadFont(path + "\\" + "NotoSans-Regular.ttf", { ATG::UnicodeRange::LatinSymbols, ATG::UnicodeRange::LatinSupplementalExtended, ATG::UnicodeRange::LatinAlphaNumberic }, 10) && overallResult;
    overallResult = LoadFont(path + "\\" + "NotoSansCJKkr-Regular.otf", { ATG::UnicodeRange::Korean }, 0) && overallResult;
    overallResult = LoadFont(path + "\\" + "NotoSansCJKjp-Regular.otf", { ATG::UnicodeRange::Japanese }, 0) && overallResult;
    overallResult = LoadFont(path + "\\" + "NotoSansCJKsc-Regular.otf", { ATG::UnicodeRange::Chinese }, 0) && overallResult;
    overallResult = LoadFont(path + "\\" + "NotoSans-Regular.ttf", { ATG::UnicodeRange::Russian }, 0) && overallResult;
    overallResult = LoadFont(path + "\\" + "NotoSerifBengali-Regular.ttf", { ATG::UnicodeRange::Bengali }, 0) && overallResult;
    overallResult = LoadFont(path + "\\" + "NotoSerifThai-Regular.ttf", { ATG::UnicodeRange::Thai }, 0) && overallResult;
    overallResult = LoadFont(path + "\\" + "NotoSans-Regular.ttf", { ATG::UnicodeRange::Greek }, 0) && overallResult;
    overallResult = LoadFont(path + "\\" + "NotoSerifDevanagari-Regular.ttf", { ATG::UnicodeRange::Hindi }, 0) && overallResult;
    overallResult = LoadFont(path + "\\" + "NotoNaskhArabic-Regular.ttf", { ATG::UnicodeRange::Arabic }, 0) && overallResult;
    overallResult = LoadFont(path + "\\" + "NotoSerifHebrew-Regular.ttf", { ATG::UnicodeRange::Hebrew }, 0) && overallResult;
    return overallResult;
}

void GlyphCache::PrecacheGlyphs(const char* utf8String, int fontSize)
{
    pImpl->Lock();
    pImpl->PrecacheGlyphs(utf8String, fontSize);
    pImpl->Unlock();
}

void XM_CALLCONV GlyphCache::EnqueueDrawText(const char* utf8String, int fontSize, float xPos, float yPos, FXMVECTOR color)
{
    pImpl->Lock();
    pImpl->DrawText(utf8String, fontSize, xPos, yPos, color);
    pImpl->Unlock();
}

void GlyphCache::MeasureText(const char* utf8String, int fontSize, int* outDrawWidth, int* outDrawHeight)
{
    pImpl->Lock();
    pImpl->MeasureText(utf8String, fontSize, outDrawWidth, outDrawHeight);
    pImpl->Unlock();
}

void GlyphCache::ClearCache()
{
    pImpl->Lock();
    pImpl->ClearCache();
    pImpl->Unlock();
}

size_t GlyphCache::GetTextureCount()
{
    pImpl->Lock();
    size_t textureCount = pImpl->GetTextureCount();
    pImpl->Unlock();

    return textureCount;
}

_Use_decl_annotations_
void GlyphCache::CreateTextures(ID3D12CommandQueue* commandQueue, DescriptorHeap* descriptorHeap, size_t descriptorHeapOffset)
{
    pImpl->Lock();
    pImpl->CreateTextures(commandQueue, descriptorHeap, descriptorHeapOffset);
    pImpl->Unlock();
}

_Use_decl_annotations_
void XM_CALLCONV GlyphCache::DrawString(ID3D12GraphicsCommandList* commandList,
    SpriteBatch* spriteBatch,
    char const* utf8String,
    XMFLOAT2 const& position,
    FXMVECTOR color,
    float rotation,
    XMFLOAT2 const& origin,
    float fontSize,
    SpriteEffects effects = DirectX::SpriteEffects_None,
    float layerDepth = 0.0f)
{
    pImpl->Lock();
    pImpl->DrawString(commandList, spriteBatch, utf8String, position, color, rotation, origin, fontSize, effects, layerDepth);
    pImpl->Unlock();    
}
