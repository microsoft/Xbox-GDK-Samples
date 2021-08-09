// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

#include "GameScreen.h"

namespace GameSaveSample
{
    class ErrorPopUpScreen : public GameScreen
    {
    public:
        ErrorPopUpScreen(ScreenManager& screenManager, const std::string& message = nullptr);
        virtual ~ErrorPopUpScreen();

        virtual void LoadContent( ATG::AssetLoadResources& loadRes ) override;
        virtual void UnloadContent() override;
        virtual void HandleInput(const DirectX::InputState& inputState) override;
        virtual void Draw(ID3D12GraphicsCommandList* commandList, float totalTime, float elapsedTime) override;

    private:
        ATG::AssetRef<DX::Texture>              m_backgroundTexture;
        std::string                             m_errorMessage;
    };
}
