//--------------------------------------------------------------------------------------
// XAudio2Manager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <xaudio2.h>

class XAudio2Manager : public IXAudio2EngineCallback, public IXAudio2VoiceCallback
{
    public:
        XAudio2Manager() noexcept(false);
        virtual ~XAudio2Manager();

        XAudio2Manager(XAudio2Manager&&) = default;
        XAudio2Manager& operator= (XAudio2Manager&&) = default;

        XAudio2Manager(XAudio2Manager const&) = delete;
        XAudio2Manager& operator= (XAudio2Manager const&) = delete;

        HRESULT InitializeDevice(wchar_t* endpoint, uint32_t locationCount, GUID* locations);
        bool IsPlaying() const { return m_playing; };
        HRESULT ConfigureWaveSource(const wchar_t* filename);
        HRESULT Play();
        HRESULT Stop();
        HRESULT TitleSuspend();
        HRESULT TitleResume();

    private:
        Microsoft::WRL::ComPtr<IXAudio2>  m_xAudio2;
        IXAudio2MasteringVoice*           m_masteringVoice;
        IXAudio2SourceVoice*              m_sourceVoice;
        std::unique_ptr<uint8_t[]>        m_waveFileData;
        uint32_t                          m_locationCount;
        GUID*                             m_locations;
        bool                              m_playing;

        void Shutdown();

        // Inherited via IXAudio2EngineCallback
        void __stdcall OnProcessingPassStart(void) noexcept override;
        void __stdcall OnProcessingPassEnd(void) noexcept override;
        void __stdcall OnCriticalError(HRESULT Error) noexcept override;

        void __stdcall OnVoiceProcessingPassStart(UINT32 BytesRequired) noexcept override;
        void __stdcall OnVoiceProcessingPassEnd(void) noexcept override;
        void __stdcall OnStreamEnd(void) noexcept override;
        void __stdcall OnBufferStart(void* pBufferContext) noexcept override;
        void __stdcall OnBufferEnd(void* pBufferContext) noexcept override;
        void __stdcall OnLoopEnd(void* pBufferContext) noexcept override;
        void __stdcall OnVoiceError(void* pBufferContext, HRESULT Error) noexcept override;
};
