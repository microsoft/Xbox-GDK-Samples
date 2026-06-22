// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

#include "MenuScreen.h"
#include "Texture.h"
#include "..\Helpers\UserManager.h"

namespace GameSaveSample
{
    class AcquireUserScreen : public MenuScreen
    {
    public:
        AcquireUserScreen(ScreenManager& screenManager, bool autoSignIn);
        virtual ~AcquireUserScreen();

        virtual void LoadContent( ATG::AssetLoadResources& loadRes ) override;
        virtual void Draw(ID3D12GraphicsCommandList* commandList, float totalTime, float elapsedTime) override;
        virtual void HandleInput(const DirectX::InputState& input) override;

    protected:
        virtual void OnCancel() override;

    private:
        enum class AcquireUserStatus
        {
            SigningIn,
            NeedsUserInteraction,
            Ready,
            Exiting
        };

        enum class SigninMethod
        {
            Silent,
            Normal
        };

        void AcquireUser();
        void SelectUser();
        void SwitchUser();
        void PrepareToExit();

        void OnCurrentUserChanged( ATG::UserManager::CurrentUserChangedResult result, std::shared_ptr<ATG::User>& oldUser,
           std::shared_ptr<ATG::User>& newUser );

        std::recursive_mutex                    m_menuLock;
        AcquireUserStatus                       m_status;

        ATG::AssetRef<DX::Texture>              m_backgroundTexture;
        ATG::AssetRef<DX::Texture>              m_titleLogo;
    };
}
