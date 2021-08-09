//--------------------------------------------------------------------------------------
// MeshUtilities.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "MeshUtilities.h"

using namespace ATG;

uint8_t* ExportVB::GetVertex(size_t uIndex)
{
    if (!m_vertexData)
        return nullptr;

    if (uIndex >= m_vertexCount)
        return nullptr;

    return m_vertexData.get() + (uIndex * m_vertexSizeBytes);
}

const uint8_t* ExportVB::GetVertex(size_t uIndex) const
{
    if (!m_vertexData)
        return nullptr;

    if (uIndex >= m_vertexCount)
        return nullptr;

    return m_vertexData.get() + (uIndex * m_vertexSizeBytes);
}


void ExportVB::Allocate()
{
    uint32_t byteSize = GetVertexDataSize();
    if (m_bufferSize < byteSize)
    {
        m_vertexData.reset(new uint8_t[byteSize]);
        m_bufferSize = byteSize;
    }

    std::memset(m_vertexData.get(), 0, m_bufferSize);
}

void ExportIB::Allocate()
{
    uint32_t byteSize = m_indexCount * m_indexSize;
    if (m_bufferSize < byteSize)
    {
        m_indexData.reset(new uint8_t[byteSize]);
        m_bufferSize = byteSize;
    }

    std::memset(m_indexData.get(), 0, m_bufferSize);
}
