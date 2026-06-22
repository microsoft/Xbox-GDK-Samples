//--------------------------------------------------------------------------------------
// MenuScreen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "GameScreen.h"

namespace NetRumble
{

typedef std::function<void(bool adjustLeft)> MenuEntryAdjustFn;
typedef std::function<void()> MenuEntrySelectFn;

class MenuEntry
{
public:
    MenuEntry(std::string_view label, MenuEntrySelectFn onSelect = nullptr, MenuEntryAdjustFn onAdjust = nullptr, std::string_view initialValue = "");

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
    virtual ~MenuScreen() = default;;

    virtual void HandleInput() override;
    virtual void LoadContent() override;
    virtual void Draw(float totalTime, float elapsedTime) override;
    virtual void ReadCurrentEntry() override;

protected:
    MenuScreen();

    virtual void OnCancel() = 0;

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

    void DrawCurrentUser();

    bool                            m_animateSelected;
    DirectX::XMFLOAT4               m_menuBounds;
    std::vector<MenuEntry>          m_menuEntries;
    float                           m_menuTextScale;
    bool                            m_showCurrentUser;

private:
    DirectX::SimpleMath::Vector2 ComputeDrawStartPosition(float viewportWidth, float viewportHeight);

    bool                                m_drawCentered;
    DirectX::SimpleMath::Vector2        m_menuOffset;
    size_t                              m_selectedEntry;
    std::shared_ptr<DX::Texture> m_textureAdjustableEntryIndicatorLeft;
    int                                 m_transitionOnMultiplier;
    int                                 m_transitionOffMultiplier;
};

}
