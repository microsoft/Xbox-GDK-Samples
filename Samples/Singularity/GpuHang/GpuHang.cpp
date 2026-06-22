//--------------------------------------------------------------------------------------
// GpuHang.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GpuHang.h"

#include "ATGColors.h"
#include "FindMedia.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false):
    m_frame(0),
    m_scrollY(0.f),
    m_hang(false),
    m_takeCapture(false),
    m_queueType(IHang::QueueType::QueueTypeCount),
    m_hangAction(IHang::HangAction::HangActionCount),
    m_selectedHangIndex(0)
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
void Sample::Initialize(HWND window, int width, int height)
{
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
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    m_mouse->EndOfInputFrame();

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

    {
        auto kb = m_keyboard->GetState();
        m_keyboardButtons.Update(kb);

        if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Escape))
        {
            ExitSample();
        }

        if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Down))
        {
            m_selectedHangIndex = ++m_selectedHangIndex % size;
            m_scrollY = 0.0f;

            GoToNextSupportedQueue();
            GoToNextSupportedHangAction();
        }

        if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Up))
        {
            m_selectedHangIndex = (m_selectedHangIndex + uint32_t(size) - 1) % size;
            m_scrollY = 0.0f;

            GoToNextSupportedQueue();
            GoToNextSupportedHangAction();
        }

        if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::A))
        {
            m_hang = true;
        }

        if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::X))
        {
            m_takeCapture = true;
        }

        if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::B))
        {
            m_queueType = ++m_queueType % IHang::QueueType::QueueTypeCount;
            GoToNextSupportedQueue();
        }

        if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Y))
        {
            m_hangAction = ++m_hangAction % IHang::HangAction::HangActionCount;
            GoToNextSupportedHangAction();
        }

    }

    auto mouse = m_mouse->GetState();
    mouse;

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
    {
        // Don't try to render anything before the first Update.
        if (m_timer.GetFrameCount() == 0)
        {
            return;
        }

        auto device = m_deviceResources->GetD3DDevice();
        auto hang = Hang::HangList()[m_selectedHangIndex];

        auto commandQueue = m_deviceResources->GetCommandQueue();

        hang->Initialize(device, commandQueue);

        if (m_takeCapture)
        {
            BeginCapture();
        }

        hang->Render(device, commandQueue, m_deviceResources->GetVendorId(), m_hang);
        hang->Check(device, commandQueue);

        if (m_takeCapture)
        {
            EndCapture();
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

void Sample::RenderUI()
{
    auto hangs = Hang::HangList();

    auto commandList = m_deviceResources->GetCommandList();

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_resourceDescriptorHeap->Heap() };

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

    // Draw title
    DirectX::SimpleMath::Vector2 titlePos(60.0f, 20.0f);

    m_spriteBatch->Begin(commandList);

    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    m_spriteBatch->SetViewport(viewport);

    m_fontTitle->DrawString(m_spriteBatch.get(), L"GpuHang sample", titlePos);

    m_spriteBatch->End();

    // Draw instructions
    DirectX::SimpleMath::Vector2 instructionsPos = titlePos;
    instructionsPos.y += yInc;
    instructionsPos.y += yInc;
    instructionsPos.y += yInc;

    m_spriteBatch->Begin(commandList);

    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    m_spriteBatch->SetViewport(viewport);

    DX::DrawControllerString(m_spriteBatch.get(), m_fontInstructions.get(), m_fontController.get(), L"Press [A] (or 'a' key) to provoke the selected hang (application may terminate, and PC may TDR)", instructionsPos);
    instructionsPos.y += yInc;
    DX::DrawControllerString(m_spriteBatch.get(), m_fontInstructions.get(), m_fontController.get(), L"Press [X] (or 'x' key) to take a PIX capture of something similar to the hang", instructionsPos);
    instructionsPos.y += yInc;
    auto linePos = instructionsPos;
    auto NextText = [this, &linePos] (wchar_t const* text, DirectX::FXMVECTOR color = DirectX::Colors::White)->void
    {
        DX::DrawControllerString(m_spriteBatch.get(), m_fontInstructions.get(), m_fontController.get(), text, linePos, color);
        auto rect = DX::MeasureControllerDrawBounds(m_fontInstructions.get(), m_fontController.get(), text, linePos);
        linePos.x += rect.right - rect.left;
    };
    NextText(L"Press [B] (or 'b' key) to change among {");
    NextText(L"graphics", queueColors[IHang::QueueType::Graphics]);
    NextText(L"|");
    NextText(L"async", queueColors[IHang::QueueType::Async]);
    NextText(L"|");
    NextText(L"copy", queueColors[IHang::QueueType::Copy]);
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

    // Draw list of hang cases
    DirectX::SimpleMath::Vector2 listPos(instructionsPos);
    listPos.y += yInc;

    m_spriteBatch->Begin(commandList);

    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

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

    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

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
std::wstring Sample::CaptureFileName() const
{
    static std::wstring platformName = L"Pc";

    auto queueName = IHang::QueueNames[SelectedHang()->GetQueue()];

    // Default to working directory
    std::wostringstream captureFileName;
    captureFileName << SelectedHang()->GetFileName();
    if (IHang::Graphics != SelectedHang()->GetQueue())
    {
        captureFileName << L"-";
        captureFileName << queueName;
    }
    captureFileName << L"-";
    captureFileName << platformName;
    captureFileName << L".wpix";

    return captureFileName.str();
}

void Sample::BeginCapture() const
{
    auto captureName = CaptureFileName();

    std::wostringstream message;
    message << L"Starting PIX capture ";
    message << captureName;
    message << std::endl;
    OutputDebugString(message.str().c_str());

    PIXCaptureParameters parameters = {};
    parameters.GpuCaptureParameters.FileName = captureName.c_str();
    auto hr = PIXBeginCapture(PIX_CAPTURE_GPU, &parameters);
    if (!SUCCEEDED(hr))
    {
        throw std::exception("Failure in PIXGpuBeginCapture.\n");
    }
}

void Sample::EndCapture() const
{
    auto captureName = CaptureFileName();

    std::wostringstream message;
    message << L"Ending PIX capture ";
    message << captureName;
    message << std::endl;
    OutputDebugString(message.str().c_str());

    auto hr = PIXEndCapture(false);
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
}

void Sample::OnResuming()
{
    m_timer.ResetElapsedTime();
    for (int player = 0; player < GamePad::MAX_PLAYER_COUNT; ++player)
    {
        auto& gamePadButtons = m_gamePadButtons[player];

        gamePadButtons.Reset();
    }
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
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptorHeap = std::make_unique<DescriptorHeap>(device, ResourceDescriptors::Count);

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    const RenderTargetState renderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pipelineDescription(renderTargetState);
    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pipelineDescription);

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_48.spritefont");
    m_fontTitle = std::make_unique<SpriteFont>(device,
        resourceUpload,
        strFilePath,
        m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::FontTitle),
        m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::FontTitle));
    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    m_fontHangList = std::make_unique<SpriteFont>(device,
        resourceUpload, 
        strFilePath,
        m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::FontHangList),
        m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::FontHangList));
    DX::FindMediaFile(strFilePath, MAX_PATH, L"Courier_16.spritefont");
    m_fontInstructions = std::make_unique<SpriteFont>(device,
        resourceUpload, 
        strFilePath,
        m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::FontInstructions),
        m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::FontInstructions));
    DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerSmall.spritefont");
    m_fontController = std::make_unique<SpriteFont>(device,
        resourceUpload, 
        strFilePath,
        m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::FontController),
        m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::FontController));
    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18_Italic.spritefont");
    m_fontDescription = std::make_unique<SpriteFont>(device,
        resourceUpload, 
        strFilePath,
        m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::FontDescription),
        m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::FontDescription));

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());   
    uploadResourcesFinished.wait();     // Wait for resources to upload
}
// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Render all UI at 1080p so that it's easy to switch between 4K/1080p
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920;
    viewportUI.Height = 1080;
    m_spriteBatch->SetViewport(viewportUI);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_deviceResources.reset();
    m_graphicsMemory.reset();
    m_resourceDescriptorHeap.reset();
    m_spriteBatch.reset();
    m_fontTitle.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
