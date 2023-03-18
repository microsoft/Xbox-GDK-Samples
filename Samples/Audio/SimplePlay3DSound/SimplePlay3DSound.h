//--------------------------------------------------------------------------------------
// SimplePlay3DSound.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public IXAudio2EngineCallback
{
public:

    Sample() noexcept(false);
    virtual ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();
    void OnConstrained() {}
    void OnUnConstrained() {}

private:

    void InitializeXAudio();

    STDMETHOD_(void, OnProcessingPassStart) () override {}
    STDMETHOD_(void, OnProcessingPassEnd)() override {}
    STDMETHOD_(void, OnCriticalError) (THIS_ HRESULT)
    {
        //When the renderer is invalidated, restart
        if (m_XAudio2)
        {
            m_XAudio2->StopEngine();

            m_XAudio2->UnregisterForCallbacks(this);

            if (m_sourceVoice)
            {
                m_sourceVoice->DestroyVoice();
                m_sourceVoice = nullptr;
            }

            if (m_submixVoice)
            {
                m_submixVoice->DestroyVoice();
                m_submixVoice = nullptr;
            }

            if (m_masteringVoice)
            {
                m_masteringVoice->DestroyVoice();
                m_masteringVoice = nullptr;
            }

            m_reverbEffect.Reset();
            m_XAudio2.Reset();
        }

        m_CritErrorOccurred = true;
    };

    //Audio functions
	void SetReverb(int index);
	void SetupEnvironment();
	void PlayFile(const wchar_t* filename);
	void AdjustFront(float value, X3DAUDIO_VECTOR *orientFront, float *angle);

	//Draw functions
	void XM_CALLCONV DrawGrid(ID3D12GraphicsCommandList* commandList, size_t xdivs, size_t ydivs, DirectX::FXMVECTOR color);
	void DrawCircle(ID3D12GraphicsCommandList* commandList, X3DAUDIO_VECTOR position, float radius);
	void XM_CALLCONV DrawEmitter(ID3D12GraphicsCommandList* commandList, X3DAUDIO_CONE* cone, X3DAUDIO_VECTOR position, float angle, DirectX::FXMVECTOR color, UINT size);
	void XM_CALLCONV DrawListener(ID3D12GraphicsCommandList* commandList, X3DAUDIO_VECTOR position, DirectX::FXMVECTOR color);

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
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;
	std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;
	std::unique_ptr<DirectX::CommonStates>      m_states;
	std::unique_ptr<DirectX::BasicEffect>       m_batchEffectLine;
	std::unique_ptr<DirectX::BasicEffect>       m_batchEffectTri;

	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;

	Microsoft::WRL::ComPtr<ID3D12Resource>		m_circleTexture;
	Microsoft::WRL::ComPtr<ID3D12Resource>		m_background;

    enum Descriptors
    {
		Circle,
		TextFont,
		CtrlFont,
        Background,
        Count,
    };

	//XAudio2 interfaces
	std::unique_ptr<uint8_t[]>                      m_waveFile;
	Microsoft::WRL::ComPtr<IXAudio2>                m_XAudio2;
	IXAudio2MasteringVoice*                         m_masteringVoice;
	IXAudio2SourceVoice*                            m_sourceVoice;
	IXAudio2SubmixVoice*                            m_submixVoice;
	Microsoft::WRL::ComPtr<IUnknown>                m_reverbEffect;
	std::unique_ptr<float[]>                        m_matrix;

	//X3DAudio values
	X3DAUDIO_DISTANCE_CURVE_POINT                   m_volumePoints[10];
	X3DAUDIO_DISTANCE_CURVE                         m_volumeCurve;
	X3DAUDIO_DISTANCE_CURVE_POINT                   m_reverbPoints[10];
	X3DAUDIO_DISTANCE_CURVE                         m_reverbCurve;
	X3DAUDIO_HANDLE			                        m_X3DInstance;
	X3DAUDIO_LISTENER		                        m_X3DListener;
	X3DAUDIO_EMITTER		                        m_X3DEmitter;
	X3DAUDIO_DSP_SETTINGS                           m_X3DDSPSettings;
	XAUDIO2_VOICE_DETAILS                           m_deviceDetails;
	X3DAUDIO_CONE                                   m_emitterCone;

	// Game state
	int                                             m_reverbIndex;
	float                                           m_listenerAngle;
	float                                           m_emitterAngle;
    std::atomic<bool>                               m_CritErrorOccurred;
};
