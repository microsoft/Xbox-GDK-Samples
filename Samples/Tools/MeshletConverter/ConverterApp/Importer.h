//--------------------------------------------------------------------------------------
// Importer.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "MeshletSet.h"
#include <vector>

namespace ATG
{
    struct ImportOptions
    {
        uint32_t    MeshletMaxVerts;
        uint32_t    MeshletMaxPrims;
        float       UnitScale;
        bool        FlipZ;
        bool        FlipTriangles;
        bool        Force32BitIndices;
        bool        TriangulateMeshes;

        ImportOptions(void)
            : MeshletMaxVerts(128)
            , MeshletMaxPrims(128)
            , UnitScale(1.0f)
            , FlipZ(false)
            , FlipTriangles(false)
            , Force32BitIndices(false)
            , TriangulateMeshes(false)
        { }
    };

    // Imports an FBX or OBJ file. Returns whether any meshes were processed.
    bool ImportFile(const char* filename, const ImportOptions& options, std::vector<MeshletSet>& meshlets);
    bool ImportFileSDKMesh(const char* filename, const ImportOptions& options, std::vector<MeshletSet>& meshlets);
}
