//--------------------------------------------------------------------------------------
// DrawIndexedX.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// This sample shows how to use the Xbox only feature - DrawIndexedX.
// DrawIndexedX can be used instead of DrawIndexedInstanced for a single instance.
// In case of multiple instances, DrawIndexedX can be separately called for each
// instance and the instance data can be passed into a buffer with some bits of index buffer
// identifying the correct instance.
// When using this API, the driver can pack threads from multiple consecutive draw calls
// into a single wave and execute them as a single draw.
// This packing happens only if there are no state changes between draw calls.
// In case of state changes, the draw call behaves same as DrawIndexedInstanced.
// Only the index buffer and format can be changed for each DrawIndexedX call.
// An offset can be provided which offsets into the vertex buffer.
// The vertex buffer needs to be combined into a single big buffer containing all vertex data.
//
// The easiest place to use this feature with immediate gains is the depth pass as the waves
// are not different. In the Color Pass, it can cause divergence within the threads of a wave
// as it might fetch from different textures, though the divergence might not be bad (testing
// your game will help determine this). A texture atlas can be used to decrease divergence.
//
// On Xbox One X, this feature behaves the same as DrawIndexedInstanced due to some
// hardware settings. Some extra complicated microcode will need to be implemented for
// turning on this feature on Xbox One X. Please contact us if you need this feature on Xbox One X.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DrawIndexedX.h"

#include "ATGColors.h"
#include "ReadData.h"
#include "Common.h"
#include "ControllerFont.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const wchar_t* g_sampleTitle = L"DrawIndexedX Sample";
    const wchar_t* g_sampleDescription = L"Sample shows usage of DrawIndexedX API";
    const ATG::HelpButtonAssignment g_helpButtons[] = {
        { ATG::HelpID::MENU_BUTTON,         L"Show/hide help" },
        { ATG::HelpID::VIEW_BUTTON,         L"Exit" },
        { ATG::HelpID::A_BUTTON,            L"Toggle Draw methods" },
        { ATG::HelpID::Y_BUTTON,            L"Show Debug Screen" },
        { ATG::HelpID::X_BUTTON,            L"Texture using Atlas" },
        { ATG::HelpID::LEFT_SHOULDER,       L"Show depth buffer" },
        { ATG::HelpID::LEFT_STICK,          L"Translate camera" },
        { ATG::HelpID::RIGHT_STICK,         L"Rotate camera" },
    };

    // Assuming a max number of textures for a given model - used in the Descriptor heap.
    // As this is for the textures from the model using hash maps, it is a low number
    constexpr uint32_t c_numTextures = 32;

    enum class DescriptorHeapEntry
    {
        TextFont,
        ControllerFont,
        UAVCountWavesBuffer,
        MeshAtlasTexture,
        MeshTexture = MeshAtlasTexture + c_numTextures,
        TotalDescriptorHeapEntryCount = 128
    };

    enum class TimerCounters
    {
        DEPTH_PASS,
        MESH_RENDER,
        TOTAL_TIME,
        NUM_TIMER_COUNTERS
    };

    // Variations of the PSOs
    enum class PSODesc
    {
        DEPTH_PASS_DRAW_INSTANCED,
        DEPTH_PASS_DRAW_INDEXED_X,
        MESH_RENDER_DRAW_INSTANCED,
        MESH_RENDER_DRAW_INDEXED_X,
        MESH_RENDER_COLORS_DEBUG,
        MESH_RENDER_COLORS_DEBUG_DRAW_INDEXED_X,
        TOTAL_WAVES_DEBUG,
    };
    static_assert(static_cast<uint32_t>(PSODesc::TOTAL_WAVES_DEBUG) == static_cast<uint32_t>(NUM_PSO), "Mismatch in number of PSOs");
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_modelEffect{},
    m_descrptorIncrementSizeCBVSRVUAV(0),
    m_currFrameIndex(0),
    m_fullScreenVBView{},
    m_dsvState(D3D12_RESOURCE_STATE_COMMON),
    m_bufferWavesDescriptorCPU{},
    m_bufferWavesDescriptorGPU{},
    m_bufferWavesMapped(nullptr),
    m_showDebugData(false),
    m_renderDepthBuffer(false),
    m_drawUsingDrawIndexedX(true),
    m_showHelp(false),
    m_selectedModel(SelectedModel::ModelIndividual),
    m_textureDescriptorHeapEntry(0),
    m_isXboxOneX(false),
    m_indexFormat{},
    m_indexBufferSize{},
    m_totalVertexOffset{}
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT,
        2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_textureDescriptorHeapEntry = static_cast<size_t>(DescriptorHeapEntry::MeshTexture);
    m_help = std::make_unique<ATG::Help>(g_sampleTitle, g_sampleDescription, g_helpButtons, _countof(g_helpButtons), true);
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

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    m_deviceResources->CreateWindowSizeDependentResources();
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();

#ifdef _GAMING_XBOX_SCARLETT
    auto systemAnalyticsInfo = XSystemGetAnalyticsInfo();
    auto hostingOsVersion = systemAnalyticsInfo.hostingOsVersion;
    auto recoveryVersion = (uint32_t(hostingOsVersion.build) << 16) | uint32_t(hostingOsVersion.revision);
    uint32_t minScarlettSupportVersion = (22000 << 16) | 1931;
    if (recoveryVersion < minScarlettSupportVersion)
    {
        OutputDebugString(L"ERROR: Recovery version too old for Xbox Series X|S support.\n");
        throw std::exception();
    }
#endif
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

        if (pad.IsViewPressed())
        {
            ExitSample();
        }
        if (m_gamePadButtons.menu == GamePad::ButtonStateTracker::PRESSED)
        {
            m_showHelp = !m_showHelp;
        }

        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            m_gpuTimer->Reset();
            m_showDebugData = !m_showDebugData;
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_gpuTimer->Reset();
            m_drawUsingDrawIndexedX = !m_drawUsingDrawIndexedX;
        }

        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            m_gpuTimer->Reset();
            m_selectedModel = static_cast<SelectedModel>((static_cast<uint32_t>(m_selectedModel) + 1) % SelectedModel::NumModels);
            m_textureDescriptorHeapEntry = static_cast<size_t>(DescriptorHeapEntry::MeshAtlasTexture) + m_selectedModel * c_numTextures;
        }

        if (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::PRESSED)
        {
            m_gpuTimer->Reset();
            m_renderDepthBuffer = !m_renderDepthBuffer;
            m_showDebugData = false;
        }

        for (uint32_t modelNum = 0; modelNum < SelectedModel::NumModels; ++modelNum)
        {
            m_modelEffect[modelNum].m_effect->UpdateCamera(pad, elapsedTime);
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
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    m_modelEffect[m_selectedModel].m_effect->ResetAllDirtyFlags();
    m_gpuTimer->BeginFrame(commandList);

    if (m_dsvState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Depth Buffer Barrier");
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(), m_dsvState, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        commandList->ResourceBarrier(1, &barrier);
        PIXEndEvent(commandList);
        m_dsvState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }

    // Clear all targets
    Clear();

    if (m_showHelp)
    {
        m_help->Render(commandList);
    }
    else
    {
        // Measure total frame time
        m_gpuTimer->Start(commandList, static_cast<uint32_t>(TimerCounters::TOTAL_TIME));
        m_cpuTimer->Start(static_cast<uint32_t>(TimerCounters::TOTAL_TIME));

        // Initialize wave counts to 0
        uint32_t iClearValues[BUFFER_TOTAL_ITEMS] = {};
        commandList->ClearUnorderedAccessViewUint(
            m_bufferWavesDescriptorGPU,
            m_bufferWavesDescriptorCPU,
            m_bufferWaves.Get(),
            iClearValues,
            0U,
            nullptr);

        m_currFrameIndex = m_deviceResources->GetCurrentFrameIndex();

        // As the Root Signature is the same for all the mesh parts, setting it once
        m_modelEffect[m_selectedModel].m_effect->SetRootSignature(commandList);

        m_modelEffect[m_selectedModel].m_effect->SetDrawDataBuffer(commandList);

        // Depth pass is the best place for using DrawIndexedX as the pixel shader doesn't
        // exist and all the vertex shader waves take the same path.
        // This helps in packing threads from different draw calls in
        // a single wave without any divergence.
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Depth Pass");
        {
            m_gpuTimer->Start(commandList, static_cast<uint32_t>(TimerCounters::DEPTH_PASS));
            m_cpuTimer->Start(static_cast<uint32_t>(TimerCounters::DEPTH_PASS));
            uint32_t drawCount = 0;
            bool applyEffect = true;
            for (auto& modelMesh : m_modelEffect[m_selectedModel].m_sampleModel->meshes)
            {
                for (auto& meshPart : modelMesh->opaqueMeshParts)
                {
                    if (meshPart->vertexStride != m_modelEffect[m_selectedModel].m_meshVertexStride)
                    {
                        ++drawCount;
                        continue;
                    }
                    if (m_drawUsingDrawIndexedX)
                    {
                        // Using DrawIndexedX

                        if (applyEffect)
                        {
                            applyEffect = false;
                            auto gpuDescriptorHandle = m_modelEffect[m_selectedModel].m_textures->Heap()->GetGPUDescriptorHandleForHeapStart();
                            gpuDescriptorHandle.ptr += m_descrptorIncrementSizeCBVSRVUAV * m_textureDescriptorHeapEntry;
                            m_modelEffect[m_selectedModel].m_effect->SetCurrentStateArgs(m_currFrameIndex, static_cast<uint32_t>(PSODesc::DEPTH_PASS_DRAW_INDEXED_X), gpuDescriptorHandle);
                            m_modelEffect[m_selectedModel].m_effect->Apply(commandList);

                            commandList->IASetPrimitiveTopology(meshPart->primitiveType);

                            m_modelEffect[m_selectedModel].m_effect->SetVertexBufferAsSRV(commandList, m_modelEffect[m_selectedModel].m_singleVertexBufferAddress);
                        }

                        commandList->DrawIndexedX(m_indexBuffer[drawCount].GpuAddress(), m_indexFormat[drawCount], meshPart->indexCount, m_totalVertexOffset[drawCount]);
                    }
                    else
                    {
                        // Using DrawIndexedInstanced

                        auto gpuDescriptorHandle = m_modelEffect[m_selectedModel].m_sampleModel->GetGpuTextureHandleForMaterialIndex(meshPart->materialIndex,
                            m_modelEffect[m_selectedModel].m_textures->Heap(),
                            m_descrptorIncrementSizeCBVSRVUAV,
                            m_textureDescriptorHeapEntry);
                        m_modelEffect[m_selectedModel].m_effect->SetCurrentStateArgs(m_currFrameIndex, static_cast<uint32_t>(PSODesc::DEPTH_PASS_DRAW_INSTANCED), gpuDescriptorHandle);
                        m_modelEffect[m_selectedModel].m_effect->Apply(commandList);
                        meshPart->Draw(commandList);
                    }
                    ++drawCount;
                }
            }

            // Stop the Timers
            m_cpuTimer->Stop(static_cast<uint32_t>(TimerCounters::DEPTH_PASS));
            m_gpuTimer->Stop(commandList, static_cast<uint32_t>(TimerCounters::DEPTH_PASS));
        }

        PIXEndEvent(commandList);

        // Using DrawIndexedX in the Color pass can cause divergence in the threads within a wave.
        // It can still result in overall gain. Test out performance to determine if that's the case in your game.
        // Texture atlasses is a good solution to avoid divergence.
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Color Pass");
        {
            if (m_dsvState != D3D12_RESOURCE_STATE_DEPTH_READ)
            {
                D3D12_RESOURCE_BARRIER dsvWriteToReadbarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(), m_dsvState, D3D12_RESOURCE_STATE_DEPTH_READ);
                commandList->ResourceBarrier(1, &dsvWriteToReadbarrier);
                m_dsvState = D3D12_RESOURCE_STATE_DEPTH_READ;
            }

            m_gpuTimer->Start(commandList, static_cast<uint32_t>(TimerCounters::MESH_RENDER));
            m_cpuTimer->Start(static_cast<uint32_t>(TimerCounters::MESH_RENDER));
            uint32_t drawCount = 0;
            bool applyEffect = true;
            for (auto& modelMesh : m_modelEffect[m_selectedModel].m_sampleModel->meshes)
            {
                for (auto& meshPart : modelMesh->opaqueMeshParts)
                {
                    if (meshPart->vertexStride != m_modelEffect[m_selectedModel].m_meshVertexStride)
                    {
                        ++drawCount;
                        continue;
                    }

                    if (m_drawUsingDrawIndexedX)
                    {
                        // Using DrawIndexedX

                        if (applyEffect)
                        {
                            applyEffect = false;
                            auto gpuDescriptorHandle = m_modelEffect[m_selectedModel].m_textures->Heap()->GetGPUDescriptorHandleForHeapStart();
                            gpuDescriptorHandle.ptr += m_descrptorIncrementSizeCBVSRVUAV * m_textureDescriptorHeapEntry;

                            if (m_showDebugData)
                            {
                                commandList->SetGraphicsRootUnorderedAccessView(static_cast<uint32_t>(SampleEffect::RootParameterIndex::UAV0), m_bufferWaves->GetGPUVirtualAddress());
                                m_modelEffect[m_selectedModel].m_effect->SetCurrentStateArgs(m_currFrameIndex, static_cast<uint32_t>(PSODesc::MESH_RENDER_COLORS_DEBUG_DRAW_INDEXED_X), gpuDescriptorHandle);
                            }
                            else
                            {
                                m_modelEffect[m_selectedModel].m_effect->SetCurrentStateArgs(m_currFrameIndex, static_cast<uint32_t>(PSODesc::MESH_RENDER_DRAW_INDEXED_X), gpuDescriptorHandle);
                            }
                            m_modelEffect[m_selectedModel].m_effect->Apply(commandList);

                            commandList->IASetPrimitiveTopology(meshPart->primitiveType);

#if _XDK_VER < 0x38390818 // Oct 2016 XDK
                            // Bug 8461523 - Driver validation bug - Index buffer needs to be attached for DrawIndexedX,
                            // otherwise it throws a validation error - Fixed in Oct 2016 XDK
                            D3D12_INDEX_BUFFER_VIEW ibv;
                            ibv.BufferLocation = meshPart->indexBuffer.GpuAddress();
                            ibv.SizeInBytes = meshPart->indexBufferSize;
                            ibv.Format = meshPart->indexFormat;
                            commandList->IASetIndexBuffer(&ibv);
#endif
                            m_modelEffect[m_selectedModel].m_effect->SetVertexBufferAsSRV(commandList, m_modelEffect[m_selectedModel].m_singleVertexBufferAddress);
                        }
                        commandList->DrawIndexedX(m_indexBuffer[drawCount].GpuAddress(), m_indexFormat[drawCount], meshPart->indexCount, m_totalVertexOffset[drawCount]);
                    }
                    else
                    {
                        // Using DrawIndexedInstanced

                        auto gpuDescriptorHandle = m_modelEffect[m_selectedModel].m_sampleModel->GetGpuTextureHandleForMaterialIndex(meshPart->materialIndex,
                            m_modelEffect[m_selectedModel].m_textures->Heap(),
                            m_descrptorIncrementSizeCBVSRVUAV,
                            m_textureDescriptorHeapEntry);
                        if (m_showDebugData)
                        {
                            commandList->SetGraphicsRootUnorderedAccessView(static_cast<uint32_t>(SampleEffect::RootParameterIndex::UAV0), m_bufferWaves->GetGPUVirtualAddress());
                            m_modelEffect[m_selectedModel].m_effect->SetCurrentStateArgs(m_currFrameIndex, static_cast<uint32_t>(PSODesc::MESH_RENDER_COLORS_DEBUG), gpuDescriptorHandle);
                        }
                        else
                        {
                            m_modelEffect[m_selectedModel].m_effect->SetCurrentStateArgs(m_currFrameIndex, static_cast<uint32_t>(PSODesc::MESH_RENDER_DRAW_INSTANCED), gpuDescriptorHandle);
                        }
                        m_modelEffect[m_selectedModel].m_effect->Apply(commandList);
                        meshPart->Draw(commandList);
                    }
                    ++drawCount;
                }
            }

            D3D12_RESOURCE_BARRIER dsvReadToWritebarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(), m_dsvState, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            commandList->ResourceBarrier(1, &dsvReadToWritebarrier);
            m_dsvState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

            // Stop the Timers
            m_cpuTimer->Stop(static_cast<uint32_t>(TimerCounters::MESH_RENDER));
            m_gpuTimer->Stop(commandList, static_cast<uint32_t>(TimerCounters::MESH_RENDER));
        }
        PIXEndEvent(commandList);

        if (m_renderDepthBuffer)
        {
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render Depth Buffer");
            {
                D3D12_RESOURCE_BARRIER dsvbarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(), m_dsvState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                commandList->ResourceBarrier(1, &dsvbarrier);
                m_dsvState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

                commandList->SetGraphicsRootDescriptorTable(0, m_cbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
                auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
                commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
                commandList->SetPipelineState(m_fullScreenPSO.Get());
                commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_RECTLIST);
                commandList->IASetVertexBuffers(0, 1, &m_fullScreenVBView);
                commandList->DrawInstanced(3, 1, 0, 0);

                // Depth Buffer Barrier
                D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(), m_dsvState, D3D12_RESOURCE_STATE_DEPTH_WRITE);
                commandList->ResourceBarrier(1, &barrier);
                m_dsvState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            }
            PIXEndEvent(commandList);
        }

        // Stop the total frame time
        m_cpuTimer->Stop(static_cast<uint32_t>(TimerCounters::TOTAL_TIME));
        m_gpuTimer->Stop(commandList, static_cast<uint32_t>(TimerCounters::TOTAL_TIME));

        m_gpuTimer->EndFrame(commandList);

        RenderUI(commandList);
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
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

// Render UI
void Sample::RenderUI(ID3D12GraphicsCommandList * commandList)
{
    uint64_t divergentWaves = 0;
    uint64_t totalWavesVS = 0;
    uint64_t totalWavesPS = 0;

    if (m_showDebugData)
    {
        CD3DX12_RESOURCE_BARRIER barriersBeforeData[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_bufferWaves.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE)
        };
        commandList->ResourceBarrier(_countof(barriersBeforeData), barriersBeforeData);

        // Read the counters for waves
        divergentWaves = m_bufferWavesMapped[BUFFER_OFFSET_DIVERGENT_WAVES_PS];
        totalWavesPS = m_bufferWavesMapped[BUFFER_OFFSET_TOTAL_WAVES_PS];
        totalWavesVS = m_bufferWavesMapped[BUFFER_OFFSET_TOTAL_WAVES_VS];

        CD3DX12_RESOURCE_BARRIER barriersAfterData[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_bufferWaves.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        commandList->ResourceBarrier(_countof(barriersAfterData), barriersAfterData);
    }

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render UI");
    {
        XMVECTOR textPosition = XMVectorSet(50, 50, 0, 1);
        XMVECTOR textColor = Colors::Orange;
        auto outputSize = m_deviceResources->GetOutputSize();
        XMVECTORF32 diffInY = { 0.f, (outputSize.bottom > 1440) ? 60.0f : 35.0f, 0.f, 0.f };

        m_fontBatch->Begin(commandList);

        m_fontText->DrawString(m_fontBatch.get(), g_sampleTitle, textPosition, textColor);
        textPosition = XMVectorAdd(textPosition, diffInY);
        m_fontText->DrawString(m_fontBatch.get(), g_sampleDescription, textPosition, textColor);

        // Total frame time
        {
            textPosition = XMVectorAdd(textPosition, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), L"Total Frame Time", textPosition, textColor);

            wchar_t bufTotalPassTiming[1024] = {};
            XMVECTOR textPositionMoveX = XMVectorAdd(textPosition, XMVectorSet(100, 0, 0, 0));

            swprintf_s(bufTotalPassTiming, _countof(bufTotalPassTiming) - 1, L"GPU Average: %.2f ms", m_gpuTimer->GetAverageMS(static_cast<uint32_t>(TimerCounters::TOTAL_TIME)));
            textPositionMoveX = XMVectorAdd(textPositionMoveX, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufTotalPassTiming, textPositionMoveX, textColor);

            swprintf_s(bufTotalPassTiming, _countof(bufTotalPassTiming) - 1, L"GPU Elapsed: %.2f ms", m_gpuTimer->GetElapsedMS(static_cast<uint32_t>(TimerCounters::TOTAL_TIME)));
            textPositionMoveX = XMVectorAdd(textPositionMoveX, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufTotalPassTiming, textPositionMoveX, textColor);

            swprintf_s(bufTotalPassTiming, _countof(bufTotalPassTiming) - 1, L"CPU Elapsed: %.2f ms", m_cpuTimer->GetElapsedMS(static_cast<uint32_t>(TimerCounters::TOTAL_TIME)));
            textPositionMoveX = XMVectorAdd(textPositionMoveX, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufTotalPassTiming, textPositionMoveX, textColor);

            textPosition = XMVectorSetY(textPosition, XMVectorGetY(textPositionMoveX));
        }

        // Depth pass timings
        {
            textPosition = XMVectorAdd(textPosition, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), L"Depth Pass Timings", textPosition, textColor);

            wchar_t bufDepthPassTiming[1024] = {};
            XMVECTOR textPositionMoveX = XMVectorAdd(textPosition, XMVectorSet(100, 0, 0, 0));
            swprintf_s(bufDepthPassTiming, _countof(bufDepthPassTiming) - 1, L"GPU Average: %.2f ms", m_gpuTimer->GetAverageMS(static_cast<uint32_t>(TimerCounters::DEPTH_PASS)));
            textPositionMoveX = XMVectorAdd(textPositionMoveX, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufDepthPassTiming, textPositionMoveX, textColor);

            swprintf_s(bufDepthPassTiming, _countof(bufDepthPassTiming) - 1, L"GPU Elapsed: %.2f ms", m_gpuTimer->GetElapsedMS(static_cast<uint32_t>(TimerCounters::DEPTH_PASS)));
            textPositionMoveX = XMVectorAdd(textPositionMoveX, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufDepthPassTiming, textPositionMoveX, textColor);

            swprintf_s(bufDepthPassTiming, _countof(bufDepthPassTiming) - 1, L"CPU Elapsed: %.2f ms", m_cpuTimer->GetElapsedMS(static_cast<uint32_t>(TimerCounters::DEPTH_PASS)));
            textPositionMoveX = XMVectorAdd(textPositionMoveX, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufDepthPassTiming, textPositionMoveX, textColor);

            textPosition = XMVectorSetY(textPosition, XMVectorGetY(textPositionMoveX));
        }

        // Mesh render pass timings
        {
            textPosition = XMVectorAdd(textPosition, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), L"Mesh Render Timings", textPosition, textColor);

            wchar_t bufMeshRenderTiming[1024] = {};
            XMVECTOR textPositionMoveX = XMVectorAdd(textPosition, XMVectorSet(100, 0, 0, 0));
            swprintf_s(bufMeshRenderTiming, _countof(bufMeshRenderTiming) - 1, L"GPU Average: %.2f ms", m_gpuTimer->GetAverageMS(static_cast<uint32_t>(TimerCounters::MESH_RENDER)));
            textPositionMoveX = XMVectorAdd(textPositionMoveX, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufMeshRenderTiming, textPositionMoveX, textColor);

            swprintf_s(bufMeshRenderTiming, _countof(bufMeshRenderTiming) - 1, L"GPU Elapsed: %.2f ms", m_gpuTimer->GetElapsedMS(static_cast<uint32_t>(TimerCounters::MESH_RENDER)));
            textPositionMoveX = XMVectorAdd(textPositionMoveX, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufMeshRenderTiming, textPositionMoveX, textColor);

            swprintf_s(bufMeshRenderTiming, _countof(bufMeshRenderTiming) - 1, L"CPU Elapsed: %.2f ms", m_cpuTimer->GetElapsedMS(static_cast<uint32_t>(TimerCounters::MESH_RENDER)));
            textPositionMoveX = XMVectorAdd(textPositionMoveX, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufMeshRenderTiming, textPositionMoveX, textColor);

            textPosition = XMVectorSetY(textPosition, XMVectorGetY(textPositionMoveX));
        }

        wchar_t bufInfo[1024] = {};
        wchar_t drawMethodName[25] = {};
        if (m_drawUsingDrawIndexedX)
            swprintf_s(drawMethodName, _countof(drawMethodName) - 1, L"DrawIndexedX");
        else
            swprintf_s(drawMethodName, _countof(drawMethodName) - 1, L"DrawIndexedInstanced");

        swprintf_s(bufInfo, _countof(bufInfo) - 1, L"[A] - Change draw method - %s", drawMethodName);
        textPosition = XMVectorAdd(textPosition, diffInY);
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), bufInfo, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);

        swprintf_s(bufInfo, _countof(bufInfo) - 1, L"[X] - Use Atlas - %s", m_selectedModel == static_cast<uint32_t>(SelectedModel::ModelAtlas) ? L"True" : L"False");
        textPosition = XMVectorAdd(textPosition, diffInY);
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), bufInfo, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);

        if (m_showDebugData)
            swprintf_s(bufInfo, _countof(bufInfo) - 1, L"[Y] - Turn off wave debug data");
        else
            swprintf_s(bufInfo, _countof(bufInfo) - 1, L"[Y] - Collect wave debug data");
        textPosition = XMVectorAdd(textPosition, diffInY);
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), bufInfo, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);

        if (m_showDebugData)
        {
            wchar_t wavesData[1024] = {};
            swprintf_s(wavesData, _countof(wavesData) - 1, L"Total waves VS: %llu", totalWavesVS);
            textPosition = XMVectorAdd(textPosition, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), wavesData, textPosition, textColor);

            swprintf_s(wavesData, _countof(wavesData) - 1, L"Num of Divergent waves PS: %llu", divergentWaves);
            textPosition = XMVectorAdd(textPosition, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), wavesData, textPosition, textColor);

            swprintf_s(wavesData, _countof(wavesData) - 1, L"Total waves PS: %llu", totalWavesPS);
            textPosition = XMVectorAdd(textPosition, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), wavesData, textPosition, textColor);

            float percentDivergentWavesPS = (totalWavesPS == 0) ? 0 : (divergentWaves * 100.0f) / totalWavesPS;
            swprintf_s(wavesData, _countof(wavesData) - 1, L"Percentage Divergent Waves PS: %.2f%%", percentDivergentWavesPS);
            textPosition = XMVectorAdd(textPosition, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), wavesData, textPosition, textColor);

            swprintf_s(wavesData, _countof(wavesData) - 1, L"Red shows divergent waves");
            textPosition = XMVectorAdd(textPosition, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), wavesData, textPosition, textColor);
        }

        if (m_isXboxOneX)
        {
            textPosition = XMVectorAdd(textPosition, diffInY);
            m_fontText->DrawString(m_fontBatch.get(), L"DrawIndexedX behaves same as DrawIndexedInstanced on Xbox One X and Xbox Series X|S.", textPosition, textColor);
        }

        swprintf_s(bufInfo, _countof(bufInfo) - 1, L"[Menu] - Show Help Screen");
        textPosition = XMVectorAdd(textPosition, diffInY);
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), bufInfo, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);

        // End font batch
        m_fontBatch->End();
    }
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

    m_descrptorIncrementSizeCBVSRVUAV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12XBOX_GPU_HARDWARE_CONFIGURATION hwConfig = {};
    device->GetGpuHardwareConfigurationX(&hwConfig);
    if ((hwConfig.HardwareVersion == D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X) ||
        (hwConfig.HardwareVersion == D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X_DEVKIT))
    {
        m_isXboxOneX = true;
    }
    else
    {
        m_isXboxOneX = false;
    }

    LoadResources(device);

    m_dsvState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());
    m_cpuTimer = std::make_unique<DX::CPUTimer>();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    InitializeCamera();
    auto size = m_deviceResources->GetOutputSize();
    m_help->SetWindow(size);
}

// Intitalize camera and create constant buffer with camera data
void Sample::InitializeCamera()
{
    auto camPos = XMVectorSet(71.9079819f, 16.8714714f, -3.54146695f, 0.f);
    auto camLookAt = XMVectorSet(-374.811951f, 17.7530460f, 17.4464588f, 0.f);
    auto nearPlane = 2.0f;
    auto farPlane = 500.0f;

    for (uint32_t modelNum = 0; modelNum < SelectedModel::NumModels; ++modelNum)
    {
        m_modelEffect[modelNum].m_effect->InitializeCamera(camPos, camLookAt, nearPlane, farPlane, m_deviceResources->GetScreenViewport().Width / m_deviceResources->GetScreenViewport().Height);
    }
}

// Load the meshes
void Sample::LoadMeshes(ID3D12Device * device)
{
    const D3D12_DEPTH_STENCIL_DESC depthReadCompareEqual =
    {
        TRUE,                           // DepthEnable
        D3D12_DEPTH_WRITE_MASK_ZERO,    // DepthWriteMask
        D3D12_COMPARISON_FUNC_EQUAL,    // DepthFunc
        FALSE,                          // StencilEnable
        D3D12_DEFAULT_STENCIL_READ_MASK,
        D3D12_DEFAULT_STENCIL_WRITE_MASK,
        {
            D3D12_STENCIL_OP_KEEP,       // StencilFailOp
            D3D12_STENCIL_OP_KEEP,       // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,       // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
        },                               // FrontFace,
        {
            D3D12_STENCIL_OP_KEEP,       // StencilFailOp
            D3D12_STENCIL_OP_KEEP,       // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,       // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS // StencilFunc
        }                                // BackFace
    };

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    EffectPipelineStateDescription psd[NUM_PSO] = {
        EffectPipelineStateDescription(nullptr, CommonStates::Opaque, CommonStates::DepthDefault, CommonStates::CullCounterClockwise, rtState),		// DEPTH_PASS_DRAW_INSTANCED,
        EffectPipelineStateDescription(nullptr, CommonStates::Opaque, CommonStates::DepthDefault, CommonStates::CullCounterClockwise, rtState),		// DEPTH_PASS_DRAW_INDEXED_X,
        EffectPipelineStateDescription(nullptr, CommonStates::Opaque, depthReadCompareEqual, CommonStates::CullCounterClockwise, rtState),			// MESH_RENDER_DRAW_INSTANCED,
        EffectPipelineStateDescription(nullptr, CommonStates::Opaque, depthReadCompareEqual, CommonStates::CullCounterClockwise, rtState),			// MESH_RENDER_DRAW_INDEXED_X,
        EffectPipelineStateDescription(nullptr, CommonStates::Opaque, depthReadCompareEqual, CommonStates::CullCounterClockwise, rtState), 			// MESH_RENDER_COLORS_DEBUG,
        EffectPipelineStateDescription(nullptr, CommonStates::Opaque, depthReadCompareEqual, CommonStates::CullCounterClockwise, rtState) 			// MESH_RENDER_COLORS_DEBUG_DRAW_INDEXED_X,
    };

    // Load Shaders
    std::vector<uint8_t> meshVSBlob[] =
    {
        DX::ReadData(L"MeshVS.cso"),
        DX::ReadData(L"MeshDrawIndexedXVS.cso"),
        DX::ReadData(L"MeshDebugWavesVS.cso"),
        DX::ReadData(L"MeshDebugWavesDrawIndexedXVS.cso"),
    };
    std::vector<uint8_t> meshPSBlob[] =
    {
        DX::ReadData(L"MeshPS.cso"),
        DX::ReadData(L"MeshDrawIndexedXPS.cso"),
        DX::ReadData(L"MeshDebugWavesPS.cso"),
        DX::ReadData(L"MeshDebugWavesDrawIndexedXPS.cso")
    };
    D3D12_SHADER_BYTECODE vsByteCode[NUM_PSO] = {
        { meshVSBlob[0].data(), meshVSBlob[0].size() },
        { meshVSBlob[1].data(), meshVSBlob[1].size() },
        { meshVSBlob[0].data(), meshVSBlob[0].size() },
        { meshVSBlob[1].data(), meshVSBlob[1].size() },
        { meshVSBlob[2].data(), meshVSBlob[2].size() },
        { meshVSBlob[3].data(), meshVSBlob[3].size() },
    };
    D3D12_SHADER_BYTECODE psByteCode[NUM_PSO] = {
        { nullptr, 0 },
        { nullptr, 0 },
        { meshPSBlob[0].data(), meshPSBlob[0].size() },
        { meshPSBlob[1].data(), meshPSBlob[1].size() },
        { meshPSBlob[2].data(), meshPSBlob[2].size() },
        { meshPSBlob[3].data(), meshPSBlob[3].size() },
    };

    static const wchar_t modelAtlasName[50] = L"Building_Atlas.sdkmesh";
    m_modelEffect[0].m_sampleModel = Model::CreateFromSDKMESH(device, modelAtlasName);
    static const wchar_t modelName[50] = L"Building_Individual.sdkmesh";
    m_modelEffect[1].m_sampleModel = Model::CreateFromSDKMESH(device, modelName);

    for (uint32_t modelNum = 0; modelNum < SelectedModel::NumModels; ++modelNum)
    {
        auto sampleModel = m_modelEffect[modelNum].m_sampleModel.get();

        // The Input Layout is same for all the meshes in the model.
        // Just choosing the first one to create PSO
        auto modelMeshPart = sampleModel->meshes[0]->opaqueMeshParts[0].get();
        D3D12_INPUT_LAYOUT_DESC inputLayoutModel =
        {
            modelMeshPart->vbDecl->data(),
            (UINT)modelMeshPart->vbDecl->size()
        };

        // Store vertex stride of first mesh part as the sample assumes
        // that the vertex stride remains constant throughout the mesh.
        // If the vertex stride changes, a common single vertex will not work.
        m_modelEffect[modelNum].m_meshVertexStride = modelMeshPart->vertexStride;
        for (uint32_t i = 0; i < NUM_PSO; ++i)
        {
            psd[i].inputLayout = inputLayoutModel;
        }

        m_modelEffect[modelNum].m_effect = std::make_unique<SampleEffect>(
            device,
            m_deviceResources->GetBackBufferCount(),
            psd,
            vsByteCode,
            psByteCode);

        sampleModel->LoadTextures(*(m_modelEffect[modelNum].m_textures).get(), int(static_cast<int>(DescriptorHeapEntry::MeshAtlasTexture) + modelNum * c_numTextures));

        // Update draw data for the meshes
        uint32_t totalVertexCount = 0;
        DrawData drawData;
        uint32_t drawCount = 0;
        uint64_t totalVBSizeBytes = 0;
        uint32_t vertexIDBits = __popcnt(VERTEX_ID_MASK);

        // Calculate the total size required to create a single vertex buffer.
        // Also transfer vertex buffer from 16 to 32 bits. This will give extra bits
        // to store the vertexID from a single vertex buffer as well as
        // leave some extra bits to append other information like InstanceID/MaterialID etc.
        for (auto& modelMesh : sampleModel->meshes)
        {
            for (auto& meshPart : modelMesh->opaqueMeshParts)
            {
                drawData.baseVertexID = totalVertexCount + meshPart->vertexOffset;
                drawData.materialID = uint32_t(sampleModel->materials[meshPart->materialIndex].diffuseTextureIndex);
                drawData.drawID = drawCount;
                m_modelEffect[modelNum].m_effect->UpdateDrawData(&drawData);

                // Ignoring meshes which don't have the same vertex buffer format as the first one
                if (meshPart->vertexStride != m_modelEffect[modelNum].m_meshVertexStride)
                {
                    ++drawCount;
                    continue;
                }

                // Total Size of vertex buffer
                totalVBSizeBytes += meshPart->vertexBufferSize;
                m_totalVertexOffset[drawCount] = totalVertexCount + meshPart->vertexOffset;
                totalVertexCount += meshPart->vertexCount;

                // Modify index buffer data to have draw ID
                assert(drawCount <= MAX_DRAWS);
                auto indexBufferMemory = (uint16_t*)meshPart->indexBuffer.Memory();
                // 32-bit format must be 4 aligned
                SharedGraphicsResource newIndexBuffer = GraphicsMemory::Get().Allocate((size_t)(2 * meshPart->indexBufferSize), 4, GraphicsMemory::TAG_INDEX);
                auto newIndexBufferMemory = (uint32_t*)newIndexBuffer.Memory();
                for (size_t i = 0; i < meshPart->indexCount; i += 1)
                {
                    // Only copy values from the index buffer which are being used.
                    // Ignoring all values before the startIndex - These values may have
                    // been used in the previous mesh - Avoiding unneeded values.
                    uint32_t bufferIndex = static_cast<uint32_t>(meshPart->startIndex + i);
                    newIndexBufferMemory[i] = static_cast<uint32_t>(indexBufferMemory[bufferIndex]);
                    newIndexBufferMemory[i] |= (drawCount & MAX_DRAWS) << vertexIDBits;
                }

#ifdef DRAW_INDEXED_X_ONLY
                // If only using DrawIndexedX, we can modify the index buffer in place.
                // We don't do that by default here as the sample also compares the number of waves
                // generated when using DrawIndexedInstanced versus DrawIndexedX in the same run.
                // Modify format and store new values
                meshPart->indexBufferSize = 2 * meshPart->indexBufferSize;
                meshPart->indexFormat = DXGI_FORMAT_R32_UINT;
                meshPart->indexBuffer.Reset();
                meshPart->indexBuffer = newIndexBuffer;
#endif
                m_indexBufferSize[drawCount] = 2 * meshPart->indexBufferSize;
                m_indexFormat[drawCount] = DXGI_FORMAT_R32_UINT;
                m_indexBuffer[drawCount] = newIndexBuffer;

                ++drawCount;
            }
        }

        m_modelEffect[modelNum].m_singleVertexBufferSize = totalVBSizeBytes;

        auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
            m_modelEffect[modelNum].m_singleVertexBufferSize                // UINT64 width,
                                                                            // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
        );	        												        // UINT64 alignment = 0

        DWORD flAllocation = MEM_64K_PAGES | MEM_RESERVE | MEM_COMMIT;
        DWORD flXMemAllocationFlags = XMEM_GRAPHICS;
        DWORD flProtect = PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GRAPHICS_READWRITE;
        m_modelEffect[modelNum].m_singleVertexBufferAddress =
            reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(XMemVirtualAlloc(nullptr,
                m_modelEffect[modelNum].m_singleVertexBufferSize,
                flAllocation, flXMemAllocationFlags, flProtect));
        assert(m_modelEffect[modelNum].m_singleVertexBufferAddress);

        DX::ThrowIfFailed(device->CreatePlacedResourceX(m_modelEffect[modelNum].m_singleVertexBufferAddress,
            &vertexBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_modelEffect[modelNum].m_singleVertexBuffer.ReleaseAndGetAddressOf())));
        m_modelEffect[modelNum].m_singleVertexBuffer->SetName(L"Single Vertex Buffer");

        uint8_t* vertexBufferMapped;
        DX::ThrowIfFailed(m_modelEffect[modelNum].m_singleVertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vertexBufferMapped)));

        // Copy the vertex buffer data for each mesh into the single buffer
        uint64_t currVBOffset = 0;
        drawCount = 0;
        for (auto& modelMesh : sampleModel->meshes)
        {
            for (auto& meshPart : modelMesh->opaqueMeshParts)
            {
                if (meshPart->vertexStride != m_modelEffect[modelNum].m_meshVertexStride) { ++drawCount; continue; }

                assert(currVBOffset + meshPart->vertexBufferSize <= totalVBSizeBytes);

                memcpy((uint8_t*)vertexBufferMapped + currVBOffset, meshPart->vertexBuffer.Memory(), meshPart->vertexBufferSize);

                currVBOffset += meshPart->vertexBufferSize;
                ++drawCount;
            }
        }
    }
}

// Create all the required buffers
void Sample::CreateBuffers(ID3D12Device* device)
{
    uint32_t bufferSize = BUFFER_TOTAL_ITEMS * sizeof(uint32_t);

    DX::ThrowIfFailed(
        CreateUAVBuffer(device, bufferSize, m_bufferWaves.ReleaseAndGetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    SetDebugObjectName(m_bufferWaves.Get(), L"Buffer Count Waves");

    auto cpuHeapStart = m_modelEffect[0].m_textures->Heap()->GetCPUDescriptorHandleForHeapStart();
    auto gpuHeapStart = m_modelEffect[0].m_textures->Heap()->GetGPUDescriptorHandleForHeapStart();

    m_bufferWavesDescriptorCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuHeapStart, static_cast<uint32_t>(DescriptorHeapEntry::UAVCountWavesBuffer), m_descrptorIncrementSizeCBVSRVUAV);
    m_bufferWavesDescriptorGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuHeapStart, static_cast<uint32_t>(DescriptorHeapEntry::UAVCountWavesBuffer), m_descrptorIncrementSizeCBVSRVUAV);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc =
    {
        DXGI_FORMAT_UNKNOWN,                                    // DXGI_FORMAT Format;
        D3D12_UAV_DIMENSION_BUFFER,                             // D3D12_UAV_DIMENSION ViewDimension;
        {
            0,                                                  // UINT64 FirstElement;
            BUFFER_TOTAL_ITEMS,                                 // UINT NumElements;
            sizeof(uint32_t),                                   // UINT StructureByteStride;
            0,                                                  // UINT64 CounterOffsetInBytes;
            D3D12_BUFFER_UAV_FLAG_NONE,                         // D3D12_BUFFER_UAV_FLAGS Flags;
        },                                                      // D3D12_BUFFER_UAV Buffer;
    };
    device->CreateUnorderedAccessView(m_bufferWaves.Get(), nullptr, &uavDesc, m_bufferWavesDescriptorCPU);

    m_bufferWaves->Map(0, nullptr, reinterpret_cast<void**>(&m_bufferWavesMapped));
    for (uint32_t i = 0; i < BUFFER_TOTAL_ITEMS; ++i)
        m_bufferWavesMapped[i] = 0;
}

// Load resources using DirectXTK
void Sample::LoadResources(ID3D12Device * device)
{
    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    for (uint32_t modelNum = 0; modelNum < SelectedModel::NumModels; ++modelNum)
    {
        // Create a texture factory for loading the model textures
        m_modelEffect[modelNum].m_textures = std::make_unique<EffectTextureFactory>(device, resourceUpload, (size_t)DescriptorHeapEntry::TotalDescriptorHeapEntryCount);
    }

    // Load the assets
    LoadMeshes(device);
    CreateBuffers(device);
    InitializeSpriteFonts(device, resourceUpload, rtState);
    m_help->RestoreDevice(device, resourceUpload, rtState);

    // Requires model effect
    CreateFullScreenRectResources(device, resourceUpload);

    auto resourceUploadEvent = resourceUpload.End(m_deviceResources->GetCommandQueue());
    resourceUploadEvent.wait(); // wait for resources to upload
}

// Initialize all the fonts used
void Sample::InitializeSpriteFonts(ID3D12Device* device, ResourceUploadBatch& resourceUpload, const RenderTargetState& rtState)
{
    SpriteBatchPipelineStateDescription pd(
        rtState,
        &CommonStates::AlphaBlend);

    auto viewport = m_deviceResources->GetScreenViewport();
    m_fontBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd, &viewport);

    auto cpuDescHandleText = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_modelEffect[0].m_textures->GetCpuDescriptorHandle(0),
        (int)DescriptorHeapEntry::TextFont,
        m_descrptorIncrementSizeCBVSRVUAV);
    auto gpuDescHandleText = CD3DX12_GPU_DESCRIPTOR_HANDLE(
        m_modelEffect[0].m_textures->GetGpuDescriptorHandle(0),
        (int)DescriptorHeapEntry::TextFont,
        m_descrptorIncrementSizeCBVSRVUAV);

    auto outputSize = m_deviceResources->GetOutputSize();
    m_fontText = std::make_unique<SpriteFont>(
        device,
        resourceUpload,
        (outputSize.bottom > 1440) ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
        cpuDescHandleText,
        gpuDescHandleText);

    auto cpuDescHandleController = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_modelEffect[0].m_textures->GetCpuDescriptorHandle(0),
        (int)DescriptorHeapEntry::ControllerFont,
        m_descrptorIncrementSizeCBVSRVUAV);
    auto gpuDescHandleController = CD3DX12_GPU_DESCRIPTOR_HANDLE(
        m_modelEffect[0].m_textures->GetGpuDescriptorHandle(0),
        (int)DescriptorHeapEntry::ControllerFont,
        m_descrptorIncrementSizeCBVSRVUAV);

    m_fontController = std::make_unique<SpriteFont>(
        device,
        resourceUpload,
        (outputSize.bottom > 1440) ? L"XboxOneController.spritefont" : L"XboxOneControllerSmall.spritefont",
        cpuDescHandleController,
        gpuDescHandleController);
}

// Full screen resources to display the depth buffer
void Sample::CreateFullScreenRectResources(ID3D12Device * device, ResourceUploadBatch& resourceUpload)
{
    // Load Shaders
    std::vector<uint8_t> fullScreenVSBlob = DX::ReadData(L"FullScreenVS.cso");
    std::vector<uint8_t> fullScreenPSBlob = DX::ReadData(L"FullScreenPS.cso");

    D3D12_SHADER_BYTECODE vsByteCode = { fullScreenVSBlob.data(), fullScreenVSBlob.size() };
    D3D12_SHADER_BYTECODE psByteCode = { fullScreenPSBlob.data(), fullScreenPSBlob.size() };

    static const D3D12_INPUT_ELEMENT_DESC s_inputElementDesc[2] =
    {
        { "SV_Position", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,  0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,  0 },
    };

    // Create PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_modelEffect[0].m_effect->GetRootSignature(); // Root signature is the same for all models
    psoDesc.VS = vsByteCode;
    psoDesc.PS = psByteCode;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CommonStates::DepthNone;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.InputLayout = { s_inputElementDesc, _countof(s_inputElementDesc) };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_fullScreenPSO.ReleaseAndGetAddressOf())));

    // Create vertex buffer.
    {
        struct Vertex
        {
            XMFLOAT4 position;
            XMFLOAT2 texcoord;
        };

        static const Vertex s_vertexData[] =
        {
            {{-1.0f, -1.0f, 1.0f, 1.0f},{0.f, 1.f}},
            {{ 1.0f, -1.0f, 1.0f, 1.0f},{1.f, 1.f}},
            {{ 1.0f,  1.0f, 1.0f, 1.0f},{1.f, 0.f}},
        };

        DX::ThrowIfFailed(
            CreateStaticBuffer(device, resourceUpload, s_vertexData, std::size(s_vertexData), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_fullScreenVB.ReleaseAndGetAddressOf()));
        SetDebugObjectName(m_fullScreenVB.Get(), L"Full Screen Vertex Buffer");

        // Initialize the vertex buffer view
        m_fullScreenVBView.BufferLocation = m_fullScreenVB->GetGPUVirtualAddress();
        m_fullScreenVBView.StrideInBytes = sizeof(Vertex);
        m_fullScreenVBView.SizeInBytes = sizeof(s_vertexData);

        // Create descriptor heap for shader resource view
        D3D12_DESCRIPTOR_HEAP_DESC descHeapCbvSrv =
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,		    // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1, // For SRV							        // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,	    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                            // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapCbvSrv, IID_GRAPHICS_PPV_ARGS(m_cbvSrvUavDescriptorHeap.ReleaseAndGetAddressOf())));

        // Create SRV descriptor in the SRV descriptor heap
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(m_deviceResources->GetDepthStencil(), &srvDesc,
            m_cbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    }
}
#pragma endregion
