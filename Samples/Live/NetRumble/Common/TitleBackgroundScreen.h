//--------------------------------------------------------------------------------------
// StarfieldScreen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "GameScreen.h"

namespace NetRumble
{

class TitleBackgroundScreen : public GameScreen
{
public:
    TitleBackgroundScreen() noexcept;

    virtual void LoadContent() override;
    virtual void Reset() override;
    virtual void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;
    virtual void Draw(float totalTime, float elapsedTime) override;

private:
    TextureHandle m_title;
    TextureHandle m_background;
    DirectX::SimpleMath::Vector2 m_centerpoint = DirectX::SimpleMath::Vector2::Zero;
    int m_xdir = 1;
    int m_ydir = 1;
};

}
