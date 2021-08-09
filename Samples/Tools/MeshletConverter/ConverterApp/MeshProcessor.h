//--------------------------------------------------------------------------------------
// MeshProcessor.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "MeshUtilities.h"
#include "TriangleAllocator.h"
#include "MeshletSet.h"

#include <memory>
#include <vector>

namespace fbxsdk
{
    class FbxNode;
    class FbxMesh;
}

namespace ATG
{
    class FbxTransformer;

    class MeshProcessor
    {
    public:
        MeshProcessor()
            : m_dccVertexCount(0)
        { }

        // Generates meshlets for the given FbxNode's mesh.
        // Returns whether the operation was successful.
        bool GenerateMeshlets(
            fbxsdk::FbxNode* node,
            const FbxTransformer& transformer,
            uint32_t meshletMaxVerts,
            uint32_t meshletMaxPrims,
            bool flipTriangles,
            bool force32BitIndices,
            MeshletSet& meshlet);

    private:
        void Reset();
        bool Extract(fbxsdk::FbxNode* node);
        void Optimize(const FbxTransformer& transformer, bool force32BitIndices);

        template <typename T>
        void Meshletize(uint32_t meshletMaxVerts, uint32_t meshletMaxPrims, MeshletSet& set);

    private:
        ExportVB                                m_vertexBuffer;
        ExportIB                                m_indexBuffer;

        uint32_t                                m_dccVertexCount;
        TriangleAllocator                       m_triAllocator;
        TriangleArray                           m_rawTriangles;
        std::vector<std::pair<size_t, size_t>>  m_subsets;

        std::vector<uint32_t>                   m_indexData;
        VertexArray                             m_vertexData;
    };
}
