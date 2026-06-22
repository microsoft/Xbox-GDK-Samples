// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

#include "GameScreen.h"
#include "InputState.h"
#include "Texture.h"

namespace GameSaveSample
{
    typedef std::function<void(bool adjustLeft)> MenuEntryAdjustFn;
    typedef std::function<void(int controllingPlayer)> MenuEntrySelectFn;

    class ScreenManager;

    class MenuEntry
    {
    public:
        MenuEntry(const std::string& label, MenuEntrySelectFn onSelect = nullptr, MenuEntryAdjustFn onAdjust = nullptr, const std::string& initialValue = u8"");
        virtual ~MenuEntry();

        MenuEntryAdjustFn OnAdjust; // bool adjustLeft == true on left thumbstick left OR dpad left, false on left thumbstick right OR dpad right
        MenuEntrySelectFn OnSelect;

        bool m_active;
        bool m_hasValue;
        std::string m_label;
        std::string m_value;
    };

    class MenuScreen : public GameScreen
    {
    public:
        MenuScreen() = delete;
        virtual ~MenuScreen();

        virtual void LoadContent( ATG::AssetLoadResources& loadRes ) override;
        virtual void HandleInput(const DirectX::InputState& input) override;
        virtual void Draw(ID3D12GraphicsCommandList* commandList, float totalTime, float elapsedTime) override;

    protected:
        MenuScreen(ScreenManager& screenManager);

        virtual void OnCancel() = 0;

        void ClearMenuEntries();
        virtual void ComputeMenuBounds(float viewportWidth, float viewportHeight);
        void ConfigureAsPopUpMenu();

        inline void SetCenterJustified(bool centerJustified)
        {
            m_drawCentered = centerJustified;
        }

        inline void SetMenuOffset(float xOffset, float yOffset)
        {
            m_menuOffset.x = xOffset;
            m_menuOffset.y = yOffset;
        }

        inline void SetTransitionDirections(bool transitionOnFromBelow, bool transitionOffTowardsBelow)
        {
            m_transitionOnMultiplier = transitionOnFromBelow ? 1 : -1;
            m_transitionOffMultiplier = transitionOffTowardsBelow ? 1 : -1;
        }

        bool                            m_animateSelected;
        bool                            m_menuActive;
        
        DX::RectangleF                  m_menuBounds;
        std::vector<MenuEntry>          m_menuEntries;
        float                           m_menuSpacing;
        float                           m_menuTextScale;
        bool                            m_showCurrentUser;

    private:
        DirectX::XMFLOAT2 ComputeDrawStartPosition(float viewportWidth, float viewportHeight);

        bool                                m_drawCentered;
        DirectX::XMFLOAT2                   m_menuOffset;
        size_t                              m_selectedEntry;
        int                                 m_transitionOnMultiplier;
        int                                 m_transitionOffMultiplier;
    };
}
