//--------------------------------------------------------------------------------------
// DataBreakPoints.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DataBreakPoints.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "ControllerFont.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
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
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (m_gamePadButtons.a == DirectX::GamePad::ButtonStateTracker::PRESSED)
        {
            m_executionTestResult = m_dataTest.RunTest(DataBreakpointTest::WhichTest::ExecutionTest) ? TestStatus::TEST_SUCCESS : TestStatus::TEST_FAILURE;
        }
        if (m_gamePadButtons.b == DirectX::GamePad::ButtonStateTracker::PRESSED)
        {
            m_readTestResult = m_dataTest.RunTest(DataBreakpointTest::WhichTest::ReadTest) ? TestStatus::TEST_SUCCESS : TestStatus::TEST_FAILURE;
        }
        if (m_gamePadButtons.x == DirectX::GamePad::ButtonStateTracker::PRESSED)
        {
            m_readWriteTestResult = m_dataTest.RunTest(DataBreakpointTest::WhichTest::ReadWriteTest) ? TestStatus::TEST_SUCCESS : TestStatus::TEST_FAILURE;
        }
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

    std::wstring outputString;
    outputString = L"Data Breakpoint Tests ";
    uint32_t numDots = (m_timer.GetFrameCount() % 10) + 1;
    for (uint32_t i = 0; i < numDots; i++)
    {
        outputString += L".";
    }
    m_regularFont->DrawString(m_spriteBatch.get(), outputString.c_str(), pos);
    pos.y += m_regularFont->GetLineSpacing() * 3;

    DrawStatusString(L"[A]", L"Execution Breakpoint", m_executionTestResult, pos);
    DrawHelpText(pos, DataBreakpointTest::WhichTest::ExecutionTest);
    pos.y += m_regularFont->GetLineSpacing() * 3;

    DrawStatusString(L"[B]", L"Read Breakpoint", m_readTestResult, pos);
    DrawHelpText(pos, DataBreakpointTest::WhichTest::ReadTest);
    pos.y += m_regularFont->GetLineSpacing() * 3;

    DrawStatusString(L"[X]", L"Read/Write Breakpoint", m_readWriteTestResult, pos);
    DrawHelpText(pos, DataBreakpointTest::WhichTest::ReadWriteTest);
    pos.y += m_regularFont->GetLineSpacing() * 3;

    m_spriteBatch->End();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::DrawStatusString(const std::wstring& button, const std::wstring& testName, TestStatus status, XMFLOAT2& pos)
{
    RECT stringBounds;
    std::wstring outputString;
    std::wstring resultString;
    XMVECTORF32 resultColor = ATG::Colors::OffWhite;
    outputString = L"Press ";
    outputString += button;
    outputString += L" to run ";
    outputString += testName;
    outputString += L" Test: ";
    switch (status)
    {
    case TestStatus::TEST_NOT_RUN:
        resultString += L"not run yet";
        break;
    case TestStatus::TEST_SUCCESS:
        resultString += L"success";
        resultColor = ATG::Colors::Green;
        break;
    case TestStatus::TEST_FAILURE:
        resultString += L"failure";
        resultColor = ATG::Colors::Orange;
        break;
    }
    DX::DrawControllerString(m_spriteBatch.get(), m_regularFont.get(), m_ctrlFont.get(), outputString.c_str(), pos);
    stringBounds = DX::MeasureControllerDrawBounds(m_regularFont.get(), m_ctrlFont.get(), outputString.c_str(), pos);
    pos.x += (stringBounds.right - stringBounds.left);
    m_regularFont->DrawString(m_spriteBatch.get(), resultString.c_str(), pos, resultColor);
    pos.x -= (stringBounds.right - stringBounds.left);
}

void Sample::DrawHelpText(DirectX::XMFLOAT2& pos, DataBreakpointTest::WhichTest whichTest)
{
    static const wchar_t *helpText[] = {
        L"  Execution breakpoints.",
        L"    Sets a breakpoint when a particular instruction is executed, for example the entry point of a function.",
        L"    This is useful in finding specific code paths during automation, for instance calling physics outside the physics phase",
        L"  Memory read breakpoints.",
        L"    Sets a breakpoint when a particular variable is read from.",
        L"    This is useful to track down issues where data is being used after delete if the breakpoint is set during free.",
        L"  Memory read/write breakpoints",
        L"    Sets a breakpoint when a particular memory address is either written to or read from.",
        L"    This is useful to track down various memory issues like access off the end of an array. Set a breakpoint at the first address past the array.",
    };
    uint32_t startIndex(0);
    switch (whichTest)
    {
    case DataBreakpointTest::WhichTest::ExecutionTest:
        startIndex = 0;
        break;
    case DataBreakpointTest::WhichTest::ReadTest:
        startIndex = 3;
        break;
    case DataBreakpointTest::WhichTest::ReadWriteTest:
        startIndex = 6;
        break;
    }
    for (uint32_t i = 0; i < 3; i++)
    {
        pos.y += m_regularFont->GetLineSpacing() * 1.1f;
        m_regularFont->DrawString(m_spriteBatch.get(), helpText[i + startIndex], pos);
    }
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
void Sample::GetDefaultSize(int& width, int& height) const noexcept
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

#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    wchar_t strFilePath[MAX_PATH] = {};

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        Descriptors::Count);

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    SpriteBatchPipelineStateDescription pd(rtState);
    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

    DX::FindMediaFile(strFilePath, MAX_PATH, L"Assets\\ATGSampleBackground.dds");
    DX::ThrowIfFailed(
        CreateDDSTextureFromFileEx(device, resourceUpload,
            strFilePath,
            0, D3D12_RESOURCE_FLAG_NONE, DDS_LOADER_FORCE_SRGB,
            m_background.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));

    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
        m_regularFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_resourceDescriptors->GetCpuHandle(Descriptors::RegularFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::RegularFont));
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
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
