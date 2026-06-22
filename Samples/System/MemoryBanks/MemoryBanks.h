//--------------------------------------------------------------------------------------
// MemoryBanks.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "MemoryDemo.h"
#include "TestFramework.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample    : public ATG::TestFramework
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

	// Main class driving the demo code
	ATG::MemoryDemo m_memoryBankDemo;
	enum class TestStatus
	{
		TEST_NOT_RUN,
		TEST_SUCCESS,
		TEST_FAILURE,
	};
	TestStatus m_randomBankStatus = {};
	TestStatus m_fixedBankStatus = {};
	TestStatus m_readOnlyBankStatus = {};
	TestStatus m_bankSwitchingStatus = {};
	TestStatus m_sharedAddressStatus = {};

	void DrawStatusString(const std::wstring& button, const std::wstring& testName, TestStatus status, DirectX::XMFLOAT2& pos);
	void DrawHelpText(DirectX::XMFLOAT2& pos, ATG::MemoryBankDemoTests);

	void Update(DX::StepTimer const& timer);
	void Render();

	void Clear();

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();

	// Device resources.
	std::unique_ptr<DX::DeviceResources>        m_deviceResources;

	// Rendering loop timer.
	uint64_t                                    m_frame;
	DX::StepTimer                               m_timer;

	// Input device.
	std::unique_ptr<ATGGamePad>           m_gamePad;
	DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

	// DirectXTK objects.
	std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
	std::unique_ptr<DirectX::DescriptorHeap>	m_resourceDescriptors;
	std::unique_ptr<DirectX::SpriteBatch>		m_spriteBatch;
	Microsoft::WRL::ComPtr<ID3D12Resource>		m_background;
	std::unique_ptr<DirectX::SpriteFont>		m_regularFont;
	std::unique_ptr<DirectX::SpriteFont>		m_largeFont;
	std::unique_ptr<DirectX::SpriteFont>		m_ctrlFont;

	enum Descriptors
	{
		Background,
		RegularFont,
		LargeFont,
		CtrlFont,
		Count
	};

    // TestFramework
    ID3D12CommandQueue* GetCommandQueue() { return m_deviceResources->GetCommandQueue(); }
    ID3D12Resource* GetRenderTarget() { return m_deviceResources->GetRenderTarget(); }
    ATGGamePad* GetGamePad() { return m_gamePad.get(); }
};
