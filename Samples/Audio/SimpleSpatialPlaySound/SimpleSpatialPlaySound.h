//--------------------------------------------------------------------------------------
// SimpleSpatialPlaySound.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "SpriteFont.h"
#include "ISACRenderer.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
#define MAX_CHANNELS 12 //up to 7.1.4 channels

	struct Audiochannel
	{
		BYTE * wavBuffer;
		UINT32 buffersize;
		float  volume;
		UINT32 curBufferLoc;

		Microsoft::WRL::ComPtr<ISpatialAudioObject> object;
		AudioObjectType		objType;

	};

public:

    Sample() noexcept(false);
    ~Sample();

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();
    void Render();

    // Rendering helpers
    void Clear();

    // Messages
    void OnSuspending();
    void OnResuming();


	ISACRenderer*      m_Renderer;

	int		m_numChannels;
	Audiochannel WavChannels[MAX_CHANNELS];
	bool	m_bThreadActive;
	bool	m_bPlayingSound;
	int		m_availableObjects;
    
private:

    void Update(DX::StepTimer const& timer);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

	bool LoadFile(LPCWSTR inFile);

	void SetChannelPosVolumes(void);
	HRESULT InitializeSpatialStream(void);

	// Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device
    std::unique_ptr<DirectX::GamePad>           m_gamePad;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    // UI
	std::unique_ptr<DirectX::SpriteBatch>		m_spriteBatch;
	std::unique_ptr<DirectX::SpriteFont>		m_font;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_background;

    enum Descriptors
    {
        TextFont,
		CtrlFont,
        Background,
        Count,
    };

	bool	m_fileLoaded;
	int		m_curFile;
    bool 	m_playPressed;
    bool    m_nextPressed;

	//worker thread for spatial system
	PTP_WORK m_workThread;

};
