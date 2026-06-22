//--------------------------------------------------------------------------------------
// StartMenuScreen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "MenuScreen.h"

namespace NetRumble
{
    class UserStartupScreen : public MenuScreen
    {
    public:
        UserStartupScreen();
        virtual ~UserStartupScreen() = default;

        void Reset() override;
        void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;
        void HandleInput() override;
        void Draw(float totalTime, float elapsedTime) override;
        void ExitScreen(bool immediate = false) override;

    protected:
        void OnCancel() override;

    private:
        void Intializing();
        void AcquireUserMenu();
        void WaitForNetworkInitialization();
        void SetPlayFabPartyUser();

	    enum class State
	    {
		    Initializing,
		    AcquireUser
	    };

	    State m_state;
        TextureHandle m_title;
        uint32_t m_signInCallbackToken;
    };
}
