//--------------------------------------------------------------------------------------
// AudioManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Manager.h"

namespace NetRumble
{

class AudioManager : public Manager
{
public:
    AudioManager() noexcept;

    void Initialize();
    void Suspend();
    void Resume();
    void ShutDown();
    void Tick();
    void PlaySoundTrack(bool play);
    void PlaySound(const std::wstring& soundName, bool loop = false);
    void SetMasterVolume(float volume);

    bool SoundTrackOn;
    bool PlaySoundEffects;

    void LoadSound(const std::wstring &name, const std::wstring &path);
private:
    std::unique_ptr<DirectX::AudioEngine>                         _audEngine;
    std::unique_ptr<DirectX::SoundEffect>                         _backgroundSound;
    std::unique_ptr<DirectX::SoundEffectInstance>                 _backgroundSoundInstance;
    std::map<std::wstring, std::shared_ptr<DirectX::SoundEffect>> _soundEffects;
};

}

