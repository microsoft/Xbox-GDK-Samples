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
