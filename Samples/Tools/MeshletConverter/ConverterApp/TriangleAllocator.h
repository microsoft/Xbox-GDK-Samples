//--------------------------------------------------------------------------------------
// TriangleAllocator.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <DirectXMath.h>
#include <list>
#include <memory>
#include <vector>

namespace ATG
{
    struct Vertex
    {
        uint32_t            DCCVertexIndex;
        DirectX::XMFLOAT3   Position;
        DirectX::XMFLOAT3   Normal;
        Vertex*             NextDuplicateVertex;

        Vertex()
        {
            Initialize();
        }

        void Initialize()
        {
            std::memset(this, 0, sizeof(Vertex));
        }

        bool Equals(const Vertex* pOtherVertex) const;
    };
    using VertexArray = std::vector<Vertex*>;

    struct Triangle
    {
        Vertex  Vertex[3];
        int     SubsetIndex;
        int     PolygonIndex;

        Triangle()
            : SubsetIndex(0)
            , PolygonIndex(-1)
        { }

        void Initialize()
        {
            SubsetIndex = 0;
            Vertex[0].Initialize();
            Vertex[1].Initialize();
            Vertex[2].Initialize();
        }
    };
    using TriangleArray = std::vector<Triangle*>;

    class TriangleAllocator
    {
    public:
        TriangleAllocator()
            : m_totalCount(0)
            , m_allocatedCount(0)
        { }

        ~TriangleAllocator()
        {
            Terminate();
        }

        void Initialize() { SetSizeHint(50000); }
        void Terminate();
        void SetSizeHint(uint32_t uAnticipatedSize);
        Triangle* GetNewTriangle();
        void ClearAllTriangles();

    private:
        struct AllocationBlock
        {
            Triangle* TriangleArray;
            uint32_t  TriangleCount;
        };
        using AllocationBlockList = std::list<AllocationBlock>;

        AllocationBlockList m_allocationBlocks;
        uint32_t            m_totalCount;
        uint32_t            m_allocatedCount;
    };

    uint32_t FindOrAddVertex(VertexArray& vertsArray, Vertex* testVertex);
}
