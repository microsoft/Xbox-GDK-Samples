//--------------------------------------------------------------------------------------
// Importer.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "Importer.h"

#include "MeshProcessor.h"
#include "FbxTransformer.h"
#include "SDKMesh.h"

#include <cassert>
#include <fbxsdk.h>
#include <iostream>
#include <queue>

using namespace ATG;
using namespace DirectX;

namespace
{
    HRESULT ReadEntireFile(
        _In_z_ wchar_t const* fileName,
        _Inout_ std::unique_ptr<uint8_t[]>& data,
        _Out_ size_t* dataSize)
    {
        if (!fileName || !dataSize)
            return E_INVALIDARG;

        *dataSize = 0;

        // Open the file.
        HANDLE hFile = CreateFile2(
            fileName,
            GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING,
            nullptr);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            if (hFile)
            {
                CloseHandle(hFile);
            }
            return HRESULT_FROM_WIN32(GetLastError());
        }

        // Get the file size.
        FILE_STANDARD_INFO fileInfo;
        if (!GetFileInformationByHandleEx(hFile, FileStandardInfo, &fileInfo, sizeof(fileInfo)))
        {
            if (hFile)
            {
                CloseHandle(hFile);
            }
            return HRESULT_FROM_WIN32(GetLastError());
        }

        // File is too big for 32-bit allocation, so reject read.
        if (fileInfo.EndOfFile.HighPart > 0)
        {
            if (hFile)
            {
                CloseHandle(hFile);
            }
            return E_FAIL;
        }

        // Create enough space for the file data.
        data.reset(new uint8_t[fileInfo.EndOfFile.LowPart]);

        if (!data)
        {
            if (hFile)
            {
                CloseHandle(hFile);
            }
            return E_OUTOFMEMORY;
        }

        // Read the data in.
        DWORD bytesRead = 0;

        if (!ReadFile(hFile, data.get(), fileInfo.EndOfFile.LowPart, &bytesRead, nullptr))
        {
            if (hFile)
            {
                CloseHandle(hFile);
            }
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (bytesRead < fileInfo.EndOfFile.LowPart)
        {
            if (hFile)
            {
                CloseHandle(hFile);
            }
            return E_FAIL;
        }

        *dataSize = bytesRead;

        if (hFile)
        {
            CloseHandle(hFile);
        }

        return S_OK;
    }
}

bool ATG::ImportFile(const char* filename, const ImportOptions& options, std::vector<MeshletSet>& meshlets)
{
    if (!filename)
        return false;

    FbxManager* manager   = FbxManager::Create();
    FbxScene* scene       = FbxScene::Create(manager, "");
    FbxImporter* importer = FbxImporter::Create(manager, "");

    if (!importer->Initialize(filename, -1, manager->GetIOSettings()))
    {
        FbxString error = importer->GetStatus().GetErrorString();
        std::cout << "Error during initialization of file \"" << filename << "\"" << std::endl;
        std::cout << "\tFbx Error: " << error.Buffer() << std::endl;

        return false;
    }

    if (!importer->Import(scene))
    {
        FbxString error = importer->GetStatus().GetErrorString();
        std::cout << "Error during import of file \"" << filename << "\"" << std::endl;
        std::cout << "\tFbx Error: " << error.Buffer() << std::endl;

        return false;
    }

    if (!scene->GetRootNode())
    {
        return false;
    }

    std::cout << "Processing file \"" << filename << "\"" << std::endl;

    if (options.TriangulateMeshes)
    {
        FbxGeometryConverter converter(manager);
        converter.Triangulate(scene, true);
    }

    // Accumulate visible mesh nodes of the Fbx scene
    std::queue<FbxNode*> queue;
    queue.push(scene->GetRootNode());

    std::vector<FbxNode*> meshNodes;
    while (!queue.empty())
    {
        FbxNode* node = queue.front();
        queue.pop();

        // Add mesh nodes to our list
        if (node->GetVisibility() && node->GetMesh())
        {
            meshNodes.push_back(node);
        }

        // Add children to traversal queue
        for (int i = 0; i < node->GetChildCount(); ++i)
        {
            queue.push(node->GetChild(i));
        }
    }


    std::cout << "Found " << meshNodes.size() << " mesh nodes." << std::endl;

    // Extract and meshletize the mesh nodes
    meshlets.clear();


    FbxTransformer transformer(options.UnitScale, options.FlipZ);
    transformer.Initialize(scene);

    MeshProcessor processor;
    for (auto& node : meshNodes)
    {
        MeshletSet result;
        if (processor.GenerateMeshlets(
            node,
            transformer,
            options.MeshletMaxVerts,
            options.MeshletMaxPrims,
            options.FlipTriangles,
            options.Force32BitIndices,
            result))
        {
            meshlets.emplace_back(std::move(result));
        }
        else
        {
            std::cout << "Failed to process mesh node with name \"" << node->GetNameOnly().Buffer() << "\"" << std::endl;
        }
    }

    // Cleanup
    importer->Destroy();
    scene->Destroy();
    manager->Destroy();

    return !meshlets.empty();
}

bool ATG::ImportFileSDKMesh(const char* filename, const ImportOptions& options, std::vector<MeshletSet>& meshlets)
{
    if (!filename)
        return false;

    size_t dataSize = 0;
    std::unique_ptr<uint8_t[]> data;
    {
        size_t newsize = strlen(filename) + 1;
        auto wfilename = std::make_unique<wchar_t[]>(newsize);
        size_t convertedChars = 0;
        mbstowcs_s(&convertedChars, wfilename.get(), newsize, filename, _TRUNCATE);
        HRESULT hr = ReadEntireFile(wfilename.get(), data, &dataSize);

        if (FAILED(hr))
        {
            std::cout << "ERROR: failed (%08X) loading " << filename << std::endl;
            return false;
        }
    }

    std::cout << "Processing file \"" << filename << "\"" << std::endl;

    // Extract and meshletize the mesh nodes
    meshlets.clear();

    const uint8_t* meshData = data.get();
    if (!meshData)
    {
        std::cout << "meshData cannot be null" << std::endl;
        return false;
    }

    // File Headers
    if (dataSize < sizeof(DXUT::SDKMESH_HEADER))
    {
        std::cout << "Unexpected end-of-file" << std::endl;
        return false;
    }

    auto header = reinterpret_cast<const DXUT::SDKMESH_HEADER*>(meshData);

    const size_t headerSize = sizeof(DXUT::SDKMESH_HEADER)
        + header->NumVertexBuffers * sizeof(DXUT::SDKMESH_VERTEX_BUFFER_HEADER)
        + header->NumIndexBuffers * sizeof(DXUT::SDKMESH_INDEX_BUFFER_HEADER);
    if (header->HeaderSize != headerSize)
    {
        std::cout << "Not a valid SDKMESH file" << std::endl;
        return false;
    }

    if (dataSize < header->HeaderSize)
    {
        std::cout << "Unexpected end-of-file" << std::endl;
        return false;
    }

    if (header->Version != DXUT::SDKMESH_FILE_VERSION && header->Version != DXUT::SDKMESH_FILE_VERSION_V2)
    {
        std::cout << "Not a supported SDKMESH version" << std::endl;
        return false;
    }

    if (header->IsBigEndian)
    {
        std::cout << "Loading BigEndian SDKMESH files not supported" << std::endl;
        return false;
    }

    if (!header->NumMeshes)
    {
        std::cout << "No meshes found" << std::endl;
        return false;
    }

    if (!header->NumVertexBuffers)
    {
        std::cout << "No vertex buffers found" << std::endl;
        return false;
    }

    if (!header->NumIndexBuffers)
    {
        std::cout << "No index buffers found" << std::endl;
        return false;
    }

    if (!header->NumTotalSubsets)
    {
        std::cout << "No subsets found" << std::endl;
        return false;
    }

    if (!header->NumMaterials)
    {
        std::cout << "No materials found" << std::endl;
        return false;
    }

    // Sub-headers
    if (dataSize < header->VertexStreamHeadersOffset
        || (dataSize < (header->VertexStreamHeadersOffset + uint64_t(header->NumVertexBuffers) * sizeof(DXUT::SDKMESH_VERTEX_BUFFER_HEADER))))
    {
        std::cout << "Unexpected end-of-file" << std::endl;
        return false;
    }
    auto vbArray = reinterpret_cast<const DXUT::SDKMESH_VERTEX_BUFFER_HEADER*>(meshData + header->VertexStreamHeadersOffset);

    if (dataSize < header->IndexStreamHeadersOffset
        || (dataSize < (header->IndexStreamHeadersOffset + uint64_t(header->NumIndexBuffers) * sizeof(DXUT::SDKMESH_INDEX_BUFFER_HEADER))))
    {
        std::cout << "Unexpected end-of-file" << std::endl;
        return false;
    }
    auto ibArray = reinterpret_cast<const DXUT::SDKMESH_INDEX_BUFFER_HEADER*>(meshData + header->IndexStreamHeadersOffset);

    if (dataSize < header->MeshDataOffset
        || (dataSize < (header->MeshDataOffset + uint64_t(header->NumMeshes) * sizeof(DXUT::SDKMESH_MESH))))
    {
        std::cout << "Unexpected end-of-file" << std::endl;
        return false;
    }
    auto meshArray = reinterpret_cast<const DXUT::SDKMESH_MESH*>(meshData + header->MeshDataOffset);

    if (dataSize < header->SubsetDataOffset
        || (dataSize < (header->SubsetDataOffset + uint64_t(header->NumTotalSubsets) * sizeof(DXUT::SDKMESH_SUBSET))))
    {
        std::cout << "Unexpected end-of-file" << std::endl;
        return false;
    }
    auto subsetArray = reinterpret_cast<const DXUT::SDKMESH_SUBSET*>(meshData + header->SubsetDataOffset);


    // Buffer data
    const uint64_t bufferDataOffset = header->HeaderSize + header->NonBufferDataSize;
    if ((dataSize < bufferDataOffset)
        || (dataSize < bufferDataOffset + header->BufferDataSize))
    {
        std::cout << "Unexpected end-of-file" << std::endl;
        return false;
    }
    const uint8_t* bufferData = meshData + bufferDataOffset;

    // Validate vertex buffers
    for (size_t j = 0; j < header->NumVertexBuffers; ++j)
    {
        auto& vh = vbArray[j];

        if (vh.SizeBytes > UINT32_MAX)
        {
            std::cout << "VB too large" << std::endl;
            return false;
        }

        if (dataSize < vh.DataOffset
            || (dataSize < vh.DataOffset + vh.SizeBytes))
        {
            std::cout << "Unexpected end-of-file" << std::endl;
            return false;
        }
    }

    // Validate index buffers
    for (size_t j = 0; j < header->NumIndexBuffers; ++j)
    {
        auto& ih = ibArray[j];

        if (ih.SizeBytes > UINT32_MAX)
        {
            std::cout << "IB too large" << std::endl;
            return false;
        }

        if (dataSize < ih.DataOffset
            || (dataSize < ih.DataOffset + ih.SizeBytes))
        {
            std::cout << "Unexpected end-of-file" << std::endl;
            return false;
        }

        if (ih.IndexType != DXUT::IT_16BIT && ih.IndexType != DXUT::IT_32BIT)
        {
            std::cout << "Invalid index buffer type found" << std::endl;
            return false;
        }
    }

    // Validate meshes
    for (size_t meshIndex = 0; meshIndex < header->NumMeshes; ++meshIndex)
    {
        auto& mh = meshArray[meshIndex];

        if (!mh.NumSubsets
            || !mh.NumVertexBuffers
            || mh.IndexBuffer >= header->NumIndexBuffers
            || mh.VertexBuffers[0] >= header->NumVertexBuffers)
        {
            std::cout << "Invalid mesh found" << std::endl;
            return false;
        }

        if (dataSize < mh.SubsetOffset
            || (dataSize < mh.SubsetOffset + uint64_t(mh.NumSubsets) * sizeof(uint32_t)))
        {
            std::cout << "Unexpected end-of-file" << std::endl;
            return false;
        }

        auto subsets = reinterpret_cast<const uint32_t*>(meshData + mh.SubsetOffset);
        for (size_t j = 0; j < mh.NumSubsets; ++j)
        {
            auto const sIndex = subsets[j];
            if (sIndex >= header->NumTotalSubsets)
            {
                std::cout << "Invalid mesh found" << std::endl;
                return false;
            }

            auto& subset = subsetArray[sIndex];
            if (subset.PrimitiveType != DXUT::PT_TRIANGLE_LIST)
            {
                std::cout << "Only triangle lists are supported." << std::endl;
                return false;
            }
        }
    }

    // Generate meshlets
    for (size_t meshIndex = 0; meshIndex < header->NumMeshes; ++meshIndex)
    {

        auto& mh = meshArray[meshIndex];
        auto subsets = reinterpret_cast<const uint32_t*>(meshData + mh.SubsetOffset);

        // Vertex data
        const auto& vh = vbArray[mh.VertexBuffers[0]];
        const uint8_t* verts = bufferData + (vh.DataOffset - bufferDataOffset);

        // Create subsets
        std::vector<std::pair<size_t, size_t>> meshSubsets(mh.NumSubsets);
        for (size_t j = 0; j < mh.NumSubsets; ++j)
        {
            auto& subset = subsetArray[subsets[j]];
            meshSubsets[j] = std::pair<size_t, size_t>(subset.IndexStart / 3, subset.IndexCount / 3);
        }

        // Index data
        const auto& ih = ibArray[mh.IndexBuffer];
        auto indices = bufferData + (ih.DataOffset - bufferDataOffset);

        MeshletSet result;
        MeshProcessor::GenerateMeshlets(
            result,
            options.MeshletMaxVerts,
            options.MeshletMaxPrims,
            verts,
            vh.NumVertices,
            vh.StrideBytes,
            indices,
            ih.NumIndices / 3,
            ibArray[mh.IndexBuffer].IndexType == DXUT::IT_32BIT,
            meshSubsets);

        meshlets.emplace_back(std::move(result));
    }


    return !meshlets.empty();
}
