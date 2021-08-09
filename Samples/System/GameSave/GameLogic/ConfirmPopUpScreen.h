// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

#include "MenuScreen.h"
#include "Texture.h"
#include <functional>

namespace GameSaveSample
{
    typedef std::function<void(bool isConfirmed)> ConfirmChoiceFn;

    class ConfirmPopUpScreen : public GameScreen
    {
    public:
        ConfirmPopUpScreen(ScreenManager& screenManager, const std::string& messageTitle, const std::string& message, ConfirmChoiceFn onChoose);
        virtual ~ConfirmPopUpScreen();

        virtual void LoadContent( ATG::AssetLoadResources& loadRes ) override;
        virtual void HandleInput(const DirectX::InputState& inputState) override;
        virtual void Draw(ID3D12GraphicsCommandList* commandList, float totalTime, float elapsedTime) override;

    private:
        ConfirmChoiceFn OnChoose;

        ATG::AssetRef<DX::Texture>              m_backgroundTexture;
        std::string                             m_displayMessage;
        std::string                             m_displayTitle;
    };
}
