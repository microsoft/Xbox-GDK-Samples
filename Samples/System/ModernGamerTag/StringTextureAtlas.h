//--------------------------------------------------------------------------------------
// File: StringTextureAtlas.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "SpriteBatch.h"
#include "DescriptorHeap.h"
#include "CommonStates.h"
#include "SimpleMath.h"
#include "StringShaper.h"
#include <cstddef>
#include <cstdint>
#include <memory>

#include <StringUtil.h>
#include <set>
#include <queue>

namespace TextRenderer
{
    // Used to generate the ID's of empty evicted atlas item partitions.
    constexpr char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    class StringTextureAtlasItem;
    class AtlasPartitionRow;

    class StringTextureAtlas
    {
    public:
        StringTextureAtlas(uint16_t dimension, ID3D12Device* device);
        const StringTextureAtlasItem* GetStringAtlasItem(const std::string& utf8String, const int32_t fontSize);
        HRESULT Remove(const char* utf8String, const int32_t fontSize);

        void CommitUpdates(ID3D12GraphicsCommandList* commandList);
        void AddShapedStringToAtlas(std::unique_ptr<ShapedString> shapedString, std::string utf8String, int32_t fontSize);

        // Generates an item used to help with partitioning. It's also added to the evictedItem table.
        std::pair<ShapedStringKey, StringTextureAtlasItem*> GenerateEmptyItem();

        // When initialized, m_textureSize.right,bottom are decreased by 1 to account for m_textureSize.left,top starting at 0
        const RECT                                                          m_textureSize;
        const uint16_t                                                      m_dimension;
        std::unique_ptr<DirectX::DescriptorHeap>                            m_textureDescriptorHeap;
        Microsoft::WRL::ComPtr<ID3D12Resource>                              m_textureAtlasResource;

    private:
        // Returns an evicted item that resulted from combining 2 nodes.
        // If only 1 item is evicted, just return that node.
        StringTextureAtlasItem* CoalesceEvictedItems(StringTextureAtlasItem* leftItem, StringTextureAtlasItem* rightItem);

        // Combine empty rows. If only 1 row is empty, return the empty row
        // A row is not guarenteed to be empty/evicted like atlas items, so if both aren't empty, return the right row.
        AtlasPartitionRow* CoalesceRows(AtlasPartitionRow* leftRow, AtlasPartitionRow* rightRow);

        // DX12 fields
        ID3D12Device* m_d3dDevice;

        // A table of all committed items in the texture atlas.
        std::map<ShapedStringKey, std::unique_ptr<StringTextureAtlasItem>>  m_itemLookupTable;

        // A table of items that are waiting to be committed(drawn) to the texture atlas
        std::map<ShapedStringKey, std::unique_ptr<StringTextureAtlasItem>>  m_pendingAtlasItems;

        // A table of items that can be overwritten by other items.
        // Evicted items partitioned out of existence are deleted from this table.
        std::map<ShapedStringKey, std::unique_ptr<StringTextureAtlasItem>>  m_evictedItems;

        std::vector<std::unique_ptr<AtlasPartitionRow>>                     m_partitionRows;
    };

    // Used to organize ShapedStrings by row in the texture atlas.
    class AtlasPartitionRow
    {
    public:
        AtlasPartitionRow(RECT rect, StringTextureAtlasItem* evictedChild) :
            m_rowBoundsRect(rect),
            m_prev(nullptr),
            m_next(nullptr)
        {
            m_children.push_back(evictedChild);
            ResizeRow(rect);
        }

        ~AtlasPartitionRow() = default;

        uint32_t GetWidth() const;
        uint32_t GetHeight() const;
        RECT GetRowBoundsRect() const;

        // Should only be called on rows that only contain 1 evicted item.
        // Rows with only evicted items are considered empty.
        void ResizeRow(RECT rowBounds);

        // Represents the shaped strings within the row
        std::vector<StringTextureAtlasItem*>                  m_children;

        // Used to coalesce empty rows
        RECT                                                  m_rowBoundsRect;
        AtlasPartitionRow*                                    m_prev;
        AtlasPartitionRow*                                    m_next;
    };

    // Represents an ShapedString with a texture in the string texture atlas. 
    class StringTextureAtlasItem
    {
    public:
        // Used when adding a new item. Once added, it will need to be committed to the atlas.
        StringTextureAtlasItem(ShapedStringKey ssKey, std::unique_ptr<ShapedString> shapedString, RECT textureRect) :
            m_shapedString(shapedString ? std::move(shapedString) : nullptr),
            m_parentRow(nullptr),
            m_textureRect(textureRect),
            m_ssKey(ssKey),
            m_left(nullptr),
            m_right(nullptr),
#ifdef _DEBUG
            DEBUGfontSize(ssKey.m_fontSize),
            DEBUGfutf8String(ssKey.m_utf8String),
#endif
            m_isEvicted(false),
            m_isCommited(false)
        {
        }

        // Used when creating an empty evicted item.
        StringTextureAtlasItem() :
            m_shapedString(nullptr),
            m_parentRow(nullptr),
            m_textureRect({ -1,-1,-1,-1 }),
            m_ssKey({"", -1}),
            m_left(nullptr),
            m_right(nullptr),
#ifdef _DEBUG
            DEBUGfontSize(invalidFontSize),
            DEBUGfutf8String(""),
#endif
            m_isEvicted(false),
            m_isCommited(false)
        {
            Evict();
        }

        ~StringTextureAtlasItem() = default;

        void Commit();

        // Mark our Item as evicted and change the sskey utf8string so that if the same string is added to our
        // atlas and evicted again, it doesn't break the eviction lookup table.
        void Evict();

        bool IsCommited() const;
        bool IsEvicted() const;

        // Get the width and height of the shaped string.
        // If an item is evicted, it represents the width of empty space.
        uint32_t GetTextureWidth() const;

        // Used to get the height of an item.
        // When inserting an item into a row, this actually isn't used
        // as we care more about the height of the whole row than an individual item.
        uint32_t GetTextureHeight() const;

        // Assigns a parent row to the String Atlas item.
        void SetRow(AtlasPartitionRow* row);

        // This should only be called on EVICTED items.
        void Resize(RECT bounds);

        const std::unique_ptr<ShapedString>         m_shapedString;
        AtlasPartitionRow*                          m_parentRow;

        // A rectangle that represents the actual bounds of a shaped string in a partition.
        // It's probable that the row containing the StringTextureAtlasItem is taller than the string.
        // so this rect sepeartes the two.
        RECT                                        m_textureRect;

        // The key of this StringTextureAtlasItem that is used in the AtlasItem lookup tables.
        ShapedStringKey                             m_ssKey;

        // Used to coalesce evicted items.
        StringTextureAtlasItem*                     m_left;
        StringTextureAtlasItem*                     m_right;

    private:

#ifdef _DEBUG
        int32_t                                     DEBUGfontSize;
        std::string                                 DEBUGfutf8String;
#endif
        // If evicted, then the texture is set for removal, or it can be overwritten.
        bool                                        m_isEvicted;

        // If commited, then the texture area will be drawn to.
        bool                                        m_isCommited;
    };
}
