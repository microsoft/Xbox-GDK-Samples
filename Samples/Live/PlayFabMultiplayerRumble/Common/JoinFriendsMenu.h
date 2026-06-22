//--------------------------------------------------------------------------------------
// ErrorScreen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "MenuScreen.h"

namespace PlayFabMultiplayerRumble
{

class JoinFriendsMenu : public MenuScreen
{
public:
    JoinFriendsMenu();
    virtual ~JoinFriendsMenu() = default;

    virtual void Draw(float totalTime, float elapsedTime) override;
    virtual void OnCancel() override;

private:
    std::vector<std::shared_ptr<OnlineUser>> m_friendsGames;
};

}
