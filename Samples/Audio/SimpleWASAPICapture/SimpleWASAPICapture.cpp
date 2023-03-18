//--------------------------------------------------------------------------------------
// SimpleWASAPICapture.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleWASAPICapture.h"

#include "ATGColors.h"
#include "ControllerFont.h"

extern void ExitSample();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;
using namespace Windows::Foundation;

namespace
{
	bool g_bListDirty = true;
	int g_CaptureID = 0;

	void NotifyListUpdate(int iCaptureID)
	{
		g_CaptureID = iCaptureID;
		g_bListDirty = true;
	}
}

Sample::Sample() noexcept(false) :
	m_bHasCaptured(false),
	m_keyDown(false)
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

	m_wm = std::make_unique<WASAPIManager>();

	m_wm->SetDeviceChangeCallback(&NotifyListUpdate);
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

	if (!m_keyDown)
	{
		if (pad.IsAPressed() && m_bHasCaptured)
		{
			m_wm->PlayToggle();
			m_keyDown = true;
		}
		else if (pad.IsBPressed())
		{
			m_wm->RecordToggle();
			m_keyDown = true;
			m_bHasCaptured = true;
		}
		else if (pad.IsDPadUpPressed())
		{
			m_keyDown = true;
			g_CaptureID--;
			if (g_CaptureID < 0)
			{
				g_CaptureID = (int)m_deviceList.size() - 1;
			}

			HRESULT hr = m_wm->SetCaptureDevice(UINT32(g_CaptureID));
			if (FAILED(hr))
			{
				m_errorString = L"Error: " + std::to_wstring(hr);
			}
			else
			{
				m_errorString.clear();
			}
		}
		else if (pad.IsDPadDownPressed())
		{
			m_keyDown = true;
			g_CaptureID++;
			if (g_CaptureID > int(m_deviceList.size() - 1))
			{
				g_CaptureID = 0;
			}

			HRESULT hr = m_wm->SetCaptureDevice(UINT32(g_CaptureID));
			if (FAILED(hr))
			{
				m_errorString = L"Error: " + std::to_wstring(hr);
			}
			else
			{
				m_errorString.clear();
			}
		}
		else if (pad.IsViewPressed())
		{
			ExitSample();
		}

	}
	else if (!pad.IsAPressed() && !pad.IsBPressed() && !pad.IsDPadUpPressed() && !pad.IsDPadDownPressed())
	{
		m_keyDown = false;
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
	std::wstring tempString;

	if (g_bListDirty)
	{
		m_wm->GetCaptureDevices(m_deviceList);
		g_bListDirty = false;
	}

	auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_spriteBatch->Begin(commandList);

	float spacing = m_font->GetLineSpacing();

	m_font->DrawString(m_spriteBatch.get(), L"Audio captured from the selected mic is played back to the default output", pos, ATG::Colors::OffWhite);
	pos.y += spacing;
	m_font->DrawString(m_spriteBatch.get(), L"Note that no sample conversion is done!", pos, ATG::Colors::OffWhite);
	pos.y += spacing;

	if (!m_errorString.empty())
	{
		m_font->DrawString(m_spriteBatch.get(), m_errorString.c_str(), pos, ATG::Colors::Orange);
	}

	pos.y += spacing;

	if (m_deviceList.empty())
	{
		m_font->DrawString(m_spriteBatch.get(), L"No capture devices!", pos, ATG::Colors::Orange);
	}
	else
	{
		for (size_t i = 0; i < m_deviceList.size(); i++)
		{
			if (int(i) == g_CaptureID)
			{
				tempString = L"> " + std::wstring(m_deviceList.at(i));
			}
			else
			{
				tempString = m_deviceList.at(i);
			}

			m_font->DrawString(m_spriteBatch.get(), tempString.c_str(), pos, ATG::Colors::OffWhite);
			pos.y += spacing;
		}
	}

	pos.y += spacing;

	if (m_bHasCaptured)
	{
		DX::DrawControllerString(m_spriteBatch.get(), m_font.get(), m_ctrlFont.get(), L"Press [A] Button to start / stop playback of last recording", pos);
		pos.y += spacing;
	}

	DX::DrawControllerString(m_spriteBatch.get(), m_font.get(), m_ctrlFont.get(), L"Press [B] Button to start / stop recording", pos, ATG::Colors::OffWhite);
	pos.y += spacing;
	DX::DrawControllerString(m_spriteBatch.get(), m_font.get(), m_ctrlFont.get(), L"Press [DPad] Up/Down to change capture device", pos, ATG::Colors::OffWhite);
	pos.y += spacing * 1.5f;

	tempString = L"Capture: " + convertBoolToRunning(m_wm->m_recordingSound);
	m_font->DrawString(m_spriteBatch.get(), tempString.c_str(), pos, ATG::Colors::OffWhite);
	pos.y += spacing;
	tempString = L"Playback: " + convertBoolToRunning(m_wm->m_playingSound);
	m_font->DrawString(m_spriteBatch.get(), tempString.c_str(), pos, ATG::Colors::OffWhite);
	pos.y += spacing;

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
    m_wm->StopPlayback();

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

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

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
