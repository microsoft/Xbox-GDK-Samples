//--------------------------------------------------------------------------------------
// OSPrimitiveTestCombo.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "OSPrimitiveTestCombo.h"
#include "PerfRun.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "Processor.h"
#include "CommandLine.h"

extern void ExitSample();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

void Sample::PerformTests()
{
    ATG::SetupProcessorData();
    std::vector<uint64_t> coreMasks;
    std::vector<std::wstring> coreTestNames;
#ifdef _GAMING_DESKTOP
    if (ATG::GetNumLogicalCores() < 4)
    {
        m_finishedTestRun = true;
        return;
    }

    if (ATG::IsHyperThreaded())
    {
        coreMasks.push_back(0x01);
        coreMasks.push_back(0x03);
        coreMasks.push_back(0x11);
        coreTestNames.push_back(L"Single Core");
        coreTestNames.push_back(L"Same Physical");
        coreTestNames.push_back(L"Separate Physical");
        if (ATG::GetNumberTopLevelCacheSets() > 1)
        {
            uint64_t coreMask(0);
            for (uint32_t i = 0; i < ATG::GetNumberTopLevelCacheSets(); i++)
            {
                uint64_t temp = ATG::GetTopLevelCacheCoreMask(i);
                DWORD index(0);
                _BitScanForward64(&index, temp);
                coreMask |= 1ULL << index;
            }
            coreMasks.push_back(coreMask);
            coreTestNames.push_back(L"Cross Clusters");
        }
    }
    else
    {
        coreMasks.push_back(0x01);
        coreMasks.push_back(0x03);
        coreTestNames.push_back(L"Single Core");
        coreTestNames.push_back(L"Seperate Physical");
        if (ATG::GetNumberTopLevelCacheSets() > 1)
        {
            uint64_t coreMask(0);
            for (uint32_t i = 0; i < ATG::GetNumberTopLevelCacheSets(); i++)
            {
                uint64_t temp = ATG::GetTopLevelCacheCoreMask(i);
                DWORD index(0);
                _BitScanForward64(&index, temp);
                coreMask |= 1ULL << index;
            }
            coreMasks.push_back(coreMask);
            coreTestNames.push_back(L"Cross Clusters");
        }
    }
#else
    if (ATG::IsHyperThreaded())
    {
        coreMasks.push_back(0x01);
        coreMasks.push_back(0x03);
        coreMasks.push_back(0x11);
        coreMasks.push_back(0x101);
        coreTestNames.push_back(L"Single Core");
        coreTestNames.push_back(L"Same physical");
        coreTestNames.push_back(L"Same Cluster");
        coreTestNames.push_back(L"Cross Cluster");
    }
    else
    {
        coreMasks.push_back(0x01);
        coreMasks.push_back(0x03);
        coreMasks.push_back(0x11);
        coreTestNames.push_back(L"Single Core");
        coreTestNames.push_back(L"Same Cluster");
        coreTestNames.push_back(L"Cross Cluster");
    }
#endif

    {
        PerfRun* baseRun;
        baseRun = new PerfRun();
        m_runWorking = UINT32_MAX;

        for (uint32_t testRun = PerfRun::TestType::FirstTestExecuted; (testRun <= PerfRun::TestType::LastTestExecuted) && (!m_shutdownThread); testRun++)
        {
            m_runWorking = testRun;
            uint32_t coreMaskIndex = 0;
            for (const auto& iter : coreMasks)
            {
                DWORD firstBit, secondBit;
                _BitScanForward64(&firstBit, iter);
                _BitScanReverse64(&secondBit, iter);				// could be the same bit as firstBit which means no contention case
                m_firstProcessorCore = firstBit;
                m_secondProcessorCore = secondBit;
                baseRun->RunTests(firstBit, secondBit, static_cast<PerfRun::TestType> (testRun), true, coreTestNames[coreMaskIndex], coreMaskIndex);
                if (m_shutdownThread)
                    break;
                coreMaskIndex++;
            }
        }
        delete baseRun;
    }
    m_finishedTestRun = true;
}

Sample::Sample() noexcept(false) :
    m_shutdownThread(false)
    , m_finishedTestRun(false)
    , m_runWorking(UINT32_MAX)
    , m_firstProcessorCore(0)
    , m_secondProcessorCore(0)
    , m_frame(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    if (m_workerThread)
    {
        m_shutdownThread = true;
        if (!m_finishedTestRun)
            TerminateThread(m_workerThread->native_handle(), 0);
        m_workerThread->join();
        delete m_workerThread;
    }

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
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    if (timer.GetFrameCount() == 3)
    {
        m_runWorking = UINT32_MAX;
        m_workerThread = new std::thread(&Sample::PerformTests, this);
    }

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
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

    RECT safeRect = SimpleMath::Viewport::ComputeTitleSafeArea(1920, 1080);

    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    ID3D12DescriptorHeap* pHeaps[] = { m_resourceDescriptors->Heap() };
    commandList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), XMFLOAT2(0, 0));

    if (m_timer.GetFrameCount() > 3)
    {
        uint32_t numDots = (m_timer.GetFrameCount() % 10) + 1;
        std::wstring outputString;
        if (m_finishedTestRun)
        {
            outputString = L"Finished Test Run";
        }
        else
        {
            if (m_shutdownThread)
            {
                outputString = L"Shutting Down";
            }
            else if (m_runWorking == UINT32_MAX)
            {
                outputString = L"Waiting for test to start";
            }
            else
            {
                outputString = L"Doing ";
                outputString += PerfRun::ConvertTestTypeToString(static_cast<PerfRun::TestType> (m_runWorking.load()));
                wchar_t buffer[64];
                if (m_firstProcessorCore.load() == m_secondProcessorCore.load())
                {
                    outputString += L": no contention";
                    swprintf_s(buffer, 64, L" - core %d", m_firstProcessorCore.load());
                }
                else
                {
                    outputString += L": contention";
                    swprintf_s(buffer, 64, L" - cores %d, %d", m_firstProcessorCore.load(), m_secondProcessorCore.load());
                }
                outputString += buffer;
            }
        }

        for (uint32_t i = 0; i < numDots; i++)
        {
            outputString += L".";
        }
        m_regularFont->DrawString(m_spriteBatch.get(), outputString.c_str(), pos);
    }

    m_spriteBatch->End();
    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(m_deviceResources->GetCommandQueue(), PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent(m_deviceResources->GetCommandQueue());
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

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
void Sample::OnActivated()
{
}

void Sample::OnDeactivated()
{
}

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
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

// Properties
void Sample::GetDefaultSize(int& width, int& height) const
{
    width = 1280;
    height = 720;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    wchar_t strFilePath[MAX_PATH] = {};

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    DX::FindMediaFile(strFilePath, MAX_PATH, L"ATGSampleBackground.dds");
    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, resourceUpload,
            strFilePath,
            m_background.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pd(rtState);

    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
        m_regularFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_resourceDescriptors->GetCpuHandle(Descriptors::RegularFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::RegularFont));
    }

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
