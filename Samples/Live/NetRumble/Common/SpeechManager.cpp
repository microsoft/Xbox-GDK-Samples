#include "pch.h"

#include <json/json.hpp>

#include "Game.h"
#include "Manager.h"
#include <WAVFileReader.h>

#include "SpeechManager.h"

using namespace NetRumble;

namespace
{
    static constexpr auto restUri = "https://profile.xboxlive.com/users/me/profile/settings?settings=SpeechAccessibility";
}

SpeechManager::SpeechManager()
{
    // Initialize XAudio2 objects
    DX::ThrowIfFailed(XAudio2Create(m_xaudio2.GetAddressOf(), 0));
    HRESULT hr = m_xaudio2->CreateMasteringVoice(&m_masteringVoice);
    if(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
    {
        DEBUGLOG("No default audio device; running in 'silent mode'\n");
        silentMode = true;
    }
    else if(FAILED(hr))
    {
        DEBUGLOG("CreateMasteringVoice failed: %08X\n", hr);
    }
}

HRESULT SpeechManager::LoadAccessibiltySettingsAsync(
    XUserHandle user,
    std::function<void(HRESULT result)> callback = nullptr
)
{
    struct AccessibilityContext
    {
        SpeechManager* manager{};
        std::function<void(HRESULT result)> callback{};
        XAsyncBlock async{};
        XblContextHandle context{};
        XblHttpCallHandle handle{};
    };

    auto context = new AccessibilityContext{ this, callback };

    context->async.context = context;
    context->async.callback = [](XAsyncBlock* async)
    {
        auto context = static_cast<AccessibilityContext*>(async->context);
        const char* body{};

        // Retrieve the response
        HRESULT hr = XblHttpCallGetResponseString(context->handle, &body);

        if (SUCCEEDED(hr))
        {
            try
            {
                // Now parse the response JSON
                auto propsJson = json::parse(body);

                for (auto& node : propsJson["profileUsers"][0]["settings"])
                {
                    auto id = (std::string)node["id"];

                    // If we find an accessibility settings node, parse that
                    if (id == "SpeechAccessibility")
                    {
                        auto internalJson = (std::string)node["value"];

                        if (!internalJson.empty())
                        {
                            auto nodeJson = json::parse(internalJson);

                            auto settings = AccessibilitySettings
                            {
                                nodeJson.contains("GameChatSTT") ? (bool)nodeJson["GameChatSTT"] : false,
                                nodeJson.contains("GameChatTTS") ? (bool)nodeJson["GameChatTTS"] : false,
                                nodeJson.contains("GameTextSS") ? (bool)nodeJson["GameTextSS"] : false,
                                nodeJson.contains("PersonaGender") ? (int)nodeJson["PersonaGender"] : 0,
                                nodeJson.contains("PersonaId") ? (std::string)nodeJson["PersonaId"] : "",
                                nodeJson.contains("PersonaLang") ? (std::string)nodeJson["PersonaLang"] : "",
                                nodeJson.contains("PersonaName") ? (std::string)nodeJson["PersonaName"] : ""
                            };

                            // Establish the user's settings
                            context->manager->SetAccessibilitySettings(settings);
                        }
                    }
                }
            }
            catch (std::exception& e)
            {
                DEBUGLOG("Unable to parse response JSON: %s: %s\n", e.what(), body);
            }
        }
        else
        {
            DEBUGLOG("XblHttpCallGetResponseString failed: 0x%x\n", hr);
        }

        if (context->callback)
        {
            context->callback(hr);
        }

        XblHttpCallCloseHandle(context->handle);
        XblContextCloseHandle(context->context);

        delete context;
    };

    // Create an Xbl Context handle for the user
    HRESULT hr = XblContextCreateHandle(user, &context->context);

    if (FAILED(hr))
    {
        DEBUGLOG("XblContextCreateHandle failed: 0x%x\n", hr);
        delete context;
        return hr;
    }

    // Create an authenticated call handle
    hr = XblHttpCallCreate(
        context->context,   // XblContextHandle
        "GET",              // Verb
        restUri,            // REST Url
        &context->handle);  // XblHttpCallHandle

    if (FAILED(hr))
    {
        DEBUGLOG("XblHttpCallCreate failed: 0x%x\n", hr);
        XblContextCloseHandle(context->context);
        delete context;
        return hr;
    }

    hr = XblHttpCallRequestSetHeader(context->handle, "x-xbl-contract-version", "3", true);
    if (FAILED(hr))
    {
        DEBUGLOG("XblHttpCallRequestSetHeader failed: 0x%x\n", hr);
        XblHttpCallCloseHandle(context->handle);
        XblContextCloseHandle(context->context);
        delete context;
        return hr;
    }

    hr = XblHttpCallPerformAsync(
        context->handle,                        // XblHttpCallHandle
        XblHttpCallResponseBodyType::String,    // Body type
        &context->async);                       // XAsync block

    if (FAILED(hr))
    {
        DEBUGLOG("XblHttpCallPerformAsync failed: 0x%x\n", hr);
        XblHttpCallCloseHandle(context->handle);
        XblContextCloseHandle(context->context);
        delete context;
        return hr;
    }

    return hr;
}

HRESULT SpeechManager::RenderTextToSpeech(std::string_view text, bool force)
{
    if ((!m_settings.LetGamesReadToMe && !force) || text.empty() || silentMode)
    {
        return S_FALSE;
    }

    // Only service TTS requests one at a time
    std::lock_guard<std::mutex> lock(m_lock);

    // Stop any existing speech in favor of the latest
    if (m_sourceVoice)
    {
        XAUDIO2_VOICE_STATE vs{};

        m_sourceVoice->GetState(&vs);

        if (vs.BuffersQueued > 0)
        {
            m_sourceVoice->Stop();
        }

        m_sourceVoice->FlushSourceBuffers();
        m_sourceVoice->DestroyVoice();

        m_sourceVoice = nullptr;
    }

    // Generate the voice stream
    XSpeechSynthesizerHandle synthHandle{};

    HRESULT hr = XSpeechSynthesizerCreate(&synthHandle);
    if (FAILED(hr))
    {
        DEBUGLOG("XSpeechSynthesizerCreate failed: 0x%x\n", hr);
        return hr;
    }

    // Set the voice information
    if (!m_settings.PersonaId.empty())
    {
        hr = XSpeechSynthesizerSetCustomVoice(synthHandle, m_settings.PersonaId.c_str());
        if (FAILED(hr))
        {
            DEBUGLOG("XSpeechSynthesizerSetCustomVoice failed: 0x%x\n", hr);
            XSpeechSynthesizerCloseHandle(synthHandle);
            return hr;
        }
    }

    // Create the stream object
    XSpeechSynthesizerStreamHandle streamHandle{};

    hr = XSpeechSynthesizerCreateStreamFromText(synthHandle, text.data(), &streamHandle);
    if (FAILED(hr))
    {
        DEBUGLOG("XSpeechSynthesizerCreateStreamFromText failed: 0x%x\n", hr);
        XSpeechSynthesizerCloseHandle(synthHandle);
        return hr;
    }

    // Get the needed buffer size
    size_t bufferSize;

    hr = XSpeechSynthesizerGetStreamDataSize(streamHandle, &bufferSize);
    if (FAILED(hr))
    {
        DEBUGLOG("XSpeechSynthesizerGetStreamDataSize failed: 0x%x\n", hr);
        XSpeechSynthesizerCloseStreamHandle(streamHandle);
        XSpeechSynthesizerCloseHandle(synthHandle);
        return hr;
    }

    // Generate the voice buffer
    m_voiceBuffer.resize(bufferSize);

    hr = XSpeechSynthesizerGetStreamData(streamHandle, bufferSize, m_voiceBuffer.data(), &bufferSize);
    if (FAILED(hr))
    {
        DEBUGLOG("XSpeechSynthesizerGetStreamData failed: 0x%x\n", hr);
        XSpeechSynthesizerCloseStreamHandle(streamHandle);
        XSpeechSynthesizerCloseHandle(synthHandle);
        return hr;
    }

    // Done with the speech synthesizer
    XSpeechSynthesizerCloseStreamHandle(streamHandle);
    XSpeechSynthesizerCloseHandle(synthHandle);

    // Convert voice buffer to WAV
    DX::WAVData wav{};

    hr = DX::LoadWAVAudioInMemoryEx(m_voiceBuffer.data(), bufferSize, wav);
    if (FAILED(hr))
    {
        DEBUGLOG("LoadWAVAudioInMemoryEx failed: 0x%x\n", hr);
        return hr;
    }

    hr = m_xaudio2->CreateSourceVoice(&m_sourceVoice, wav.wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO);
    if (FAILED(hr))
    {
        DEBUGLOG("CreateSourceVoice failed: 0x%x\n", hr);
        return hr;
    }

    // Submit the WAV to XAudio2
    XAUDIO2_BUFFER xbuffer = {};

    xbuffer.pAudioData = wav.startAudio;
    xbuffer.Flags = XAUDIO2_END_OF_STREAM;
    xbuffer.AudioBytes = wav.audioBytes;

    hr = m_sourceVoice->SubmitSourceBuffer(&xbuffer);
    if (FAILED(hr))
    {
        DEBUGLOG("SubmitSourceBuffer failed: 0x%x\n", hr);
        return hr;
    }

    // Start playing the voice
    hr = m_sourceVoice->Start();
    if (FAILED(hr))
    {
        DEBUGLOG("Start failed: 0x%x\n", hr);
        return hr;
    }

    return S_OK;
}

void SpeechManager::SetAccessibilitySettings(AccessibilitySettings& settings)
{
    m_settings = settings;
}
