//--------------------------------------------------------------------------------------
// GltfRender.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GltfRender.h"

#include "ATGColors.h"
#include "Constants.h"
#include "FindMedia.h"
#include "gltf/gltf.h"
#include "gltf/GltfProcessing.h"
#include "ReadData.h"
#include "Scene.h"

#include <filesystem>


extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    // Asset paths
    const wchar_t* c_folderPaths[] =
    {
        L"Media\\Meshes\\Ship",
        L"Media\\Meshes\\CoastRocks",
        0
    };

    const wchar_t* c_gltfPaths[] =
    {
        L"ship_pinnace_4k_merged_meshes.gltf",
        L"coast_rocks_03_4k.gltf"
    };

    const SceneInstance c_sceneCollection[] =
    {
        { 0, XMMatrixScaling(1, 1, 1) * XMMatrixTranslation(-70.0f, 0.0f, -50.0f) },
        { 0, XMMatrixRotationY(1.5f) * XMMatrixScaling(1, 1, 1) * XMMatrixTranslation(-50.0f, 0.0f, 70.0f) },
        { 0, XMMatrixScaling(2, 2, 2) * XMMatrixTranslation(40.0f, 0.0f, 40.0f) },
        { 1, XMMatrixScaling(5, 5, 5) },
    };

    struct DescriptorIndex
    {
        enum : uint32_t
        {
            RTOutputUAV,
            MeshInfo,
            Count
        };
    };

    struct RootSigRT
    {
        enum
        {
            Constants,
            TLAS,
            RTOutput,
            MeshInfo
        };
    };

    struct RootSigRaster
    {
        enum
        {
            Constants,
            RootConstants,
            MeshInfo
        };
    };
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_buildTLAS(true),
    m_buildBLAS(true),
    m_useRaytracing(false),
    m_supportsRaytracing(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT, 2, DX::DeviceResources::c_ReverseDepth | DX::DeviceResources::c_EnableDXR);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

#ifdef _GAMING_XBOX
    m_deviceResources->WaitForOrigin();
#endif

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    m_mouse->EndOfInputFrame();

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);
        m_camera.Update(elapsedTime, pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.a == DirectX::GamePad::ButtonStateTracker::PRESSED)
        {
            if (m_supportsRaytracing)
            {
                m_useRaytracing = !m_useRaytracing;
            }
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
        ExitSample();
    }

    if (kb.A)
    {
        if (m_supportsRaytracing)
        {
            m_useRaytracing = !m_useRaytracing;
        }
    }

    auto mouse = m_mouse->GetState();
    mouse;
    m_camera.Update(elapsedTime, *m_mouse.get(), *m_keyboard.get());

    PIXEndEvent();
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    if (m_useRaytracing)
    {
        // This leaves the backbuffer in present state
        m_deviceResources->Prepare(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT);
    }
    else
    {
        m_deviceResources->Prepare();
        Clear();
    }

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    auto const size = m_deviceResources->GetOutputSize();
    auto displayWidth = static_cast<uint32_t>(size.right - size.left);
    auto displayHeight = static_cast<uint32_t>(size.bottom - size.top);

#ifndef _GAMING_XBOX_XBOXONE
    if (m_useRaytracing)
    {
        if (m_buildBLAS)
        {
            BuildBLASesFromGLTF();
            m_buildBLAS = false;
        }

        if (m_buildTLAS)
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"TLAS rebuild");
            BuildTLASFromGLTF();
            m_buildTLAS = false;

            PIXEndEvent(commandList);
        }
    }
#endif

    // Update scene constants
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Update scene constants");

        auto cbUploadMem = m_graphicsMemory->Allocate(sizeof(SceneConstants));
        SceneConstants& constants = *static_cast<SceneConstants*>(cbUploadMem.Memory());

        XMMATRIX camView = m_camera.GetView();
        XMMATRIX camProj = m_camera.GetProjection();
        XMVECTOR camPos = m_camera.GetPosition();
        XMMATRIX worldViewProjection = camView * camProj;

        XMStoreFloat4x4(&constants.projectionViewWorld, m_useRaytracing ? XMMatrixInverse(nullptr, worldViewProjection) : XMMatrixTranspose(worldViewProjection));
        XMStoreFloat3(&constants.cameraWorldPos, camPos);
        constants.rayMaxLength = 5000.0f;
        constants.lightDiffuseColor = { 2.0f, 2.0f, 2.0f, 1.0f };
        constants.lightAmbientColor = { 0.2f, 0.2f, 0.2f, 1.0f };
        constants.lightPosition = {-100.f, 1250.f, 1500.f};

        constants.invScreenDimensions = { 1.0f / displayWidth, 1.0f / displayHeight };

        // Copy upload data to constant buffer
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_sceneConstants.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->ResourceBarrier(1, &barrier);

        commandList->CopyBufferRegion(m_sceneConstants.Get(), 0, cbUploadMem.Resource(), cbUploadMem.ResourceOffset(), cbUploadMem.Size());

        PIXEndEvent(commandList);
    }

    if (m_useRaytracing)
    {
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Raytrace");
            D3D12_RESOURCE_BARRIER barriers[] =
            {
                CD3DX12_RESOURCE_BARRIER::Transition(m_sceneConstants.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
                CD3DX12_RESOURCE_BARRIER::Transition(m_rtOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            };
            commandList->ResourceBarrier(ARRAYSIZE(barriers), barriers);

            ID3D12DescriptorHeap* heaps[] = { m_srvHeap->Heap(), m_samplerHeap->Heap()};
            commandList->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);

            commandList->SetComputeRootSignature(m_rootSignatureRT.Get());
            commandList->SetComputeRootConstantBufferView(RootSigRT::Constants, m_sceneConstants->GetGPUVirtualAddress());
            commandList->SetComputeRootShaderResourceView(RootSigRT::TLAS, m_TLAS->GetGPUVirtualAddress());
            commandList->SetComputeRootDescriptorTable(RootSigRT::MeshInfo, m_srvHeap->GetGpuHandle(DescriptorIndex::MeshInfo));
            commandList->SetComputeRootDescriptorTable(RootSigRT::RTOutput, m_srvHeap->GetGpuHandle(DescriptorIndex::RTOutputUAV));

            commandList->SetPipelineState(m_pipelineStateRT.Get());

            commandList->Dispatch(AlignUp(displayWidth, THREAD_GROUP_X) / THREAD_GROUP_X, AlignUp(displayHeight, THREAD_GROUP_Y) / THREAD_GROUP_Y, 1);
            PIXEndEvent(commandList);
        }

        // Copy from raytracing output buffer to backbuffer
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Copy to backbuffer");

            D3D12_RESOURCE_BARRIER barriers[2] =
            {
                CD3DX12_RESOURCE_BARRIER::Transition(m_rtOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST)
            };
            commandList->ResourceBarrier(ARRAYSIZE(barriers), barriers);

            commandList->CopyResource(m_deviceResources->GetRenderTarget(), m_rtOutput.Get());

            D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
            commandList->ResourceBarrier(1, &barrier);
            PIXEndEvent(commandList);
        }
    }
    else
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Rasterize");
        D3D12_RESOURCE_BARRIER barriers[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_sceneConstants.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
        };
        commandList->ResourceBarrier(ARRAYSIZE(barriers), barriers);

        ID3D12DescriptorHeap* heaps[] = { m_srvHeap->Heap(), m_samplerHeap->Heap() };
        commandList->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);

        commandList->SetGraphicsRootSignature(m_rootSignatureRaster.Get());
        commandList->SetGraphicsRootConstantBufferView(RootSigRaster::Constants, m_sceneConstants->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(RootSigRaster::MeshInfo, m_srvHeap->GetGpuHandle(DescriptorIndex::MeshInfo));

        commandList->SetPipelineState(m_pipelineStateRaster.Get());

        for (size_t sceneIndex = 0; sceneIndex < _countof(c_sceneCollection); ++sceneIndex)
        {
            auto& scene = m_scenes[c_sceneCollection[sceneIndex].sceneIndex];
            for (auto& modelInstance : scene->m_instances)
            {
                auto& model = scene->m_uniqueModels[static_cast<uint32_t>(modelInstance->modelIndex)];
                for (size_t meshIndex = 0; meshIndex < model->meshBuffers.size(); ++meshIndex)
                {
                    auto& mesh = model->meshBuffers[meshIndex];
                    MeshConstants mc;
                    XMStoreFloat4x4(&mc.world, XMMatrixTranspose(modelInstance->world* c_sceneCollection[sceneIndex].transform));
                    mc.meshInfoIndex = model->m_meshInfoOffset + static_cast<uint32_t>(meshIndex);
                    commandList->SetGraphicsRoot32BitConstants(RootSigRaster::RootConstants, sizeof(MeshConstants) / sizeof(uint32_t), &mc, 0);
                    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    commandList->DrawInstanced(mesh.indexCount, 1, 0, 0);
                }
            }
        }

        PIXEndEvent(commandList);
    }

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
}

void Sample::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

// Properties
void Sample::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 1920;
    height = 1080;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_6 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_6))
    {
        throw std::runtime_error("Shader Model 6.6 is not supported!");
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 features = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features, sizeof(features)))
        || (features.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED))
    {
        m_useRaytracing = false;
        m_supportsRaytracing = false;
    }
    else
    {
        m_supportsRaytracing = true;
    }
#endif

#ifdef _GAMING_XBOX_XBOXONE
    m_useRaytracing = false;
    m_supportsRaytracing = false;
#elif defined(_GAMING_XBOX_SCARLETT)
    m_supportsRaytracing = true;
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // Load GLTF scenes
    std::vector<std::unique_ptr<glTF::Asset>> assets(_countof(c_gltfPaths));
    for (size_t i = 0; i < assets.size(); ++i)
    {
        assets[i] = std::make_unique<glTF::Asset>(c_gltfPaths[i], c_folderPaths);
    }

    size_t numGltfMeshes = 0;
    size_t numTextures = 0;
    size_t numSamplers = 0;
    uint32_t sceneIndex = 0;
    m_scenes.resize(assets.size());
    for (auto& asset : assets)
    {
        numTextures += asset->m_textures.size();
        numSamplers += asset->m_samplers.size();
        for (const auto& mesh : asset->m_meshes)
        {
            numGltfMeshes += mesh.primitives.size();
        }

        m_scenes[sceneIndex] = std::make_unique<Scene>();
        m_scenes[sceneIndex]->m_uniqueModels.resize(asset->m_meshes.size());
        sceneIndex++;
    }

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Create descriptor heap.
    m_srvHeap = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        DescriptorIndex::Count + numTextures + numGltfMeshes * 2);

    m_samplerHeap = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        numSamplers);

    // Upload textures to GPU.
    m_textureFactory = std::make_unique<EffectTextureFactory>(device, resourceUpload, m_srvHeap->Heap());
    uint32_t texOffset = DescriptorIndex::Count;
    uint32_t sampOffset = 0;
    std::vector<uint32_t> texOffsets(_countof(c_gltfPaths));
    std::vector<uint32_t> sampOffsets(_countof(c_gltfPaths));
    for (size_t assetIndex = 0; assetIndex < assets.size(); ++assetIndex)
    {
        const auto& asset = assets[assetIndex];
        texOffsets[assetIndex] = texOffset;
        for (auto& tex : asset->m_textures)
        {
#ifdef _GAMING_XBOX
            // remove directory from file name since all files are copied in the root path
            auto filename = std::filesystem::path(tex.source->path).filename().wstring();
            m_textureFactory->CreateTexture(filename.c_str(), static_cast<int>(texOffset));
#else
            wchar_t path[MAX_PATH];
            wchar_t strFilePath[MAX_PATH] = {};
            const char* filename = tex.source->path.c_str();
            mbstowcs_s(nullptr, path, filename, MAX_PATH);
            DX::FindMediaFile(strFilePath, MAX_PATH, path, c_folderPaths);
            m_textureFactory->CreateTexture(strFilePath, static_cast<int>(texOffset));
#endif
            ++texOffset;
        }

        sampOffsets[assetIndex] = sampOffset;
        for (size_t samplerIndex = 0; samplerIndex < asset->m_samplers.size(); ++samplerIndex)
        {
            D3D12_SAMPLER_DESC samp = {};
            samp.Filter = asset->m_samplers[samplerIndex].filter;
            samp.AddressU = asset->m_samplers[samplerIndex].wrapS;
            samp.AddressV = asset->m_samplers[samplerIndex].wrapT;
            samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samp.MinLOD = 0;
            samp.MaxLOD = D3D12_FLOAT32_MAX;
            samp.MipLODBias = 0.0f;
            samp.MaxAnisotropy = 1;

            device->CreateSampler(&samp, m_samplerHeap->GetCpuHandle(sampOffset));
            ++sampOffset;
        }
    }

    std::unique_ptr<MeshInfo[]> meshInfos;
    meshInfos.reset(new MeshInfo[numGltfMeshes]);
    uint32_t meshCounter = 0;

    // process GLTF scene
    sceneIndex = 0;
    for (size_t assetIndex = 0; assetIndex < assets.size(); ++assetIndex)
    {
        const auto& asset = assets[assetIndex];
        auto& scene = m_scenes[sceneIndex++];

        std::function<void(size_t, uint32_t, size_t)> perMeshCallback = [&scene, meshCounter](size_t meshIndex, uint32_t meshOffset, size_t numPrimitives)
            {
                ModelData* model = new ModelData();
                model->m_meshInfoOffset = meshCounter + meshOffset;
                scene->m_uniqueModels[meshIndex].reset(model);
                model->meshBuffers.resize(numPrimitives);
            };

        auto& srvHeapRef = m_srvHeap;
        uint32_t assetTexOffset = texOffsets[assetIndex];
        uint32_t assetSampOffset = sampOffsets[assetIndex];
        std::function<void(size_t, uint32_t, size_t, int, const Renderer::Primitive&)> perPrimitiveCallback
            = [&device, &scene, &resourceUpload, meshCounter, &srvHeapRef, &meshInfos, assetTexOffset, assetSampOffset, texOffset, &asset]
            (size_t meshIndex, uint32_t meshOffset, size_t primitiveIndex, int materialIndex, const Renderer::Primitive& outPrim)
            {
                const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
                ModelData& model = *scene->m_uniqueModels[meshIndex];
                auto& meshVbIb = model.meshBuffers[primitiveIndex];

                // upload vertex buffer
                {
                    auto const desc = CD3DX12_RESOURCE_DESC::Buffer(outPrim.VB->size());
                    DX::ThrowIfFailed(device->CreateCommittedResource(
                        &heapProperties,
                        D3D12_HEAP_FLAG_NONE,
                        &desc,
#ifdef _GAMING_XBOX
                        D3D12_RESOURCE_STATE_COPY_DEST,
#else
                        D3D12_RESOURCE_STATE_COMMON,
#endif
                        nullptr,
                        IID_GRAPHICS_PPV_ARGS(meshVbIb.vb.ReleaseAndGetAddressOf())
                    ));

                    meshVbIb.vb->SetName(L"mesh vb");
                    meshVbIb.vertexStride = outPrim.vertexStride;
                    meshVbIb.vertexCount = outPrim.vertexCount;

                    D3D12_SUBRESOURCE_DATA initData = { outPrim.VB->data(), 0, 0 };
                    resourceUpload.Upload(meshVbIb.vb.Get(), 0, &initData, 1);

                    resourceUpload.Transition(meshVbIb.vb.Get(),
                        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                }

                // upload index buffer
                {
                    auto const desc = CD3DX12_RESOURCE_DESC::Buffer(outPrim.IB->size());
                    DX::ThrowIfFailed(device->CreateCommittedResource(
                        &heapProperties,
                        D3D12_HEAP_FLAG_NONE,
                        &desc,
#ifdef _GAMING_XBOX
                        D3D12_RESOURCE_STATE_COPY_DEST,
#else
                        D3D12_RESOURCE_STATE_COMMON,
#endif
                        nullptr,
                        IID_GRAPHICS_PPV_ARGS(meshVbIb.ib.ReleaseAndGetAddressOf())
                    ));

                    meshVbIb.ib->SetName(L"mesh ib");
                    meshVbIb.indexFormat = outPrim.index32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
                    meshVbIb.indexCount = outPrim.primCount;

                    D3D12_SUBRESOURCE_DATA initData = { outPrim.IB->data(), 0, 0 };
                    resourceUpload.Upload(meshVbIb.ib.Get(), 0, &initData, 1);

                    resourceUpload.Transition(meshVbIb.ib.Get(),
                        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                }

                // Create SRVs and geometry descs for model resources
                {
                    MeshInfo& meshInfo = meshInfos[meshCounter + meshOffset];
                    uint32_t srvOffset = 2 * (meshCounter + meshOffset) + texOffset;
                    meshInfo.indicesIndex = srvOffset;
                    meshInfo.vbIndex = srvOffset + 1;
                    meshInfo.texIndex = asset->m_materials[static_cast<uint32_t>(materialIndex)].textures[glTF::Material::kBaseColor]->index - 1 + assetTexOffset;
                    meshInfo.samplerIndex = asset->m_materials[static_cast<uint32_t>(materialIndex)].textures[glTF::Material::kBaseColor]->samplerIndex + assetSampOffset;

                    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

                    // create either a 16-bit or 32-bit index buffer view
                    srvDesc.Format = meshVbIb.indexFormat;
                    srvDesc.Buffer.FirstElement = 0; // change to non-zero if we support packed index buffers in the future
                    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
                    srvDesc.Buffer.NumElements = outPrim.primCount;
                    device->CreateShaderResourceView(meshVbIb.ib.Get(), &srvDesc, srvHeapRef->GetCpuHandle(meshInfo.indicesIndex));

                    srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
                    srvDesc.Buffer.FirstElement = 0;
                    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
                    srvDesc.Buffer.NumElements = outPrim.vertexCount * outPrim.vertexStride / 4;
                    device->CreateShaderResourceView(meshVbIb.vb.Get(), &srvDesc, srvHeapRef->GetCpuHandle(meshInfo.vbIndex));


                    meshInfo.vertexStride = outPrim.vertexStride;
                    meshInfo.normalOffset = outPrim.normalOffset;
                    meshInfo.texOffset = outPrim.texCoordOffset;
                }
            };

        meshCounter += IterateMeshes(*asset, perMeshCallback, perPrimitiveCallback);

        // iterate through nodes and flatten graph
        {
            scene->m_instances.resize(asset->m_nodes.size());

            std::function callback = [&scene](uint32_t curPos, int modelIndex, const DirectX::SimpleMath::Matrix& xform) -> void
                {
                    ModelInstance* modelInstance = new ModelInstance();
                    scene->m_instances[curPos].reset(modelInstance);
                    modelInstance->world = xform;
                    modelInstance->modelIndex = modelIndex;
                };

            WalkGraph(callback, asset->m_scene->nodes, 0, XMMatrixIdentity());
        }
    }

    DX::ThrowIfFailed(CreateStaticBuffer<MeshInfo>(device,
        resourceUpload,
        meshInfos.get(),
        numGltfMeshes,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        m_meshInfoBuffer.ReleaseAndGetAddressOf()));
    m_meshInfoBuffer->SetName(L"Mesh Info");

    // Structured buffer view for mesh info
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.StructureByteStride = sizeof(MeshInfo);
    srvDesc.Buffer.NumElements = static_cast<UINT>(numGltfMeshes);
    device->CreateShaderResourceView(m_meshInfoBuffer.Get(), &srvDesc, m_srvHeap->GetCpuHandle(DescriptorIndex::MeshInfo));

    // Create scene constant buffer
    {
        constexpr size_t sceneConstantsSize = sizeof(SceneConstants);
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sceneConstantsSize, D3D12_RESOURCE_FLAG_NONE);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
#ifdef _GAMING_XBOX
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
#else
            D3D12_RESOURCE_STATE_COMMON,
#endif
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_sceneConstants.ReleaseAndGetAddressOf())));

        m_sceneConstants->SetName(L"Scene Constants");
    }

    // Create root signature and pipeline states
#ifndef _GAMING_XBOX_XBOXONE
    if (m_supportsRaytracing)
    {
        auto computeShaderBlob = DX::ReadData(L"InlineRTCS.cso");

        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignatureRT.ReleaseAndGetAddressOf())));
        m_rootSignatureRT->SetName(L"RT RootSig");

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_rootSignatureRT.Get();
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateRT.ReleaseAndGetAddressOf())));
        m_pipelineStateRT->SetName(L"InlineRTCS");
    }
#endif

    {
        auto const vertexShaderBlob = DX::ReadData(L"VertexShader.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignatureRaster.ReleaseAndGetAddressOf())));

        auto const pixelShaderBlob = DX::ReadData(L"PixelShader.cso");

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = D3D12_INPUT_LAYOUT_DESC(); // use empty input layout
        psoDesc.pRootSignature = m_rootSignatureRaster.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CommonStates::DepthReverseZ;
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateRaster.ReleaseAndGetAddressOf())));
    }

    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    finished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const size = m_deviceResources->GetOutputSize(); 
    auto displayWidth = static_cast<int>(size.right - size.left);
    auto displayHeight = static_cast<int>(size.bottom - size.top);

    m_camera.SetWindow(displayWidth, displayHeight);
    m_camera.SetProjectionParameters(XM_PIDIV4, 10000.0f, 1.0f, true);
    m_camera.SetLookAt(SimpleMath::Vector3(0.6f, 37.0f, 178.0f), SimpleMath::Vector3::Forward);
    m_camera.SetSensitivity(500.0f, 100.0f, 1000.0f, 10.0f);

    // Create output resource for raytracing
    auto device = m_deviceResources->GetD3DDevice();
    auto const backbufferFormat = m_deviceResources->GetBackBufferFormat();
    auto rtOutputDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        backbufferFormat,
        static_cast<uint32_t>(displayWidth),
        static_cast<uint32_t>(displayHeight),
        1, // array size
        1, // mip levels
        1, //sample count
        0, // sample quality
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    const CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &rtOutputDesc,
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_rtOutput.ReleaseAndGetAddressOf())));
    m_rtOutput->SetName(L"Raytracing output");
    DirectX::CreateUnorderedAccessView(device, m_rtOutput.Get(), m_srvHeap->GetCpuHandle(DescriptorIndex::RTOutputUAV), 0);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_srvHeap.reset();
    m_samplerHeap.reset();
    m_textureFactory.reset();
    m_meshInfoBuffer.Reset();
    m_sceneConstants.Reset();
    m_rootSignatureRT.Reset();
    m_pipelineStateRT.Reset();
    m_rtOutput.Reset();
    m_TLASBuildScratch.Reset();
    m_TLAS.Reset();

    m_buildBLAS = true;
    m_buildTLAS = true;
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

#ifndef _GAMING_XBOX_XBOXONE
void Sample::BuildTLASFromGLTF()
{
    auto device = m_deviceResources->GetD3DDevice();
    ID3D12Device5* device5;
    device->QueryInterface(__uuidof(ID3D12Device5), (void**)&device5);
    auto commandList = m_deviceResources->GetCommandList();
    ID3D12GraphicsCommandList4* commandList4;
    commandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList4), (void**)&commandList4);

    size_t numInstances = 0;
    for (size_t i = 0; i < _countof(c_sceneCollection); ++i)
    {
        numInstances += m_scenes[c_sceneCollection[i].sceneIndex]->m_instances.size();
    }

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = tlasBuildDesc.Inputs;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = buildFlags;
    topLevelInputs.NumDescs = static_cast<UINT>(numInstances);
    topLevelInputs.pGeometryDescs = nullptr;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    auto instanceDescUploadMem = m_graphicsMemory->Allocate(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numInstances);

    D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs = reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(instanceDescUploadMem.Memory());

    uint32_t instanceCounter = 0;
    for (size_t sceneIndex = 0; sceneIndex < _countof(c_sceneCollection); ++sceneIndex)
    {
        auto& scene = m_scenes[c_sceneCollection[sceneIndex].sceneIndex];
        for (auto& modelInstance : scene->m_instances)
        {
            instanceDescs[instanceCounter] = {};
            XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(&instanceDescs[instanceCounter].Transform), modelInstance->world * c_sceneCollection[sceneIndex].transform);
            instanceDescs[instanceCounter].InstanceMask = 1;
            instanceDescs[instanceCounter].InstanceID = scene->m_uniqueModels[static_cast<uint32_t>(modelInstance->modelIndex)]->m_meshInfoOffset;
            instanceDescs[instanceCounter].InstanceContributionToHitGroupIndex = 0;
            instanceDescs[instanceCounter].AccelerationStructure = scene->m_uniqueModels[static_cast<uint32_t>(modelInstance->modelIndex)]->m_BLAS->GetGPUVirtualAddress();
            ++instanceCounter;
        }
    }

    // Set instance memory pointer in TLAS description
    tlasBuildDesc.Inputs.InstanceDescs = instanceDescUploadMem.GpuAddress();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    device5->GetRaytracingAccelerationStructurePrebuildInfo(&tlasBuildDesc.Inputs,
        &topLevelPrebuildInfo);
    DX::ThrowIfFalse(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0, "Illegal TLAS size");

    // Allocate scratch buffer for build
    UINT64 scratchSize = topLevelPrebuildInfo.ScratchDataSizeInBytes;
    const CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
#ifdef _GAMING_XBOX
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
#else
        D3D12_RESOURCE_STATE_COMMON,
#endif
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_TLASBuildScratch.ReleaseAndGetAddressOf())));
    m_TLASBuildScratch->SetName(L"TLASBuildScratch");

    bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    DX::ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_TLAS.ReleaseAndGetAddressOf())));
    m_TLAS->SetName(L"TLAS");

    // Set resource pointers in TLAS description
    tlasBuildDesc.DestAccelerationStructureData = m_TLAS->GetGPUVirtualAddress();
    tlasBuildDesc.ScratchAccelerationStructureData = m_TLASBuildScratch->GetGPUVirtualAddress();

    commandList4->BuildRaytracingAccelerationStructure(&tlasBuildDesc, 0, nullptr);
}

void Sample::BuildBLASesFromGLTF()
{
    auto device = m_deviceResources->GetD3DDevice();
    ID3D12Device5* device5;
    device->QueryInterface(__uuidof(ID3D12Device5), (void**)&device5);
    auto commandList = m_deviceResources->GetCommandList();
    ID3D12GraphicsCommandList4* commandList4;
    commandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList4), (void**)&commandList4);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    size_t totalUniqueModels = 0;
    for (auto& scene : m_scenes)
    {
        totalUniqueModels += scene->m_uniqueModels.size();
    }

    for (auto& scene : m_scenes)
    {
        for (size_t modelIndex = 0; modelIndex < scene->m_uniqueModels.size(); ++modelIndex)
        {
            size_t totalGeos = scene->m_uniqueModels[modelIndex]->meshBuffers.size();
            std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geoDescs(totalGeos);
            size_t geoCounter = 0;
            for (auto& mesh : scene->m_uniqueModels[modelIndex]->meshBuffers)
            {
                // Create SRVs and geometry descs for model resources
                {
                    geoDescs[geoCounter] = {};
                    geoDescs[geoCounter].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
                    geoDescs[geoCounter].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
                    geoDescs[geoCounter].Triangles.IndexBuffer = mesh.ib->GetGPUVirtualAddress();
                    geoDescs[geoCounter].Triangles.IndexCount = mesh.indexCount;
                    geoDescs[geoCounter].Triangles.IndexFormat = mesh.indexFormat;
                    geoDescs[geoCounter].Triangles.Transform3x4 = 0;
                    geoDescs[geoCounter].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
                    geoDescs[geoCounter].Triangles.VertexCount = mesh.vertexCount;
                    geoDescs[geoCounter].Triangles.VertexBuffer.StartAddress = mesh.vb->GetGPUVirtualAddress();
                    geoDescs[geoCounter].Triangles.VertexBuffer.StrideInBytes = mesh.vertexStride;
                }
                geoCounter++;
            }

            // Build bottom-level-acceleration structure (BLAS)
            UINT64 blasCreationScratchBytes = {};
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC modelBLASDesc = {};
            modelBLASDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            modelBLASDesc.Inputs.Flags = buildFlags;
            modelBLASDesc.Inputs.NumDescs = static_cast<UINT>(totalGeos);
            modelBLASDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            modelBLASDesc.Inputs.pGeometryDescs = geoDescs.data();

            auto postBuildCurrentSize = m_graphicsMemory->Allocate(sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE_DESC));
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postbuildInfo[] =
            {
                {postBuildCurrentSize.GpuAddress(), D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_CURRENT_SIZE}
            };

            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
            device5->GetRaytracingAccelerationStructurePrebuildInfo(&modelBLASDesc.Inputs,
                &bottomLevelPrebuildInfo);
            DX::ThrowIfFalse(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0, "Illegal BLAS size");

            // Allocate scratch buffer for build
            blasCreationScratchBytes = bottomLevelPrebuildInfo.ScratchDataSizeInBytes;
            auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(blasCreationScratchBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
#ifdef _GAMING_XBOX
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
#else
                D3D12_RESOURCE_STATE_COMMON,
#endif
                nullptr,
                IID_GRAPHICS_PPV_ARGS(scene->m_uniqueModels[modelIndex]->m_BLASScratch.ReleaseAndGetAddressOf())));
            scene->m_uniqueModels[modelIndex]->m_BLASScratch->SetName(L"BLASScratch");

            // Allocate acceleration structure buffer
            bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(scene->m_uniqueModels[modelIndex]->m_BLAS.ReleaseAndGetAddressOf())));
            scene->m_uniqueModels[modelIndex]->m_BLAS->SetName(L"BLAS");

            // Set resource pointers in BLAS description
            modelBLASDesc.ScratchAccelerationStructureData = scene->m_uniqueModels[modelIndex]->m_BLASScratch->GetGPUVirtualAddress();
            modelBLASDesc.DestAccelerationStructureData = scene->m_uniqueModels[modelIndex]->m_BLAS->GetGPUVirtualAddress();

            commandList4->BuildRaytracingAccelerationStructure(&modelBLASDesc, 0, nullptr);
        }
    }

    D3D12_RESOURCE_BARRIER barriers[] =
    {
        CD3DX12_RESOURCE_BARRIER::UAV(nullptr),
    };
    commandList->ResourceBarrier(ARRAYSIZE(barriers), barriers);
}
#endif
