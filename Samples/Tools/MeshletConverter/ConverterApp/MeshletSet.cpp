#include "MeshletSet.h"

#include <fstream>

using namespace ATG;

namespace
{
    struct MeshletFileHeader
    {
        enum
        {
            MESHLET_VERSION_INITIAL = 0x0,
            MESHLET_VERSION_CULLDATA = 0x1,
            MESHLET_VERSION_CULLDATA_UPDATE = 0x2,
            MESHLET_VERSION_GEN_UPDATE = 0x3,
            MESHLET_VERSION_CURRENT = MESHLET_VERSION_GEN_UPDATE
        };

        uint32_t Prolog;
        uint32_t Version;
        uint32_t Count;
    };
}

void MeshletSet::Write(std::ostream& stream) const
{
    stream.write(reinterpret_cast<const char*>(&maxVerts), sizeof(maxVerts));
    stream.write(reinterpret_cast<const char*>(&maxPrims), sizeof(maxPrims));

    {
        uint32_t meshletCount = (uint32_t)meshlets.size();

        stream.write(reinterpret_cast<const char*>(&meshletCount), 4);
        stream.write(reinterpret_cast<const char*>(meshlets.data()), meshletCount * sizeof(meshlets[0]));
        stream.write(reinterpret_cast<const char*>(cullData.data()), meshletCount * sizeof(cullData[0]));
    }

    {
        uint32_t submeshCount = (uint32_t)subsets.size();

        stream.write(reinterpret_cast<const char*>(&submeshCount), 4);
        stream.write(reinterpret_cast<const char*>(subsets.data()), submeshCount * sizeof(subsets[0]));
    }

    {
        uint32_t indexBytes = indexSize;
        uint32_t indexCount = (uint32_t)uniqueVertexIndices.size() / indexBytes;

        stream.write(reinterpret_cast<const char*>(&indexBytes), 4);
        stream.write(reinterpret_cast<const char*>(&indexCount), 4);
        stream.write(reinterpret_cast<const char*>(uniqueVertexIndices.data()), indexCount * indexBytes);
    }

    {
        uint32_t primCount = (uint32_t)primitiveIndices.size();

        stream.write(reinterpret_cast<const char*>(&primCount), 4);
        stream.write(reinterpret_cast<const char*>(primitiveIndices.data()), primCount * sizeof(primitiveIndices[0]));
    }
}

bool MeshletSet::Write(const wchar_t* filePath, const std::vector<MeshletSet>& meshlets)
{
    auto file = std::ofstream(filePath, std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    MeshletFileHeader header;
    header.Prolog = 'MSHL';
    header.Version = MeshletFileHeader::MESHLET_VERSION_CURRENT;
    header.Count = static_cast<uint32_t>(meshlets.size());

    file.write(reinterpret_cast<char*>(&header), sizeof(header));

    for (auto& m : meshlets)
    {
        m.Write(file);
    }

    return true;
}

bool MeshletSet::Write(const char* filePath, const std::vector<MeshletSet>& meshlets)
{
    wchar_t widePath[MAX_PATH] = {};
    size_t size;
    mbstowcs_s(&size, widePath, filePath, strlen(filePath));

    return Write(widePath, meshlets);
}
