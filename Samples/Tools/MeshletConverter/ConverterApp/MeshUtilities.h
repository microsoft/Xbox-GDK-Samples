//--------------------------------------------------------------------------------------
// MeshUtilities.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <cassert>
#include <inttypes.h>
#include <memory>

namespace ATG
{
    class ExportVB
    {
    public:
        ExportVB()
            : m_vertexSizeBytes(0)
            , m_vertexCount(0)
            , m_bufferSize(0)
        { }

        void SetVertexSize(uint32_t byteCount) { m_vertexSizeBytes = byteCount; }
        uint32_t GetVertexSize() const { return m_vertexSizeBytes; }

        void SetVertexCount(uint32_t vertexCount) { m_vertexCount = vertexCount; }
        uint32_t GetVertexCount() const { return m_vertexCount; }

        void Allocate();

        uint8_t* GetVertex(size_t index);
        const uint8_t* GetVertex(size_t index) const;

        uint8_t* GetVertexData() { return m_vertexData.get(); }
        const uint8_t* GetVertexData() const { return m_vertexData.get(); }

        uint32_t GetVertexDataSize() const { return m_vertexSizeBytes * m_vertexCount; }

    private:
        uint32_t                    m_vertexSizeBytes;
        uint32_t                    m_vertexCount;

        uint32_t                    m_bufferSize;
        std::unique_ptr<uint8_t[]>  m_vertexData;
    };


    class ExportIB
    {
    public:
        ExportIB()
            : m_indexSize(0)
            , m_indexCount(2)
            , m_bufferSize(0)
        { }

        void SetIndexSize(uint32_t indexSize) { assert(indexSize == 2 || indexSize == 4); m_indexSize = indexSize; }
        uint32_t GetIndexSize() const { return m_indexSize; }

        void SetIndexCount(uint32_t indexCount) { m_indexCount = indexCount; }
        uint32_t GetIndexCount() const { return m_indexCount; }

        void Allocate();

        uint32_t GetIndex(size_t index) const
        {
            if (m_indexSize == 2)
            {
                auto pIndexData16 = reinterpret_cast<const uint16_t*>(m_indexData.get());
                return pIndexData16[index];
            }
            else
            {
                auto pIndexData32 = reinterpret_cast<const uint32_t*>(m_indexData.get());
                return pIndexData32[index];
            }
        }

        void SetIndex(size_t index, uint32_t data)
        {
            if (m_indexSize == 2)
            {
                auto pIndexData16 = reinterpret_cast<uint16_t*>(m_indexData.get());
                pIndexData16[index] = static_cast<uint16_t>(data);
            }
            else
            {
                auto pIndexData32 = reinterpret_cast<uint32_t*>(m_indexData.get());
                pIndexData32[index] = data;
            }
        }

        uint8_t* GetIndexData() { return m_indexData.get(); }
        const uint8_t* GetIndexData() const { return m_indexData.get(); }
        size_t GetIndexDataSize() const { return m_indexCount * m_indexSize; }

    private:
        uint32_t                    m_indexSize;
        uint32_t                    m_indexCount;

        uint32_t                    m_bufferSize;
        std::unique_ptr<uint8_t[]>  m_indexData;
    };
}
