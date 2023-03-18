//--------------------------------------------------------------------------------------
// AdvancedExceptionHandling.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AdvancedExceptionHandling.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "ControllerFont.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

std::wstring Sample::m_testOutputMessage;
bool Sample::s_isSuspending = false;

void GenerateDump(EXCEPTION_POINTERS* exceptionPointers)
{
    static uint32_t callCount = 0;

    Sample::m_testOutputMessage += L"Saving a crash dump with MiniDumpWriteDump\n";

    wchar_t buffer[128];
    BOOL miniDumpSuccessful;
    HANDLE dumpFile;
    MINIDUMP_EXCEPTION_INFORMATION expParam;
    // Saving the minidump to the system scratch drive to allow an easy copy back to a development PC even if the title is not running
    swprintf(buffer, 128, L"d:\\AdvancedExceptionHandling_%d.dmp", callCount++);

    dumpFile = CreateFileW(buffer, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
    if (dumpFile == INVALID_HANDLE_VALUE)
    {
        Sample::m_testOutputMessage += L"Failed to create dump file\n";
        return;
    }

    expParam.ThreadId = GetCurrentThreadId();
    expParam.ExceptionPointers = exceptionPointers;
    expParam.ClientPointers = NULL;

    MINIDUMP_TYPE dumpType = MiniDumpNormal;
    miniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFile, dumpType, &expParam, NULL, NULL);
    if (!miniDumpSuccessful)
    {
        Sample::m_testOutputMessage += L"Failed to save the crash dump\n";
    }

    CloseHandle(dumpFile);
}

Sample::Sample() noexcept(false) :
    m_lastTestRun(e_null)
    , m_frame(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
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

    auto pad = m_gamePad->GetState(GamePad::c_MostRecent);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }
        if (pad.IsAPressed())
        {
            FullExceptionSystem();
        }
        if (pad.IsBPressed())
        {
            OutOfProcHandler();
        }
        if (pad.IsXPressed())
        {
            AddingDataToWER();
        }
        if (pad.IsYPressed())
        {
            UploadingMiniDumps();
        }
        if (pad.IsLeftShoulderPressed())
        {
            PLMExceptionHandling();
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

    if (m_keyboardButtons.pressed.A)
    {
        FullExceptionSystem();
    }
    if (m_keyboardButtons.pressed.B)
    {
        OutOfProcHandler();
    }
    if (m_keyboardButtons.pressed.X)
    {
        AddingDataToWER();
    }
    if (m_keyboardButtons.pressed.Y)
    {
        UploadingMiniDumps();
    }
    if (m_keyboardButtons.pressed.L)
    {
        PLMExceptionHandling();
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

    RECT safeRect = SimpleMath::Viewport::ComputeTitleSafeArea(1920, 1080);
    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    ID3D12DescriptorHeap* pHeaps[] = { m_resourceDescriptors->Heap() };
    commandList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), GetTextureSize(m_background.Get()), XMFLOAT2(0, 0));

    if (m_timer.GetFrameCount() > 3)
    {
        m_largeFont->DrawString(m_spriteBatch.get(), L"AdvancedExceptionHandling", pos);
        pos.y += (m_largeFont->GetLineSpacing() * 1.1f);
        pos.x += XMVectorGetX(m_largeFont->MeasureString(L"XXX"));

        DrawHelpText(pos, e_fullExceptionSystem, L"[A]");
        pos.y += m_regularFont->GetLineSpacing() * 1.1f;

        DrawHelpText(pos, e_outOfProcHandler, L"[B]");
        pos.y += m_regularFont->GetLineSpacing() * 1.1f;

        DrawHelpText(pos, e_addingDataToWER, L"[X]");
        pos.y += m_regularFont->GetLineSpacing() * 1.1f;

        DrawHelpText(pos, e_uploadingMiniDumps, L"[Y]");
        pos.y += m_regularFont->GetLineSpacing() * 1.1f;

        DrawHelpText(pos, e_plmExceptionHandling, L"[LB]");
        pos.y += m_regularFont->GetLineSpacing() * 1.1f;

        switch (m_lastTestRun)
        {
        case e_fullExceptionSystem:
            m_largeFont->DrawString(m_spriteBatch.get(), L"Output from Full Exception System example", pos);
            break;
        case e_outOfProcHandler:
            m_largeFont->DrawString(m_spriteBatch.get(), L"Output from using a second process to create crash dumps example", pos);
            break;
        case e_addingDataToWER:
            m_largeFont->DrawString(m_spriteBatch.get(), L"Output from the adding data for Windows Error Reporting collection example", pos);
            break;
        case e_uploadingMiniDumps:
            m_largeFont->DrawString(m_spriteBatch.get(), L"Output from uploading crash dumps example", pos);
            break;
        case e_plmExceptionHandling:
            m_largeFont->DrawString(m_spriteBatch.get(), L"Output from PLM exception handling example", pos);
            break;
        case e_null:
            m_largeFont->DrawString(m_spriteBatch.get(), L"No example run yet", pos);
            break;
        }
        pos.y += (m_largeFont->GetLineSpacing() * 1.1f);
        pos.x += XMVectorGetX(m_regularFont->MeasureString(L"XXX"));
        m_regularFont->DrawString(m_spriteBatch.get(), m_testOutputMessage.c_str(), pos);
    }

    m_spriteBatch->End();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::DrawHelpText(DirectX::XMFLOAT2& pos, ExceptionTest whichTest, const std::wstring& button)
{
    static const wchar_t *helpText[] = {
        L"Full Exception System",
        L"    Putting all the pieces together into a full exception system.",
        L"Using a separate process to save crash dumps",
        L"    Using a separate process to create crash dumps, this is the recommended pattern for creating crash dumps.",
        L"Adding custom data to a Windows Error Reporting crash dump",
        L"    Shows how to add data to the Windows Error Reporting system that is uploaded along with crash dumps to the Microsoft servers for later analysis.",
        L"Uploading Crashing Dumps to a studio server",
        L"    How to upload crash dumps to your own servers so they don't interfere with title execution and possibly cause more exceptions.",
        L"Handling exceptions during suspend/resume (PLM)",
        L"    How to handle exceptions that happen during the PLM Suspend/Resume path.",
    };
    int32_t startIndex(0);
    startIndex = whichTest * 2;
    std::wstring outputString;
    outputString = L"Press ";
    outputString += button;
    outputString += L" to run the ";
    outputString += helpText[startIndex];

    DX::DrawControllerString(m_spriteBatch.get(), m_regularFont.get(), m_ctrlFont.get(), outputString.c_str(), pos);

    pos.y += m_regularFont->GetLineSpacing() * 1.1f;
    m_regularFont->DrawString(m_spriteBatch.get(), helpText[1 + startIndex], pos);
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
void Sample::OnSuspending()
{
    s_isSuspending = true;
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    s_isSuspending = false;
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
#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
        throw std::exception("Shader Model 6.0 is not supported!");
    }
#endif
    wchar_t strFilePath[MAX_PATH] = {};

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

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

    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_24.spritefont");
        m_largeFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_resourceDescriptors->GetCpuHandle(Descriptors::LargeFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::LargeFont));
    }

    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneController.spritefont");
        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_resourceDescriptors->GetCpuHandle(Descriptors::CtrlFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::CtrlFont));
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
    m_resourceDescriptors.reset();
    m_spriteBatch.reset();
    m_background.Reset();
    m_regularFont.reset();
    m_largeFont.reset();
    m_ctrlFont.reset();
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
