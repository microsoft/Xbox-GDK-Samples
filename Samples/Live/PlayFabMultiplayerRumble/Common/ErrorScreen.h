//--------------------------------------------------------------------------------------
// ErrorScreen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "GameScreen.h"

namespace PlayFabMultiplayerRumble
{

class ErrorScreen : public GameScreen
{
public:
    ErrorScreen(std::string_view message, std::function<void(void)> callback = nullptr);
    virtual ~ErrorScreen() = default;

    virtual void HandleInput() override;
    virtual void Draw(float totalTime, float elapsedTime) override;
    virtual void ExitScreen(bool immediate = false) override;

private:
    std::string _message;
    std::function<void(void)> _callback;
};

}
