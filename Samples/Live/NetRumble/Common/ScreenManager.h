//--------------------------------------------------------------------------------------
// ScreenManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace NetRumble
{

class Manager;
class GameScreen;
class STTOverlayScreen;

class ScreenManager : public Manager
{
    using GameScreenList = std::vector<std::unique_ptr<GameScreen>>;
    using ScreensIterator = GameScreenList::iterator;
    using ReverseScreensIterator = GameScreenList::reverse_iterator;

public:
    ScreenManager() noexcept;
    ~ScreenManager() noexcept;

    void CreateWindowSizeDependentResources();

    // Call ExitScreen() on all screens, from topmost down, to gracefully shut them all down in preparation for a game state reset
    void ExitAllScreens();

    // Add a new screen to the manager with the specified controlling player. Pass -1 for controllingPlayer to enable all game pads to control the screen.
    void AddGameScreen(std::unique_ptr<GameScreen> screen);
    template<typename ScreenType, typename... Args>
    void AddGameScreen(Args... args)
    {
        static_assert(std::is_base_of_v<GameScreen, ScreenType>, "ScreenType must be dervied from GameScreen.");
        AddGameScreen(std::make_unique<ScreenType>(std::forward<Args>(args)...));
    }

    void AddBackgroundScreen(std::unique_ptr<GameScreen> screen);
    void AddForegroundScreen(std::unique_ptr<GameScreen> screen);

    inline void SetBackgroundsVisible(bool visible) { m_backgroundsVisible = visible; }
    inline void SetForegroundsVisible(bool visible) { m_foregroundsVisible = visible; }
    inline bool GetBackgroundsVisible() { return m_backgroundsVisible; }
    inline bool GetForegroundsVisible() { return m_foregroundsVisible; }

    // Removes a screen from the manager. You should normally use GameScreen::ExitScreen instead of calling this directly so the screen can gradually
    // transition off rather than just being instantly removed.
    //void RemoveScreen(const std::shared_ptr<GameScreen>& screen);

    // Return a pointer to the currently enabled game screen
    GameScreen* GetCurrentGameScreen() const;

    // Return a pointer to the STT window
    inline std::shared_ptr<STTOverlayScreen> GetSTTWindow() { return m_STTWindow; }

    // Display a pop-up error
    void ShowError(std::string_view message, std::function<void(void)> andthen = nullptr);

    // Updates all active screens in the ScreenManager
    void Update(DX::StepTimer const& timer);

    // Handle suspend and resume events
    void Suspend();
    void Resume();

    // Draws all active screens in the ScreenManager
    void Render(DX::StepTimer const& timer);

private:
    // Used by GameScreen base class to remove itself from the manager
    void RemoveScreen(GameScreen* screen);

    void AddPendingGameScreens();
    void RemovePendingGameScreens();

    // Background screens
    GameScreenList m_backgroundScreens;

    // Foreground screens
    GameScreenList m_foregroundScreens;

    // Stack of the current screens
    GameScreenList m_gameScreens;
    GameScreenList m_pendingGameScreensAdds;
    std::vector<GameScreen*> m_pendingGameScreensRemovals;

    // Screen for Speech To Text strings
    std::shared_ptr<STTOverlayScreen> m_STTWindow;

    // Set when ExitAllScreens is called
    bool m_screenStackCleared;

    bool m_backgroundsVisible;
    bool m_foregroundsVisible;

    std::mutex m_screenLock;

    friend class GameScreen;
};

}
