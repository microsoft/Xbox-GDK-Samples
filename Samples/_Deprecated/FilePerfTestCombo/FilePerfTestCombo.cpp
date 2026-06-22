//--------------------------------------------------------------------------------------
// FilePerfTestCombo.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FilePerfTestCombo.h"

#include <ATGColors.h>
#include <FindMedia.h>
#include "Processor.h"
#include "CommandLine.h"

#ifdef _GAMING_DESKTOP
#include "FileCreate\SetupFlatFileData.h"
#include "FileCreate\SetupZipFileData.h"
#endif

extern bool RunDirectedTest(const std::wstring& layoutFileName, const std::wstring& readSetFileName, uint32_t numIterations, const std::wstring& packFileName, const std::wstring& rootDirName, const std::wstring& dataOrder, uint32_t dataSize, bool forceAsync = false, uint32_t qdiOverrideLow = FIRST_OVERLAP_DEPTH, uint32_t LAST_OVERLAP_DEPTH = UINT32_MAX);
extern bool RunDirectedDStorageTest(const std::wstring& layoutFileName, const std::wstring& readSetFileName, uint32_t numIterations, const std::wstring& packFileName, const std::wstring& rootDirName, const std::wstring& dataOrder, uint32_t dataSize, bool forceSync = false, bool zipFileCompressed = false, bool useRealtime = false);

extern void ExitSample();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Sample* Sample::s_instance = nullptr;

#ifdef _GAMING_DESKTOP
// The default directory for GDK Desktop is forced to c:\windows\.., the executable file location is the default current directory for Win32, however not for GDK Desktop so we have to search for it
// For this case we want to location of the sln file since that is where we're going to write the data files
void FixWorkingDirectoryForDataCreation()
{
    std::filesystem::path curDir(std::filesystem::current_path());

    // early skip if we happen to be in the windows directory which is never right
    if (strncmp(curDir.string().c_str(), "C:\\WINDOWS", 10) != 0)
    {
        for (auto& p : std::filesystem::directory_iterator(curDir))
        {
            if (std::filesystem::is_regular_file(p.path()))
            {
                if (p.path().extension().string() == ".sln")
                {
                    SetCurrentDirectoryA(curDir.string().c_str());
                    return;
                }
            }
        }
    }

    wchar_t processPath[256] = {};
    GetModuleFileNameW(nullptr, processPath, 256);
    curDir = processPath;
    curDir = curDir.parent_path();

    while ((curDir != curDir.root_path()) && (!curDir.empty()))
    {
        for (auto& p : std::filesystem::directory_iterator(curDir))
        {
            if (std::filesystem::is_regular_file(p.path()))
            {
                if (p.path().extension().string() == ".sln")
                {
                    SetCurrentDirectoryA(curDir.string().c_str());
                    return;
                }
            }
        }
        curDir = curDir.parent_path();
    }
}
#endif

void Sample::PerformFileTasks()
{
    SetThreadAffinityMask(GetCurrentThread(), 1ULL << 4);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    ATG::SetupProcessorData();

#ifdef _GAMING_DESKTOP
    //performfullsetup createpackedfile numiterations 5
    if (m_performFullSetup)
    {
        m_creatingFlatFiles = true;
        FixWorkingDirectoryForDataCreation();   // Note: Data files should be created in the project directory as opposed to the default GDK current directory
                                                // When creating files we leave the current directory pointing here for later file loading this run
                                                // We don't want to have to copy them into the build directory
        SetupFlatFileData* setup = new SetupFlatFileData();
        setup->PerformFullSetup(m_createPackedFile, m_numIterations, 100, L"FileData", L"test_file");
        delete setup;
        m_creatingFlatFiles = false;
    }
    if (m_performZipSetup)
    {
        m_creatingZipFiles = true;
        FixWorkingDirectoryForDataCreation();   // Note: Data files should be created in the project directory as opposed to the default GDK current directory
                                                // When creating files we leave the current directory pointing here for later file loading this run
                                                // We don't want to have to copy them into the build directory
        SetupZipFileData* setup = new SetupZipFileData();
        setup->PerformFullSetup(true, m_numIterations, m_compressionRatio, L"ZipData", L"test_file");
        delete setup;
        m_creatingZipFiles = false;
    }

    try
    {
        static const wchar_t* dataSearchFolders[] =
        {
            L"LayoutSets",
            0
        };
        wchar_t strFilePath[MAX_PATH] = {};
        DX::FindMediaFile(strFilePath, MAX_PATH, L"FileData.xml", dataSearchFolders);

        std::filesystem::path newDir(strFilePath);
        newDir = newDir.parent_path().parent_path();
        if (!newDir.empty())
            SetCurrentDirectoryA(newDir.string().c_str());
    }
    catch (...)         // main data file was not found, there is no fallback so catch the error from FindMediaFile and exit
    {
        m_finishedTestRun = true;
        return;
    }

#endif

#ifdef _GAMING_XBOX
    ATG::SetProcessorName(ATG::GetTrueProcessorName() + L"_GDK");
#elif defined(_GAMING_DESKTOP)
    ATG::SetProcessorName(ATG::GetTrueProcessorName() + L"_GDK");
#else	// WIN32
    ATG::SetProcessorName(ATG::GetTrueProcessorName() + L"_WIN32");
#endif

    if (m_doSync && m_numIterations)
    {
        m_testTypeRunning = Doing_Sync;
        for (uint32_t dataOrder = m_minLoadOrder; (dataOrder <= m_maxLoadOrder) && (!m_shutdownThread); dataOrder++)
        {
            for (uint32_t dataSize = m_minDataSize; (dataSize <= m_maxDataSize) && (!m_shutdownThread); dataSize++)
            {
                wchar_t readSetFileName[256];
                swprintf(readSetFileName, 256, L"ReadSets\\win32_directedReadSet_%s_%s.xml", ConvertLoadOrderToString(dataOrder).c_str(), ConvertDataSizeToString(dataSize).c_str());
                if (GetFileAttributesW(readSetFileName) == INVALID_FILE_ATTRIBUTES)
                    swprintf(readSetFileName, 256, L"ReadSets\\directedReadSet_%s_%s.xml", ConvertLoadOrderToString(dataOrder).c_str(), ConvertDataSizeToString(dataSize).c_str());

                std::wstring layoutFile(m_layoutFileName);
                if (m_layoutFileName.size() == 0)
                {
                    layoutFile = L"LayoutSets\\FileData";
                }
                layoutFile += L".xml";

                m_workingLoadOrder = dataOrder;
                m_workingDataSize = dataSize;
                RunDirectedTest(layoutFile, readSetFileName, m_numIterations, m_usePackedFile ? L"FileData" : L"", L"FileData", ConvertLoadOrderToString(dataOrder), ConvertDataSizeToInteger(dataSize), false, depth_1, depth_1);
                m_finishedLoadOrder = dataOrder;
                m_finishedDataSize = dataSize;
            }
        }
    }

    if (m_doAsync && m_numIterations)
    {
        m_testTypeRunning = Doing_Async;

        for (uint32_t dataOrder = m_minLoadOrder; (dataOrder <= m_maxLoadOrder) && (!m_shutdownThread); dataOrder++)
        {
            for (uint32_t queueDepth = m_minOverlapDepth; (queueDepth <= m_maxOverlapDepth) && (!m_shutdownThread); queueDepth++)
            {
                for (uint32_t dataSize = m_minDataSize; (dataSize <= m_maxDataSize) && (!m_shutdownThread); dataSize++)
                {
                    wchar_t readSetFileName[256];
                    swprintf(readSetFileName, 256, L"ReadSets\\win32_directedReadSet_%s_%s.xml", ConvertLoadOrderToString(dataOrder).c_str(), ConvertDataSizeToString(dataSize).c_str());
                    if (GetFileAttributesW(readSetFileName) == INVALID_FILE_ATTRIBUTES)
                        swprintf(readSetFileName, 256, L"ReadSets\\directedReadSet_%s_%s.xml", ConvertLoadOrderToString(dataOrder).c_str(), ConvertDataSizeToString(dataSize).c_str());

                    std::wstring layoutFile(m_layoutFileName);
                    if (m_layoutFileName.size() == 0)
                    {
                        layoutFile = L"LayoutSets\\FileData";
                    }
                    layoutFile += L".xml";

                    m_workingLoadOrder = dataOrder;
                    m_workingDataSize = dataSize;
                    RunDirectedTest(layoutFile, readSetFileName, m_numIterations, m_usePackedFile ? L"FileData" : L"", L"FileData", ConvertLoadOrderToString(dataOrder), ConvertDataSizeToInteger(dataSize), true, queueDepth, queueDepth);
                    m_finishedLoadOrder = dataOrder;
                    m_finishedDataSize = dataSize;
                }
            }
        }
    }

    if ((m_doAsyncDStorage || m_doSyncDStorage || m_doSyncZipDStorage || m_doAsyncZipDStorage) && m_numIterations)
    {
        for (uint32_t dataOrder = m_minLoadOrder; (dataOrder <= m_maxLoadOrder) && (!m_shutdownThread); dataOrder++)
        {
            for (uint32_t dataSize = m_minDataSize; (dataSize <= m_maxDataSize) && (!m_shutdownThread); dataSize++)
            {
                wchar_t readSetFileName[256];

                m_workingLoadOrder = dataOrder;
                m_workingDataSize = dataSize;
                std::wstring layoutFile;
                std::wstring dataFolder;
                if (m_doAsyncDStorage || m_doSyncDStorage)
                {
                    swprintf(readSetFileName, 256, L"ReadSets\\ds_directedReadSet_%s_%s.xml", ConvertLoadOrderToString(dataOrder).c_str(), ConvertDataSizeToString(dataSize).c_str());
                    if (GetFileAttributesW(readSetFileName) == INVALID_FILE_ATTRIBUTES)
                        swprintf(readSetFileName, 256, L"ReadSets\\directedReadSet_%s_%s.xml", ConvertLoadOrderToString(dataOrder).c_str(), ConvertDataSizeToString(dataSize).c_str());
                    dataFolder = L"FileData";
                    if (m_layoutFileName.size() == 0)
                    {
                        layoutFile = L"LayoutSets\\FileData";
                    }
                    else
                    {
                        layoutFile = m_layoutFileName;
                    }
                    layoutFile += L".xml";
                    if (m_doAsyncDStorage)
                    {
                        m_testTypeRunning = Doing_DStorage_Async;
                        RunDirectedDStorageTest(layoutFile, readSetFileName, m_numIterations, m_usePackedFile ? dataFolder : L"", dataFolder, ConvertLoadOrderToString(dataOrder), ConvertDataSizeToInteger(dataSize), false, false, m_doRealtime);
                    }
                    if (m_doSyncDStorage)
                    {
                        m_testTypeRunning = Doing_DStorage_Sync;
                        RunDirectedDStorageTest(layoutFile, readSetFileName, m_numIterations, m_usePackedFile ? dataFolder : L"", dataFolder, ConvertLoadOrderToString(dataOrder), ConvertDataSizeToInteger(dataSize), true, false, m_doRealtime);
                    }
                }

                if (m_doAsyncZipDStorage || m_doSyncZipDStorage)
                {
                    swprintf(readSetFileName, 256, L"ReadSets\\zipData_%s_%s.xml", ConvertDataSizeToString(dataSize).c_str(), ConvertLoadOrderToString(dataOrder).c_str());
                    dataFolder = L"ZipData_";
                    dataFolder += ConvertDataSizeToString(dataSize);
                    if (m_layoutFileName.size() == 0)
                        layoutFile = L"LayoutSets\\ZipData_";
                    else
                        layoutFile += L"_";
                    layoutFile += ConvertDataSizeToString(dataSize);
                    layoutFile += L".xml";

                    if (m_doAsyncZipDStorage)
                    {
                        m_testTypeRunning = Doing_DStorage_Async_zlib;
                        RunDirectedDStorageTest(layoutFile, readSetFileName, m_numIterations, m_usePackedFile ? dataFolder : L"", dataFolder, ConvertLoadOrderToString(dataOrder), ConvertDataSizeToInteger(dataSize), false, true, m_doRealtime);
                    }
                    if (m_doSyncZipDStorage)
                    {
                        m_testTypeRunning = Doing_DStorage_Sync_zlib;
                        RunDirectedDStorageTest(layoutFile, readSetFileName, m_numIterations, m_usePackedFile ? dataFolder : L"", dataFolder, ConvertLoadOrderToString(dataOrder), ConvertDataSizeToInteger(dataSize), true, true, m_doRealtime);
                    }
                }
            }
        }
    }

    m_finishedTestRun = true;
}

Sample::Sample() noexcept(false) :
    m_cmdLineError(false)
    , m_doAsync(false)
    , m_doSync(false)
    , m_doZip(false)
    , m_doRealtime(false)

    , m_performFullSetup(false)
    , m_performZipSetup(false)
    , m_createPackedFile(false)
    , m_compressionRatio(50)

    , m_doSyncDStorage(false)
    , m_doAsyncDStorage(false)
    , m_doSyncZipDStorage(false)
    , m_doAsyncZipDStorage(false)

    , m_usePackedFile(false)
    , m_minLoadOrder(FIRST_LOAD_ORDER)
    , m_maxLoadOrder(LAST_PERF_ORDER)
    , m_minDataSize(FIRST_DATA_SIZE)
    , m_maxDataSize(LAST_DATA_SIZE)
    , m_minOverlapDepth(FIRST_OVERLAP_DEPTH)
    , m_maxOverlapDepth(LAST_OVERLAP_DEPTH)
    , m_numIterations(1)
    , m_shutdownThread(false)
    , m_finishedTestRun(false)
    , m_creatingFlatFiles(false)
    , m_creatingZipFiles(false)
    , m_workerThread(nullptr)
{
    assert(!s_instance);
    s_instance = this;

    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    if (m_workerThread)
    {
        m_shutdownThread = true;
        if (!m_finishedTestRun)
        {
#pragma warning(suppress: 6258) // TerminateThread is intentional as a last-resort cleanup
            TerminateThread(m_workerThread->native_handle(), 0);
        }
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
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    if (timer.GetFrameCount() == 3)
    {
        SetThreadAffinityMask(GetCurrentThread(), 0x01);

        if (!m_workerThread)
        {
            m_workingLoadOrder = m_minLoadOrder;
            m_workingDataSize = m_minDataSize;
            m_workingOverlapDepth = m_minOverlapDepth;
            m_finishedLoadOrder = m_minLoadOrder;
            m_finishedDataSize = m_minDataSize;
            m_finishedOverlapDepth = m_minOverlapDepth;

            m_testTypeRunning = Doing_Nothing;
            m_finishedTestRun = false;
            m_workerThread = new std::thread(&Sample::PerformFileTasks, this);
        }
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

    const RECT safeRect = SimpleMath::Viewport::ComputeTitleSafeArea(1920, 1080);

    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    ID3D12DescriptorHeap* pHeaps[] = { m_resourceDescriptors->Heap() };
    commandList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), XMFLOAT2(0, 0));

    if (m_timer.GetFrameCount() > 3)
    {
        uint32_t numDots = (m_timer.GetFrameCount() % 10) + 1;
        std::wstring outputString;
        if (m_cmdLineError)
        {
            outputString = L"Error Parsing Command Line";
        }
        else if (m_creatingFlatFiles)
        {
            outputString = L"Creating Data Files, this is expensive";
        }
        else if (m_creatingZipFiles)
        {
            outputString = L"Creating Zip Data Files, this is very expensive";
        }
        else if (m_finishedTestRun)
        {
            outputString = L"Finished Test Run";
        }
        else
        {
            switch (m_testTypeRunning)
            {
            case Doing_Nothing:
                outputString = L"Running: ";
                break;
            case Doing_Async:
                outputString = L"Performing Async: ";
                break;
            case Doing_Sync:
                outputString = L"Performing Sync: ";
                break;
            case Doing_DStorage_Async:
                outputString = L"Performing Async DirectStorage: ";
                break;
            case Doing_DStorage_Sync:
                outputString = L"Performing Sync DirectStorage: ";
                break;
            case Doing_DStorage_Async_zlib:
                outputString = L"Performing Async zlib DirectStorage: ";
                break;
            case Doing_DStorage_Sync_zlib:
                outputString = L"Performing Sync zlib DirectStorage: ";
                break;
            }
            if (m_testTypeRunning != Doing_Nothing)
            {
                outputString += ConvertLoadOrderToString(m_workingLoadOrder);
                outputString += L" - ";
                outputString += ConvertDataSizeToString(m_workingDataSize);
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
    auto const viewport = m_deviceResources->GetScreenViewport();
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

void Sample::ParseCommandLine(const wchar_t* commandlineParams)
{
    std::vector<std::wstring> cmdLine;

    cmdLine = ATG::BreakCommandLine(commandlineParams);

    for (auto& iter : cmdLine)
    {
        std::for_each(iter.begin(), iter.end(), [](wchar_t& letter) {letter = static_cast<wchar_t> (towlower(letter)); });
    }
    std::vector<std::wstring>::iterator iter, endIter;
    std::vector<std::wstring>::iterator firstParam, secondParam;
    endIter = cmdLine.end();
    for (iter = cmdLine.begin(); (iter != endIter) && (!m_cmdLineError);)
    {
        if (*iter == L"numiterations")
        {
            firstParam = iter;
            ++firstParam;
            if (firstParam == endIter)
            {
                m_cmdLineError = true;
                continue;
            }
            m_numIterations = static_cast<uint32_t> (std::stoi(*firstParam));
            if (m_numIterations == 0)
            {
                m_cmdLineError = true;
                continue;
            }
            ++iter;
            ++iter;
        }
        else if (*iter == L"doasync")
        {
            m_doAsync = true;
            ++iter;
        }
        else if (*iter == L"dozip")
        {
            m_doZip = true;
            ++iter;
        }

        else if (*iter == L"dosyncdstorage")
        {
            m_doSyncDStorage = true;
            ++iter;
        }
        else if (*iter == L"doasyncdstorage")
        {
            m_doAsyncDStorage = true;
            ++iter;
        }
        else if (*iter == L"dosynczipdstorage")
        {
            m_doSyncZipDStorage = true;
            ++iter;
        }
        else if (*iter == L"doasynczipdstorage")
        {
            m_doAsyncZipDStorage = true;
            ++iter;
        }
        else if (*iter == L"dorealtime")
        {
            m_doRealtime = true;
            ++iter;
        }
        else if (*iter == L"dosync")
        {
            m_doSync = true;
            ++iter;
        }
        else if (*iter == L"layout")
        {
            ++iter;
            if (iter == endIter)
            {
                m_cmdLineError = true;
                continue;
            }
            m_layoutFileName = *iter;
            ++iter;
        }

        else if (*iter == L"usepackedfile")
        {
            m_usePackedFile = true;
            ++iter;
        }
        else if (*iter == L"load")
        {
            firstParam = iter;
            ++firstParam;
            if (firstParam == endIter)
            {
                m_cmdLineError = true;
                continue;
            }
            secondParam = firstParam;
            ++secondParam;
            if (secondParam == endIter)
            {
                m_cmdLineError = true;
                continue;
            }
            m_minLoadOrder = ConvertStringToLoadOrder(*firstParam);
            m_maxLoadOrder = ConvertStringToLoadOrder(*secondParam);
            ++iter;
            ++iter;
            ++iter;
        }
        else if (*iter == L"size")
        {
            firstParam = iter;
            ++firstParam;
            if (firstParam == endIter)
            {
                m_cmdLineError = true;
                continue;
            }
            secondParam = firstParam;
            ++secondParam;
            if (secondParam == endIter)
            {
                m_cmdLineError = true;
                continue;
            }
            m_minDataSize = ConvertStringToDataSize(*firstParam);
            m_maxDataSize = ConvertStringToDataSize(*secondParam);
            ++iter;
            ++iter;
            ++iter;
        }
        else if (*iter == L"depth")
        {
            firstParam = iter;
            ++firstParam;
            if (firstParam == endIter)
            {
                m_cmdLineError = true;
                continue;
            }
            secondParam = firstParam;
            ++secondParam;
            if (secondParam == endIter)
            {
                m_cmdLineError = true;
                continue;
            }
            m_minOverlapDepth = ConvertStringToOverlapDepth(*firstParam);
            m_maxOverlapDepth = ConvertStringToOverlapDepth(*secondParam);
            ++iter;
            ++iter;
            ++iter;
        }
        else if (*iter == L"performfullsetup")
        {
            m_performFullSetup = true;
            ++iter;
            continue;
        }
        else if (*iter == L"performzipsetup")
        {
            m_performZipSetup = true;
            ++iter;
            continue;
        }
        else if (*iter == L"ratio")
        {
            firstParam = iter;
            ++firstParam;
            if (firstParam == endIter)
            {
                m_cmdLineError = true;
                continue;
            }
            m_compressionRatio = static_cast<uint32_t> (std::stoi(*firstParam));
            if (m_numIterations == 0)
            {
                m_cmdLineError = true;
                continue;
            }
            ++iter;
            ++iter;
        }
        else if (*iter == L"createpackedfile")
        {
            m_createPackedFile = true;
            ++iter;
        }
        else
        {
            m_cmdLineError = true;
        }
    }
}

uint32_t ConvertDataSizeToInteger(DataSize dataSize)
{
    switch (dataSize)
    {
    case DataSize::size_8k:
        return 8 * 1024ULL;
        break;
    case DataSize::size_12k:
        return 12 * 1024ULL;
        break;
    case DataSize::size_16k:
        return 16 * 1024ULL;
        break;
    case DataSize::size_32k:
        return 32 * 1024ULL;
        break;
    case DataSize::size_64k:
        return 64 * 1024ULL;
        break;
    case DataSize::size_128k:
        return 128 * 1024ULL;
        break;
    case DataSize::size_192k:
        return 192 * 1024ULL;
        break;
    case DataSize::size_256k:
        return 256 * 1024ULL;
        break;
    case DataSize::size_512k:
        return 512 * 1024ULL;
        break;
    case DataSize::size_1024k:
        return 1024 * 1024ULL;
        break;
    case DataSize::size_2048k:
        return 2048 * 1024ULL;
        break;
    case DataSize::size_4096k:
        return 4096 * 1024ULL;
        break;
    case DataSize::size_8192k:
        return 8192 * 1024ULL;
        break;
    case DataSize::size_16384k:
        return 16384 * 1024ULL;
        break;
    case DataSize::size_32768k:
        return 32768 * 1024ULL;
        break;
    case DataSize::LAST_DATA_SIZE:
        return 32768 * 1024ULL;
    }
    return 65536;
}

std::wstring ConvertDataSizeToString(DataSize dataSize)
{
    switch (dataSize)
    {
    case DataSize::size_8k:
        return L"8k";
        break;
    case DataSize::size_12k:
        return L"12k";
        break;
    case DataSize::size_16k:
        return L"16k";
        break;
    case DataSize::size_32k:
        return L"32k";
        break;
    case DataSize::size_64k:
        return L"64k";
        break;
    case DataSize::size_128k:
        return L"128k";
        break;
    case DataSize::size_192k:
        return L"192k";
        break;
    case DataSize::size_256k:
        return L"256k";
        break;
    case DataSize::size_512k:
        return L"512k";
        break;
    case DataSize::size_1024k:
        return L"1024k";
        break;
    case DataSize::size_2048k:
        return L"2048k";
        break;
    case DataSize::size_4096k:
        return L"4096k";
        break;
    case DataSize::size_8192k:
        return L"8192k";
        break;
    case DataSize::size_16384k:
        return L"16384k";
        break;
    case DataSize::size_32768k:
        return L"32768k";
        break;
    case DataSize::LAST_DATA_SIZE:
        return L"unknown data size";
        break;
    }
    return L"unknown data size";
}

std::wstring ConvertLoadOrderToString(LoadOrder loadOrder)
{
    switch (loadOrder)
    {
    case LoadOrder::TrueSequential:
        return L"true_sequential";
        break;
    case LoadOrder::RandomSequential:
        return L"random_sequential";
        break;
    case LoadOrder::Random:
        return L"random";
        break;
    case LoadOrder::Backwards:
        return L"backwards";
        break;
    case LoadOrder::Redundant:
        return L"redundant";
        break;
    case LoadOrder::LAST_LOAD_ORDER:
        return L"unknown load order";
    }
    return L"unknown load order";
}

OverlapDepth ConvertStringToOverlapDepth(const std::wstring& str)
{
    if (str.compare(L"depth_1") == 0)
        return depth_1;
    else if (str.compare(L"depth_2") == 0)
        return depth_2;
    else if (str.compare(L"depth_4") == 0)
        return depth_4;
    else if (str.compare(L"depth_8") == 0)
        return depth_8;
    else if (str.compare(L"depth_12") == 0)
        return depth_12;
    else if (str.compare(L"depth_16") == 0)
        return depth_16;
    else if (str.compare(L"depth_24") == 0)
        return depth_24;
    else if (str.compare(L"depth_32") == 0)
        return depth_32;
    else if (str.compare(L"depth_64") == 0)
        return depth_64;
    else if (str.compare(L"depth_128") == 0)
        return depth_128;
    else if (str.compare(L"depth_256") == 0)
        return depth_256;
    else if (str.compare(L"depth_512") == 0)
        return depth_512;
    else if (str.compare(L"depth_768") == 0)
        return depth_768;
    else if (str.compare(L"depth_1024") == 0)
        return depth_1024;
    else if (str.compare(L"depth_2048") == 0)
        return depth_2048;
    else if (str.compare(L"depth_4096") == 0)
        return depth_4096;
    return depth_1;
}

uint32_t ConvertOverlapDepthToValue(OverlapDepth overlapDepth)
{
    switch (overlapDepth)
    {
    case OverlapDepth::depth_1:
        return 1;
    case OverlapDepth::depth_2:
        return 2;
    case OverlapDepth::depth_4:
        return 4;
    case OverlapDepth::depth_8:
        return 8;
    case OverlapDepth::depth_12:
        return 12;
    case OverlapDepth::depth_16:
        return 16;
    case OverlapDepth::depth_24:
        return 24;
    case OverlapDepth::depth_32:
        return 32;
    case OverlapDepth::depth_64:
        return 64;
    case OverlapDepth::depth_128:
        return 128;
    case OverlapDepth::depth_256:
        return 256;
    case OverlapDepth::depth_512:
        return 512;
    case OverlapDepth::depth_768:
        return 768;
    case OverlapDepth::depth_1024:
        return 1024;
    case OverlapDepth::depth_2048:
        return 2048;
    case OverlapDepth::depth_4096:
        return 4096;
    case OverlapDepth::LAST_OVERLAP_DEPTH:
        return 1;
    }
    return 1;
}

LoadOrder ConvertStringToLoadOrder(const std::wstring& str)
{
    if (str.compare(L"true_sequential") == 0)
        return TrueSequential;
    else if (str.compare(L"random_sequential") == 0)
        return RandomSequential;
    else if (str.compare(L"random") == 0)
        return Random;
    else if (str.compare(L"backwards") == 0)
        return Backwards;
    else if (str.compare(L"redundant") == 0)
        return Redundant;
    return TrueSequential;
}

DataSize ConvertStringToDataSize(const std::wstring& str)
{
    if (str.compare(L"size_8k") == 0)
        return size_8k;
    else if (str.compare(L"size_12k") == 0)
        return size_12k;
    else if (str.compare(L"size_16k") == 0)
        return size_16k;
    else if (str.compare(L"size_32k") == 0)
        return size_32k;
    else if (str.compare(L"size_64k") == 0)
        return size_64k;
    else if (str.compare(L"size_128k") == 0)
        return size_128k;
    else if (str.compare(L"size_192k") == 0)
        return size_192k;
    else if (str.compare(L"size_256k") == 0)
        return size_256k;
    else if (str.compare(L"size_512k") == 0)
        return size_512k;
    else if (str.compare(L"size_1024k") == 0)
        return size_1024k;
    else if (str.compare(L"size_2048k") == 0)
        return size_2048k;
    else if (str.compare(L"size_4096k") == 0)
        return size_4096k;
    else if (str.compare(L"size_8192k") == 0)
        return size_8192k;
    else if (str.compare(L"size_16384k") == 0)
        return size_16384k;
    else if (str.compare(L"size_32768k") == 0)
        return size_32768k;
    return size_8k;
}

std::wstring ConvertOverlapDepthToString(OverlapDepth overlapDepth)
{
    switch (overlapDepth)
    {
    case OverlapDepth::depth_1:
        return L"d1 ";
    case OverlapDepth::depth_2:
        return L"d2 ";
    case OverlapDepth::depth_4:
        return L"d4 ";
    case OverlapDepth::depth_8:
        return L"d8 ";
    case OverlapDepth::depth_12:
        return L"d12 ";
    case OverlapDepth::depth_16:
        return L"d16 ";
    case OverlapDepth::depth_24:
        return L"d24 ";
    case OverlapDepth::depth_32:
        return L"d32 ";
    case OverlapDepth::depth_64:
        return L"d64 ";
    case OverlapDepth::depth_128:
        return L"d128 ";
    case OverlapDepth::depth_256:
        return L"d256 ";
    case OverlapDepth::depth_512:
        return L"d512 ";
    case OverlapDepth::depth_768:
        return L"d768 ";
    case OverlapDepth::depth_1024:
        return L"d1024 ";
    case OverlapDepth::depth_2048:
        return L"d2048 ";
    case OverlapDepth::depth_4096:
        return L"d4096 ";
    case OverlapDepth::LAST_OVERLAP_DEPTH:
        return L"unknown overlap depth";
    }
    return L"unknown overlap depth";
}
