//--------------------------------------------------------------------------------------
// STTOverlayScreen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once

#include "GameScreen.h"

namespace NetRumble
{

struct STTLineItem
{
    STTLineItem(std::string s, std::string m, bool t, bool f = false) :
        sender(s),
        message(m),
        transcribed(t),
        timeout(0.0f),
        fragment(f)
    {
    }

    std::string sender;
    std::string message;
    bool transcribed;
    float timeout;
    bool fragment;
};

class STTOverlayScreen : public GameScreen
{
public:
    STTOverlayScreen();
    virtual ~STTOverlayScreen();

    virtual void Draw(float totalTime, float elapsedTime) override;
    virtual void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;

    void AddSTTString(std::string sender, std::string message, bool transcribed, bool fragment = false);

    const float c_TextDisplayTimeout = 5.0f;
    const int c_MaxLinesToShow = 10;
    const float c_STTBoxLeft = .05f;
    const float c_STTBoxTop = .30f;

private:
    std::vector<STTLineItem> m_pendingLines;
    std::vector<STTLineItem> m_currentLines;
    TextureHandle m_textIcon;
    TextureHandle m_voiceIcon;
};

}
