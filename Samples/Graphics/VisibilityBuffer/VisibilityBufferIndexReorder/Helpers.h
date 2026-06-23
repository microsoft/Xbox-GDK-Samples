#pragma once

namespace Helpers
{
    // Load SDKMESH model, and generateW vertex/index buffer views.
    inline void LoadModel(ID3D12Device* device,
                            ID3D12CommandQueue* commandQueue,
                            const wchar_t* path,
                            std::unique_ptr<DirectX::ModelMeshPart>& model,
                            D3D12_VERTEX_BUFFER_VIEW* vertexBufferView,
                            D3D12_INDEX_BUFFER_VIEW* indexBufferView,
                            bool compressed)
    {
        std::unique_ptr<DirectX::Model> firstModel;
        if (compressed)
        {
            auto modelBlob = DX::ReadCompressedData(path);
            firstModel = DirectX::Model::CreateFromSDKMESH(device, modelBlob.data(), modelBlob.size());
        }
        else
        {
            firstModel = DirectX::Model::CreateFromSDKMESH(device, path);
        }

        auto& meshData = firstModel->meshes.front()->opaqueMeshParts.front();

        // Reorder index triplets to have "new" vertices as the leading vertex. This lessens reshading required for SV_PrimitiveID.
        int historyCount = 26;
        uint32_t* indexBuffer = (uint32_t*)meshData->indexBuffer.Memory();

        auto lookBackFunc = [indexBuffer](int currentIndex, int lookbackCount, uint32_t value) -> bool
        {
            int startIndex = currentIndex - lookbackCount;

            if (startIndex < 0)
            {
                startIndex = 0;
            }

            for (int i = startIndex; i < currentIndex; i++)
            {
                if (indexBuffer[i] == value)
                {
                    return true;
                }
            }

            return false;
        };

        for (size_t p = 0; p < meshData->indexCount / 3; p++)
        {
            int leadingIndexID = int(p) * 3;

            if (!lookBackFunc(leadingIndexID, historyCount, indexBuffer[leadingIndexID]))
            {
                continue;
            }

            for (int i = 1; i < 3; i++)
            {
                int indexID = int(p) * 3 + i;
                if (!lookBackFunc(indexID, historyCount, indexBuffer[indexID]))
                {
                    // Need to be careful to preserve winding order and avoid flipped triangles. Can't swap indices, must rotate.
                    std::swap(indexBuffer[indexID], indexBuffer[leadingIndexID]);
                    std::swap(indexBuffer[leadingIndexID + 1], indexBuffer[leadingIndexID + 2]);
                    continue;
                }
            }
        }

        // Move vertex/index buffers to default heap for better performance.
        DirectX::ResourceUploadBatch resourceUpload(device);

        resourceUpload.Begin();

        firstModel->LoadStaticBuffers(device, resourceUpload, true);

        auto uploadResourcesFinished = resourceUpload.End(commandQueue);

        uploadResourcesFinished.wait();

        if (vertexBufferView)
        {
            vertexBufferView->BufferLocation = meshData->staticVertexBuffer->GetGPUVirtualAddress();
            vertexBufferView->StrideInBytes = meshData->vertexStride;
            vertexBufferView->SizeInBytes = meshData->vertexBufferSize;
        }

        if (indexBufferView)
        {
            indexBufferView->BufferLocation = meshData->staticIndexBuffer->GetGPUVirtualAddress();
            indexBufferView->SizeInBytes = meshData->indexBufferSize;
            indexBufferView->Format = meshData->indexFormat;
        }

        // Move model out of scene, before scene is destroyed.
        model = std::move(meshData);
    }
}
