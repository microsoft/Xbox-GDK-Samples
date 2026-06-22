//--------------------------------------------------------------------------------------
// ModelDrawHelper.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Draw the model using custom commands instead of drawing 
// directly using Model->Draw()
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"
#include "ModelEffect.h"
#include "WriteOwnBundlesHelper.h"

namespace ModelDrawHelper
{
    inline void SetVertexIndexBuffersBYOB(const DirectX::ModelMeshPart* meshPart, uint32_t** writeAddress)
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = meshPart->staticVertexBuffer->GetGPUVirtualAddress();
        vbv.StrideInBytes = meshPart->vertexStride;
        vbv.SizeInBytes = meshPart->vertexBufferSize;
        WriteOwnBundle::IASetVertexBuffersBYOB(writeAddress, 0, 1, &vbv);

        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = meshPart->staticIndexBuffer->GetGPUVirtualAddress();
        ibv.SizeInBytes = meshPart->indexBufferSize;
        ibv.Format = meshPart->indexFormat;
        WriteOwnBundle::IASetIndexBufferBYOB(writeAddress, &ibv);
    }

    inline void ModelSetVertexIndexBuffersBYOB(const DirectX::ModelMeshPart* modelMeshPart, uint32_t** writeAddress)
    {
        SetVertexIndexBuffersBYOB(modelMeshPart, writeAddress);
    }

    inline void ModelDraw(const DirectX::ModelMeshPart* modelMeshPart, _In_ ID3D12GraphicsCommandList* commandList)
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = modelMeshPart->staticVertexBuffer->GetGPUVirtualAddress();
        vbv.StrideInBytes = modelMeshPart->vertexStride;
        vbv.SizeInBytes = modelMeshPart->vertexBufferSize;
        commandList->IASetVertexBuffers(0, 1, &vbv);

        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = modelMeshPart->staticIndexBuffer->GetGPUVirtualAddress();
        ibv.SizeInBytes = modelMeshPart->indexBufferSize;
        ibv.Format = modelMeshPart->indexFormat;
        commandList->IASetIndexBuffer(&ibv);

        commandList->IASetPrimitiveTopology(modelMeshPart->primitiveType);

        commandList->DrawIndexedInstanced(modelMeshPart->indexCount, 1, modelMeshPart->startIndex, modelMeshPart->vertexOffset, 0);
    }

    inline void ModelBuildOwnBundle(const DirectX::ModelMeshPart* modelMeshPart,
        _Inout_ uint32_t** writeAddress,
        _In_ bool setVBIB)
    {
        if (setVBIB)
            SetVertexIndexBuffersBYOB(modelMeshPart, writeAddress);

        WriteOwnBundle::DrawIndexedInstancedBYOB(writeAddress, modelMeshPart->indexCount, 1, modelMeshPart->startIndex, modelMeshPart->vertexOffset, 0);
    }
};