#pragma once

#include <XUser.h>
#include <xaudio2.h>
#include <functional>

#include "Manager.h"

namespace NetRumble
{
    struct AccessibilitySettings
    {
        bool SpeechToTextEnabled;
        bool TextToSpeechEnabled;
        bool LetGamesReadToMe;
        int PersonaGender;
        std::string PersonaId;
        std::string PersonaLanguage;
        std::string PersonaName;
    };

    class SpeechManager : public Manager
    {
    public:
        SpeechManager();

        HRESULT LoadAccessibiltySettingsAsync(
            XUserHandle user,
            std::function<void(HRESULT result)> callback
        );

        HRESULT RenderTextToSpeech(
            std::string_view text,
            bool force = false
        );

    private:
        void SetAccessibilitySettings(
            AccessibilitySettings& settings
        );

        AccessibilitySettings m_settings{};
        Microsoft::WRL::ComPtr<IXAudio2> m_xaudio2{};
        IXAudio2MasteringVoice* m_masteringVoice{};
        IXAudio2SourceVoice* m_sourceVoice{};
        std::vector<uint8_t> m_voiceBuffer{};
        std::mutex m_lock{};
        bool silentMode = false;
    };
}
