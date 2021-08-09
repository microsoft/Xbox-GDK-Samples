//--------------------------------------------------------------------------------------
// Importer.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "Importer.h"

#include "MeshProcessor.h"
#include "FbxTransformer.h"

#include <cassert>
#include <fbxsdk.h>
#include <iostream>
#include <queue>

using namespace ATG;
using namespace DirectX;

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
