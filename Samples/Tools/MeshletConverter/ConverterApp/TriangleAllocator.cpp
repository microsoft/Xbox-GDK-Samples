//--------------------------------------------------------------------------------------
// TriangleAllocator.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "TriangleAllocator.h"

#include <algorithm>

using namespace ATG;
using namespace DirectX;

void TriangleAllocator::Terminate()
{
    AllocationBlockList::iterator iter = m_allocationBlocks.begin();
    AllocationBlockList::iterator end = m_allocationBlocks.end();
    while (iter != end)
    {
        AllocationBlock& block = *iter;
        delete[] block.TriangleArray;
        ++iter;
    }
    m_allocationBlocks.clear();
    m_totalCount = 0;
    m_allocatedCount = 0;
}

void TriangleAllocator::SetSizeHint(uint32_t uAnticipatedSize)
{
    if (uAnticipatedSize <= m_totalCount)
        return;
    uint32_t uNewCount = std::max<uint32_t>(uAnticipatedSize - m_totalCount, 10000);
    AllocationBlock NewBlock;
    NewBlock.TriangleCount = uNewCount;
    NewBlock.TriangleArray = new Triangle[uNewCount];
    m_allocationBlocks.push_back(NewBlock);
    m_totalCount += uNewCount;
}

Triangle* TriangleAllocator::GetNewTriangle()
{
    if (m_allocatedCount == m_totalCount)
        SetSizeHint(m_totalCount + 10000);
    uint32_t uIndex = m_allocatedCount;
    m_allocatedCount++;
    AllocationBlockList::iterator iter = m_allocationBlocks.begin();
    AllocationBlockList::iterator end = m_allocationBlocks.end();
    while (iter != end)
    {
        AllocationBlock& block = *iter;
        if (uIndex < block.TriangleCount)
        {
            Triangle* pTriangle = &block.TriangleArray[uIndex];
            pTriangle->Initialize();
            return pTriangle;
        }
        uIndex -= block.TriangleCount;
        ++iter;
    }
    assert(false);
    return nullptr;
}

void TriangleAllocator::ClearAllTriangles()
{
    m_allocatedCount = 0;
}

bool Vertex::Equals(const Vertex* pOtherVertex) const
{
    if (!pOtherVertex)
        return false;

    if (pOtherVertex == this)
        return true;

    XMVECTOR v0 = XMLoadFloat3(&Position);
    XMVECTOR v1 = XMLoadFloat3(&pOtherVertex->Position);
    if (XMVector3NotEqual(v0, v1))
        return false;

    XMVECTOR n0 = XMLoadFloat3(&Normal);
    XMVECTOR n1 = XMLoadFloat3(&pOtherVertex->Normal);
    if (XMVector3NotEqual(n0, n1))
        return false;

    return true;
}

uint32_t ATG::FindOrAddVertex(VertexArray& vertsArray, Vertex* testVertex)
{
    uint32_t index = testVertex->DCCVertexIndex;
    assert(index < vertsArray.size());

    if (Vertex* vertex = vertsArray[index])
    {
        Vertex* lastVertex = nullptr;
        while (vertex)
        {
            index = vertex->DCCVertexIndex;
            if (vertex->Equals(testVertex))
                return index;

            lastVertex = vertex;
            vertex = vertex->NextDuplicateVertex;
        }
        assert(vertex == nullptr);

        index = static_cast<uint32_t>(vertsArray.size());
        vertsArray.push_back(testVertex);
        testVertex->DCCVertexIndex = index;

        if (lastVertex)
        {
            lastVertex->NextDuplicateVertex = testVertex;
        }
    }
    else
    {
        vertsArray[index] = testVertex;
    }

    return index;
}
