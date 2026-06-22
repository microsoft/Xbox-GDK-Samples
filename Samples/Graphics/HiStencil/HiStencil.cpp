//--------------------------------------------------------------------------------------
// HiStencil.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// HiStencil is turned off by default on D3D12.X. But the good news is that the title
// can configure HiStencil as per their title needs. There are 2 HiStencil comparison
// states that can be set (State0 and State1).
// SetHiStencilStateX and SetHiStencilControlX APIs have been added to D3D12.X
// to help set these HiStencil comparison states.
// * The SetHiStencilStateX is used to set the HiStencil compare test on a depth stencil
// resource and will persist on a given resource when it is bound.
// * The SetHiStencilControlX is used to temporarily set the state globally and will be
// used for any stencil resource until a depth stencil with another HiStencil state
// is bound to the pipeline.
//
// The results of the comparison are stored in the HTile buffer. These tests when
// correlated to a stencil test later in the frame, can be an effective way to
// reject/accept tiles instead of reading individual sample stencil values.
//
// Please refer to ReadMe.docx for more information
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "HiStencil.h"

#include "ATGColors.h"
#include "CommonHeader.h"
#include "ControllerFont.h"
#include "ReadData.h"

extern void ExitSample();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
#define COMPARE_VALUE 0x3F
#define COMPARE_VALUE_2 0x2

    struct Vertex
    {
        XMFLOAT4 position;
        XMFLOAT4 color;
    };

    struct Instance
    {
        XMFLOAT4 scale;
        XMFLOAT4 translate;
    };

    enum class DSV_HEAP
    {
        DepthBuffer,
        DepthBufferEsram,
        TotalNum
    };

    enum class RTV_HEAP
    {
        RTBuffer,
        RTBufferEsram,
        TotalNum
    };

    struct ScaleTranslate
    {
        XMFLOAT4 scale;
        XMFLOAT4 translate;
    };

    struct HTileDesc
    {
        uint32_t hTilePitch;
        uint32_t isHTileLinear;
    };

    enum class Timers
    {
        RenderTimer,
        DispatchTimer,
        TransitionTimer,
        ExecuteIndirectTimer,
        StencilPassPsTimer,
        ResummarizeTimer
    };

    const wchar_t* g_sampleTitle = L"Hi-stencil Sample - Peek inside Bot";
    const wchar_t* g_sampleDescription = L"Sample showing hi-stencil buffer usage on Xbox One";
    constexpr ATG::HelpButtonAssignment g_helpButtons[] = {
        { ATG::HelpID::MENU_BUTTON,         L"Show/hide help" },
        { ATG::HelpID::VIEW_BUTTON,         L"Exit" },
        { ATG::HelpID::A_BUTTON,            L"Hi-stencil On/Off" },
        { ATG::HelpID::X_BUTTON,            L"Pixel Shader/Compute Shader" },
        { ATG::HelpID::B_BUTTON,            L"HiStencilControlX" },
        { ATG::HelpID::Y_BUTTON,            L"Resummarize" },
        { ATG::HelpID::LEFT_SHOULDER,       L"Use ESRAM if available" },
        { ATG::HelpID::RIGHT_SHOULDER,      L"Compute Shader Debug values" },
        { ATG::HelpID::DPAD_RIGHT,          L"Set DSV to override HiStencilControlX" },
        { ATG::HelpID::DPAD_LEFT,           L"HTile values prior to exterior render" },
        { ATG::HelpID::RIGHT_STICK,         L"Camera Rotate" },
        { ATG::HelpID::LEFT_STICK,          L"Camera Translate" },
    };
};

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_dsvState(D3D12_RESOURCE_STATE_COMMON),
    m_dsvStateEsram(D3D12_RESOURCE_STATE_COMMON),
    m_hTileState(D3D12_RESOURCE_STATE_COMMON),
    m_rtBufferState(D3D12_RESOURCE_STATE_COMMON),
    m_rtBufferStateEsram(D3D12_RESOURCE_STATE_COMMON),
    m_rectVBView{},
    m_xgResLayout{},
    m_depthPlane(nullptr),
    m_stencilPlane(nullptr),
    m_hTilePlane(nullptr),
    m_hwConfig{},
    m_mayPassAppendBufferDescriptorCPU{},
    m_mayPassCountBufferDescriptorCPU{},
    m_mayPassAppendBufferDescriptorGPU{},
    m_mayPassCountBufferDescriptorGPU{},
    m_mayPassCountBufferMapped(nullptr),
    m_mayFailAppendBufferDescriptorCPU{},
    m_mayFailCountBufferDescriptorCPU{},
    m_mayFailAppendBufferDescriptorGPU{},
    m_mayFailCountBufferDescriptorGPU{},
    m_mayFailCountBufferMapped(nullptr),
    m_smemSingleValueAppendBufferDescriptorCPU{},
    m_smemSingleValueCountBufferDescriptorCPU{},
    m_smemSingleValueAppendBufferDescriptorGPU{},
    m_smemSingleValueCountBufferDescriptorGPU{},
    m_smemSingleValueCountBufferMapped(nullptr),
    m_renderTargetUAVGPU{},
    m_renderTargetEsramUAVGPU{},
    m_stencilCBDataCPU(nullptr),
    m_renderUsingCs(false),
    m_hiSOn(true),
    m_hiStencilControlEnable(false),
    m_resummarize(false),
    m_showCount(false),
    m_isEsramAvailable(false),
    m_inEsram(false),
    m_dispatchBeforeOccluder(false),
    m_setDSVToOverrideHiSControlX(false),
    m_uavCpuHeapStart{},
    m_uavGpuHeapStart{},
    m_showHelp(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT, 2, DX::DeviceResources::c_Enable4K_UHD);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_help = std::make_unique<ATG::Help>(g_sampleTitle, g_sampleDescription, g_helpButtons, std::size(g_helpButtons));
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

    auto pad = m_gamePad->GetState(0);
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
        if (!m_renderUsingCs && (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED))
        {
            m_hiSOn = !m_hiSOn;
            m_gpuTimer.Reset();
            m_cpuTimer.Reset();
        }
        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            m_renderUsingCs = !m_renderUsingCs;
            // Turn on HiStencil for Compute shader
            if (m_renderUsingCs)
                m_hiSOn = true;
            m_gpuTimer.Reset();
            m_cpuTimer.Reset();
        }
        if (m_renderUsingCs && m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            m_resummarize = !m_resummarize;
            m_gpuTimer.Reset();
            m_cpuTimer.Reset();
        }
        if (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::PRESSED)
        {
            m_showCount = !m_showCount;
            m_gpuTimer.Reset();
            m_cpuTimer.Reset();
        }
        if (m_isEsramAvailable && m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::PRESSED)
        {
            m_inEsram = !m_inEsram;
            m_gpuTimer.Reset();
            m_cpuTimer.Reset();
        }
        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_hiStencilControlEnable = !m_hiStencilControlEnable;
            m_gpuTimer.Reset();
            m_cpuTimer.Reset();
        }
        if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
        {
            m_dispatchBeforeOccluder = !m_dispatchBeforeOccluder;
            m_gpuTimer.Reset();
            m_cpuTimer.Reset();
        }

        if (m_hiStencilControlEnable && m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
        {
            m_setDSVToOverrideHiSControlX = !m_setDSVToOverrideHiSControlX;
            m_gpuTimer.Reset();
            m_cpuTimer.Reset();
        }

        m_camera->Update(elapsedTime, pad);
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    auto viewProj = m_camera->GetView() * m_camera->GetProjection();
    for (uint32_t index = 0; index < static_cast<uint32_t>(NUM_EFFECTS::TotalEffects); ++index)
    {
        m_effect[index]->UpdateConstants(viewProj);
    }

    for (uint32_t index = 0; index < static_cast<uint32_t>(NUM_MESHES::TotalIntExtMeshes); ++index)
    {
        m_effectPbr[index]->SetView(m_camera->GetView());
        m_effectPbr[index]->SetProjection(m_camera->GetProjection());
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

    auto graphicsCmdList = m_deviceResources->GetCommandList();

    auto currDepthStencilBuffer = m_inEsram ? m_depthStencilBufferEsram.Get() : m_depthStencilBuffer.Get();
    auto& currDsvState = m_inEsram ? m_dsvStateEsram : m_dsvState;
    auto currRtvBuffer = m_inEsram ? m_rtBufferEsram.Get() : m_rtBuffer.Get();
    auto& currRtState = m_inEsram ? m_rtBufferStateEsram : m_rtBufferState;

    if (!(currDsvState & D3D12_RESOURCE_STATE_DEPTH_WRITE) || currRtState != D3D12_RESOURCE_STATE_RENDER_TARGET)
    {
        D3D12_RESOURCE_BARRIER* barrier = new D3D12_RESOURCE_BARRIER[2];
        uint32_t barrierIndex = 0;
        if (!(currDsvState & D3D12_RESOURCE_STATE_DEPTH_WRITE))
        {
            barrier[barrierIndex++] = CD3DX12_RESOURCE_BARRIER::Transition(currDepthStencilBuffer, currDsvState, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            currDsvState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        }
        if (currRtState != D3D12_RESOURCE_STATE_RENDER_TARGET)
        {
            barrier[barrierIndex++] = CD3DX12_RESOURCE_BARRIER::Transition(currRtvBuffer, currRtState, D3D12_RESOURCE_STATE_RENDER_TARGET);
            currRtState = D3D12_RESOURCE_STATE_RENDER_TARGET;
        }

        graphicsCmdList->ResourceBarrier(barrierIndex, barrier);

        delete[] barrier;
    }
    Clear();

    m_gpuTimer.BeginFrame(graphicsCmdList);

    if (m_showHelp)
    {
        auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
        graphicsCmdList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
        m_help->Render(graphicsCmdList);
    }
    else
    {
        m_cpuTimer.Start(static_cast<uint32_t>(Timers::RenderTimer));
        m_gpuTimer.Start(graphicsCmdList, static_cast<uint32_t>(Timers::RenderTimer));

        PIXScopedEvent(graphicsCmdList, PIX_COLOR_DEFAULT, L"Render");

        for (uint32_t index = 0; index < static_cast<uint32_t>(NUM_EFFECTS::TotalEffects); ++index)
        {
            m_effect[index]->SetAllDirtyFlags();
        }

        // Root Signature is same for all meshes not using PBREffect
        graphicsCmdList->SetGraphicsRootSignature(m_effect[static_cast<uint32_t>(NUM_EFFECTS::Disc)]->GetRootSignature());

        D3D12XBOX_HISTENCIL_CONTROL hiStencilControl = {};
        if (m_hiSOn)
        {
            hiStencilControl.State0.Enabled = TRUE;
            hiStencilControl.State0.CompareFunction = D3D12XBOX_HISTENCIL_COMPARE_FUNCTION_LEQUAL;
            hiStencilControl.State0.CompareValue = COMPARE_VALUE;
            hiStencilControl.State0.CompareMask = 0xFF;
            hiStencilControl.State1.Enabled = FALSE;
            hiStencilControl.State1.CompareFunction = D3D12XBOX_HISTENCIL_COMPARE_FUNCTION_LEQUAL;
            hiStencilControl.State1.CompareValue = 0;
            hiStencilControl.State1.CompareMask = 0xFF;
            // SetHiStencilStateX is set on a resource, so it will be used whenever this resource is bound
            graphicsCmdList->SetHiStencilStateX(currDepthStencilBuffer, &hiStencilControl);

            m_stencilCBDataCPU[0] = COMPARE_VALUE;
        }
        else
        {
            // Clear Hi-Stencil value
            hiStencilControl.State0.Enabled = FALSE;
            hiStencilControl.State1.Enabled = FALSE;
            graphicsCmdList->SetHiStencilStateX(currDepthStencilBuffer, &hiStencilControl);
        }

        {
            PIXScopedEvent(graphicsCmdList, PIX_COLOR_DEFAULT, L"Render Cutouts");

            {
                // Draw disc
                // Write to stencil buffer with Stencil value = COMPARE_VALUE
                graphicsCmdList->OMSetStencilRef(COMPARE_VALUE);
                D3D12_GPU_DESCRIPTOR_HANDLE meshTexGpuDescriptorHandle = {};
                m_effect[static_cast<uint32_t>(NUM_EFFECTS::Disc)]->SetDescriptors(graphicsCmdList, m_deviceResources->GetCurrentFrameIndex(), meshTexGpuDescriptorHandle);
                m_effect[static_cast<uint32_t>(NUM_EFFECTS::Disc)]->Apply(graphicsCmdList);
                m_model[static_cast<uint32_t>(NUM_MESHES::Disc1)]->Draw(graphicsCmdList);

                // Write to stencil buffer with Stencil value = COMPARE_VALUE + 1
                graphicsCmdList->OMSetStencilRef(COMPARE_VALUE + 1);
                m_model[static_cast<uint32_t>(NUM_MESHES::Disc2)]->Draw(graphicsCmdList);
            }

            if (m_hiStencilControlEnable)
            {
                uint32_t stencilCompareValue = COMPARE_VALUE_2 + 1;

                // Change SR1 using Hi-Stencil Control to include more stencil values in the hi-stencil pass
                // Keep SR0 comparison as it is
                // SetHiStencilControlX is used to set the GPU State directly and will be used for any stencil buffer that is
                // used after this. But if we set the Stencil resource again which has a pre-defined stencil test (using
                // SetHiStencilStateX), then the test set using SetHiStencilStateX will be used.
                hiStencilControl.State1.Enabled = TRUE;
                hiStencilControl.State1.CompareFunction = D3D12XBOX_HISTENCIL_COMPARE_FUNCTION_LEQUAL;
                hiStencilControl.State1.CompareValue = stencilCompareValue;
                hiStencilControl.State1.CompareMask = 0xFF;
                graphicsCmdList->SetHiStencilControlX(&hiStencilControl);

                m_stencilCBDataCPU[0] = stencilCompareValue;

                // If the resource is set again, the HiStencil parameters set on the resource using SetHiStencilStateX is used
                // instead of the one set using SetHiStencilControlX above
                if (m_setDSVToOverrideHiSControlX)
                {
                    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = m_inEsram ? m_rtvHeap->GetCpuHandle(static_cast<uint32_t>(RTV_HEAP::RTBufferEsram)) : m_rtvHeap->GetCpuHandle(static_cast<uint32_t>(RTV_HEAP::RTBuffer));
                    auto dsvDescriptor = m_inEsram ? m_dsvHeap->GetCpuHandle(static_cast<uint32_t>(DSV_HEAP::DepthBufferEsram)) : m_dsvHeap->GetCpuHandle(static_cast<uint32_t>(DSV_HEAP::DepthBuffer));
                    graphicsCmdList->OMSetRenderTargets(1, &(rtvDescriptor), FALSE, &dsvDescriptor);
                }
            }

            // Write to stencil buffer with Stencil value = COMPARE_VALUE_NO_DRAW
            graphicsCmdList->OMSetStencilRef(COMPARE_VALUE_2);
            m_model[static_cast<uint32_t>(NUM_MESHES::Disc3)]->Draw(graphicsCmdList);

            graphicsCmdList->OMSetStencilRef(COMPARE_VALUE_2 + 1);
            m_model[static_cast<uint32_t>(NUM_MESHES::Disc4)]->Draw(graphicsCmdList);

            // Dispatch to read the HTile buffer before rendering the meshes, which modify the stencil state when reading it
            if (m_dispatchBeforeOccluder && m_renderUsingCs)
            {
                DispatchToCountHiSValues(graphicsCmdList);
                SingleTransition(graphicsCmdList, currDepthStencilBuffer, currDsvState, D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12XBOX_RESOURCE_STATE_PRESERVE_COMPRESSED_DEPTH_STENCIL);
            }
        }

        RenderExteriorMesh(graphicsCmdList);
        RenderInteriorMesh(graphicsCmdList);

        graphicsCmdList->SetGraphicsRootSignature(m_effect[static_cast<uint32_t>(NUM_EFFECTS::Disc)]->GetRootSignature());

        if (m_renderUsingCs)
        {
            if (!m_dispatchBeforeOccluder)
            {
                DispatchToCountHiSValues(graphicsCmdList);
            }
            else
            {
                SingleTransition(graphicsCmdList, currDepthStencilBuffer, currDsvState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12XBOX_RESOURCE_STATE_PRESERVE_COMPRESSED_DEPTH_STENCIL);
            }

            // Resource Transition
            m_gpuTimer.Start(graphicsCmdList, static_cast<uint32_t>(Timers::TransitionTimer));
            {
                D3D12_RESOURCE_BARRIER barrier[2];
                barrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_mayPassCountBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                barrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_mayPassAppendBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                graphicsCmdList->ResourceBarrier(2, barrier);
            }
            m_gpuTimer.Stop(graphicsCmdList, static_cast<uint32_t>(Timers::TransitionTimer));

            // Ignoring the transition from the timing as we need the uncompressed version of the render target to write from Compute shader
            SingleTransition(graphicsCmdList, currRtvBuffer, currRtState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            RenderStencilPixelsComputeShader(graphicsCmdList);

            // Resource Transition
            {
                PIXScopedEvent(graphicsCmdList, PIX_COLOR_DEFAULT, L"Transition Barrier");
                D3D12_RESOURCE_STATES depthAfterState = D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12XBOX_RESOURCE_STATE_PRESERVE_COMPRESSED_DEPTH_STENCIL;
                uint32_t numBarriers = 0;
                D3D12_RESOURCE_BARRIER* barrier = new D3D12_RESOURCE_BARRIER[4]();
                barrier[numBarriers++] = CD3DX12_RESOURCE_BARRIER::Transition(m_mayPassCountBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                barrier[numBarriers++] = CD3DX12_RESOURCE_BARRIER::Transition(m_mayPassAppendBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                currRtState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                barrier[numBarriers++] = CD3DX12_RESOURCE_BARRIER::Transition(currRtvBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, currRtState);
                if (depthAfterState != currDsvState)
                    barrier[numBarriers++] = CD3DX12_RESOURCE_BARRIER::Transition(currDepthStencilBuffer, currDsvState, depthAfterState);
                currDsvState = depthAfterState;
                graphicsCmdList->ResourceBarrier(numBarriers, barrier);
                delete[] barrier;
            }
        }
        else
        {
            RenderStencilPixelsPixelShader(graphicsCmdList);

            SingleTransition(graphicsCmdList, currRtvBuffer, currRtState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }

        {
            // Copy Intermediate target to Backbuffer
            PIXScopedEvent(graphicsCmdList, PIX_COLOR_DEFAULT, L"Copy Intermediate target to Backbuffer");
            auto copyRectRtvDescriptor = m_deviceResources->GetRenderTargetView();
            graphicsCmdList->OMSetRenderTargets(1, &copyRectRtvDescriptor, FALSE, nullptr);
            RenderRect(graphicsCmdList, false);
        }

        m_gpuTimer.Stop(graphicsCmdList, static_cast<uint32_t>(Timers::RenderTimer));
        m_cpuTimer.Stop(static_cast<uint32_t>(Timers::RenderTimer));

        m_gpuTimer.EndFrame(graphicsCmdList);

        RenderUI(graphicsCmdList);
    }

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
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor[2] =
    {
        m_inEsram ? m_rtvHeap->GetCpuHandle(static_cast<uint32_t>(RTV_HEAP::RTBufferEsram)) : m_rtvHeap->GetCpuHandle(static_cast<uint32_t>(RTV_HEAP::RTBuffer)),
        m_deviceResources->GetRenderTargetView()
    };
    auto const dsvDescriptor = m_inEsram ? m_dsvHeap->GetCpuHandle(static_cast<uint32_t>(DSV_HEAP::DepthBufferEsram)) : m_dsvHeap->GetCpuHandle(static_cast<uint32_t>(DSV_HEAP::DepthBuffer));

    commandList->OMSetRenderTargets(2, rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor[0], ATG::Colors::Background, 0, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor[1], ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    commandList->OMSetRenderTargets(1, &(rtvDescriptor[0]), FALSE, &dsvDescriptor);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

void Sample::RenderExteriorMesh(ID3D12GraphicsCommandList * graphicsCmdList)
{
    PIXScopedEvent(graphicsCmdList, PIX_COLOR_DEFAULT, L"Render Exterior Mesh");

    auto texHeapGpuDescriptorHandle = m_textures->Heap()->GetGPUDescriptorHandleForHeapStart();

    // Draw mesh pixels only when cut-outs from above are not present in stencil. RefValue > CurrentStencilBufferValue
    graphicsCmdList->OMSetStencilRef(COMPARE_VALUE_2);

    D3D12_GPU_DESCRIPTOR_HANDLE albedoGpuDescriptorHandle, normalGpuDescriptorHandle, RMAGpuDescriptorHandle;
    albedoGpuDescriptorHandle.ptr = texHeapGpuDescriptorHandle.ptr + D3D12XBOX_DESCRIPTOR_SHADER_RESOURCE_VIEW_SIZE_IN_BYTES * static_cast<size_t>(CommonHeader::DescriptorHeapEntry::MeshTextureExterior);
    normalGpuDescriptorHandle.ptr = albedoGpuDescriptorHandle.ptr + D3D12XBOX_DESCRIPTOR_SHADER_RESOURCE_VIEW_SIZE_IN_BYTES;
    RMAGpuDescriptorHandle.ptr = normalGpuDescriptorHandle.ptr + D3D12XBOX_DESCRIPTOR_SHADER_RESOURCE_VIEW_SIZE_IN_BYTES;
    m_effectPbr[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)]->SetSurfaceTextures(
        albedoGpuDescriptorHandle,
        normalGpuDescriptorHandle,
        RMAGpuDescriptorHandle,
        m_commonStates->AnisotropicClamp());
    D3D12_GPU_DESCRIPTOR_HANDLE radianceGpuDescriptorHandle, irradianceGpuDescriptorHandle;
    radianceGpuDescriptorHandle.ptr = RMAGpuDescriptorHandle.ptr + D3D12XBOX_DESCRIPTOR_SHADER_RESOURCE_VIEW_SIZE_IN_BYTES;
    irradianceGpuDescriptorHandle.ptr = radianceGpuDescriptorHandle.ptr + D3D12XBOX_DESCRIPTOR_SHADER_RESOURCE_VIEW_SIZE_IN_BYTES;
    m_effectPbr[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)]->SetIBLTextures(
        radianceGpuDescriptorHandle,
        1,
        irradianceGpuDescriptorHandle,
        m_commonStates->LinearClamp());
    m_effectPbr[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)]->Apply(graphicsCmdList);
    m_model[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)]->Draw(graphicsCmdList);
}

void Sample::RenderInteriorMesh(ID3D12GraphicsCommandList * graphicsCmdList)
{
    PIXScopedEvent(graphicsCmdList, PIX_COLOR_DEFAULT, L"Render Interior Mesh");

    auto texHeapGpuDescriptorHandle = m_textures->Heap()->GetGPUDescriptorHandleForHeapStart();

    size_t textureHeapOffsetInteriorMeshes[static_cast<uint32_t>(NUM_MESHES::TotalIntExtMeshes)];
    textureHeapOffsetInteriorMeshes[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Furnishing)] = static_cast<size_t>(CommonHeader::DescriptorHeapEntry::MeshTextureInteriorFurnishings);
    textureHeapOffsetInteriorMeshes[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Gears)] = static_cast<size_t>(CommonHeader::DescriptorHeapEntry::MeshTextureInteriorGears);
    textureHeapOffsetInteriorMeshes[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Robot)] = static_cast<size_t>(CommonHeader::DescriptorHeapEntry::MeshTextureInteriorRobot);

    D3D12_GPU_DESCRIPTOR_HANDLE radianceGpuDescriptorHandle, irradianceGpuDescriptorHandle;
    radianceGpuDescriptorHandle.ptr = texHeapGpuDescriptorHandle.ptr + D3D12XBOX_DESCRIPTOR_SHADER_RESOURCE_VIEW_SIZE_IN_BYTES * static_cast<size_t>(CommonHeader::DescriptorHeapEntry::MeshTextureRadiance);
    irradianceGpuDescriptorHandle.ptr = radianceGpuDescriptorHandle.ptr + D3D12XBOX_DESCRIPTOR_SHADER_RESOURCE_VIEW_SIZE_IN_BYTES;

    for (uint32_t index = static_cast<uint32_t>(NUM_MESHES::TotalExteriorMeshes); index < static_cast<uint32_t>(NUM_MESHES::TotalIntExtMeshes); ++index)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE albedoGpuDescriptorHandle, normalGpuDescriptorHandle, RMAGpuDescriptorHandle;
        albedoGpuDescriptorHandle.ptr = texHeapGpuDescriptorHandle.ptr + D3D12XBOX_DESCRIPTOR_SHADER_RESOURCE_VIEW_SIZE_IN_BYTES * textureHeapOffsetInteriorMeshes[index];
        normalGpuDescriptorHandle.ptr = albedoGpuDescriptorHandle.ptr + D3D12XBOX_DESCRIPTOR_SHADER_RESOURCE_VIEW_SIZE_IN_BYTES;
        RMAGpuDescriptorHandle.ptr = normalGpuDescriptorHandle.ptr + D3D12XBOX_DESCRIPTOR_SHADER_RESOURCE_VIEW_SIZE_IN_BYTES;
        m_effectPbr[index]->SetSurfaceTextures(
            albedoGpuDescriptorHandle,
            normalGpuDescriptorHandle,
            RMAGpuDescriptorHandle,
            m_commonStates->AnisotropicClamp());
        m_effectPbr[index]->SetIBLTextures(
            radianceGpuDescriptorHandle,
            1,
            irradianceGpuDescriptorHandle,
            m_commonStates->LinearClamp());
        m_effectPbr[index]->Apply(graphicsCmdList);
        m_model[index]->Draw(graphicsCmdList);
    }
}

void Sample::RenderStencilPixelsComputeShader(ID3D12GraphicsCommandList* graphicsCmdList)
{
    m_gpuTimer.Start(graphicsCmdList, static_cast<uint32_t>(Timers::ExecuteIndirectTimer));

    // Use ExecuteIndirect to draw pixels that have passed stencil
    {
        PIXScopedEvent(graphicsCmdList, PIX_COLOR_DEFAULT, L"Write to target when Stencil passes");

        graphicsCmdList->SetPipelineState(m_writeStencilPixelsPSO.Get());
        graphicsCmdList->SetComputeRootShaderResourceView(static_cast<uint32_t>(CommonHeader::RootParameterIndex::SRV1), m_mayPassAppendBuffer->GetGPUVirtualAddress());
        graphicsCmdList->SetComputeRootDescriptorTable(static_cast<uint32_t>(CommonHeader::RootParameterIndex::DescriptorTableSRV),
            m_textures->GetGpuDescriptorHandle(static_cast<size_t>(m_inEsram ? CommonHeader::DescriptorHeapEntry::DepthBufferEsram : CommonHeader::DescriptorHeapEntry::DepthBuffer)));
        graphicsCmdList->SetComputeRootDescriptorTable(static_cast<uint32_t>(CommonHeader::RootParameterIndex::DescriptorTableUAV), m_inEsram ? m_renderTargetEsramUAVGPU : m_renderTargetUAVGPU);
        graphicsCmdList->SetComputeRootConstantBufferView(static_cast<uint32_t>(CommonHeader::RootParameterIndex::ConstantBuffer), m_stencilDataCB->GetGPUVirtualAddress());

        graphicsCmdList->ExecuteIndirect(m_commandSignature.Get(), 1, m_mayPassCountBuffer.Get(), 0, nullptr, 0);
    }
    m_gpuTimer.Stop(graphicsCmdList, static_cast<uint32_t>(Timers::ExecuteIndirectTimer));
}

void Sample::RenderStencilPixelsPixelShader(ID3D12GraphicsCommandList* graphicsCmdList)
{
    PIXScopedEvent(graphicsCmdList, PIX_COLOR_DEFAULT, L"Stencil Pass");
    m_gpuTimer.Start(graphicsCmdList, static_cast<uint32_t>(Timers::StencilPassPsTimer));
    auto writePassStencilRtvDescriptor = m_inEsram ? m_rtvHeap->GetCpuHandle(static_cast<uint32_t>(RTV_HEAP::RTBufferEsram)) : m_rtvHeap->GetCpuHandle(static_cast<uint32_t>(RTV_HEAP::RTBuffer));
    auto writePassStencilDsvDescriptor = m_inEsram ? m_dsvHeap->GetCpuHandle(static_cast<uint32_t>(DSV_HEAP::DepthBufferEsram)) : m_dsvHeap->GetCpuHandle(static_cast<uint32_t>(DSV_HEAP::DepthBuffer));
    graphicsCmdList->OMSetRenderTargets(1, &writePassStencilRtvDescriptor, FALSE, &writePassStencilDsvDescriptor);
    const float blendFactor[4] = { 0.5, 0.5, 0.5, 1 };
    graphicsCmdList->OMSetBlendFactor(blendFactor);
    graphicsCmdList->OMSetStencilRef(m_stencilCBDataCPU[0]);
    RenderRect(graphicsCmdList, true);
    m_gpuTimer.Stop(graphicsCmdList, static_cast<uint32_t>(Timers::StencilPassPsTimer));
}

void Sample::RenderRect(ID3D12GraphicsCommandList* graphicsCmdList, bool withStencil)
{
    graphicsCmdList->SetPipelineState(withStencil ? m_rectWithStencilPso.Get() : m_rectPso.Get());
    graphicsCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_RECTLIST);
    graphicsCmdList->IASetVertexBuffers(0, 1, &m_rectVBView);
    auto viewport = m_deviceResources->GetScreenViewport();

    graphicsCmdList->RSSetViewports(1, &viewport);
    graphicsCmdList->SetGraphicsRootDescriptorTable(static_cast<uint32_t>(CommonHeader::RootParameterIndex::DescriptorTableSRV),
        m_textures->GetGpuDescriptorHandle(static_cast<size_t>(m_inEsram ? CommonHeader::DescriptorHeapEntry::IntermediateTargetEsram : CommonHeader::DescriptorHeapEntry::IntermediateTarget)));

    graphicsCmdList->DrawInstanced(3, 1, 0, 0);
}

void Sample::SingleTransition(ID3D12GraphicsCommandList* graphicsCmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES& beforeState, D3D12_RESOURCE_STATES afterState)
{
    if (beforeState != afterState)
    {
        D3D12_RESOURCE_BARRIER barrier;
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, beforeState, afterState);
        beforeState = afterState;
        graphicsCmdList->ResourceBarrier(1, &barrier);
    }
}

// Dispatch compute shader (CS) to count the number of "May Pass", "May Fail" and single SMem values
// in the HTile buffer. The CS also fills up an Append buffer with pixel indices which have "May Pass" in the HTile buffer
void Sample::DispatchToCountHiSValues(ID3D12GraphicsCommandList* graphicsCmdList)
{
    PIXScopedEvent(graphicsCmdList, PIX_COLOR_DEFAULT, L"Read Hi-Stencil using Compute");
    auto currDepthStencilBuffer = m_inEsram ? m_depthStencilBufferEsram.Get() : m_depthStencilBuffer.Get();
    auto& currDsvState = m_inEsram ? m_dsvStateEsram : m_dsvState;

    // Resummarize
    if (m_resummarize)
    {
        m_gpuTimer.Start(graphicsCmdList, static_cast<uint32_t>(Timers::ResummarizeTimer));
        graphicsCmdList->CopyResourceX(currDepthStencilBuffer, currDepthStencilBuffer, D3D12XBOX_COPY_FLAG_RESUMMARIZE_HTILE);
        m_gpuTimer.Stop(graphicsCmdList, static_cast<uint32_t>(Timers::ResummarizeTimer));
    }

    m_gpuTimer.Start(graphicsCmdList, static_cast<uint32_t>(Timers::DispatchTimer));
    {
        PIXScopedEvent(graphicsCmdList, PIX_COLOR_DEFAULT, L"Count May Pass Tiles");
        // Initialize ll UAVs
        uint32_t iClearValues[4] = { 0, 1, 1, 1 };
        graphicsCmdList->ClearUnorderedAccessViewUint(
            m_mayPassCountBufferDescriptorGPU,
            m_mayPassCountBufferDescriptorCPU,
            m_mayPassCountBuffer.Get(),
            iClearValues,
            0U,
            nullptr);

        if (m_showCount)
        {
            graphicsCmdList->ClearUnorderedAccessViewUint(
                m_mayFailCountBufferDescriptorGPU,
                m_mayFailCountBufferDescriptorCPU,
                m_mayFailCountBuffer.Get(),
                iClearValues,
                0U,
                nullptr);
            graphicsCmdList->ClearUnorderedAccessViewUint(
                m_smemSingleValueCountBufferDescriptorGPU,
                m_smemSingleValueCountBufferDescriptorCPU,
                m_smemSingleValueCountBuffer.Get(),
                iClearValues,
                0U,
                nullptr);
        }

        SingleTransition(graphicsCmdList, currDepthStencilBuffer, currDsvState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12XBOX_RESOURCE_STATE_PRESERVE_COMPRESSED_DEPTH_STENCIL);

        // Using the same root signature
        graphicsCmdList->SetComputeRootSignature(m_effect[static_cast<uint32_t>(NUM_EFFECTS::Disc)]->GetRootSignature());
        if (m_showCount)
        {
            if (m_hiStencilControlEnable)
                graphicsCmdList->SetPipelineState(m_readHTileSr0Sr1WithCountersComputePSO.Get());
            else
                graphicsCmdList->SetPipelineState(m_readHTileSr0WithCountersComputePSO.Get());
        }
        else
        {
            if (m_hiStencilControlEnable)
                graphicsCmdList->SetPipelineState(m_readHTileSr0Sr1ComputePSO.Get());
            else
                graphicsCmdList->SetPipelineState(m_readHTileSr0ComputePSO.Get());
        }
        graphicsCmdList->SetComputeRootDescriptorTable(static_cast<uint32_t>(CommonHeader::RootParameterIndex::DescriptorTableSRV),
            m_textures->GetGpuDescriptorHandle(static_cast<size_t>(m_inEsram ? CommonHeader::DescriptorHeapEntry::HTileBufferEsram : CommonHeader::DescriptorHeapEntry::HTileBuffer)));
        graphicsCmdList->SetComputeRootDescriptorTable(static_cast<uint32_t>(CommonHeader::RootParameterIndex::DescriptorTableUAV), m_uavGpuHeapStart);
        graphicsCmdList->SetComputeRootConstantBufferView(static_cast<uint32_t>(CommonHeader::RootParameterIndex::ConstantBuffer), m_htileDescCB->GetGPUVirtualAddress());
        graphicsCmdList->Dispatch(static_cast<uint32_t>(m_hTilePlane->SizeBytes) / (4 * 64), 1, 1);
    }
    m_gpuTimer.Stop(graphicsCmdList, static_cast<uint32_t>(Timers::DispatchTimer));
}

// Render UI
void Sample::RenderUI(ID3D12GraphicsCommandList * graphicsCmdList)
{
    PIXScopedEvent(graphicsCmdList, PIX_COLOR_DEFAULT, L"Render UI");
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    graphicsCmdList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    XMVECTOR textPosition = XMVectorSet(50, 50, 0, 1);
    XMVECTOR textColor = ATG::ColorsLinear::White;
    auto outputSize = m_deviceResources->GetOutputSize();
    XMVECTORF32 textDiffInY = { 0.f, (outputSize.bottom > 1080) ? 60.0f : 35.0f, 0.f, 0.f };

    m_fontBatch->Begin(graphicsCmdList);

    m_fontText->DrawString(m_fontBatch.get(), g_sampleTitle, textPosition, textColor);
    textPosition = XMVectorAdd(textPosition, textDiffInY);
    m_fontText->DrawString(m_fontBatch.get(), g_sampleDescription, textPosition, textColor);

    wchar_t bufInfo[1024] = {};

    CD3DX12_RESOURCE_BARRIER countBarrierUAVToCopySrc[] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(m_mayPassCountBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(m_mayFailCountBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(m_smemSingleValueCountBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE)
    };
    graphicsCmdList->ResourceBarrier(static_cast<UINT>(std::size(countBarrierUAVToCopySrc)), countBarrierUAVToCopySrc);

    // Read the count to know exactly how many bytes of data
    // were written by the compute shader when building the bundle
    uint32_t counterValueMayPass = m_mayPassCountBufferMapped[0];
    uint32_t counterValueMayFail = m_mayFailCountBufferMapped[0];
    uint32_t counterValueSmemSingleValue = m_smemSingleValueCountBufferMapped[0];

    CD3DX12_RESOURCE_BARRIER countBarrierCopySrcToUAV[] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(m_mayPassCountBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_mayFailCountBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(m_smemSingleValueCountBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    };
    graphicsCmdList->ResourceBarrier(static_cast<UINT>(std::size(countBarrierUAVToCopySrc)), countBarrierCopySrcToUAV);

    {
        swprintf_s(bufInfo, L"[X] - %ls", m_renderUsingCs ? L"Compute Shader" : L"Pixel Shader");
        textPosition = XMVectorAdd(textPosition, textDiffInY);
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), bufInfo, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);

        if (!m_renderUsingCs)
        {
            swprintf_s(bufInfo, L"[A] - HiStencil: %ls", m_hiSOn ? L"On" : L"Off");
            textPosition = XMVectorAdd(textPosition, textDiffInY);
            DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), bufInfo, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);
        }

        {
            swprintf_s(bufInfo, L"Stencil Pass Write Time");
            textPosition = XMVectorAdd(textPosition, textDiffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufInfo, textPosition, textColor);

            auto gpuStencilPassWriteAvgTime = m_renderUsingCs ?
                (m_gpuTimer.GetAverageMS(static_cast<uint32_t>(Timers::DispatchTimer)) + m_gpuTimer.GetAverageMS(static_cast<uint32_t>(Timers::TransitionTimer)) + m_gpuTimer.GetAverageMS(static_cast<uint32_t>(Timers::ExecuteIndirectTimer)))
                : m_gpuTimer.GetAverageMS(static_cast<uint32_t>(Timers::StencilPassPsTimer));
            auto gpuStencilPassWriteElapsedTime = m_renderUsingCs ?
                (m_gpuTimer.GetElapsedMS(static_cast<uint32_t>(Timers::DispatchTimer)) + m_gpuTimer.GetElapsedMS(static_cast<uint32_t>(Timers::TransitionTimer)) + m_gpuTimer.GetElapsedMS(static_cast<uint32_t>(Timers::ExecuteIndirectTimer)))
                : m_gpuTimer.GetElapsedMS(static_cast<uint32_t>(Timers::StencilPassPsTimer));
            auto textPositionTemp = XMVectorAdd(textPosition, XMVectorSet(100, 0, 0, 0));

            swprintf_s(bufInfo, L"GPU Average: %.3f ms", gpuStencilPassWriteAvgTime);
            textPositionTemp = XMVectorAdd(textPositionTemp, textDiffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufInfo, textPositionTemp, textColor);

            swprintf_s(bufInfo, L"GPU Elapsed: %.3f ms", gpuStencilPassWriteElapsedTime);
            textPositionTemp = XMVectorAdd(textPositionTemp, textDiffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufInfo, textPositionTemp, textColor);

            textPosition = XMVectorSetY(textPosition, XMVectorGetY(textPositionTemp));
        }

        {
            swprintf_s(bufInfo, L"Total Render Time");
            textPosition = XMVectorAdd(textPosition, textDiffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufInfo, textPosition, textColor);

            auto gpuAvgTime = m_gpuTimer.GetAverageMS(static_cast<uint32_t>(Timers::RenderTimer));
            auto gpuElapsedTime = m_gpuTimer.GetElapsedMS(static_cast<uint32_t>(Timers::RenderTimer));
            auto textPositionTemp = XMVectorAdd(textPosition, XMVectorSet(100, 0, 0, 0));

            swprintf_s(bufInfo, L"GPU Average: %.3f ms", gpuAvgTime);
            textPositionTemp = XMVectorAdd(textPositionTemp, textDiffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufInfo, textPositionTemp, textColor);

            swprintf_s(bufInfo, L"GPU Elapsed: %.3f ms", gpuElapsedTime);
            textPositionTemp = XMVectorAdd(textPositionTemp, textDiffInY);
            m_fontText->DrawString(m_fontBatch.get(), bufInfo, textPositionTemp, textColor);

            textPosition = XMVectorSetY(textPosition, XMVectorGetY(textPositionTemp));
        }
    }

    if (m_renderUsingCs && m_showCount)
    {
        swprintf_s(bufInfo, L"Tiles May Pass: %d", counterValueMayPass);
        textPosition = XMVectorAdd(textPosition, textDiffInY);
        m_fontText->DrawString(m_fontBatch.get(), bufInfo, textPosition, textColor);

        swprintf_s(bufInfo, L"Tiles May Fail: %d", counterValueMayFail);
        textPosition = XMVectorAdd(textPosition, textDiffInY);
        m_fontText->DrawString(m_fontBatch.get(), bufInfo, textPosition, textColor);

        swprintf_s(bufInfo, L"Tiles SMEM Single Value: %d", counterValueSmemSingleValue);
        textPosition = XMVectorAdd(textPosition, textDiffInY);
        m_fontText->DrawString(m_fontBatch.get(), bufInfo, textPosition, textColor);
    }

    swprintf_s(bufInfo, L"[B] - HiStencilControlX Used: %ls", m_hiStencilControlEnable ? L"Yes" : L"No");
    textPosition = XMVectorAdd(textPosition, textDiffInY);
    DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), bufInfo, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);

    if (m_hiStencilControlEnable)
    {
        swprintf_s(bufInfo, L"[LT] - Set Resource to override HiSControlX: %ls", m_setDSVToOverrideHiSControlX ? L"Yes" : L"No");
        textPosition = XMVectorAdd(textPosition, textDiffInY);
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), bufInfo, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);
    }

    if (m_renderUsingCs)
    {
        if (m_resummarize)
            swprintf_s(bufInfo, L"[Y] - Resummarize: On - GPU Avg: %.2f ms", m_gpuTimer.GetAverageMS(static_cast<uint32_t>(Timers::ResummarizeTimer)));
        else
            swprintf_s(bufInfo, L"[Y] - Resummarize: Off");
        textPosition = XMVectorAdd(textPosition, textDiffInY);
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), bufInfo, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);

        swprintf_s(bufInfo, L"[RT] - Collect values before drawing exterior mesh: %ls", m_dispatchBeforeOccluder ? L"Yes" : L"No");
        textPosition = XMVectorAdd(textPosition, textDiffInY);
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), bufInfo, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);
    }
    if (m_isEsramAvailable)
    {
        swprintf_s(bufInfo, L"[LB] - ESRAM: %ls", m_inEsram ? L"Yes" : L"No");
        textPosition = XMVectorAdd(textPosition, textDiffInY);
        DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), bufInfo, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);
    }
    swprintf_s(bufInfo, L"[Menu] - Show Help Screen");
    textPosition = XMVectorAdd(textPosition, textDiffInY);
    DX::DrawControllerString(m_fontBatch.get(), m_fontText.get(), m_fontController.get(), bufInfo, XMFLOAT2(50, XMVectorGetY(textPosition)), textColor, 1.0f);

    if (m_renderUsingCs)
    {
        textPosition = XMVectorAdd(textPosition, textDiffInY);
        textPosition = XMVectorAdd(textPosition, textDiffInY);
        XMVECTORF32 textColorGreen = { 0, 0.25f, 0, 0 };
        m_fontText->DrawString(m_fontBatch.get(), L"Green ", textPosition, textColorGreen);
        auto textPositionTemp = XMVectorAdd(textPosition, XMVectorSet(150, 0, 0, 0));
        m_fontText->DrawString(m_fontBatch.get(), L"-> Single value tiles", textPositionTemp, textColor);
        textPosition = XMVectorAdd(textPosition, textDiffInY);
        XMVECTORF32 textColorBlue = { 0, 0, 0.25f, 0 };
        m_fontText->DrawString(m_fontBatch.get(), L"Blue ", textPosition, textColorBlue);
        textPositionTemp = XMVectorAdd(textPosition, XMVectorSet(150, 0, 0, 0));
        m_fontText->DrawString(m_fontBatch.get(), L"-> Expanded SMem values", textPositionTemp, textColor);
    }

    // End font batch
    m_fontBatch->End();
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

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_hwConfig = {};
    device->GetGpuHardwareConfigurationX(&m_hwConfig);

    LoadMeshes(device, resourceUpload);

    m_rootSignature = m_effect[static_cast<uint32_t>(NUM_EFFECTS::Disc)]->GetRootSignature();
    CreateResources(device, resourceUpload);
    InitializeSpriteFonts(device, resourceUpload, rtState);
    m_help->RestoreDevice(device, resourceUpload, rtState);

    auto resourceUploadEvent = resourceUpload.End(m_deviceResources->GetCommandQueue());
    // Wait for resources to upload
    resourceUploadEvent.wait();

    m_gpuTimer.RestoreDevice(device, m_deviceResources->GetCommandQueue());
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Initialize camera
    m_camera = std::make_unique<DX::FlyCamera>();
    m_camera->SetWindow(static_cast<int>(m_deviceResources->GetScreenViewport().Width), static_cast<int>(m_deviceResources->GetScreenViewport().Height));
    constexpr XMVECTORF32 camPosition = { -0.361f, 15.430f, -21.045f, 0.0f };
    constexpr XMVECTORF32 camRotation = { 0.257f, 0.947f, 0.159f, -0.111f };
    m_camera->SetProjectionParameters(0.4, 0.1, 1000, false);
    m_camera->SetPosition(camPosition);
    m_camera->SetRotation(camRotation);
    m_camera->SetSensitivity(40.0f, 40.0f, 40.0f, 0.0f);
    m_camera->SetRotationRate(0.25f);
    m_camera->SetFlags(m_camera->GetFlags() | DX::FlyCamera::c_FlagsDisableSensitivityControl);

    // Help
    m_help->SetWindow(m_deviceResources->GetOutputSize());

    // Font
    m_fontBatch->SetViewport(m_deviceResources->GetScreenViewport());
}

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#pragma warning(disable : 4061)

void Sample::CreateResources(ID3D12Device* device, ResourceUploadBatch& resourceUpload)
{
    // Create Depth Stencil
    m_dsvHeap = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        static_cast<size_t>(DSV_HEAP::TotalNum));
    SetDebugObjectName(m_dsvHeap->Heap(), L"DSV Heap");

    auto outputSize = m_deviceResources->GetOutputSize();

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    constexpr uint32_t c_64KPageSize = 64 * 1024;
    constexpr uint32_t c_totalEsramPages = 512;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Width = UINT64(outputSize.right);
    resDesc.Height = UINT(outputSize.bottom);
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    resDesc.SampleDesc.Count = 1;
    resDesc.SampleDesc.Quality = 0;
    resDesc.Layout = D3D12XBOX_TEXTURE_LAYOUT_TILE_MODE_COMP_DEPTH_0;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // Get Layout
    XGTextureAddressComputer* texComputer = nullptr;
    DX::ThrowIfFailed(XGCreateTextureComputer(reinterpret_cast<XG_RESOURCE_DESC*>(&resDesc), &texComputer));
    DX::ThrowIfFailed(texComputer->GetResourceLayout(&m_xgResLayout));

    for (uint32_t i = 0; i < m_xgResLayout.Planes; ++i)
    {
        switch (m_xgResLayout.Plane[i].Usage)
        {
        case XG_PLANE_USAGE_DEPTH:
            m_depthPlane = &m_xgResLayout.Plane[i];
            break;
        case XG_PLANE_USAGE_STENCIL:
            m_stencilPlane = &m_xgResLayout.Plane[i];
            break;
        case XG_PLANE_USAGE_HTILE:
            m_hTilePlane = &m_xgResLayout.Plane[i];
            break;
        default:
            break;
        }
    }

    auto alignedSize = AlignUp(m_xgResLayout.SizeBytes, m_xgResLayout.BaseAlignmentBytes);
    uint32_t numDepthPages = static_cast<uint32_t>(std::ceil(alignedSize / static_cast<float>(c_64KPageSize)));

    XGTextureAddressComputer* rtvTexComputer = nullptr;
    auto rtvResDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_deviceResources->GetBackBufferFormat(), UINT64(outputSize.right), UINT(outputSize.bottom), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    DX::ThrowIfFailed(XGCreateTextureComputer(reinterpret_cast<XG_RESOURCE_DESC*>(&rtvResDesc), &rtvTexComputer));
    XG_RESOURCE_LAYOUT rtvXgLayout;
    DX::ThrowIfFailed(rtvTexComputer->GetResourceLayout(&rtvXgLayout));
    auto alignedRtSize = AlignUp(rtvXgLayout.SizeBytes, rtvXgLayout.BaseAlignmentBytes);
    uint32_t numRtvPages = static_cast<uint32_t>(std::ceil(alignedRtSize / static_cast<float>(c_64KPageSize)));

    uint32_t numPages = numDepthPages + numRtvPages;
    uint32_t numSystemPages = numPages;
    uint32_t numEsramPages = 0;

    if (m_hwConfig.HardwareVersion < D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X)
    {
        // Create resources in both DRAM and ESRAM to allow switching on the fly if changed by user
        numEsramPages = std::min(numPages, c_totalEsramPages);
        numSystemPages = numPages > numEsramPages ? numPages - numEsramPages : 0u;
        numSystemPages += numEsramPages;
        //numDepthEsramPages = std::min(numDepthPages, c_totalEsramPages);
        //numRtvEsramPages = std::min(numRtvPages, c_totalEsramPages - numDepthEsramPages);
        m_isEsramAvailable = true;
    }

    XGMemoryLayoutEngine layoutEngine;
    XGMemoryLayout* defaultLayout;
    XGMemoryLayoutMapping defaultLayoutMapping;

    DX::ThrowIfFailed(layoutEngine.InitializeWithPageCounts(numSystemPages, numEsramPages, 0));

    wchar_t layoutName[128];
    _snwprintf_s(layoutName, _TRUNCATE, L"DS = %d, RT = %d", numDepthPages, numRtvPages);
    DX::ThrowIfFailed(layoutEngine.CreateMemoryLayout(layoutName, c_64KPageSize * (numEsramPages + numSystemPages), 0, &defaultLayout));

    DX::ThrowIfFailed(defaultLayout->CreateMapping(L"DefaultLayout", &defaultLayoutMapping));
    DX::ThrowIfFailed(defaultLayout->MapSimple(&defaultLayoutMapping, numSystemPages + numEsramPages, numEsramPages));
    auto mappingBaseAdress = static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(defaultLayoutMapping.MappingBaseAddress);
    D3D12_GPU_VIRTUAL_ADDRESS depthGpuAddressEsram = m_isEsramAvailable ? mappingBaseAdress : 0;
    D3D12_GPU_VIRTUAL_ADDRESS intermediateRtvGpuAddressEsram = m_isEsramAvailable ? depthGpuAddressEsram + numDepthPages * c_64KPageSize : 0;
    D3D12_GPU_VIRTUAL_ADDRESS depthGpuAddress = m_isEsramAvailable ? intermediateRtvGpuAddressEsram + numRtvPages * c_64KPageSize : mappingBaseAdress;
    D3D12_GPU_VIRTUAL_ADDRESS intermediateRtvGpuAddress = depthGpuAddress + numDepthPages * c_64KPageSize;
    m_dsvState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    device->CreatePlacedResourceX(depthGpuAddress, &resDesc, m_dsvState, &clearValue, IID_GRAPHICS_PPV_ARGS(m_depthStencilBuffer.ReleaseAndGetAddressOf()));
    m_depthStencilBuffer->SetName(L"Depth Stencil buffer");
    if (m_isEsramAvailable)
    {
        m_dsvStateEsram = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        device->CreatePlacedResourceX(depthGpuAddressEsram, &resDesc, m_dsvStateEsram, &clearValue, IID_GRAPHICS_PPV_ARGS(m_depthStencilBufferEsram.ReleaseAndGetAddressOf()));
        m_depthStencilBufferEsram->SetName(L"Depth Stencil buffer");
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;
    device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &depthStencilViewDesc, m_dsvHeap->GetCpuHandle(static_cast<uint32_t>(DSV_HEAP::DepthBuffer)));
    if (m_isEsramAvailable)
    {
        device->CreateDepthStencilView(m_depthStencilBufferEsram.Get(), &depthStencilViewDesc, m_dsvHeap->GetCpuHandle(static_cast<uint32_t>(DSV_HEAP::DepthBufferEsram)));
    }

    // Create depth SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc = {};
    depthSrvDesc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthSrvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(m_depthStencilBuffer.Get(), &depthSrvDesc, m_textures->GetCpuDescriptorHandle(static_cast<size_t>(CommonHeader::DescriptorHeapEntry::DepthBuffer)));
    if (m_isEsramAvailable)
    {
        device->CreateShaderResourceView(m_depthStencilBufferEsram.Get(), &depthSrvDesc, m_textures->GetCpuDescriptorHandle(static_cast<size_t>(CommonHeader::DescriptorHeapEntry::DepthBufferEsram)));
    }

    // Create stencil SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC stencilSrvDesc = {};
    stencilSrvDesc.Format = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
    stencilSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    stencilSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    stencilSrvDesc.Texture2D.MipLevels = 1;
    stencilSrvDesc.Texture2D.PlaneSlice = 1;
    device->CreateShaderResourceView(m_depthStencilBuffer.Get(), &stencilSrvDesc, m_textures->GetCpuDescriptorHandle(static_cast<size_t>(CommonHeader::DescriptorHeapEntry::StencilBuffer)));
    if (m_isEsramAvailable)
    {
        device->CreateShaderResourceView(m_depthStencilBufferEsram.Get(), &stencilSrvDesc, m_textures->GetCpuDescriptorHandle(static_cast<size_t>(CommonHeader::DescriptorHeapEntry::StencilBufferEsram)));
    }
    // Point HTile buffer to the correct address
    D3D12_RESOURCE_ALLOCATION_INFO resAllocInfo = {};
    resAllocInfo.SizeInBytes = m_hTilePlane->SizeBytes;
    auto hiStencilDesc = CD3DX12_RESOURCE_DESC::Buffer(resAllocInfo, D3D12_RESOURCE_FLAG_NONE);
    m_hTileState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    device->CreatePlacedResourceX(depthGpuAddress + m_hTilePlane->BaseOffsetBytes, &hiStencilDesc, m_hTileState, nullptr, IID_GRAPHICS_PPV_ARGS(m_hTileBuffer.ReleaseAndGetAddressOf()));
    m_hTileBuffer->SetName(L"Hi Stencil buffer");
    if (m_isEsramAvailable)
    {
        device->CreatePlacedResourceX(depthGpuAddressEsram + m_hTilePlane->BaseOffsetBytes, &hiStencilDesc, m_hTileState, nullptr, IID_GRAPHICS_PPV_ARGS(m_hTileBufferEsram.ReleaseAndGetAddressOf()));
        m_hTileBufferEsram->SetName(L"Hi Stencil buffer");
    }
    D3D12_SHADER_RESOURCE_VIEW_DESC hiStencilSrvDesc = {};
    hiStencilSrvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    hiStencilSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    hiStencilSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    hiStencilSrvDesc.Buffer.NumElements = static_cast<uint32_t>(m_hTilePlane->SizeBytes / sizeof(uint32_t));
    hiStencilSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    device->CreateShaderResourceView(m_hTileBuffer.Get(), &hiStencilSrvDesc, m_textures->GetCpuDescriptorHandle(static_cast<size_t>(CommonHeader::DescriptorHeapEntry::HTileBuffer)));
    if (m_isEsramAvailable)
    {
        device->CreateShaderResourceView(m_hTileBufferEsram.Get(), &hiStencilSrvDesc, m_textures->GetCpuDescriptorHandle(static_cast<size_t>(CommonHeader::DescriptorHeapEntry::HTileBufferEsram)));
    }

    CreateRectResources(device, resourceUpload);
    CreateComputeShaderResources(device);

    {
        // Create intermediate render target
        m_rtvHeap = std::make_unique<DescriptorHeap>(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            static_cast<uint32_t>(RTV_HEAP::TotalNum));
        SetDebugObjectName(m_rtvHeap->Heap(), L"RTV Heap");

        D3D12_CLEAR_VALUE clearValueRtv = {};
        clearValueRtv.Format = m_deviceResources->GetBackBufferFormat();
        m_rtBufferState = D3D12_RESOURCE_STATE_RENDER_TARGET;
        device->CreatePlacedResourceX(intermediateRtvGpuAddress, &rtvResDesc, m_rtBufferState, &clearValueRtv, IID_GRAPHICS_PPV_ARGS(m_rtBuffer.ReleaseAndGetAddressOf()));
        m_rtBuffer->SetName(L"Intermediate RT");
        if (m_isEsramAvailable)
        {
            m_rtBufferStateEsram = D3D12_RESOURCE_STATE_RENDER_TARGET;
            device->CreatePlacedResourceX(intermediateRtvGpuAddressEsram, &rtvResDesc, m_rtBufferStateEsram, &clearValueRtv, IID_GRAPHICS_PPV_ARGS(m_rtBufferEsram.ReleaseAndGetAddressOf()));
            m_rtBufferEsram->SetName(L"Intermediate RT");
        }

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = m_deviceResources->GetBackBufferFormat();
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        device->CreateRenderTargetView(m_rtBuffer.Get(), &rtvDesc, m_rtvHeap->GetCpuHandle(static_cast<uint32_t>(RTV_HEAP::RTBuffer)));
        if (m_isEsramAvailable)
        {
            device->CreateRenderTargetView(m_rtBufferEsram.Get(), &rtvDesc, m_rtvHeap->GetCpuHandle(static_cast<uint32_t>(RTV_HEAP::RTBufferEsram)));
        }

        uint32_t handleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        auto renderTargetUAVCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_uavCpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::RenderTarget_UAV), handleIncrementSize);
        m_renderTargetUAVGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_uavGpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::RenderTarget_UAV), handleIncrementSize);
        auto renderTargetEsramUAVCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_uavCpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::RenderTargetEsram_UAV), handleIncrementSize);
        m_renderTargetEsramUAVGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_uavGpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::RenderTargetEsram_UAV), handleIncrementSize);

        // Create UAV for render target
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavRtvDesc = {};
        uavRtvDesc.Format = m_deviceResources->GetBackBufferFormat();
        uavRtvDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        device->CreateUnorderedAccessView(m_rtBuffer.Get(), nullptr, &uavRtvDesc, renderTargetUAVCPU);
        if (m_isEsramAvailable)
        {
            device->CreateUnorderedAccessView(m_rtBufferEsram.Get(), nullptr, &uavRtvDesc, renderTargetEsramUAVCPU);
        }

        // Create intermediate target SRV
        D3D12_SHADER_RESOURCE_VIEW_DESC targetSrvDesc = {};
        targetSrvDesc.Format = m_deviceResources->GetBackBufferFormat();
        targetSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        targetSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        targetSrvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(m_rtBuffer.Get(), &targetSrvDesc, m_textures->GetCpuDescriptorHandle(static_cast<size_t>(CommonHeader::DescriptorHeapEntry::IntermediateTarget)));
        if (m_isEsramAvailable)
        {
            device->CreateShaderResourceView(m_rtBufferEsram.Get(), &targetSrvDesc, m_textures->GetCpuDescriptorHandle(static_cast<size_t>(CommonHeader::DescriptorHeapEntry::IntermediateTargetEsram)));
        }
    }
}

void Sample::CreateComputeShaderResources(ID3D12Device* device)
{
    m_uavHeap = std::make_unique<DescriptorHeap>(device,
        static_cast<uint32_t>(CommonHeader::UAV_HEAP::TotalNum));
    SetDebugObjectName(m_uavHeap->Heap(), L"UAV Heap");

    m_uavCpuHeapStart = m_uavHeap->GetFirstCpuHandle();
    m_uavGpuHeapStart = m_uavHeap->GetFirstGpuHandle();
    uint32_t handleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_mayPassAppendBufferDescriptorCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_uavCpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::MayPassAppendBuffer), handleIncrementSize);
    m_mayPassCountBufferDescriptorCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_uavCpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::MayPassCountBuffer), handleIncrementSize);
    m_mayPassAppendBufferDescriptorGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_uavGpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::MayPassAppendBuffer), handleIncrementSize);
    m_mayPassCountBufferDescriptorGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_uavGpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::MayPassCountBuffer), handleIncrementSize);

    m_mayFailAppendBufferDescriptorCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_uavCpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::MayFailAppendBuffer), handleIncrementSize);
    m_mayFailCountBufferDescriptorCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_uavCpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::MayFailCountBuffer), handleIncrementSize);
    m_mayFailAppendBufferDescriptorGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_uavGpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::MayFailAppendBuffer), handleIncrementSize);
    m_mayFailCountBufferDescriptorGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_uavGpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::MayFailCountBuffer), handleIncrementSize);

    m_smemSingleValueAppendBufferDescriptorCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_uavCpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::SMemSingleValueAppendBuffer), handleIncrementSize);
    m_smemSingleValueCountBufferDescriptorCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_uavCpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::SMemSingleValuesCountBuffer), handleIncrementSize);
    m_smemSingleValueAppendBufferDescriptorGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_uavGpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::SMemSingleValueAppendBuffer), handleIncrementSize);
    m_smemSingleValueCountBufferDescriptorGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_uavGpuHeapStart, static_cast<uint32_t>(CommonHeader::UAV_HEAP::SMemSingleValuesCountBuffer), handleIncrementSize);

    const CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    auto appendBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        m_hTilePlane->SizeBytes,                                                    // UINT64 width,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS |
        D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER                               // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                                    // UINT64 alignment = 0
    );
    auto countBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        sizeof(uint32_t) * 3, // 3 for x,y,z of Dispatch passed to ExecuteIndirect  // UINT64 width,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER  // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                                    // UINT64 alignment = 0
    );

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.NumElements = static_cast<uint32_t>(m_hTilePlane->SizeBytes) / sizeof(uint32_t);
    uavDesc.Buffer.StructureByteStride = sizeof(uint32_t);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavCountDesc = {};
    uavCountDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavCountDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavCountDesc.Buffer.NumElements = 3; // 3 for Dispatch x,y,z
    uavCountDesc.Buffer.StructureByteStride = sizeof(uint32_t);

    // Create Resources
    {
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,    	// _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
            D3D12_HEAP_FLAG_NONE,														// D3D12_HEAP_FLAGS HeapFlags,
            &appendBufferDesc,															// _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,										// D3D12_RESOURCE_STATES InitialState,
            nullptr,																	// _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                                                                        // REFIID riidResource,
            IID_GRAPHICS_PPV_ARGS(m_mayPassAppendBuffer.ReleaseAndGetAddressOf())));	// _Outptr_opt_ void** ppvResource
        m_mayPassAppendBuffer->SetName(L"SR0 May Pass Append Buffer");

        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,    	// _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
            D3D12_HEAP_FLAG_NONE,														// D3D12_HEAP_FLAGS HeapFlags,
            &countBufferDesc,															// _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,										// D3D12_RESOURCE_STATES InitialState,
            nullptr,																	// _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                                                                        // REFIID riidResource,
            IID_GRAPHICS_PPV_ARGS(m_mayPassCountBuffer.ReleaseAndGetAddressOf())));	// _Outptr_opt_ void** ppvResource
        m_mayPassCountBuffer->SetName(L"SR0 May Pass Count Buffer");

        device->CreateUnorderedAccessView(m_mayPassAppendBuffer.Get(), m_mayPassCountBuffer.Get(), &uavDesc, m_mayPassAppendBufferDescriptorCPU);
        device->CreateUnorderedAccessView(m_mayPassCountBuffer.Get(), nullptr, &uavCountDesc, m_mayPassCountBufferDescriptorCPU);

        // Leaving the append count buffer mapped as it has to be queried each frame for outputting the value to screen
        m_mayPassCountBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mayPassCountBufferMapped));
    }
    {
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,    	// _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
            D3D12_HEAP_FLAG_NONE,														// D3D12_HEAP_FLAGS HeapFlags,
            &appendBufferDesc,															// _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,										// D3D12_RESOURCE_STATES InitialState,
            nullptr,																	// _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                                                                        // REFIID riidResource,
            IID_GRAPHICS_PPV_ARGS(m_mayFailAppendBuffer.ReleaseAndGetAddressOf())));	// _Outptr_opt_ void** ppvResource
        m_mayFailAppendBuffer->SetName(L"SR0 May Fail Append Buffer");

        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,    	// _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
            D3D12_HEAP_FLAG_NONE,														// D3D12_HEAP_FLAGS HeapFlags,
            &countBufferDesc,															// _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,										// D3D12_RESOURCE_STATES InitialState,
            nullptr,																	// _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                                                                        // REFIID riidResource,
            IID_GRAPHICS_PPV_ARGS(m_mayFailCountBuffer.ReleaseAndGetAddressOf())));	// _Outptr_opt_ void** ppvResource
        m_mayFailCountBuffer->SetName(L"SR0 May Fail Count Buffer");

        device->CreateUnorderedAccessView(m_mayFailAppendBuffer.Get(), m_mayFailCountBuffer.Get(), &uavDesc, m_mayFailAppendBufferDescriptorCPU);
        device->CreateUnorderedAccessView(m_mayFailCountBuffer.Get(), nullptr, &uavCountDesc, m_mayFailCountBufferDescriptorCPU);

        // Leaving the append count buffer mapped as it has to be queried each frame for outputting the value to screen
        m_mayFailCountBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mayFailCountBufferMapped));
    }
    {
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,    	// _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
            D3D12_HEAP_FLAG_NONE,														// D3D12_HEAP_FLAGS HeapFlags,
            &appendBufferDesc,															// _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,										// D3D12_RESOURCE_STATES InitialState,
            nullptr,																	// _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                                                                        // REFIID riidResource,
            IID_GRAPHICS_PPV_ARGS(m_smemSingleValueAppendBuffer.ReleaseAndGetAddressOf())));	// _Outptr_opt_ void** ppvResource
        m_smemSingleValueAppendBuffer->SetName(L"SMem Single Value Append Buffer");

        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,    	// _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
            D3D12_HEAP_FLAG_NONE,														// D3D12_HEAP_FLAGS HeapFlags,
            &countBufferDesc,															// _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,										// D3D12_RESOURCE_STATES InitialState,
            nullptr,																	// _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                                                                        // REFIID riidResource,
            IID_GRAPHICS_PPV_ARGS(m_smemSingleValueCountBuffer.ReleaseAndGetAddressOf())));	// _Outptr_opt_ void** ppvResource
        m_smemSingleValueCountBuffer->SetName(L"SMem Single Value Count Buffer");

        device->CreateUnorderedAccessView(m_smemSingleValueAppendBuffer.Get(), m_smemSingleValueCountBuffer.Get(), &uavDesc, m_smemSingleValueAppendBufferDescriptorCPU);
        device->CreateUnorderedAccessView(m_smemSingleValueCountBuffer.Get(), nullptr, &uavCountDesc, m_smemSingleValueCountBufferDescriptorCPU);

        m_mayPassCountBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mayPassCountBufferMapped));
        m_mayPassCountBufferMapped[0] = 0;
        m_mayPassCountBufferMapped[1] = 1;
        m_mayPassCountBufferMapped[2] = 1;

        m_smemSingleValueCountBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_smemSingleValueCountBufferMapped));
        m_smemSingleValueCountBufferMapped[0] = 0;

        m_mayFailCountBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mayFailCountBufferMapped));
        m_mayFailCountBufferMapped[0] = 0;

        // Constant buffer for HTile data
        const CD3DX12_HEAP_PROPERTIES defaultHeapProp(D3D12_HEAP_TYPE_DEFAULT);
        auto htileBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(HTileDesc));
        DX::ThrowIfFailed(
            device->CreateCommittedResource(&defaultHeapProp,
                D3D12_HEAP_FLAG_NONE,
                &htileBufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_htileDescCB.ReleaseAndGetAddressOf())));
        m_htileDescCB->SetName(L"CB for HTile Data");

        // Copy the constant buffer data
        HTileDesc* hTileCBDataBegin;
        CD3DX12_RANGE readRange(0, 0);
        DX::ThrowIfFailed(
            m_htileDescCB->Map(0, &readRange, reinterpret_cast<void**>(&hTileCBDataBegin)));
        // HTile is stored in linear format if the depth buffer has less than 2 million (power of 2) pixels
        hTileCBDataBegin->isHTileLinear = ((m_depthPlane->MipLayout[0].WidthElements) * (m_depthPlane->MipLayout[0].HeightElements) <= 0x200000u) ? 1u : 0u;
        // Calculate HTile pitch using padded width elements
        hTileCBDataBegin->hTilePitch = m_depthPlane->MipLayout[0].PaddedWidthElements / 8;
        m_htileDescCB->Unmap(0, nullptr);

        // Constant buffer for Stencil data
        const CD3DX12_HEAP_PROPERTIES uploadHeapProp(D3D12_HEAP_TYPE_UPLOAD);
        auto stencilDataDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint32_t));
        DX::ThrowIfFailed(
            device->CreateCommittedResource(&uploadHeapProp,
                D3D12_HEAP_FLAG_NONE,
                &stencilDataDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_stencilDataCB.ReleaseAndGetAddressOf())));
        m_stencilDataCB->SetName(L"CB for Stencil Data");

        DX::ThrowIfFailed(m_stencilDataCB->Map(0, &readRange, reinterpret_cast<void**>(&m_stencilCBDataCPU)));
        m_stencilCBDataCPU[0] = COMPARE_VALUE;
    }

    // Create Compute PSOs
    std::vector<uint8_t> readHTileSr0CSBlob, readHTileSr0WithCountersCSBlob;
    std::vector<uint8_t> readHTileSr0Sr1CSBlob, readHTileSr0Sr1WithCountersCSBlob;
    if (m_hwConfig.HardwareVersion >= D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X)
    {
        readHTileSr0CSBlob = DX::ReadData(L"ReadHTileCS_SR0_XboxOneX.cso");
        readHTileSr0WithCountersCSBlob = DX::ReadData(L"ReadHTileCS_SR0_WithCounters_XboxOneX.cso");
        readHTileSr0Sr1CSBlob = DX::ReadData(L"ReadHTileCS_SR0_SR1_XboxOneX.cso");
        readHTileSr0Sr1WithCountersCSBlob = DX::ReadData(L"ReadHTileCS_SR0_SR1_WithCounters_XboxOneX.cso");
    }
    else
    {
        readHTileSr0CSBlob = DX::ReadData(L"ReadHTileCS_SR0_XboxOne.cso");
        readHTileSr0WithCountersCSBlob = DX::ReadData(L"ReadHTileCS_SR0_WithCounters_XboxOne.cso");
        readHTileSr0Sr1CSBlob = DX::ReadData(L"ReadHTileCS_SR0_SR1_XboxOne.cso");
        readHTileSr0Sr1WithCountersCSBlob = DX::ReadData(L"ReadHTileCS_SR0_SR1_WithCounters_XboxOne.cso");
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC computePSODesc = {};
    computePSODesc.pRootSignature = m_rootSignature.Get();
    computePSODesc.CS = { readHTileSr0CSBlob.data(), readHTileSr0CSBlob.size() };
    DX::ThrowIfFailed(device->CreateComputePipelineState(&computePSODesc, IID_GRAPHICS_PPV_ARGS(m_readHTileSr0ComputePSO.ReleaseAndGetAddressOf())));

    computePSODesc.CS = { readHTileSr0WithCountersCSBlob.data(), readHTileSr0WithCountersCSBlob.size() };
    DX::ThrowIfFailed(device->CreateComputePipelineState(&computePSODesc, IID_GRAPHICS_PPV_ARGS(m_readHTileSr0WithCountersComputePSO.ReleaseAndGetAddressOf())));

    computePSODesc.CS = { readHTileSr0Sr1CSBlob.data(), readHTileSr0Sr1CSBlob.size() };
    DX::ThrowIfFailed(device->CreateComputePipelineState(&computePSODesc, IID_GRAPHICS_PPV_ARGS(m_readHTileSr0Sr1ComputePSO.ReleaseAndGetAddressOf())));

    computePSODesc.CS = { readHTileSr0Sr1WithCountersCSBlob.data(), readHTileSr0Sr1WithCountersCSBlob.size() };
    DX::ThrowIfFailed(device->CreateComputePipelineState(&computePSODesc, IID_GRAPHICS_PPV_ARGS(m_readHTileSr0Sr1WithCountersComputePSO.ReleaseAndGetAddressOf())));

    D3D12_COMMAND_SIGNATURE_DESC cmdSignDesc = {};
    cmdSignDesc.ByteStride = 12;
    cmdSignDesc.NumArgumentDescs = 1;
    D3D12_INDIRECT_ARGUMENT_DESC indirectArgsDesc[1];
    indirectArgsDesc[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
    cmdSignDesc.pArgumentDescs = indirectArgsDesc;
    DX::ThrowIfFailed(device->CreateCommandSignature(&cmdSignDesc, m_rootSignature.Get(), IID_GRAPHICS_PPV_ARGS(m_commandSignature.ReleaseAndGetAddressOf())));
    m_commandSignature->SetName(L"Dispatch Command Signature");

    auto writeStencilPixelsCSBlob = DX::ReadData(L"WriteStencilPixelsCS.cso");

    D3D12_COMPUTE_PIPELINE_STATE_DESC writeStencilPixelsPSODesc = {};
    writeStencilPixelsPSODesc.pRootSignature = m_rootSignature.Get();
    writeStencilPixelsPSODesc.CS = { writeStencilPixelsCSBlob.data(), writeStencilPixelsCSBlob.size() };

    DX::ThrowIfFailed(device->CreateComputePipelineState(&writeStencilPixelsPSODesc, IID_GRAPHICS_PPV_ARGS(m_writeStencilPixelsPSO.ReleaseAndGetAddressOf())));
}

// Load the mesh
void Sample::LoadMeshes(ID3D12Device * device, ResourceUploadBatch& resourceUpload)
{
    m_commonStates = std::make_unique<CommonStates>(device);
    m_model[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)] = Model::CreateFromSDKMESH(device, L"MechRobot-exterior.sdkmesh");
    m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Furnishing)] = Model::CreateFromSDKMESH(device, L"MechRobot-interiorFurnishings.sdkmesh");
    m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Gears)] = Model::CreateFromSDKMESH(device, L"MechRobot-interiorGears.sdkmesh");
    m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Robot)] = Model::CreateFromSDKMESH(device, L"MechRobot-interiorRobot.sdkmesh");
    m_model[static_cast<uint32_t>(NUM_MESHES::Disc1)] = Model::CreateFromSDKMESH(device, L"disc.sdkmesh");
    m_model[static_cast<uint32_t>(NUM_MESHES::Disc2)] = Model::CreateFromSDKMESH(device, L"disc2.sdkmesh");
    m_model[static_cast<uint32_t>(NUM_MESHES::Disc3)] = Model::CreateFromSDKMESH(device, L"disc3.sdkmesh");
    m_model[static_cast<uint32_t>(NUM_MESHES::Disc4)] = Model::CreateFromSDKMESH(device, L"disc4.sdkmesh");

    // Create a texture factory for loading the model textures
    m_textures = std::make_unique<EffectTextureFactory>(device, resourceUpload, (size_t)CommonHeader::DescriptorHeapEntry::TotalDescriptorHeapEntryCount);

    {
        m_model[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)]->textureNames.resize(static_cast<uint32_t>(CommonHeader::PbrTextureEntries::TotalTextures));
        m_model[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)]->textureNames.clear();
        m_model[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)]->textureNames.push_back(L"RobotMaterial_baseColor.DDS");
        m_model[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)]->textureNames.push_back(L"RobotMaterial_normal.DDS");
        m_model[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)]->textureNames.push_back(L"RobotMaterial_occlusionRoughnessMetallic.DDS");
        m_model[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)]->textureNames.push_back(L"Stonewall_Ref_radiance.dds");
        m_model[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)]->textureNames.push_back(L"Stonewall_Ref_irradiance.dds");

        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Furnishing)]->textureNames.resize(static_cast<uint32_t>(CommonHeader::PbrTextureEntries::NumMeshTextures));
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Furnishing)]->textureNames.clear();
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Furnishing)]->textureNames.push_back(L"InteriorMaterial_baseColor.DDS");
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Furnishing)]->textureNames.push_back(L"InteriorMaterial_normal.DDS");
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Furnishing)]->textureNames.push_back(L"InteriorMaterial_occlusionRoughnessMetallic.DDS");

        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Gears)]->textureNames.resize(static_cast<uint32_t>(CommonHeader::PbrTextureEntries::NumMeshTextures));
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Gears)]->textureNames.clear();
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Gears)]->textureNames.push_back(L"GearsMaterial_baseColor.DDS");
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Gears)]->textureNames.push_back(L"GearsMaterial_normal.DDS");
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Gears)]->textureNames.push_back(L"GearsMaterial_occlusionRoughnessMetallic.DDS");

        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Robot)]->textureNames.resize(static_cast<uint32_t>(CommonHeader::PbrTextureEntries::NumMeshTextures));
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Robot)]->textureNames.clear();
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Robot)]->textureNames.push_back(L"MechMaterial_baseColor.DDS");
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Robot)]->textureNames.push_back(L"MechMaterial_normal.DDS");
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Robot)]->textureNames.push_back(L"MechMaterial_occlusionRoughnessMetallic.DDS");

        m_model[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)]->LoadTextures(*(m_textures).get(), (int)(CommonHeader::DescriptorHeapEntry::MeshTextureExterior));
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Furnishing)]->LoadTextures(*(m_textures).get(), (int)(CommonHeader::DescriptorHeapEntry::MeshTextureInteriorFurnishings));
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Gears)]->LoadTextures(*(m_textures).get(), (int)(CommonHeader::DescriptorHeapEntry::MeshTextureInteriorGears));
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Robot)]->LoadTextures(*(m_textures).get(), (int)(CommonHeader::DescriptorHeapEntry::MeshTextureInteriorRobot));
    }

    // Optimize mesh rendering
    {
        m_model[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)]->LoadStaticBuffers(device, resourceUpload);
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Furnishing)]->LoadStaticBuffers(device, resourceUpload);
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Gears)]->LoadStaticBuffers(device, resourceUpload);
        m_model[static_cast<uint32_t>(NUM_MESHES::InteriorMesh_Robot)]->LoadStaticBuffers(device, resourceUpload);
        m_model[static_cast<uint32_t>(NUM_MESHES::Disc1)]->LoadStaticBuffers(device, resourceUpload);
        m_model[static_cast<uint32_t>(NUM_MESHES::Disc2)]->LoadStaticBuffers(device, resourceUpload);
        m_model[static_cast<uint32_t>(NUM_MESHES::Disc3)]->LoadStaticBuffers(device, resourceUpload);
        m_model[static_cast<uint32_t>(NUM_MESHES::Disc4)]->LoadStaticBuffers(device, resourceUpload);
    }

    // For meshes
    {
        const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

        // Stencil Greater than Ref
        D3D12_DEPTH_STENCIL_DESC depthStencilExteriorDesc = {};
        depthStencilExteriorDesc.DepthEnable = TRUE;
        depthStencilExteriorDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilExteriorDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        depthStencilExteriorDesc.StencilEnable = TRUE;
        depthStencilExteriorDesc.StencilReadMask = 0xFF;
        depthStencilExteriorDesc.StencilWriteMask = 0;
        depthStencilExteriorDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        depthStencilExteriorDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        depthStencilExteriorDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        depthStencilExteriorDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER;
        depthStencilExteriorDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        depthStencilExteriorDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        depthStencilExteriorDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        depthStencilExteriorDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER;

        auto psdExteriorMesh = EffectPipelineStateDescription(nullptr, CommonStates::Opaque, depthStencilExteriorDesc, CommonStates::CullClockwise, rtState);

        D3D12_DEPTH_STENCIL_DESC depthStencilInteriorDesc = {};
        depthStencilInteriorDesc.DepthEnable = TRUE;
        depthStencilInteriorDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilInteriorDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        depthStencilInteriorDesc.StencilEnable = TRUE;
        depthStencilInteriorDesc.StencilReadMask = 0xFF;
        depthStencilInteriorDesc.StencilWriteMask = 0x0;
        depthStencilInteriorDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        depthStencilInteriorDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        depthStencilInteriorDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        depthStencilInteriorDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        depthStencilInteriorDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        depthStencilInteriorDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        depthStencilInteriorDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        depthStencilInteriorDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

        auto psdInteriorMesh = EffectPipelineStateDescription(nullptr, CommonStates::Opaque, depthStencilInteriorDesc, CommonStates::CullClockwise, rtState);

        // Same layout for all models
        auto sampleModel = m_model[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)].get();
        auto modelMeshPart = sampleModel->meshes[0]->opaqueMeshParts[0].get();
        D3D12_INPUT_LAYOUT_DESC inputLayoutModel =
        {
            modelMeshPart->vbDecl->data(),
            (UINT)modelMeshPart->vbDecl->size()
        };
        psdExteriorMesh.inputLayout = inputLayoutModel;
        psdInteriorMesh.inputLayout = inputLayoutModel;

        m_effectPbr[static_cast<uint32_t>(NUM_MESHES::ExteriorMesh)] = std::make_unique<PBREffect>(device, EffectFlags::Texture, psdExteriorMesh);
        for (uint32_t index = static_cast<uint32_t>(NUM_MESHES::TotalExteriorMeshes); index < static_cast<uint32_t>(NUM_MESHES::TotalIntExtMeshes); ++index)
        {
            m_effectPbr[index] = std::make_unique<PBREffect>(device, EffectFlags::Texture, psdInteriorMesh);
        }
        for (uint32_t index = 0; index < static_cast<uint32_t>(NUM_MESHES::TotalIntExtMeshes); ++index)
        {
            m_effectPbr[index]->EnableDefaultLighting();
            m_effectPbr[index]->SetRenderTargetSizeInPixels(
                m_deviceResources->GetOutputSize().right - m_deviceResources->GetOutputSize().left,
                m_deviceResources->GetOutputSize().bottom - m_deviceResources->GetOutputSize().top);
        }
    }

    // For disc
    {
        const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

        D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable = FALSE;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        depthStencilDesc.StencilEnable = TRUE;
        depthStencilDesc.StencilReadMask = 0xFF;
        depthStencilDesc.StencilWriteMask = 0xFF;
        depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_REPLACE;
        depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE;
        depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
        depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_REPLACE;
        depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE;
        depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
        depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

        auto psd = EffectPipelineStateDescription(nullptr, CommonStates::Opaque, depthStencilDesc, CommonStates::CullClockwise, rtState);

        // Load Shaders
        std::vector<uint8_t> discVSBlob = DX::ReadData(L"DiscVS.cso");
        D3D12_SHADER_BYTECODE vsByteCode = { discVSBlob.data(), discVSBlob.size() };

        auto sampleModel = m_model[static_cast<uint32_t>(NUM_MESHES::Disc1)].get();
        auto modelMeshPart = sampleModel->meshes[0]->opaqueMeshParts[0].get();
        D3D12_INPUT_LAYOUT_DESC inputLayoutModel =
        {
            modelMeshPart->vbDecl->data(),
            (UINT)modelMeshPart->vbDecl->size()
        };
        psd.inputLayout = inputLayoutModel;

        // Create Effect to create root signature and PSO
        m_effect[static_cast<uint32_t>(NUM_EFFECTS::Disc)] = std::make_unique<SampleEffect>(device,
            &psd,
            m_deviceResources->GetBackBufferCount(),
            &vsByteCode,
            nullptr,
            true,
            true);
    }
}

// Used for full screen draws
void Sample::CreateRectResources(ID3D12Device* device, ResourceUploadBatch& resourceUpload)
{
    static const D3D12_INPUT_ELEMENT_DESC inputElemDesc[2] =
    {
        { "SV_POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 },
    };
    struct VertexData
    {
        XMFLOAT4 position;
        XMFLOAT2 texCoord;
    };
    static const VertexData s_vertexData[3] =
    {
        { { -1.0f, -1.0f, 1.0f, 1.0f },{ 0.f, 1.f } },
        { { 1.0f, -1.0f, 1.0f, 1.0f },{ 1.f, 1.f } },
        { { 1.0f,  1.0f, 1.0f, 1.0f },{ 1.f, 0.f } },
    };

    DX::ThrowIfFailed(
        CreateStaticBuffer(device, resourceUpload, s_vertexData, std::size(s_vertexData), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            m_rectVertexBuffer.ReleaseAndGetAddressOf()));

    m_rectVBView.BufferLocation = m_rectVertexBuffer->GetGPUVirtualAddress();
    m_rectVBView.SizeInBytes = sizeof(s_vertexData);
    m_rectVBView.StrideInBytes = sizeof(VertexData);

    // Shaders
    auto vsBlob = DX::ReadData(L"QuadVS.cso");
    auto psBlob = DX::ReadData(L"QuadPS.cso");

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = FALSE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencilDesc.StencilEnable = FALSE;
    depthStencilDesc.StencilReadMask = 0xFF;
    depthStencilDesc.StencilWriteMask = 0xFF;
    depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    // PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElemDesc, static_cast<UINT>(std::size(inputElemDesc)) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { vsBlob.data(), vsBlob.size() };
    psoDesc.PS = { psBlob.data(), psBlob.size() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12XBOX_PRIMITIVE_TOPOLOGY_TYPE_RECT;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
    psoDesc.SampleDesc.Count = 1;
    DX::ThrowIfFailed(
        device->CreateGraphicsPipelineState(&psoDesc,
            IID_GRAPHICS_PPV_ARGS(m_rectPso.ReleaseAndGetAddressOf())));
    m_rectPso->SetName(L"Rect PSO");

    auto psWritePassStencilBlob = DX::ReadData(L"WriteStencilPixelsPS.cso");
    depthStencilDesc.StencilEnable = TRUE;
    depthStencilDesc.StencilReadMask = 0xFF;
    depthStencilDesc.StencilWriteMask = 0xFF;
    depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_BLEND_FACTOR;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_BLEND_FACTOR;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_BLEND_FACTOR;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_BLEND_FACTOR;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; // RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE;

    psoDesc.PS = { psWritePassStencilBlob.data(), psWritePassStencilBlob.size() };
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.BlendState = blendDesc;

    DX::ThrowIfFailed(
        device->CreateGraphicsPipelineState(&psoDesc,
            IID_GRAPHICS_PPV_ARGS(m_rectWithStencilPso.ReleaseAndGetAddressOf())));
    m_rectWithStencilPso->SetName(L"Rect with stencil PSO");
}

// Initialize all the fonts used
void Sample::InitializeSpriteFonts(ID3D12Device* device, ResourceUploadBatch& resourceUpload, const RenderTargetState& rtState)
{
    SpriteBatchPipelineStateDescription pd(
        rtState,
        &CommonStates::AlphaBlend);

    auto viewport = m_deviceResources->GetScreenViewport();
    m_fontBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd, &viewport);

    auto outputSize = m_deviceResources->GetOutputSize();
    m_fontText = std::make_unique<SpriteFont>(
        device,
        resourceUpload,
        (outputSize.bottom > 1080) ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
        m_textures->GetCpuDescriptorHandle(static_cast<size_t>(CommonHeader::DescriptorHeapEntry::TextFont)),
        m_textures->GetGpuDescriptorHandle(static_cast<size_t>(CommonHeader::DescriptorHeapEntry::TextFont)));

    m_fontController = std::make_unique<SpriteFont>(
        device,
        resourceUpload,
        (outputSize.bottom > 1080) ? L"XboxOneController.spritefont" : L"XboxOneControllerSmall.spritefont",
        m_textures->GetCpuDescriptorHandle(static_cast<size_t>(CommonHeader::DescriptorHeapEntry::ControllerFont)),
        m_textures->GetGpuDescriptorHandle(static_cast<size_t>(CommonHeader::DescriptorHeapEntry::ControllerFont)));
}
#pragma endregion
