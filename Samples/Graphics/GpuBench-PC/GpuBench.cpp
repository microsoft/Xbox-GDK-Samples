//--------------------------------------------------------------------------------------
// GpuBench.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GpuBench.h"

#include "Benchmark.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "ControllerFont.h"

// Setup Agility SDK exports: https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_scrollY(0.0f),
    m_versionCheck(true),
    m_logFileName(L"d:\\GpuBench.txt"), 
    m_logFile(nullptr),
    m_runningBenchmarkIndex(m_invalidBenchmarkIndex), 
    m_selectedBenchmarkIndex(0) 
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

void Sample::ParseCommandLine(const wchar_t* commandLine)
{
    bool captureToPls = false;

    wchar_t commandLineCopy[1024] = L"";
    wcscpy_s(commandLineCopy, _countof(commandLineCopy), commandLine);
    wchar_t* context = commandLineCopy;
    while (auto token = wcstok_s(context, L" \t", &context))
    {
        if (0 == _wcsicmp(L"noversioncheck", token))
        {
            m_versionCheck = false;
        }
        else if (0 == _wcsicmp(L"nocapture", token))
        {
            Capture::SetEnabled(false);
        }
        else if (0 == _wcsicmp(L"pls", token))
        {
            captureToPls = true;
        }
        else if (0 == _wcsicmp(L"dumpcounters", token))
        {
            Test::m_dumpCounters = true;
        }
        else if (0 == _wcsicmp(L"all", token))
        {
            for (auto benchmarkIndex = 0U; benchmarkIndex < Benchmark::BenchmarkList().size(); ++benchmarkIndex)
            {
                m_pendingBenchmarkIndices.push(benchmarkIndex);
            }
        }
        else
        {
            for (auto benchmarkIndex = 0U; benchmarkIndex < Benchmark::BenchmarkList().size(); ++benchmarkIndex)
            {
                if (0 == _wcsicmp(Benchmark::BenchmarkList()[benchmarkIndex]->GetName(), token))
                {
                    m_pendingBenchmarkIndices.push(benchmarkIndex);
                }
            }
        }
    }

    if (captureToPls)
    {
        // Captures are saved to Persistent Local Storage to support running benchmarks
        // when Developer Scratch is unavailable.
        HRESULT hr = S_OK;
        size_t pathSize = 0ULL;
        hr = XPersistentLocalStorageGetPathSize(&pathSize);
        if (FAILED(hr))
        {
            throw std::exception("Failure in creating Persistent Local Storage.\n");
        }
        auto plsPath = new char[pathSize];
        hr = XPersistentLocalStorageGetPath(pathSize, plsPath, &pathSize);
        if (FAILED(hr))
        {
            throw std::exception("Failure in creating Persistent Local Storage.\n");
        }
        auto wplsPath = new wchar_t[pathSize];
        if (0 != mbstowcs_s(nullptr, wplsPath, pathSize, plsPath, pathSize))
        {
            throw std::exception("Attempt to convert PLS string failed");
        }
        delete[] plsPath;

        Capture::SetRoot(wplsPath);

        delete[] wplsPath;
    }
    else
    {
        // Captures are saved to developer scratch
        Capture::SetRoot(L"d:\\");
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
#ifdef _GAMING_XBOX
    // Generate chicken bit file to allow GPU counters to work with retail driver.
    // This must be done before the driver is created.
    // This won't have any effect in retail *mode*, just in developer mode with the retail driver.
    // In retail mode, you can't use GPU counters.
    {
        FILE* file;
        if (0 != _wfopen_s(&file, L"d:\\D3D__RetailGpuCounters", L"wt") || nullptr == file)
        {
            throw std::exception("Could not open log file");
        }
        if (EOF == fclose(file))
        {
            throw std::exception("Could not close log file");
        }
    }

	constexpr auto compileVersion = uint32_t(_GXDK_VER);

	{
		std::wostringstream message;
		message << L"Detected compiled GXDK version ";
		message << ((compileVersion & 0xffff0000) >> 16);
		message << L".";
		message << ((compileVersion & 0x0000ffff) >> 0);
		message << L" (";
		message << _GXDK_VER_STRING_W;
		message << L")";
		message << std::endl;
		OutputDebugString(message.str().c_str());
	}

	XSystemAnalyticsInfo systemAnalyticsInfo = XSystemGetAnalyticsInfo();
	XVersion osVersion = systemAnalyticsInfo.osVersion;

	{
		std::wostringstream message;
		message << L"Detected runtime Game OS version ";
		message << (osVersion.build);
		message << L".";
		message << (osVersion.revision);
		message << std::endl;
		OutputDebugString(message.str().c_str());
	}

	auto gameOsVersion = (uint32_t(osVersion.build) << 16) | uint32_t(osVersion.revision);
	if (compileVersion != gameOsVersion)
	{
        if (m_versionCheck)
        {
            throw std::exception("Game OS version mismatch with compiled GXDK version");
        }
        else
        {
            OutputDebugString(L"Game OS version mismatch with compiled GXDK version.\n");
        }
	}

	XVersion hostingOsVersion = systemAnalyticsInfo.hostingOsVersion;

	{
		std::wostringstream message;
		message << L"Detected recovery version ";
		message << (hostingOsVersion.build);
		message << L".";
		message << (hostingOsVersion.revision);
		message << std::endl;
		OutputDebugString(message.str().c_str());
	}

	auto recoveryVersion = (uint32_t(hostingOsVersion.build) << 16) | uint32_t(hostingOsVersion.revision);
	if (gameOsVersion > recoveryVersion && m_versionCheck)
	{
        if (m_versionCheck)
        {
            throw std::exception("Game OS version is newer than recovery version");
        }
        else
        {
            OutputDebugString(L"Game OS version is newer than recovery version.\n");
        }
	}
#endif // #ifdef _GAMING_XBOX

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
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

#ifdef _GAMING_XBOX
    m_deviceResources->WaitForOrigin();
#endif

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& /*timer*/)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    if (nullptr == RunningBenchmark())
    {
        auto kb = m_keyboard->GetState();
        m_keyboardButtons.Update(kb);
        if (m_keyboardButtons.IsKeyPressed(Keyboard::Keys::Escape))
        {
            ExitSample();
        }
        if (m_keyboardButtons.IsKeyPressed(Keyboard::Keys::Down))
        {
            m_selectedBenchmarkIndex = ++m_selectedBenchmarkIndex % Benchmark::BenchmarkList().size();
            m_scrollY = 0.0f;
        }

        if (m_keyboardButtons.IsKeyPressed(Keyboard::Keys::Up))
        {
            m_selectedBenchmarkIndex = (m_selectedBenchmarkIndex + uint32_t(Benchmark::BenchmarkList().size()) - 1) % Benchmark::BenchmarkList().size();
            m_scrollY = 0.0f;
        }

        if (m_keyboardButtons.IsKeyPressed(Keyboard::Keys::A))
        {
            m_pendingBenchmarkIndices.push(m_selectedBenchmarkIndex);
        }

        if (m_keyboardButtons.IsKeyPressed(Keyboard::Keys::X))
        {
            for (auto benchmarkIndex = 0U; benchmarkIndex < Benchmark::BenchmarkList().size(); ++benchmarkIndex)
            {
                m_pendingBenchmarkIndices.push(benchmarkIndex);
            }
        }

        for (int player = 0; player < GamePad::MAX_PLAYER_COUNT; ++player)
        {
            auto pad = m_gamePad->GetState(player);
            if (pad.IsConnected())
            {
                auto& gamePadButtons = m_gamePadButtons[player];

                gamePadButtons.Update(pad);

                if (pad.IsViewPressed())
                {
                    ExitSample();
                }

                if (gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_selectedBenchmarkIndex = ++m_selectedBenchmarkIndex % Benchmark::BenchmarkList().size();
                    m_scrollY = 0.0f;
                }

                if (gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_selectedBenchmarkIndex = (m_selectedBenchmarkIndex + uint32_t(Benchmark::BenchmarkList().size()) - 1) % Benchmark::BenchmarkList().size();
                    m_scrollY = 0.0f;
                }

                if (gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
                {
                    m_pendingBenchmarkIndices.push(m_selectedBenchmarkIndex);
                }

                if (gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
                {
                    for (auto benchmarkIndex = 0U; benchmarkIndex < Benchmark::BenchmarkList().size(); ++benchmarkIndex)
                    {
                        m_pendingBenchmarkIndices.push(benchmarkIndex);
                    }
                }

                if (pad.IsRightThumbStickUp() || pad.IsRightThumbStickDown())
                {
                    auto benchmark = SelectedBenchmark();

                    auto reportHeight = benchmark->GetReportRowCount() * m_fontReport->GetLineSpacing();;

                    float scrollScale = 10.0f;
                    m_scrollY -= pad.thumbSticks.rightY * scrollScale;
                    m_scrollY = std::max(0.0f, m_scrollY);
                    m_scrollY = std::min(float(reportHeight) - 40.0f, m_scrollY);
                }
            }
        }

        if (!m_pendingBenchmarkIndices.empty())
        {
            StartBenchmark(m_pendingBenchmarkIndices.front());
            m_pendingBenchmarkIndices.pop();
        }
    }

    PIXEndEvent();
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto commandQueue = m_deviceResources->GetCommandQueue();
    auto benchmark = RunningBenchmark();

    if (benchmark && IBenchmark::STATE_NOT_STARTED != benchmark->GetState() && IBenchmark::STATE_STOPPED != benchmark->GetState())
    {
        benchmark->RunOneFrame(device, commandQueue);
        if (IBenchmark::STATE_COMPLETE == benchmark->GetState())
        {
            StopBenchmark();
        }
    }

    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the render target to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    PIXBeginEvent(commandQueue, PIX_COLOR_DEFAULT, L"Render");

    RenderUI();

    PIXEndEvent(commandQueue);

    // Show the new frame.
    PIXBeginEvent(commandQueue, PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(commandQueue);
    PIXEndEvent(commandQueue);
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

void Sample::RenderUI()
{
    auto const size = m_deviceResources->GetOutputSize();
    auto const width = size.right - size.left;
    auto const height = size.bottom - size.top;

    auto commandList = m_deviceResources->GetCommandList();

    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptorHeap->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    m_spriteBatch->Begin(commandList);

    auto const viewport = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewport);

    float benchmarkX = 20.0f;
    float benchmarkY = 20.0f;
    float yInc = 40.0f;

    DirectX::SimpleMath::Vector2 benchmarkPos(benchmarkX, benchmarkY);

    for (auto benchmark : Benchmark::BenchmarkList())
    {
        auto color = benchmark == SelectedBenchmark() ? DirectX::Colors::LightGreen : DirectX::Colors::LightGray;
        m_fontBenchmark->DrawString(m_spriteBatch.get(), benchmark->GetName(), benchmarkPos, color);
        benchmarkPos.y += yInc;
    }

    m_spriteBatch->End();

    auto benchmark = SelectedBenchmark();

    DirectX::SimpleMath::Vector2 reportPos(250.0f, 20.0f);

    switch (benchmark->GetState())
    {
    case IBenchmark::STATE_NOT_STARTED:
    {
        m_spriteBatch->Begin(commandList);

        DX::DrawControllerString(m_spriteBatch.get(), m_fontReport.get(), m_fontController.get(), L"Press [A] to run selection, [X] to run all...\n", reportPos);

        m_spriteBatch->End();
    }
    break;

    case IBenchmark::STATE_STARTED:
    case IBenchmark::STATE_MEASURE_TIME:
    {
        m_spriteBatch->Begin(commandList);

        wchar_t buffer[1024] = L"";
        swprintf_s(buffer, _countof(buffer), L"Measuring timings for benchmark \"%s\"...\n", benchmark->GetName());
        m_fontReport->DrawString(m_spriteBatch.get(), buffer, reportPos);

        m_spriteBatch->End();
    }
    break;

    case IBenchmark::STATE_MEASURE_COUNTERS:
    {
        m_spriteBatch->Begin(commandList);

        wchar_t buffer[1024] = L"";
        swprintf_s(buffer, _countof(buffer), L"Measuring GPU counters for benchmark \"%s\"...\n", benchmark->GetName());
        m_fontReport->DrawString(m_spriteBatch.get(), buffer, reportPos);

        m_spriteBatch->End();
    }
    break;

    case IBenchmark::STATE_REPORT:
    case IBenchmark::STATE_PIX_CAPTURE:
    case IBenchmark::STATE_COMPLETE:
    case IBenchmark::STATE_STOPPED:
    default:
    {
        m_spriteBatch->Begin(commandList);

        m_fontReport->DrawString(m_spriteBatch.get(), benchmark->GetReportHeader().c_str(), reportPos);
        auto drawBounds = m_fontReport->MeasureDrawBounds(benchmark->GetReportHeader().c_str(), reportPos);
        reportPos.y += float(drawBounds.bottom - drawBounds.top);

        m_spriteBatch->End();

        m_spriteBatch->Begin(commandList, SpriteSortMode_Immediate);

        D3D12_RECT rectReportBody =
        {
            0,                                                      // LONG    left;
            0,                                                      // LONG    top;
            width,                                                  // LONG    right;
            height,                                                 // LONG    bottom;
        };
        rectReportBody.top = LONG(reportPos.y);
        rectReportBody.bottom -= LONG(reportPos.y);
        commandList->RSSetScissorRects(1, &rectReportBody);

        reportPos.y -= m_scrollY;
        for (auto row = 0U; row < benchmark->GetReportRowCount(); ++row)
        {
            m_fontReport->DrawString(m_spriteBatch.get(), benchmark->GetReportRow(row).c_str(), reportPos, benchmark->GetReportRowColor(row));
            reportPos.y += m_fontReport->GetLineSpacing();
        }

        commandList->RSSetScissorRects(0, nullptr);

        m_spriteBatch->End();
    }
    break;
    }
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
    for (int player = 0; player < GamePad::MAX_PLAYER_COUNT; ++player)
    {
        m_gamePadButtons[player].Reset();
    }
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

#ifdef _GAMING_XBOX
    // Determine hardware type
    device->GetGpuHardwareConfigurationX(&g_gpuHardwareConfiguration);

#ifdef _GAMING_XBOX_SCARLETT
    // GetGpuHardwareConfigurationX still returns 0 for GpuFrequency on Scarlett
    if (0ULL == g_gpuHardwareConfiguration.GpuFrequency)
    {
        if ((D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_DEVKIT == g_gpuHardwareConfiguration.HardwareVersion
            || D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_ANACONDA == g_gpuHardwareConfiguration.HardwareVersion))
        {
            g_gpuHardwareConfiguration.GpuFrequency = 1825000000ULL;
        }
        else if (D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART == g_gpuHardwareConfiguration.HardwareVersion)
        {
            g_gpuHardwareConfiguration.GpuFrequency = 1563000000ULL;
        }
    }
#endif

    {
        std::wostringstream message;
        message << L"Detected GPU configuration ";
        switch (g_gpuHardwareConfiguration.HardwareVersion)
        {
        case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE:               message << L"D3D12XBOX_HARDWARE_VERSION_XBOX_ONE";                  break;
        case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_S:             message << L"D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_S";                break;
        case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X:             message << L"D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X";                break;
        case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X_DEVKIT:      message << L"D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X_DEVKIT";         break;
#ifdef _GAMING_XBOX_SCARLETT
        case D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART: message << L"D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART";    break;
        case D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_ANACONDA: message << L"D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_ANACONDA";    break;
        case D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_DEVKIT:   message << L"D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_DEVKIT";      break;
#endif
        default:                                                message << L"<unknown>"; break;
        }
        message << std::endl;
        OutputDebugString(message.str().c_str());
    }

    auto deviceType = XSystemGetDeviceType();
    {
        std::wostringstream message;
        message << L"Detected console mode ";
        switch (deviceType)
        {
        case XSystemDeviceType::Unknown:               message << L"Unknown";               break;
        case XSystemDeviceType::Pc:                    message << L"Pc";                    break;
        case XSystemDeviceType::XboxOne:               message << L"XboxOne";               break;
        case XSystemDeviceType::XboxOneS:              message << L"XboxOneS";              break;
        case XSystemDeviceType::XboxOneX:              message << L"XboxOneX";              break;
        case XSystemDeviceType::XboxOneXDevkit:        message << L"XboxOneXDevkit";        break;
        case XSystemDeviceType::XboxScarlettLockhart:  message << L"XboxSeriesS";           break;
        case XSystemDeviceType::XboxScarlettAnaconda:  message << L"XboxSeriesX";           break;
        case XSystemDeviceType::XboxScarlettDevkit:    message << L"XboxSeriesXDevkit";     break;
        default:                                       message << L"Unexpected";            break;
        }
        message << std::endl;
        OutputDebugString(message.str().c_str());
    }

    auto d3dBasedDeviceType = XSystemDeviceType::Unknown;
    switch (g_gpuHardwareConfiguration.HardwareVersion)
    {
    case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE:               d3dBasedDeviceType = XSystemDeviceType::XboxOne;                break;
    case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_S:             d3dBasedDeviceType = XSystemDeviceType::XboxOneS;               break;
    case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X:             d3dBasedDeviceType = XSystemDeviceType::XboxOneX;               break;
    case D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_X_DEVKIT:      d3dBasedDeviceType = XSystemDeviceType::XboxOneXDevkit;         break;
#ifdef _GAMING_XBOX_SCARLETT
    case D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART: d3dBasedDeviceType = XSystemDeviceType::XboxScarlettLockhart;   break;
    case D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_ANACONDA: d3dBasedDeviceType = XSystemDeviceType::XboxScarlettAnaconda;   break;
    case D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_DEVKIT:   d3dBasedDeviceType = XSystemDeviceType::XboxScarlettDevkit;     break;
#endif
    default:                                                d3dBasedDeviceType = XSystemDeviceType::Unknown;                break;
    }

    if (d3dBasedDeviceType != deviceType)
    {
        throw std::exception("Mismatch between D3D console type and GXDK console type (should not be possible).");
    }

#ifdef _GAMING_XBOX_SCARLETT
    // Future hardware will trigger this.
    if (g_gpuHardwareConfiguration.HardwareVersion > D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_DEVKIT)
    {
        throw std::exception("Unrecognized (new) GPU hardware type.");
    }
#else
    // Future hardware will trigger this.
    if (g_gpuHardwareConfiguration.HardwareVersion > D3D12XBOX_HARDWARE_VERSION_SCORPIO_DEVKIT)
    {
        throw std::exception("Unrecognized (new) GPU hardware type.");
    }
#endif

#else // #ifdef _GAMING_DESKTOP
    DXGI_ADAPTER_DESC1 adapterDesc;
    if (S_OK == m_deviceResources->GetDeviceAdapter()->GetDesc1(&adapterDesc))
    {
        GpuProperties::Initialize(device, adapterDesc.VendorId);
    }
#endif // #ifdef _GAMING_XBOX

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_resourceDescriptorHeap = std::make_unique<DescriptorHeap>(device, ResourceDescriptors::Count);

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    m_fontBenchmark = std::make_unique<SpriteFont>(device, 
        resourceUpload, 
        strFilePath,
        m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::FontBenchmark), 
        m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::FontBenchmark));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"Courier_16.spritefont");
    m_fontReport = std::make_unique<SpriteFont>(device, 
        resourceUpload, 
        strFilePath,
        m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::FontReport), 
        m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::FontReport));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerSmall.spritefont");
    m_fontController = std::make_unique<SpriteFont>(device, 
        resourceUpload,
        strFilePath,
        m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::FontController), 
        m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::FontController));

    const RenderTargetState renderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pipelineDescription(renderTargetState);
    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pipelineDescription);

    auto commandQueue = m_deviceResources->GetCommandQueue();
    resourceUpload.End(commandQueue);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
}
#pragma endregion

#pragma region Benchmarks
void Sample::OpenLogFile(bool reset)
{
    if (nullptr != m_logFile)
    {
        throw std::exception("Attempt to open log file which is already open");
    }

#ifdef _GAMING_DESKTOP
    std::wstring logFileName = m_logFileName;

    DXGI_ADAPTER_DESC1 adapterDesc;
    if (S_OK == m_deviceResources->GetDeviceAdapter()->GetDesc1(&adapterDesc))
    {
        size_t splitPos = logFileName.rfind(L".txt");
        if (splitPos != std::wstring::npos)
        {
            wchar_t buffer[64];
            _snwprintf_s(buffer, std::size(buffer), sizeof(buffer) / sizeof(wchar_t), L" vendorId=0x%04x deviceId=0x%08x revision=0x%08x", adapterDesc.VendorId, adapterDesc.DeviceId, adapterDesc.Revision);
           
            logFileName = logFileName.substr(0, splitPos);
            logFileName += buffer;
            logFileName += L".txt";
        }
    }
    if (0 != _wfopen_s(&m_logFile, logFileName.c_str(), reset ? L"wt" : L"at") || nullptr == m_logFile)
#else
    if (0 != _wfopen_s(&m_logFile, m_logFileName, reset ? L"wt" : L"at") || nullptr == m_logFile)
#endif
    {
        throw std::exception("Could not open log file");
    }
}

void Sample::CloseLogFile()
{
    if (nullptr == m_logFile)
    {
        throw std::exception("Attempt to close log file which is not open");
    }
    if (EOF == fclose(m_logFile))
    {
        throw std::exception("Could not close log file");
    }
    m_logFile = nullptr;
}

void Sample::Output(const wchar_t* str, size_t len) const
{
    if (nullptr == m_logFile)
    {
        throw std::exception("Attempt to write to invalid log file");
    }
    OutputDebugString(str);

    std::vector<char> buf(64 * 1024, '\0');
    if (0 != wcstombs_s(nullptr, buf.data(), buf.size(), str, len))
    {
        throw std::exception("Attempt to convert log string failed");
    }
    if (0 == fwrite(buf.data(), sizeof(char), len, m_logFile))
    {
        throw std::exception("Attempt to write to log file failed");
    }
}

void Sample::StartBenchmark(uint32_t benchmarkIndex)
{
    m_runningBenchmarkIndex = m_selectedBenchmarkIndex = benchmarkIndex;

    auto benchmark = RunningBenchmark();

    if (IBenchmark::STATE_NOT_STARTED == benchmark->GetState() || IBenchmark::STATE_STOPPED == benchmark->GetState())
    {
        std::wostringstream message;
        message << L"Starting benchmark \"";
        message << benchmark->GetName();
        message << L"\"";
        message << std::endl;
        OutputDebugString(message.str().c_str());

        benchmark->Start(m_deviceResources->GetD3DDevice());

        if (IBenchmark::STATE_STARTED != benchmark->GetState())
        {
            throw std::exception("Benchmark failed to start");
        }
    }
    else
    {
        throw std::exception("Tried to start an already running benchmark");
    }
}

void Sample::StopBenchmark()
{
    auto benchmark = RunningBenchmark();

    m_runningBenchmarkIndex = m_invalidBenchmarkIndex;

    benchmark->Stop();

    if (IBenchmark::STATE_STOPPED != benchmark->GetState())
    {
        throw std::exception("Benchmark failed to stop");
    }

    std::wostringstream message;
    message << L"Stopping benchmark \"";
    message << benchmark->GetName();
    message << L"\"";
    message << std::endl;
    OutputDebugString(message.str().c_str());

    static bool firstTime = true;
    OpenLogFile(firstTime);
    if (firstTime)
    {
        firstTime = false;
    }

    std::wostringstream reportMessage;
    reportMessage << L"Report for benchmark \"";
    reportMessage << benchmark->GetName();
    reportMessage << L"\"";

    Output(reportMessage.str().c_str(), reportMessage.str().length());
    Output(L"\n", 1);
    Output(benchmark->GetReportHeader().c_str(), benchmark->GetReportHeader().length());
    for (auto row = 0U; row < benchmark->GetReportRowCount(); ++row)
    {
        Output(benchmark->GetReportRow(row).c_str(), benchmark->GetReportRow(row).length());
    }
    Output(L"\n", 1);

    CloseLogFile();
}
void Sample::OnDeviceLost()
{
    m_spriteBatch.reset();
    m_fontController.reset();
    m_fontReport.reset();
    m_fontBenchmark.reset();
    m_resourceDescriptorHeap.reset();
    m_graphicsMemory.reset();

}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
