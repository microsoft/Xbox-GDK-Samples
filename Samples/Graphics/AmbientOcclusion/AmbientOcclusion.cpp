//--------------------------------------------------------------------------------------
// AmbientOcclusion.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AmbientOcclusion.h"

#include "ATGColors.h"
#include "Constants.h"
#include "ControllerFont.h"
#include "FindMedia.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr float c_nearClipDistance = 1.0f;
    constexpr float c_farClipDistance = 10000.0f;
    constexpr float c_zClear = 0.0f;
    constexpr uint32_t c_hTileTileWidth = 8;
    constexpr uint32_t c_hTileTileHeight = 8;
    constexpr uint32_t c_depthTiledArraySize = 16;
    const float c_sampleThickness[12] =
    {
        sqrtf(1.0f - 0.2f * 0.2f),
        sqrtf(1.0f - 0.4f * 0.4f),
        sqrtf(1.0f - 0.6f * 0.6f),
        sqrtf(1.0f - 0.8f * 0.8f),
        sqrtf(1.0f - 0.2f * 0.2f - 0.2f * 0.2f),
        sqrtf(1.0f - 0.2f * 0.2f - 0.4f * 0.4f),
        sqrtf(1.0f - 0.2f * 0.2f - 0.6f * 0.6f),
        sqrtf(1.0f - 0.2f * 0.2f - 0.8f * 0.8f),
        sqrtf(1.0f - 0.4f * 0.4f - 0.4f * 0.4f),
        sqrtf(1.0f - 0.4f * 0.4f - 0.6f * 0.6f),
        sqrtf(1.0f - 0.4f * 0.4f - 0.8f * 0.8f),
        sqrtf(1.0f - 0.6f * 0.6f - 0.6f * 0.6f),
    };

    struct DescriptorIndex
    {
        enum
        {
            Font,
            ControllerFont,
            GrayTexture,
            HilbertSrv,
            AOSrv,
            DepthSrv,
            LinearDepthSrv,
            LinearDepthUav,
            DepthDownsize1Uav,
            DepthTiled1Uav,
            DepthDownsize2Uav,
            DepthTiled2Uav,
            DepthDownsize3Uav,
            DepthTiled3Uav,
            DepthDownsize4Uav,
            DepthTiled4Uav,
            AOUav,
            AoMerged1Uav,
            AoMerged2Uav,
            AoMerged3Uav,
            AoMerged4Uav,
            AoSmooth1Uav,
            AoSmooth2Uav,
            AoSmooth3Uav,
            AoHigh1Uav,
            AoHigh2Uav,
            AoHigh3Uav,
            AoHigh4Uav,
            AoMerged1Srv,
            AoMerged2Srv,
            AoMerged3Srv,
            AoMerged4Srv,
            AoSmooth1Srv,
            AoSmooth2Srv,
            AoSmooth3Srv,
            AoHigh1Srv,
            AoHigh2Srv,
            AoHigh3Srv,
            AoHigh4Srv,
            DepthTiled1Srv,
            DepthTiled2Srv,
            DepthTiled3Srv,
            DepthTiled4Srv,
            DepthDownsize1Srv,
            DepthDownsize2Srv,
            DepthDownsize3Srv,
            DepthDownsize4Srv,
            DepthOutputSrv,
            DepthOutputMip0Uav,
            DepthOutputMip1Uav,
            DepthOutputMip2Uav,
            DepthOutputMip3Uav,
            DepthOutputMip4Uav,
            SceneNormalsSrv,
            SceneNormalsUav,
            WorkingAoSrv,
            WorkingAoTempSrv,
            WorkingAoTempUav,
            WorkingEdgesSrv,
            WorkingAoUav,
            WorkingEdgesUav,
            Count,
            DownsampleUavRange1Start = LinearDepthUav,
            DownsampleUavRange2Start = DepthDownsize3Uav,
            DepthOutputUavRangeStart = DepthOutputMip0Uav,
            WorkingUavRange = WorkingAoUav
        };
    };

    struct RootSignatureLighting
    {
        enum
        {
            Constants,
            DiffuseTex,
            AOTex
        };
    };

    struct RootSignatureSSAO
    {
        enum
        {
            RootConstants,
            CBV,
            UAVs,
            T0,
            T1,
            T2,
            T3,
            T4,
            T5
        };
    };

    struct RootSignatureGTAO
    {
        enum
        {
            CBV,
            UAVs,
            T0,
            T1,
            T2
        };
    };

    struct Resolution
    {
        enum
        {
            Full,
            Res1,
            Res2,
            Res3,
            Res4,
            Res5,
            Res6,
            Count,
            Tiled1 = Res3,
            Tiled2 = Res4,
            Tiled3 = Res5,
            Tiled4 = Res6
        };
    };

    struct SSAOQuality
    {
        enum
        {
            VeryLow,
            Low,
            Medium,
            High,
            VeryHigh,
            Count
        };
    };

    struct GTAOQuality
    {
        enum
        {
            Low,
            Medium,
            High,
            Ultra,
            Count
        };
    };

    struct SelectedOption
    {
        enum
        {
            HierarchyDepth,
            SsaoQuality,
            NoiseFilterTolerance,
            BlurTolerance,
            UpsampleTolerance,
            RejectionFalloff,
            Accentuation,
            SampleExhaustively,
            Count,
            GtaoQuality = HierarchyDepth,
            DenoisePasses,
            Radius,
            GtaoCount
        };
    };

    struct AoImplementation
    {
        enum
        {
            SSAO,
            GTAO,
            Count
        };
    };

    struct BufferRes
    {
        uint32_t width;
        uint32_t height;
    } s_bufferResolutions[Resolution::Count];

    void DrawModelManually(ID3D12GraphicsCommandList* commandList, const DirectX::Model* model, bool setTextures, const DirectX::DescriptorHeap* heap, size_t descriptorSize)
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, model->name.c_str());
        for (const auto& mesh : model->meshes)
        {
            assert(mesh.get() != nullptr);
            for (const auto& part : mesh->opaqueMeshParts)
            {
                assert(part.get() != nullptr);
                if (setTextures)
                {
                    auto textureDescriptor = model->GetGpuTextureHandleForMaterialIndex(part->materialIndex, heap->Heap(), descriptorSize, DescriptorIndex::Count);
                    commandList->SetGraphicsRootDescriptorTable(RootSignatureLighting::DiffuseTex, textureDescriptor);
                    commandList->SetGraphicsRootDescriptorTable(RootSignatureLighting::AOTex, heap->GetGpuHandle(DescriptorIndex::AOSrv));
                }
                D3D12_INDEX_BUFFER_VIEW ibv;
                ibv.BufferLocation = part->staticIndexBuffer ? part->staticIndexBuffer->GetGPUVirtualAddress() : part->indexBuffer.GpuAddress();
                ibv.SizeInBytes = part->indexBufferSize;
                ibv.Format = part->indexFormat;
                D3D12_VERTEX_BUFFER_VIEW vbv;
                vbv.BufferLocation = part->staticVertexBuffer ? part->staticVertexBuffer->GetGPUVirtualAddress() : part->vertexBuffer.GpuAddress();
                vbv.StrideInBytes = part->vertexStride;
                vbv.SizeInBytes = part->vertexBufferSize;
                commandList->IASetVertexBuffers(0, 1, &vbv);
                commandList->IASetIndexBuffer(&ibv);
                commandList->IASetPrimitiveTopology(part->primitiveType);
                commandList->DrawIndexedInstanced(part->indexCount, 1, part->startIndex, part->vertexOffset, 0);
            }
        }
    }

    uint32_t CalcDispatchSSAO(uint32_t resolution, uint32_t threadGroup)
    {
        return AlignUp(resolution * threadGroup, threadGroup) / threadGroup;
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_hTileInfo(0),
    m_selectedOption(SelectedOption::HierarchyDepth),
    m_hierarchyDepth(3),
    m_ssaoQuality(SSAOQuality::High),
    m_noiseFilterTolerance(-3.0f),
    m_blurTolerance(-5.0f),
    m_upsampleTolerance(-7.0f),
    m_rejectionFalloff(2.5f),
    m_accentuation(0.1f),
    m_showAOTexture(true),
    m_applyAO(true),
    m_useFp16(false),
    m_sampleExhaustively(false),
    m_aoImplementation(AoImplementation::GTAO),
    m_settings()
{
    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        DXGI_FORMAT_D32_FLOAT,
        2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD | DX::DeviceResources::c_ReverseDepth);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

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

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);
        m_camera.Update(elapsedTime, pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
        {
            if (m_aoImplementation == AoImplementation::SSAO)
            {
                switch (m_selectedOption)
                {
                case SelectedOption::HierarchyDepth:
                    m_hierarchyDepth = ((m_hierarchyDepth == 4) ? 1 : m_hierarchyDepth + 1);
                    break;
                case SelectedOption::SsaoQuality:
                    m_ssaoQuality = ((m_ssaoQuality == SSAOQuality::Count - 1) ? 0 : m_ssaoQuality + 1);
                    break;
                case SelectedOption::NoiseFilterTolerance:
                    m_noiseFilterTolerance = std::min(m_noiseFilterTolerance + 0.25f, 0.0f);
                    break;
                case SelectedOption::BlurTolerance:
                    m_blurTolerance = std::min(m_blurTolerance + 0.25f, -1.0f);
                    break;
                case SelectedOption::UpsampleTolerance:
                    m_upsampleTolerance = std::min(m_upsampleTolerance + 0.5f, -1.0f);
                    break;
                case SelectedOption::RejectionFalloff:
                    m_rejectionFalloff = std::min(m_rejectionFalloff + 0.5f, 10.0f);
                    break;
                case SelectedOption::Accentuation:
                    m_accentuation = std::min(m_accentuation + 0.1f, 1.0f);
                    break;
                case SelectedOption::SampleExhaustively:
                    m_sampleExhaustively = !m_sampleExhaustively;
                    break;
                }
            }
            else
            {
                switch (m_selectedOption)
                {
                case SelectedOption::GtaoQuality:
                    m_settings.QualityLevel = ((m_settings.QualityLevel == GTAOQuality::Count - 1) ? 0 : m_settings.QualityLevel + 1);
                    break;
                case SelectedOption::DenoisePasses:
                    m_settings.DenoisePasses = ((m_settings.DenoisePasses == 3) ? 0 : m_settings.DenoisePasses + 1);
                    break;
                case SelectedOption::Radius:
                    m_settings.Radius = std::min(m_settings.Radius + 0.5f, 200.0f);
                    break;
                }
            }
        }

        if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
        {
            if (m_aoImplementation == AoImplementation::SSAO)
            {
                switch (m_selectedOption)
                {
                case SelectedOption::HierarchyDepth:
                    m_hierarchyDepth = ((m_hierarchyDepth == 1) ? 4 : m_hierarchyDepth - 1);
                    break;
                case SelectedOption::SsaoQuality:
                    m_ssaoQuality = ((m_ssaoQuality == 0) ? SSAOQuality::Count - 1 : m_ssaoQuality - 1);
                    break;
                case SelectedOption::NoiseFilterTolerance:
                    m_noiseFilterTolerance = std::max(m_noiseFilterTolerance - 0.25f, -8.0f);
                    break;
                case SelectedOption::BlurTolerance:
                    m_blurTolerance = std::max(m_blurTolerance - 0.25f, -8.0f);
                    break;
                case SelectedOption::UpsampleTolerance:
                    m_upsampleTolerance = std::max(m_upsampleTolerance - 0.5f, -12.0f);
                    break;
                case SelectedOption::RejectionFalloff:
                    m_rejectionFalloff = std::max(m_rejectionFalloff - 0.5f, 1.0f);
                    break;
                case SelectedOption::Accentuation:
                    m_accentuation = std::max(m_accentuation - 0.1f, 0.0f);
                    break;
                case SelectedOption::SampleExhaustively:
                    m_sampleExhaustively = !m_sampleExhaustively;
                    break;
                }
            }
            else
            {
                switch (m_selectedOption)
                {
                case SelectedOption::GtaoQuality:
                    m_settings.QualityLevel = ((m_settings.QualityLevel == 0) ? GTAOQuality::Count - 1 : m_settings.QualityLevel - 1);
                    break;
                case SelectedOption::DenoisePasses:
                    m_settings.DenoisePasses = ((m_settings.DenoisePasses == 0) ? 3 : m_settings.DenoisePasses - 1);
                    break;
                case SelectedOption::Radius:
                    m_settings.Radius = std::max(m_settings.Radius - 0.5f, 0.0f);
                    break;
                }
            }
        }

        uint32_t lastOption = m_aoImplementation == AoImplementation::SSAO ? SelectedOption::Count : SelectedOption::GtaoCount;
        if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
        {
            m_selectedOption = ((m_selectedOption == 0) ? lastOption - 1 : m_selectedOption - 1);
        }

        if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
        {
            m_selectedOption = ((m_selectedOption == lastOption - 1) ? 0 : m_selectedOption + 1);
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_showAOTexture = !m_showAOTexture;
        }

        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_aoImplementation = ((m_aoImplementation == AoImplementation::Count - 1) ? 0 : m_aoImplementation + 1);
            m_selectedOption = 0;
        }

        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            m_applyAO = !m_applyAO;
        }

#ifdef _GAMING_XBOX_SCARLETT
        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            m_useFp16 = !m_useFp16;
        }
#endif
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
    uint32_t frameCount = m_timer.GetFrameCount();
    if (frameCount == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    m_gpuTimer->BeginFrame(commandList);
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    XMMATRIX worldViewProj = m_camera.GetView() * m_camera.GetProjection();

    Constants constants;
    XMStoreFloat4x4(&constants.WorldViewProj, XMMatrixTranspose(worldViewProj));
    XMStoreFloat4x4(&constants.WorldInverseTranspose, XMMatrixIdentity());
    constants.aoMultiplier = m_applyAO ? 1.0f : 0.0f;
    auto const sceneCbMem = m_graphicsMemory->AllocateConstant<Constants>(constants);
    

    // Z Prepass
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Depth Pre-Pass");
        auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();
        commandList->OMSetRenderTargets(0, nullptr, false, &dsvDescriptor);

        commandList->SetGraphicsRootSignature(m_rootSignatureZpp.Get());
        commandList->SetPipelineState(m_pipelineStateZpp.Get());
        commandList->SetGraphicsRootConstantBufferView(RootSignatureLighting::Constants, sceneCbMem.GpuAddress());

        DrawModelManually(commandList, m_model.get(), false, m_srvHeap.get(), m_srvHeap->Increment());
    }

    // Compute AO
    m_gpuTimer->Start(commandList, 0);
    if (m_aoImplementation == AoImplementation::SSAO)
    {
        RenderSSAO(frameCount);
    }
    else // GTAO
    {
        RenderGTAO();
    }
    m_gpuTimer->Stop(commandList, 0);

    if (m_showAOTexture)
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"AO Display Pass");
        auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
        commandList->OMSetRenderTargets(1, &rtvDescriptor, false, nullptr);

        commandList->SetGraphicsRootSignature(m_rootSignatureAoTex.Get());
        commandList->SetPipelineState(m_pipelineStateAoTex.Get());
        commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGpuHandle(DescriptorIndex::AOSrv));
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        commandList->DrawInstanced(4, 1, 0, 0);
    }
    else // Lighting Pass
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Lighting Pass");
        auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
        auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();
        commandList->OMSetRenderTargets(1, &rtvDescriptor, false, &dsvDescriptor);

        commandList->SetGraphicsRootSignature(m_rootSignatureLighting.Get());
        commandList->SetPipelineState(m_pipelineStateLighting.Get());
        commandList->SetGraphicsRootConstantBufferView(RootSignatureLighting::Constants, sceneCbMem.GpuAddress());

        DrawModelManually(commandList, m_model.get(), true, m_srvHeap.get(), m_srvHeap->Increment());
    }

    // Transition depth back to write for the next frame
    {
        D3D12_RESOURCE_STATES beforeState = m_aoImplementation == AoImplementation::SSAO ? D3D12XBOX_RESOURCE_STATE_PRESERVE_COMPRESSED_DEPTH_STENCIL | D3D12_RESOURCE_STATE_DEPTH_READ : D3D12_RESOURCE_STATE_DEPTH_READ;
        D3D12_RESOURCE_BARRIER barrier =
            CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(), beforeState, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        commandList->ResourceBarrier(1, &barrier);
    }


    RenderHUD();

    PIXEndEvent(commandList);
    m_gpuTimer->EndFrame(commandList);

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

    commandList->OMSetRenderTargets(1, &rtvDescriptor, false, &dsvDescriptor);
    // Use linear clear color for gamma-correct rendering.
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, c_zClear, 0, 0, nullptr);

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
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());

    m_model = Model::CreateFromSDKMESH(device, L"AbstractCathedral.sdkmesh");
    uint32_t modelDescriptors = static_cast<uint32_t>(m_model->textureNames.size());

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Create descriptor heap.
    m_srvHeap = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        modelDescriptors + DescriptorIndex::Count);

    // Upload textures to GPU.
    m_textureFactory = std::make_unique<EffectTextureFactory>(device, resourceUpload, m_srvHeap->Heap());
    m_model->LoadTextures(*m_textureFactory, DescriptorIndex::Count);

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

    // Create scene root signature and pipeline state for zpp
    {
        auto const vertexShaderBlob = DX::ReadData(L"ZppVS.cso");

        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignatureZpp.ReleaseAndGetAddressOf())));

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = DX12::VertexPositionNormalTexture::InputLayout;
        psoDesc.pRootSignature = m_rootSignatureZpp.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CommonStates::DepthReverseZ;
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 0;
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateZpp.ReleaseAndGetAddressOf())));
    }

    // Create scene root signature and pipeline state for lighting
    {
        auto const vertexShaderBlob = DX::ReadData(L"LightingVS.cso");

        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignatureLighting.ReleaseAndGetAddressOf())));

        auto const pixelShaderBlob = DX::ReadData(L"LightingPS.cso");

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = DX12::VertexPositionNormalTexture::InputLayout;
        psoDesc.pRootSignature = m_rootSignatureLighting.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CommonStates::DepthRead;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateLighting.ReleaseAndGetAddressOf())));
    }

    // Create Ao texture root signature and pipeline state
    {
        auto const vertexShaderBlob = DX::ReadData(L"AoTexVS.cso");

        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignatureAoTex.ReleaseAndGetAddressOf())));
        m_rootSignatureAoTex->SetName(L"AO Tex RootSig");

        auto const pixelShaderBlob = DX::ReadData(L"AoTexPS.cso");

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = D3D12_INPUT_LAYOUT_DESC(); // use empty input layout
        psoDesc.pRootSignature = m_rootSignatureAoTex.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CommonStates::DepthNone;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateAoTex.ReleaseAndGetAddressOf())));
        m_pipelineStateAoTex->SetName(L"AoTexPSO");
    }

    // Create SSAO root signature and pipeline states
    {
        auto computeShaderBlob = DX::ReadData(L"AoPrepareDepthBuffers1CS.cso");

        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignatureSSAO.ReleaseAndGetAddressOf())));
        m_rootSignatureSSAO->SetName(L"SSAO RootSig");

        // Describe and create the graphics pipeline state objects (PSOs).
        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_rootSignatureSSAO.Get();
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStatePrepareDepthBuffers1.ReleaseAndGetAddressOf())));
        m_pipelineStatePrepareDepthBuffers1->SetName(L"AoPrepareDepthBuffers1CS");

        computeShaderBlob = DX::ReadData(L"AoPrepareDepthBuffers2CS.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStatePrepareDepthBuffers2.ReleaseAndGetAddressOf())));
        m_pipelineStatePrepareDepthBuffers2->SetName(L"AoPrepareDepthBuffers2CS");

        computeShaderBlob = DX::ReadData(L"AoBlurUpsampleBlendOutCS.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateBlurAndUpsample.ReleaseAndGetAddressOf())));
        m_pipelineStateBlurAndUpsample->SetName(L"AoBlurUpsampleBlendOutCS");

        computeShaderBlob = DX::ReadData(L"AoBlurUpsamplePreMinBlendOutCS.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateBlurAndUpsampleHigh.ReleaseAndGetAddressOf())));
        m_pipelineStateBlurAndUpsampleHigh->SetName(L"AoBlurUpsamplePreMinBlendOutCS");

        computeShaderBlob = DX::ReadData(L"AoBlurUpsampleCS.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateBlurAndUpsampleFinal[PipelineStates::Fp32].ReleaseAndGetAddressOf())));
        m_pipelineStateBlurAndUpsampleFinal[PipelineStates::Fp32]->SetName(L"AoBlurUpsampleCS");

        computeShaderBlob = DX::ReadData(L"AoBlurUpsamplePreMinCS.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateBlurAndUpsampleFinalHigh[PipelineStates::Fp32].ReleaseAndGetAddressOf())));
        m_pipelineStateBlurAndUpsampleFinalHigh[PipelineStates::Fp32]->SetName(L"AoBlurUpsamplePreMinCS");

        computeShaderBlob = DX::ReadData(L"AoRender1CS.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateAoRender1[PipelineStates::Fp32].ReleaseAndGetAddressOf())));
        m_pipelineStateAoRender1[PipelineStates::Fp32]->SetName(L"AoRender1CS");

        computeShaderBlob = DX::ReadData(L"AoRender2CS.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateAoRender2[PipelineStates::Fp32].ReleaseAndGetAddressOf())));
        m_pipelineStateAoRender2[PipelineStates::Fp32]->SetName(L"AoRender2CS");

        computeShaderBlob = DX::ReadData(L"AoRender1ExhaustivelyCS.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateAoRender1SampleExhaustively.ReleaseAndGetAddressOf())));
        m_pipelineStateAoRender1SampleExhaustively->SetName(L"AoRender1ExhaustivelyCS");

        computeShaderBlob = DX::ReadData(L"AoRender2ExhaustivelyCS.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateAoRender2SampleExhaustively.ReleaseAndGetAddressOf())));
        m_pipelineStateAoRender2SampleExhaustively->SetName(L"AoRender2ExhaustivelyCS");

#ifdef _GAMING_XBOX_SCARLETT
        computeShaderBlob = DX::ReadData(L"AoRender1CSFP16.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateAoRender1[PipelineStates::Fp16].ReleaseAndGetAddressOf())));
        m_pipelineStateAoRender1[PipelineStates::Fp16]->SetName(L"AoRender1CSFP16");

        computeShaderBlob = DX::ReadData(L"AoRender2CSFP16.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateAoRender2[PipelineStates::Fp16].ReleaseAndGetAddressOf())));
        m_pipelineStateAoRender2[PipelineStates::Fp16]->SetName(L"AoRender2CSFP16");

        computeShaderBlob = DX::ReadData(L"AoBlurUpsampleCSFP16.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateBlurAndUpsampleFinal[PipelineStates::Fp16].ReleaseAndGetAddressOf())));
        m_pipelineStateBlurAndUpsampleFinal[PipelineStates::Fp16]->SetName(L"AoBlurUpsampleCSFP16");

        computeShaderBlob = DX::ReadData(L"AoBlurUpsamplePreMinCSFP16.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateBlurAndUpsampleFinalHigh[PipelineStates::Fp16].ReleaseAndGetAddressOf())));
        m_pipelineStateBlurAndUpsampleFinalHigh[PipelineStates::Fp16]->SetName(L"AoBlurUpsamplePreMinCSFP16");
#endif
    }

    // Create GTAO root signature and pipeline states
    {
        auto computeShaderBlob = DX::ReadData(L"CSPrefilterDepths16x16.cso");

        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignatureGTAO.ReleaseAndGetAddressOf())));
        m_rootSignatureGTAO->SetName(L"GTAO RootSig");

        // Describe and create the graphics pipeline state objects (PSOs).
        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_rootSignatureGTAO.Get();
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStatePrefilterDepths.ReleaseAndGetAddressOf())));
        m_pipelineStatePrefilterDepths->SetName(L"CSPrefilterDepths16x16");

        computeShaderBlob = DX::ReadData(L"CSGTAOLow.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateGTAOLow[PipelineStates::Fp32].ReleaseAndGetAddressOf())));
        m_pipelineStateGTAOLow[PipelineStates::Fp32]->SetName(L"CSGTAOLow");

        computeShaderBlob = DX::ReadData(L"CSGTAOMedium.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateGTAOMedium[PipelineStates::Fp32].ReleaseAndGetAddressOf())));
        m_pipelineStateGTAOMedium[PipelineStates::Fp32]->SetName(L"CSGTAOMedium");

        computeShaderBlob = DX::ReadData(L"CSGTAOHigh.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateGTAOHigh[PipelineStates::Fp32].ReleaseAndGetAddressOf())));
        m_pipelineStateGTAOHigh[PipelineStates::Fp32]->SetName(L"CSGTAOHigh");

        computeShaderBlob = DX::ReadData(L"CSGTAOUltra.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateGTAOUltra[PipelineStates::Fp32].ReleaseAndGetAddressOf())));
        m_pipelineStateGTAOUltra[PipelineStates::Fp32]->SetName(L"CSGTAOUltra");

        computeShaderBlob = DX::ReadData(L"CSDenoisePass.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateDenoisePass.ReleaseAndGetAddressOf())));
        m_pipelineStateDenoisePass->SetName(L"CSDenoisePass");

        computeShaderBlob = DX::ReadData(L"CSDenoiseLastPass.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateDenoiseLastPass.ReleaseAndGetAddressOf())));
        m_pipelineStateDenoiseLastPass->SetName(L"CSDenoiseLastPass");

        computeShaderBlob = DX::ReadData(L"CSGenerateNormals.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateGenerateNormals.ReleaseAndGetAddressOf())));
        m_pipelineStateGenerateNormals->SetName(L"CSGenerateNormals");

#ifdef _GAMING_XBOX_SCARLETT
        computeShaderBlob = DX::ReadData(L"CSGTAOLowFP16.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateGTAOLow[PipelineStates::Fp16].ReleaseAndGetAddressOf())));
        m_pipelineStateGTAOLow[PipelineStates::Fp16]->SetName(L"CSGTAOLowFP16");

        computeShaderBlob = DX::ReadData(L"CSGTAOMediumFP16.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateGTAOMedium[PipelineStates::Fp16].ReleaseAndGetAddressOf())));
        m_pipelineStateGTAOMedium[PipelineStates::Fp16]->SetName(L"CSGTAOMediumFP16");

        computeShaderBlob = DX::ReadData(L"CSGTAOHighFP16.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateGTAOHigh[PipelineStates::Fp16].ReleaseAndGetAddressOf())));
        m_pipelineStateGTAOHigh[PipelineStates::Fp16]->SetName(L"CSGTAOHighFP16");

        computeShaderBlob = DX::ReadData(L"CSGTAOUltraFP16.cso");
        psoDesc.CS = { computeShaderBlob.data(), computeShaderBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_pipelineStateGTAOUltra[PipelineStates::Fp16].ReleaseAndGetAddressOf())));
        m_pipelineStateGTAOUltra[PipelineStates::Fp16]->SetName(L"CSGTAOUltraFP16");
#endif
    }

    auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    {
        // Create single pixel texture for text underlay box
        auto const pixelDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &pixelDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_grayTexture.ReleaseAndGetAddressOf())));
        m_grayTexture->SetName(L"Gray Texture");

        device->CreateShaderResourceView(m_grayTexture.Get(), nullptr, m_srvHeap->GetCpuHandle(DescriptorIndex::GrayTexture));

        // Upload a single grey pixel to the underlay resource
        const uint8_t color[4] = { 5, 5, 5, 196 };

        D3D12_SUBRESOURCE_DATA data = {};
        data.pData = color;
        data.RowPitch = D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT;
        data.SlicePitch = 0;

        resourceUpload.Upload(m_grayTexture.Get(), 0, &data, 1);
        resourceUpload.Transition(m_grayTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    {
        // Create a 64x64 Hilbert LUT
        auto const pixelDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16_UINT, 64, 64);

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &pixelDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_hilbertLUT.ReleaseAndGetAddressOf())));
        m_hilbertLUT->SetName(L"Hilbert LUT");

        device->CreateShaderResourceView(m_hilbertLUT.Get(), nullptr, m_srvHeap->GetCpuHandle(DescriptorIndex::HilbertSrv));

        uint16_t* lutData = new uint16_t[64 * 64];
        XeGTAO::GenerateHilbertTexture(&lutData);

        D3D12_SUBRESOURCE_DATA data = {};
        data.pData = lutData;
        data.RowPitch = 64 * sizeof(uint16_t);
        data.SlicePitch = 0;

        resourceUpload.Upload(m_hilbertLUT.Get(), 0, &data, 1);
        resourceUpload.Transition(m_hilbertLUT.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        delete[] lutData;
    }

    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    finished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const size = m_deviceResources->GetOutputSize();
    auto displayWidth = static_cast<uint32_t>(size.right - size.left);
    auto displayHeight = static_cast<uint32_t>(size.bottom - size.top);

    s_bufferResolutions[Resolution::Full] = { displayWidth, displayHeight };
    s_bufferResolutions[Resolution::Res1] = { (displayWidth + 1) / 2, (displayHeight + 1) / 2 };
    s_bufferResolutions[Resolution::Res2] = { (displayWidth + 3) / 4, (displayHeight + 3) / 4 };
    s_bufferResolutions[Resolution::Res3] = { (displayWidth + 7) / 8, (displayHeight + 7) / 8 };
    s_bufferResolutions[Resolution::Res4] = { (displayWidth + 15) / 16, (displayHeight + 15) / 16 };
    s_bufferResolutions[Resolution::Res5] = { (displayWidth + 31) / 32, (displayHeight + 31) / 32 };
    s_bufferResolutions[Resolution::Res6] = { (displayWidth + 63) / 64, (displayHeight + 63) / 64 };

    // Create resources
    {
        auto device = m_deviceResources->GetD3DDevice();
        auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        m_depthDownsize1 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R32_FLOAT,
            s_bufferResolutions[Resolution::Res1].width,
            s_bufferResolutions[Resolution::Res1].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"Depth Downsize 1",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthDownsize1Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthDownsize1Uav));

        m_depthDownsize2 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R32_FLOAT,
            s_bufferResolutions[Resolution::Res2].width,
            s_bufferResolutions[Resolution::Res2].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"Depth Downsize 2",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthDownsize2Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthDownsize2Uav));

        m_depthDownsize3 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R32_FLOAT,
            s_bufferResolutions[Resolution::Res3].width,
            s_bufferResolutions[Resolution::Res3].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"Depth Downsize 3",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthDownsize3Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthDownsize3Uav));

        m_depthDownsize4 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R32_FLOAT,
            s_bufferResolutions[Resolution::Res4].width,
            s_bufferResolutions[Resolution::Res4].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"Depth Downsize 4",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthDownsize4Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthDownsize4Uav));

        m_depthTiled1 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R16_FLOAT,
            s_bufferResolutions[Resolution::Tiled1].width,
            s_bufferResolutions[Resolution::Tiled1].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"Depth De-Interleaved 1",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthTiled1Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthTiled1Uav),
            c_depthTiledArraySize);

        m_depthTiled2 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R16_FLOAT,
            s_bufferResolutions[Resolution::Tiled2].width,
            s_bufferResolutions[Resolution::Tiled2].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"Depth De-Interleaved 2",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthTiled2Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthTiled2Uav),
            c_depthTiledArraySize);

        m_depthTiled3 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R16_FLOAT,
            s_bufferResolutions[Resolution::Tiled3].width,
            s_bufferResolutions[Resolution::Tiled3].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"Depth De-Interleaved 3",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthTiled3Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthTiled3Uav),
            c_depthTiledArraySize);

        m_depthTiled4 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R16_FLOAT,
            s_bufferResolutions[Resolution::Tiled4].width,
            s_bufferResolutions[Resolution::Tiled4].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"Depth De-Interleaved 4",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthTiled4Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::DepthTiled4Uav),
            c_depthTiledArraySize);

        m_aoMerged1 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R8_UNORM,
            s_bufferResolutions[Resolution::Res1].width,
            s_bufferResolutions[Resolution::Res1].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"AO Re-Interleaved 1",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoMerged1Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoMerged1Uav));

        m_aoMerged2 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R8_UNORM,
            s_bufferResolutions[Resolution::Res2].width,
            s_bufferResolutions[Resolution::Res2].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"AO Re-Interleaved 2",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoMerged2Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoMerged2Uav));

        m_aoMerged3 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R8_UNORM,
            s_bufferResolutions[Resolution::Res3].width,
            s_bufferResolutions[Resolution::Res3].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"AO Re-Interleaved 3",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoMerged3Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoMerged3Uav));

        m_aoMerged4 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R8_UNORM,
            s_bufferResolutions[Resolution::Res4].width,
            s_bufferResolutions[Resolution::Res4].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"AO Re-Interleaved 4",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoMerged4Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoMerged4Uav));

        m_aoSmooth1 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R8_UNORM,
            s_bufferResolutions[Resolution::Res1].width,
            s_bufferResolutions[Resolution::Res1].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"AO Smoothed 1",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoSmooth1Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoSmooth1Uav));

        m_aoSmooth2 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R8_UNORM,
            s_bufferResolutions[Resolution::Res2].width,
            s_bufferResolutions[Resolution::Res2].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"AO Smoothed 2",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoSmooth2Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoSmooth2Uav));

        m_aoSmooth3 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R8_UNORM,
            s_bufferResolutions[Resolution::Res3].width,
            s_bufferResolutions[Resolution::Res3].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"AO Smoothed 3",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoSmooth3Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoSmooth3Uav));

        m_aoHigh1 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R8_UNORM,
            s_bufferResolutions[Resolution::Res1].width,
            s_bufferResolutions[Resolution::Res1].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"AO High Quality 1",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoHigh1Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoHigh1Uav));

        m_aoHigh2 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R8_UNORM,
            s_bufferResolutions[Resolution::Res2].width,
            s_bufferResolutions[Resolution::Res2].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"AO High Quality 2",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoHigh2Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoHigh2Uav));

        m_aoHigh3 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R8_UNORM,
            s_bufferResolutions[Resolution::Res3].width,
            s_bufferResolutions[Resolution::Res3].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"AO High Quality 3",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoHigh3Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoHigh3Uav));

        m_aoHigh4 = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R8_UNORM,
            s_bufferResolutions[Resolution::Res4].width,
            s_bufferResolutions[Resolution::Res4].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"AO High Quality 4",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoHigh4Srv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::AoHigh4Uav));

        m_aoFullRes = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R8_UNORM,
            s_bufferResolutions[Resolution::Full].width,
            s_bufferResolutions[Resolution::Full].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"SSAO Full Res",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::AOSrv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::AOUav));

        m_linearDepth = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R16_UNORM,
            s_bufferResolutions[Resolution::Full].width,
            s_bufferResolutions[Resolution::Full].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"Linear Depth",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::LinearDepthSrv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::LinearDepthUav));

        m_normals = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R11G11B10_FLOAT,
            s_bufferResolutions[Resolution::Full].width,
            s_bufferResolutions[Resolution::Full].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"Scene Normals",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::SceneNormalsSrv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::SceneNormalsUav));

        m_workingAO = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R8_UNORM,
            s_bufferResolutions[Resolution::Full].width,
            s_bufferResolutions[Resolution::Full].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"Working AO Term",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::WorkingAoSrv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::WorkingAoUav));

        m_workingAOTemp = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R8_UNORM,
            s_bufferResolutions[Resolution::Full].width,
            s_bufferResolutions[Resolution::Full].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"Working AO Temp",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::WorkingAoTempSrv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::WorkingAoTempUav));

        m_workingEdges = std::make_unique<DX::Texture>(
            device,
            DXGI_FORMAT_R8_UINT,
            s_bufferResolutions[Resolution::Full].width,
            s_bufferResolutions[Resolution::Full].height,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            L"Working Edges",
            defaultHeap,
            m_srvHeap->GetCpuHandle(DescriptorIndex::WorkingEdgesSrv),
            m_srvHeap->GetCpuHandle(DescriptorIndex::WorkingEdgesUav));

        {
            D3D12_RESOURCE_DESC descTex = CD3DX12_RESOURCE_DESC::Tex2D(
                DXGI_FORMAT_R16_FLOAT,
                s_bufferResolutions[Resolution::Full].width,
                s_bufferResolutions[Resolution::Full].height,
                1,
                5,
                1, 0,
                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

            DX::ThrowIfFailed(device->CreateCommittedResource(
                &defaultHeap,
                D3D12_HEAP_FLAG_NONE,
                &descTex,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_depthOutput.ReleaseAndGetAddressOf())));

            DX::ThrowIfFailed(m_depthOutput->SetName(L"Depth Output"));

            CreateShaderResourceView(device, m_depthOutput.Get(), m_srvHeap->GetCpuHandle(DescriptorIndex::DepthOutputSrv));

            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = descTex.Format;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

            uavDesc.Texture2D.MipSlice = 0;
            device->CreateUnorderedAccessView(m_depthOutput.Get(), nullptr, &uavDesc, m_srvHeap->GetCpuHandle(DescriptorIndex::DepthOutputMip0Uav));
            uavDesc.Texture2D.MipSlice = 1;
            device->CreateUnorderedAccessView(m_depthOutput.Get(), nullptr, &uavDesc, m_srvHeap->GetCpuHandle(DescriptorIndex::DepthOutputMip1Uav));
            uavDesc.Texture2D.MipSlice = 2;
            device->CreateUnorderedAccessView(m_depthOutput.Get(), nullptr, &uavDesc, m_srvHeap->GetCpuHandle(DescriptorIndex::DepthOutputMip2Uav));
            uavDesc.Texture2D.MipSlice = 3;
            device->CreateUnorderedAccessView(m_depthOutput.Get(), nullptr, &uavDesc, m_srvHeap->GetCpuHandle(DescriptorIndex::DepthOutputMip3Uav));
            uavDesc.Texture2D.MipSlice = 4;
            device->CreateUnorderedAccessView(m_depthOutput.Get(), nullptr, &uavDesc, m_srvHeap->GetCpuHandle(DescriptorIndex::DepthOutputMip4Uav));
        }


        // Create placed resources for the depth and htile planes of the depth buffer
        D3D12_GPU_VIRTUAL_ADDRESS HTile = 0;
        D3D12_GPU_VIRTUAL_ADDRESS DepthSamples = 0;
        uint32_t paddedWidthElements = 0;
        uint32_t paddedHeightElements = 0;
        uint32_t htileAlignmentInBytes = 0;
        uint32_t htileSizeBytes = 0;
        void* depthTextureAddress = m_deviceResources->GetDepthTexAddress();
        auto const depthBufferLayout = m_deviceResources->GetDepthLayout();
        for (const auto& plane : depthBufferLayout.Plane)
        {
            switch (plane.Usage)
            {
            case XG_PLANE_USAGE_HTILE:
                HTile = reinterpret_cast<UINT64&>(depthTextureAddress) + plane.BaseOffsetBytes;
                htileAlignmentInBytes = static_cast<uint32_t>(plane.BaseAlignmentBytes);
                htileSizeBytes = static_cast<uint32_t>(plane.SizeBytes);
                break;

            case XG_PLANE_USAGE_DEPTH:
                DepthSamples = reinterpret_cast<UINT64&>(depthTextureAddress) + plane.BaseOffsetBytes;
                paddedWidthElements = plane.MipLayout[0].PaddedWidthElements;
                paddedHeightElements = plane.MipLayout[0].PaddedHeightElements;
                break;

            case XG_PLANE_USAGE_STENCIL:
            case XG_PLANE_USAGE_UNUSED:
            case XG_PLANE_USAGE_DEFAULT:
            case XG_PLANE_USAGE_COLOR_MASK:
            case XG_PLANE_USAGE_FRAGMENT_MASK:
            case XG_PLANE_USAGE_LUMA:
            case XG_PLANE_USAGE_CHROMA:
            case XG_PLANE_USAGE_DELTA_COLOR_COMPRESSION:
                break;
            }
        }

        {
            // Generate an HTile descriptor for the compute decompression system
            uint32_t TileCountX = paddedWidthElements / c_hTileTileWidth;
            uint32_t TileCountY = paddedHeightElements / c_hTileTileHeight;
            D3D12XBOX_GPU_HARDWARE_CONFIGURATION hwConfig;
            device->GetGpuHardwareConfigurationX(&hwConfig);
#ifdef _GAMING_XBOX_SCARLETT
            const uint32_t IsHTileLinear = 0u;
            const uint32_t PipeCount = hwConfig.HardwareVersion == D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART ? 8u : 32u;
            const uint32_t MacroTileWidth = hwConfig.HardwareVersion == D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART ? 64u : 128u;
            const uint32_t MacroTileHeight = hwConfig.HardwareVersion == D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART ? 64u : 128u;
#else
            const uint32_t IsHTileLinear = displayWidth * displayHeight < 0x200000 ? 1u : 0u;
            const uint32_t PipeCount = hwConfig.HardwareVersion >= D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X ? 8u : 4u;
            const uint32_t MacroTileWidth = 64u;
            const uint32_t MacroTileHeight = 8u * PipeCount;
#endif
            if (!IsHTileLinear)
            {
                TileCountX = AlignUp(TileCountX, MacroTileWidth);
                TileCountY = AlignUp(TileCountY, MacroTileHeight);
                assert(htileSizeBytes == TileCountX * TileCountY * 4);
            }
            m_hTileInfo = IsHTileLinear << 31 | PipeCount << 24 | TileCountX | TileCountY << 12;
        }

        // Create a texture which can hold htile information (encoded)
        // This must be a placement allocation because it aliases the actual htile data.
        {
            D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(
                htileSizeBytes,
                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                htileAlignmentInBytes
            );

            DX::ThrowIfFailed(
                device->CreatePlacedResourceX(
                    *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(&HTile),
                    &desc,
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_hTileBuffer.GetAddressOf())));

            m_hTileBuffer->SetName(L"HTile buffer");
        }

        D3D12_RESOURCE_DESC descDepth = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R32_FLOAT,
            s_bufferResolutions[Resolution::Full].width,
            s_bufferResolutions[Resolution::Full].height,
            1,
            1,
            1, 0,
            D3D12_RESOURCE_FLAG_NONE,
            static_cast<D3D12_TEXTURE_LAYOUT>(m_deviceResources->GetDepthTileMode()));

        DX::ThrowIfFailed(
            device->CreatePlacedResourceX(
                *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(&DepthSamples),
                &descDepth,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_depthPlane.GetAddressOf())));

        m_depthPlane->SetName(L"Depth Plane");

        // Create the SRV for the linear depth resource
        D3D12_SHADER_RESOURCE_VIEW_DESC depthPlaneSrvDesc = {};
        depthPlaneSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        depthPlaneSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        depthPlaneSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        depthPlaneSrvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(m_depthPlane.Get(), &depthPlaneSrvDesc, m_srvHeap->GetCpuHandle(DescriptorIndex::DepthSrv));
    }

    m_camera.SetWindow(static_cast<int>(displayWidth), static_cast<int>(displayHeight));
    // Reverse near and far clip because we're using reverse depth
    m_camera.SetProjectionParameters(XM_PIDIV4, c_farClipDistance, c_nearClipDistance, true);
    m_camera.SetLookAt(SimpleMath::Vector3(0.0f, 0.0f, 0.0f), SimpleMath::Vector3::Forward);
    m_camera.SetSensitivity(500.0f, 100.0f, 1000.0f, 10.0f);

    D3D12_VIEWPORT hudViewport = { 0, 0, 1920, 1080 };
    m_hudBatch->SetViewport(hudViewport);
}
#pragma endregion

void Sample::BlurAndUpsample(uint32_t highResWidth, uint32_t highResHeight, uint32_t lowResWidth, uint32_t lowResHeight)
{
    float kBlurTolerance = 1.0f - powf(10.0f, m_blurTolerance) * 1920.0f / (float)lowResWidth;
    kBlurTolerance *= kBlurTolerance;
    float kUpsampleTolerance = powf(10.0f, m_upsampleTolerance);
    float kNoiseFilterWeight = 1.0f / (powf(10.0f, m_noiseFilterTolerance) + kUpsampleTolerance);

    BlurAndUpsampleConstants constants;
    XMStoreFloat2(&constants.InvLowResolution, SimpleMath::Vector2(1.0f / lowResWidth, 1.0f / lowResHeight));
    XMStoreFloat2(&constants.InvHighResolution, SimpleMath::Vector2(1.0f / highResWidth, 1.0f / highResHeight));
    constants.NoiseFilterStrength = kNoiseFilterWeight;
    constants.StepSize = 1920.0f / (float)lowResWidth;
    constants.kBlurTolerance = kBlurTolerance;
    constants.kUpsampleTolerance = kUpsampleTolerance;

    auto cbMem = m_graphicsMemory->AllocateConstant<BlurAndUpsampleConstants>(constants);
    auto commandList = m_deviceResources->GetCommandList();
    commandList->SetComputeRootConstantBufferView(RootSignatureSSAO::CBV, cbMem.GpuAddress());

    commandList->Dispatch(AlignUp(highResWidth + 2, 16) / 16, AlignUp(highResHeight + 2, 16) / 16, 1);
}

void Sample::ComputeAO(const float TanHalfFovH, const uint32_t bufferWidth, const uint32_t bufferHeight, const uint32_t arrayCount)
{
    // Here we compute multipliers that convert the center depth value into (the reciprocal of)
    // sphere thicknesses at each sample location.  This assumes a maximum sample radius of 5
    // units, but since a sphere has no thickness at its extent, we don't need to sample that far
    // out.  Only samples whole integer offsets with distance less than 25 are used.  This means
    // that there is no sample at (3, 4) because its distance is exactly 25 (and has a thickness of 0.)

    // The shaders are set up to sample a circular region within a 5-pixel radius.
    const float ScreenspaceDiameter = 10.0f;

    // SphereDiameter = CenterDepth * ThicknessMultiplier.  This will compute the thickness of a sphere centered
    // at a specific depth.  The ellipsoid scale can stretch a sphere into an ellipsoid, which changes the
    // characteristics of the AO.
    // TanHalfFovH:  Radius of sphere in depth units if its center lies at Z = 1
    // ScreenspaceDiameter:  Diameter of sample sphere in pixel units
    // ScreenspaceDiameter / bufferWidth:  Ratio of the screen width that the sphere actually covers
    // Note about the "2.0f * ":  Diameter = 2 * Radius
    float ThicknessMultiplier = 2.0f * TanHalfFovH * ScreenspaceDiameter / bufferWidth;

    if (arrayCount == 1)
    {
        ThicknessMultiplier *= 2.0f;
    }

    // This will transform a depth value from [0, thickness] to [0, 1].
    float InverseRangeFactor = 1.0f / ThicknessMultiplier;

    SSOARenderConstants ssaoCB;

    // The thicknesses are smaller for all off-center samples of the sphere.  Compute thicknesses relative
    // to the center sample.
    ssaoCB.gInvThicknessTable[0].x = InverseRangeFactor / c_sampleThickness[0];
    ssaoCB.gInvThicknessTable[0].y = InverseRangeFactor / c_sampleThickness[1];
    ssaoCB.gInvThicknessTable[0].z = InverseRangeFactor / c_sampleThickness[2];
    ssaoCB.gInvThicknessTable[0].w = InverseRangeFactor / c_sampleThickness[3];
    ssaoCB.gInvThicknessTable[1].x = InverseRangeFactor / c_sampleThickness[4];
    ssaoCB.gInvThicknessTable[1].y = InverseRangeFactor / c_sampleThickness[5];
    ssaoCB.gInvThicknessTable[1].z = InverseRangeFactor / c_sampleThickness[6];
    ssaoCB.gInvThicknessTable[1].w = InverseRangeFactor / c_sampleThickness[7];
    ssaoCB.gInvThicknessTable[2].x = InverseRangeFactor / c_sampleThickness[8];
    ssaoCB.gInvThicknessTable[2].y = InverseRangeFactor / c_sampleThickness[9];
    ssaoCB.gInvThicknessTable[2].z = InverseRangeFactor / c_sampleThickness[10];
    ssaoCB.gInvThicknessTable[2].w = InverseRangeFactor / c_sampleThickness[11];

    // These are the weights that are multiplied against the samples because not all samples are
    // equally important.  The farther the sample is from the center location, the less they matter.
    // We use the thickness of the sphere to determine the weight.  The scalars in front are the number
    // of samples with this weight because we sum the samples together before multiplying by the weight,
    // so as an aggregate all of those samples matter more.  After generating this table, the weights
    // are normalized.
    ssaoCB.gSampleWeightTable[0].x = 4.0f * c_sampleThickness[0];	// Axial
    ssaoCB.gSampleWeightTable[0].y = 4.0f * c_sampleThickness[1];	// Axial
    ssaoCB.gSampleWeightTable[0].z = 4.0f * c_sampleThickness[2];	// Axial
    ssaoCB.gSampleWeightTable[0].w = 4.0f * c_sampleThickness[3];	// Axial
    ssaoCB.gSampleWeightTable[1].x = 4.0f * c_sampleThickness[4];	// Diagonal
    ssaoCB.gSampleWeightTable[1].y = 8.0f * c_sampleThickness[5];	// L-shaped
    ssaoCB.gSampleWeightTable[1].z = 8.0f * c_sampleThickness[6];	// L-shaped
    ssaoCB.gSampleWeightTable[1].w = 8.0f * c_sampleThickness[7];	// L-shaped
    ssaoCB.gSampleWeightTable[2].x = 4.0f * c_sampleThickness[8];	// Diagonal
    ssaoCB.gSampleWeightTable[2].y = 8.0f * c_sampleThickness[9];	// L-shaped
    ssaoCB.gSampleWeightTable[2].z = 8.0f * c_sampleThickness[10];	// L-shaped
    ssaoCB.gSampleWeightTable[2].w = 4.0f * c_sampleThickness[11];	// Diagonal

    // If we aren't using all of the samples, delete their weights before we normalize.
    if (!m_sampleExhaustively)
    {
        ssaoCB.gSampleWeightTable[0].x = 0.0f;
        ssaoCB.gSampleWeightTable[0].z = 0.0f;
        ssaoCB.gSampleWeightTable[1].y = 0.0f;
        ssaoCB.gSampleWeightTable[1].w = 0.0f;
        ssaoCB.gSampleWeightTable[2].y = 0.0f;
    }

    // Normalize the weights by dividing by the sum of all weights
    float totalWeight = 0.0f;
    for (float4& weight : ssaoCB.gSampleWeightTable)
    {
        totalWeight += weight.x;
        totalWeight += weight.y;
        totalWeight += weight.z;
        totalWeight += weight.w;
    }

    for (float4& weight : ssaoCB.gSampleWeightTable)
    {
        weight.x /= totalWeight;
        weight.y /= totalWeight;
        weight.z /= totalWeight;
        weight.w /= totalWeight;
    }

    ssaoCB.gInvSliceDimension.x = 1.0f / bufferWidth;
    ssaoCB.gInvSliceDimension.y = 1.0f / bufferHeight;
    ssaoCB.gRejectFadeoff = 1.0f / -m_rejectionFalloff;
    ssaoCB.gRcpAccentuation = 1.0f / (1.0f + m_accentuation);

    auto commandList = m_deviceResources->GetCommandList();
    auto cbMem = m_graphicsMemory->AllocateConstant<SSOARenderConstants>(ssaoCB);
    commandList->SetComputeRootConstantBufferView(RootSignatureSSAO::CBV, cbMem.GpuAddress());

    if (arrayCount == 1)
    {
        commandList->Dispatch(AlignUp(bufferWidth, 16) / 16, AlignUp(bufferHeight, 16) / 16, 1);
    }
    else
    {
        commandList->Dispatch(AlignUp(bufferWidth, 8) / 8, AlignUp(bufferHeight, 8) / 8, arrayCount);
    }
}

void Sample::RenderSSAO(uint32_t frameCount)
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Compute SSAO");

    commandList->SetComputeRootSignature(m_rootSignatureSSAO.Get());

    // Phase 1:  Decompress, linearize, downsample, and deinterleave the depth buffer
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Decompress and downsample");
        constexpr float zMagic = (c_farClipDistance - c_nearClipDistance) / c_nearClipDistance;
        SSAOConstants ssaoConstants = { zMagic, c_zClear, m_hTileInfo, frameCount % 2 };
        commandList->SetComputeRoot32BitConstants(RootSignatureSSAO::RootConstants, sizeof(SSAOConstants) / sizeof(uint32_t), &ssaoConstants, 0);
        commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthSrv));
        commandList->SetComputeRootShaderResourceView(RootSignatureSSAO::T5, m_hTileBuffer.Get()->GetGPUVirtualAddress());

        D3D12_RESOURCE_BARRIER barrierSrvToUav[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_linearDepth->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_depthDownsize1->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_depthTiled1->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_depthDownsize2->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_depthTiled2->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12XBOX_RESOURCE_STATE_PRESERVE_COMPRESSED_DEPTH_STENCIL | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
        };
        commandList->ResourceBarrier(_countof(barrierSrvToUav), barrierSrvToUav);
        commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::DownsampleUavRange1Start));

        commandList->SetPipelineState(m_pipelineStatePrepareDepthBuffers1.Get());
        commandList->Dispatch(CalcDispatchSSAO(s_bufferResolutions[Resolution::Tiled2].width, 8), CalcDispatchSSAO(s_bufferResolutions[Resolution::Tiled2].height, 8), 1);

        if (m_hierarchyDepth > 2)
        {
            D3D12_RESOURCE_BARRIER barrier[] =
            {
                CD3DX12_RESOURCE_BARRIER::Transition(m_depthDownsize2->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(m_depthDownsize3->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition(m_depthTiled3->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition(m_depthDownsize4->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition(m_depthTiled4->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            };
            commandList->ResourceBarrier(_countof(barrier), barrier);

            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::DownsampleUavRange2Start));
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthDownsize2Srv));
            commandList->SetPipelineState(m_pipelineStatePrepareDepthBuffers2.Get());
            commandList->Dispatch(CalcDispatchSSAO(s_bufferResolutions[Resolution::Tiled4].width, 8), CalcDispatchSSAO(s_bufferResolutions[Resolution::Tiled4].height, 8), 1);
        }
    }

    // Phase 2:  Render SSAO for each sub-tile
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Analyze depth volumes");

        // Load first element of projection matrix which is the cotangent of the horizontal FOV divided by 2.
        const float fovTangent = 1.0f / XMVectorGetX(m_camera.GetProjection().r[0]);

        D3D12_RESOURCE_BARRIER barrier[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_aoMerged1->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_aoHigh1->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_depthTiled1->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_depthTiled2->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_depthDownsize1->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),

            // only if m_hierarchyDepth > 1
            CD3DX12_RESOURCE_BARRIER::Transition(m_aoMerged2->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_aoHigh2->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),

            // only if m_hierarchyDepth > 2
            CD3DX12_RESOURCE_BARRIER::Transition(m_depthDownsize3->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_depthTiled3->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_depthDownsize4->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_depthTiled4->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_aoMerged3->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_aoHigh3->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),

            // only if m_hierarchyDepth > 3
            CD3DX12_RESOURCE_BARRIER::Transition(m_aoMerged4->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_aoHigh4->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        };

        if (m_hierarchyDepth <= 2)
        {
            barrier[m_hierarchyDepth > 1 ? 7 : 5] = CD3DX12_RESOURCE_BARRIER::Transition(m_depthDownsize2->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        }
        commandList->ResourceBarrier(m_hierarchyDepth > 3 ? _countof(barrier) : m_hierarchyDepth > 2 ? _countof(barrier) - 2 : m_hierarchyDepth > 1 ? _countof(barrier) - 7 : _countof(barrier) - 9, barrier);

        const auto render1PSO = m_sampleExhaustively ? m_pipelineStateAoRender1SampleExhaustively.Get() : m_pipelineStateAoRender1[m_useFp16 ? PipelineStates::Fp16 : PipelineStates::Fp32].Get();
        const auto render2PSO = m_sampleExhaustively ? m_pipelineStateAoRender2SampleExhaustively.Get() : m_pipelineStateAoRender2[m_useFp16 ? PipelineStates::Fp16 : PipelineStates::Fp32].Get();
        if (m_hierarchyDepth > 3)
        {
            commandList->SetPipelineState(render1PSO);
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::AoMerged4Uav));
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthTiled4Srv));
            ComputeAO(fovTangent, s_bufferResolutions[Resolution::Tiled4].width, s_bufferResolutions[Resolution::Tiled4].height, c_depthTiledArraySize);
            if (m_ssaoQuality >= SSAOQuality::Low)
            {
                commandList->SetPipelineState(render2PSO);
                commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::AoHigh4Uav));
                commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthDownsize4Srv));
                ComputeAO(fovTangent, s_bufferResolutions[Resolution::Res4].width, s_bufferResolutions[Resolution::Res4].height, 1);
            }
        }

        if (m_hierarchyDepth > 2)
        {
            commandList->SetPipelineState(render1PSO);
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::AoMerged3Uav));
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthTiled3Srv));
            ComputeAO(fovTangent, s_bufferResolutions[Resolution::Tiled3].width, s_bufferResolutions[Resolution::Tiled3].height, c_depthTiledArraySize);
            if (m_ssaoQuality >= SSAOQuality::Medium)
            {
                commandList->SetPipelineState(render2PSO);
                commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::AoHigh3Uav));
                commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthDownsize3Srv));
                ComputeAO(fovTangent, s_bufferResolutions[Resolution::Res3].width, s_bufferResolutions[Resolution::Res3].height, 1);
            }
        }

        if (m_hierarchyDepth > 1)
        {
            commandList->SetPipelineState(render1PSO);
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::AoMerged2Uav));
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthTiled2Srv));
            ComputeAO(fovTangent, s_bufferResolutions[Resolution::Tiled2].width, s_bufferResolutions[Resolution::Tiled2].height, c_depthTiledArraySize);
            if (m_ssaoQuality >= SSAOQuality::High)
            {
                commandList->SetPipelineState(render2PSO);
                commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::AoHigh2Uav));
                commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthDownsize2Srv));
                ComputeAO(fovTangent, s_bufferResolutions[Resolution::Res2].width, s_bufferResolutions[Resolution::Res2].height, 1);
            }
        }

        {
            commandList->SetPipelineState(render1PSO);
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::AoMerged1Uav));
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthTiled1Srv));
            ComputeAO(fovTangent, s_bufferResolutions[Resolution::Tiled1].width, s_bufferResolutions[Resolution::Tiled1].height, c_depthTiledArraySize);
            if (m_ssaoQuality >= SSAOQuality::VeryHigh)
            {
                commandList->SetPipelineState(render2PSO);
                commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::AoHigh1Uav));
                commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthDownsize1Srv));
                ComputeAO(fovTangent, s_bufferResolutions[Resolution::Res1].width, s_bufferResolutions[Resolution::Res1].height, 1);
            }
        }
    }

    // Phase 3:  Iteratively blur and upsample, combining each result
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Blur and upsample");

        ID3D12Resource* nextResource = m_aoMerged4->GetResource();
        D3D12_GPU_DESCRIPTOR_HANDLE nextSrv = m_srvHeap->GetGpuHandle(DescriptorIndex::AoMerged4Srv);

        if (m_hierarchyDepth > 3)
        {
            D3D12_RESOURCE_BARRIER barrier[] =
            {
                CD3DX12_RESOURCE_BARRIER::Transition(m_aoSmooth3->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition(nextResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(m_aoHigh4->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(m_aoMerged3->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            };
            commandList->ResourceBarrier(_countof(barrier), barrier);
            commandList->SetPipelineState(m_ssaoQuality >= SSAOQuality::Low ? m_pipelineStateBlurAndUpsampleHigh.Get() : m_pipelineStateBlurAndUpsample.Get());
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::AoSmooth3Uav));
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthDownsize4Srv));
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T1, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthDownsize3Srv));
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T2, nextSrv);
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T4, m_srvHeap->GetGpuHandle(DescriptorIndex::AoMerged3Srv));
            if (m_ssaoQuality >= SSAOQuality::Low)
            {
                commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T3, m_srvHeap->GetGpuHandle(DescriptorIndex::AoHigh4Srv));
            }

            BlurAndUpsample(s_bufferResolutions[Resolution::Res3].width, s_bufferResolutions[Resolution::Res3].height, s_bufferResolutions[Resolution::Res4].width, s_bufferResolutions[Resolution::Res4].height);

            nextResource = m_aoSmooth3->GetResource();
            nextSrv = m_srvHeap->GetGpuHandle(DescriptorIndex::AoSmooth3Srv);
        }
        else
        {
            nextResource = m_aoMerged3->GetResource();
            nextSrv = m_srvHeap->GetGpuHandle(DescriptorIndex::AoMerged3Srv);
        }

        if (m_hierarchyDepth > 2)
        {
            D3D12_RESOURCE_BARRIER barrier[] =
            {
                CD3DX12_RESOURCE_BARRIER::Transition(m_aoSmooth2->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition(nextResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(m_aoHigh3->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(m_aoMerged2->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            };
            commandList->ResourceBarrier(_countof(barrier), barrier);
            commandList->SetPipelineState(m_ssaoQuality >= SSAOQuality::Medium ? m_pipelineStateBlurAndUpsampleHigh.Get() : m_pipelineStateBlurAndUpsample.Get());
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::AoSmooth2Uav));
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthDownsize3Srv));
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T1, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthDownsize2Srv));
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T2, nextSrv);
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T4, m_srvHeap->GetGpuHandle(DescriptorIndex::AoMerged2Srv));
            if (m_ssaoQuality >= SSAOQuality::Medium)
            {
                commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T3, m_srvHeap->GetGpuHandle(DescriptorIndex::AoHigh3Srv));
            }


            BlurAndUpsample(s_bufferResolutions[Resolution::Res2].width, s_bufferResolutions[Resolution::Res2].height, s_bufferResolutions[Resolution::Res3].width, s_bufferResolutions[Resolution::Res3].height);

            nextResource = m_aoSmooth2->GetResource();
            nextSrv = m_srvHeap->GetGpuHandle(DescriptorIndex::AoSmooth2Srv);
        }
        else
        {
            nextResource = m_aoMerged2->GetResource();
            nextSrv = m_srvHeap->GetGpuHandle(DescriptorIndex::AoMerged2Srv);
        }

        if (m_hierarchyDepth > 1)
        {
            D3D12_RESOURCE_BARRIER barrier[] =
            {
                CD3DX12_RESOURCE_BARRIER::Transition(m_aoSmooth1->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition(nextResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(m_aoHigh2->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(m_aoMerged1->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            };
            commandList->ResourceBarrier(_countof(barrier), barrier);
            commandList->SetPipelineState(m_ssaoQuality >= SSAOQuality::High ? m_pipelineStateBlurAndUpsampleHigh.Get() : m_pipelineStateBlurAndUpsample.Get());
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::AoSmooth1Uav));
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthDownsize2Srv));
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T1, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthDownsize1Srv));
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T2, nextSrv);
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T4, m_srvHeap->GetGpuHandle(DescriptorIndex::AoMerged1Srv));
            if (m_ssaoQuality >= SSAOQuality::High)
            {
                commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T3, m_srvHeap->GetGpuHandle(DescriptorIndex::AoHigh2Srv));
            }

            BlurAndUpsample(s_bufferResolutions[Resolution::Res1].width, s_bufferResolutions[Resolution::Res1].height, s_bufferResolutions[Resolution::Res2].width, s_bufferResolutions[Resolution::Res2].height);

            nextResource = m_aoSmooth1->GetResource();
            nextSrv = m_srvHeap->GetGpuHandle(DescriptorIndex::AoSmooth1Srv);
        }
        else
        {
            nextResource = m_aoMerged1->GetResource();
            nextSrv = m_srvHeap->GetGpuHandle(DescriptorIndex::AoMerged1Srv);
        }

        D3D12_RESOURCE_BARRIER barrier[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_aoFullRes->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_linearDepth->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(nextResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_aoHigh1->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        };
        commandList->ResourceBarrier(_countof(barrier), barrier);
        commandList->SetPipelineState(m_ssaoQuality >= SSAOQuality::VeryHigh ? m_pipelineStateBlurAndUpsampleFinalHigh[m_useFp16 ? PipelineStates::Fp16 : PipelineStates::Fp32].Get() : m_pipelineStateBlurAndUpsampleFinal[m_useFp16 ? PipelineStates::Fp16 : PipelineStates::Fp32].Get());
        commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::AOUav));
        commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthDownsize1Srv));
        commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T1, m_srvHeap->GetGpuHandle(DescriptorIndex::LinearDepthSrv));
        commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T2, nextSrv);
        if (m_ssaoQuality >= SSAOQuality::VeryHigh)
        {
            commandList->SetComputeRootDescriptorTable(RootSignatureSSAO::T3, m_srvHeap->GetGpuHandle(DescriptorIndex::AoHigh1Srv));
        }

        BlurAndUpsample(s_bufferResolutions[Resolution::Full].width, s_bufferResolutions[Resolution::Full].height, s_bufferResolutions[Resolution::Res1].width, s_bufferResolutions[Resolution::Res1].height);
    }

    // Transition depth to read state to render scene with z equal depth testing
    D3D12_RESOURCE_BARRIER barrier[] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(m_aoFullRes->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(), D3D12XBOX_RESOURCE_STATE_PRESERVE_COMPRESSED_DEPTH_STENCIL | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12XBOX_RESOURCE_STATE_PRESERVE_COMPRESSED_DEPTH_STENCIL | D3D12_RESOURCE_STATE_DEPTH_READ)
    };
    commandList->ResourceBarrier(_countof(barrier), barrier);
}

void Sample::RenderGTAO()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Compute GTAO");

    commandList->SetComputeRootSignature(m_rootSignatureGTAO.Get());

    XeGTAO::GTAOConstants gtaoConstants;
    XeGTAO::GTAOUpdateConstants(gtaoConstants, s_bufferResolutions[Resolution::Full].width, s_bufferResolutions[Resolution::Full].height, m_settings, m_camera.GetProjection(), 0);
    auto cbMem = m_graphicsMemory->AllocateConstant<XeGTAO::GTAOConstants>(gtaoConstants);
    commandList->SetComputeRootConstantBufferView(RootSignatureGTAO::CBV, cbMem.GpuAddress());

    // Tranisition to initial state
    {
        D3D12_RESOURCE_BARRIER barriers[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_depthOutput.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_normals->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        };
        commandList->ResourceBarrier(_countof(barriers), barriers);
    }

    // Phase 1: Prefilter depths
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Prefilter Depths");

        commandList->SetComputeRootDescriptorTable(RootSignatureGTAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthSrv));
        commandList->SetComputeRootDescriptorTable(RootSignatureGTAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthOutputUavRangeStart));

        commandList->SetPipelineState(m_pipelineStatePrefilterDepths.Get());
        commandList->Dispatch(AlignUp(s_bufferResolutions[Resolution::Full].width, 16) / 16, AlignUp(s_bufferResolutions[Resolution::Full].height, 16) / 16, 1);
    }

    // Phase 2: Generate Normals
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Generate Normals");

        commandList->SetComputeRootDescriptorTable(RootSignatureGTAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthSrv));
        commandList->SetComputeRootDescriptorTable(RootSignatureGTAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::SceneNormalsUav));

        commandList->SetPipelineState(m_pipelineStateGenerateNormals.Get());
        commandList->Dispatch(AlignUp(s_bufferResolutions[Resolution::Full].width, XE_GTAO_NUMTHREADS_X) / XE_GTAO_NUMTHREADS_X, AlignUp(s_bufferResolutions[Resolution::Full].height, XE_GTAO_NUMTHREADS_Y) / XE_GTAO_NUMTHREADS_Y, 1);
    }

    // Phase 3: Main GTAO pass
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Main GTAO Pass");

        D3D12_RESOURCE_BARRIER barriers[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_depthOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_normals->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_workingAO->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_workingEdges->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        };
        commandList->ResourceBarrier(_countof(barriers), barriers);

        commandList->SetComputeRootDescriptorTable(RootSignatureGTAO::T0, m_srvHeap->GetGpuHandle(DescriptorIndex::DepthOutputSrv));
        commandList->SetComputeRootDescriptorTable(RootSignatureGTAO::T1, m_srvHeap->GetGpuHandle(DescriptorIndex::SceneNormalsSrv));
        commandList->SetComputeRootDescriptorTable(RootSignatureGTAO::T2, m_srvHeap->GetGpuHandle(DescriptorIndex::HilbertSrv));
        commandList->SetComputeRootDescriptorTable(RootSignatureGTAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::WorkingUavRange));

        switch (m_settings.QualityLevel)
        {
        case GTAOQuality::Low:
            commandList->SetPipelineState(m_pipelineStateGTAOLow[m_useFp16 ? PipelineStates::Fp16 : PipelineStates::Fp32].Get());
            break;
        case GTAOQuality::Medium:
            commandList->SetPipelineState(m_pipelineStateGTAOMedium[m_useFp16 ? PipelineStates::Fp16 : PipelineStates::Fp32].Get());
            break;
        case GTAOQuality::High:
            commandList->SetPipelineState(m_pipelineStateGTAOHigh[m_useFp16 ? PipelineStates::Fp16 : PipelineStates::Fp32].Get());
            break;
        case GTAOQuality::Ultra:
            commandList->SetPipelineState(m_pipelineStateGTAOUltra[m_useFp16 ? PipelineStates::Fp16 : PipelineStates::Fp32].Get());
            break;
        }

        commandList->Dispatch(AlignUp(s_bufferResolutions[Resolution::Full].width, XE_GTAO_NUMTHREADS_X_MAIN) / XE_GTAO_NUMTHREADS_X_MAIN, AlignUp(s_bufferResolutions[Resolution::Full].height, XE_GTAO_NUMTHREADS_Y_MAIN) / XE_GTAO_NUMTHREADS_Y_MAIN, 1);
    }

    // Phase 4: Spatial denoise filter
    const int passCount = std::max(1, m_settings.DenoisePasses); // even without denoising we have to run a single last pass to output correct term into the external output texture
    {
        PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"Denoise");

        D3D12_RESOURCE_BARRIER barriers[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_workingAO->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_workingAOTemp->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition(m_workingEdges->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_aoFullRes->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        };
        commandList->ResourceBarrier(_countof(barriers), barriers);

        commandList->SetComputeRootDescriptorTable(RootSignatureGTAO::T1, m_srvHeap->GetGpuHandle(DescriptorIndex::WorkingEdgesSrv));
        commandList->SetPipelineState(m_pipelineStateDenoisePass.Get());

        for (int i = 0; i < passCount; i++)
        {
            const bool lastPass = i == passCount - 1;
            const bool evenPass = i % 2 == 0;

            commandList->SetComputeRootDescriptorTable(RootSignatureGTAO::T0, m_srvHeap->GetGpuHandle(evenPass ? DescriptorIndex::WorkingAoSrv : DescriptorIndex::WorkingAoTempSrv));

            if (lastPass)
            {
                commandList->SetPipelineState(m_pipelineStateDenoiseLastPass.Get());

                commandList->SetComputeRootDescriptorTable(RootSignatureGTAO::UAVs, m_srvHeap->GetGpuHandle(DescriptorIndex::AOUav));
            }
            else
            {
                commandList->SetComputeRootDescriptorTable(RootSignatureGTAO::UAVs, m_srvHeap->GetGpuHandle(evenPass ? DescriptorIndex::WorkingAoTempUav : DescriptorIndex::WorkingAoUav));
            }

            commandList->Dispatch(AlignUp(s_bufferResolutions[Resolution::Full].width, (XE_GTAO_NUMTHREADS_X * 2)) / (XE_GTAO_NUMTHREADS_X * 2), AlignUp(s_bufferResolutions[Resolution::Full].height, XE_GTAO_NUMTHREADS_Y) / XE_GTAO_NUMTHREADS_Y, 1);

            if (!lastPass)
            {
                D3D12_RESOURCE_BARRIER barriersPass[] =
                {
                    CD3DX12_RESOURCE_BARRIER::Transition(evenPass ? m_workingAOTemp->GetResource() : m_workingAO->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                    CD3DX12_RESOURCE_BARRIER::Transition(evenPass ? m_workingAO->GetResource() : m_workingAOTemp->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                };
                commandList->ResourceBarrier(_countof(barriersPass), barriersPass);
            }
        }
    }

    // Transition depth to read state to render scene with z equal depth testing
    D3D12_RESOURCE_BARRIER barriers[] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(m_aoFullRes->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_READ),
        CD3DX12_RESOURCE_BARRIER::Transition(passCount % 2 == 0 ? m_workingAO->GetResource() : m_workingAOTemp->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
    };
    commandList->ResourceBarrier(_countof(barriers), barriers);
}

void Sample::RenderHUD()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXScopedEvent(commandList, PIX_COLOR_DEFAULT, L"RenderHUD");
    m_hudBatch->Begin(commandList);

    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &rtvDescriptor, false, nullptr);

    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(1920, 1080);
    const int boxPadding = 10;
    constexpr size_t textLength = 512;
    wchar_t topText[textLength] = {};
    wchar_t optionsText[textLength] = {};

    auto hierarchyDepthSelection = m_selectedOption == SelectedOption::HierarchyDepth ? L" * " : L"   ";
    auto ssaoQualitySelection = m_selectedOption == SelectedOption::SsaoQuality ? L" * " : L"   ";
    auto noiseFilterToleranceSelection = m_selectedOption == SelectedOption::NoiseFilterTolerance ? L" * " : L"   ";
    auto blurToleranceSelection = m_selectedOption == SelectedOption::BlurTolerance ? L" * " : L"   ";
    auto upsampleToleranceSelection = m_selectedOption == SelectedOption::UpsampleTolerance ? L" * " : L"   ";
    auto rejectionFalloffSelection = m_selectedOption == SelectedOption::RejectionFalloff ? L" * " : L"   ";
    auto accentuationSelection = m_selectedOption == SelectedOption::Accentuation ? L" * " : L"   ";
    auto gtaoQualitySelection = m_selectedOption == SelectedOption::GtaoQuality ? L" * " : L"   ";
    auto denoisePassesSelection = m_selectedOption == SelectedOption::DenoisePasses ? L" * " : L"   ";
    auto radiusSelection = m_selectedOption == SelectedOption::Radius ? L" * " : L"   ";
    auto sampleExhaustivelySelection = m_selectedOption == SelectedOption::SampleExhaustively ? L" * " : L"   ";

    if (m_aoImplementation == AoImplementation::SSAO)
    {
        swprintf_s(optionsText,
            L"SSAO Options\n"\
            "%lsHierarchy Depth %u\n"\
            "%lsSSAO Quality %u\n"\
            "%lsNoise Filter Tolerance %.2f\n"\
            "%lsBlur Tolerance %.2f\n"\
            "%lsUpsample Tolerance %.2f\n"\
            "%lsRejection Falloff %.2f\n"\
            "%lsAccentuation %.2f\n"\
            "%lsSample Exhaustively %ls",
            hierarchyDepthSelection, m_hierarchyDepth,
            ssaoQualitySelection, m_ssaoQuality,
            noiseFilterToleranceSelection, m_noiseFilterTolerance,
            blurToleranceSelection, m_blurTolerance,
            upsampleToleranceSelection, m_upsampleTolerance,
            rejectionFalloffSelection, m_rejectionFalloff,
            accentuationSelection, m_accentuation,
            sampleExhaustivelySelection, m_sampleExhaustively ? L"on" : L"off");
    }
    else
    {
        swprintf_s(optionsText,
            L"GTAO Options\n"\
            "%lsGTAO Quality %u\n"\
            "%lsDenoise Passes %u\n"\
            "%lsRadius %.2f\n",
            gtaoQualitySelection, m_settings.QualityLevel,
            denoisePassesSelection, m_settings.DenoisePasses,
            radiusSelection, m_settings.Radius);
    }
    auto const aoDisplayText = m_showAOTexture ? L"Render Mode: AO Texture" : L"Render Mode: Scene";
    auto const aoImplementationText = m_aoImplementation == AoImplementation::SSAO ? L"AO Implementation: SSAO" : L"AO Implementation: GTAO";
    auto const aoApplyText = m_applyAO ? L"AO applied to scene" : L"AO off";
    auto const fp16Text = m_useFp16 ? L"Using FP16 shaders" : L"Using FP32 shaders";
    swprintf_s(topText, textLength, L"Ambient Occlusion\nAO Time: %0.3f ms\n%ls\n%ls\n%ls\n%ls\n%ls", m_gpuTimer->GetElapsedMS(0), aoImplementationText, aoDisplayText, aoApplyText, fp16Text, optionsText);

    auto textMeasure = m_font->MeasureString(topText);
    int textWidth = static_cast<int>(XMVectorGetX(textMeasure));
    int textHeight = static_cast<int>(XMVectorGetY(textMeasure));
    auto const grayTextureHandle = m_srvHeap->GetGpuHandle(DescriptorIndex::GrayTexture);

    m_hudBatch->Draw(
        grayTextureHandle,
        { 1, 1 },
        RECT{ safe.left - boxPadding, safe.top - boxPadding, safe.left + textWidth + boxPadding, safe.top + textHeight + boxPadding });

    m_font->DrawString(m_hudBatch.get(), topText, XMFLOAT2(float(safe.left), float(safe.top)), DirectX::Colors::DarkKhaki);

    auto const bottomText =
#ifdef _GAMING_XBOX_SCARLETT
        L"[DPAD] Up/Down: Select an option\n"\
        "[DPAD] Left/Right: Modify option value\n"\
        "[A]: Toggle show AO texture\n"\
        "[B]: Switch AO implementation\n"\
        "[Y]: Toggle apply AO to scene\n"\
        "[X]: Toggle use FP16 shaders";
#else
        L"[DPAD] Up/Down: Select an option\n"\
        "[DPAD] Left/Right: Modify option value\n"\
        "[A]: Toggle show AO texture\n"\
        "[B]: Switch AO implementation\n"\
        "[Y]: Toggle apply AO to scene";
#endif

    textMeasure = m_font->MeasureString(bottomText);
    textWidth = static_cast<int>(XMVectorGetX(textMeasure));
    textHeight = static_cast<int>(XMVectorGetY(textMeasure));

    m_hudBatch->Draw(
        grayTextureHandle,
        { 1, 1 },
        RECT{ safe.left, safe.bottom - textHeight - boxPadding, safe.left + textWidth, safe.bottom + boxPadding });

    DX::DrawControllerString(m_hudBatch.get(), m_font.get(), m_ctrlFont.get(), bottomText, XMFLOAT2(float(safe.left), float(safe.bottom - textHeight)), DirectX::Colors::DarkKhaki);

    m_hudBatch->End();
}
