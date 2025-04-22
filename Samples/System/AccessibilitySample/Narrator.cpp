//--------------------------------------------------------------------------------------
// Narrator.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "Narrator.h"
#include <string.h>
#include <XSpeechSynthesizer.h>

#pragma region Thread Management
// Callback added to the work queue that transforms text to audio
void CALLBACK SynthesizeTextCallback(void* context, bool)
{
    Narrator* narrator = reinterpret_cast<Narrator*>(context);
    NarratorState& narratorState = narrator->m_narratorState;

    // Stop audio playback
    HRESULT hr = narrator->m_sourceVoice->Stop();

    std::string text = narratorState.GetText();

    char buffer[256];
    if (FAILED(hr))
    {
        snprintf(buffer, sizeof(buffer), "Failed to stop audio playback. HRESULT: 0x%08lx \n", hr);
        OutputDebugStringA(buffer);
        return;
    }

    // Flush the source voice buffer to clear current audio streams
    hr = narrator->m_sourceVoice->FlushSourceBuffers();
    if (FAILED(hr))
    {
        snprintf(buffer, sizeof(buffer), "Failed to flush source buffers. HRESULT: 0x%08lx \n", hr);
        OutputDebugStringA(buffer);
        return;
    }

    // Restart audio playback
    hr = narrator->m_sourceVoice->Start(0);
    if (FAILED(hr))
    {
        snprintf(buffer, sizeof(buffer), "Failed to start audio playback. HRESULT: 0x%08lx \n", hr);
        OutputDebugStringA(buffer);
        return;
    }

    // Speech synthesizer stream related variables
    size_t bufferSize;
    XAUDIO2_BUFFER audioBuffer =
    {
        /* Flags */                 0,
        /* AudioBytes */            0,
        /* pAudioData */            nullptr,
        /* PlayBegin */             0,
        /* PlayLength */            0,
        /* LoopBegin */             0,
        /* LoopLength */            0,
        /* LoopCount */             0,
        /* pContext */              nullptr
    };

    XSpeechSynthesizerHandle ssHandle;
    XSpeechSynthesizerStreamHandle ssStreamHandle;

    // Create the speech synthesizer used to convert text to audio data
    hr = XSpeechSynthesizerCreate(&ssHandle);

    if (FAILED(hr))
    {
        snprintf(buffer, sizeof(buffer), "Failed to create speech synthesizer. HRESULT: 0x%08lx \n", hr);
        OutputDebugStringA(buffer);
        return;
    }

    hr = XSpeechSynthesizerCreateStreamFromText(ssHandle, text.data(), &ssStreamHandle);
    if (FAILED(hr))
    {
        snprintf(buffer, sizeof(buffer), "Failed to create speech synthesizer stream from text. HRESULT: 0x%08lx \n", hr);
        OutputDebugStringA(buffer);
        return;
    }

    hr = XSpeechSynthesizerGetStreamDataSize(ssStreamHandle, &bufferSize);
    if (FAILED(hr))
    {
        snprintf(buffer, sizeof(buffer), "Failed to get speech synthesizer stream data size. HRESULT: 0x%08lx \n", hr);
        OutputDebugStringA(buffer);
        return;
    }

    narrator->m_audioData.resize(bufferSize);
    hr = XSpeechSynthesizerGetStreamData(ssStreamHandle, bufferSize, narrator->m_audioData.data(), &bufferSize);
    if (FAILED(hr))
    {;
        snprintf(buffer, sizeof(buffer), "Failed to get speech synthesizer stream data. HRESULT: 0x%08lx \n", hr);
        OutputDebugStringA(buffer);
        return;
    }

    // Clean up the speech synthesizer stream
    hr = XSpeechSynthesizerCloseStreamHandle(ssStreamHandle);
    if (FAILED(hr))
    {
        snprintf(buffer, sizeof(buffer), "Failed to close speech synthesizer stream handle. HRESULT: 0x%08lx \n", hr);
        OutputDebugStringA(buffer);
        return;
    }

    hr = XSpeechSynthesizerCloseHandle(ssHandle);
    if (FAILED(hr))
    {
        snprintf(buffer, sizeof(buffer), "Failed to close speech synthesizer handle. HRESULT: 0x%08lx \n", hr);
        OutputDebugStringA(buffer);
        return;
    }

    // Convert the synth stream data to an XAudio2 buffer.
    audioBuffer.AudioBytes = static_cast<UINT32>(bufferSize);
    audioBuffer.pAudioData = reinterpret_cast<const BYTE*>(narrator->m_audioData.data());

    // Submit XAudio2 buffer to source voice
    hr = narrator->m_sourceVoice->SubmitSourceBuffer(&audioBuffer);
    if (FAILED(hr))
    {
        snprintf(buffer, sizeof(buffer), "Failed to submit XAudio2 buffer to source voice. HRESULT: 0x%08lx \n", hr);
        OutputDebugStringA(buffer);
        return;
    }
}


// Thread that processes pending narrations. This thread is woken up when a new narration is added
void NarrationManagementThread(Narrator* narrator, XTaskQueueHandle& handle)
{
    while (narrator->IsThreadActive())
    {
        XTaskQueueDispatch(handle, XTaskQueuePort::Work, 1000);
    }
}
#pragma endregion

#pragma region Narrator
void Narrator::AddNarration(std::string_view stringView)
{
    if (m_enabled)
    {
        if (!m_narratorState.Update(stringView.data()))
        {
            return;
        }

        // Submit task to the work queue
        XTaskQueueSubmitCallback(m_taskQueue, XTaskQueuePort::Work, this, SynthesizeTextCallback);
    }
}

Narrator::Narrator()
{
    // Narrator enabled by default
    m_enabled = true;
    m_isRunning = false;

    Initialize();

    if (FAILED(Initialize()))
    {
        printf("Failed to initialize Narrator");
        return;
    }

    // Start the narration thread.
    StartNarrationThread();
    m_narrationThread = std::thread([&] { NarrationManagementThread(this, m_taskQueue); });
}

HRESULT Narrator::Initialize()
{
    // Set up XAudio2 and start the source voice
    HRESULT hr = XAudio2Create(&m_xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr))
    {
        return hr;
    }

    // Create mastering voice. It encapsulates an audio device
    hr = m_xAudio2->CreateMasteringVoice(&m_masterVoice);
    if (FAILED(hr))
    {
        return hr;
    }

    m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    m_waveFormat.nChannels = 1;
    m_waveFormat.nSamplesPerSec = 22050;
    m_waveFormat.nAvgBytesPerSec = 22050 * sizeof(short);
    m_waveFormat.nBlockAlign = sizeof(short);
    m_waveFormat.wBitsPerSample = 16;
    m_waveFormat.cbSize = 0;

    // Create source voice to process audio buffers
    hr = m_xAudio2->CreateSourceVoice(&m_sourceVoice, &m_waveFormat);
    if (FAILED(hr))
    {
        return hr;
    }

    // Make source voice starts consuming audio
    hr = m_sourceVoice->Start(0);
    if (FAILED(hr))
    {
        return hr;
    }

    // Set up the task queue
    hr = XTaskQueueCreate(XTaskQueueDispatchMode::Manual, XTaskQueueDispatchMode::Manual, &m_taskQueue);
    if (FAILED(hr))
    {
        return hr;
    }
    return S_OK;
}

Narrator::~Narrator()
{
    // Stop the narration thread
    StopNarrationThread();
    XTaskQueueSubmitCallback(m_taskQueue, XTaskQueuePort::Work, this, [](void* , bool) {});

    if (m_narrationThread.joinable())
    {
        m_narrationThread.join(); // Wait for the thread to finish
    }

    // Release the task queue
    XTaskQueueTerminate(m_taskQueue, true, nullptr, nullptr);
    XTaskQueueCloseHandle(m_taskQueue);

    // Stop the source voice
    m_sourceVoice->Stop();
    m_sourceVoice->FlushSourceBuffers();

    // Release the source voice
    m_sourceVoice->DestroyVoice();

    // Release the mastering voice
    m_masterVoice->DestroyVoice();

    // Release the XAudio2 object
    m_xAudio2->Release();
}
#pragma endregion
