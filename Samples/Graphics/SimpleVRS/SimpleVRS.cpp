//--------------------------------------------------------------------------------------
// SimpleVRS.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleVRS.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

// Cloud Sprite Rendering and Physics constants
DirectX::XMUINT2 Sample::c_cloudTextureSize = DirectX::XMUINT2(0, 0);
DirectX::SimpleMath::Vector2 Sample::c_spriteSpeed = Vector2(-1.0f * (float)c_maxSpeed * 0.6f, 0.0f);
float Sample::c_uiScale = 0.75f;

namespace
{
    struct TriangleVertex
    {
        XMFLOAT4 position;
        XMFLOAT4 color;
    };

    enum class TimerCounters
    {
        TotalFrameTime,
        VRSImage,
        Scene,
        TotalTimers,
    };

    const wchar_t* TimerCounterNames[] =
    {
        L"Total Frame Time",
        L"VRS Image Generation",
        L"Scene",
    };
}

void Sample::AllocateShadingRateImage(UINT width, UINT height)
{
    auto device = m_deviceResources->GetD3DDevice();

    // Allocate GPU memory for VRS Shading rate image and describe the resource
    CD3DX12_RESOURCE_DESC shadingRateImageDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8_UINT, // Required Shading Rate Image Format
        static_cast<UINT64>(width),
        height,
        1,
        1,
        1,
        0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &shadingRateImageDesc,
        D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_shadingRateImage.ReleaseAndGetAddressOf())));

    D3D12_RESOURCE_ALLOCATION_INFO info = device->GetResourceAllocationInfo(0, 1, &shadingRateImageDesc);
    m_shadingRateImageSizeBytes = UINT(info.SizeInBytes);

    m_shadingRateImage->SetName(L"VRS Image");

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = shadingRateImageDesc.Format; 
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    device->CreateUnorderedAccessView(m_shadingRateImage.Get(), nullptr, nullptr, m_SRVUAVDescriptorHeap->GetCpuHandle(Descriptors::ShadingRateImageUAV));
    device->CreateShaderResourceView(m_shadingRateImage.Get(), &srvDesc, m_SRVUAVDescriptorHeap->GetCpuHandle(Descriptors::ShadingRateImageSRV));

    // Store texture dimensions
    m_shadingRateImageWidth = float(width);
    m_shadingRateImageHeight = float(height);
}

void Sample::SpriteState::Reset(float screenWidth, float screenHeight, float offset)
{
    Position.x = screenWidth + offset;
    Position.y = screenHeight * 0.05f;
}

void Sample::SpriteState::Update(float time, float screenWidth, float screenHeight)
{
    Position += c_spriteSpeed * time;

    // Once Cloud sprite travels off-screen, reset its position
    if (Position.x < -1.0f * (c_spriteScaleFactor * (float)c_cloudTextureSize.x ))
    {
        Reset(screenWidth, screenHeight);
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        DXGI_FORMAT_D32_FLOAT,
        2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
}

// Initialize all the fonts used
void Sample::InitializeSpriteFonts(ID3D12Device* device, ResourceUploadBatch& resourceUpload, const RenderTargetState& rtState)
{
    SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::AlphaBlend);
    m_fontBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

    auto const outputSize = m_deviceResources->GetOutputSize();
    m_fontText = std::make_unique<SpriteFont>(
        device,
        resourceUpload,
        L"SegoeUI_18.spritefont",
        m_SRVUAVDescriptorHeap->GetCpuHandle(Descriptors::TextFont),
        m_SRVUAVDescriptorHeap->GetGpuHandle(Descriptors::TextFont));

    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        (outputSize.bottom > 1440) ? L"XboxOneControllerLegend.spritefont" : L"XboxOneControllerLegendSmall.spritefont",
        m_SRVUAVDescriptorHeap->GetCpuHandle(Descriptors::ControlFont),
        m_SRVUAVDescriptorHeap->GetGpuHandle(Descriptors::ControlFont));
}

void Sample::InitializeCloudAlphaBuffer()
{
    m_cloudAlphaScene = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R8_UNORM);
    XMVECTORF32 color = {};
    color.v = XMColorSRGBToRGB(Colors::Black);
    m_cloudAlphaScene->SetClearColor(color);
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_deviceResources->SetWindow(window);
    m_gamePad = std::make_unique<GamePad>();
    m_deviceResources->CreateDeviceResources();
    m_deviceResources->CreateWindowSizeDependentResources();
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
    AllocateShadingRateImage((UINT)m_deviceResources->GetScreenViewport().Width / c_VRSTileSize, (UINT)m_deviceResources->GetScreenViewport().Height / c_VRSTileSize);
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
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");
    const float elapsedSeconds = static_cast<float>(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_VRSEnabled = !m_VRSEnabled;
            if (!m_VRSEnabled) m_shadingRateVisualizationEnabled = false;
            m_gpuTimer->Reset();
        }

        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            if (m_VRSEnabled) m_shadingRateVisualizationEnabled = !m_shadingRateVisualizationEnabled;
            m_gpuTimer->Reset();
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    // Update Cloud Sprite Positions Across Screen 
    auto const frameWidth = m_deviceResources->GetScreenViewport().Width;
    auto const frameHeight = m_deviceResources->GetScreenViewport().Height;

    for (unsigned int i = 0; i < m_spriteStates.size(); i++)
    {
        m_spriteStates[i]->Update(elapsedSeconds, frameWidth, frameHeight);
    }

    PIXEndEvent();
}
#pragma endregion

#pragma region Frame Render

void Sample::RenderUI(ID3D12GraphicsCommandList* graphicsCmdList)
{
    auto const size = m_deviceResources->GetOutputSize();
    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea((UINT)size.right, (UINT)size.bottom);

    PIXBeginEvent(graphicsCmdList, PIX_COLOR_DEFAULT, L"Render UI");
    {
        // Draw dark semi-transparent rectangles behind UI text
        // to make the words more visible and distinguishable from
        // the rendered scene in the background.

        m_basicEffect->Apply(graphicsCmdList);
        m_vertexBatch->Begin(graphicsCmdList);

        // GPU Timing UI Rectangle
        VertexPositionColor vGPU0(Vector3(c_minX, c_maxYGPUTiming, c_UIRectangleSceneDepth), c_UIBackground);
        VertexPositionColor vGPU1(Vector3(c_maxX, c_maxYGPUTiming, c_UIRectangleSceneDepth), c_UIBackground);
        VertexPositionColor vGPU2(Vector3(c_maxX, c_minYGPUTiming, c_UIRectangleSceneDepth), c_UIBackground);
        VertexPositionColor vGPU3(Vector3(c_minX, c_minYGPUTiming, c_UIRectangleSceneDepth), c_UIBackground);

        // Shading Rate Visualization UI Rectangle
        VertexPositionColor vShowShadingRates0(Vector3(c_minX, c_maxYVRSVisualization, c_UIRectangleSceneDepth), c_UIBackground);
        VertexPositionColor vShowShadingRates1(Vector3(c_maxX, c_maxYVRSVisualization, c_UIRectangleSceneDepth), c_UIBackground);
        VertexPositionColor vShowShadingRates2(Vector3(c_maxX, c_minYVRSVisualization, c_UIRectangleSceneDepth), c_UIBackground);
        VertexPositionColor vShowShadingRates3(Vector3(c_minX, c_minYVRSVisualization, c_UIRectangleSceneDepth), c_UIBackground);

        // Sample Control Instructions UI Rectangle
        VertexPositionColor vControls0(Vector3(c_minX, c_maxYControls, c_UIRectangleSceneDepth), c_UIBackground);
        VertexPositionColor vControls1(Vector3(c_maxX, c_maxYControls, c_UIRectangleSceneDepth), c_UIBackground);
        VertexPositionColor vControls2(Vector3(c_maxX, c_minYControls, c_UIRectangleSceneDepth), c_UIBackground);
        VertexPositionColor vControls3(Vector3(c_minX, c_minYControls, c_UIRectangleSceneDepth), c_UIBackground);

        m_vertexBatch->DrawQuad(vGPU0, vGPU1, vGPU2, vGPU3);
        m_vertexBatch->DrawQuad(vControls0, vControls1, vControls2, vControls3);

        if (m_shadingRateVisualizationEnabled)
        {
            m_vertexBatch->DrawQuad(vShowShadingRates0, vShowShadingRates1, vShowShadingRates2, vShowShadingRates3);
        }

        m_vertexBatch->End();

        XMVECTOR textPosition = XMVectorSet(100, 100, 0, 1);
        XMVECTOR textDiffInY = { 0.f,  45.f };
        XMVECTOR textGroupDiffInY = { 0.f, 150.f };

        XMVECTOR textColor = ATG::Colors::OffWhite;
        XMVECTOR textPositionMoveX;
        auto const viewportUI = m_deviceResources->GetScreenViewport();

        m_fontBatch->SetViewport(viewportUI);
        m_fontBatch->Begin(graphicsCmdList);
        wchar_t strText[1024] = {};

        m_fontText->DrawString(m_fontBatch.get(), L"SimpleVRS Sample",
            textPosition, ATG::Colors::OffWhite, 0, g_XMZero, c_uiScale);

        textPosition = XMVectorAdd(textPosition, textDiffInY);

        // Render pass timings
        for (uint32_t passID = 0; passID < static_cast<uint32_t>(TimerCounters::TotalTimers); ++passID)
        {
            textPosition = XMVectorAdd(textPosition, textDiffInY);
            swprintf_s(strText, _countof(strText) - 1, L"%ls", TimerCounterNames[passID]);
            m_fontText->DrawString(m_fontBatch.get(), strText, textPosition, textColor, 0, g_XMZero, c_uiScale);

            textPositionMoveX = XMVectorAdd(textPosition, XMVectorSet(100, 0, 0, 0));
            swprintf_s(strText, _countof(strText) - 1, L"GPU Average: %.2f ms", m_gpuTimer->GetAverageMS(passID));
            textPositionMoveX = XMVectorAdd(textPositionMoveX, textDiffInY);
            m_fontText->DrawString(m_fontBatch.get(), strText, textPositionMoveX, textColor, 0, g_XMZero, c_uiScale);

            textPosition = XMVectorSetY(textPosition, XMVectorGetY(textPositionMoveX));
        }

        textPosition = XMVectorAdd(textPosition, textDiffInY);

        textPosition = XMVectorAdd(textPosition, textDiffInY);
        swprintf_s(strText, _countof(strText) - 1, L"VRS Screen Space Image: %ls", (m_VRSEnabled) ? L"Enabled" : L"Disabled");
        m_fontText->DrawString(m_fontBatch.get(), strText, textPosition, textColor, 0, g_XMZero, c_uiScale);

        textPosition = XMVectorAdd(textPosition, textDiffInY);
        swprintf_s(strText, _countof(strText) - 1, L"Shading Rate Visualization: %ls", (m_shadingRateVisualizationEnabled) ? L"Enabled" : L"Disabled");
        m_fontText->DrawString(m_fontBatch.get(), strText, textPosition, textColor, 0, g_XMZero, c_uiScale);

        if (m_shadingRateVisualizationEnabled)
        {
            textPosition = XMVectorAdd(textPosition, textGroupDiffInY);
            swprintf_s(strText, _countof(strText) - 1, L"Shading Rate Visualization Legend:");
            m_fontText->DrawString(m_fontBatch.get(), strText, textPosition, textColor, 0, g_XMZero, c_uiScale);

            textPosition = XMVectorAdd(textPosition, textDiffInY);
            swprintf_s(strText, _countof(strText) - 1, L"1X1 Rate == Red");
            m_fontText->DrawString(m_fontBatch.get(), strText, textPosition, textColor, 0, g_XMZero, c_uiScale);

            textPosition = XMVectorAdd(textPosition, textDiffInY);
            swprintf_s(strText, _countof(strText) - 1, L"1X2 Rate == Yellow");
            m_fontText->DrawString(m_fontBatch.get(), strText, textPosition, textColor, 0, g_XMZero, c_uiScale);

            textPosition = XMVectorAdd(textPosition, textDiffInY);
            swprintf_s(strText, _countof(strText) - 1, L"2X1 Rate == Cyan");
            m_fontText->DrawString(m_fontBatch.get(), strText, textPosition, textColor, 0, g_XMZero, c_uiScale);

            textPosition = XMVectorAdd(textPosition, textDiffInY);
            swprintf_s(strText, _countof(strText) - 1, L"2X2 Rate == Green");
            m_fontText->DrawString(m_fontBatch.get(), strText, textPosition, textColor, 0, g_XMZero, c_uiScale);
        }

        const wchar_t* legendStr = L"A: Toggle VRS On/Off   B: Visualize Shading Rate";
        DX::DrawControllerString(m_fontBatch.get(),
            m_fontText.get(), m_ctrlFont.get(),
            legendStr,
            XMFLOAT2(100.f,
                float(safe.bottom) - 20.f -m_fontText->GetLineSpacing()),
            ATG::ColorsHDR::OffWhite, c_uiScale);

        m_fontBatch->End();
    }
    PIXEndEvent(graphicsCmdList);
}

void Sample::DrawCloudSprites(DirectX::SpriteBatch* sprites, ID3D12GraphicsCommandList5* commandList, D3D12_VIEWPORT viewport)
{
    sprites->Begin(commandList);
    for (size_t i = 0; i < m_spriteStates.size(); i++)
    {
        // Cull non-visible sprites
        auto spritePosition = m_spriteStates[i]->Position;
        if (spritePosition.x >= (-1.0f * c_spriteScaleFactor * c_cloudTextureSize.x - c_spriteSpacing) && spritePosition.x <= viewport.Width)
        {
            // Render waves of clouds to cover more pixels with the VRS mask.
            for (uint32_t wave = 0; wave < c_spriteWaves; wave++)
            {
                spritePosition += Vector2(0, 200);
                sprites->Draw(m_SRVUAVDescriptorHeap->GetGpuHandle(Descriptors::CloudSRV), XMUINT2(c_cloudTextureSize.x * c_spriteScaleFactor, c_cloudTextureSize.y * c_spriteScaleFactor),
                    spritePosition);
            }
        }
    }
    sprites->End();
}

// Draws the scene.
void Sample::Render()
{
    static bool s_firstFrame = true;
    GraphicsResource cbPerFrame;
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto cloudAlphaViewport = viewport;
    cloudAlphaViewport.Width /= c_VRSTileSize;
    cloudAlphaViewport.Height /= c_VRSTileSize;

    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    auto commandList = m_deviceResources->GetCommandList();

    ID3D12DescriptorHeap* pHeaps[] = { m_SRVUAVDescriptorHeap->Heap() };
    commandList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);

    Clear();

    m_gpuTimer->BeginFrame(commandList);
    m_gpuTimer->Start(commandList, static_cast<uint32_t>(TimerCounters::TotalFrameTime));   

    if (m_VRSEnabled)
    {
        // Render Cloud Alphas for VRS Shading Rate Image 
        {
            commandList->RSSetViewports(1, &cloudAlphaViewport);
            auto vrsScissorRect = m_deviceResources->GetScissorRect();
            vrsScissorRect.left /= c_VRSTileSize;
            vrsScissorRect.top /= c_VRSTileSize;
            vrsScissorRect.right /= c_VRSTileSize;
            vrsScissorRect.bottom /= c_VRSTileSize;
            commandList->RSSetScissorRects(1, &vrsScissorRect);

            Constants cb = {};
            cb.shadingRateImageWidth = m_shadingRateImageWidth;
            cb.shadingRateImageHeight = m_shadingRateImageHeight;
            cbPerFrame = m_graphicsMemory->AllocateConstant(cb);

            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render Cloud Alphas to Smaller Buffer");
            // Disable shading rate image usage to begin generating values for the next frame
            D3D12_SHADING_RATE_COMBINER combiners[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT] =
            {
                D3D12_SHADING_RATE_COMBINER_PASSTHROUGH,    // pass thorough is the render state
                D3D12_SHADING_RATE_COMBINER_PASSTHROUGH     // take the result of the previous combiner
            };

            m_commandList5->RSSetShadingRate(D3D12_SHADING_RATE_1X1, combiners);
            m_commandList5->RSSetShadingRateImage(nullptr);
         
            DrawCloudSprites(m_alphaCloudSprites.get(), commandList, viewport);

            m_cloudAlphaScene->EndScene(commandList);
            PIXEndEvent(commandList);
        }

        // Generate VRS Shading Rate Image
        {
            // Initialize shading rate image on first frame, with 1x1
            if (s_firstFrame)
            {
                UINT32 fillValue = (D3D12_SHADING_RATE_1X1 << 24) | (D3D12_SHADING_RATE_1X1 << 16) | (D3D12_SHADING_RATE_1X1 << 8) | D3D12_SHADING_RATE_1X1;
                commandList->FillMemoryWith32BitValueX(m_shadingRateImage->GetGPUVirtualAddress(), m_shadingRateImageSizeBytes, fillValue, D3D12XBOX_COPY_FLAG_NONE);
                s_firstFrame = false;
            }

            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"VRS Calc Shading Rate");
            m_gpuTimer->Start(commandList, static_cast<uint32_t>(TimerCounters::VRSImage));
            commandList->SetComputeRootSignature(m_VRSRS.Get());
            commandList->SetComputeRootDescriptorTable(0, m_SRVUAVDescriptorHeap->GetGpuHandle(ShadingRateImageUAV));
            commandList->SetComputeRootConstantBufferView(1, cbPerFrame.GpuAddress());
            commandList->SetComputeRootDescriptorTable(2, m_SRVUAVDescriptorHeap->GetGpuHandle(CloudAlphaSRV));

            TransitionResource(commandList, m_shadingRateImage.Get(), D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            // Avoid processing 16 pixel border around edge of the screen,
            // since shading differences close to the screen are normally unnoticeable.
            // This allows us to compute less shading rates.
            UINT dispatchX = UINT(m_deviceResources->GetScissorRect().right) - 32;
            UINT dispatchY = UINT(m_deviceResources->GetScissorRect().bottom) - 32;

            commandList->SetPipelineState(m_VRSPSO.Get());
            commandList->Dispatch(dispatchX / c_VRSTileSize, dispatchY / c_VRSTileSize, 1);
            TransitionResource(commandList, m_shadingRateImage.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
            m_gpuTimer->Stop(commandList, static_cast<uint32_t>(TimerCounters::VRSImage));
            PIXEndEvent(commandList);

            // Set Shading Rate Image in D3D12 pipeline to enable VRS during scene rendering
            static const D3D12_SHADING_RATE shadingRates[] =
            {
                D3D12_SHADING_RATE_1X1,
                D3D12_SHADING_RATE_1X2,
                D3D12_SHADING_RATE_2X1,
                D3D12_SHADING_RATE_2X2,
                D3D12_SHADING_RATE_2X4,
                D3D12_SHADING_RATE_4X2,
                D3D12_SHADING_RATE_4X4
            };

            D3D12_SHADING_RATE shadingRate = shadingRates[0];
            D3D12_SHADING_RATE_COMBINER chooseScreenspaceImage[2] = { D3D12_SHADING_RATE_COMBINER_OVERRIDE, D3D12_SHADING_RATE_COMBINER_OVERRIDE }; // Choose shading rate image
            m_commandList5->RSSetShadingRate(shadingRate, chooseScreenspaceImage);
            m_commandList5->RSSetShadingRateImage(m_shadingRateImage.Get());
        }
    }

    // Scene Rendering
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render Scene");
        m_gpuTimer->Start(commandList, static_cast<uint32_t>(TimerCounters::Scene));

        auto const scissorRect = m_deviceResources->GetScissorRect();
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);
        auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
        auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

        // This sample doesn't require a DSV and should really use nullptr instead of dsvDescriptor when calling OMSetRenderTargets
        // But a current driver bug (34719677) will disable VRS if no DSV is bound.
        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);

        // Render triangle
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Triangle with Slow Pixel Shader");
        commandList->SetGraphicsRootSignature(m_triangleRootSignature.Get());
        commandList->SetPipelineState(m_trianglePSO.Get());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &m_triangleVertexBufferView);
        commandList->DrawInstanced(3, 1, 0, 0);
        PIXEndEvent(commandList);

        // Render clouds
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clouds");
        m_cloudSprites->SetViewport(viewport);
        DrawCloudSprites(m_cloudSprites.get(), commandList, viewport);
        m_gpuTimer->Stop(commandList, static_cast<uint32_t>(TimerCounters::Scene));

        PIXEndEvent(commandList);
        PIXEndEvent(commandList);
    }
   
    // Visualize VRS Shading Rates
    if (m_shadingRateVisualizationEnabled && m_VRSEnabled)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Visualize VRS Rates");
        TransitionResource(commandList, m_shadingRateImage.Get(), D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
        auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();
        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);

        m_fullScreenQuad->Draw(commandList, m_VisualizeVRSPSO.Get(), m_SRVUAVDescriptorHeap->GetGpuHandle(Descriptors::CloudSRV), m_SRVUAVDescriptorHeap->GetGpuHandle(Descriptors::ShadingRateImageSRV), cbPerFrame.GpuAddress());
        TransitionResource(commandList, m_shadingRateImage.Get(), D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        PIXEndEvent(commandList);
    }

    // UI
    RenderUI(commandList);

    m_gpuTimer->Stop(commandList, static_cast<uint32_t>(TimerCounters::TotalFrameTime));
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

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Transition the cloud alpha texture into an RTV and clear it when VRS mode is enabled.
    if (m_VRSEnabled)
    {
        m_cloudAlphaScene->BeginScene(commandList);
        auto const cloudAlphaRTVDescriptor = m_rtvDescriptorHeap->GetCpuHandle(RTDescriptors::CloudAlphaRTV);
        commandList->OMSetRenderTargets(1, &cloudAlphaRTVDescriptor, FALSE, &dsvDescriptor);
        m_cloudAlphaScene->Clear(commandList);
    }
    
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
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    InitializeCloudAlphaBuffer();
    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    ResourceUploadBatch resourceUpload(device);

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // UI Geometry Setup
    m_vertexBatch = std::make_unique<PrimitiveBatch<VertexType>>(device);
    const RenderTargetState UIRectRtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    
    EffectPipelineStateDescription UIRectPd(
        &VertexType::InputLayout,
        CommonStates::AlphaBlend,
        CommonStates::DepthDefault,
        CommonStates::CullNone,
        UIRectRtState);

    m_basicEffect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, UIRectPd);

    // Cloud Alpha Render Texture Setup
    m_SRVUAVDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, Descriptors::Count);

    m_rtvDescriptorHeap = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        RTDescriptors::RTCount);

    // Triangle Resources Setup
    {
        // Create root signature.
        auto vertexShaderBlob = DX::ReadData(L"VertexShader.cso");

        // Xbox One best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vertexShaderBlob.data(), vertexShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_triangleRootSignature.ReleaseAndGetAddressOf())));

        // Create the pipeline state, which includes loading shaders.
        auto pixelShaderBlob = DX::ReadData(L"PixelShader.cso");

        static const D3D12_INPUT_ELEMENT_DESC s_inputElementDesc[2] =
        {
            { "SV_Position", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,  0 },
            { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 },
        };

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { s_inputElementDesc, _countof(s_inputElementDesc) };
        psoDesc.pRootSignature = m_triangleRootSignature.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc,
                IID_GRAPHICS_PPV_ARGS(m_trianglePSO.ReleaseAndGetAddressOf())));

        // Create vertex buffer.
        {
            static const TriangleVertex s_vertexData[3] =
            {
                { { 0.0f,   0.5f,  0.5f, 1.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },  // Top / Red
                { { 0.5f,  -0.5f,  0.5f, 1.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },  // Right / Green
                { { -0.5f, -0.5f,  0.5f, 1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } }   // Left / Blue
            };

            // Note: using upload heaps to transfer static data like vert buffers is not 
            // recommended. Every time the GPU needs it, the upload heap will be marshalled 
            // over. Please read up on Default Heap usage. An upload heap is used here for 
            // code simplicity and because there are very few verts to actually transfer.
            const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
            auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(s_vertexData));

            DX::ThrowIfFailed(
                device->CreateCommittedResource(&heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &resDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_triangleVertexBuffer.ReleaseAndGetAddressOf())));

            // Copy the triangle data to the vertex buffer.
            UINT8* pVertexDataBegin = nullptr;
            CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
            DX::ThrowIfFailed(
                m_triangleVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
            memcpy(pVertexDataBegin, s_vertexData, sizeof(s_vertexData));
            m_triangleVertexBuffer->Unmap(0, nullptr);

            // Initialize the vertex buffer view.
            m_triangleVertexBufferView.BufferLocation = m_triangleVertexBuffer->GetGPUVirtualAddress();
            m_triangleVertexBufferView.StrideInBytes = sizeof(TriangleVertex);
            m_triangleVertexBufferView.SizeInBytes = sizeof(s_vertexData);
        }
    }

    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());

    auto const backBufferWidth = m_deviceResources->GetScreenViewport().Width;
    auto const backBufferHeight = m_deviceResources->GetScreenViewport().Height;

    // Upload Cloud Textures and Fonts
    {
        resourceUpload.Begin();
        InitializeSpriteFonts(device, resourceUpload, rtState);

        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, resourceUpload, L"Assets/Cloud/cloud.png", m_cloudTexture.ReleaseAndGetAddressOf(), true)
        );
        CreateShaderResourceView(device, m_cloudTexture.Get(), m_SRVUAVDescriptorHeap->GetCpuHandle(Descriptors::CloudSRV));

        // Create Resource for Cloud Alpha Texture
        {
            auto pixelShaderBlob = DX::ReadData(L"RenderAlphaToVRS.cso");

            m_cloudAlphaScene->SetDevice(device,
                m_SRVUAVDescriptorHeap->GetCpuHandle(Descriptors::CloudAlphaSRV),
                m_rtvDescriptorHeap->GetCpuHandle(RTDescriptors::CloudAlphaRTV));

            auto const size = m_deviceResources->GetOutputSize();
            auto VRSSize = size;
            auto tileSizeLong = (LONG)c_VRSTileSize;
            VRSSize.right = size.right / tileSizeLong;
            VRSSize.left = size.left / tileSizeLong;
            VRSSize.top = size.top / tileSizeLong;
            VRSSize.bottom = size.bottom / tileSizeLong;
            m_cloudAlphaScene->SetWindow(VRSSize);

            const RenderTargetState CloudAlphaRTState(DXGI_FORMAT_R8_UNORM, m_deviceResources->GetDepthBufferFormat());
            SpriteBatchPipelineStateDescription pd(CloudAlphaRTState);

            pd.customPixelShader.BytecodeLength = pixelShaderBlob.size();
            pd.customPixelShader.pShaderBytecode = pixelShaderBlob.data();
            m_alphaCloudSprites = std::make_unique<SpriteBatch>(device, resourceUpload, pd);
        }

        // RGBA Cloud Sprite Resources
        {
            SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::NonPremultiplied);
            m_cloudSprites = std::make_unique<SpriteBatch>(device, resourceUpload, pd);
        }

        auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        uploadResourcesFinished.wait();

        // Cloud Sprite State Initialization
        {
            SpriteState clouds[c_cloudsPerWave];
            auto xOffset = -1.0f * backBufferWidth;
            c_cloudTextureSize = GetTextureSize(m_cloudTexture.Get());

            for (uint32_t i = 0; i < c_cloudsPerWave; i++)
            {
                clouds[i].Reset(backBufferWidth, backBufferHeight, xOffset);
                m_spriteStates.push_back(std::make_shared<SpriteState>(std::move(clouds[i])));
                xOffset += c_cloudTextureSize.x + c_spriteSpacing;
            }
        }   
    }

    auto csBlob = DX::ReadData(L"CalculateVRS.cso");
    // Create an empty root signature.
    {
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;

        DX::ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        DX::ThrowIfFailed(device->CreateRootSignature(0, csBlob.data(), csBlob.size(), IID_GRAPHICS_PPV_ARGS(m_VRSRS.ReleaseAndGetAddressOf())));
        m_VRSRS->SetName(L"m_VRSRS");
    }

    // Create the pipeline state for populating the VRS shading image
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_VRSRS.Get();
        psoDesc.CS.pShaderBytecode = csBlob.data();
        psoDesc.CS.BytecodeLength = csBlob.size();

        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_VRSPSO.ReleaseAndGetAddressOf())));
        m_VRSPSO->SetName(L"CalculateVRS.cso");
    }

    // Transparent Shading Rate Visualization Resource Setup
    {
        m_fullScreenQuad = std::make_unique<DX::FullScreenQuad>();
        m_fullScreenQuad->Initialize(device);
        auto psBlob = DX::ReadData(L"VisualizeShadingRatesPS.cso");
        auto vsBlob = DX::ReadData(L"FullScreenQuadVS.cso");

        const RenderTargetState VRSRtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

        EffectPipelineStateDescription pd(nullptr,
            CommonStates::AlphaBlend,
            CommonStates::DepthNone,
            CommonStates::CullCounterClockwise,
            VRSRtState);
        
        D3D12_SHADER_BYTECODE vs = { vsBlob.data(), vsBlob.size() };
        D3D12_SHADER_BYTECODE ps = { psBlob.data(), psBlob.size() };
        pd.CreatePipelineState(device, m_fullScreenQuad->GetRootSignature(), vs, ps, m_VisualizeVRSPSO.ReleaseAndGetAddressOf());
        m_VisualizeVRSPSO->SetName(L"m_VisualizeVRSPSO");
        
    }

    // Create the command list that supports VRS.
    ComPtr<ID3D12GraphicsCommandList> commandList = m_deviceResources->GetCommandList();
    commandList.As(&m_commandList5);

    m_deviceResources->WaitForGpu();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const viewport = m_deviceResources->GetScreenViewport();
    m_cloudSprites->SetViewport(viewport);
    m_alphaCloudSprites->SetViewport(viewport);
    switch (static_cast<int>(viewport.Height))
    {
    case 1080:
        c_uiScale = 1.f;
        break;
    case 1440:
        c_uiScale = 1.333333f;
        break;
    default: /*4K*/
        c_uiScale = 2.f;
        break;
    }
}
#pragma endregion
