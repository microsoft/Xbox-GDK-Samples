//--------------------------------------------------------------------------------------
// FileFormat.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once
#pragma once

namespace OfflineRTFiles
{
    // File header for model data produced by RTBuilder tool
    //
    // The file contains multiple parts linearly laid out as follows in the file
    // container:
    //
    // Part content         Part size
    // ----------------------------------------------------
    // Header               Header.HeaderSize
    // BVH blob data        Header.PostbuildSerializedSize
    // Index buffer data    Header.IndexCount * sizeof(uint32_t)
    // Vertex buffer data   Header.VertexCount * 2 * sizeof(XMFLOAT3)
    struct ModelFileHeader
    {
        UINT        Magic;                      // MAKEFOURCC(`M`,`D`,`A`,`T`)
        UINT        HeaderSize;                 // Size of this header

        UINT        IndexCount;                 // Number of indices in the index part (format: uint32_t)
        UINT        VertexCount;                // Number of vertices in the vertex part (format: vertex{ XMFLOAT3 pos, XMFLOAT3 normal })

        // BVH build stats:
        UINT64      PrebuildScratchSize;        // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO::ScratchDataSizeInBytes
        UINT64      PrebuildResultMaxSize;      // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO::ResultDataMaxSizeInBytes
        UINT64      PostbuildCurrentSize;       // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC::CurrentSizeInBytes
        UINT64      PostbuildSerializedSize;    // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_SERIALIZATION_DESC::SerializedSizeInBytes
        float       BuildTime;                  // Build time in milliseconds
        float       SerializationTime;          // Serialization time in milliseconds
    };

    enum class StateObjectType : UINT
    {
        RTPSO,          // Fully compiled and and linked RTPSO
        Collection      // Collection of precompiled shaders that can be linked at runtime 
    };

    // File header for state object data produced by RTBuilder tool
    //
    // Part content         Part size
    // ----------------------------------------------------
    // Header               Header.HeaderSize
    // State object data    Header.SerializedSize
    struct StateObjectFileHeader
    {
        UINT            Magic;                      // MAKEFOURCC(`S`,`O`,`B`,`J`)
        UINT            HeaderSize;                 // Size of this header

        StateObjectType Type;
        UINT            SerializedSize;             // Size of state object data following this header in the file

        float           BuildTime;                  // Build time in milliseconds
        float           SerializationTime;          // Serialization time in milliseconds
    };

}; // End namespace: OfflineRT
