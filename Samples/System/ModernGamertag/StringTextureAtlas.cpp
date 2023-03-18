//--------------------------------------------------------------------------------------
// File: StringTextureAtlas.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "StringTextureAtlas.h"

using namespace DirectX;

namespace TextRenderer
{
#pragma region TextureAtlas

    StringTextureAtlas::StringTextureAtlas(
        uint16_t dimension, ID3D12Device* device) :
        m_textureSize(RECT{ 0, 0, dimension - 1, dimension - 1 }),
        m_dimension(dimension),
        m_d3dDevice(device)
    {
        // Initialize a row with an empty object.
        std::pair<ShapedStringKey, StringTextureAtlasItem*> emptyItemPair = GenerateEmptyItem();
        auto row = std::make_unique<AtlasPartitionRow>(m_textureSize, emptyItemPair.second);

        // Add the a row ptr to our empty item
        emptyItemPair.second->SetRow(row.get());

        // Add our default row
        m_partitionRows.push_back(std::move(row));

        // Init texture descriptor heap
        m_textureDescriptorHeap = std::make_unique<DirectX::DescriptorHeap>(m_d3dDevice,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1);

        D3D12_RESOURCE_DESC desc = {};
        desc.Width = static_cast<uint32_t>(dimension);
        desc.Height = static_cast<uint32_t>(dimension);
        desc.MipLevels = static_cast<uint16_t>(1);
        desc.DepthOrArraySize = 1;
        desc.Format = DXGI_FORMAT_R8_UNORM;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        HRESULT hr = E_FAIL;

        hr = m_d3dDevice->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_textureAtlasResource.GetAddressOf()));

        if (SUCCEEDED(hr))
        {
            assert(m_textureAtlasResource.GetAddressOf() != nullptr && *m_textureAtlasResource.GetAddressOf() != nullptr);
            _Analysis_assume_(outTexture != nullptr && *outTexture != nullptr);

            SetDebugObjectName(*m_textureAtlasResource.GetAddressOf(), L"Multifont Texture Atlas");
        }

        // Create the SRV for the atlas
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 0, 0, 0);
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = (!desc.MipLevels) ? uint32_t(-1) : desc.MipLevels;

        m_d3dDevice->CreateShaderResourceView(m_textureAtlasResource.Get(), &srvDesc, m_textureDescriptorHeap->GetCpuHandle(0));
    }

    const StringTextureAtlasItem* StringTextureAtlas::GetStringAtlasItem(const std::string& utf8String, const int32_t fontSize)
    {
        ShapedStringKey ssKey = { utf8String, fontSize };
        if (m_itemLookupTable.find(ssKey) != m_itemLookupTable.end())
        {
            return m_itemLookupTable[ssKey].get();
        }
        else if (m_pendingAtlasItems.find(ssKey) != m_pendingAtlasItems.end())
        {
            return m_pendingAtlasItems[ssKey].get();
        }
        return nullptr;
    }

    HRESULT StringTextureAtlas::Remove(const char* utf8String, const int32_t fontSize)
    {
        ShapedStringKey beforeEvictionSSKey = { utf8String, fontSize };

        // If our item is pending, it hasn't been rendered to the atlas so return.
        if (m_pendingAtlasItems.find(beforeEvictionSSKey) != m_pendingAtlasItems.end())
        {
            return E_PENDING;
        }

        auto itemPairToEvict = m_itemLookupTable.find(beforeEvictionSSKey);
        if (itemPairToEvict == m_itemLookupTable.end())
        {
            return E_FAIL;
        }

        StringTextureAtlasItem* item = m_itemLookupTable.find(beforeEvictionSSKey)->second.get();
        AtlasPartitionRow* partitionRow = item->m_parentRow;

        // Mark our item as evicted
        item->Evict();

        // Move our newly evicted texture atlas item from the lookup table into our evicted table.
        // A new key was generated for the atlas item, so we store it as a new Key val pair in the evicted atlas. 
        m_evictedItems.insert(std::pair<ShapedStringKey,
            std::unique_ptr<StringTextureAtlasItem>>(item->m_ssKey,
                std::move(m_itemLookupTable.find(beforeEvictionSSKey)->second)));
        m_itemLookupTable.erase(beforeEvictionSSKey);

        // Coalesce evicted items
        StringTextureAtlasItem* mergedItem = CoalesceEvictedItems(item->m_left, item);
        CoalesceEvictedItems(mergedItem, mergedItem->m_right);

        // Coalesce empty rows
        AtlasPartitionRow* mergedrow = CoalesceRows(partitionRow->m_prev, partitionRow);
        CoalesceRows(mergedrow, mergedrow->m_next);

        return S_OK;
    }

    StringTextureAtlasItem* StringTextureAtlas::CoalesceEvictedItems(StringTextureAtlasItem* leftItem, StringTextureAtlasItem* rightItem)
    {
        // If only 1 item is evicted, just return that item
        if (rightItem == nullptr || (leftItem != nullptr && leftItem->IsEvicted() && !rightItem->IsEvicted()))
        {
            return leftItem;
        }
        else if (leftItem == nullptr || (rightItem != nullptr && rightItem->IsEvicted() && !leftItem->IsEvicted()))
        {
            return rightItem;
        }

        // The right item will ALWAYS be merged into the left item.
        RECT itemResizeRect = leftItem->m_textureRect;
        itemResizeRect.right = rightItem->m_textureRect.right;
        itemResizeRect.bottom = rightItem->m_textureRect.bottom;

        // Make the left item point to the right neighbors right neighbor. If that neighbor exist, point it back to our left item.
        leftItem->m_right = rightItem->m_right;
        if (rightItem->m_right)
        {
            rightItem->m_right->m_left = leftItem;
        }

        // Remove the right item from the rows children vector
        leftItem->m_parentRow->m_children.erase(std::remove_if(leftItem->m_parentRow->m_children.begin(), leftItem->m_parentRow->m_children.end(),
            [&](const StringTextureAtlasItem* ptr)
            {
                return ptr == rightItem;
            }), leftItem->m_parentRow->m_children.end());

        // Delete the evicted item from existence.
        m_evictedItems.erase(rightItem->m_ssKey);

        // Resize the left item to be the merged size.
        leftItem->Resize(itemResizeRect);

        return leftItem;
    }

    AtlasPartitionRow* StringTextureAtlas::CoalesceRows(AtlasPartitionRow* leftRow, AtlasPartitionRow* rightRow)
    {
        // If one of the rows are empty, return the empty row. If both rows aren't empty,
        // return the right row.
        if (rightRow == nullptr || (leftRow != nullptr &&
            leftRow->m_children.size() == 1 &&
            leftRow->m_children[0]->IsEvicted() &&
            (rightRow->m_children.size() > 1 || !rightRow->m_children[0]->IsEvicted())))
        {
            return leftRow;
        }
        else if (leftRow == nullptr || (rightRow != nullptr &&
            rightRow->m_children.size() == 1 &&
            rightRow->m_children[0]->IsEvicted() &&
            (leftRow->m_children.size() > 1 || !leftRow->m_children[0]->IsEvicted())))
        {
            return rightRow;
        }
        else if ((leftRow->m_children.size() > 1 || !leftRow->m_children[0]->IsEvicted()) &&
            (rightRow->m_children.size() > 1 || !rightRow->m_children[0]->IsEvicted()))
        {
            return rightRow;
        }

        // The right row will ALWAYS be merged into the left row.
        RECT itemResizeRect = leftRow->m_rowBoundsRect;
        itemResizeRect.right = rightRow->m_rowBoundsRect.right;
        itemResizeRect.bottom = rightRow->m_rowBoundsRect.bottom;

        // Clear out the right row children.
        m_evictedItems.erase(rightRow->m_children[0]->m_ssKey);
        rightRow->m_children.clear();

        // Make the left row point to the right neighbors right neighbor. If that neighbor exist, point it back to our left row.
        leftRow->m_next = rightRow->m_next;
        if (rightRow->m_next)
        {
            rightRow->m_next->m_prev = leftRow;
        }

        // Remove the right row from the rows vector
        m_partitionRows.erase(std::remove_if(m_partitionRows.begin(), m_partitionRows.end(),
            [&](const std::unique_ptr<AtlasPartitionRow>& ptr)
            {
                if (ptr.get() == rightRow)
                {
                    return true;
                }
                return ptr.get() == rightRow;
            }), m_partitionRows.end());

        // Resize the left row to be the merged size.
        leftRow->ResizeRow(itemResizeRect);

        return leftRow;
    }

    void StringTextureAtlas::CommitUpdates(ID3D12GraphicsCommandList* commandList)
    {
        // NOTE: D3D12_TEXTURE_DATA_PITCH_ALIGNMENT isn't in the Xbox headers?
#ifdef _GAMING_XBOX
        size_t d3d12TextureDataPitchAlignment = D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT;
#else
        size_t d3d12TextureDataPitchAlignment = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
#endif

        auto heap = m_textureDescriptorHeap->Heap();
        commandList->SetDescriptorHeaps(1, &heap);

        // Allocate upload buffer
        GraphicsMemory& graphicsMemory = GraphicsMemory::Get(m_d3dDevice);

        auto ssKItemPairIterator = m_pendingAtlasItems.begin();
        if (ssKItemPairIterator != m_pendingAtlasItems.end())
        {
            // Cant modify a map with an active iterator, so keep track of all items that need ownership to be swapped.
            std::vector<ShapedStringKey> committedItemKeys;

            // Transition the texture atlas to a D3D12_RESOURCE_STATE_COPY_DEST so we can copy to it.
            CD3DX12_RESOURCE_BARRIER barrierToCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(m_textureAtlasResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
            commandList->ResourceBarrier(1, &barrierToCopyDest);
            for (;ssKItemPairIterator != m_pendingAtlasItems.end(); ++ssKItemPairIterator)
            {
                StringTextureAtlasItem* item = ssKItemPairIterator->second.get();
                const ShapedString* ShapedString = item->m_shapedString.get();

                // Describe Upload Buffer
                D3D12_SUBRESOURCE_FOOTPRINT uploadDesc;
                uploadDesc.Format = DXGI_FORMAT_R8_UNORM;
                uploadDesc.Width = ShapedString->m_width;
                uploadDesc.Height = ShapedString->m_height;
                uploadDesc.Depth = 1;
                uploadDesc.RowPitch = (uint32_t)AlignUp(ShapedString->m_width, d3d12TextureDataPitchAlignment);
                GraphicsResource graphicsResource = graphicsMemory.Allocate(size_t(uploadDesc.Height) * size_t(uploadDesc.RowPitch), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

                // Describe placed texture
                D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTexture2D;
                placedTexture2D.Offset = graphicsResource.ResourceOffset();
                placedTexture2D.Footprint = uploadDesc;

                // Copy our dx12 shaped string texture to the texture atlas
                memcpy(graphicsResource.Memory(), ShapedString->m_data.get(), ShapedString->m_size);
                CD3DX12_TEXTURE_COPY_LOCATION destCopyLoc = CD3DX12_TEXTURE_COPY_LOCATION(m_textureAtlasResource.Get(), 0);
                CD3DX12_TEXTURE_COPY_LOCATION sourceCopyLoc = CD3DX12_TEXTURE_COPY_LOCATION(graphicsResource.Resource(), placedTexture2D);
                commandList->CopyTextureRegion(
                    &destCopyLoc,
                    (uint32_t)item->m_textureRect.left, (uint32_t)item->m_textureRect.top, 0,
                    &sourceCopyLoc,
                    nullptr
                );

                // Our texture is now in the commandlist to get added to the atlas, so mark it as comitted
                item->Commit();
                committedItemKeys.push_back(item->m_ssKey);
            }

            // Transition the atlas back to a SRV
            CD3DX12_RESOURCE_BARRIER barrierToResource = CD3DX12_RESOURCE_BARRIER::Transition(m_textureAtlasResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            commandList->ResourceBarrier(1, &barrierToResource);

            // Items are no longer pending, so move them into the Lookup table.
            auto committedItemKeyIter = committedItemKeys.begin();
            for (; committedItemKeyIter != committedItemKeys.end(); ++committedItemKeyIter)
            {
                // Move all commited items into the lookup table
                m_itemLookupTable.insert(std::pair<ShapedStringKey, std::unique_ptr<StringTextureAtlasItem>>(*committedItemKeyIter, std::move(m_pendingAtlasItems[*committedItemKeyIter])));

                // Remove item from pending atlas table.
                m_pendingAtlasItems.erase(*committedItemKeyIter);
            }
        }
    }

    std::pair<ShapedStringKey, StringTextureAtlasItem*> StringTextureAtlas::GenerateEmptyItem()
    {
        auto emptyItem = std::make_unique<StringTextureAtlasItem>();

        StringTextureAtlasItem* itemValue = emptyItem.get();

        ShapedStringKey ssKey = emptyItem->m_ssKey;
        //Add our new item to the evicted item table. The evicted table has ownership over ALL evicted items. BREAK POINT HERE, check if itemValue is danging
        m_evictedItems.insert(std::make_pair(emptyItem->m_ssKey, std::move(emptyItem)));

        return std::make_pair(ssKey, itemValue);
    }

    void StringTextureAtlas::AddShapedStringToAtlas(std::unique_ptr<ShapedString> shapedString, std::string utf8String, int32_t fontSize)
    {
        AtlasPartitionRow* prevRow = nullptr;
        auto rowIterator = m_partitionRows.begin();

        for (; rowIterator != m_partitionRows.end(); ++rowIterator)
        {
            // Because we are dealing with vectors and vectors do not support modifications with an active iterator,
            // we store the current row in a pointer. The only time the row is modified is if its split into 2 and an extra row
            // is added to the texture atlas.
            // If this is the case, we can also be certain that the shaped string were adding WILL fit.
            AtlasPartitionRow* currentRow = rowIterator->get();

            bool rowIsEmpty = currentRow->m_children.size() == 1 && currentRow->m_children[0]->IsEvicted();

            // Can you split current row to fit the child? If so, split the row on the X axis.
            // It's splittable if the row has only an evicted child and its taller than the current texture.
            // + 1 is to ensure that we dont split a row only 2 pixel taller than our shaped string. to account for padding of the string.
            if (rowIsEmpty && currentRow->GetHeight() > AlignUp(shapedString->m_height, 4) + 2)
            {
                uint32_t rowWereAddingHeight = AlignUp(shapedString->m_height, 4);
                RECT originalRowRect = currentRow->GetRowBoundsRect();

                // Split row into two, current row were adding to moves to the top, other created row moves to the bottom
                RECT rowWereAddingRect = originalRowRect;
                rowWereAddingRect.bottom = rowWereAddingRect.top + long(rowWereAddingHeight);
                currentRow->ResizeRow(rowWereAddingRect);

                // Create the new row and add the empty partition to it
                RECT rowWereNotAddingRect = originalRowRect;
                rowWereNotAddingRect.top = rowWereAddingRect.bottom + 1;

                std::pair<ShapedStringKey, StringTextureAtlasItem*> emptyItemPair = GenerateEmptyItem();
                auto newEmptyRow = std::make_unique<AtlasPartitionRow>(rowWereNotAddingRect, emptyItemPair.second);

                // Add the row ptr to our empty item. This will be used when evicting an item and we need to get its parent row.
                emptyItemPair.second->SetRow(newEmptyRow.get());

                // Update row neighbors
                if (prevRow)
                {
                    prevRow->m_next = currentRow;
                    currentRow->m_prev = prevRow;
                }

                // If were inserting in an evicted row NOT at the bottom of the atlas, ensure that the
                // empty row at the 2nd half of this row split is pointing to the correct rows
                if (currentRow->m_next != nullptr)
                {
                    newEmptyRow->m_next = currentRow->m_next;
                    newEmptyRow->m_next->m_prev = newEmptyRow.get();
                }

                currentRow->m_next = newEmptyRow.get();
                newEmptyRow->m_prev = currentRow;

                m_partitionRows.insert(rowIterator + 1, std::move(newEmptyRow));
            }

            // Attempt to add the item to the current row. If it doesn't fit, check the next row.
            if (currentRow->GetHeight() >= shapedString->m_height)
            {
                StringTextureAtlasItem* prevItem = nullptr;

                for (size_t i = 0; i < currentRow->m_children.size(); i++)
                {
                    if (currentRow->m_children[i]->IsEvicted())
                    {
                        StringTextureAtlasItem* evictedAtlasItem = currentRow->m_children[i];
                        uint32_t ssWidth = shapedString->m_width;
                        RECT evictedRect = evictedAtlasItem->m_textureRect;
                        RECT newRect = evictedRect;

                        // -1 accounts for the padding the strings have.
                        newRect.right = newRect.left + (long(shapedString->m_width) - 1);
                        newRect.bottom = newRect.top + (long(shapedString->m_height) - 1);

                        // Attempt to split the CURRENT evicted item across the Y axis, the new item goes to the left and evicted item to the right
                        if (evictedAtlasItem->GetTextureWidth() >= ssWidth)
                        {
                            ShapedStringKey ssKey = { utf8String, fontSize };
                            auto newItem = std::make_unique<StringTextureAtlasItem>(ssKey, std::move(shapedString), newRect);
                            newItem->SetRow(currentRow);

                            // Set the left bounds of our old evicted rect to the right side of our new item were adding
                            evictedRect.left = newRect.right + 1;
                            evictedAtlasItem->Resize(evictedRect);

                            // Link together the previous item in the row to our new item.
                            newItem->m_left = prevItem;
                            if (prevItem)
                            {
                                prevItem->m_right = newItem.get();
                            }

                            // If the evicted item ends up with a <= 0 width, its been partitioned out of existence so remove it.
                            if (evictedRect.left >= evictedRect.right)
                            {
                                // Make the new item next point to the evicted next.
                                newItem->m_right = evictedAtlasItem->m_right;

                                // Make the evicted items.next prev item point to the new item were inserting.
                                if (evictedAtlasItem->m_right)
                                {
                                    evictedAtlasItem->m_right->m_left = newItem.get();
                                }

                                // Remove our evicted item from our row
                                currentRow->m_children.erase(currentRow->m_children.begin() + (int64_t)i);

                                // Remove our evicted item from the evicted item table 
                                m_evictedItems.erase(evictedAtlasItem->m_ssKey);
                            }
                            else
                            {
                                // Link together the resized evicted item and our new item.
                                newItem->m_right = evictedAtlasItem;
                                evictedAtlasItem->m_left = newItem.get();
                            }

                            currentRow->m_children.insert(currentRow->m_children.begin() + (int64_t)i, newItem.get());

                            // Add our item to the pending commit table
                            m_pendingAtlasItems.insert(std::make_pair(ssKey, std::move(newItem)));
                            return;
                        }
                    }
                    prevItem = currentRow->m_children[i];
                }
            }
            prevRow = currentRow;
        }

        throw std::exception("String didn't fit in atlas. It was wider than the whole texture atlas.");
    }

#pragma endregion

#pragma region AtlasPartitionRow
    void AtlasPartitionRow::ResizeRow(RECT rowBounds)
    {
        // Resize the evicted child
        m_rowBoundsRect = rowBounds;
        m_children[0]->Resize(rowBounds);
    }

    uint32_t AtlasPartitionRow::GetWidth() const
    {
        return uint32_t(m_rowBoundsRect.right - m_rowBoundsRect.left);
    }

    uint32_t AtlasPartitionRow::GetHeight() const
    {
        return uint32_t(m_rowBoundsRect.bottom - m_rowBoundsRect.top);
    }

    RECT AtlasPartitionRow::GetRowBoundsRect() const
    {
        return m_rowBoundsRect;
    }

#pragma endregion

#pragma region StringTextureAtlasItem
    void StringTextureAtlasItem::SetRow(AtlasPartitionRow* row)
    {
        m_parentRow = row;
    }

    void StringTextureAtlasItem::Resize(RECT bounds)
    {
        if (!m_isEvicted)
        {
            throw std::exception("Tried to resize an unevicted item.");
        }
        m_textureRect = bounds;
    }

    void StringTextureAtlasItem::Commit()
    {
        m_isCommited = true;
    };

    void StringTextureAtlasItem::Evict()
    {
        if (m_isEvicted)
        {
            throw std::exception("Item already evicted.");
        }

        std::string randomString;

        for (int i = 0; i < 12; ++i)
        {
            randomString += alphanum[rand() % (sizeof(alphanum) - 1)];
        }
        m_ssKey.m_utf8String = randomString;
        m_ssKey.m_fontSize = invalidFontSize;
        m_isEvicted = true;

#ifdef _DEBUG
        DEBUGfontSize = -1;
        DEBUGfutf8String = randomString;
#endif
    };

    bool StringTextureAtlasItem::IsCommited() const
    {
        return m_isCommited;
    }

    bool StringTextureAtlasItem::IsEvicted() const
    {
        return m_isEvicted;
    }

    uint32_t StringTextureAtlasItem::GetTextureWidth() const
    {
        return uint32_t(m_textureRect.right - m_textureRect.left);
    }

    uint32_t StringTextureAtlasItem::GetTextureHeight() const
    {
        return uint32_t(m_textureRect.bottom - m_textureRect.top);
    }

#pragma endregion
}

