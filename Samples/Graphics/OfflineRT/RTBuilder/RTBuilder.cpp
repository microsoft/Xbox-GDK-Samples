//--------------------------------------------------------------------------------------
// RTBuilder.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include <OfflineRT/FileFormat.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-user-defined-literal"
#endif

#include <External/bvhtoy/bvhtoy.h>
#include <External/bvhtoy/bvhtoyxbox.h>
#include <fast_obj/fast_obj.h>
#include <meshoptimizer/meshoptimizer.h>

#ifdef __clang__
#pragma clang diagnostic push
#endif

using namespace Microsoft::WRL;
using namespace std::chrono;

#if (_GXDK_VER >= 0x63360C4B)   /* GDK Edition March 2024 */
#define DEHYDRATED_BVH_SUPPORTED
#endif

namespace
{
    uint32_t bvhtoyRun(D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *outInternalNodes, uint32_t *inoutInternalNodeCount,
                        D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *outLeafNodes, uint32_t *inoutLeafNodeCount,
                        const uint32_t *indices, uint32_t indexCount,
                        const float *vertices, uint32_t vertexCount, uint32_t vertexSize,
                        uint32_t useTriangleLeafNodesOnly)
    {
        struct PrintHelper
        {
            static void print(void* userdata, const char* format, va_list args)
            {
                vprintf(format, args);
            }
        };

        struct AllocHelper
        {
            void* head;

            struct AllocEntry
            {
                void* next;
                void* base;
                unsigned long long size;
            };

            void* allocate(unsigned long long size)
            {
                void* base = malloc(size + sizeof(AllocEntry));
                AllocEntry* entry = (AllocEntry*)base;
                if (!entry)
                    return NULL;
                entry->next = head;
                entry->base = base;
                entry->size = size;
                head = base;
                return reinterpret_cast<void*>(entry + 1);
            }

            void deallocate(void* memory)
            {
                AllocEntry* entry = reinterpret_cast<AllocEntry*>(memory) - 1;
                assert(entry->base == head);
                head = entry->next;
                free(entry->base);
            }

            static void* allocate(void* context, unsigned long long size)
            {
                AllocHelper* helper = reinterpret_cast<AllocHelper*>(context);
                return helper->allocate(size);
            }

            static void deallocate(void* context, void* memory)
            {
                AllocHelper* helper = reinterpret_cast<AllocHelper*>(context);
                helper->deallocate(memory);
            }
        };

        AllocHelper helper = { NULL };
        bvhtoySetPrintCallback(PrintHelper::print, NULL);
        bvhtoySetAllocatorCallbacks(AllocHelper::allocate, AllocHelper::deallocate, &helper);

        return bvhtoyXboxRunDefault(outInternalNodes, inoutInternalNodeCount,
                                    outLeafNodes, inoutLeafNodeCount,
                                    indices, indexCount,
                                    vertices, vertexCount, vertexSize,
                                    useTriangleLeafNodesOnly);
    }


    // Simple helper class for allocating and maintaining lifetime of an aligned data buffer
    class DataBuffer
    {
    public:
        DataBuffer(size_t size, size_t alignment)
        {
            m_data = _aligned_malloc(size, alignment);
        }

        ~DataBuffer()
        {
            Reset();
        }

        // Non-copyable
        DataBuffer(const DataBuffer&) = delete;
        DataBuffer& operator=(const DataBuffer&) = delete;

        // Movable
        DataBuffer(DataBuffer&& other) noexcept(false)
        {
            m_data = other.m_data;
            other.m_data = nullptr;
        }

        DataBuffer& operator=(DataBuffer&& other) noexcept(false)
        {
            if (this != &other)
            {
                Reset();
                m_data = other.m_data;
                other.m_data = nullptr;
            }

            return *this;
        }

        void* Get() const { return m_data; }

        D3D12_GPU_VIRTUAL_ADDRESS GetAsGPUAddress() const { return reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(Get()); }

        void Reset()
        {
            if (m_data)
            {
                _aligned_free(m_data);
                m_data = nullptr;
            }
        }

    private:
        void* m_data;
    };

    float SerializeBvhToMemory(DataBuffer & dstBvhMemory,
                               size_t dstBvhMemorySizeInBytes,
                               DataBuffer & srcBvhMemory,
                               ID3D12GraphicsCommandList6 *cmdList,
                               unsigned bvhCopyMode,
                               const char *objFile)
    {
        memset((void*)dstBvhMemory.GetAsGPUAddress(), 0, dstBvhMemorySizeInBytes);

        auto serializationStartTime = std::chrono::high_resolution_clock::now();
        cmdList->CopyRaytracingAccelerationStructure(dstBvhMemory.GetAsGPUAddress(), srcBvhMemory.GetAsGPUAddress(), static_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE>(bvhCopyMode));
        auto serializationEndTime = std::chrono::high_resolution_clock::now();

        float serializationTime = std::chrono::duration<float, std::milli>(serializationEndTime - serializationStartTime).count();

        D3D12_SERIALIZED_RAYTRACING_ACCELERATION_STRUCTURE_HEADER* serializedHeader = (D3D12_SERIALIZED_RAYTRACING_ACCELERATION_STRUCTURE_HEADER*)dstBvhMemory.GetAsGPUAddress();

        printf("Finished serialization: time = %5.2f ms, size = %llu bytes, original uncompressed size = %llu bytes\n", serializationTime, dstBvhMemorySizeInBytes, serializedHeader->SerializedSizeInBytesIncludingHeader);
        printf("%s,%llu,%llu\n", objFile, dstBvhMemorySizeInBytes, serializedHeader->SerializedSizeInBytesIncludingHeader);

        return serializationTime;
    }

    void WriteBvhToMdatFile(const wchar_t *dstFile,
                                      DataBuffer & buffer,
                                      size_t serializedSize,
                                      size_t deserializedSize,
                                      const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO& prebuildInfo,
                                      const D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC & triangleDesc,
                                      size_t vertexWriteStrideInBytes,
                                      float blasBuildTime,
                                      float blasSerializationTime,
                                      bool compressedBvh)
    {
        

        std::ofstream outStream(dstFile, std::ios::out | std::ios::binary | std::ios::trunc);

        // Build file header
        printf("Writing serialized data to file: %ls\n", dstFile);

        OfflineRTFiles::ModelFileHeader fileHeader = {};

        fileHeader.Magic = MAKEFOURCC('M', 'D', 'A', 'T');
        fileHeader.HeaderSize = sizeof(fileHeader);

        fileHeader.IndexCount = triangleDesc.IndexCount;
        fileHeader.VertexCount = triangleDesc.VertexCount;

        // Override the scratch size for compressed builds as we had to use a large scratch buffer to hold the original BVH
        fileHeader.PrebuildScratchSize = compressedBvh?4096:prebuildInfo.ScratchDataSizeInBytes;
        fileHeader.PrebuildResultMaxSize = prebuildInfo.ResultDataMaxSizeInBytes;
        fileHeader.PostbuildCurrentSize = deserializedSize;
        fileHeader.PostbuildSerializedSize = serializedSize;
        fileHeader.BuildTime = blasBuildTime;
        fileHeader.SerializationTime = blasSerializationTime;

        // Write out data
        outStream.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
        outStream.write(reinterpret_cast<const char*>(buffer.Get()), serializedSize);
        outStream.write(reinterpret_cast<const char*>(triangleDesc.IndexBuffer), triangleDesc.IndexCount * sizeof(uint32_t));

        for (UINT v = 0; v < triangleDesc.VertexCount; ++v)
        {
            outStream.write(reinterpret_cast<const char*>(triangleDesc.VertexBuffer.StartAddress + v * triangleDesc.VertexBuffer.StrideInBytes), vertexWriteStrideInBytes);
        }

        printf("Done writing serialized data\n");

        outStream.close();
    }

    typedef struct vertex_t
    {
        float px, py, pz;
        float nx, ny, nz;
    } vertex_t;

    void LoadObjFile(std::vector<vertex_t> & outVertices, std::vector<uint32_t> & outIndices, const char *objFile, bool flipZ)
    {
        const float scaleFactor = 1.0f;
        auto objFileParseStart = std::chrono::high_resolution_clock::now();
        fastObjMesh* obj = fast_obj_read(objFile);
        auto objFileParseEndTime = std::chrono::high_resolution_clock::now();

        size_t idxCount = 0;

        for (unsigned int i = 0; i < obj->face_count; ++i)
            idxCount += 3 * (static_cast<size_t>(obj->face_vertices[i]) - 2);

        outVertices.resize(idxCount);

        size_t vtxOffset = 0;
        size_t idxOffset = 0;

        for (unsigned int i = 0; i < obj->face_count; ++i)
        {
            for (unsigned int j = 0; j < obj->face_vertices[i]; ++j)
            {
                fastObjIndex gi = obj->indices[idxOffset + j];

                vertex_t v =
                {
                    obj->positions[gi.p * 3] * scaleFactor,
                    obj->positions[gi.p * 3 + 1] * scaleFactor,
                    obj->positions[gi.p * 3 + 2] * scaleFactor,

                    obj->normals[gi.n * 3],
                    obj->normals[gi.n * 3 + 1],
                    obj->normals[gi.n * 3 + 2],
                };

                // triangulate polygon on the fly; offset-3 is always the first polygon vertex
                if (j >= 3)
                {
                    outVertices[vtxOffset + 0] = outVertices[vtxOffset - 3];
                    outVertices[vtxOffset + 1] = outVertices[vtxOffset - 1];
                    vtxOffset += 2;
                }

                outVertices[vtxOffset] = v;
                vtxOffset++;
            }

            idxOffset += obj->face_vertices[i];
        }
        auto objFileVertexFetchEndTime = std::chrono::high_resolution_clock::now();

        fast_obj_destroy(obj);
        auto objFileUnloadEndTime = std::chrono::high_resolution_clock::now();

        outIndices.resize(idxCount);
        size_t vtxCount = meshopt_generateVertexRemap(outIndices.data(), NULL, idxCount, outVertices.data(), outVertices.size(), sizeof(vertex_t));

        // some vertices were duduplicated and original vertex buffer is too large
        if (outVertices.size() != vtxCount)
        {
            std::vector<vertex_t> uniqueVertices(vtxCount);
            meshopt_remapVertexBuffer(uniqueVertices.data(), outVertices.data(), outVertices.size(), sizeof(vertex_t), outIndices.data());

            uniqueVertices.swap(outVertices);
        }
        auto objFileVertexRemapEndTime = std::chrono::high_resolution_clock::now();

        size_t i = 0, idxCountWithDegenerateTriangles = idxCount;
        for (; i < idxCount; i += 3)
        {
            unsigned int a = outIndices[i], b = outIndices[i + 1], c = outIndices[i + 2];

            if (a == b || a == c || b == c)
            {
                idxCount -= 3;
                outIndices[i + 0] = outIndices[idxCount + 0];
                outIndices[i + 1] = outIndices[idxCount + 1];
                outIndices[i + 2] = outIndices[idxCount + 2];
            }
        }
        outIndices.resize(idxCount);

        if (flipZ)
        {
            assert(idxCount % 3 == 0);
            for (size_t i = 0; i < idxCount; i += 3)
            {
                uint32_t it = outIndices[i];
                outIndices[i] = outIndices[i + 2];
                outIndices[i + 2] = it;
            }

            for (size_t i = 0; i < vtxCount; ++i)
            {
                auto& v = outVertices[i];
                v.pz = flipZ ? -v.pz : v.pz;
                v.nz = flipZ ? -v.nz : v.nz;
            }
        }
        auto objFileMirrorEndTime = std::chrono::high_resolution_clock::now();

        printf("Loaded: %llu indices (degenerate tris removed=%llu), %llu vertices.\n\tTotal Time: %.3f ms\n\tObjFile Parse(fast_obj): %.3f ms\n\tVertex Fetch: %.3f\n\tObjFile Unload: %.3f ms\n\tVertex Remap(meshoptimizer): %.3f ms\n\tMirror Time: %.3f ms\n",
            outIndices.size(), idxCountWithDegenerateTriangles - idxCount, outVertices.size(),
            std::chrono::duration<float, std::milli>(objFileMirrorEndTime - objFileParseStart).count(),
            std::chrono::duration<float, std::milli>(objFileParseEndTime - objFileParseStart).count(),
            std::chrono::duration<float, std::milli>(objFileVertexFetchEndTime - objFileParseEndTime).count(),
            std::chrono::duration<float, std::milli>(objFileUnloadEndTime - objFileVertexFetchEndTime).count(),
            std::chrono::duration<float, std::milli>(objFileVertexRemapEndTime - objFileUnloadEndTime).count(),
            std::chrono::duration<float, std::milli>(objFileMirrorEndTime - objFileVertexRemapEndTime).count());
    }

    // Function loads a Wavefront OBJ file and creates a Xbox-native bottom-
    // level acceleration structure (BLAS) for the model. The BVH is
    // serialized (along with index and vertex data for demo visualization)
    // into a binary file which can be deserialized by the XDXR runtime in the
    // Xbox Scarlett graphics driver.
    //
    // This functionality allows the memory-costly BLAS BVH-generation process
    // to be shifted from runtime to pipeline time. It also produces better
    // BVH data because the Intel Embree (https://www.embree.org/) builder can
    // be utilized by the PC version of the Scarlett UMD.
    template<bool UseCanonicalApiAndCustomBvhBuild, bool UseTriangleOnlyLeavesInCustomBvhBuild>
    void BuildModelData(const char* objFile, const wchar_t* dstFile, bool flipZ)
    {
        // Load model data
        printf("Loading model: %s - Building BVH\n", objFile);

        std::vector<vertex_t> vertices;
        std::vector<uint32_t> indices;
        LoadObjFile(vertices, indices, objFile, flipZ);

        // Create UMD12 device - this will fail if the dependent DLLs are not available
        // in the working directory or DLL search path. The project copies these during
        // a custom build step.
        printf("Creating offline Scarlett UMD device\n");
        ComPtr<ID3D12Device5> umdDevice;
        DX::ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_GRAPHICS_PPV_ARGS(umdDevice.ReleaseAndGetAddressOf())));

        // Create command allocator and command list. These are only needed to access the
        // raytracing API and don't actually launch commands on the GPU when used with
        // the PC version of the UMD12 device.
        ComPtr<ID3D12CommandAllocator> cmdAllocator;
        DX::ThrowIfFailed(umdDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(cmdAllocator.ReleaseAndGetAddressOf())));
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> cmdList;
        DX::ThrowIfFailed(umdDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(cmdList.ReleaseAndGetAddressOf())));

        // Bottom-level-acceleration (BLAS) description
        D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
        geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        geometryDesc.Triangles.IndexBuffer = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(indices.data());
        geometryDesc.Triangles.IndexCount = static_cast<UINT>(indices.size());
        geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
        geometryDesc.Triangles.Transform3x4 = 0;
        geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        geometryDesc.Triangles.VertexCount = static_cast<UINT>(vertices.size());
        geometryDesc.Triangles.VertexBuffer.StartAddress = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(vertices.data());
        geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(vertex_t);

        // This sample uses BUILD_FLAG_PREFER_FAST_TRACE to optimize for ray tracing performance
        // at the cost of build time and memory. You may wish to use BUILD_FLAG_MINIMIZE_MEMORY
        // if you are memory constrained.
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& buildInputs = buildDesc.Inputs;
        buildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        buildInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        buildInputs.NumDescs = 1;
        buildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        buildInputs.pGeometryDescs = &geometryDesc;

#if defined(DEHYDRATED_BVH_SUPPORTED)
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC dehydratedBuildDesc = buildDesc;
        dehydratedBuildDesc.Inputs.Flags |= D3D12XBOX_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_DEHYDRATE;
#endif

        // Determine prebuild information: maximum output and scratch buffer sizes
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
        umdDevice->GetRaytracingAccelerationStructurePrebuildInfo(&buildInputs, &prebuildInfo);
        DX::ThrowIfFalse(prebuildInfo.ResultDataMaxSizeInBytes > 0, "Illegal prebuild result size");

#if defined(DEHYDRATED_BVH_SUPPORTED)
        // Determine prebuild information: maximum output and scratch buffer sizes
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO dehydratedPrebuildInfo = {};
        umdDevice->GetRaytracingAccelerationStructurePrebuildInfo(&dehydratedBuildDesc.Inputs, &dehydratedPrebuildInfo);
        DX::ThrowIfFalse(dehydratedPrebuildInfo.ResultDataMaxSizeInBytes > 0, "Illegal prebuild result size");
#endif

        // Allocate build buffers
        DataBuffer scratchBuffer(prebuildInfo.ScratchDataSizeInBytes, 256);
        DataBuffer resultBuffer(prebuildInfo.ResultDataMaxSizeInBytes, 256);

        buildDesc.ScratchAccelerationStructureData = scratchBuffer.GetAsGPUAddress();
        buildDesc.DestAccelerationStructureData = resultBuffer.GetAsGPUAddress();

#if defined(DEHYDRATED_BVH_SUPPORTED)
        // Allocate dehydrated build buffers
        DataBuffer scratchBufferDehydrated(dehydratedPrebuildInfo.ScratchDataSizeInBytes, 256);
        DataBuffer resultBufferDehydrated(dehydratedPrebuildInfo.ResultDataMaxSizeInBytes, 256);

        dehydratedBuildDesc.ScratchAccelerationStructureData = scratchBufferDehydrated.GetAsGPUAddress();
        dehydratedBuildDesc.DestAccelerationStructureData = resultBufferDehydrated.GetAsGPUAddress();
#endif

        // Build acceleration structure and retrieve current and serialized sizes
        printf("Building bottom-level acceleration structure\n");

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION_DESC postbuildSerializationInfo = {};
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC postbuildCurrentSizeInfo = {};
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postbuildInfo[] =
        {
            {reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(&postbuildSerializationInfo), D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION},
            {reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(&postbuildCurrentSizeInfo), D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE}
        };

        steady_clock::time_point buildStartTime, buildEndTime;

#if defined(DEHYDRATED_BVH_SUPPORTED)
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC dehydratedPostbuildCurrentSizeInfo = {};
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC dehydratedPostbuildInfo[] =
        {
            {reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(&dehydratedPostbuildCurrentSizeInfo), D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE}
        };

        steady_clock::time_point dehydratedBuildStartTime, dehydratedBuildEndTime;
#endif

        if (UseCanonicalApiAndCustomBvhBuild == false)
        {
            buildStartTime = std::chrono::high_resolution_clock::now();
            cmdList->BuildRaytracingAccelerationStructure(&buildDesc, _ARRAYSIZE(postbuildInfo), postbuildInfo);
            buildEndTime = std::chrono::high_resolution_clock::now();

#if defined(DEHYDRATED_BVH_SUPPORTED)
            // Build it a second time to output the Dehydrated BVH supported in March 2024 GDK.
            dehydratedBuildStartTime = std::chrono::high_resolution_clock::now();
            cmdList->BuildRaytracingAccelerationStructure(&dehydratedBuildDesc, _ARRAYSIZE(dehydratedPostbuildInfo), dehydratedPostbuildInfo);
            dehydratedBuildEndTime = std::chrono::high_resolution_clock::now();
#endif
        }
        else
        {
            uint32_t triangleCount =  (uint32_t)indices.size() / 3;
            uint32_t leafNodeCount = triangleCount;
            uint32_t internalNodeCount = triangleCount - 1;

            D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *leafNodes = (D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *)malloc(sizeof(D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE) * leafNodeCount);
            D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *internalNodes = (D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE *)malloc(sizeof(D3D12XBOX_RAYTRACING_CANONICAL_ACCELERATION_STRUCTURE_NODE) * internalNodeCount);

            if(!leafNodes || !internalNodes)
            {
                printf("Failed to allocate memory for custom BVH build\n");
                return;
            }
            uint32_t rootIndex = bvhtoyRun(internalNodes, &internalNodeCount, leafNodes, &leafNodeCount, indices.data(), (uint32_t)indices.size(), (const float*)vertices.data(), (uint32_t)vertices.size(), sizeof(vertex_t), UseTriangleOnlyLeavesInCustomBvhBuild);

            buildStartTime = std::chrono::high_resolution_clock::now();
            cmdList->BuildRaytracingAccelerationStructureFromCanonicalBVH(
                &buildDesc,
                &internalNodes[rootIndex],
                internalNodes,
                internalNodeCount,
                leafNodes,
                leafNodeCount,
                1, &postbuildInfo[0] // NOTE: October GDK crashes if passing > 1 postbuild infos. Fixed in March 2024.
            );
            buildEndTime = std::chrono::high_resolution_clock::now();

#if defined(DEHYDRATED_BVH_SUPPORTED)
            dehydratedBuildStartTime = std::chrono::high_resolution_clock::now();

            cmdList->BuildRaytracingAccelerationStructureFromCanonicalBVH(
                &dehydratedBuildDesc,
                &internalNodes[rootIndex],
                internalNodes,
                internalNodeCount,
                leafNodes,
                leafNodeCount,
                1, &dehydratedPostbuildInfo[0] // NOTE: October GDK crashes if passing > 1 postbuild infos. Fixed in March 2024.
            );

            dehydratedBuildEndTime = std::chrono::high_resolution_clock::now();
#endif


            free(internalNodes);
            free(leafNodes);
        }

        float blasBuildTime = std::chrono::duration<float, std::milli>(buildEndTime - buildStartTime).count();
        printf("Finished build: time = %5.2f ms, size = %llu bytes\n", blasBuildTime, postbuildCurrentSizeInfo.CurrentSizeInBytes);

#if defined(DEHYDRATED_BVH_SUPPORTED)
        float dehydratedBLASBuildTime = std::chrono::duration<float, std::milli>(dehydratedBuildEndTime - dehydratedBuildStartTime).count();
        printf("Finished dehydrated build: time = %5.2f ms, size = %llu bytes\n", dehydratedBLASBuildTime, dehydratedPostbuildCurrentSizeInfo.CurrentSizeInBytes);
#endif

        // Release scratch buffer
        scratchBuffer.Reset();

        DataBuffer serializationBuffer(postbuildSerializationInfo.SerializedSizeInBytes, 256);

        static const wchar_t compressedName[] = L".compressed.mdat";
        static const wchar_t compressedGpuName[] = L".compressed.GPU.mdat";
        static const wchar_t dehydratedName[] = L".compressed.Dehydrated.mdat";

        wchar_t const *extensionStart = wcsstr(dstFile, L".mdat");
        size_t compressedFileNamePrefixSize = extensionStart ? (extensionStart - dstFile) : wcslen(dstFile);
        size_t compressedFileNameBufferSize = compressedFileNamePrefixSize + 256;

        wchar_t *compressedFileNameBuffer = (wchar_t *)malloc(compressedFileNameBufferSize * sizeof(wchar_t));
        if (!compressedFileNameBuffer)
        {
            printf("Error: Failed to allocate memory for compressed file name buffer\n");
            return;
        }
        wcsncpy_s(compressedFileNameBuffer, compressedFileNameBufferSize, dstFile, compressedFileNamePrefixSize);


        // Serialize acceleration structure
        printf("Serializing bottom-level acceleration structure (UnCompressed)\n");

        float blasSerializationTime = SerializeBvhToMemory(serializationBuffer, postbuildSerializationInfo.SerializedSizeInBytes, resultBuffer, cmdList.Get(), D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZE, objFile);

        D3D12_SERIALIZED_RAYTRACING_ACCELERATION_STRUCTURE_HEADER* serializedHeader = (D3D12_SERIALIZED_RAYTRACING_ACCELERATION_STRUCTURE_HEADER*)serializationBuffer.GetAsGPUAddress();
        WriteBvhToMdatFile(dstFile, serializationBuffer, serializedHeader->SerializedSizeInBytesIncludingHeader, serializedHeader->DeserializedSizeInBytes,
            prebuildInfo, geometryDesc.Triangles, sizeof(vertex_t), blasBuildTime, blasSerializationTime, false);

        // create dstFile-based name with ".compressed.mdat" postfix
        wcsncpy_s(compressedFileNameBuffer + compressedFileNamePrefixSize, _countof(compressedName), compressedName, _countof(compressedName));
#if (_GXDK_VER >= 0x55F00C6D) /* GDK Edition 220301 */
        printf("Serializing bottom-level acceleration structure (Compressed)\n");

        // BVH CPU Compression available in March 2022 Update 1 GDK onwards
        // Set the Copy Mode based on whether we want a Compressed BVH or not
        blasSerializationTime = SerializeBvhToMemory(serializationBuffer, postbuildSerializationInfo.SerializedSizeInBytes, resultBuffer, cmdList.Get(), D3D12XBOX_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZE_AND_COMPRESS, objFile);

        WriteBvhToMdatFile(compressedFileNameBuffer, serializationBuffer, serializedHeader->SerializedSizeInBytesIncludingHeader, serializedHeader->DeserializedSizeInBytes,
            prebuildInfo, geometryDesc.Triangles, sizeof(vertex_t), blasBuildTime, blasSerializationTime, true);
#else
        std::ofstream outCompressedStream(compressedFileNameBuffer, std::ios::out | std::ios::binary | std::ios::trunc);
        const char* emptyFileString = "Empty File - Created to prevent build errors in the OfflineRT sample for GDK's before 2203 Update 1 (220301)";
        outCompressedStream.write(emptyFileString, strlen(emptyFileString));
        outCompressedStream.close();
#endif

        // create dstFile-based name with ".compressed.GPU.mdat" postfix
        wcsncpy_s(compressedFileNameBuffer + compressedFileNamePrefixSize, _countof(compressedGpuName), compressedGpuName, _countof(compressedGpuName));
#if (_GXDK_VER >= 0x585D0BE9) /* GDK Edition 230300 */
        printf("Serializing bottom-level acceleration structure (Compressed for GPU)\n");

        // BVH GPU Compression available in March 2023 GDK onwards
        // Set the Copy Mode based on whether we want a Compressed BVH or not
        blasSerializationTime = SerializeBvhToMemory(serializationBuffer, postbuildSerializationInfo.SerializedSizeInBytes, resultBuffer, cmdList.Get(), D3D12XBOX_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZE_AND_COMPRESS_FOR_GPU, objFile);
        WriteBvhToMdatFile(compressedFileNameBuffer, serializationBuffer, serializedHeader->SerializedSizeInBytesIncludingHeader, serializedHeader->DeserializedSizeInBytes,
            prebuildInfo, geometryDesc.Triangles, sizeof(vertex_t), blasBuildTime, blasSerializationTime, true);
#else
        std::ofstream outCompressedGpuStream(compressedFileNameBuffer, std::ios::out | std::ios::binary | std::ios::trunc);
        char* emptyFileString = "Empty File - Created to prevent build errors in the OfflineRT sample for GDK's before March 2023 (230300)";
        outCompressedGpuStream.write(emptyFileString, strlen(emptyFileString));
        outCompressedGpuStream.close();
#endif

        // create dstFile-based name with ".compressed.Dehydrated.mdat" postfix
        wcsncpy_s(compressedFileNameBuffer + compressedFileNamePrefixSize, _countof(dehydratedName), dehydratedName, _countof(dehydratedName));
#if defined(DEHYDRATED_BVH_SUPPORTED)
        printf("Writing bottom-level acceleration structure (Dehydrated)\n");

        // BVH Dehydration API available in March 2024 GDK onwards
        WriteBvhToMdatFile(compressedFileNameBuffer, resultBufferDehydrated, dehydratedPostbuildCurrentSizeInfo.CurrentSizeInBytes, dehydratedPostbuildCurrentSizeInfo.CurrentSizeInBytes,
            dehydratedPrebuildInfo, geometryDesc.Triangles, sizeof(vertex_t), dehydratedBLASBuildTime, 0.0f, true);

        resultBufferDehydrated.Reset();
        scratchBufferDehydrated.Reset();
#else
        std::ofstream outDehydratedStream(compressedFileNameBuffer, std::ios::out | std::ios::binary | std::ios::trunc);
        const char* errorString = "Empty File - Created to prevent build errors in the OfflineRT sample for GDK's before March 2024 GDK";
        outDehydratedStream.write(errorString, strlen(errorString));
        outDehydratedStream.close();
#endif


        free(compressedFileNameBuffer);

        // Release result buffer
        resultBuffer.Reset();
        scratchBuffer.Reset();

        // Clean up
        printf("Cleaning up UMD device objects\n");

        serializationBuffer.Reset();
        cmdList.Reset();
        cmdAllocator.Reset();
        umdDevice.Reset();

        printf("Done building model\n\n");
    }

    // Function loads a DXIL library and creates a Xbox-native raytracing state
    // object. The state object is then serialized to a blob and written out as
    // a binary file. 
    //
    // Two different kinds of state objects can be produced: RTPSO and
    // collections. An RTPSO is a fully compiled and linked state object which
    // can be deserialized and used directly in the game. A collection is a
    // group of entry points (fully compiled to native GPU code) and sub-objects
    // that can be linked with other collections at runtime to form an RTPSO.
    // 
    // This functionality shifts the computation-heavy process of state object
    // compilation and linking from runtime to pipeline time.
    //
    // Note:
    // The PC version of the Scarlett UMD also prints a lot of additional
    // information to the debug output window which may prove valuable for
    // debugging purposes.
    void BuildStateObject(D3D12_STATE_OBJECT_TYPE type, const wchar_t* dxilLibFile, const wchar_t* dstFile)
    {
        // Create UMD12 device - this will fail if the dependent DLLs are not available
        // in the working directory or DLL search path. The project copies these during
        // a custom build step.
        printf("Creating offline Scarlett UMD device\n");
        ComPtr<ID3D12Device8> umdDevice;
        DX::ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_GRAPHICS_PPV_ARGS(umdDevice.ReleaseAndGetAddressOf())));

        // Load DXIL library data from file
        printf("Loading DXIL library: %ls\n", dxilLibFile);
        auto dxilLibData = DX::ReadData(dxilLibFile);

        // Create state object
        CD3DX12_STATE_OBJECT_DESC stateObjectDesc(type);

        // DXIL library contains all subobjects needed for the pipeline or
        // collection to build native GPU code. We rely on default association
        // (https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#subobject-association-behavior)
        // to link shaders/ hitgroups to root signatures.
        //
        // You can read more about the minimum requirements for being able to
        // natively compile a collection here:
        // https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#collection-state-object
        D3D12_SHADER_BYTECODE dxilLibBytecode = { dxilLibData.data(), dxilLibData.size() };
        auto libSubObject = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
        libSubObject->SetDXILLibrary(&dxilLibBytecode);

        printf("Building %ls object\n", type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE ? L"RTPSO" : L"collection");
        ComPtr<ID3D12StateObject> stateObject;
        auto buildStartTime = std::chrono::high_resolution_clock::now();
        DX::ThrowIfFailed(umdDevice->CreateStateObject(stateObjectDesc, IID_GRAPHICS_PPV_ARGS(stateObject.ReleaseAndGetAddressOf())));
        auto buildEndTime = std::chrono::high_resolution_clock::now();

        float buildTime = std::chrono::duration<float, std::milli>(buildEndTime - buildStartTime).count();
        printf("Finished build: time = %5.2f ms\n", buildTime);

        // Serialize state object
        printf("Serializing %ls object\n", type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE ? L"RTPSO" : L"collection");

        ComPtr<IXDXRBlob> serializedStateObject;
        auto serializationStartTime = std::chrono::high_resolution_clock::now();
        DX::ThrowIfFailed(umdDevice->SerializeStateObjectX(stateObject.Get(), 0, serializedStateObject.ReleaseAndGetAddressOf()));
        auto serializationEndTime = std::chrono::high_resolution_clock::now();

        float serializationTime = std::chrono::duration<float, std::milli>(serializationEndTime - serializationStartTime).count();
        printf("Finished serialization: time = %5.2f ms, size = %llu bytes\n", buildTime, serializedStateObject->GetBufferSize());

        // Build file header
        printf("Writing serialized object to file: %ls\n", dstFile);

        OfflineRTFiles::StateObjectFileHeader fileHeader = {};

        fileHeader.Magic = MAKEFOURCC('S', 'O', 'B', 'J');
        fileHeader.HeaderSize = sizeof(fileHeader);

        fileHeader.Type = type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE ? OfflineRTFiles::StateObjectType::RTPSO : OfflineRTFiles::StateObjectType::Collection;
        fileHeader.SerializedSize = static_cast<UINT>(serializedStateObject->GetBufferSize());

        fileHeader.BuildTime = buildTime;
        fileHeader.SerializationTime = serializationTime;

        // Write out data
        std::ofstream outStream(dstFile, std::ios::out | std::ios::binary | std::ios::trunc);
        outStream.write(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
        outStream.write(reinterpret_cast<char*>(serializedStateObject->GetBufferPointer()), fileHeader.SerializedSize);
        outStream.close();

        printf("Done writing serialized data\n");

        // Clear up
        printf("Cleaning up UMD device objects\n");
        serializedStateObject.Reset();
        stateObject.Reset();
        umdDevice.Reset();

        printf("Done building state object\n\n");
    }
} // End unnamed namespace

int main()
{
    // Disable buffering on stdout so that printf() calls are not delayed
    setvbuf(stdout, NULL, _IONBF, 0);

    // Decompress source asset files
    if (!DX::FileExists(L"..\\Build\\Int\\Dragon_LOD0.obj"))
    {
        auto decompressedDragon = DX::ReadCompressedData(L"..\\Assets\\Dragon_LOD0.ob_");
        DX::WriteData(L"..\\Build\\Int\\Dragon_LOD0.obj", decompressedDragon);
    }

    // Build model bottom-level acceleration structure + vertex data
    BuildModelData<false /* don't use canonical API and custom BVH build */, false /* Don't care */>
        ("..\\Build\\Int\\Dragon_LOD0.obj", L"..\\Build\\Dragon_LOD0.mdat", true);

    BuildModelData<true /* use canonical API and custom BVH build */, true /* Use only traingle leaves in custom BVH */>
        ("..\\Build\\Int\\Dragon_LOD0.obj", L"..\\Build\\Dragon_LOD0_bvhtoy_triangles.mdat", true);

    BuildModelData<true /* use canonical API and custom BVH build */, false /* Use both quad and traingle leaves in custom BVH */>
        ("..\\Build\\Int\\Dragon_LOD0.obj", L"..\\Build\\Dragon_LOD0_bvhtoy_quads.mdat", true);

    // Build raytracing state object collection from DXIL library. The DXIL
    // libraries are compiled from the OfflineCollectionRaygen.hlsl and
    // OfflineCollectionHitgroups.hlsl files in the project.
    BuildStateObject(
        D3D12_STATE_OBJECT_TYPE_COLLECTION,
        L"..\\Build\\Int\\OfflineCollectionRaygen.dxil-lib.bin",
        L"..\\Build\\OfflineCollectionRaygen.sobj");
    BuildStateObject(
        D3D12_STATE_OBJECT_TYPE_COLLECTION,
        L"..\\Build\\Int\\OfflineCollectionHitgroups.dxil-lib.bin",
        L"..\\Build\\OfflineCollectionHitgroups.sobj");

    // Build fully linked raytracing state object from DXIL library. The DXIL
    // library is compiled from the OfflineRTPSO.hlsl file in the project.
    BuildStateObject(
        D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
        L"..\\Build\\Int\\OfflineRTPSO.dxil-lib.bin",
        L"..\\Build\\OfflineRTPSO.sobj");

    return 0;
}
