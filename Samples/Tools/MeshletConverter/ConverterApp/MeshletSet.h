#pragma once

#include <DirectXMesh.h>
#include <inttypes.h>
#include <vector>

namespace ATG
{
    struct Subset
    {
        uint32_t Count;
        uint32_t Offset;
    };

    struct MeshletSet
    {
        uint32_t maxVerts;
        uint32_t maxPrims;
        uint32_t indexSize;

        std::vector<Subset>                    subsets;
        std::vector<DirectX::Meshlet>          meshlets;
        std::vector<uint8_t>                   uniqueVertexIndices;
        std::vector<DirectX::MeshletTriangle>  primitiveIndices;
        std::vector<DirectX::CullData>         cullData;

        void Write(std::ostream& stream) const;
        static bool Write(const wchar_t* filePath, const std::vector<MeshletSet>& meshlets);
        static bool Write(const char* filePath, const std::vector<MeshletSet>& meshlets);
    };
}
