//--------------------------------------------------------------------------------------
// Antialiasing.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Antialiasing.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadCompressedData.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

namespace
{
    const XMVECTORF32 c_eye = { 0.f, -119.f, -50.f, 0.f };
    constexpr float c_pitch = 0.f;
    constexpr float c_yaw = 0.f;

    const float c_zoomSource[4] = { 0.47f, 0.47f, 0.53f, 0.53f };
    const float c_zoomDest[4] = { 0.65f, 0.65f, 0.95f, 0.95f };

    struct FXAAConstantBuffer
    {
        float pixelWidth;
        float pixelHeight;
        uint32_t pad[2];
    };

    static_assert((sizeof(FXAAConstantBuffer) % 16) == 0, "CB should be 16-byte aligned");

    struct SMAAConstantBuffer
    {
        float subsamples[4];
        float pixelWidth;
        float pixelHeight;
        uint32_t pad[2];
    };

    static_assert((sizeof(SMAAConstantBuffer) % 16) == 0, "CB should be 16-byte aligned");
}

// SMAA textures
#include "Shaders\AreaTex.h"
#include "Shaders\SearchTex.h"

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_pitch(c_pitch),
    m_yaw(c_yaw),
    m_hardwareAA(true),
    m_msaaCount(2), // 4x MSAA
    m_antialias(AntialiasMethods::FXAA),
    m_smaaEdge(SMAAEdgeTechnique::DEPTH),
    m_zoomViewport{},
    m_zoomRect{}
{
    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        DXGI_FORMAT_D32_FLOAT, // Single-sample depth buffer needed for some render modes
        2);

    // Set up for post-process rendering.
    m_scene = std::make_unique<DX::RenderTexture>(m_deviceResources->GetBackBufferFormat());
    m_scene->SetClearColor(ATG::ColorsLinear::Background);

    m_scene2 = std::make_unique<DX::RenderTexture>(m_deviceResources->GetBackBufferFormat());
    m_scene2->SetClearColor(Colors::Black);

    m_depth = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R32_FLOAT);
    m_depth->SetClearColor(Colors::Black);

    m_depth2 = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R32_FLOAT);
    m_depth2->SetClearColor(Colors::Black);

    m_final = std::make_unique<DX::RenderTexture>(m_deviceResources->GetBackBufferFormat());
    m_final->SetClearColor(ATG::ColorsLinear::Background);

    m_pass1 = std::make_unique<DX::RenderTexture>(m_deviceResources->GetBackBufferFormat());
    m_pass1->SetClearColor(Colors::Black);

    m_pass2 = std::make_unique<DX::RenderTexture>(m_deviceResources->GetBackBufferFormat());
    m_pass2->SetClearColor(Colors::Black);

    // Set up for MSAA rendering.
    for (size_t j = 0; j < 3; ++j)
    {
        m_msaaHelper[j] = std::make_unique<DX::MSAAHelper>(m_deviceResources->GetBackBufferFormat(),
            m_deviceResources->GetDepthBufferFormat(),
            1u << static_cast<unsigned int>(j + 1));

        m_msaaHelper[j]->SetClearColor(ATG::ColorsLinear::Background);
    }
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
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %I64u", m_frame);

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const&)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        using ButtonState = GamePad::ButtonStateTracker;

        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.a == ButtonState::PRESSED)
        {
            int aa = static_cast<int>(m_antialias);
            aa = (aa + 1) % static_cast<int>(AntialiasMethods::Count);
            m_antialias = static_cast<AntialiasMethods>(aa);
        }
        else if (m_gamePadButtons.x == ButtonState::PRESSED)
        {
            int aa = static_cast<int>(m_antialias);
            aa = aa - 1;
            if (aa < 0)
                aa = static_cast<int>(AntialiasMethods::Count) - 1;
            m_antialias = static_cast<AntialiasMethods>(aa);
        }

        if (m_antialias != AntialiasMethods::SMAA2X)
        {
            if (m_gamePadButtons.b == ButtonState::PRESSED)
            {
                m_hardwareAA = !m_hardwareAA;
            }

            if (m_gamePadButtons.y == ButtonState::PRESSED)
            {
                m_msaaCount = std::max((m_msaaCount + 1) % 4, 1);
            }
        }

        if (m_antialias == AntialiasMethods::SMAA || m_antialias == AntialiasMethods::SMAA2X)
        {
            if (m_gamePadButtons.dpadLeft == ButtonState::PRESSED)
            {
                m_smaaEdge = SMAAEdgeTechnique::COLOR;
            }
            else if (m_gamePadButtons.dpadDown == ButtonState::PRESSED)
            {
                m_smaaEdge = SMAAEdgeTechnique::DEPTH;
            }
            else if (m_gamePadButtons.dpadRight == ButtonState::PRESSED)
            {
                m_smaaEdge = SMAAEdgeTechnique::LUMA;
            }
        }

        if (pad.IsLeftStickPressed())
        {
            m_pitch = c_pitch;
            m_yaw = c_yaw;
        }
        else
        {
            m_yaw += pad.thumbSticks.leftX * 0.1f;
            m_pitch += pad.thumbSticks.leftY * 0.1f;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    // Limit to avoid looking directly up or down
    const float limit = XM_PI / 2.0f - 0.01f;
    m_pitch = std::max(-limit, std::min(+limit, m_pitch));

    if (m_yaw > XM_PI)
    {
        m_yaw -= XM_PI * 2.f;
    }
    else if (m_yaw < -XM_PI)
    {
        m_yaw += XM_PI * 2.f;
    }

    XMVECTOR lookAt = XMVectorSet(
        sinf(m_yaw),
        m_pitch,
        cosf(m_yaw),
        0);

    m_view = XMMatrixLookToLH(c_eye, lookAt, g_XMIdentityR1);
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
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    if (m_antialias == AntialiasMethods::SMAA2X)
    {
        // Always use 2X MSAA.
        m_msaaHelper[0]->Prepare(commandList);
    }
    else if (m_hardwareAA)
    {
        // Use 2X, 4X, or 8X MSAA render targets.
        m_msaaHelper[m_msaaCount - 1]->Prepare(commandList);
    }
    else if (m_antialias != AntialiasMethods::NONE)
    {
        // Use post-processing.
        m_scene->BeginScene(commandList);
    }
    else
    {
        // Otherwise render directly to final result.
        m_final->BeginScene(commandList);
    }

    Clear();

    PIXEndEvent(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    RenderScene(commandList);

    // Resolve MSAA (if needed)
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Resolve");

    if (m_antialias == AntialiasMethods::SMAA2X)
    {
        D3D12_RESOURCE_BARRIER barriers[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_msaaHelper[0]->GetMSAARenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE), 
            CD3DX12_RESOURCE_BARRIER::Transition(m_msaaHelper[0]->GetMSAADepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        };
        commandList->ResourceBarrier(_countof(barriers), barriers);

        // Split MSAA color
        m_scene->BeginScene(commandList);
        m_scene2->BeginScene(commandList);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2] =
        {
            m_renderDescriptors->GetCpuHandle(RTDescriptors::SceneRTV),
            m_renderDescriptors->GetCpuHandle(RTDescriptors::SceneRTV2)
        };
        commandList->ClearRenderTargetView(rtvs[0], Colors::Black, 0, nullptr);
        commandList->ClearRenderTargetView(rtvs[1], Colors::Black, 0, nullptr);
        commandList->OMSetRenderTargets(2, rtvs, FALSE, nullptr);

        m_fullScreenQuad->Draw(commandList, m_seperatePSO.Get(), m_resourceDescriptors->GetGpuHandle(Descriptors::MSAAColorSRV));

        m_scene->EndScene(commandList);
        m_scene2->EndScene(commandList);

        // Split MSAA depth
        m_depth->BeginScene(commandList);
        m_depth2->BeginScene(commandList);

        rtvs[0] = m_renderDescriptors->GetCpuHandle(RTDescriptors::DepthRTV);
        rtvs[1] = m_renderDescriptors->GetCpuHandle(RTDescriptors::DepthRTV2);
        commandList->ClearRenderTargetView(rtvs[0], Colors::Black, 0, nullptr);
        commandList->ClearRenderTargetView(rtvs[1], Colors::Black, 0, nullptr);
        commandList->OMSetRenderTargets(2, rtvs, FALSE, nullptr);

        m_fullScreenQuad->Draw(commandList, m_seperateDepthPSO.Get(), m_resourceDescriptors->GetGpuHandle(Descriptors::MSAADepthSRV));

        m_depth->EndScene(commandList);
        m_depth2->EndScene(commandList);

        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_msaaHelper[0]->GetMSAARenderTarget(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
        barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_msaaHelper[0]->GetMSAADepthStencil(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        commandList->ResourceBarrier(_countof(barriers), barriers);
    }
    else if (m_antialias != AntialiasMethods::NONE)
    {
        // Use post-processing.
        if (m_hardwareAA)
        {
            m_msaaHelper[m_msaaCount - 1]->Resolve(commandList, m_scene->GetResource(),
                m_scene->GetCurrentState(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            m_scene->UpdateState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            if (m_antialias == AntialiasMethods::SMAA)
            {
                m_msaaHelper[m_msaaCount - 1]->ResolveDepth(commandList, m_depth->GetResource(),
                    m_depth->GetCurrentState(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

                m_depth->UpdateState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            }
        }
        else
        {
            m_scene->EndScene(commandList);
        }
    }
    else if (m_hardwareAA)
    {
        // No software AA, so resolve directly to final result.
        m_msaaHelper[m_msaaCount - 1]->Resolve(commandList, m_final->GetResource(),
            m_final->GetCurrentState(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        m_final->UpdateState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
    else
    {
        // Otherwise finish render directly to final result.
        m_final->EndScene(commandList);
    }

    PIXEndEvent(commandList);

    // Perform post-processing software anti-aliasing
    switch (m_antialias)
    {
    case AntialiasMethods::SMAA2X:
    {
        auto size = m_deviceResources->GetOutputSize();
        SMAAConstantBuffer cb = { { 1.f, 1.f, 1.f, 0.f }, 1.f / float(size.right), 1.f / float(size.bottom) };
        auto cbHandle = m_graphicsMemory->AllocateConstant(cb);
        RenderSMAA(commandList, Descriptors::SceneTex, Descriptors::DepthTex, 1.f, cbHandle.GpuAddress());

        cb = { { 2.f, 2.f, 2.f, 0.f }, 1.f / float(size.right), 1.f / float(size.bottom) };
        cbHandle = m_graphicsMemory->AllocateConstant(cb);
        RenderSMAA(commandList, Descriptors::SceneTex2, Descriptors::DepthTex2, 0.5f, cbHandle.GpuAddress());
        break;
    }
    case AntialiasMethods::SMAA:
    {
        if (!m_hardwareAA)
        {
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(),
                D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            commandList->ResourceBarrier(1, &barrier);
        }

        auto size = m_deviceResources->GetOutputSize();
        SMAAConstantBuffer cb = { { 0.f, 0.f, 0.f, 0.f }, 1.f / float(size.right), 1.f / float(size.bottom) };
        auto cbHandle = m_graphicsMemory->AllocateConstant(cb);
        RenderSMAA(commandList, Descriptors::SceneTex, m_hardwareAA ? Descriptors::DepthTex : Descriptors::DepthStencilSRV, 1.f, cbHandle.GpuAddress());

        if (!m_hardwareAA)
        {
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetDepthStencil(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

            commandList->ResourceBarrier(1, &barrier);
        }
        break;
    }

    case AntialiasMethods::FXAA:
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render FXAA");

        auto rtvDescriptor = m_renderDescriptors->GetCpuHandle(RTDescriptors::FinalRTV);
        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

        m_scene->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_final->BeginScene(commandList);

        auto size = m_deviceResources->GetOutputSize();
        FXAAConstantBuffer cb = { 1.f / float(size.right), 1.f / float(size.bottom) };
        auto cbHandle = m_graphicsMemory->AllocateConstant(cb);

        m_fullScreenQuad->Draw(commandList, m_fxaaPSO.Get(),
            m_resourceDescriptors->GetGpuHandle(Descriptors::SceneTex), cbHandle.GpuAddress());

        m_final->EndScene(commandList);

        PIXEndEvent(commandList);
        break;
    }

    case AntialiasMethods::NONE:
    default:
        // We rendered/resolved to m_final above
        break;
    }

    // Render final scene
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    m_final->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_fullScreenQuad->Draw(commandList, m_passthroughPSO.Get(), m_resourceDescriptors->GetGpuHandle(Descriptors::FinalTex));

    // Render zoom
    RenderZoom(commandList);

    // Render UI
    RenderUI(commandList);

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::RenderScene(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render scene");

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        m_modelResources->Heap(),
        m_states->Heap()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    XMMATRIX world = SimpleMath::Matrix::Identity;

    if (m_antialias == AntialiasMethods::SMAA2X)
    {
        // Always uses 2X MSAA
        Model::UpdateEffectMatrices(m_modelEffect[1], world, m_view, m_proj);

        m_model->Draw(commandList, m_modelEffect[1].cbegin());
    }
    else if (m_hardwareAA)
    {
        Model::UpdateEffectMatrices(m_modelEffect[m_msaaCount], world, m_view, m_proj);

        m_model->Draw(commandList, m_modelEffect[m_msaaCount].cbegin());
    }
    else
    {
        Model::UpdateEffectMatrices(m_modelEffect[0], world, m_view, m_proj);

        m_model->Draw(commandList, m_modelEffect[0].cbegin());
    }

    PIXEndEvent(commandList);
}

void Sample::RenderZoom(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render Zoom");
    commandList->RSSetViewports(1, &m_zoomViewport);
    commandList->RSSetScissorRects(1, &m_zoomRect);

    m_fullScreenQuad->Draw(commandList, m_zoomPSO.Get(), m_resourceDescriptors->GetGpuHandle(Descriptors::FinalTex));

    auto vp = m_deviceResources->GetScreenViewport();
    commandList->RSSetViewports(1, &vp);

    auto scissors = m_deviceResources->GetScissorRect();
    commandList->RSSetScissorRects(1, &scissors);

    m_lineEFfect->Apply(commandList);

    m_lines->Begin(commandList);

    XMFLOAT4 color(ATG::ColorsLinear::Orange);

    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomSource[0], c_zoomSource[1], 0}, color }, VertexPositionColor{ XMFLOAT3{ c_zoomSource[0], c_zoomSource[3], 0 }, color });
    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomSource[0], c_zoomSource[3], 0}, color }, VertexPositionColor{ XMFLOAT3{c_zoomSource[2], c_zoomSource[3], 0}, color });
    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomSource[2], c_zoomSource[3], 0}, color }, VertexPositionColor{ XMFLOAT3{c_zoomSource[2], c_zoomSource[1], 0}, color });
    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomSource[2], c_zoomSource[1], 0}, color }, VertexPositionColor{ XMFLOAT3{c_zoomSource[0], c_zoomSource[1], 0}, color });

    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomDest[0], c_zoomDest[1], 0}, color }, VertexPositionColor{ XMFLOAT3{ c_zoomDest[0], c_zoomDest[3], 0 }, color });
    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomDest[0], c_zoomDest[3], 0}, color }, VertexPositionColor{ XMFLOAT3{c_zoomDest[2], c_zoomDest[3], 0}, color });
    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomDest[2], c_zoomDest[3], 0}, color }, VertexPositionColor{ XMFLOAT3{c_zoomDest[2], c_zoomDest[1], 0}, color });
    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomDest[2], c_zoomDest[1], 0}, color }, VertexPositionColor{ XMFLOAT3{c_zoomDest[0], c_zoomDest[1], 0}, color });

    m_lines->End();

    PIXEndEvent(commandList);
}

void Sample::RenderUI(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render UI");

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        m_resourceDescriptors->Heap()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    auto size = m_deviceResources->GetOutputSize();
    auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));

    m_batch->Begin(commandList);

    float y = float(safe.top);

    m_font->DrawString(m_batch.get(), L"Antialiasing",
        XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    y += m_font->GetLineSpacing() * 1.5f;

    if (m_antialias == AntialiasMethods::SMAA2X)
    {
        wchar_t buff[64] = {};
        swprintf_s(buff, L"MSAA (2 samples)");

        DX::DrawControllerString(m_batch.get(), m_font.get(), m_colorCtrlFont.get(), buff,
            XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
    }
    else if (m_hardwareAA)
    {
        wchar_t buff[64] = {};
        swprintf_s(buff, L"[B] MSAA (%uX samples [Y])", 1u << m_msaaCount);

        DX::DrawControllerString(m_batch.get(), m_font.get(), m_colorCtrlFont.get(), buff,
            XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
    }
    else
    {
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_colorCtrlFont.get(),
            L"[B] No MSAA",
            XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
    }

    y += m_font->GetLineSpacing();

    const wchar_t* shaderAAStr = L"";

    switch (m_antialias)
    {
    case AntialiasMethods::FXAA: shaderAAStr = L"[A]/[X] Shader AA: FXAA"; break;
    case AntialiasMethods::SMAA: shaderAAStr = L"[A]/[X] Shader AA: SMAA 1X"; break;
    case AntialiasMethods::SMAA2X: shaderAAStr = L"[A]/[X] Shader AA: SMAA 2X"; break;
    case AntialiasMethods::NONE:
    case AntialiasMethods::Count:
    default:  shaderAAStr = L"[A]/[X] Shader AA: NONE"; break;
    }

    DX::DrawControllerString(m_batch.get(), m_font.get(), m_colorCtrlFont.get(),
        shaderAAStr,
        XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);

    if (m_antialias == AntialiasMethods::SMAA || m_antialias == AntialiasMethods::SMAA2X)
    {
        y += m_font->GetLineSpacing();

        const wchar_t* edgeTechniqueStr = L"";

        switch (m_smaaEdge)
        {
        default:
        case SMAAEdgeTechnique::DEPTH: edgeTechniqueStr = L"[DPad] SMAA Edge Detection: Depth"; break;
        case SMAAEdgeTechnique::COLOR: edgeTechniqueStr = L"[DPad] SMAA Edge Detection: Color"; break;
        case SMAAEdgeTechnique::LUMA: edgeTechniqueStr = L"[DPad] SMAA Edge Detection: Luma"; break;
        }

        DX::DrawControllerString(m_batch.get(), m_font.get(), m_colorCtrlFont.get(),
            edgeTechniqueStr,
            XMFLOAT2(float(safe.left), y), ATG::Colors::LightGrey);
    }

    const wchar_t* legendStr = L"[View] Exit   [LThumb] Rotate";

    DX::DrawControllerString(m_batch.get(),
        m_font.get(), m_ctrlFont.get(),
        legendStr, XMFLOAT2(float(safe.left), float(safe.bottom)),
        ATG::Colors::LightGrey);

    m_batch->End();

    PIXEndEvent(commandList);
}

void Sample::RenderSMAA(ID3D12GraphicsCommandList* commandList, size_t renderTargetSRV, size_t depthSRV, float blendFactor, D3D12_GPU_VIRTUAL_ADDRESS cb)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render SMAA1X");

    auto colorHandle = m_resourceDescriptors->GetGpuHandle(renderTargetSRV);
    auto depthHandle = m_resourceDescriptors->GetGpuHandle(depthSRV);

    // Pass 1: Render SMAA Edges
    m_pass1->BeginScene(commandList);

    auto rtvDescriptor = m_renderDescriptors->GetCpuHandle(RTDescriptors::PassRTV1);
    commandList->ClearRenderTargetView(rtvDescriptor, Colors::Black, 0, nullptr);
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    m_fullScreenQuad->Draw(commandList,
        m_edgeDetectPSO[static_cast<int>(m_smaaEdge)].Get(), colorHandle, depthHandle, cb);

    m_pass1->EndScene(commandList);

    // Pass 2: Render SMAA Blend Weights
    m_pass2->BeginScene(commandList);

    rtvDescriptor = m_renderDescriptors->GetCpuHandle(RTDescriptors::PassRTV2);
    commandList->ClearRenderTargetView(rtvDescriptor, Colors::Black, 0, nullptr);
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    m_fullScreenQuad->Draw(commandList, m_blendWeightsPSO.Get(),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Pass1),
        m_resourceDescriptors->GetGpuHandle(Descriptors::AreaTex),
        m_resourceDescriptors->GetGpuHandle(Descriptors::SearchTex),
        cb);

    m_pass2->EndScene(commandList);

    // Pass 3: Render SMAA Neighborhood Blending
    m_final->BeginScene(commandList);

    rtvDescriptor = m_renderDescriptors->GetCpuHandle(RTDescriptors::FinalRTV);
    if (blendFactor == 1.f)
    {
        commandList->ClearRenderTargetView(rtvDescriptor, Colors::Black, FALSE, nullptr);
    }

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    float factor[4] = { blendFactor, blendFactor, blendFactor, blendFactor };
    commandList->OMSetBlendFactor(factor);

    m_fullScreenQuad->Draw(commandList, m_neighborBlendPSO.Get(), colorHandle,
        m_resourceDescriptors->GetGpuHandle(Descriptors::Pass2), cb);

    m_final->EndScene(commandList);

    PIXEndEvent(commandList);
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = {};
    D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor = {};

    if (m_antialias == AntialiasMethods::SMAA2X)
    {
        // Always use 2X MSAA.
        rtvDescriptor = m_msaaHelper[0]->GetMSAARenderTargetView();
        dsvDescriptor = m_msaaHelper[0]->GetMSAADepthStencilView();
    }
    else if (m_hardwareAA)
    {
        // Use 2X, 4X, or 8X MSAA render targets.
        rtvDescriptor = m_msaaHelper[m_msaaCount - 1]->GetMSAARenderTargetView();
        dsvDescriptor = m_msaaHelper[m_msaaCount - 1]->GetMSAADepthStencilView();
    }
    else
    {
        rtvDescriptor = m_renderDescriptors->GetCpuHandle((m_antialias == AntialiasMethods::NONE)
            ? RTDescriptors::FinalRTV
            : RTDescriptors::SceneRTV);
        dsvDescriptor = m_deviceResources->GetDepthStencilView();
    }

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    // Use linear clear color for gamma-correct rendering.
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
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

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    m_renderDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        RTDescriptors::RTCount);

    m_states = std::make_unique<CommonStates>(device);

    {
        auto modelBlob = DX::ReadCompressedData(L"AbstractCathedral.sdkmes_");

        m_model = Model::CreateFromSDKMESH(device, modelBlob.data(), modelBlob.size());
    }

    // Upload resources to video memory
    ResourceUploadBatch upload(device);
    upload.Begin();

    m_modelResources = std::make_unique<EffectTextureFactory>(device, upload, m_model->textureNames.size());
    m_modelResources->EnableForceSRGB(true);
    m_model->LoadTextures(*m_modelResources);

    m_fxFactory = std::make_unique<EffectFactory>(m_modelResources->Heap(), m_states->Heap());

    m_model->LoadStaticBuffers(device, upload);

    RenderTargetState rtStateUI(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    {
        SpriteBatchPipelineStateDescription pd(rtStateUI, &CommonStates::AlphaBlend);
        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    {
        // Create SMAA data textures (used in pass 2)
        D3D12_SUBRESOURCE_DATA initData = { areaTexBytes, AREATEX_PITCH, 0 };

        DX::ThrowIfFailed(
            CreateTextureFromMemory(device, upload, AREATEX_WIDTH, AREATEX_HEIGHT, DXGI_FORMAT_R8G8_UNORM,
                initData, m_areaTex.ReleaseAndGetAddressOf())
        );

        m_areaTex->SetName(L"SMAA AreaTex");

        CreateShaderResourceView(device, m_areaTex.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::AreaTex));

        initData = { searchTexBytes, SEARCHTEX_PITCH, 0 };

        DX::ThrowIfFailed(
            CreateTextureFromMemory(device, upload, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, DXGI_FORMAT_R8_UNORM,
                initData, m_searchTex.ReleaseAndGetAddressOf())
        );

        m_searchTex->SetName(L"SMAA SearchTex");

        CreateShaderResourceView(device, m_searchTex.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::SearchTex));
    }

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    m_deviceResources->WaitForGpu();
    finish.wait();

    // Render targets
    m_scene->SetDevice(device,
        m_resourceDescriptors->GetCpuHandle(Descriptors::SceneTex),
        m_renderDescriptors->GetCpuHandle(RTDescriptors::SceneRTV));

    m_scene2->SetDevice(device,
        m_resourceDescriptors->GetCpuHandle(Descriptors::SceneTex2),
        m_renderDescriptors->GetCpuHandle(RTDescriptors::SceneRTV2));

    m_depth->SetDevice(device,
        m_resourceDescriptors->GetCpuHandle(Descriptors::DepthTex),
        m_renderDescriptors->GetCpuHandle(RTDescriptors::DepthRTV));

    m_depth2->SetDevice(device,
        m_resourceDescriptors->GetCpuHandle(Descriptors::DepthTex2),
        m_renderDescriptors->GetCpuHandle(RTDescriptors::DepthRTV2));

    m_final->SetDevice(device,
        m_resourceDescriptors->GetCpuHandle(Descriptors::FinalTex),
        m_renderDescriptors->GetCpuHandle(RTDescriptors::FinalRTV));

    m_pass1->SetDevice(device,
        m_resourceDescriptors->GetCpuHandle(Descriptors::Pass1),
        m_renderDescriptors->GetCpuHandle(RTDescriptors::PassRTV1));

    m_pass2->SetDevice(device,
        m_resourceDescriptors->GetCpuHandle(Descriptors::Pass2),
        m_renderDescriptors->GetCpuHandle(RTDescriptors::PassRTV2));

    for (size_t j = 0; j < 3; ++j)
    {
        m_msaaHelper[j]->SetDevice(device);
    }

    // Scene effects
    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    {
        EffectPipelineStateDescription pdOpaque(
            nullptr,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            rtState);

        EffectPipelineStateDescription pdAlpha(
            nullptr,
            CommonStates::AlphaBlend,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            rtState);

        m_modelEffect[0] = m_model->CreateEffects(*m_fxFactory, pdOpaque, pdAlpha);

        for (uint32_t i = 0; i < 3; ++i)
        {
            rtState.sampleDesc.Count = 1u << (i + 1);
            pdOpaque.renderTargetState = rtState;
            pdAlpha.renderTargetState = rtState;

            m_modelEffect[i + 1] = m_model->CreateEffects(*m_fxFactory, pdOpaque, pdAlpha);
        }
    }

    // Line drawing for Zoom window
    {
        m_lines = std::make_unique<PrimitiveBatch<VertexPositionColor>>(device);

        EffectPipelineStateDescription pd(
            &VertexPositionColor::InputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullNone,
            rtStateUI,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

        m_lineEFfect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);

        Matrix proj = Matrix::CreateScale(2.f, -2.f, 1.f)
            * Matrix::CreateTranslation(-1.f, 1.f, 0.f);
        m_lineEFfect->SetProjection(proj);
    }

    // Post-process quad
    m_fullScreenQuad = std::make_unique<DX::FullScreenQuad>();
    m_fullScreenQuad->Initialize(device);

    {
        EffectPipelineStateDescription pd(
            nullptr,
            CommonStates::Opaque,
            CommonStates::DepthNone,
            CommonStates::CullNone,
            rtStateUI);

        auto vertexShaderBlob = DX::ReadData(L"FullScreenQuadVS.cso");
        auto pixelShaderBlob = DX::ReadData(L"FullScreenQuadPS.cso");

        D3D12_SHADER_BYTECODE vertexShader =
        { vertexShaderBlob.data(), vertexShaderBlob.size() };
        D3D12_SHADER_BYTECODE pixelShader =
        { pixelShaderBlob.data(), pixelShaderBlob.size() };

        auto rootSig = m_fullScreenQuad->GetRootSignature();
        pd.CreatePipelineState(
            device,
            rootSig,
            vertexShader,
            pixelShader,
            m_passthroughPSO.ReleaseAndGetAddressOf());

        // FXAA
        pixelShaderBlob = DX::ReadData(L"FXAA.cso");
        pixelShader = { pixelShaderBlob.data(), pixelShaderBlob.size() };

        pd.CreatePipelineState(
            device,
            rootSig,
            vertexShader,
            pixelShader,
            m_fxaaPSO.ReleaseAndGetAddressOf());

        // Zoom View
        vertexShaderBlob = DX::ReadData(L"ZoomViewVS.cso");
        pixelShaderBlob = DX::ReadData(L"ZoomViewPS.cso");

        vertexShader = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        pixelShader = { pixelShaderBlob.data(), pixelShaderBlob.size() };

        pd.CreatePipelineState(
            device,
            rootSig,
            vertexShader,
            pixelShader,
            m_zoomPSO.ReleaseAndGetAddressOf());
    }

    {
        // SMAA2X split samples
        EffectPipelineStateDescription pd(
            nullptr,
            CommonStates::Opaque,
            CommonStates::DepthNone,
            CommonStates::CullNone,
            rtStateUI);

        pd.renderTargetState.numRenderTargets = 2;
        pd.renderTargetState.rtvFormats[0] = pd.renderTargetState.rtvFormats[1] = m_deviceResources->GetBackBufferFormat();

        auto vertexShaderBlob = DX::ReadData(L"FullScreenQuadVS.cso");
        auto pixelShaderBlob = DX::ReadData(L"SMAA2XSeperate.cso");

        D3D12_SHADER_BYTECODE vertexShader =
        { vertexShaderBlob.data(), vertexShaderBlob.size() };
        D3D12_SHADER_BYTECODE pixelShader =
        { pixelShaderBlob.data(), pixelShaderBlob.size() };

        auto rootSig = m_fullScreenQuad->GetRootSignature();
        pd.CreatePipelineState(
            device,
            rootSig,
            vertexShader,
            pixelShader,
            m_seperatePSO.ReleaseAndGetAddressOf());

        pd.renderTargetState.rtvFormats[0] = pd.renderTargetState.rtvFormats[1] = DXGI_FORMAT_R32_FLOAT;

        pd.CreatePipelineState(
            device,
            rootSig,
            vertexShader,
            pixelShader,
            m_seperateDepthPSO.ReleaseAndGetAddressOf());
    }

    {
        // SMAA Edge detection (pass 1)
        EffectPipelineStateDescription pd(
            nullptr,
            CommonStates::Opaque,
            CommonStates::DepthNone,
            CommonStates::CullNone,
            rtStateUI);

        auto vertexShaderBlob = DX::ReadData(L"SMAAEdgeVS.cso");
        auto pixelShaderBlob = DX::ReadData(L"SMAAEdgeDepth.cso");

        D3D12_SHADER_BYTECODE vertexShader =
        { vertexShaderBlob.data(), vertexShaderBlob.size() };
        D3D12_SHADER_BYTECODE pixelShader =
        { pixelShaderBlob.data(), pixelShaderBlob.size() };

        auto rootSig = m_fullScreenQuad->GetRootSignature();
        pd.CreatePipelineState(
            device,
            rootSig,
            vertexShader,
            pixelShader,
            m_edgeDetectPSO[static_cast<int>(SMAAEdgeTechnique::DEPTH)].ReleaseAndGetAddressOf());

        pixelShaderBlob = DX::ReadData(L"SMAAEdgeColor.cso");
        pixelShader = { pixelShaderBlob.data(), pixelShaderBlob.size() };

        pd.CreatePipelineState(
            device,
            rootSig,
            vertexShader,
            pixelShader,
            m_edgeDetectPSO[static_cast<int>(SMAAEdgeTechnique::COLOR)].ReleaseAndGetAddressOf());

        pixelShaderBlob = DX::ReadData(L"SMAAEdgeLuma.cso");
        pixelShader = { pixelShaderBlob.data(), pixelShaderBlob.size() };

        pd.CreatePipelineState(
            device,
            rootSig,
            vertexShader,
            pixelShader,
            m_edgeDetectPSO[static_cast<int>(SMAAEdgeTechnique::LUMA)].ReleaseAndGetAddressOf());
    }

    {
        // SMAA Blend Weights (pass 2)
        EffectPipelineStateDescription pd(
            nullptr,
            CommonStates::Opaque,
            CommonStates::DepthNone,
            CommonStates::CullNone,
            rtStateUI);

        auto vertexShaderBlob = DX::ReadData(L"SMAABlendWeightsVS.cso");
        auto pixelShaderBlob = DX::ReadData(L"SMAABlendWeightsPS.cso");

        D3D12_SHADER_BYTECODE vertexShader =
        { vertexShaderBlob.data(), vertexShaderBlob.size() };
        D3D12_SHADER_BYTECODE pixelShader =
        { pixelShaderBlob.data(), pixelShaderBlob.size() };

        pd.CreatePipelineState(
            device,
            m_fullScreenQuad->GetRootSignature(),
            vertexShader,
            pixelShader,
            m_blendWeightsPSO.ReleaseAndGetAddressOf());
    }

    {
        // SMAA Neighborhood Blending (pass 3)
        auto blendFactor = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        blendFactor.RenderTarget[0].BlendEnable = TRUE;
        blendFactor.RenderTarget[0].SrcBlend = D3D12_BLEND_BLEND_FACTOR;
        blendFactor.RenderTarget[0].DestBlend = D3D12_BLEND_INV_BLEND_FACTOR;
        blendFactor.RenderTarget[0].SrcBlendAlpha = blendFactor.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendFactor.RenderTarget[0].BlendOp = blendFactor.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendFactor.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
        blendFactor.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        EffectPipelineStateDescription pd(
            nullptr,
            blendFactor,
            CommonStates::DepthNone,
            CommonStates::CullNone,
            rtStateUI);

        auto vertexShaderBlob = DX::ReadData(L"SMAANeighborBlendVS.cso");
        auto pixelShaderBlob = DX::ReadData(L"SMAANeighborBlendPS.cso");

        D3D12_SHADER_BYTECODE vertexShader =
        { vertexShaderBlob.data(), vertexShaderBlob.size() };
        D3D12_SHADER_BYTECODE pixelShader =
        { pixelShaderBlob.data(), pixelShaderBlob.size() };

        pd.CreatePipelineState(
            device,
            m_fullScreenQuad->GetRootSignature(),
            vertexShader,
            pixelShader,
            m_neighborBlendPSO.ReleaseAndGetAddressOf());
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    m_batch->SetViewport(m_deviceResources->GetScreenViewport());

    auto size = m_deviceResources->GetOutputSize();

    auto device = m_deviceResources->GetD3DDevice();
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_font = std::make_unique<SpriteFont>(device, resourceUpload,
        (size.bottom > 1080) ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Font));

    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        (size.bottom > 1080) ? L"XboxOneControllerLegend.spritefont" : L"XboxOneControllerLegendSmall.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::CtrlFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::CtrlFont));

    m_colorCtrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        (size.bottom > 1080) ? L"XboxOneController.spritefont" : L"XboxOneControllerSmall.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::ColorCtrlFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ColorCtrlFont));

    // Wait until assets have been uploaded to the GPU.
    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    // Offscreen render targets
    m_scene->SetWindow(size);
    m_scene2->SetWindow(size);
    m_depth->SetWindow(size);
    m_depth2->SetWindow(size);
    m_final->SetWindow(size);
    m_pass1->SetWindow(size);
    m_pass2->SetWindow(size);

    // Set specific resource names and create some SRVs
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        
        device->CreateShaderResourceView(m_deviceResources->GetDepthStencil(), &srvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::DepthStencilSRV));
    }

    auto res = m_scene->GetResource();
    if (res)
        res->SetName(L"Scene A");

    res = m_scene2->GetResource();
    if (res)
        res->SetName(L"Scene B");

    res = m_depth->GetResource();
    if (res)
        res->SetName(L"Scene Depth A");

    res = m_depth2->GetResource();
    if (res)
        res->SetName(L"Scene Depth B");

    res = m_final->GetResource();
    if (res)
        res->SetName(L"Final Scene");

    res = m_pass1->GetResource();
    if (res)
        res->SetName(L"SMAA First Pass");

    res = m_pass2->GetResource();
    if (res)
        res->SetName(L"SMAA Second Pass");

    for (size_t j = 0; j < 3; ++j)
    {
        m_msaaHelper[j]->SetWindow(size);

        wchar_t buff[64] = {};
        swprintf_s(buff, L"MSAA %uX Render Target", 1u << (j + 1));
        res = m_msaaHelper[j]->GetMSAARenderTarget();
        if (res)
        {
            res->SetName(buff);

            if (!j)
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = m_msaaHelper[0]->GetBackBufferFormat();
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;

                device->CreateShaderResourceView(res, &srvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::MSAAColorSRV));
            }
        }

        swprintf_s(buff, L"MSAA %uX Depth/Stencil Target", 1u << (j + 1));
        res = m_msaaHelper[j]->GetMSAADepthStencil();
        if (res)
        {
            res->SetName(buff);

            if (!j)
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;

                device->CreateShaderResourceView(res, &srvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::MSAADepthSRV));
            }
        }
    }

    // Camera
    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), 0.1f, 10000.f);

    // Set zoom rectangles
    m_zoomViewport.TopLeftX = c_zoomDest[0] * float(size.right);
    m_zoomViewport.TopLeftY = c_zoomDest[1] * float(size.bottom);
    m_zoomViewport.Width = (c_zoomDest[2] - c_zoomDest[0]) * float(size.right);
    m_zoomViewport.Height = (c_zoomDest[3] - c_zoomDest[1]) * float(size.bottom);
    m_zoomViewport.MinDepth = 0.0f;
    m_zoomViewport.MaxDepth = 1.0f;

    m_zoomRect.left = LONG(m_zoomViewport.TopLeftX);
    m_zoomRect.top = LONG(m_zoomViewport.TopLeftY);
    m_zoomRect.right = m_zoomRect.left + LONG(m_zoomViewport.Width);
    m_zoomRect.bottom = m_zoomRect.top + LONG(m_zoomViewport.Height);
}
#pragma endregion
