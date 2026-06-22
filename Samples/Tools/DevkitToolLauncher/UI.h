//--------------------------------------------------------------------------------------
// UI.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "UITK.h"
#include "LauncherCache.h"
#include "DirectoryBrowser.h"

namespace ATG
{
    enum class UIState
    {
        Browser,
        Launcher,
        Runtime
    };

    enum class UIInputCommand
    {
        NavUp,
        NavDown,
        NavLeft,
        NavRight,
        NavPageUp,
        NavPageDown,
        RefreshBrowser,
        ToggleAutoScroll,
        Select,
        Back
    };

    // This class initializes and managed all the UI for the DevkitToolLauncher sample.
    // The UI provides 3 screens for functionality:
    //  - Tool Browser: A directory navigation screen over a specified directory
    //  - Launch Settings: A screen to setup an executable launch with parameters
    //  - Runtime Log: A log screen that shows the output from a running executable
    class UI
    {
    public:

        // Callbacks so the UI can get the user to perform some work
        struct Callbacks
        {
            using SpawnToolProcess = std::function<void(const std::string& /*processCommandLine*/, const std::string& /*workingDir*/, bool /*usesGpu*/)>;
            using IsToolProcessActive = std::function<bool()>;
            using GetCommandLineProcess = std::function<std::string()>;
            using GetCommandLineWorkingDir = std::function<std::string()>;

            SpawnToolProcess            m_spawnToolProcessCallback;
            IsToolProcessActive         m_isToolProcessActiveCallback;
            GetCommandLineProcess       m_getCommandLineProcessCallback;
            GetCommandLineWorkingDir    m_getCommandLineWorkingDirCallback;
        };

    public:

        UI();
        ~UI();

        void Initialize(ATG::UITK::D3DResourcesProvider& resourcesProvider, int sizeX, int sizeY, bool jumpToRuntimeScreen, Callbacks callbacks);
        void Update(float elapsedSeconds);
        void Render();

        void HandleInputCommand(UIInputCommand command);
        void Log(const char* text);

        void OnSuspending();
        void OnResuming();

    protected:

        // General methods
        void SwitchTo(UIState state);

        // UITK helper methods
        void UITK_NavUp();
        void UITK_NavDown();
        void UITK_NavLeft();
        void UITK_NavRight();

        // Browser Methods
        void Browser_Initialize();
        void Browser_Update();
        void Browser_HandleInputCommand(UIInputCommand command);
        void Browser_UpdateUIData();
        void Browser_NavUp();
        void Browser_NavDown();
        void Browser_NavLeft();
        void Browser_NavRight();
        void Browser_Select();
        void Browser_Refresh();

        // Launcher Methods
        bool Launcher_Initialize(std::shared_ptr<File> file);
        void Launcher_Update();
        void Launcher_HandleInputCommand(UIInputCommand command);
        void Launcher_Launch(bool gpu);
        void Launcher_Cancel();
        void Launcher_SpawnVirtualKeyboard();

        // Runtime Methods
        void Runtime_Initialize(bool getDisplayDataFromCallback = false);
        void Runtime_Update();
        void Runtime_HandleInputCommand(UIInputCommand command);
        void Runtime_UpdateUIData();
        void Runtime_UpdateButtonPromptUIData();
        void Runtime_ScrollUp();
        void Runtime_ScrollDown();
        void Runtime_ScrollPageUp();
        void Runtime_ScrollPageDown();
        void Runtime_ToggleAutoScroll();
        void Runtime_Log(const char* text);
        void Runtime_ClearLog();
        void Runtime_ReturnToBrowser();

    protected:

        // Callbacks
        Callbacks                                           m_callbacks;

        // UITK
        ATG::UITK::UIManager                                m_uiManager;
        ATG::UITK::UIInputState                             m_inputState;
        ATG::UITK::UIElementPtr                             m_browserLayout;
        ATG::UITK::UIElementPtr                             m_launcherLayout;
        ATG::UITK::UIElementPtr                             m_runtimeLayout;

        // Overall Info
        UIState                                             m_state;
        XTaskQueueHandle                                    m_taskQueue;

        // Browser Info
        std::shared_ptr<ATG::UITK::UIStackPanel>            m_browserVerticalStack;
        int                                                 m_browserElementCount;
        int                                                 m_browserStartOffset;
        DirectoryBrowser::Iterator                          m_browserIter;
        std::unique_ptr<DirectoryBrowser>                   m_browser;

        // Launcher Info
        std::string                                         m_exePath;
        std::string                                         m_exeCommandLine;
        std::string                                         m_exeWorkingDir;
        UITK::UIElementPtr                                  m_launcherExeEntryPanel;
        std::shared_ptr<UITK::UIStaticText>                 m_launcherExeEntryText;
        UITK::UIElementPtr                                  m_launcherCommandLineEntryPanel;
        std::shared_ptr<UITK::UIStaticText>                 m_launcherCommandLineEntryText;
        UITK::UIElementPtr                                  m_launcherWorkingDirEntryPanel;
        std::shared_ptr<UITK::UIStaticText>                 m_launcherWorkingDirEntryText;
        UITK::UIElementPtr                                  m_launcherLaunchCPUOnlyButton;
        UITK::UIElementPtr                                  m_launcherLaunchCPUAndGPUButton;
        UITK::UIElementPtr                                  m_launcherCancelButton;
        std::unique_ptr<LauncherCache>                      m_launcherCache;

        // Runtime Info
        std::shared_ptr<UITK::UIStaticText>                 m_runtimeExeCommandlineText;
        std::shared_ptr<UITK::UIStaticText>                 m_runtimeWorkingDirText;
        std::shared_ptr<UITK::UIStaticText>                 m_runtimeButtonPromptText;
        std::vector<std::shared_ptr<UITK::UIStaticText>>    m_runtimeLogTexts;

        // Runtime Log data
        std::vector<std::string>                            m_log;
        size_t                                              m_logMaxBufferHistory;
        size_t                                              m_logRenderIndex;
        bool                                                m_logAutoScroll;
    };

}
