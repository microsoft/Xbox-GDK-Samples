//--------------------------------------------------------------------------------------
// DynamicCubeMap.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DynamicCubeMap.h"

#include "ATGColors.h"
#include "ConstantBuffer.h"
#include "ControllerFont.h"
#include "FindMedia.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace ATG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr float s_fovy = XM_PIDIV4;
    constexpr uint32_t c_envMapSize = 256;
    constexpr uint32_t c_mipLevels = 9;
    constexpr uint32_t c_mipLevelsPlusFakes = c_mipLevels + MIPS_IN_ONE_SHADER - 1;
    constexpr uint32_t c_cubeMapFaces = 6;
    constexpr uint32_t c_numRootConstants = sizeof(GenerateMipsConstants) / sizeof(uint32_t);
    constexpr float c_roomScale = 1.f;
    constexpr float c_cactus1Scale = 0.5f;
    constexpr float c_cactus2Scale = 0.2f;

    struct GenerateCubemapRootSignature
    {
        enum
        {
            Constants,
            CubemapConstants,
            MeshInfo,
            DiffuseTex,
            VerticesMS,
            VerticesVS,
            Meshlets,
            UniqueVertexIndices,
            PrimitiveIndices,
            CullData,
            RootConstants
        };
    };

    struct SceneRootSignature
    {
        enum
        {
            Constants,
            DiffuseTex,
            Vertices,
            RootConstants
        };
    };

    struct DescriptorIndex
    {
        enum
        {
            Font,
            ControllerFont,
            WhiteTexture,
            CubeMap,
            CubeMapArraySrv,
            CubeMapUav,
            Count
        };
    };

    struct DSVIndex
    {
        enum
        {
            Array,
            Single,
            Count
        };
    };

    struct RTVIndex
    {
        enum
        {
            Array,
            Face0,
            Face1,
            Face2,
            Face3,
            Face4,
            Face5,
            Count
        };
    };

    // Barebones definition of scene objects
    struct ObjectDefinition
    {
        size_t modelIndex;
        Matrix world;
        float  scale;
    };

    // Assest paths
    const wchar_t* c_modelPaths[] =
    {
        L"AbstractCathedral.sdkmesh",
        L"cactus.sdkmesh",
    };

    // Barebones definition of a scene
    const ObjectDefinition c_sceneDefinition[] =
    {
        { 0, XMMatrixScaling(c_roomScale, c_roomScale, c_roomScale) * XMMatrixTranslation(0.0f, 160.0f, 0.0f), c_roomScale },
        { 1, XMMatrixScaling(c_cactus1Scale, c_cactus1Scale, c_cactus1Scale) * XMMatrixTranslation(70.0f, 0.0f, 100.0f), c_cactus1Scale },
        { 1, XMMatrixScaling(c_cactus2Scale, c_cactus2Scale, c_cactus2Scale) * XMMatrixTranslation(-40.0f, 0.0f, 120.0f), c_cactus2Scale },
    };

    uint32_t GetElementOffset(const char* semanticName, std::shared_ptr<std::vector<D3D12_INPUT_ELEMENT_DESC>> vbDecl)
    {
        uint32_t offset = 0;
        for (auto& inputElement : *vbDecl.get())
        {
            if (strcmp(inputElement.SemanticName, semanticName) == 0)
            {
                break;
            }

            switch (inputElement.Format)
            {
            case DXGI_FORMAT_R32G32B32_FLOAT:
                offset += 12;
                break;
            case DXGI_FORMAT_B8G8R8A8_UNORM:
                offset += 4;
                break;
            case DXGI_FORMAT_R32G32_FLOAT:
                offset += 8;
                break;
            case DXGI_FORMAT_R8G8B8A8_UINT:
                offset += 4;
                break;
            case DXGI_FORMAT_R8G8B8A8_UNORM:
                offset += 4;
                break;
            default:
                break;
            }
        }

        return offset;
    }

    void DrawModelManually(
        ID3D12GraphicsCommandList* commandList,
        const Sample::ModelInfo& modelInfo,
        const DescriptorHeap& srvHeap,
        UINT textureRootIndex,
        UINT vertexBufferRootIndex,
        UINT rootConstantsIndex,
        UINT numInstances)
    {
        const DirectX::Model* model = modelInfo.m_model.get();
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, model->name.c_str());
        for (size_t j = 0; j < model->meshes.size(); ++j)
        {
            auto mesh = model->meshes[j].get();
            assert(mesh != nullptr);

            for (size_t k = 0; k < mesh->opaqueMeshParts.size(); ++k)
            {
                auto part = mesh->opaqueMeshParts[k].get();
                assert(part != nullptr);
                VBLayout vbLayout;
                vbLayout.vertexStride = part->vertexStride;
                vbLayout.texOffset = modelInfo.m_texCoordOffsets[j][k];
                auto textureDescriptor = model->GetGpuTextureHandleForMaterialIndex(part->materialIndex, srvHeap.Heap(), srvHeap.Increment(), modelInfo.m_descriptorOffset);
                commandList->SetGraphicsRootDescriptorTable(textureRootIndex, textureDescriptor.ptr == 0 ?  srvHeap.GetGpuHandle(DescriptorIndex::WhiteTexture) : textureDescriptor);
                commandList->SetGraphicsRootShaderResourceView(vertexBufferRootIndex, part->staticVertexBuffer->GetGPUVirtualAddress());
                commandList->SetGraphicsRoot32BitConstants(rootConstantsIndex, sizeof(VBLayout) / sizeof(uint32_t), &vbLayout, 0);
                D3D12_INDEX_BUFFER_VIEW ibv;
                ibv.BufferLocation = part->staticIndexBuffer ? part->staticIndexBuffer->GetGPUVirtualAddress() : part->indexBuffer.GpuAddress();
                ibv.SizeInBytes = part->indexBufferSize;
                ibv.Format = part->indexFormat;
                commandList->IASetIndexBuffer(&ibv);
                commandList->IASetPrimitiveTopology(part->primitiveType);
                commandList->DrawIndexedInstanced(part->indexCount, numInstances, part->startIndex, part->vertexOffset, 0);
            }
        }
    }

#ifdef _GAMING_XBOX_SCARLETT
    void DrawModelMeshShader(ID3D12GraphicsCommandList6* commandList, const Sample::ModelInfo& modelInfo, const DescriptorHeap& srvHeap)
    {
        const DirectX::Model* model = modelInfo.m_model.get();
        const std::vector<ATG::MeshletSet>& meshletData = modelInfo.m_meshlet;
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, model->name.c_str());

        for (size_t j = 0; j < meshletData.size(); ++j)
        {
            const MeshletSet& meshletSet = meshletData[j];
            const std::vector<Submesh>& submeshes = meshletSet.GetSubmeshes();
            for (size_t k = 0; k < meshletSet.GetSubmeshCount(); ++k)
            {
                SubMeshlet subMeshData = {};
                subMeshData.offset = submeshes[k].Offset;
                subMeshData.vertexStride = model->meshes[j]->opaqueMeshParts[k]->vertexStride;
                subMeshData.texOffset = modelInfo.m_texCoordOffsets[j][k];
                commandList->SetGraphicsRootConstantBufferView(GenerateCubemapRootSignature::MeshInfo, meshletSet.GetMeshInfoResource()->GetGPUVirtualAddress());
                commandList->SetGraphicsRootDescriptorTable(GenerateCubemapRootSignature::DiffuseTex, model->GetGpuTextureHandleForMaterialIndex(model->meshes[j]->opaqueMeshParts[k]->materialIndex, srvHeap.Heap(), srvHeap.Increment(), modelInfo.m_descriptorOffset));
                commandList->SetGraphicsRootShaderResourceView(GenerateCubemapRootSignature::VerticesMS, model->meshes[j]->opaqueMeshParts[k]->staticVertexBuffer->GetGPUVirtualAddress());
                commandList->SetGraphicsRootShaderResourceView(GenerateCubemapRootSignature::Meshlets, meshletSet.GetMeshletResource()->GetGPUVirtualAddress());
                commandList->SetGraphicsRootShaderResourceView(GenerateCubemapRootSignature::UniqueVertexIndices, meshletSet.GetUniqueIndexResource()->GetGPUVirtualAddress());
                commandList->SetGraphicsRootShaderResourceView(GenerateCubemapRootSignature::PrimitiveIndices, meshletSet.GetPrimitiveResource()->GetGPUVirtualAddress());
                commandList->SetGraphicsRootShaderResourceView(GenerateCubemapRootSignature::CullData, meshletSet.GetCullDataResource()->GetGPUVirtualAddress());
                commandList->SetGraphicsRoot32BitConstants(GenerateCubemapRootSignature::RootConstants, sizeof(SubMeshlet) / sizeof(uint32_t), &subMeshData, 0);

                auto meshletCount = meshletSet.GetMeshletCount(static_cast<uint32_t>(k));
                commandList->DispatchMesh(meshletCount, 1, 1);
            }
        }
    }
#endif
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_useAsyncComputeForMipGeneration(false),
    m_renderMode(RenderMode::Loop)
{
    // Use gamma-correct rendering.
    m_deviceResources =
        std::make_unique<DX::DeviceResources>(
            DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
            DXGI_FORMAT_D32_FLOAT,
            2,
            DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD
            | DX::DeviceResources::c_GeometryShaders | DX::DeviceResources::c_AmplificationShaders
            );
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    // Generate cube map view matrices
    static const XMVECTORF32 c_eyePoint = { 0.f, 0.f, 0.f, 1.f };
    XMVECTORF32 lookDir;
    XMVECTORF32 upDir;

    lookDir = { 1.f, 0.f, 0.f, 1.f };
    upDir = { 0.0f, 1.0f, 0.0f, 1 };
    m_amCubeMapViewAdjust[0] = XMMatrixLookAtLH(c_eyePoint, lookDir, upDir);
    lookDir = { -1.0f, 0.f, 0.0f, 1 };
    upDir = { 0.0f, 1.0f, 0.0f, 1 };
    m_amCubeMapViewAdjust[1] = XMMatrixLookAtLH(c_eyePoint, lookDir, upDir);
    lookDir = { 0.0f, 1.0f, 0.0f, 1 };
    upDir = { 0.0f, 0.0f, -1.0f, 1 };
    m_amCubeMapViewAdjust[2] = XMMatrixLookAtLH(c_eyePoint, lookDir, upDir);
    lookDir = { 0.0f, -1.0f, 0.0f, 1 };
    upDir = { 0.0f, 0.0f, 1.0f, 1 };
    m_amCubeMapViewAdjust[3] = XMMatrixLookAtLH(c_eyePoint, lookDir, upDir);
    lookDir = { 0.0f, 0.f, 1.0f, 1 };
    upDir = { 0.0f, 1.0f, 0.0f, 1 };
    m_amCubeMapViewAdjust[4] = XMMatrixLookAtLH(c_eyePoint, lookDir, upDir);
    lookDir = { 0.0f, 0.f, -1.0f, 1 };
    upDir = { 0.0f, 1.0f, 0.0f, 1 };
    m_amCubeMapViewAdjust[5] = XMMatrixLookAtLH(c_eyePoint, lookDir, upDir);

    // Create the projection matrices
    m_projCBM = XMMatrixPerspectiveFovLH(XM_PI * 0.5f, 1.0f, .5f, 1000.f);

    m_deviceResources->SetWindow(window);

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

    m_deviceResources->WaitForOrigin();

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());
    auto const pad = m_gamePad->GetState(DirectX::GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        using ButtonState = DirectX::GamePad::ButtonStateTracker::ButtonState;
        m_gamePadButtons.Update(pad);
        m_camera.Update(elapsedTime, pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.y == ButtonState::PRESSED)
        {
            m_useAsyncComputeForMipGeneration = !m_useAsyncComputeForMipGeneration;
        }

        if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
        {
            m_renderMode = static_cast<RenderMode>((static_cast<int>(m_renderMode) + 1) % static_cast<int>(RenderMode::Count));
        }

        if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
        {
            int renderModeInt = static_cast<int>(m_renderMode);
            m_renderMode = static_cast<RenderMode>(renderModeInt > 0 ? renderModeInt - 1 : static_cast<int>(RenderMode::Count) - 1);
        }

        if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::HELD)
        {
            m_sphereMatrix.m[3][1] += 0.05f;
        }
        if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::HELD)
        {
            m_sphereMatrix.m[3][1] -= 0.05f;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }
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
    m_deviceResources->Prepare();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // If not using async compute then close the compute command list
    if (!m_useAsyncComputeForMipGeneration)
    {
        auto computeCommandList = m_deviceResources->GetComputeCommandList();
        computeCommandList->Close();
    }

    RenderSceneIntoCubeMap();

    // When using async compute the graphics queue must signal that the cubemap rendering is done and the compute queue will wait to kick off work
    // When not using async compute all work is recorded to one command list and executed at the end of the frame.
    if (m_useAsyncComputeForMipGeneration)
    {
        m_deviceResources->ExecuteCommandListAndReuse();
        m_deviceResources->ComputeWaiteOnGraphics();
    }

    GenerateMips();

    Clear();
    commandList->SetGraphicsRootSignature(m_rootSignatureScene.Get());
    commandList->SetPipelineState(m_pipelineStateScene.Get());

    RenderScene(
        commandList,
        m_camera.GetView(),
        m_camera.GetProjection(),
        SceneRootSignature::DiffuseTex,
        SceneRootSignature::Vertices,
        SceneRootSignature::RootConstants,
        1
#ifdef _GAMING_XBOX_SCARLETT
        , false
#endif
    );

    // When using async compute the compute queue must signal that the mip generation is done and the graphics queue will wait to kick off rendering
    // When not using async compute all work is recorded to one command list and executed at the end of the frame.
    if (m_useAsyncComputeForMipGeneration)
    {
        m_deviceResources->ExecuteCommandListAndReuse();
        m_deviceResources->GraphicsWaitOnCompute();
    }

    // Draw the reflective sphere
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Reflective Sphere");
        SetRenderTargetsAndViewports();
        TransitionResource(commandList, m_cubeMap.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->SetGraphicsRootSignature(m_rootSignatureSampleCubeMap.Get());
        commandList->SetPipelineState(m_pipelineStateSampleCubeMap.Get());
        commandList->SetGraphicsRootDescriptorTable(1, m_srvHeap->GetGpuHandle(DescriptorIndex::CubeMap));
        UpdateConstants(commandList, m_camera.GetView(), m_camera.GetProjection(), m_sphereMatrix, 1.0f);
        m_reflectiveSphere->Draw(commandList);
    }

    RenderHUD(commandList);

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::RenderScene(
    ID3D12GraphicsCommandList* commandList,
    XMMATRIX view,
    XMMATRIX proj,
    UINT textureRootIndex,
    UINT vertexBufferRootIndex,
    UINT rootConstantsIndex,
    UINT numInstances
#ifdef _GAMING_XBOX_SCARLETT
    , bool useMeshShader
#endif
)
{
    PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"RenderScene");
    ID3D12DescriptorHeap* heaps[] = { m_srvHeap->Heap() };
    commandList->SetDescriptorHeaps(1, heaps);

    // Draw the scene.
    for (auto& obj : m_scene)
    {
        UpdateConstants(commandList, view, proj, obj.m_world, obj.m_scale);
#ifdef _GAMING_XBOX_SCARLETT
        if (useMeshShader)
        {
            if (!obj.m_modelInfo->m_meshlet.empty())
            {
                DrawModelMeshShader(
                    static_cast<ID3D12GraphicsCommandList6*>(commandList),
                    *obj.m_modelInfo,
                    *m_srvHeap);
            }
        }
        else
#endif
        {
            DrawModelManually(
                commandList,
                *obj.m_modelInfo,
                *m_srvHeap,
                textureRootIndex,
                vertexBufferRootIndex,
                rootConstantsIndex,
                numInstances);
        }
    }
}

//--------------------------------------------------------------------------------------
// Render the scene into a cube map. This renders the scene with or without instancing
// and also using a render target array all in one call or one face at a time
//--------------------------------------------------------------------------------------
void Sample::RenderSceneIntoCubeMap()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"RenderSceneIntoCubeMap");

    // Set a new viewport for rendering to cube map
    CD3DX12_VIEWPORT vp(0.0f, 0.0f, c_envMapSize, c_envMapSize);
    commandList->RSSetViewports(1, &vp);

    auto const scissorRect = CD3DX12_RECT(0, 0, c_envMapSize, c_envMapSize);
    commandList->RSSetScissorRects(1, &scissorRect);

    TransitionResource(commandList, m_cubeMap.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Here, compute the view matrices used for cube map rendering.
    // First, construct mViewAlignCM, a view matrix with the same
    // orientation as m_mView but with eye point at the sphere position.
    XMMATRIX mViewAlignCBM = XMMatrixIdentity();
    mViewAlignCBM.r[3] = XMVectorSetW(DirectX::XMVECTOR(-m_sphereMatrix.Translation()), 1.0f);
    CBPerCubeRender cubeConstants;
    XMStoreFloat3(&cubeConstants.cubeViewPos, m_sphereMatrix.Translation());
    XMMATRIX mViewCBM[6];

    // Combine with the 6 different view directions to obtain the final view matrices.
    for (size_t view = 0; view < 6; ++view)
    {
        mViewCBM[view] = XMMatrixMultiply(mViewAlignCBM, m_amCubeMapViewAdjust[view]);
        XMStoreFloat4x4(&cubeConstants.mViewCBM[view], XMMatrixTranspose(mViewCBM[view]));

        // Update camera frustum planes for culling
        // http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
        XMMATRIX cViewProj = XMMatrixMultiply(mViewCBM[view], m_projCBM);
        XMMATRIX viewProj = XMMatrixTranspose(cViewProj);

        XMVECTOR planes[6] =
        {
            XMPlaneNormalize(XMVectorAdd(viewProj.r[3], viewProj.r[0])),      // Left
            XMPlaneNormalize(XMVectorSubtract(viewProj.r[3], viewProj.r[0])), // Right
            XMPlaneNormalize(XMVectorAdd(viewProj.r[3], viewProj.r[1])),      // Bottom
            XMPlaneNormalize(XMVectorSubtract(viewProj.r[3], viewProj.r[1])), // Top
            XMPlaneNormalize(viewProj.r[2]),                                  // Near
            XMPlaneNormalize(XMVectorSubtract(viewProj.r[3], viewProj.r[2])), // Far
        };

        for (size_t planeIndex = 0; planeIndex < 6; ++planeIndex)
        {
            XMStoreFloat4(&cubeConstants.planesPerCube[view][planeIndex], planes[planeIndex]);
        }
    }

    // RenderMode::Loop - submit the scene 6 times, once for each cube map face
    // RenderMode::GeometryShader - submit the scene once and use the geometry shader to send it to each of the cube faces with SV_RenderTargetArrayIndex
    // RenderMode::Instancing - submit the scene once and use instancing with SV_RenderTargetArrayIndex to send it to each of the cube faces
    // RenderMode::MeshShader - submit the scene once and use an amplification/mesh shader to send it to each of the cube faces with SV_RenderTargetArrayIndex
    if (m_renderMode != RenderMode::Loop)
    {
        auto const rtvHandle(m_rtvHeap->GetCpuHandle(RTVIndex::Array));
        auto const dsvHandle(m_dsvHeap->GetCpuHandle(DSVIndex::Array));
        commandList->ClearRenderTargetView(rtvHandle, ATG::ColorsLinear::Background, 0, nullptr);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

        ID3D12PipelineState* pPSO = nullptr;
#if MS_AND_GS_ALLOWED
        if (m_renderMode == RenderMode::GeometryShader)
        {
            pPSO = m_pipelineStateGenerateCubeMap.Get();
        }
        else
#endif
        if (m_renderMode == RenderMode::Instancing)
        {
            pPSO = m_pipelineStateGenerateCubeMapInstanced.Get();
        }
        else //RenderMode::MeshShader
        {
            pPSO = m_pipelineStateGenerateCubeMapMeshShader.Get();
        }

        commandList->SetGraphicsRootSignature(m_rootSignatureGenerateCubeMap.Get());
        commandList->SetPipelineState(pPSO);
        auto cbMem = m_graphicsMemory->AllocateConstant<CBPerCubeRender>(cubeConstants);
        commandList->SetGraphicsRootConstantBufferView(1, cbMem.GpuAddress());
        RenderScene(
            commandList,
            mViewCBM[0],
            m_projCBM,
            GenerateCubemapRootSignature::DiffuseTex,
            GenerateCubemapRootSignature::VerticesVS,
            GenerateCubemapRootSignature::RootConstants,
            m_renderMode == RenderMode::Instancing ? c_cubeMapFaces : 1
#ifdef _GAMING_XBOX_SCARLETT
#if MS_AND_GS_ALLOWED
            , m_renderMode == RenderMode::MeshShader
#else
            , false
#endif
#endif
        );
    }
    else
    {
        // Render one cube face at a time
        for (size_t view = 0; view < c_cubeMapFaces; ++view)
        {
            auto const rtvHandle(m_rtvHeap->GetCpuHandle(view + RTVIndex::Face0));
            auto const dsvHandle(m_dsvHeap->GetCpuHandle(DSVIndex::Single));
            commandList->ClearRenderTargetView(rtvHandle, ATG::ColorsLinear::Background, 0, nullptr);
            commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

            commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

            commandList->SetGraphicsRootSignature(m_rootSignatureScene.Get());
            commandList->SetPipelineState(m_pipelineStateGenerateCubeMapSingleView.Get());
            RenderScene(
                commandList,
                mViewCBM[view],
                m_projCBM,
                SceneRootSignature::DiffuseTex,
                SceneRootSignature::Vertices,
                SceneRootSignature::RootConstants,
                1
#ifdef _GAMING_XBOX_SCARLETT
                , false
#endif
            );
        }
    }

    TransitionResource(commandList, m_cubeMap.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void Sample::GenerateMips()
{
    auto commandList = m_useAsyncComputeForMipGeneration ? m_deviceResources->GetComputeCommandList() : m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"GenerateMips");
    commandList->SetComputeRootSignature(m_rootSignatureGenerateMips.Get());
    commandList->SetPipelineState(m_pipelineStateGenerateMips.Get());

    D3D12_RESOURCE_BARRIER barrierUAV = {};
    barrierUAV.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrierUAV.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierUAV.UAV.pResource = m_cubeMap.Get();

    commandList->SetComputeRootDescriptorTable(1, m_srvHeap->GetGpuHandle(DescriptorIndex::CubeMapArraySrv));

    for (uint32_t level = 0; level < c_mipLevels - 1;)
    {
        uint32_t srcWidth = std::max<uint32_t>(c_envMapSize >> level, 1u);
        uint32_t srcHeight = std::max<uint32_t>(c_envMapSize >> level, 1u);

        uint32_t destWidth = srcWidth >> 1;
        uint32_t destHeight = srcHeight >> 1;

        DWORD numMipsInDispatch = 0;
        _BitScanForward(&numMipsInDispatch, ((destWidth == 1) ? 0u : destWidth) | ((destHeight == 1) ? 0u : destHeight));

        // The minimum value for width and height should be 1.
        // The value of 0 prior to this makes sure that the number
        // of mips generated in a dispatch do not depend on this dimension
        if (destWidth == 0) destWidth = 1;
        if (destWidth == 0) destWidth = 1;

        numMipsInDispatch = std::min<DWORD>(numMipsInDispatch + 1, MIPS_IN_ONE_SHADER);
        numMipsInDispatch = std::min<DWORD>(c_mipLevels - level, numMipsInDispatch);

        GenerateMipsConstants constants;
        constants.SrcMipIndex = level;
        constants.InvOutTexelSize = XMFLOAT2(1.0f / float(destWidth), 1.0f / float(destHeight));
        constants.numMips = numMipsInDispatch;
        commandList->SetComputeRoot32BitConstants(0, c_numRootConstants, &constants, 0);
        commandList->SetComputeRootDescriptorTable(2, m_srvHeap->GetGpuHandle(static_cast<size_t>(DescriptorIndex::CubeMapUav) + level));

        // Transition current mip level to UAV to write the mip data
        D3D12_RESOURCE_BARRIER srvToUav[c_cubeMapFaces * MIPS_IN_ONE_SHADER];
        for (size_t curMip = 0; curMip < numMipsInDispatch; ++curMip)
        {
            for (size_t arraySlice = 0; arraySlice < c_cubeMapFaces; ++arraySlice)
            {
                size_t barrierIndex = curMip * c_cubeMapFaces + arraySlice;
                srvToUav[barrierIndex] = {};
                srvToUav[barrierIndex].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                srvToUav[barrierIndex].Transition.pResource = m_cubeMap.Get();
                srvToUav[barrierIndex].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                srvToUav[barrierIndex].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                srvToUav[barrierIndex].Transition.Subresource = static_cast<uint32_t>((level  + 1 + curMip) + arraySlice * c_mipLevels);

            }
        }
        commandList->ResourceBarrier(c_cubeMapFaces * numMipsInDispatch, srvToUav);


        UINT threadGroupX = (destWidth + 7) / 8;
        UINT threadGroupY = (destHeight + 7) / 8;
        commandList->Dispatch(threadGroupX, threadGroupY, 6);

        // Transition current mip level to SRV to be read from for computing next mip level
        D3D12_RESOURCE_BARRIER uavToSrvAndSync[c_cubeMapFaces * MIPS_IN_ONE_SHADER + 1];
        uavToSrvAndSync[0] = barrierUAV;
        for (size_t curMip = 0; curMip < numMipsInDispatch; ++curMip)
        {
            for (size_t arraySlice = 0; arraySlice < c_cubeMapFaces; ++arraySlice)
            {
                size_t barrierIndex = curMip * c_cubeMapFaces + arraySlice + 1;
                uavToSrvAndSync[barrierIndex] = {};
                uavToSrvAndSync[barrierIndex].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                uavToSrvAndSync[barrierIndex].Transition.pResource = m_cubeMap.Get();
                uavToSrvAndSync[barrierIndex].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                uavToSrvAndSync[barrierIndex].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                uavToSrvAndSync[barrierIndex].Transition.Subresource = static_cast<uint32_t>((level  + 1 + curMip) + arraySlice * c_mipLevels);
            }
        }
        commandList->ResourceBarrier(c_cubeMapFaces * numMipsInDispatch + 1, uavToSrvAndSync);

        level += numMipsInDispatch;
    }

    PIXEndEvent(commandList);

    // only execute the command list if we're using async compute
    // for non-async compute all work is recorded to the graphics command list
    if (m_useAsyncComputeForMipGeneration)
    {
        m_deviceResources->ExecuteComputeCommandList();
    }
}

void Sample::RenderHUD(ID3D12GraphicsCommandList* commandList)
{
    PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"RenderHUD");
    m_hudBatch->Begin(commandList);

    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(1920, 1080);
    const int boxPadding = 10;
    constexpr size_t textLength = 256;
    wchar_t topText[textLength] = {};
    const wchar_t* renderModeText = L"";
    const wchar_t* mipText = m_useAsyncComputeForMipGeneration ? L"Generating cube map mips on async compute queue" : L"Generating cube map mips on graphics queue";

    switch (m_renderMode)
    {
    case RenderMode::Loop:
        renderModeText = L"Rendering geometry six times, once per face";
        break;
#if MS_AND_GS_ALLOWED
    case RenderMode::GeometryShader:
        renderModeText = L"Submitting geometry once and transforming six times in a GS";
        break;
#endif
    case RenderMode::Instancing:
        renderModeText = L"Submitting geometry once with instancing, redirecting to a cubemap face via SV_RenderTargetArrayIndex";
        break;
#ifdef _GAMING_XBOX_SCARLETT
#if MS_AND_GS_ALLOWED
    case RenderMode::MeshShader:
        renderModeText = L"Submitting geometry once and amplifying six times in an AS with culling before the MS";
        break;
#endif
#endif
    case RenderMode::Count:
        renderModeText = L"";
        break;
    }

    swprintf_s(topText, textLength, L"Dynamic Cube Map\n%ls\n%ls", renderModeText, mipText);

    auto textMeasure = m_font->MeasureString(topText);
    int textWidth = static_cast<int>(XMVectorGetX(textMeasure));
    int textHeight = static_cast<int>(XMVectorGetY(textMeasure));
    auto const whiteTextureHandle = m_srvHeap->GetGpuHandle(DescriptorIndex::WhiteTexture);

    m_hudBatch->Draw(
        whiteTextureHandle,
        { 1, 1 },
        RECT{ safe.left - boxPadding, safe.top - boxPadding, safe.left + textWidth + boxPadding, safe.top + textHeight + boxPadding });

    m_font->DrawString(m_hudBatch.get(), topText, XMFLOAT2(float(safe.left), float(safe.top)), DirectX::Colors::DarkKhaki);

    auto const bottomText =
        L"[DPAD]: Cycle render modes\n"\
        "[DPAD]: Move the sphere up/down\n"\
        "[Y]: Toggle async compute";

    textMeasure = m_font->MeasureString(bottomText);
    textWidth = static_cast<int>(XMVectorGetX(textMeasure));
    textHeight = static_cast<int>(XMVectorGetY(textMeasure));

    m_hudBatch->Draw(
        whiteTextureHandle,
        { 1, 1 },
        RECT{ safe.left, safe.bottom - textHeight - boxPadding, safe.left + textWidth, safe.bottom + boxPadding });

    DX::DrawControllerString(m_hudBatch.get(), m_font.get(), m_ctrlFont.get(), bottomText, XMFLOAT2(float(safe.left), float(safe.bottom - textHeight)), DirectX::Colors::DarkKhaki);

    m_hudBatch->End();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    SetRenderTargetsAndViewports();
    // Use linear clear color for gamma-correct rendering.
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    PIXEndEvent(commandList);
}

void Sample::SetRenderTargetsAndViewports()
{
    auto commandList = m_deviceResources->GetCommandList();

    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
}

void Sample::UpdateConstants(ID3D12GraphicsCommandList* commandList, XMMATRIX view, XMMATRIX proj, Matrix world, float scale)
{
    XMMATRIX worldView = world * view;
    XMMATRIX worldViewProj = worldView * proj;

    CBMultiPerFrame constants;
    XMStoreFloat4x4(&constants.mWorldViewProj, XMMatrixTranspose(worldViewProj));
    XMStoreFloat4x4(&constants.mWorld, XMMatrixTranspose(world));
    XMStoreFloat4x4(&constants.mView, XMMatrixTranspose(view));
    XMStoreFloat4x4(&constants.mProj, XMMatrixTranspose(proj));
    XMStoreFloat3(&constants.vEye, m_camera.GetPosition());
    constants.scale = scale;

    auto cbMem = m_graphicsMemory->AllocateConstant<CBMultiPerFrame>(constants);
    commandList->SetGraphicsRootConstantBufferView(0, cbMem.GpuAddress());
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
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // Create the reflective sphere
    m_reflectiveSphere = GeometricPrimitive::CreateSphere(40.0f, 16, false, false, device);
    m_sphereMatrix = XMMatrixTranslation(-20.0f, 20.0f, 0);

    // Load models from disk.
    m_modelInfo.resize(_countof(c_modelPaths));
    for (size_t i = 0; i < m_modelInfo.size(); ++i)
    {
        m_modelInfo[i].m_model = Model::CreateFromSDKMESH(device, c_modelPaths[i]);

        wchar_t meshletFilepath[256] = {};
        swprintf_s(meshletFilepath, L"%s.bin", c_modelPaths[i]);

        m_modelInfo[i].m_meshlet = ATG::MeshletSet::Read(meshletFilepath);
    }

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Optimize meshes for rendering
    m_reflectiveSphere->LoadStaticBuffers(device, resourceUpload);

    for (size_t i = 0; i < m_modelInfo.size(); ++i)
    {
        m_modelInfo[i].m_model->LoadStaticBuffers(device, resourceUpload);
        for (auto& mesh : m_modelInfo[i].m_model->meshes)
        {
            std::vector<uint32_t> texCoordOffsets(mesh->opaqueMeshParts.size());
            for(size_t p = 0; p < mesh->opaqueMeshParts.size(); ++p)
            {
                auto& part = mesh->opaqueMeshParts[p];
                texCoordOffsets[p] = GetElementOffset("TEXCOORD", part->vbDecl);
                // transition the vertex buffers to non-pixel shader resource since they will be used as SRVs in mesh and vertex shaders
                resourceUpload.Transition(part->staticVertexBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            }
            m_modelInfo[i].m_texCoordOffsets.emplace_back(std::move(texCoordOffsets));
        }

        for (auto& m : m_modelInfo[i].m_meshlet)
        {
            m.CreateResources(device, resourceUpload);
        }
    }

    auto texOffsets = std::vector<uint32_t>(m_modelInfo.size());
    uint32_t totalDescriptors = 0;
    for (size_t i = 0; i < m_modelInfo.size(); ++i)
    {
        texOffsets[i] = totalDescriptors + DescriptorIndex::Count + c_mipLevelsPlusFakes - 1;
        totalDescriptors += static_cast<uint32_t>(m_modelInfo[i].m_model->textureNames.size());
    }

    // Create descriptor heaps.
    {
        m_srvHeap = std::make_unique<DescriptorHeap>(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            totalDescriptors + DescriptorIndex::Count + c_mipLevelsPlusFakes - 1); // model texture descriptors + cube map + fonts

        m_dsvHeap = std::make_unique<DescriptorHeap>(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            DSVIndex::Count);

        m_rtvHeap = std::make_unique<DescriptorHeap>(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            RTVIndex::Count); // 1 single RTV + 6 face RTVs
    }

    // Upload textures to GPU.
    m_textureFactory = std::make_unique<EffectTextureFactory>(device, resourceUpload, m_srvHeap->Heap());
    for (size_t i = 0; i < m_modelInfo.size(); ++i)
    {
        m_modelInfo[i].m_model->LoadTextures(*m_textureFactory, int(texOffsets[i]));
        m_modelInfo[i].m_descriptorOffset = texOffsets[i];
    }

    // Initialize the scene
    m_scene.resize(_countof(c_sceneDefinition));
    for (size_t i = 0; i < m_scene.size(); i++)
    {
        size_t index = c_sceneDefinition[i].modelIndex;

        assert(index < m_modelInfo.size());

        m_scene[i].m_world = c_sceneDefinition[i].world;
        m_scene[i].m_modelInfo = &m_modelInfo[index];
        m_scene[i].m_scale = c_sceneDefinition[i].scale;
    }

    // HUD
    auto const backBufferRts = RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto const spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
    m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    m_font = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_srvHeap->GetCpuHandle(DescriptorIndex::Font),
        m_srvHeap->GetGpuHandle(DescriptorIndex::Font));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_srvHeap->GetCpuHandle(DescriptorIndex::ControllerFont),
        m_srvHeap->GetGpuHandle(DescriptorIndex::ControllerFont));


    // Create scene root signature and pipeline state
    {
        // Create root signature.
        auto const vertexShaderBlob = DX::ReadData(L"SceneVertexShader.cso");

        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignatureScene.ReleaseAndGetAddressOf())));

        auto const pixelShaderBlob = DX::ReadData(L"ScenePixelShader.cso");

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = D3D12_INPUT_LAYOUT_DESC(); // use empty input layout
        psoDesc.pRootSignature = m_rootSignatureScene.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CommonStates::DepthDefault;
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateScene.ReleaseAndGetAddressOf())));

        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateGenerateCubeMapSingleView.ReleaseAndGetAddressOf())));
    }

    // Create sample cube map root signature and pipeline state
    {
        // Create root signature.
        auto const vertexShaderBlob = DX::ReadData(L"SampleCubeMapVertexShader.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignatureSampleCubeMap.ReleaseAndGetAddressOf())));

        auto const pixelShaderBlob = DX::ReadData(L"SampleCubeMapPixelShader.cso");

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = GeometricPrimitive::VertexType::InputLayout;
        psoDesc.pRootSignature = m_rootSignatureSampleCubeMap.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CommonStates::DepthDefault;
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateSampleCubeMap.ReleaseAndGetAddressOf())));
    }

    // Create generate cube map root signature and pipeline state
    {
        // Create root signature.
        auto const vertexShaderBlob = DX::ReadData(L"GenerateCubeMapVertexShader.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignatureGenerateCubeMap.ReleaseAndGetAddressOf())));

        auto const geometryShaderBlob = DX::ReadData(L"GenerateCubeMapGeometryShader.cso");
        auto const pixelShaderBlob = DX::ReadData(L"GenerateCubeMapPixelShader.cso");

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = GeometricPrimitive::VertexType::InputLayout;
        psoDesc.pRootSignature = m_rootSignatureGenerateCubeMap.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.GS = { geometryShaderBlob.data(), geometryShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CommonStates::DepthDefault;
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateGenerateCubeMap.ReleaseAndGetAddressOf())));

        auto const vertexShaderBlobInstanced = DX::ReadData(L"GenerateCubeMapInstancedVertexShader.cso");
        psoDesc.VS = { vertexShaderBlobInstanced.data(), vertexShaderBlobInstanced.size() };
        psoDesc.GS = D3D12_SHADER_BYTECODE(); // clear out the GS
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateGenerateCubeMapInstanced.ReleaseAndGetAddressOf())));
    }

#ifdef _GAMING_XBOX_SCARLETT
    // Create generate cube map root signature and pipeline state
    {
        // Create root signature.
        auto const amplificationShaderBlob = DX::ReadData(L"GenerateCubeMapAmplificationShader.cso");
        auto const meshShaderBlob = DX::ReadData(L"GenerateCubeMapMeshShader.cso");
        auto const pixelShaderBlob = DX::ReadData(L"GenerateCubeMapPixelShader.cso");

        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_rootSignatureGenerateCubeMap.Get();
        psoDesc.AS = { amplificationShaderBlob.data(), amplificationShaderBlob.size() };
        psoDesc.MS = { meshShaderBlob.data(), meshShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CommonStates::DepthDefault;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.SampleDesc = DefaultSampleDesc();

        auto meshStreamDesc = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

        D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
        streamDesc.SizeInBytes = sizeof(meshStreamDesc);
        streamDesc.pPipelineStateSubobjectStream = &meshStreamDesc;

        DX::ThrowIfFailed(device->CreatePipelineState(&streamDesc, IID_GRAPHICS_PPV_ARGS(m_pipelineStateGenerateCubeMapMeshShader.ReleaseAndGetAddressOf())));
    }
#endif

    // Create generate mips root signature and pipeline state
    {
        // Create root signature.
        auto const computeShaderBlob = DX::ReadData(L"GenerateMipsComputeShader.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignatureGenerateMips.ReleaseAndGetAddressOf())));

        // Describe and create the compute pipeline state object (PSO).
        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_rootSignatureGenerateMips.Get();
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateGenerateMips.ReleaseAndGetAddressOf())));
    }

    // Create resources
    {
        auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        // Create cube map depth
        D3D12_RESOURCE_DESC descTex = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT,                      // format
            c_envMapSize,                               // width
            c_envMapSize,                               // height
            c_cubeMapFaces,                             // arraySize
            1,                                          // mipLevels
            1, 0,                                       // sampleCount, sampleQuality
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);   // miscFlags
        D3D12_CLEAR_VALUE envMapDepthClearValue = CD3DX12_CLEAR_VALUE(descTex.Format, 1.0f, 0);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &descTex,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &envMapDepthClearValue,
            IID_GRAPHICS_PPV_ARGS(m_cubeMapDepth.ReleaseAndGetAddressOf())));

        DX::ThrowIfFailed(m_cubeMapDepth->SetName(L"Cube Map Depth"));

        // Create the depth stencil view for the entire cube
        D3D12_DEPTH_STENCIL_VIEW_DESC descDSV = {};
        descDSV.Format = descTex.Format;
        descDSV.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        descDSV.Texture2DArray.ArraySize = descTex.DepthOrArraySize;
        device->CreateDepthStencilView(m_cubeMapDepth.Get(), &descDSV, m_dsvHeap->GetCpuHandle(DSVIndex::Array));

        // Create the depth stencil view for single face rendering
        descDSV.Texture2DArray.ArraySize = 1;
        device->CreateDepthStencilView(m_cubeMapDepth.Get(), &descDSV, m_dsvHeap->GetCpuHandle(DSVIndex::Single));

        // Create the cube map for env map render target
        descTex.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        descTex.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        descTex.MipLevels = c_mipLevels;
        CD3DX12_CLEAR_VALUE envMapClearColor(descTex.Format, ATG::ColorsLinear::Background);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &descTex,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &envMapClearColor,
            IID_GRAPHICS_PPV_ARGS(m_cubeMap.ReleaseAndGetAddressOf())));
        DX::ThrowIfFailed(m_cubeMap->SetName(L"Environment Map"));

        // Create the 6-face render target view
        D3D12_RENDER_TARGET_VIEW_DESC descRTV = {};
        descRTV.Format = descTex.Format;
        descRTV.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        descRTV.Texture2DArray.ArraySize = c_cubeMapFaces;
        device->CreateRenderTargetView(m_cubeMap.Get(), &descRTV, m_rtvHeap->GetCpuHandle(RTVIndex::Array));

        // Create the one-face render target views
        descRTV.Texture2DArray.ArraySize = 1;
        for (size_t i = 0; i < c_cubeMapFaces; ++i)
        {
            descRTV.Texture2DArray.FirstArraySlice = static_cast<uint32_t>(i);
            device->CreateRenderTargetView(m_cubeMap.Get(), &descRTV, m_rtvHeap->GetCpuHandle(i + RTVIndex::Face0));
        }

        // Create the shader resource view for the cubic env map
        D3D12_SHADER_RESOURCE_VIEW_DESC cubeMapSrvDesc = {};
        cubeMapSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        cubeMapSrvDesc.Format = descTex.Format;
        cubeMapSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        cubeMapSrvDesc.TextureCube.MipLevels = c_mipLevels;
        device->CreateShaderResourceView(m_cubeMap.Get(), &cubeMapSrvDesc, m_srvHeap->GetCpuHandle(DescriptorIndex::CubeMap));

        cubeMapSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        cubeMapSrvDesc.Texture2DArray.MipLevels = c_mipLevels;
        cubeMapSrvDesc.Texture2DArray.ArraySize = c_cubeMapFaces;
        device->CreateShaderResourceView(m_cubeMap.Get(), &cubeMapSrvDesc, m_srvHeap->GetCpuHandle(DescriptorIndex::CubeMapArraySrv));

        // Create the UAVs for the cubic env map
        D3D12_UNORDERED_ACCESS_VIEW_DESC cubeMapUAVDesc = {};
        cubeMapUAVDesc.Format = descTex.Format;
        cubeMapUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        cubeMapUAVDesc.Texture2DArray.ArraySize = 6;
        uint32_t mip = 1;
        for (; mip < c_mipLevels; ++mip)
        {
            cubeMapUAVDesc.Texture2DArray.MipSlice = mip;
            device->CreateUnorderedAccessView(m_cubeMap.Get(), nullptr, &cubeMapUAVDesc, m_srvHeap->GetCpuHandle(static_cast<size_t>(DescriptorIndex::CubeMapUav) + mip - 1));
        }

        // Create psuedo descriptors for validated build
        for (uint32_t j = mip; j <= c_mipLevelsPlusFakes; ++j)
        {
            cubeMapUAVDesc.Texture2D.MipSlice = static_cast<UINT>(mip - 1);
            device->CreateUnorderedAccessView(m_cubeMap.Get(), nullptr, &cubeMapUAVDesc, m_srvHeap->GetCpuHandle(static_cast<size_t>(DescriptorIndex::CubeMapUav) + j - 1));
        }

        // Create single pixel texture for text underlay box
        auto const pixelDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &pixelDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_whiteTexture.ReleaseAndGetAddressOf())));

        device->CreateShaderResourceView(m_whiteTexture.Get(), nullptr, m_srvHeap->GetCpuHandle(DescriptorIndex::WhiteTexture));

        // Upload a single grey pixel to the underlay resource
        const uint8_t color[4] = { 5, 5, 5, 196 };

        D3D12_SUBRESOURCE_DATA data = {};
        data.pData = color;
        data.RowPitch = D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT;
        data.SlicePitch = 0;

        resourceUpload.Upload(m_whiteTexture.Get(), 0, &data, 1);
        resourceUpload.Transition(m_whiteTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
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
    m_camera.SetProjectionParameters(s_fovy, 1.0f, 10000.0f, true);

    m_camera.SetLookAt(Vector3(-70.0f, 150.0f, 240.0f), Vector3::Down * 15.0f);
    m_camera.SetSensitivity(500.0f, 100.0f, 1000.0f, 10.0f);

    D3D12_VIEWPORT hudViewport = { 0, 0, 1920, 1080 };
    m_hudBatch->SetViewport(hudViewport);
}
#pragma endregion
