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
// Author(s):  James Stanard
//             Chuck Walbourn (ATG)
//
// This code depends on DirectXMesh
//

#include "pch.h"
#include "GltfProcessing.h"

#include "DirectXMesh.h"
#include "gltf.h"

using namespace DirectX;
using namespace glTF;

namespace
{
    inline void ThrowIfFalse(bool cond, const char* message)
    {
        if (!cond)
        {
            throw std::exception(message);
        }
    }

    DXGI_FORMAT JointIndexFormat(const Accessor& accessor)
    {
        switch (accessor.componentType)
        {
        case Accessor::kUnsignedByte:  return DXGI_FORMAT_R8G8B8A8_UINT;
        case Accessor::kUnsignedShort: return DXGI_FORMAT_R16G16B16A16_UINT;
        default:
            ThrowIfFalse(false, "Invalid joint index format");
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    DXGI_FORMAT AccessorFormat(const Accessor& accessor)
    {
        switch (accessor.componentType)
        {
        case Accessor::kUnsignedByte:
            switch (accessor.type)
            {
            case Accessor::kScalar: return DXGI_FORMAT_R8_UNORM;
            case Accessor::kVec2:   return DXGI_FORMAT_R8G8_UNORM;
            default:                return DXGI_FORMAT_R8G8B8A8_UNORM;
            }
        case Accessor::kUnsignedShort:
            switch (accessor.type)
            {
            case Accessor::kScalar: return DXGI_FORMAT_R16_UNORM;
            case Accessor::kVec2:   return DXGI_FORMAT_R16G16_UNORM;
            default:                return DXGI_FORMAT_R16G16B16A16_UNORM;
            }
        case Accessor::kFloat:
            switch (accessor.type)
            {
            case Accessor::kScalar: return DXGI_FORMAT_R32_FLOAT;
            case Accessor::kVec2:   return DXGI_FORMAT_R32G32_FLOAT;
            case Accessor::kVec3:   return DXGI_FORMAT_R32G32B32_FLOAT;
            default:                return DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
        default:
            ThrowIfFalse(false, "Invalid accessor format");
            return DXGI_FORMAT_UNKNOWN;
        }
    }
} // namespace

void OptimizeMesh(Renderer::Primitive& outPrim, const glTF::Primitive& inPrim, const glTF::Material material)
{
    ThrowIfFalse(inPrim.attributes[0] != nullptr, "Must have POSITION");
    
    // Additional null check to satisfy static analysis
    if (inPrim.attributes[0] == nullptr)
    {
        throw std::exception("Position attribute is null");
    }
    
    const uint32_t vertexCount = inPrim.attributes[0]->count;
    outPrim.vertexCount = vertexCount;

    void* indices = nullptr;
    uint32_t indexCount;
    bool b32BitIndices;
    uint32_t maxIndex = inPrim.maxIndex;

    if (inPrim.indices == nullptr)
    {
        ThrowIfFalse(inPrim.mode == 4, "Impossible primitive topology when lacking indices");

        indexCount = vertexCount * 3;
        maxIndex = indexCount - 1;
        if (indexCount > 0xFFFF)
        {
            b32BitIndices = true;
            outPrim.IB = std::make_shared<std::vector<byte>>(4 * indexCount);
            indices = outPrim.IB->data();
            uint32_t* tmp = (uint32_t*)indices;
            for (uint32_t i = 0; i < indexCount; ++i)
                tmp[i] = i;
        }
        else
        {
            b32BitIndices = false;
            outPrim.IB = std::make_shared<std::vector<byte>>(2 * indexCount);
            indices = outPrim.IB->data();
            uint16_t* tmp = (uint16_t*)indices;
            for (uint16_t i = 0; i < indexCount; ++i)
                tmp[i] = i;
        }
    }
    else
    {
        switch (inPrim.mode)
        {
        default:
        case 0: // POINT LIST
        case 1: // LINE LIST
        case 2: // LINE LOOP
        case 3: // LINE STRIP
            OutputDebugStringA("Found unsupported primitive topology\n");
            return;
        case 4: // TRIANGLE LIST
            break;
        case 5: // TODO: Convert TRIANGLE STRIP
        case 6: // TODO: Convert TRIANGLE FAN
            OutputDebugStringA("Found an index buffer that needs to be converted to a triangle list\n");
            return;
        }

        indices = inPrim.indices->dataPtr;
        indexCount = inPrim.indices->count;
        if (maxIndex == 0)
        {
            if (inPrim.indices->componentType == Accessor::kUnsignedInt)
            {
                uint32_t* ib = (uint32_t*)inPrim.indices->dataPtr;
                for (uint32_t k = 0; k < indexCount; ++k)
                    maxIndex = std::max(ib[k], maxIndex);
            }
            else
            {
                uint16_t* ib = (uint16_t*)inPrim.indices->dataPtr;
                for (uint32_t k = 0; k < indexCount; ++k)
                    maxIndex = std::max<uint32_t>(ib[k], maxIndex);
            }
        }
        b32BitIndices = maxIndex > 0xFFFF;
        uint32_t indexSize = b32BitIndices ? 4U : 2U;
        outPrim.IB = std::make_shared<std::vector<byte>>(indexSize * indexCount);

        if (b32BitIndices == (inPrim.indices->componentType == Accessor::kUnsignedInt))
        {
            outPrim.IB->assign((byte*)indices, (byte*)indices + indexCount * indexSize);
        }
        else
        {
            auto outIndices = (uint16_t*)outPrim.IB->data();
            auto inIndices = (uint32_t*)inPrim.indices->dataPtr;
            for (uint32_t i = 0; i < indexCount; ++i)
            {
                // Conversion is valid because we know maxIndex < 0x10000
                outIndices[i] = uint16_t(inIndices[i]);
            }
        }

        indices = outPrim.IB->data();
    }

    ThrowIfFalse(maxIndex > 0, "maxIndex not greater than 0");

    const bool HasNormals = inPrim.attributes[glTF::Primitive::kNormal] != nullptr;
    const bool HasTangents = inPrim.attributes[glTF::Primitive::kTangent] != nullptr;
    const bool HasUV0 = inPrim.attributes[glTF::Primitive::kTexcoord0] != nullptr;
    const bool HasUV1 = inPrim.attributes[glTF::Primitive::kTexcoord1] != nullptr;
    const bool HasJoints = inPrim.attributes[glTF::Primitive::kJoints0] != nullptr;
    const bool HasWeights = inPrim.attributes[glTF::Primitive::kWeights0] != nullptr;
    const bool HasSkin = HasJoints && HasWeights;

    std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;
    InputElements.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, glTF::Primitive::kPosition });
    if (HasNormals)
    {
        InputElements.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, glTF::Primitive::kNormal });
    }
    if (HasTangents)
    {
        InputElements.push_back({ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,  glTF::Primitive::kTangent });
    }
    if (HasUV0)
    {
        InputElements.push_back({ "TEXCOORD", 0,
            AccessorFormat(*inPrim.attributes[glTF::Primitive::kTexcoord0]),
            glTF::Primitive::kTexcoord0 });
    }
    if (HasUV1)
    {
        InputElements.push_back({ "TEXCOORD", 1,
            AccessorFormat(*inPrim.attributes[glTF::Primitive::kTexcoord1]),
            glTF::Primitive::kTexcoord1 });
    }
    if (HasSkin)
    {
        InputElements.push_back({ "BLENDINDICES", 0,
            JointIndexFormat(*inPrim.attributes[glTF::Primitive::kJoints0]),
            glTF::Primitive::kJoints0 });
        InputElements.push_back({ "BLENDWEIGHT", 0,
            AccessorFormat(*inPrim.attributes[glTF::Primitive::kWeights0]),
            glTF::Primitive::kWeights0 });
    }

    VBReader vbr;
    vbr.Initialize({ InputElements.data(), (uint32_t)InputElements.size() });

    for (uint32_t i = 0; i < Primitive::kNumAttribs; ++i)
    {
        Accessor* attrib = inPrim.attributes[i];
        if (attrib)
            vbr.AddStream(attrib->dataPtr, vertexCount, i, attrib->stride);
    }

    std::unique_ptr<XMFLOAT3[]> position;
    std::unique_ptr<XMFLOAT3[]> normal;
    std::unique_ptr<XMFLOAT4[]> tangent;
    std::unique_ptr<XMFLOAT2[]> texcoord0;
    std::unique_ptr<XMFLOAT2[]> texcoord1;
    std::unique_ptr<XMFLOAT4[]> joints;
    std::unique_ptr<XMFLOAT4[]> weights;
    position.reset(new XMFLOAT3[vertexCount]);
    normal.reset(new XMFLOAT3[vertexCount]);

    DX::ThrowIfFailed(vbr.Read(position.get(), "POSITION", 0, vertexCount));

    if (HasNormals)
    {
        DX::ThrowIfFailed(vbr.Read(normal.get(), "NORMAL", 0, vertexCount));
    }
    else
    {
        const size_t faceCount = indexCount / 3;

        if (b32BitIndices)
            ComputeNormals((const uint32_t*)indices, faceCount, position.get(), vertexCount, CNORM_DEFAULT, normal.get());
        else
            ComputeNormals((const uint16_t*)indices, faceCount, position.get(), vertexCount, CNORM_DEFAULT, normal.get());
    }

    if (HasUV0)
    {
        texcoord0.reset(new XMFLOAT2[vertexCount]);
        DX::ThrowIfFailed(vbr.Read(texcoord0.get(), "TEXCOORD", 0, vertexCount));
    }

    if (HasUV1)
    {
        texcoord1.reset(new XMFLOAT2[vertexCount]);
        DX::ThrowIfFailed(vbr.Read(texcoord1.get(), "TEXCOORD", 1, vertexCount));
    }

    if (HasTangents)
    {
        tangent.reset(new XMFLOAT4[vertexCount]);
        DX::ThrowIfFailed(vbr.Read(tangent.get(), "TANGENT", 0, vertexCount));
    }
    else
    {
        ThrowIfFalse(maxIndex < vertexCount, "max index greater than vertex count");
        ThrowIfFalse(indexCount % 3 == 0, "index count not a multiple of three");

        HRESULT hr = S_OK;

        if (HasUV0 && material.properties.normalUV == 0)
        {
            tangent.reset(new XMFLOAT4[vertexCount]);
            if (b32BitIndices)
            {
                hr = ComputeTangentFrame((uint32_t*)indices, indexCount / 3, position.get(), normal.get(), texcoord0.get(),
                    vertexCount, tangent.get());
            }
            else
            {
                hr = ComputeTangentFrame((uint16_t*)indices, indexCount / 3, position.get(), normal.get(), texcoord0.get(),
                    vertexCount, tangent.get());
            }
        }
        else if (HasUV1 && material.properties.normalUV == 1)
        {
            tangent.reset(new XMFLOAT4[vertexCount]);
            if (b32BitIndices)
            {
                hr = ComputeTangentFrame((uint32_t*)indices, indexCount / 3, position.get(), normal.get(), texcoord1.get(),
                    vertexCount, tangent.get());
            }
            else
            {
                hr = ComputeTangentFrame((uint16_t*)indices, indexCount / 3, position.get(), normal.get(), texcoord1.get(),
                    vertexCount, tangent.get());
            }
        }

        if (FAILED(hr))
        {
            OutputDebugStringA("Error generating a tangent frame");
        }
        DX::ThrowIfFailed(hr);
    }

    if (HasSkin)
    {
        joints.reset(new XMFLOAT4[vertexCount]);
        weights.reset(new XMFLOAT4[vertexCount]);
        DX::ThrowIfFailed(vbr.Read(joints.get(), "BLENDINDICES", 0, vertexCount));
        DX::ThrowIfFailed(vbr.Read(weights.get(), "BLENDWEIGHT", 0, vertexCount));
    }

    // Use VBWriter to generate a new, interleaved and compressed vertex buffer
    std::vector<D3D12_INPUT_ELEMENT_DESC> OutputElements;
    uint32_t elementIndex = 0;

    outPrim.psoFlags = PSOFlags::kHasPosition | PSOFlags::kHasNormal;
    OutputElements.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT });
    ++elementIndex;
    OutputElements.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT });
    uint32_t normalIndex = elementIndex;
    ++elementIndex;
    if (tangent.get())
    {
        OutputElements.push_back({ "TANGENT", 0, DXGI_FORMAT_R10G10B10A2_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT });
        outPrim.psoFlags |= PSOFlags::kHasTangent;
        ++elementIndex;
    }
    uint32_t texCoordIndex = 0;
    if (texcoord0.get())
    {
        OutputElements.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT });
        outPrim.psoFlags |= PSOFlags::kHasUV0;
        texCoordIndex = elementIndex;
        ++elementIndex;
    }
    if (texcoord1.get())
    {
        OutputElements.push_back({ "TEXCOORD", 1, DXGI_FORMAT_R16G16_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT });
        outPrim.psoFlags |= PSOFlags::kHasUV1;
    }
    if (HasSkin)
    {
        OutputElements.push_back({ "BLENDINDICES", 0, DXGI_FORMAT_R16G16B16A16_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT });
        OutputElements.push_back({ "BLENDWEIGHT", 0, DXGI_FORMAT_R16G16B16A16_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT });
        outPrim.psoFlags |= PSOFlags::kHasSkin;
    }

    D3D12_INPUT_LAYOUT_DESC layout = { OutputElements.data(), (uint32_t)OutputElements.size() };

    VBWriter vbw;
    vbw.Initialize(layout);

    uint32_t offsets[10];
    uint32_t strides[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    ComputeInputLayout(layout, offsets, strides);
    uint32_t stride = strides[0];
    outPrim.normalOffset = offsets[normalIndex];
    outPrim.texCoordOffset = offsets[texCoordIndex];

    outPrim.VB = std::make_shared<std::vector<byte>>(stride * vertexCount);
    DX::ThrowIfFailed(vbw.AddStream(outPrim.VB->data(), vertexCount, 0, stride));

    vbw.Write(position.get(), "POSITION", 0, vertexCount);
    vbw.Write(normal.get(), "NORMAL", 0, vertexCount, true);
    if (tangent.get())
        vbw.Write(tangent.get(), "TANGENT", 0, vertexCount, true);
    if (texcoord0.get())
        vbw.Write(texcoord0.get(), "TEXCOORD", 0, vertexCount);
    if (texcoord1.get())
        vbw.Write(texcoord1.get(), "TEXCOORD", 1, vertexCount);
    if (HasSkin)
    {
        vbw.Write(joints.get(), "BLENDINDICES", 0, vertexCount);
        vbw.Write(weights.get(), "BLENDWEIGHT", 0, vertexCount);
    }

    outPrim.vertexStride = (uint16_t)stride;
    outPrim.index32 = b32BitIndices;

    outPrim.primCount = indexCount;
}

uint32_t IterateMeshes(
    const glTF::Asset& asset,
    std::function<void (size_t meshIndex, uint32_t meshOffset, size_t numPrimitives)> perMeshCallback,
    std::function<void(size_t meshIndex, uint32_t meshOffset, size_t primitiveIndex, int materialIndex, const Renderer::Primitive& outPrim)> perPrimitiveCallback)
{
    uint32_t totalCount = 0;
    for (size_t meshIndex = 0; meshIndex < asset.m_meshes.size(); ++meshIndex)
    {
        const auto& mesh = asset.m_meshes[meshIndex];
        perMeshCallback(meshIndex, totalCount, mesh.primitives.size());

        for (size_t primIndex = 0; primIndex < mesh.primitives.size(); ++ primIndex)
        {
            const auto& primitive = mesh.primitives[primIndex];
            Renderer::Primitive outPrim;
            OptimizeMesh(outPrim, primitive, asset.m_materials[static_cast<uint32_t>(primitive.materialIndex)]);
            perPrimitiveCallback(meshIndex, totalCount, primIndex, primitive.materialIndex, outPrim);
            ++totalCount;
        }
    }

    return totalCount;
}

uint32_t WalkGraph(
    std::function<void(uint32_t curPos, int modelIndex, const DirectX::SimpleMath::Matrix& xform)> nodeProcessedCallback,
    const std::vector<glTF::Node*>& siblings,
    uint32_t curPos,
    const SimpleMath::Matrix& xform)
{
    size_t numSiblings = siblings.size();

    for (size_t i = 0; i < numSiblings; ++i)
    {
        glTF::Node* curNode = siblings[i];
        DirectX::SimpleMath::Matrix world;

        if (curNode->properties.hasMatrix)
        {
            std::memcpy((float*)&world, curNode->matrix, sizeof(curNode->matrix));
        }
        else
        {
            XMFLOAT3 translation = *(const XMFLOAT3*)curNode->srt.translation;
            FXMVECTOR quat = *(const FXMVECTOR*)curNode->srt.rotation;
            float scale = *(float*)curNode->srt.scale;
            world = xform * SimpleMath::Matrix(
                XMMatrixRotationQuaternion(quat) * XMMatrixScaling(scale, scale, scale) *
                XMMatrixTranslation(translation.x, translation.y, translation.z)
            );
        }

        int modelIndex = -1;
        if (curNode->properties.pointsToCamera)
        {
            modelIndex = -1;
        }
        else
        {
            modelIndex = curNode->meshIdx;
        }

        nodeProcessedCallback(curPos, modelIndex, world);

        uint32_t nextPos = curPos + 1;

        if (curNode->children.size() > 0)
        {
            nextPos = WalkGraph(nodeProcessedCallback, curNode->children, nextPos, world);
        }

        curPos = nextPos;
    }

    return curPos;
}
