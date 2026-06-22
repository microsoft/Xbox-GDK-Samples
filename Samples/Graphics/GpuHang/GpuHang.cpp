//--------------------------------------------------------------------------------------
// GpuHang.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GpuHang.h"

#include "ATGColors.h"
#include "ControllerFont.h"

#include "Util.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Sample::Sample() :
    m_frame(0), 
    m_scrollY(0.f),
    m_hang(false),
    m_takeCapture(false),
    m_queueType(IHang::QueueType::QueueTypeCount),
    m_hangAction(IHang::HangAction::HangActionCount),
    m_selectedHangIndex(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->SetClearColor(ATG::Colors::Background);
}

void Sample::ParseCommandLine(const wchar_t* commandLine)
{
    wchar_t commandLineCopy[1024] = L"";
    wcscpy_s(commandLineCopy, _countof(commandLineCopy), commandLine);
    wchar_t* context = commandLineCopy;
    while (auto token = wcstok_s(context, L" \t", &context))
    {
        if (0 == _wcsicmp(L"hang", token))
        {
            m_hang = true;
        }
        else
        {
            // Might be the name of a hang
            for (auto hangIndex = 0U; hangIndex < Hang::HangList().size(); ++hangIndex)
            {
                if (0 == _wcsicmp(Hang::HangList()[hangIndex]->GetFileName(), token))
                {
                    m_selectedHangIndex = hangIndex;
                }
            }

            // Might be the name of a queue
            for (auto queueType = 0U; queueType < IHang::QueueType::QueueTypeCount; ++queueType)
            {
                if (0 == _wcsicmp(IHang::QueueNames[queueType], token))
                {
                    m_queueType = queueType;
                }
            }

            // Might be the name of an action 
            for (auto hangAction = 0U; hangAction < IHang::HangAction::HangActionCount; ++hangAction)
            {
                if (0 == _wcsicmp(IHang::HangActionNames[hangAction], token))
                {
                    m_hangAction = hangAction;
                }
            }
        }
    }

    // Check for valid queue
    if (IHang::QueueType::QueueTypeCount != m_queueType)
    {
        auto hangs = Hang::HangList();
        auto hang = hangs[m_selectedHangIndex];

        if (!hang->SetQueue(Hang::QueueType(m_queueType)))
        {
            m_queueType = IHang::QueueType::QueueTypeCount;
        }
    }

    // Check for valid hang action
    if (IHang::HangAction::HangActionCount != m_hangAction)
    {
        auto hangs = Hang::HangList();
        auto hang = hangs[m_selectedHangIndex];

        if (!hang->SetHangAction(Hang::HangAction(m_hangAction)))
        {
            m_hangAction = IHang::HangAction::HangActionCount;
        }
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
void Sample::Update(DX::StepTimer const& /*timer*/)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    auto hangs = Hang::HangList();
    auto size = hangs.size();

    auto GoToNextSupportedQueue = [this, hangs]()->void
    {
        auto hang = hangs[m_selectedHangIndex];
        while (!hang->SetQueue(Hang::QueueType(m_queueType)))
        {
            m_queueType = ++m_queueType % IHang::QueueType::QueueTypeCount;
        }
    };

    auto GoToNextSupportedHangAction = [this, hangs]()->void
    {
        auto hang = hangs[m_selectedHangIndex];
        while (!hang->SetHangAction(Hang::HangAction(m_hangAction)))
        {
            m_hangAction = ++m_hangAction % IHang::HangAction::HangActionCount;
        }
    };

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
                m_selectedHangIndex = ++m_selectedHangIndex % size;
                m_scrollY = 0.0f;

                GoToNextSupportedQueue();
                GoToNextSupportedHangAction();
            }

            if (gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
            {
                m_selectedHangIndex = (m_selectedHangIndex + uint32_t(size) - 1) % size;
                m_scrollY = 0.0f;

                GoToNextSupportedQueue();
                GoToNextSupportedHangAction();
            }

            if (gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
            {
                m_hang = true;
            }

            if (gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
            {
                m_takeCapture = true;
            }

            if (gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
            {
                m_queueType = ++m_queueType % IHang::QueueType::QueueTypeCount;
                GoToNextSupportedQueue();
            }

            if (gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
            {
                m_hangAction = ++m_hangAction % IHang::HangAction::HangActionCount;
                GoToNextSupportedHangAction();
            }
        }
    }

    if (IHang::QueueType::QueueTypeCount == m_queueType)
    {
        m_queueType = IHang::QueueType::Graphics;

        GoToNextSupportedQueue();
    }

    if (IHang::HangAction::HangActionCount == m_hangAction)
    {
        m_hangAction = IHang::HangAction::Dump;

        GoToNextSupportedHangAction();
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

    auto device = m_deviceResources->GetD3DDevice();
    auto hang = Hang::HangList()[m_selectedHangIndex];

    m_dumpFileName = DumpFileName();

    auto commandQueue = m_deviceResources->GetCommandQueue();

    hang->Initialize(device, commandQueue);

    if (m_takeCapture)
    {
        BeginCapture(commandQueue);
    }

    hang->Render(device, commandQueue, m_hang);
    hang->Check(device, commandQueue);

    if (m_takeCapture)
    {
        EndCapture(commandQueue);
    }

    hang->Uninitialize(device);

    m_hang = false;
    m_takeCapture = false;

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginRetailEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    RenderUI();

    PIXEndRetailEvent(commandList);

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
    PIXBeginRetailEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

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

    PIXEndRetailEvent(commandList);
}

void Sample::RenderUI()
{
    auto hangs = Hang::HangList();

    auto commandList = m_deviceResources->GetCommandList();

    auto const viewport = m_deviceResources->GetScreenViewport();

    constexpr float yInc = 40.0f;

    DirectX::FXMVECTOR queueColors[IHang::QueueType::QueueTypeCount] =
    {
        DirectX::Colors::LightGreen,
        DirectX::Colors::Orange,
        DirectX::Colors::LightPink,
    };

    DirectX::FXMVECTOR hangActionColors[IHang::HangAction::HangActionCount] =
    {
        DirectX::Colors::Yellow,
#ifdef LIVE_DEBUGGING_SUPPORT
        DirectX::Colors::Cyan,
        DirectX::Colors::Red,
#endif
    };
    static_assert(_countof(hangActionColors) == IHang::HangAction::HangActionCount, "Array size mismatch");

    // Draw instructions
    DirectX::SimpleMath::Vector2 instructionsPos(60.0f, 20.0f);

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->SetViewport(viewport);

    DX::DrawControllerString(m_spriteBatch.get(), m_fontInstructions.get(), m_fontController.get(), L"Press [A] to provoke the selected hang (may require multiple attempts, application may terminate, and console may reboot)", instructionsPos);
    instructionsPos.y += yInc;
    DX::DrawControllerString(m_spriteBatch.get(), m_fontInstructions.get(), m_fontController.get(), L"Press [X] to take a PIX capture of something similar to the hang", instructionsPos);
    instructionsPos.y += yInc;
    auto linePos = instructionsPos;
    auto NextText = [this, &linePos] (wchar_t const* text, DirectX::FXMVECTOR color = DirectX::Colors::White)->void
    {
        DX::DrawControllerString(m_spriteBatch.get(), m_fontInstructions.get(), m_fontController.get(), text, linePos, color);
        auto rect = DX::MeasureControllerDrawBounds(m_fontInstructions.get(), m_fontController.get(), text, linePos);
        linePos.x += rect.right - rect.left;
    };
    NextText(L"Press [B] to change among {");
    NextText(L"graphics", queueColors[IHang::QueueType::Graphics]);
    NextText(L"|");
    NextText(L"async", queueColors[IHang::QueueType::Async]);
    NextText(L"|");
    NextText(L"dma", queueColors[IHang::QueueType::Dma]);
    NextText(L"} hang for cases which support it\n");
    instructionsPos.y += yInc;
    linePos = instructionsPos;

#ifdef LIVE_DEBUGGING_SUPPORT
    NextText(L"Press [Y] to change between {");
    NextText(L"dump", hangActionColors[IHang::HangAction::Dump]);
    NextText(L"|");
    NextText(L"live-debug", hangActionColors[IHang::HangAction::LiveDebug]);
    NextText(L"|");
    NextText(L"live-debug-then-dump", hangActionColors[IHang::HangAction::LiveDebugThenDump]);
    NextText(L"} hang for cases which support it\n");
    instructionsPos.y += yInc;
    linePos = instructionsPos;
#endif

    m_spriteBatch->End();

    DirectX::SimpleMath::Vector2 listPos(instructionsPos);
    listPos.y += yInc;

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->SetViewport(viewport);

    for (auto hang : hangs)
    {
        auto color = hang == SelectedHang() 
            ? queueColors[m_queueType]
            : DirectX::Colors::LightGray;
        m_fontHangList->DrawString(m_spriteBatch.get(), hang->GetName(), listPos, color);

        if (hang == SelectedHang())
        {
            // Flashing cursor --- so we can tell when we're hung
            float cursorX = 0.0f;
            DirectX::SimpleMath::Vector2 cursorPos(cursorX, listPos.y);
            auto seconds = (float)m_timer.GetTotalSeconds();
            auto lerpFactor = (sinf(XM_2PI * seconds) + 1.0f) / 2.0f;
            auto cursorColor = XMVectorLerp(hangActionColors[m_hangAction], DirectX::Colors::Black, lerpFactor);
            m_fontHangList->DrawString(m_spriteBatch.get(), L">>>", cursorPos, cursorColor);
        }

        listPos.y += yInc;
    }

    m_spriteBatch->End();

    // Draw description
    m_spriteBatch->Begin(commandList);

    m_spriteBatch->SetViewport(viewport);

    DirectX::SimpleMath::Vector2 descriptionPos(instructionsPos);
    descriptionPos.x += 400.0f;
    descriptionPos.y += yInc;
    auto descriptionColor = DirectX::Colors::LightGray;

    for (auto line : SelectedHang()->GetDescription())
    {
        m_fontDescription->DrawString(m_spriteBatch.get(), line, descriptionPos, descriptionColor);

        descriptionPos.y += yInc;
    }

    m_spriteBatch->End();

}
#pragma endregion

#pragma region PIX capture support
std::wstring Sample::DumpFileName() const
{
    static std::wstring platformName;
    if (platformName.empty())
    {
        auto deviceType = XSystemGetDeviceType();
        switch (deviceType)
        {
        case XSystemDeviceType::Unknown:               platformName = L"Unknown";               break;
        case XSystemDeviceType::Pc:                    platformName = L"Pc";                    break;
        case XSystemDeviceType::XboxOne:               platformName = L"XboxOne";               break;
        case XSystemDeviceType::XboxOneS:              platformName = L"XboxOneS";              break;
        case XSystemDeviceType::XboxOneX:              platformName = L"XboxOneX";              break;
        case XSystemDeviceType::XboxOneXDevkit:        platformName = L"XboxOneXDevkit";        break;
        case XSystemDeviceType::XboxScarlettLockhart:  platformName = L"XboxSeriesS";           break;
        case XSystemDeviceType::XboxScarlettAnaconda:  platformName = L"XboxSeriesX";           break;
        case XSystemDeviceType::XboxScarlettDevkit:    platformName = L"XboxSeriesXDevkit";     break;
        default:                                       platformName = L"Unexpected";            break;
        }
    }

    auto queueName = IHang::QueueNames[SelectedHang()->GetQueue()];

    std::wostringstream dumpFileName;
    dumpFileName << L"d:\\";
    dumpFileName << SelectedHang()->GetFileName();
    if (IHang::Graphics != SelectedHang()->GetQueue())
    {
        dumpFileName << L"-";
        dumpFileName << queueName;
    }
    dumpFileName << L"-";
    dumpFileName << platformName;
#ifdef _GAMING_XBOX_SCARLETT
    dumpFileName << L".hix";
#else
    dumpFileName << L".xhit";
#endif

    return dumpFileName.str();
}

std::wstring Sample::CaptureFileName() const
{
    static std::wstring platformName;
    if (platformName.empty())
    {
        auto deviceType = XSystemGetDeviceType();
        switch (deviceType)
        {
        case XSystemDeviceType::Unknown:               platformName = L"Unknown";               break;
        case XSystemDeviceType::Pc:                    platformName = L"Pc";                    break;
        case XSystemDeviceType::XboxOne:               platformName = L"XboxOne";               break;
        case XSystemDeviceType::XboxOneS:              platformName = L"XboxOneS";              break;
        case XSystemDeviceType::XboxOneX:              platformName = L"XboxOneX";              break;
        case XSystemDeviceType::XboxOneXDevkit:        platformName = L"XboxOneXDevkit";        break;
        case XSystemDeviceType::XboxScarlettLockhart:  platformName = L"XboxSeriesS";           break;
        case XSystemDeviceType::XboxScarlettAnaconda:  platformName = L"XboxSeriesX";           break;
        case XSystemDeviceType::XboxScarlettDevkit:    platformName = L"XboxSeriesXDevkit";     break;
        default:                                       platformName = L"Unexpected";            break;
        }
    }

    auto queueName = IHang::QueueNames[SelectedHang()->GetQueue()];

    std::wostringstream captureFileName;
    captureFileName << L"d:\\";
    captureFileName << SelectedHang()->GetFileName();
    if (IHang::Graphics != SelectedHang()->GetQueue())
    {
        captureFileName << L"-";
        captureFileName << queueName;
    }
    captureFileName << L"-";
    captureFileName << platformName;
    captureFileName << L".xpix";

    return captureFileName.str();
}

void Sample::BeginCapture(ID3D12CommandQueue* commandQueue) const
{
    auto captureName = CaptureFileName();

    std::wostringstream message;
    message << L"Starting PIX capture ";
    message << captureName;
    message << std::endl;
    OutputDebugString(message.str().c_str());

    auto hr = commandQueue->PIXGpuBeginCapture(D3D12XBOX_PIX_CAPTURE_API | D3D12XBOX_PIX_CAPTURE_ALL_ENGINES | D3D12XBOX_PIX_CAPTURE_ALL_MEMORY, captureName.c_str());
    if (!SUCCEEDED(hr))
    {
        throw std::exception("Failure in PIXGpuBeginCapture.\n");
    }
}

void Sample::EndCapture(ID3D12CommandQueue* commandQueue) const
{
    auto captureName = CaptureFileName();

    std::wostringstream message;
    message << L"Ending PIX capture ";
    message << captureName;
    message << std::endl;
    OutputDebugString(message.str().c_str());

    auto hr = commandQueue->PIXGpuEndCapture();
    if (!SUCCEEDED(hr))
    {
        throw std::exception("Failure in PIXGpuEndCapture.\n");
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
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

#ifdef HIX_EXCEPTION_SUPPORT
    // Break on shader exceptions (may take a couple of frames to take effect)
    ID3D12Device11* device11 = nullptr;
    DX::ThrowIfFailed(device->QueryInterface(IID_GRAPHICS_PPV_ARGS(&device11)));

    D3D12XBOX_LIVE_DEBUGGING_GLOBAL_PARAMETERS liveDebuggingParametersGlobal = {};
    liveDebuggingParametersGlobal.Type = D3D12XBOX_LIVE_DEBUGGING_SHADER_STAGE_ANY;
    liveDebuggingParametersGlobal.Flags = D3D12XBOX_LIVE_DEBUGGING_SHADER_GLOBAL_FLAGS_ALL_EXCEPTIONS;
    device11->EnableLiveDebuggingX(&liveDebuggingParametersGlobal);
#endif

#ifdef HIX_BLOB_SUPPORT
    // We need a global copy of the D3D12 device for use in the callbacks
    static ID3D12Device8* g_deviceForHangCallbacks = nullptr;
    device->QueryInterface(IID_GRAPHICS_PPV_ARGS(&g_deviceForHangCallbacks));

    D3D12XBOX_HANGBEGINCALLBACK BeginCallback = [] (UINT64 /*Flags*/) -> UINT
    {
        // Demonstrate adding title-specific data to .Hix file
        ULONGLONG allocationAttributes = MAKE_XALLOC_ATTRIBUTES(
            eXALLOCAllocatorId_GameMin,
            0U,                                 // ObjectType --- title-defined
            XALLOC_MEMTYPE_HEAP_CACHEABLE,
            XALLOC_PAGESIZE_4KB,
            XALLOC_ALIGNMENT_ANY,
            0);

        UINT memSizeBytes = 1024U;
        void* memory = XMemAlloc(memSizeBytes, allocationAttributes);

        // Set some bits
        memset(memory, 0xba, memSizeBytes);
        const UINT64 GPU_HANG_SAMPLE_BLOB1 = 'HSB1';
        g_deviceForHangCallbacks->AddBlobToGpuHangDumpX((D3D12_GPU_VIRTUAL_ADDRESS)memory, memSizeBytes, GPU_HANG_SAMPLE_BLOB1);

        XMemFree(memory, allocationAttributes);

        memSizeBytes = 64;
        memory = XMemAlloc(memSizeBytes, allocationAttributes);
        // Set some bits
        memset(memory, 0xcd, memSizeBytes);
        const UINT64 GPU_HANG_SAMPLE_BLOB2 = 'HSB2';
        g_deviceForHangCallbacks->AddBlobToGpuHangDumpX((D3D12_GPU_VIRTUAL_ADDRESS)memory, memSizeBytes, GPU_HANG_SAMPLE_BLOB2);

        XMemFree(memory, allocationAttributes);

        return 1;
    };
#else
    D3D12XBOX_HANGBEGINCALLBACK BeginCallback = [] (UINT64 /*Flags*/) -> UINT
    {
        return 1;
    };
#endif

    D3D12XBOX_HANGPRINTCALLBACK PrintCallback = [] (const CHAR* /*strLine*/) -> void 
    {
    };

    D3D12XBOX_HANGDUMPCALLBACK DumpCallback = [] (const WCHAR* strFileName) -> void 
    {
        std::wostringstream message;
        message << L"Moving GPU hang dump from ";
        message << strFileName;
        message << L" to ";
        message << m_dumpFileName;
        message << std::endl;
        OutputDebugString(message.str().c_str());

        MoveFileEx(strFileName, m_dumpFileName.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED);
    };

    device->SetHangCallbacksX(BeginCallback, PrintCallback, DumpCallback);

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptorHeap = std::make_unique<DescriptorHeap>(device, ResourceDescriptors::Count);

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    const RenderTargetState renderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pipelineDescription(renderTargetState);
    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pipelineDescription);

    m_fontHangList = std::make_unique<SpriteFont>(device, 
        resourceUpload, 
        L"SegoeUI_18.spritefont", 
        m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::FontHangList), 
        m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::FontHangList));
    m_fontInstructions = std::make_unique<SpriteFont>(device, 
        resourceUpload, 
        L"Courier_16.spritefont", 
        m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::FontInstructions), 
        m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::FontInstructions));
    m_fontController = std::make_unique<SpriteFont>(device, 
        resourceUpload, 
        L"XboxOneControllerSmall.spritefont", 
        m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::FontController), 
        m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::FontController));
    m_fontDescription = std::make_unique<SpriteFont>(device, 
        resourceUpload, 
        L"SegoeUI_18_Italic.spritefont", 
        m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::FontDescription), 
        m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::FontDescription));

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());   
    uploadResourcesFinished.wait();     // Wait for resources to upload
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
}
#pragma endregion
