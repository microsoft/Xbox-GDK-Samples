//--------------------------------------------------------------------------------------
// SimpleWASAPICapture.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "WASAPIManager.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

private:
	const std::wstring convertBoolToEnabled(bool bIsEnabled)
	{
		return bIsEnabled ? L"Enabled" : L"Disabled";
	}

	const std::wstring convertBoolToRunning(bool bIsRunning)
	{
		return bIsRunning ? L"Running" : L"Stopped";
	}

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void Play(const wchar_t* szFilename);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;
	std::unique_ptr<DirectX::SpriteFont>		m_ctrlFont;
	
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_background;

	std::unique_ptr<WASAPIManager>				m_wm;

	std::vector<LPWSTR>							m_deviceList;
	bool										m_bHasCaptured;
	bool										m_keyDown;

    enum Descriptors
    {
        TextFont,
		CtrlFont,
        Background,
        Count,
    };

	std::wstring								m_errorString;
};
