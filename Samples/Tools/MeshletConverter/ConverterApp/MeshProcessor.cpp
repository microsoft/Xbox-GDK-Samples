//--------------------------------------------------------------------------------------
// MeshProcessor.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "MeshProcessor.h"

#include "FbxTransformer.h"

#include <cassert>
#include <algorithm>
#include <d3d12.h>
#include <DirectXMesh.h>
#include <fbxsdk.h>
#include <iostream>

using namespace ATG;
using namespace DirectX;

namespace
{
    void ThrowIfFailed(HRESULT hr)
    {
        if (hr != S_OK)
        {
            throw std::exception("Failed HRESULT!");
        }
    }
}

void MeshProcessor::Reset()
{
    m_dccVertexCount = 0;
    m_triAllocator.ClearAllTriangles();
    m_rawTriangles.clear();
    m_subsets.clear();

    m_indexData.clear();
    m_vertexData.clear();
}

bool MeshProcessor::GenerateMeshlets(
    FbxNode* node,
    const FbxTransformer& transformer,
    uint32_t meshletMaxVerts,
    uint32_t meshletMaxPrims,
    bool flipTriangles,
    bool force32BitIndices,
    MeshletSet& meshlet)
{
    if (!Extract(node))
    {
        return false;
    }

    Optimize(transformer, force32BitIndices);
    if (m_indexBuffer.GetIndexSize() == 4)
    {
        Meshletize<uint32_t>(meshletMaxVerts, meshletMaxPrims, meshlet);
    }
    else
    {
        Meshletize<uint16_t>(meshletMaxVerts, meshletMaxPrims, meshlet);
    }

    Reset();

    return true;
}

bool MeshProcessor::Extract(FbxNode* node)
{
    if (!node || !node->GetMesh())
        return false;

    FbxMesh* mesh = node->GetMesh();

    int layerCount = mesh->GetLayerCount();
    if (!layerCount || !mesh->GetLayer(0)->GetNormals())
    {
        mesh->InitNormals();
#if (FBXSDK_VERSION_MAJOR >= 2015)
        mesh->GenerateNormals();
#else
        mesh->ComputeVertexNormals();
#endif
    }

    // Find the layer with mesh materials.
    // Note: If multiple material mesh layers exist it chooses the last one found.
    FbxLayerElementMaterial* materialSet = nullptr;
    for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
        if (mesh->GetLayer(layerIndex)->GetMaterials())
        {
            materialSet = mesh->GetLayer(layerIndex)->GetMaterials();
        }
    }

    // Compute total transformation
    FbxAMatrix vertMatrix;
    FbxAMatrix normMatrix;

    {
        auto trans = node->GetGeometricTranslation(FbxNode::eSourcePivot);
        auto rot = node->GetGeometricRotation(FbxNode::eSourcePivot);
        auto scale = node->GetGeometricScaling(FbxNode::eSourcePivot);

        FbxAMatrix geom;
        geom.SetT(trans);
        geom.SetR(rot);
        geom.SetS(scale);

        auto global = node->EvaluateGlobalTransform();
        vertMatrix = global * geom;

        // Calculate the normal transform matrix (inverse-transpose)
        normMatrix = vertMatrix;
        normMatrix = normMatrix.Inverse();
        normMatrix = normMatrix.Transpose();
    }
    
    // Loop over polygons.
    for (int polyIndex = 0; polyIndex < mesh->GetPolygonCount(); ++polyIndex)
    {
        // Triangulate each polygon into one or more triangles.
        int polySize = mesh->GetPolygonSize(polyIndex);
        if (polySize != 3)
        {
            std::cout << "Mesh must first be triangulated to meshletize without modifying vertex data. Use the command line switch '-t' if desired." << std::endl;
            return false;
        }

        int materialIndex = 0;
        if (materialSet)
        {
            if (materialSet->GetMappingMode() == FbxLayerElement::eByPolygon)
            {
                switch (materialSet->GetReferenceMode())
                {
                case FbxLayerElement::eDirect:
                    materialIndex = polyIndex;
                    break;
                case FbxLayerElement::eIndex:
                case FbxLayerElement::eIndexToDirect:
                    materialIndex = materialSet->GetIndexArray().GetAt(polyIndex);
                    break;
                default:
                    break;
                }
            }
        }

        int cornerIndices[3] =
        {
            mesh->GetPolygonVertex(polyIndex, 0),
            mesh->GetPolygonVertex(polyIndex, 1),
            mesh->GetPolygonVertex(polyIndex, 2)
        };

        FbxVector4 vNormals[3];
        std::memset(vNormals, 0, 3 * sizeof(FbxVector4));

        mesh->GetPolygonVertexNormal(polyIndex, 0, vNormals[0]);
        mesh->GetPolygonVertexNormal(polyIndex, 1, vNormals[1]);
        mesh->GetPolygonVertexNormal(polyIndex, 2, vNormals[2]);

        // Build the raw triangle.
        auto tri = m_triAllocator.GetNewTriangle();

        // Store polygon index
        tri->PolygonIndex = static_cast<int>(polyIndex);

        // Store material subset index
        tri->SubsetIndex = materialIndex;

        for (uint32_t cornerIndex = 0; cornerIndex < 3; ++cornerIndex)
        {
            const uint32_t& dccIndex = cornerIndices[cornerIndex];

            // Store DCC vertex index (this helps the mesh reduction/VB generation code)
            tri->Vertex[cornerIndex].DCCVertexIndex = dccIndex;

            // Store vertex position
            auto vertPositions = mesh->GetControlPoints();
            auto finalPos = vertMatrix.MultT(vertPositions[dccIndex]);

            tri->Vertex[cornerIndex].Position.x = (float)finalPos.mData[0];
            tri->Vertex[cornerIndex].Position.y = (float)finalPos.mData[1];
            tri->Vertex[cornerIndex].Position.z = (float)finalPos.mData[2];

            // Store vertex normal
            auto finalNorm = vNormals[cornerIndex];
            finalNorm.mData[3] = 0.0;
            finalNorm = normMatrix.MultT(finalNorm);
            finalNorm.Normalize();

            tri->Vertex[cornerIndex].Normal.x = (float)finalNorm.mData[0];
            tri->Vertex[cornerIndex].Normal.y = (float)finalNorm.mData[1];
            tri->Vertex[cornerIndex].Normal.z = (float)finalNorm.mData[2];
        }

        // Add raw triangle to the mesh.
        m_rawTriangles.push_back(tri);

        m_dccVertexCount = max(m_dccVertexCount, tri->Vertex[0].DCCVertexIndex + 1);
        m_dccVertexCount = max(m_dccVertexCount, tri->Vertex[1].DCCVertexIndex + 1);
        m_dccVertexCount = max(m_dccVertexCount, tri->Vertex[2].DCCVertexIndex + 1);
    }

    return true;
}

void MeshProcessor::Optimize(const FbxTransformer& transformer, bool force32BitIndices)
{
    if (m_rawTriangles.empty())
        return;

    // Apply a AttributeSort optimization
    std::stable_sort(m_rawTriangles.begin(), m_rawTriangles.end(), [](Triangle* a, Triangle* b) { return a->SubsetIndex < b->SubsetIndex; });

    int currentSubsetIndex = -1;

    m_indexData.reserve(m_rawTriangles.size() * 3);
    m_vertexData.resize(m_dccVertexCount, nullptr);

    // loop through raw triangles
    for (size_t triIndex = 0; triIndex < m_rawTriangles.size(); ++triIndex)
    {
        Triangle* tri = m_rawTriangles[triIndex];

        // Create a new subset if one is encountered.
        // Note: Subset index will be monotonically increasing.
        if (tri->SubsetIndex > currentSubsetIndex)
        {
            m_subsets.emplace_back();
            m_subsets.back().first = m_indexData.size() / 3;
            m_subsets.back().second = 0;

            currentSubsetIndex = tri->SubsetIndex;
        }

        // Collapse the triangle verts into the final vertex list.
        // This removes unnecessary duplicates, and retains necessary duplicates.
        uint32_t indexA = FindOrAddVertex(m_vertexData, &tri->Vertex[0]);
        uint32_t indexB = FindOrAddVertex(m_vertexData, &tri->Vertex[1]);
        uint32_t indexC = FindOrAddVertex(m_vertexData, &tri->Vertex[2]);

        // record final indices into the index list
        m_indexData.push_back(indexA);
        if (true)
        {
            m_indexData.push_back(indexC);
            m_indexData.push_back(indexB);
        }
        else
        {
            m_indexData.push_back(indexB);
            m_indexData.push_back(indexC);
        }

        m_subsets.back().second++;
    }


    // Create real index buffer from index list
    bool use32BitIndex = (m_vertexData.size() > 65535) || force32BitIndices;

    m_indexBuffer.SetIndexCount((uint32_t)m_indexData.size());
    m_indexBuffer.SetIndexSize(use32BitIndex ? 4 : 2);
    m_indexBuffer.Allocate();

    for (size_t i = 0; i < m_indexData.size(); i++)
    {
        m_indexBuffer.SetIndex(i, m_indexData[i]);
    }


    // Create vertex buffer and allocate storage.
    m_vertexBuffer.SetVertexCount((uint32_t)m_vertexData.size());
    m_vertexBuffer.SetVertexSize(24); // Position & Normal - (XMFLOAT3 + XMFLOAT3) = 24 bytes
    m_vertexBuffer.Allocate();

    // Copy raw vertex data into the packed vertex buffer.
    for (size_t vertIndex = 0; vertIndex < m_vertexData.size(); ++vertIndex)
    {
        auto destVertex = m_vertexBuffer.GetVertex(vertIndex);

        auto srcVertex = m_vertexData[vertIndex];
        if (!srcVertex)
        {
            continue;
        }

        auto dest = reinterpret_cast<XMFLOAT3*>(destVertex);
        transformer.TransformPosition(dest, &srcVertex->Position);

        ++dest;
        transformer.TransformDirection(dest, &srcVertex->Normal);
    }
}

template <typename T>
void MeshProcessor::Meshletize(uint32_t meshletMaxVerts, uint32_t meshletMaxPrims, MeshletSet& m)
{
    m.maxVerts = meshletMaxVerts;
    m.maxPrims = meshletMaxPrims;
    m.indexSize = sizeof(T);

    // Create a position-only buffer
    std::vector<XMFLOAT3> positions;
    positions.resize(m_vertexBuffer.GetVertexCount());

    for (size_t i = 0; i < positions.size(); ++i)
    {
        positions[i] = *reinterpret_cast<XMFLOAT3*>(m_vertexBuffer.GetVertex(i));
    }

    std::vector<std::pair<size_t, size_t>> meshletSubsets;
    meshletSubsets.resize(m_subsets.size());

    // Meshletize our mesh and generate per-meshlet culling data
    ThrowIfFailed(ComputeMeshlets(
        reinterpret_cast<T*>(m_indexBuffer.GetIndexData()), m_indexBuffer.GetIndexCount() / 3,
        positions.data(), positions.size(),
        m_subsets.data(), m_subsets.size(),
        nullptr,
        m.meshlets,
        m.uniqueVertexIndices,
        m.primitiveIndices,
        meshletSubsets.data(),
        meshletMaxVerts,
        meshletMaxPrims
    ));

    m.cullData.resize(m.meshlets.size());

    ThrowIfFailed(ComputeCullData(
        positions.data(), positions.size(),
        m.meshlets.data(), m.meshlets.size(),
        reinterpret_cast<T*>(m.uniqueVertexIndices.data()), m.uniqueVertexIndices.size(),
        m.primitiveIndices.data(), m.primitiveIndices.size(),
        m.cullData.data(),
        MESHLET_DEFAULT
    ));

    m.subsets.resize(meshletSubsets.size());
    for (size_t i = 0; i < meshletSubsets.size(); ++i)
    {
        m.subsets[i].Offset = static_cast<uint32_t>(meshletSubsets[i].first);
        m.subsets[i].Count = static_cast<uint32_t>(meshletSubsets[i].second);
    }
}
