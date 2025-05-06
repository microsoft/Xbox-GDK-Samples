//--------------------------------------------------------------------------------------
// Narrator.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <condition_variable>
#include <xaudio2.h>
#include <XAsync.h>

struct NarratorState
{
    NarratorState() :
        text(""),
        lastUpdateTime(std::chrono::steady_clock::now()),  // Initialize with the current time
        timerStart(std::chrono::steady_clock::now())      // Initialize with the current time
    {
    }

    // Prevent shallow copy
    NarratorState(const NarratorState&) = delete;
    NarratorState& operator=(const NarratorState&) = delete;

    bool Update(const char* newValue)
    {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastUpdateTime).count();

        // If less than 200ms since the last update, skip
        if (elapsedTime <= 200)
        {
            return false;
        }

        if (!std::strcmp(text.c_str(), newValue))
        {
            return false;
        }

        text = newValue;
        lastUpdateTime = currentTime;

        return true;
    }

    // Returns a copy of the text.
    // This function is called at most 5 times a second, so this shouldn't impact performance.
    std::string GetText()
    {
        return text;
    }

private:
    std::string                             text;

    // Timing variables
    std::chrono::steady_clock::time_point   lastUpdateTime;
    std::chrono::steady_clock::time_point   timerStart;
};

class Narrator
{
public:
    Narrator();
    ~Narrator();

    // TODO: make narration enable/disable state tied to narration thread lifetime
    void EnableNarration()
    {
        m_enabled = true;
    }

    void DisableNarration()
    {
        m_enabled = false;
    }

    void AddNarration(std::string_view stringView);

    NarratorState                                       m_narratorState;

    // Thread Management
    // TODO: combine with enable logic
    void StartNarrationThread()
    {
        m_isRunning = true;
    }
    void StopNarrationThread()
    {
        m_isRunning = false;
    }
    bool IsThreadActive()
    {
        return m_isRunning;
    }

    HRESULT Initialize();

    // Async accessed XAudio2 fields
    // TODO: make these private and figure out an OOP way to clean these up
    IXAudio2SourceVoice*                                m_sourceVoice;
    std::vector<char>                                   m_audioData; 

private:
    bool                                                m_enabled;
    std::thread                                         m_narrationThread;
    IXAudio2*                                           m_xAudio2;
    IXAudio2MasteringVoice*                             m_masterVoice;
    WAVEFORMATEX                                        m_waveFormat;
    XTaskQueueHandle                                    m_taskQueue;
    bool                                                m_isRunning;
};

