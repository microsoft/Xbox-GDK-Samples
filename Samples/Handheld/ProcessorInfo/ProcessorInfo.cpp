//--------------------------------------------------------------------------------------
// ProcessorInfo.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ProcessorInfo.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h"
#include <algorithm>

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_displayPage(Page::generalInfoPage)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN, 2);
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
    ATG::SetupProcessorData();
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
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    m_mouse->EndOfInputFrame();

    Render();
}

// Updates the world.
void Sample::Update(DX::StepTimer const& /*timer*/)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    //float elapsedTime = float(timer.GetElapsedSeconds());
    //elapsedTime;

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
            ExitSample();
        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
            ExitSample();
        if ((m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED) || (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED))
            IncrementPage();
        if ((m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED) || (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED))
            DecrementPage();
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
    if (m_keyboardButtons.IsKeyPressed(Keyboard::Right) || m_keyboardButtons.IsKeyPressed(Keyboard::Up))
        IncrementPage();
    if (m_keyboardButtons.IsKeyPressed(Keyboard::Left) || m_keyboardButtons.IsKeyPressed(Keyboard::Down))
        DecrementPage();

    //auto mouse = m_mouse->GetState();
    //mouse;

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
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), GetTextureSize(m_background.Get()), XMFLOAT2(0, 0));

    if (m_timer.GetFrameCount() > 3)
    {
        switch (m_displayPage)
        {
        case Page::generalInfoPage:
            DisplayGeneralInfoPage();
            break;
        case Page::efficiencyPage:
            DisplayEfficiencyPage();
            break;
        case Page::cachePage:
            DisplayCachePage();
            break;
        case Page::powerPage:
            DisplayPowerPage();
            break;
        default:
            assert(false);
        }
    }
    {
        int32_t winWidth, winHeight;
        GetCurrentSize(winWidth, winHeight);
        RECT safeRect = SimpleMath::Viewport::ComputeTitleSafeArea(static_cast<uint32_t> (winWidth), static_cast<uint32_t> (winHeight));
        XMFLOAT2 pos(0, 0);

        auto bounds = DX::MeasureControllerDrawBounds(m_regularFont.get(), m_ctrlFont.get(), L"[DPad] - Switch Pages", pos);
        pos.y = safeRect.bottom - bounds.bottom - m_regularFont->GetLineSpacing();
        pos.x = (float)safeRect.left;
        DX::DrawControllerString(m_spriteBatch.get(), m_regularFont.get(), m_ctrlFont.get(), L"[DPad] - Switch Pages", pos);
    }
    m_spriteBatch->End();
    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::DisplayGeneralInfoPage()
{
    wchar_t buffer[256];
    int32_t winWidth, winHeight;
    GetCurrentSize(winWidth, winHeight);
    RECT safeRect = SimpleMath::Viewport::ComputeTitleSafeArea(static_cast<uint32_t> (winWidth), static_cast<uint32_t> (winHeight));
    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    m_largeFont->DrawString(m_spriteBatch.get(), L"General Processor Information", pos);
    pos.y += (m_largeFont->GetLineSpacing() * 2);
    pos.x += XMVectorGetX(m_regularFont->MeasureString(L"XXX"));

    swprintf(buffer, 256, L"Name: %s", ATG::GetTrueProcessorName().c_str());
    m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
    pos.y += m_regularFont->GetLineSpacing();

    if (ATG::IsSMTSupported())
        swprintf(buffer, 256, L"Core Count: %d physical, %d logical", ATG::GetNumberPhysicalCores(), ATG::GetNumberLogicalCores());
    else
        swprintf(buffer, 256, L"Core Count: %d physical", ATG::GetNumberPhysicalCores());
    m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
    pos.y += m_regularFont->GetLineSpacing();

    swprintf(buffer, 256, L"RDTSCP frequency: %10.3f ticks per millisecond", ATG::GetRDTSCPFrequencyMilliseconds());
    m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
    pos.y += m_regularFont->GetLineSpacing();

    if (ATG::GetNumberEfficiencyClasses() > 1)
    {
        swprintf(buffer, 256, L"%d efficiency classes", ATG::GetNumberEfficiencyClasses());
        m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
        pos.y += m_regularFont->GetLineSpacing();
    }

    bool useTopLevelCaches = true;
    uint32_t numClusters = 1;
    {
        // using either the number of top level caches or efficiency classes to determine the number of groups/clusters.
        // this is conceptually the number of clusters/groupings that a process would want to use for splitting out threads
        if (ATG::GetNumberTopLevelCacheSets() >= ATG::GetNumberEfficiencyClasses())
        {
            swprintf(buffer, 256, L"%d cluster%s based on number of L%d cache%s", ATG::GetNumberTopLevelCacheSets(), ATG::GetNumberTopLevelCacheSets() > 1 ? L"s" : L"", ATG::GetTopLevelCacheIndex(), ATG::GetNumberTopLevelCacheSets() > 1 ? L"s" : L"");
            numClusters = ATG::GetNumberTopLevelCacheSets();
            useTopLevelCaches = true;
        }
        else
        {
            swprintf(buffer, 256, L"%d cluster%s based on efficiency classes", ATG::GetNumberEfficiencyClasses(), ATG::GetNumberTopLevelCacheSets() > 1 ? L"s" : L"");
            numClusters = ATG::GetNumberEfficiencyClasses();
            useTopLevelCaches = false;
        }

        m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
        pos.y += m_regularFont->GetLineSpacing();
    }

    for (uint32_t curCluster = 0; curCluster < numClusters; curCluster++)
    {
        uint64_t clusterMask = ATG::GetTopLevelCacheCoreMask(curCluster);
        if (!useTopLevelCaches)
            clusterMask = ATG::GetEfficiencyClassMask(curCluster);

        auto frequency = ATG::GetProcessorBaseFrequencyMHz(clusterMask);

        swprintf(buffer, 256, L"Cluster: %d     Base Freq: %dMHz     Mask: 0x%s", curCluster, frequency, ATG::GetProcessorMaskString(clusterMask, 64).c_str());
        m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
        pos.y += m_regularFont->GetLineSpacing();
    }
}

void Sample::DisplayEfficiencyPage()
{
    //wchar_t buffer[256];
    int32_t winWidth, winHeight;
    GetCurrentSize(winWidth, winHeight);
    RECT safeRect = SimpleMath::Viewport::ComputeTitleSafeArea(static_cast<uint32_t> (winWidth), static_cast<uint32_t> (winHeight));
    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    m_largeFont->DrawString(m_spriteBatch.get(), L"Efficiency Information", pos);
    pos.y += (m_largeFont->GetLineSpacing() * 2);
    pos.x += XMVectorGetX(m_regularFont->MeasureString(L"XXX"));
}

void Sample::DisplayCachePage()
{
    wchar_t buffer[256];
    int32_t winWidth, winHeight;
    GetCurrentSize(winWidth, winHeight);
    RECT safeRect = SimpleMath::Viewport::ComputeTitleSafeArea(static_cast<uint32_t> (winWidth), static_cast<uint32_t> (winHeight));
    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    m_largeFont->DrawString(m_spriteBatch.get(), L"Cache Information", pos);
    pos.y += (m_largeFont->GetLineSpacing() * 2);
    pos.x += XMVectorGetX(m_regularFont->MeasureString(L"XXX"));

    // single core cache info
    {
        m_regularFont->DrawString(m_spriteBatch.get(), L"Single Core", pos);
        pos.y += m_regularFont->GetLineSpacing();

        auto topLevelIndex = ATG::GetTopLevelCacheIndex();

        auto oldPosX = pos.x;
        pos.x += XMVectorGetX(m_regularFont->MeasureString(L"XXX"));
        uint32_t numClusters = 1;
        bool useTopLevelCaches = true;
        if (ATG::GetNumberTopLevelCacheSets() < ATG::GetNumberEfficiencyClasses())
        {
            useTopLevelCaches = false;
            numClusters = ATG::GetNumberEfficiencyClasses();
        }
        else
        {
            numClusters = ATG::GetNumberTopLevelCacheSets();
        }

        for (uint32_t curCluster = 0; curCluster < numClusters; curCluster++)
        {
            auto dividerX = winWidth - (pos.x * 2);
            dividerX /= std::min(numClusters, 4u);
            auto baseX = pos.x + (dividerX * curCluster);
            auto baseY = pos.y;

            uint64_t clusterMask = ATG::GetTopLevelCacheCoreMask(curCluster);
            if (!useTopLevelCaches)
                clusterMask = ATG::GetEfficiencyClassMask(curCluster);

            std::vector<ATG::CacheInformation> cacheInfo;
            DWORD highBitIndex;
            _BitScanForward64(&highBitIndex, clusterMask);
            ATG::GetCacheInformation(1ULL << highBitIndex, cacheInfo);

            if (numClusters > 1)
            {
                swprintf(buffer, 256, L"Cluster %d", curCluster);
                m_regularFont->DrawString(m_spriteBatch.get(), buffer, XMFLOAT2(baseX, baseY));
                baseY += m_regularFont->GetLineSpacing();

                swprintf(buffer, 256, L"Mask: 0x%s", ATG::GetProcessorMaskString(clusterMask, ATG::GetNumberLogicalCores()).c_str());
                m_regularFont->DrawString(m_spriteBatch.get(), buffer, XMFLOAT2(baseX, baseY));
                baseY += m_regularFont->GetLineSpacing();
            }

            for (uint32_t curLevel = 0; curLevel < topLevelIndex; curLevel++)
            {
                for (auto& iter : cacheInfo)
                {
                    if (iter.level == curLevel)
                    {
                        switch (iter.cacheType)
                        {
                        case ATG::CacheType::CacheData:
                            swprintf(buffer, 256, L"L%dD: %s", curLevel, ATG::GetMemorySizeString(iter.cacheSize).c_str());
                            break;
                        case ATG::CacheType::CacheInstruction:
                            swprintf(buffer, 256, L"L%dI: %s", curLevel, ATG::GetMemorySizeString(iter.cacheSize).c_str());
                            break;
                        case ATG::CacheType::CacheUnified:
                            swprintf(buffer, 256, L"L%d: %s", curLevel, ATG::GetMemorySizeString(iter.cacheSize).c_str());
                            break;
                        case ATG::CacheType::CacheTrace:
                            swprintf(buffer, 256, L"L%dT: %s", curLevel, ATG::GetMemorySizeString(iter.cacheSize).c_str());
                            break;
                        }
                        m_regularFont->DrawString(m_spriteBatch.get(), buffer, XMFLOAT2(baseX, baseY));
                        baseY += m_regularFont->GetLineSpacing();
                    }
                }
            }
        }
        pos.x = oldPosX;
        pos.y += (m_regularFont->GetLineSpacing() * ((numClusters > 1) ? 5 : 3));
    }

    // top level cache info
    {
        pos.y += m_largeFont->GetLineSpacing();
        swprintf(buffer, 256, L"%d L%d cache%s", ATG::GetNumberTopLevelCacheSets(), ATG::GetTopLevelCacheIndex(), ATG::GetNumberTopLevelCacheSets() > 1 ? L"s" : L"");
        m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
        pos.y += m_regularFont->GetLineSpacing();
        auto oldPosX = pos.x;
        pos.x += XMVectorGetX(m_regularFont->MeasureString(L"XXX"));
        for (uint32_t i = 0; i < ATG::GetNumberTopLevelCacheSets(); i++)
        {
            std::vector < ATG::CacheInformation> cacheInfo;
            ATG::GetCacheInformation(ATG::GetTopLevelCacheCoreMask(i), cacheInfo);
            uint32_t numTopLevel = 0;
            uint32_t foundTopLevel = 0;
            uint32_t topLevelSize = 0;
            for (const auto& iter : cacheInfo)
            {
                if (iter.cacheType != ATG::CacheType::CacheUnified)
                    continue;
                if (numTopLevel < iter.level)
                {
                    numTopLevel = iter.level;
                    topLevelSize = 0;
                    foundTopLevel = 0;
                }
                if (numTopLevel == iter.level)
                {
                    foundTopLevel++;
                    topLevelSize += iter.cacheSize;
                }
            }
            assert(foundTopLevel == 1);

            swprintf(buffer, 256, L"%d-L%d: Size: %s  Mask: 0x%s", i, numTopLevel, ATG::GetMemorySizeString(topLevelSize).c_str(), ATG::GetProcessorMaskString(ATG::GetTopLevelCacheCoreMask(i), ATG::GetNumberLogicalCores()).c_str());
            m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
            pos.y += m_regularFont->GetLineSpacing();
        }
        pos.x = oldPosX;
    }
}

void Sample::DisplayPowerPage()
{
    //wchar_t buffer[256];
    int32_t winWidth, winHeight;
    GetCurrentSize(winWidth, winHeight);
    RECT safeRect = SimpleMath::Viewport::ComputeTitleSafeArea(static_cast<uint32_t> (winWidth), static_cast<uint32_t> (winHeight));
    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    m_largeFont->DrawString(m_spriteBatch.get(), L"Power Information", pos);
    pos.y += (m_largeFont->GetLineSpacing() * 2);
    pos.x += XMVectorGetX(m_regularFont->MeasureString(L"XXX"));
}

void Sample::IncrementPage()
{
    switch (m_displayPage)
    {
    case Page::generalInfoPage:
        if (ATG::GetNumberEfficiencyClasses() > 1)
            m_displayPage = Page::efficiencyPage;
        else
            m_displayPage = Page::cachePage;
        break;
    case Page::efficiencyPage:
        m_displayPage = Page::cachePage;
        break;
    case Page::cachePage:
        m_displayPage = Page::powerPage;
        break;
    case Page::powerPage:
        m_displayPage = Page::generalInfoPage;
        break;
    default:
        assert(false);
        m_displayPage = Page::generalInfoPage;
    }
}

void Sample::DecrementPage()
{
    switch (m_displayPage)
    {
    case Page::generalInfoPage:
        m_displayPage = Page::powerPage;
        break;
    case Page::efficiencyPage:
        m_displayPage = Page::generalInfoPage;
        break;
    case Page::cachePage:
        if (ATG::GetNumberEfficiencyClasses() > 1)
            m_displayPage = Page::efficiencyPage;
        else
            m_displayPage = Page::generalInfoPage;
        break;
    case Page::powerPage:
        m_displayPage = Page::cachePage;
        break;
    default:
        assert(false);
        m_displayPage = Page::generalInfoPage;
    }
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    const auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    const auto viewport = m_deviceResources->GetScreenViewport();
    const auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
}

void Sample::OnResuming()
{
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
}

void Sample::OnWindowMoved()
{
    const auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

void Sample::GetCurrentSize(int32_t& width, int32_t& height) const noexcept
{
    auto rect = m_deviceResources->GetOutputSize();
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
}

// Properties
void Sample::GetDefaultSize(int32_t& width, int32_t& height) const noexcept
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

    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    wchar_t strFilePath[MAX_PATH] = {};

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        Descriptors::Count);

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    DX::FindMediaFile(strFilePath, MAX_PATH, L"ATGSampleBackground.dds");
    DX::ThrowIfFailed(
        CreateDDSTextureFromFileEx(device, resourceUpload,
            strFilePath,
            0, D3D12_RESOURCE_FLAG_NONE, DDS_LOADER_FORCE_SRGB,
            m_background.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pd(rtState);

    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    m_regularFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::RegularFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::RegularFont));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_24.spritefont");
    m_largeFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::LargeFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::LargeFont));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneController.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::CtrlFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::CtrlFont));

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());

    uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto viewport = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewport);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
