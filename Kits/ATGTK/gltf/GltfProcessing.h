//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard
//

#pragma once

#include "gltf.h"

namespace PSOFlags
{
    enum : uint16_t
    {
        kHasPosition = 0x001,  // Required
        kHasNormal = 0x002,  // Required
        kHasTangent = 0x004,
        kHasUV0 = 0x008,  // Required (for now)
        kHasUV1 = 0x010,
        kAlphaBlend = 0x020,
        kAlphaTest = 0x040,
        kTwoSided = 0x080,
        kHasSkin = 0x100,  // Implies having indices and weights
    };
}

namespace Renderer
{
    typedef std::shared_ptr<std::vector<uint8_t>> ByteArray;

    struct Primitive
    {
        ByteArray VB;
        ByteArray IB;
        uint32_t primCount = 0;
        uint16_t vertexStride = 0;
        uint16_t psoFlags = 0;
        uint32_t normalOffset = 0;
        uint32_t texCoordOffset = 0;
        uint32_t vertexCount = 0;
        bool index32 = false;
    };
}

void OptimizeMesh(Renderer::Primitive& outPrim, const glTF::Primitive& inPrim, const glTF::Material material);
uint32_t IterateMeshes(
    const glTF::Asset& asset,
    std::function<void(size_t meshIndex, uint32_t meshOffset, size_t numPrimitives)> perMeshCallback,
    std::function<void(size_t meshIndex, uint32_t meshOffset, size_t primitiveIndex, int materialIndex, const Renderer::Primitive& outPrim)> perPrimitiveCallback);
uint32_t WalkGraph(
    std::function<void(uint32_t curPos, int modelIndex, const DirectX::SimpleMath::Matrix& xform)> nodeProcessedCallback,
    const std::vector<glTF::Node*>& siblings,
    uint32_t curPos,
    const DirectX::SimpleMath::Matrix& xform);
