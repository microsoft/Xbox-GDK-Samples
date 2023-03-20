//--------------------------------------------------------------------------------------
// SimpleWASAPIPlaySound.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleWASAPIPlaySound.h"

#include "ATGColors.h"
#include "ControllerFont.h"

extern void ExitSample();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
	m_playPressed(false),
    m_playingWhenSuspended(false)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
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

	m_wm = new WASAPIManager();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    m_deviceResources->WaitForOrigin();

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Sample::Update(DX::StepTimer const&)
{
    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        if (pad.IsViewPressed())
        {
            ExitSample();
        }
    }

	if (pad.IsAPressed() && !m_playPressed)
	{
		m_wm->PlayPauseToggle();
		m_playPressed = true;
	}
	else if (!pad.IsAPressed() && m_playPressed)
	{
		m_playPressed = false;
	}
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

    auto fullscreen = m_deviceResources->GetOutputSize();

    auto safeRect = Viewport::ComputeTitleSafeArea(UINT(fullscreen.right - fullscreen.left), UINT(fullscreen.bottom - fullscreen.top));

    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_spriteBatch->Begin(commandList);

	m_font->DrawString(m_spriteBatch.get(), L"Simple WASAPI Playback:", pos, ATG::Colors::White);
	pos.y += m_font->GetLineSpacing() * 1.5f;

	wchar_t str[128] = {};
	swprintf_s(str, L"Audio Source - Test tone at 440hz : %s", (m_wm->IsPlaying()) ? L"Is Playing" : L"Is Stopped");
	m_font->DrawString(m_spriteBatch.get(), str, pos, ATG::Colors::White);

	DX::DrawControllerString(m_spriteBatch.get(), m_font.get(), m_ctrlFont.get(), L"Press [A] to start/stop playback   [View] Exit",
		XMFLOAT2(float(safeRect.left), float(safeRect.bottom) - m_font->GetLineSpacing()),
		ATG::Colors::LightGrey);

    m_spriteBatch->End();

    // Show the new frame.
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
	auto commandList = m_deviceResources->GetCommandList();

	// Clear the views.
	auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
	commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
	commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

	// Set the viewport and scissor rect.
	auto viewport = m_deviceResources->GetScreenViewport();
	auto scissorRect = m_deviceResources->GetScissorRect();
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
    m_playingWhenSuspended = m_wm->m_state == DeviceState::DeviceStatePlaying;

    m_wm->Pause();

    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();

    if (m_playingWhenSuspended)
    {
        m_wm->PlayPauseToggle();
    }
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

	//m_states = std::make_unique<DirectX::CommonStates>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        m_spriteBatch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    m_font = std::make_unique<SpriteFont>(device, upload,
        L"SegoeUI_18.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::TextFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::TextFont));

	m_ctrlFont = std::make_unique<SpriteFont>(device, upload,
		L"XboxOneControllerLegendSmall.spritefont",
		m_resourceDescriptors->GetCpuHandle(Descriptors::CtrlFont),
		m_resourceDescriptors->GetGpuHandle(Descriptors::CtrlFont));
	
	DX::ThrowIfFailed(CreateDDSTextureFromFile(device, upload, L"ATGSampleBackground.DDS", m_background.ReleaseAndGetAddressOf()));

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto vp = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(vp);
}
#pragma endregion
