// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

#include "MenuScreen.h"
#include "Texture.h"
#include "..\Assets.h"

namespace GameSaveSample
{
    class LaunchOptionsScreen : public MenuScreen
    {
    public:
        LaunchOptionsScreen(ScreenManager& screenManager);
	    virtual ~LaunchOptionsScreen();

        virtual void LoadContent( ATG::AssetLoadResources& loadRes ) override;
        virtual void Draw(ID3D12GraphicsCommandList* commandList, float totalTime, float elapsedTime) override;

    protected:
	    virtual void OnCancel() override;

    private:
        ATG::AssetRef<DX::Texture>     m_backgroundTexture;
        ATG::AssetRef<DX::Texture>     m_titleLogo;
    };
}
