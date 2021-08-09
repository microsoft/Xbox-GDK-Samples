//--------------------------------------------------------------------------------------
// GameScreens.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "UIWidgets.h"
#include "GameUser.h"

// Forward declare the sample class
class Sample;

namespace ATG
{
    class GameScreen
    {
    public:

        GameScreen(Sample* sample, ATG::UITK::UIManager* uiManager);
        virtual ~GameScreen();

        virtual void Initialize() = 0;
        virtual void Cleanup() = 0;

        virtual void Update(double dt) = 0;
        virtual void Render() = 0;

    public:

        // Notifications called from GameScreenManager to notify of events that happen for users
        virtual void NotifySignInResult(HRESULT /*result*/, XUserHandle /*newUserHandle*/, bool /*primaryUser*/, bool /*wasDefaultSignIn*/) {};
        virtual void NotifySignedOut(XUserHandle /*userHandle*/) {};
        virtual void NotifyGamerPicUpdated(XUserHandle /*userHandle*/) {};

    protected:

        Sample* m_sample;
        ATG::UITK::UIManager* m_uiManager;
        ATG::UITK::UIElementPtr m_screen;
    };

    struct UserData
    {
        UserData()
            : m_userHandle(nullptr)
            , m_localId{ 0 }
            , m_gamertag{}
            , m_guest(false)
            , m_gamerpicUploaded(false)
            , m_gamerpicAsyncInProgress(false)
            , m_gamerpicAsyncHandle(0)
            , m_signOutDeferralHandle(nullptr)
            , m_signOutDeferralRemaining(0)
            , m_requestDeferral(false)
        {
        }

        XUserHandle                     m_userHandle;
        XUserLocalId                    m_localId;
        char                            m_gamertag[XUserGamertagComponentUniqueModernMaxBytes + 1];
        bool                            m_guest;

        bool                            m_gamerpicUploaded;
        bool                            m_gamerpicAsyncInProgress;
        GameUserManager::AsyncHandle    m_gamerpicAsyncHandle;

        XUserSignOutDeferralHandle      m_signOutDeferralHandle;
        double                          m_signOutDeferralRemaining;
        std::vector<uint8_t>            m_gamerPic;
        bool                            m_requestDeferral;
    };

    class UserView
    {
    public:
        UserView(Sample* sample, ATG::UITK::UIElementPtr userPrefab);
        ~UserView();

        void Update(std::vector<APP_LOCAL_DEVICE_ID> devices);
        bool LeavePressed(std::vector<APP_LOCAL_DEVICE_ID> devices);
        bool DeferralPressed(std::vector<APP_LOCAL_DEVICE_ID> devices);
        void Render(XUserHandle user);
        void SetGamerPic(uint8_t* gamerpicData, size_t gamerpicSize);
        void SetNoUserText(const char* newText);
        void SetPrimaryUser(bool isPrimaryUser);

    protected:
        Sample*                                     m_sample;
        ATG::UITK::UIElementPtr                     m_userPrefab;
        ATG::UITK::UIElementPtr                     m_userPanel;
        std::shared_ptr<ATG::UITK::UIStaticText>    m_noUserText;
        std::shared_ptr<ATG::UITK::UIButton>        m_inputButton;
        std::shared_ptr<ATG::UITK::UIButton>        m_leaveButton;
        std::shared_ptr<ATG::UITK::UIButton>        m_deferralButton;
        std::shared_ptr<ATG::UITK::UIStaticText>    m_inputGlyphs;
        std::shared_ptr<ATG::UITK::UICheckBox>      m_deferralCheckbox;
        std::shared_ptr<ATG::UITK::UIStaticText>    m_deferralActiveText;
        std::shared_ptr<ATG::UITK::UIButton>        m_userSelectedButton;

    };


    class DevicesView
    {
    public:
        DevicesView(Sample* sample, ATG::UITK::UIElementPtr devicesPrefab);
        ~DevicesView();

        void Render();
        void SetVisible(bool isVisible);
        void SetGamerPic(uint8_t* gamerpicData, size_t gamerpicSize);

    protected:
        Sample* m_sample;
        ATG::UITK::UIElementPtr m_devicesPrefab;
        ATG::UITK::UIElementPtr m_primaryUserPanel;
        ATG::UITK::UIElementPtr m_devicesListPanel;
        std::shared_ptr<ATG::UITK::UIStaticText> m_gamerpic;
        std::shared_ptr<ATG::UITK::UIStaticText> m_gamertag;
        static const int deviceListSize = 6;
        ATG::UITK::UIElementPtr m_devicesList[deviceListSize];
    };

    class GameScreenTitle : public GameScreen
    {
    public:
        enum class TitleState
        {
            SignIn,         // No user is signed in. Prompts for sign-in.
            NoUserWarning,  // Canceled sign-in. Prompt to continue without user.
            MenuWithUser,   // Menu state with a primary user.
            MenuWithoutUser // Menu state without a user.
        };

    public:

        GameScreenTitle(Sample* sample, ATG::UITK::UIManager* uiManager);
        ~GameScreenTitle() override;

        void Initialize() override;
        void Cleanup() override;

        void Update(double dt) override;
        void Render() override;

        void NotifySignInResult(HRESULT result, XUserHandle newUserHandle, bool primaryUser, bool wasDefaultSignIn) override;
        void NotifySignedOut(XUserHandle userHandle) override;
        void NotifyGamerPicUpdated(XUserHandle userHandle) override;

        void NavigateToNextOption();
        void NavigateToPrevOption();
        void SelectCurrentOption();

    protected:

        void UpdateSignIn();
        void UpdateNoUserWarning();
        void UpdateMenu();

        void RenderNoUserWarning();
        void RenderMenu();

        void SwitchTitleState(TitleState newState);

        void HandleSingleUser();
        void HandleMultiUser();
        void HandleCrossRestart();

    protected:

        TitleState  m_titleState;

        //UI
        std::shared_ptr<ATG::UITK::UIButton> m_startButton;

        ATG::UITK::UIElementPtr m_warningScreen;
        std::shared_ptr<ATG::UITK::UIButton> m_warningSignInButton;
        std::shared_ptr<ATG::UITK::UIButton> m_warningContinueButton;
        std::shared_ptr<ATG::UITK::UIButton> m_warningSelectedOption;

        ATG::UITK::UIElementPtr m_menuScreen;
        std::shared_ptr<ATG::UITK::UIButton> m_menuSingleUserButton;
        std::shared_ptr<ATG::UITK::UIButton> m_menuMultipleUserButton;
        std::shared_ptr<ATG::UITK::UIButton> m_menuRestartButton;
        ATG::UITK::UIElementPtr m_restartDescriptionText;
        std::shared_ptr<ATG::UITK::UIButton> m_menuSelectedOption;

    };

    class GameScreenSingleUser : public GameScreen
    {
    public:

        enum class SingleUserState
        {
            Main,                   // Main state with or without an active user
            PromptForNewUser,       // Lost user. Prompt user to establish a new user or return to title
            PromptForUserChange,    // Tried to change user as a result of establishing gamepad via sign-in. Prompt to accept user change, or cancel user change.
        };

    public:

        GameScreenSingleUser(Sample* sample, ATG::UITK::UIManager* uiManager);
        ~GameScreenSingleUser() override;

        void Initialize() override;
        void Cleanup() override;

        void Update(double dt) override;
        void Render() override;

        void NotifySignInResult(HRESULT result, XUserHandle newUserHandle, bool primaryUser, bool wasDefaultSignIn) override;
        void NotifySignedOut(XUserHandle userHandle) override;
        void NotifyGamerPicUpdated(XUserHandle userHandle) override;

    protected:

        void UpdateMain();
        void UpdateNewUserPrompt();
        void UpdateUserChangePrompt();

        void RenderMain();
        void RenderNewUserPrompt();
        void RenderUserChangePrompt();

    protected:

        SingleUserState             m_state;
        UserData                    m_cachedUserData;
        std::unique_ptr<UserView>   m_playerScreen;
        ATG::UITK::UIElementPtr     m_navigateLegend;
        ATG::UITK::UIElementPtr     m_selectLegend;
        ATG::UITK::UIElementPtr     m_unpairedGamepadLegend;
        ATG::UITK::UIElementPtr     m_noUserPanel;
    };

    class GameScreenMultiUser : public GameScreen
    {
    public:

        GameScreenMultiUser(Sample* sample, ATG::UITK::UIManager* uiManager);
        ~GameScreenMultiUser() override;

        void Initialize() override;
        void Cleanup() override;

        void Update(double dt) override;
        void Render() override;

        void NotifySignInResult(HRESULT result, XUserHandle newUserHandle, bool primaryUser, bool wasDefaultSignIn) override;
        void NotifySignedOut(XUserHandle userHandle) override;
        void NotifyGamerPicUpdated(XUserHandle userHandle) override;

    protected:

        bool    m_signingIn;
        std::unique_ptr<UserView>   m_playerScreens[4];
    };

    enum EGameScreen
    {
        EGS_Title = 0,
        EGS_SingleUser = 1,
        EGS_MultiUser = 2,

        EGS_Count
    };

    class GameScreenManager
    {
    public:

        static const unsigned int s_maxUsers = 4;

    public:

        GameScreenManager(Sample* sample, ATG::UITK::UIManager* uiManager);
        ~GameScreenManager();

        void Update(double dt);
        void Render();

        void OnSuspend();
        void OnResume();

        GameScreen* GetCurrentGameScreen() const;
        void GoToScreen(EGameScreen screen);

        DevicesView* GetDevicesView();
    public:

        void HandleCrossRestart();
        void ToggleDeferralRequest(XUserHandle userHandle);

        void TryDefaultPrimaryUserSignIn(bool autoTransitionToSinglePlayer = false);
        bool SignInPrimaryUserWithUI();
        bool SignInExtraUserWithUI(bool allowGuests);
        void SignOutUser(unsigned int userIndex);
        void SignOutUser(XUserHandle userHandle);
        void SignOutAllUsers();
        void MakeUserPrimary(unsigned int userIndex);
        void MakeUserPrimary(XUserHandle userHandle);

        bool HaveUser(XUserHandle userHandle) const;
        bool HaveUser(unsigned int userIndex) const;
        bool HavePrimaryUser() const;
        bool GetUserIndex(XUserHandle userHandle, unsigned int* outUserIndex) const;
        unsigned int GetUserCount() const;
        unsigned int GetPrimaryUserIndex() const;
        XUserHandle GetPrimaryUserHandle() const;
        XUserHandle GetUserHandle(unsigned int userIndex) const;
        bool GetUserData(XUserHandle userHandle, UserData* outUserData) const;
        bool GetUserData(unsigned int userIndex, UserData* outUserData) const;

        std::string GetUserCombinedInputString(XUserHandle userHandle);
        std::string GetInputDeviceGlyphsString(const APP_LOCAL_DEVICE_ID& deviceId);
        std::string GetInputDeviceIdString(const APP_LOCAL_DEVICE_ID& deviceId, bool shortDeviceId);

        std::vector<APP_LOCAL_DEVICE_ID> GetUnpairedDevices(std::vector<XUserHandle> knownUsers) const;
        std::vector<APP_LOCAL_DEVICE_ID> GetUnpairedDevices() const;

    protected:

        void InitUserData(XUserHandle m_newUserHandle, bool primaryUser);
        void UpdateGamerPic(unsigned int userIndex);
        void UpdateSignOutDeferral(double dt);

    protected:

        static void UserEventCallback(void* context, XUserHandle userHandle, XUserChangeEvent event);

    protected:

        Sample*                                                     m_sample;

        std::unique_ptr<GameScreen>                                 m_gameScreens[EGS_Count];
        int                                                         m_currentGameScreen;
        int                                                         m_pendingGameScreen;
        std::unique_ptr<DevicesView>                                m_devicesView;

        // Users and user events
        UserData                                                    m_userData[s_maxUsers];
        GameUserManager::CallbackHandle                             m_userEventCallbackHandle;
    };

}
