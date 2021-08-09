//--------------------------------------------------------------------------------------
// GameScreens.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "GameScreens.h"
#include "UserManagement.h"

using namespace ATG;
using namespace DirectX;
using namespace ATG::UITK;

#pragma region Game Screen

GameScreen::GameScreen(Sample* sample, UIManager* uiManager)
    : m_sample(sample)
    , m_uiManager(uiManager)
{
}

GameScreen::~GameScreen()
{
}

#pragma endregion

#pragma region Game Screen - Title

GameScreenTitle::GameScreenTitle(Sample* sample, UIManager* uiManager)
    : GameScreen(sample, uiManager)
    , m_titleState(TitleState::SignIn)
{
    m_screen = m_uiManager->LoadLayoutFromFile("Assets/Layouts/title-screen.json");
    m_startButton = m_screen->GetTypedSubElementById<UIButton>(ID("start_button"));

    // Get the console sandbox so it can be displayed.
    char sandboxId[XSystemXboxLiveSandboxIdMaxBytes];
    size_t ignore;
    DX::ThrowIfFailed(XSystemGetXboxLiveSandboxId(XSystemXboxLiveSandboxIdMaxBytes, sandboxId, &ignore));

    // Fill in the sandbox element. Changing the sandbox requires a restart so this only needs to happen once.
    char sandboxDisplayString[13 + XSystemXboxLiveSandboxIdMaxBytes];
    sprintf_s(sandboxDisplayString, "Sandbox ID: %s", sandboxId);
    auto sandboxIdTextElement = m_screen->GetTypedSubElementById<UIStaticText>(ID("sandbox_text"));
    if (sandboxIdTextElement)
    {
        sandboxIdTextElement->SetDisplayText(sandboxDisplayString);
    }

    m_uiManager->AttachTo(m_screen, m_uiManager->GetRootElement());
    m_screen->SetVisible(false);

    m_warningScreen = m_uiManager->LoadLayoutFromFile("Assets/Layouts/warning-screen.json");
    m_uiManager->AttachTo(m_warningScreen, m_uiManager->GetRootElement());
    m_warningScreen->SetVisible(false);
    m_warningSignInButton = m_warningScreen->GetTypedSubElementById<UIButton>(ID("sign_in_button"));
    m_warningContinueButton = m_warningScreen->GetTypedSubElementById<UIButton>(ID("continue_button"));
    m_warningSelectedOption = m_warningSignInButton;

    m_menuScreen = m_uiManager->LoadLayoutFromFile("Assets/Layouts/menu-screen.json");
    m_uiManager->AttachTo(m_menuScreen, m_uiManager->GetRootElement());
    m_menuScreen->SetVisible(false);
    auto menuSection = m_menuScreen->GetSubElementById(ID("menu_section"));
    m_menuSingleUserButton = menuSection->GetTypedSubElementById<UIButton>(ID("single_user_button"));
    m_menuMultipleUserButton = menuSection->GetTypedSubElementById<UIButton>(ID("multiple_user_button"));
    m_menuRestartButton = menuSection->GetTypedSubElementById<UIButton>(ID("restart_button"));
    m_restartDescriptionText = menuSection->GetSubElementById(ID("restart_description_text"));
    m_menuSelectedOption = m_menuSingleUserButton;

}

GameScreenTitle::~GameScreenTitle()
{
}

void GameScreenTitle::Initialize()
{
    m_screen->SetVisible(true);

    auto gameScreenManager = m_sample->GetGameScreenManager();

    if (gameScreenManager)
    {
        if (gameScreenManager->HavePrimaryUser())
        {
            SwitchTitleState(GameScreenTitle::TitleState::MenuWithUser);
        }
        else
        {
            SwitchTitleState(GameScreenTitle::TitleState::SignIn);
        }
    }
}

void GameScreenTitle::Cleanup()
{
    m_screen->SetVisible(false);
}

void GameScreenTitle::Update(double /*dt*/)
{
    switch (m_titleState)
    {
    case GameScreenTitle::TitleState::SignIn:
        UpdateSignIn();
        break;
    case GameScreenTitle::TitleState::MenuWithUser:
    case GameScreenTitle::TitleState::MenuWithoutUser:
        UpdateMenu();
        break;
    case GameScreenTitle::TitleState::NoUserWarning:
        UpdateNoUserWarning();
        break;
    default:
        break;
    };
}

void GameScreenTitle::UpdateSignIn()
{
    const bool signInPressed = m_sample->GetGameInputCollection()->CheckInputAllDevices(
        [](const DirectX::GamePad::ButtonStateTracker& state)
        {
            return state.a == GamePad::ButtonStateTracker::PRESSED;
        });

    if (signInPressed)
    {
        m_sample->GetGameScreenManager()->TryDefaultPrimaryUserSignIn(false);
    }
}

void GameScreenTitle::UpdateNoUserWarning()
{
    assert(m_sample->GetGameScreenManager()->GetUserCount() == 0);

    const bool selectPressed = m_sample->GetGameInputCollection()->CheckInputAllDevices(
        [](const DirectX::GamePad::ButtonStateTracker& state)
        {
            return state.a == GamePad::ButtonStateTracker::PRESSED;
        });
    const bool upPressed = m_sample->GetGameInputCollection()->CheckInputAllDevices(
        [](const DirectX::GamePad::ButtonStateTracker& state)
        {
            return state.dpadUp == GamePad::ButtonStateTracker::PRESSED;
        });
    const bool downPressed = m_sample->GetGameInputCollection()->CheckInputAllDevices(
        [](const DirectX::GamePad::ButtonStateTracker& state)
        {
            return state.dpadDown == GamePad::ButtonStateTracker::PRESSED;
        });

    if (selectPressed && m_warningSignInButton == m_warningSelectedOption)
    {
        SwitchTitleState(GameScreenTitle::TitleState::SignIn);
    }
    else if(selectPressed && m_warningContinueButton == m_warningSelectedOption)
    {
        SwitchTitleState(GameScreenTitle::TitleState::MenuWithoutUser);
    }

    if (upPressed)
    {
        m_warningSelectedOption = m_warningSignInButton;
    }
    if (downPressed)
    {
        m_warningSelectedOption = m_warningContinueButton;
    }
}

void GameScreenTitle::UpdateMenu()
{
    const bool downPressed = m_sample->GetGameInputCollection()->CheckInputAllDevices(
        [](const DirectX::GamePad::ButtonStateTracker& state)
        {
            return state.dpadDown == GamePad::ButtonStateTracker::PRESSED;
        });

    const bool upPressed = m_sample->GetGameInputCollection()->CheckInputAllDevices(
        [](const DirectX::GamePad::ButtonStateTracker& state)
        {
            return state.dpadUp == GamePad::ButtonStateTracker::PRESSED;
        });

    if (upPressed)
    {
        NavigateToPrevOption();
    }
    else if (downPressed)
    {
        NavigateToNextOption();
    }

    const bool aPressed = m_sample->GetGameInputCollection()->CheckInputAllDevices(
        [](const DirectX::GamePad::ButtonStateTracker& state)
        {
            return state.a == GamePad::ButtonStateTracker::PRESSED;
        });

    if (aPressed)
    {
        SelectCurrentOption();
    }

    const bool switchUserPressed = m_sample->GetGameInputCollection()->CheckInputAllDevices(
        [](const DirectX::GamePad::ButtonStateTracker& state)
        {
            return state.y == GamePad::ButtonStateTracker::PRESSED;
        });

    if (switchUserPressed)
    {
        m_sample->GetGameScreenManager()->SignInPrimaryUserWithUI();
    }
}

void GameScreenTitle::Render()
{
    switch (m_titleState)
    {
    case GameScreenTitle::TitleState::SignIn:
        //There is no additional rendering necessary for the SignIn screen other than the UITK layout
        break;
    case GameScreenTitle::TitleState::MenuWithUser:
    case GameScreenTitle::TitleState::MenuWithoutUser:
        RenderMenu();
        break;
    case GameScreenTitle::TitleState::NoUserWarning:
        RenderNoUserWarning();
        break;
    default:
        break;
    };
}

void GameScreenTitle::RenderNoUserWarning()
{
    //Change the styles of button subelements when the button is focused.
    //Currently there are issues transfering focus between screens in the UITK so focus and
    //style changes are being adjusted 'manually' in this sample but I hope to update this soon
    //with fixes to rely on more UITK functionality.
    std::shared_ptr<UIButton> buttonList[] = { m_warningSignInButton, m_warningContinueButton };
    auto warningButtonsListLength = _countof(buttonList);
    for (unsigned int i = 0; i < warningButtonsListLength; i++)
    {
        if (buttonList[i] == m_warningSelectedOption)
        {
            buttonList[i]->SetStyleId(ID("button_focused_style"));
            buttonList[i]->GetSubElementById(ID("button_border"))->SetVisible(true);
            buttonList[i]->GetSubElementById(ID("button_label"))->SetStyleId(ID("button_focused_text_style"));
        }
        else
        {
            buttonList[i]->SetStyleId(ID("button_default_style"));
            buttonList[i]->GetSubElementById(ID("button_border"))->SetVisible(false);
            buttonList[i]->GetSubElementById(ID("button_label"))->SetStyleId(ID("button_default_text_style"));
        }
    }
}

void GameScreenTitle::RenderMenu()
{
    m_sample->GetGameScreenManager()->GetDevicesView()->Render();

    std::shared_ptr<UIButton> buttonList[] = { m_menuSingleUserButton, m_menuMultipleUserButton, m_menuRestartButton };
    auto menuButtonsListLength = _countof(buttonList);
    for (unsigned int i = 0; i < menuButtonsListLength; i++)
    {
        if (buttonList[i] == m_menuSelectedOption)
        {
            buttonList[i]->SetStyleId(ID("button_focused_style"));
            buttonList[i]->GetSubElementById(ID("button_border"))->SetVisible(true);
            buttonList[i]->GetSubElementById(ID("button_label"))->SetStyleId(ID("button_focused_text_style"));
        }
        else
        {
            buttonList[i]->SetStyleId(ID("button_default_style"));
            buttonList[i]->GetSubElementById(ID("button_border"))->SetVisible(false);
            buttonList[i]->GetSubElementById(ID("button_label"))->SetStyleId(ID("button_default_text_style"));
        }
    }

    if (m_sample->GetGameScreenManager()->HavePrimaryUser() == false)
    {
        m_menuMultipleUserButton->SetStyleId(ID("button_disabled_style"));
        m_menuMultipleUserButton->GetSubElementById(ID("button_label"))->SetStyleId(ID("button_disabled_text_style"));
    }

    m_restartDescriptionText->SetVisible(m_menuRestartButton == m_menuSelectedOption);
}

void GameScreenTitle::NotifySignInResult(HRESULT result, XUserHandle /*newUserHandle*/, bool primaryUser, bool wasDefaultSignIn)
{
#ifdef _DEBUG
    assert(primaryUser);
#else
    (void)primaryUser;
#endif

    if (SUCCEEDED(result))
    {
        SwitchTitleState(GameScreenTitle::TitleState::MenuWithUser);
    }
    else if (wasDefaultSignIn && result == E_GAMEUSER_NO_DEFAULT_USER)
    {
        // If default failed, just try again with the UI
        m_sample->GetGameScreenManager()->SignInPrimaryUserWithUI();
    }
    else if(result == E_ABORT && m_titleState == GameScreenTitle::TitleState::SignIn)
    {
        SwitchTitleState(GameScreenTitle::TitleState::NoUserWarning);
    }
    else
    {
        char logBuffer[512] = {};
        sprintf_s(logBuffer, 512, u8"GameScreenTitle::NotifySignInResult() : Failed to sign in with error hr=0x%08x\n", result);
        OutputDebugStringA(logBuffer);
    }
}

void GameScreenTitle::NotifySignedOut(XUserHandle userHandle)
{
    if (m_sample->GetGameScreenManager()->GetPrimaryUserHandle() == userHandle)
    {
        SwitchTitleState(GameScreenTitle::TitleState::SignIn);
    }
}

void GameScreenTitle::NotifyGamerPicUpdated(XUserHandle userHandle)
{
    auto gameScreenManager = m_sample->GetGameScreenManager();

    if (gameScreenManager->GetPrimaryUserHandle() == userHandle)
    {
        UserData updatedUser;
        gameScreenManager->GetUserData(userHandle, &updatedUser);
        gameScreenManager->GetDevicesView()->SetGamerPic(updatedUser.m_gamerPic.data(), updatedUser.m_gamerPic.size());
    }
}

void GameScreenTitle::NavigateToNextOption()
{
    // Skip multi-user if no user is signed in
    if (m_menuSelectedOption == m_menuSingleUserButton && m_sample->GetGameScreenManager()->HavePrimaryUser())
    {
        m_menuSelectedOption = m_menuMultipleUserButton;
    }
    else
    {
        m_menuSelectedOption = m_menuRestartButton;
    }
}
void GameScreenTitle::NavigateToPrevOption()
{
    // Skip multi-user if no user is signed in
    if (m_menuSelectedOption == m_menuRestartButton && m_sample->GetGameScreenManager()->HavePrimaryUser())
    {
        m_menuSelectedOption = m_menuMultipleUserButton;
    }
    else
    {
        m_menuSelectedOption = m_menuSingleUserButton;
    }
}

void GameScreenTitle::SelectCurrentOption()
{
    if (m_menuSelectedOption == m_menuSingleUserButton)
    {
        HandleSingleUser();
    }
    else if (m_menuSelectedOption == m_menuMultipleUserButton)
    {
        HandleMultiUser();
    }
    else
    {
        HandleCrossRestart();
    }
}

void GameScreenTitle::SwitchTitleState(TitleState newState)
{
    m_titleState = newState;
    switch (m_titleState)
    {
    case GameScreenTitle::TitleState::SignIn:
        m_screen->SetVisible(true);
        m_screen->SetEnabled(true);

        m_warningScreen->SetVisible(false);
        m_warningScreen->SetEnabled(false);

        m_menuScreen->SetVisible(false);
        m_menuScreen->SetEnabled(false);

        m_sample->GetGameScreenManager()->GetDevicesView()->SetVisible(false);
        break;
    case GameScreenTitle::TitleState::MenuWithUser:
    case GameScreenTitle::TitleState::MenuWithoutUser:
        m_screen->SetVisible(false);
        m_screen->SetEnabled(false);

        m_warningScreen->SetVisible(false);
        m_warningScreen->SetEnabled(false);

        m_menuScreen->SetVisible(true);
        m_menuScreen->SetEnabled(true);

        m_sample->GetGameScreenManager()->GetDevicesView()->SetVisible(true);
        break;
    case GameScreenTitle::TitleState::NoUserWarning:
        m_warningScreen->SetVisible(true);
        m_warningScreen->SetEnabled(true);

        m_screen->SetVisible(true);
        m_screen->SetEnabled(false);

        m_menuScreen->SetVisible(false);
        m_menuScreen->SetEnabled(false);

        m_sample->GetGameScreenManager()->GetDevicesView()->SetVisible(false);
        break;
    default:
        break;
    };
}

void GameScreenTitle::HandleSingleUser()
{
    m_sample->GetGameScreenManager()->GoToScreen(EGS_SingleUser);
}

void GameScreenTitle::HandleMultiUser()
{
    if (m_sample->GetGameScreenManager()->HavePrimaryUser())
    {
        m_sample->GetGameScreenManager()->GoToScreen(EGS_MultiUser);
    }
}

void GameScreenTitle::HandleCrossRestart()
{
    // For games that have a launcher or need to restart for special circumstances, XLaunchNewGame() can be used with a specified user and
    // arguments to support this scenario. This sample shows of that functionality by restarting the current title with the specified user
    // as the default user. Then, the restart will transition directly to the single user screen.
    m_sample->GetGameUserManager()->SignInUser(
        [&](HRESULT result, XUserHandle newUser)
        {
            if (FAILED(result))
            {
                return;
            }

            // The debugger will disconnect here as the current process is ending and a new one is spawned
            XLaunchNewGame(u8"UserManagement.exe", u8"-crossrestart", newUser);
        }, false);
}

#pragma endregion

#pragma region User View
UserView::UserView(Sample* sample, UIElementPtr userPrefab)
    : m_sample(sample)
    , m_userPrefab(userPrefab)
{
    m_userPanel = m_userPrefab->GetSubElementById(ID("have_user_panel"));
    m_noUserText = m_userPrefab->GetTypedSubElementById<UIStaticText>(ID("no_user_text"));
    m_inputButton = m_userPanel->GetTypedSubElementById<UIButton>(ID("button_input"));
    m_inputGlyphs = m_userPanel->GetTypedSubElementById<UIStaticText>(ID("active_button_label"));
    m_leaveButton = m_userPanel->GetTypedSubElementById<UIButton>(ID("leave_button"));
    m_deferralButton = m_userPanel->GetTypedSubElementById<UIButton>(ID("deferral_checkbox_button"));
    m_deferralCheckbox = m_deferralButton->GetSubElementById(ID("deferral_panel"))->GetTypedSubElementById<UICheckBox>(ID("deferral_toggle"));
    m_deferralActiveText = m_userPanel->GetTypedSubElementById<UIStaticText>(ID("deferral_active_text"));
    m_userSelectedButton = m_inputButton;
}
UserView::~UserView()
{
}

bool UserView::LeavePressed(std::vector<APP_LOCAL_DEVICE_ID> devices)
{
    if (m_userSelectedButton == m_leaveButton)
    {
        return m_sample->GetGameInputCollection()->CheckInput(devices,
            [](const DirectX::GamePad::ButtonStateTracker& state)
            {
                return state.a == GamePad::ButtonStateTracker::PRESSED;
            });
    }
    return false;
}

bool UserView::DeferralPressed(std::vector<APP_LOCAL_DEVICE_ID> devices)
{
    if (m_userSelectedButton == m_deferralButton)
    {
        return m_sample->GetGameInputCollection()->CheckInput(devices,
            [](const DirectX::GamePad::ButtonStateTracker& state)
            {
                return state.a == GamePad::ButtonStateTracker::PRESSED;
            });
    }
    return false;
}

void UserView::SetGamerPic(uint8_t* gamerpicData, size_t gamerpicSize)
{
    auto gamerpicImage = m_userPanel->GetTypedSubElementById<UIImage>(ID("profile_pic"));
    gamerpicImage->UseTextureData(gamerpicData, gamerpicSize);
}
void UserView::SetNoUserText(const char* newText)
{
    m_noUserText->SetDisplayText(newText);
}

void UserView::SetPrimaryUser(bool isPrimaryUser)
{
    //we make the primary user icon visible or invisible
    m_userPanel->GetSubElementById(ID("primary_image"))->SetVisible(isPrimaryUser);
    //and change the utton to either 'leave' or 'main menu'
    auto buttonText = m_leaveButton->GetTypedSubElementById<UIStaticText>(ID("button_label"));
    if (isPrimaryUser)
    {
        buttonText->SetDisplayText("Main Menu");
    }
    else
    {
        buttonText->SetDisplayText("Leave");
    }
}

void UserView::Update(std::vector<APP_LOCAL_DEVICE_ID> devices)
{
    const bool downPressed = m_sample->GetGameInputCollection()->CheckInput(devices,
        [](const DirectX::GamePad::ButtonStateTracker& state)
        {
            return state.dpadDown == GamePad::ButtonStateTracker::PRESSED;
        });

    const bool upPressed = m_sample->GetGameInputCollection()->CheckInput(devices,
        [](const DirectX::GamePad::ButtonStateTracker& state)
        {
            return state.dpadUp == GamePad::ButtonStateTracker::PRESSED;
        });

    if (upPressed)
    {
        if (m_userSelectedButton == m_deferralButton)
        {
            m_userSelectedButton = m_leaveButton;
        }
        else
        {
            m_userSelectedButton = m_inputButton;
        }
    }
    else if (downPressed)
    {
        if (m_userSelectedButton == m_inputButton)
        {
            m_userSelectedButton = m_leaveButton;
        }
        else
        {
            m_userSelectedButton = m_deferralButton;
        }
    }
}

void UserView::Render(XUserHandle user)
{
    if (user)
    {
        auto gameScreenManager = m_sample->GetGameScreenManager();
        UserData userData;
        gameScreenManager->GetUserData(user, &userData);

        m_userPanel->SetVisible(true);
        m_noUserText->SetVisible(false);
        //set user id
        char idText[2];
        sprintf_s(idText, "%llu", userData.m_localId.value);
        m_userPanel->GetSubElementById(ID("user_id_panel"))->GetTypedSubElementById<UIStaticText>(ID("user_id_text"))->SetDisplayText(idText);

        //set gamertag
        m_userPanel->GetTypedSubElementById<UIStaticText>(ID("gamer_tag"))->SetDisplayText(userData.m_gamertag);

        auto userDevices = m_sample->GetGameUserManager()->GetUserDevices(user);
        if (userDevices.size() == 0)
        {
            m_inputGlyphs->SetDisplayText("No paired gamepads");
        }
        else
        {
            //render input glyphs
            if (m_userSelectedButton == m_inputButton)
            {
                std::string userInputStr = gameScreenManager->GetUserCombinedInputString(user);
                m_inputGlyphs->SetDisplayText(userInputStr);
            }
            else
            {
                m_inputGlyphs->SetDisplayText("");
            }
        }

        //render either the deferral checkbox or the active deferral amount remaining
        if (userData.m_signOutDeferralHandle)
        {

            m_deferralButton->SetVisible(false);
            m_deferralActiveText->SetVisible(true);
            char deferralText[32];
            sprintf_s(deferralText, "Sign Out Deferral Active: %.0f", userData.m_signOutDeferralRemaining);
            m_deferralActiveText->SetDisplayText(deferralText);
        }
        else
        {
            m_deferralButton->SetVisible(true);
            m_deferralActiveText->SetVisible(false);
        }

        //loop through buttons and hilight or not
        std::shared_ptr<UIButton> buttonList[] = { m_inputButton, m_leaveButton, m_deferralButton };
        auto buttonsListLength = _countof(buttonList);
        for (unsigned int i = 0; i < buttonsListLength; i++)
        {
            if (buttonList[i] == m_userSelectedButton)
            {
                buttonList[i]->SetStyleId(ID("button_focused_style"));
                buttonList[i]->GetSubElementById(ID("button_border"))->SetVisible(true);
                buttonList[i]->GetSubElementById(ID("button_label"))->SetStyleId(ID("button_focused_text_style"));
            }
            else
            {
                buttonList[i]->SetStyleId(ID("button_default_style"));
                buttonList[i]->GetSubElementById(ID("button_border"))->SetVisible(false);
                buttonList[i]->GetSubElementById(ID("button_label"))->SetStyleId(ID("button_default_text_style"));
            }
        }
        if (userData.m_requestDeferral)
        {
            m_deferralCheckbox->SetStyleId(ID("checked_checkbox_style"));
        }
        else
        {
            m_deferralCheckbox->SetStyleId(ID("unchecked_checkbox_style"));
        }
    }
    else
    {
        m_userPanel->SetVisible(false);
        m_noUserText->SetVisible(true);
    }
}
#pragma endregion

#pragma region Devices View
DevicesView::DevicesView(Sample* sample, UIElementPtr devicesPrefab)
    : m_sample(sample)
    , m_devicesPrefab(devicesPrefab)
{
    m_primaryUserPanel = m_devicesPrefab->GetSubElementById(ID("primary_user_panel"));
    m_gamertag = m_primaryUserPanel->GetTypedSubElementById<UIStaticText>(ID("primary_user_name"));
    m_devicesListPanel = m_devicesPrefab->GetSubElementById(ID("devices_list"));
    char device[8];
    for (int i = 0; i < deviceListSize; i++)
    {
        sprintf_s(device, "device%d", i);
        m_devicesList[i] = m_devicesListPanel->GetSubElementById(ID(device));
    }
}
DevicesView::~DevicesView()
{
}

void DevicesView::SetGamerPic(uint8_t* gamerpicData, size_t gamerpicSize)
{
    auto gamerpicImage = m_primaryUserPanel->GetTypedSubElementById<UIImage>(ID("primary_user_picture"));
    gamerpicImage->UseTextureData(gamerpicData, gamerpicSize);
}

void DevicesView::SetVisible(bool isVisible)
{
    m_devicesPrefab->SetVisible(isVisible);
}

void DevicesView::Render()
{
    auto gameScreenManager = m_sample->GetGameScreenManager();
    const bool havePrimaryUser = gameScreenManager->HavePrimaryUser();
    auto gameUserManager = m_sample->GetGameUserManager();
    if (havePrimaryUser)
    {
        UserData userData;
        m_primaryUserPanel->SetVisible(true);
        gameScreenManager->GetUserData(gameScreenManager->GetPrimaryUserIndex(), &userData);
        m_gamertag->SetDisplayText(userData.m_gamertag);
    }
    else
    {
        m_primaryUserPanel->SetVisible(false);
    }

    int deviceIndex = 0;
    auto users = gameUserManager->GetUsers();
    for (auto user : users)
    {
        UserData userData;
        gameScreenManager->GetUserData(user, &userData);
        auto userDevices = gameUserManager->GetUserDevices(user);
        for (auto device : userDevices)
        {
            if (deviceIndex < deviceListSize)
            {
                m_devicesList[deviceIndex]->SetVisible(true);
                //set user id
                auto userIdPanel = m_devicesList[deviceIndex]->GetSubElementById(ID("user_id_panel"));
                userIdPanel->SetVisible(true);
                char userIdText[2];
                sprintf_s(userIdText, "%llu", userData.m_localId.value);
                auto userIdUIText = userIdPanel->GetTypedSubElementById<UIStaticText>(ID("user_id_text"));
                userIdUIText->SetDisplayText(userIdText);

                //set the device id
                auto deviceIdText = m_devicesList[deviceIndex]->GetTypedSubElementById<UIStaticText>(ID("device_id_text"));
                deviceIdText->SetDisplayText(gameScreenManager->GetInputDeviceIdString(device, true));

                //set the current input glyphs for this device
                auto deviceGlyphsText = m_devicesList[deviceIndex]->GetTypedSubElementById<UIStaticText>(ID("input_glyphs_text"));
                deviceGlyphsText->SetDisplayText(gameScreenManager->GetInputDeviceGlyphsString(device));

                deviceIndex++;
            }
        }
    }

    auto unpairedDevices = gameScreenManager->GetUnpairedDevices();
    for (auto unpairedDevice : unpairedDevices)
    {
        if (deviceIndex < deviceListSize)
        {
            m_devicesList[deviceIndex]->SetVisible(true);
            //make the userid invisible
            auto userIdPanel = m_devicesList[deviceIndex]->GetSubElementById(ID("user_id_panel"));
            userIdPanel->SetVisible(false);

            //set the device id
            auto deviceIdText = m_devicesList[deviceIndex]->GetTypedSubElementById<UIStaticText>(ID("device_id_text"));
            deviceIdText->SetDisplayText(gameScreenManager->GetInputDeviceIdString(unpairedDevice, true));

            //set the current input glyphs for this device
            auto deviceGlyphsText = m_devicesList[deviceIndex]->GetTypedSubElementById<UIStaticText>(ID("input_glyphs_text"));
            deviceGlyphsText->SetDisplayText(gameScreenManager->GetInputDeviceGlyphsString(unpairedDevice));

            deviceIndex++;
        }
    }

    //if no gamepads are connected let the user know
    if (deviceIndex == 0)
    {
        auto userIdPanel = m_devicesList[deviceIndex]->GetSubElementById(ID("user_id_panel"));
        userIdPanel->SetVisible(false);
        auto deviceIdText = m_devicesList[deviceIndex]->GetTypedSubElementById<UIStaticText>(ID("device_id_text"));
        deviceIdText->SetDisplayText("No Gamepads Connected");
        deviceIndex++;
    }

    for (; deviceIndex < deviceListSize; deviceIndex++)
    {
        m_devicesList[deviceIndex]->SetVisible(false);
    }
}
#pragma endregion

#pragma region Game Screen - Single User

GameScreenSingleUser::GameScreenSingleUser(Sample* sample, UIManager* uiManager)
    : GameScreen(sample, uiManager)
    , m_state(SingleUserState::Main)
{
    m_screen = m_uiManager->LoadLayoutFromFile("Assets/Layouts/singleuser-screen.json");
    m_uiManager->AttachTo(m_screen, m_uiManager->GetRootElement());
    m_screen->SetVisible(false);
    m_playerScreen = std::make_unique<UserView>(sample, m_screen->GetSubElementById(ID("users"))->GetSubElementById(ID("user1")));
    //in the single user case the user is always the primary user
    m_playerScreen->SetPrimaryUser(true);

    m_navigateLegend = m_screen->GetSubElementById(ID("navigate_legend"),true);
    m_selectLegend = m_screen->GetSubElementById(ID("select_legend"),true);
    m_unpairedGamepadLegend = m_screen->GetSubElementById(ID("unpaired_legend"), true);
    m_noUserPanel = m_screen->GetSubElementById(ID("no_user_panel"), true);
}

GameScreenSingleUser::~GameScreenSingleUser()
{
}

void GameScreenSingleUser::Initialize()
{
    // Always enter into the main state
    m_state = GameScreenSingleUser::SingleUserState::Main;
    m_screen->SetVisible(true);

    auto gameScreenManager = m_sample->GetGameScreenManager();
    if (gameScreenManager->HavePrimaryUser())
    {
        UserData primaryUser;
        gameScreenManager->GetUserData(gameScreenManager->GetPrimaryUserIndex(), &primaryUser);
        m_playerScreen->SetGamerPic(primaryUser.m_gamerPic.data(), primaryUser.m_gamerPic.size());
    }
}

void GameScreenSingleUser::Cleanup()
{
    m_screen->SetVisible(false);
}

void GameScreenSingleUser::Update(double /*dt*/)
{
    switch (m_state)
    {
    case GameScreenSingleUser::SingleUserState::Main:
        UpdateMain();
        break;
    case GameScreenSingleUser::SingleUserState::PromptForNewUser:
        UpdateNewUserPrompt();
        break;
    case GameScreenSingleUser::SingleUserState::PromptForUserChange:
        UpdateUserChangePrompt();
        break;
    default:
        break;
    };
}

void GameScreenSingleUser::UpdateMain()
{
    auto gameScreenManager = m_sample->GetGameScreenManager();
    auto gameUserManager = m_sample->GetGameUserManager();

    std::vector<APP_LOCAL_DEVICE_ID> userDevices;
    auto user = gameScreenManager->GetPrimaryUserHandle();
    if (user)
    {
        userDevices = gameUserManager->GetUserDevices(user);
        // Check if we need to sign in an unpaired gamepad
        if (userDevices.size() == 0)
        {
            const bool signIn = m_sample->GetGameInputCollection()->CheckInputAllDevices(
                [](const DirectX::GamePad::ButtonStateTracker& state)
                {
                    return state.a == GamePad::ButtonStateTracker::PRESSED;
                });

            if (signIn)
            {
                // Start an extra sign-in. If it ends up being a new user, the notification function will handle it.
                // If it's the same user (which is what we're hoping for), then it will just return the already-existing
                // primary user.
                gameScreenManager->SignInExtraUserWithUI(false);
            }
        }
        else
        {
            m_playerScreen->Update(userDevices);
            if (m_playerScreen->LeavePressed(userDevices))
            {
                gameScreenManager->GoToScreen(EGS_Title);
            }
            if (m_playerScreen->DeferralPressed(userDevices))
            {
                gameScreenManager->ToggleDeferralRequest(gameScreenManager->GetPrimaryUserHandle());
            }
        }
    }
    else
    {
        //If we enter 'gameplay' without a user just display device input
        //and let the user return to the main menu
        const bool returnToMenu = m_sample->GetGameInputCollection()->CheckInputAllDevices(
            [](const DirectX::GamePad::ButtonStateTracker& state)
            {
                return state.b == GamePad::ButtonStateTracker::PRESSED;
            });
        if (returnToMenu)
        {
            gameScreenManager->GoToScreen(EGS_Title);
        }
    }
}

void GameScreenSingleUser::UpdateNewUserPrompt()
{
    const bool returnToTitle = m_sample->GetGameInputCollection()->CheckInputAllDevices(
        [](const DirectX::GamePad::ButtonStateTracker& state)
        {
            return state.b == GamePad::ButtonStateTracker::PRESSED;
        });

    if (returnToTitle)
    {
        m_sample->GetGameScreenManager()->GoToScreen(EGS_Title);

        return;
    }

    const bool signIn = m_sample->GetGameInputCollection()->CheckInputAllDevices(
        [](const DirectX::GamePad::ButtonStateTracker& state)
        {
            return state.a == GamePad::ButtonStateTracker::PRESSED;
        });

    if (signIn)
    {
        assert(!m_sample->GetGameScreenManager()->HavePrimaryUser());

        m_sample->GetGameScreenManager()->SignInPrimaryUserWithUI();
    }
}

void GameScreenSingleUser::UpdateUserChangePrompt()
{
    auto gameScreenManager = m_sample->GetGameScreenManager();

    const bool switchPrimaryUser = m_sample->GetGameInputCollection()->CheckInputAllDevices(
        [](const DirectX::GamePad::ButtonStateTracker& state)
        {
            return state.a == GamePad::ButtonStateTracker::PRESSED;
        });
    const bool cancelPrimaryUserChange = m_sample->GetGameInputCollection()->CheckInputAllDevices(
        [](const DirectX::GamePad::ButtonStateTracker& state)
        {
            return state.b == GamePad::ButtonStateTracker::PRESSED;
        });

    if (switchPrimaryUser)
    {
        XUserHandle oldPrimaryUserHandle = gameScreenManager->GetPrimaryUserHandle();
        gameScreenManager->MakeUserPrimary(m_cachedUserData.m_userHandle);
        gameScreenManager->SignOutUser(oldPrimaryUserHandle);
        m_state = GameScreenSingleUser::SingleUserState::Main;
    }
    else if (cancelPrimaryUserChange)
    {
        gameScreenManager->SignOutUser(m_cachedUserData.m_userHandle);
        m_state = GameScreenSingleUser::SingleUserState::Main;
    }
}

void GameScreenSingleUser::Render()
{
    switch (m_state)
    {
    case GameScreenSingleUser::SingleUserState::Main:
        RenderMain();
        break;
    case GameScreenSingleUser::SingleUserState::PromptForNewUser:
        RenderNewUserPrompt();
        break;
    case GameScreenSingleUser::SingleUserState::PromptForUserChange:
        RenderUserChangePrompt();
        break;
    default:
        break;
    };
}

void GameScreenSingleUser::RenderMain()
{
    XUserHandle primaryUser = m_sample->GetGameScreenManager()->GetPrimaryUserHandle();
    if (!primaryUser)
    {
        m_playerScreen->SetNoUserText("       No User Signed In.  \n Press [B] to Return to Title");
        m_navigateLegend->SetVisible(false);
        m_selectLegend->SetVisible(false);
        m_noUserPanel->SetVisible(true);
        m_unpairedGamepadLegend->SetVisible(false);
    }
    else
    {
        m_navigateLegend->SetVisible(true);
        m_noUserPanel->SetVisible(false);

        //if the user does not have any paired devices show the legend for pairing a device
        auto userDevices = m_sample->GetGameUserManager()->GetUserDevices(primaryUser);
        m_selectLegend->SetVisible(userDevices.size() != 0);
        m_unpairedGamepadLegend->SetVisible(userDevices.size() == 0);
    }
    m_playerScreen->Render(primaryUser);
    m_sample->GetGameScreenManager()->GetDevicesView()->Render();
}

void GameScreenSingleUser::RenderNewUserPrompt()
{    
    char userString[512] = {};    
    sprintf_s(userString, 512, u8"%s is no longer signed in.\nSign in to continue?\n[A] Sign In   [B] Return to Title", m_cachedUserData.m_gamertag);
    m_playerScreen->SetNoUserText(userString);
    m_playerScreen->Render(nullptr);
    m_noUserPanel->SetVisible(true);
    m_navigateLegend->SetVisible(false);
    m_selectLegend->SetVisible(false);
    m_sample->GetGameScreenManager()->GetDevicesView()->Render();
}

void GameScreenSingleUser::RenderUserChangePrompt()
{
    auto gameScreenManager = m_sample->GetGameScreenManager();
    assert(gameScreenManager->HavePrimaryUser());
    assert(gameScreenManager->HaveUser(m_cachedUserData.m_userHandle));

    UserData newUserData;
    gameScreenManager->GetUserData(m_cachedUserData.m_userHandle, &newUserData);
    UserData primaryUserData;
    gameScreenManager->GetUserData(gameScreenManager->GetPrimaryUserIndex(), &primaryUserData);

    char userString[512] = {};
    sprintf_s(userString, 512,
        u8"%s is currently signed in and the primary user.\n Sign in %s as the new primary user?\n     [A] Switch Primary User     [B] Cancel Sign In",
        primaryUserData.m_gamertag,
        newUserData.m_gamertag);
    m_playerScreen->SetNoUserText(userString);
    m_playerScreen->Render(nullptr);
    m_noUserPanel->SetVisible(true);
    m_navigateLegend->SetVisible(false);
    m_selectLegend->SetVisible(false);
    m_sample->GetGameScreenManager()->GetDevicesView()->Render();
}

void GameScreenSingleUser::NotifySignInResult(HRESULT result, XUserHandle newUserHandle, bool primaryUser, bool /*wasDefaultSignIn*/)
{
#ifndef _DEBUG
    (void)primaryUser;
#endif

    if (FAILED(result))
    {
        return;
    }

    if (m_state == GameScreenSingleUser::SingleUserState::PromptForNewUser)
    {
        assert(primaryUser);

        // No need to do any fixup since the new user is automatically the primary user
        m_state = GameScreenSingleUser::SingleUserState::Main;
    }
    else if (m_state == GameScreenSingleUser::SingleUserState::Main)
    {
        // If here, then the user tried to sign-in from the main screen due to controller needing to be paired
        assert(!primaryUser);

        if (newUserHandle != m_sample->GetGameScreenManager()->GetPrimaryUserHandle())
        {
            // User tried to change users instead of just repair a new controller to the current user.
            // Change state to get a confirm/cancel from the user for this user change first.
            m_cachedUserData = UserData();
            m_cachedUserData.m_userHandle = newUserHandle;
            m_state = GameScreenSingleUser::SingleUserState::PromptForUserChange;
        }
    }
}

void GameScreenSingleUser::NotifySignedOut(XUserHandle userHandle)
{
    // Should be impossible to get a signout in this state since no user should be signed in
    assert(m_state != GameScreenSingleUser::SingleUserState::PromptForNewUser);

    auto gameScreenManager = m_sample->GetGameScreenManager();

    // If currently checking to see if the user intended to do a user switch when pairing a new gamepad and a user is signed out,
    // then the user intent is too difficult to discern and the sample will just kick to the title.
    if (m_state == GameScreenSingleUser::SingleUserState::PromptForUserChange)
    {
        gameScreenManager->SignOutAllUsers();
        gameScreenManager->GoToScreen(EGS_Title);
        return;
    }

    if (gameScreenManager->GetPrimaryUserHandle() == userHandle)
    {
        // XR-115 says that the title should either remove the player from the game and ensure good title state, or establish a new user.
        // To demonstrate both functionalities, the title will prompts for one or the other

        // Cache data as the primary user will be lost after this function returns
        gameScreenManager->GetUserData(m_sample->GetGameScreenManager()->GetPrimaryUserIndex(), &m_cachedUserData);

        // Switch state to the prompt
        m_state = GameScreenSingleUser::SingleUserState::PromptForNewUser;
    }
}

void GameScreenSingleUser::NotifyGamerPicUpdated(XUserHandle userHandle)
{
    auto gameScreenManager = m_sample->GetGameScreenManager();

    if (gameScreenManager->GetPrimaryUserHandle() == userHandle)
    {
        UserData updatedUser;
        gameScreenManager->GetUserData(userHandle, &updatedUser);
        m_playerScreen->SetGamerPic(updatedUser.m_gamerPic.data(), updatedUser.m_gamerPic.size());
        gameScreenManager->GetDevicesView()->SetGamerPic(updatedUser.m_gamerPic.data(), updatedUser.m_gamerPic.size());
    }
}



#pragma endregion

#pragma region Game Screen - Multi User


GameScreenMultiUser::GameScreenMultiUser(Sample* sample, UIManager* uiManager)
    : GameScreen(sample, uiManager)
    , m_signingIn(false)
{
    m_screen = m_uiManager->LoadLayoutFromFile("Assets/Layouts/multiuser-screen.json");
    m_uiManager->AttachTo(m_screen, m_uiManager->GetRootElement());
    m_screen->SetVisible(false);
    m_playerScreens[0] = std::make_unique<UserView>(sample, m_screen->GetSubElementById(ID("user1")));
    m_playerScreens[1] = std::make_unique<UserView>(sample, m_screen->GetSubElementById(ID("user2")));
    m_playerScreens[2] = std::make_unique<UserView>(sample, m_screen->GetSubElementById(ID("user3")));
    m_playerScreens[3] = std::make_unique<UserView>(sample, m_screen->GetSubElementById(ID("user4")));
    //The primary user will always be displayed in the top left spot
    m_playerScreens[0]->SetPrimaryUser(true);

}

GameScreenMultiUser::~GameScreenMultiUser()
{
}

void GameScreenMultiUser::Initialize()
{
    // Upon entering the multi-user screen, we should be guaranteed to have a primary user only
    assert(m_sample->GetGameUserManager()->NumUsers() == 1);
    m_screen->SetVisible(true);

    auto gameScreenManager = m_sample->GetGameScreenManager();
    UserData primaryUser;
    gameScreenManager->GetUserData(gameScreenManager->GetPrimaryUserIndex(), &primaryUser);
    m_playerScreens[0]->SetGamerPic(primaryUser.m_gamerPic.data(), primaryUser.m_gamerPic.size());
}

void GameScreenMultiUser::Cleanup()
{
    assert(!m_signingIn);

    // Sign out any signed-in extra users (not the primary user)
    for (unsigned int userIndex = 1; userIndex < m_sample->GetGameScreenManager()->s_maxUsers; ++userIndex)
    {
        m_sample->GetGameScreenManager()->SignOutUser(userIndex);
    }

    m_screen->SetVisible(false);
}

void GameScreenMultiUser::Update(double /*dt*/)
{
    auto gameScreenManager = m_sample->GetGameScreenManager();

    // Check for new sign-ins. This should be allowed if we're not at max users, or if a signed-in user doesn't have a paired gamepad
    bool userHasNoGamepads = false;
    for (unsigned int userIndex = 0; userIndex < gameScreenManager->s_maxUsers; ++userIndex)
    {
        if (gameScreenManager->HaveUser(userIndex) &&
            m_sample->GetGameUserManager()->GetUserDevices(gameScreenManager->GetUserHandle(userIndex)).size() == 0)
        {
            userHasNoGamepads = true;
            break;
        }
    }
    if (!m_signingIn &&
        (gameScreenManager->GetUserCount() < gameScreenManager->s_maxUsers || userHasNoGamepads))
    {
        const bool startNewSignIn = m_sample->GetGameInputCollection()->CheckInput(gameScreenManager->GetUnpairedDevices(),
            [](const DirectX::GamePad::ButtonStateTracker& state)
            {
                return state.a == GamePad::ButtonStateTracker::PRESSED;
            });

        if (startNewSignIn)
        {
            m_signingIn = gameScreenManager->SignInExtraUserWithUI(true);
        }
    }

    // Update each user looking for selection, signouts, and deferral changes
    for (unsigned int userIndex = 0; userIndex < gameScreenManager->s_maxUsers; ++userIndex)
    {
        if (!gameScreenManager->HaveUser(userIndex))
        {
            continue;
        }

        auto userDevices = m_sample->GetGameUserManager()->GetUserDevices(gameScreenManager->GetUserHandle(userIndex));
        m_playerScreens[userIndex]->Update(userDevices);

        if (m_playerScreens[userIndex]->LeavePressed(userDevices))
        {
            if (userIndex == gameScreenManager->GetPrimaryUserIndex())
            {
                m_sample->GetGameScreenManager()->GoToScreen(EGS_Title);
            }
            else
            {
                gameScreenManager->SignOutUser(userIndex);
            }
        }
        if (m_playerScreens[userIndex]->DeferralPressed(userDevices))
        {
            gameScreenManager->ToggleDeferralRequest(gameScreenManager->GetUserHandle(userIndex));
        }
    }
}

void GameScreenMultiUser::Render()
{
    auto gameScreenManager = m_sample->GetGameScreenManager();
    for (unsigned int userIndex = 0; userIndex < gameScreenManager->s_maxUsers; ++userIndex)
    {
        m_playerScreens[userIndex]->Render(gameScreenManager->GetUserHandle(userIndex));
    }
    m_sample->GetGameScreenManager()->GetDevicesView()->Render();
}

void GameScreenMultiUser::NotifySignInResult(HRESULT /*result*/, XUserHandle /*newUserHandle*/, bool /*primaryUser*/, bool /*wasDefaultSignIn*/)
{
    m_signingIn = false;
}

void GameScreenMultiUser::NotifySignedOut(XUserHandle userHandle)
{
    if (m_sample->GetGameScreenManager()->GetPrimaryUserHandle() == userHandle)
    {
        // XR-115 says that the title should either remove the player from the game and ensure good title state, or establish a new user.
        // In a multi-player experience, it is the best practice to allow other signed-in users to continue playing. For our case however,
        // we will just return to the title in the case that the primary user is removed as that is still compliant. See the single-player
        // screen to see another way of handling the user being lost
        m_sample->GetGameScreenManager()->GoToScreen(EGS_Title);
    }
}

void GameScreenMultiUser::NotifyGamerPicUpdated(XUserHandle userHandle)
{
    auto gameScreenManager = m_sample->GetGameScreenManager();
    unsigned int userIndex;
    if (gameScreenManager->GetUserIndex(userHandle, &userIndex))
    {
        UserData data;
        gameScreenManager->GetUserData(userIndex, &data);
        m_playerScreens[userIndex]->SetGamerPic(data.m_gamerPic.data(), data.m_gamerPic.size());
    }
    if (gameScreenManager->GetPrimaryUserHandle() == userHandle)
    {
        UserData updatedUser;
        gameScreenManager->GetUserData(userHandle, &updatedUser);
        gameScreenManager->GetDevicesView()->SetGamerPic(updatedUser.m_gamerPic.data(), updatedUser.m_gamerPic.size());
    }
}
#pragma endregion

#pragma region Game Screen Manager

GameScreenManager::GameScreenManager(Sample* sample, UIManager* uiManager)
    : m_sample(sample)
    , m_currentGameScreen(EGS_Title)
    , m_pendingGameScreen(-1)
    , m_userEventCallbackHandle(0)
{
    // Register callback for data changes
    m_userEventCallbackHandle = m_sample->GetGameUserManager()->RegisterUserEventCallback(&UserEventCallback, this);

    // Create the game screens
    m_gameScreens[EGS_Title] = std::make_unique<GameScreenTitle>(sample, uiManager);
    m_gameScreens[EGS_SingleUser] = std::make_unique<GameScreenSingleUser>(sample, uiManager);
    m_gameScreens[EGS_MultiUser] = std::make_unique<GameScreenMultiUser>(sample, uiManager);

    //all of the game screens need access to a single devices view
    auto devicesScreen = uiManager->LoadLayoutFromFile("Assets/Layouts/devices-panel.json");
    uiManager->AttachTo(devicesScreen, uiManager->GetRootElement());
    devicesScreen->SetVisible(false);
    m_devicesView = std::make_unique<DevicesView>(sample, devicesScreen);

    // Initialize the title screen
    GetCurrentGameScreen()->Initialize();
}

GameScreenManager::~GameScreenManager()
{
}

void GameScreenManager::Update(double dt)
{
    // Update deferral for each user
    UpdateSignOutDeferral(dt);

    GetCurrentGameScreen()->Update(dt);

    // Update any game screen change
    if (m_pendingGameScreen >= 0)
    {
        assert(m_pendingGameScreen >= EGS_Title && m_pendingGameScreen < EGS_Count);

        GetCurrentGameScreen()->Cleanup();
        m_currentGameScreen = m_pendingGameScreen;
        m_pendingGameScreen = -1;
        GetCurrentGameScreen()->Initialize();
    }
}

void GameScreenManager::Render()
{
    GetCurrentGameScreen()->Render();
}

void GameScreenManager::OnSuspend()
{
    // If there are any pending gamerpic requests, cancel them. If they fail from the suspend, they won't be useful.
    // If they succeed during the suspend, it will use rendering commands between SuspendX()/ResumeX() which isn't allowed.
    for (unsigned int userIndex = 0; userIndex < s_maxUsers; ++userIndex)
    {
        if (HaveUser(userIndex) && m_userData[userIndex].m_gamerpicAsyncInProgress)
        {
            m_sample->GetGameUserManager()->CancelGamerPicRequest(m_userData[userIndex].m_gamerpicAsyncHandle);
            m_userData[userIndex].m_gamerpicAsyncInProgress = false;
            m_userData[userIndex].m_gamerpicUploaded = false;
        }
    }
}

void GameScreenManager::OnResume()
{
    // Re-start a gamerpic request on resume if the gamerpic isn't uploaded
    for (unsigned int userIndex = 0; userIndex < s_maxUsers; ++userIndex)
    {
        if (HaveUser(userIndex) && !m_userData[userIndex].m_gamerpicUploaded)
        {
            UpdateGamerPic(userIndex);
        }
    }
}

GameScreen* GameScreenManager::GetCurrentGameScreen() const
{
    return m_gameScreens[m_currentGameScreen].get();
}

void GameScreenManager::GoToScreen(EGameScreen screen)
{
    assert(screen >= EGS_Title && screen < EGS_Count);

    if (screen == m_currentGameScreen)
    {
        return;
    }

    m_pendingGameScreen = screen;
}

DevicesView* GameScreenManager::GetDevicesView()
{
    return m_devicesView.get();
}


void GameScreenManager::HandleCrossRestart()
{
    TryDefaultPrimaryUserSignIn(true);
}

void GameScreenManager::ToggleDeferralRequest(XUserHandle userHandle)
{
    unsigned int userIndex = 0;
    GetUserIndex(userHandle, &userIndex);
    m_userData[userIndex].m_requestDeferral = !m_userData[userIndex].m_requestDeferral;
}


void GameScreenManager::TryDefaultPrimaryUserSignIn(bool autoTransitionToSinglePlayer)
{
    if (HavePrimaryUser())
    {
        return;
    }

    m_sample->GetGameUserManager()->SignInDefaultUser(
        [&, autoTransitionToSinglePlayer](HRESULT result, XUserHandle newUser)
        {
            if (SUCCEEDED(result))
            {
                assert(!HavePrimaryUser());

                InitUserData(newUser, true);

                if (autoTransitionToSinglePlayer)
                {
                    m_devicesView->SetVisible(true);
                    GoToScreen(EGS_SingleUser);
                }
                else
                {
                    GetCurrentGameScreen()->NotifySignInResult(result, newUser, true, true);
                }
            }
            else
            {
                GetCurrentGameScreen()->NotifySignInResult(result, newUser, true, true);
            }
        });
}

bool GameScreenManager::SignInPrimaryUserWithUI()
{
    // The primary user is always slot 0. Therefore, if a primary is already signed in, this will replace that user upon success

    return m_sample->GetGameUserManager()->SignInUser(
        [&](HRESULT result, XUserHandle newUser)
        {
            if (SUCCEEDED(result))
            {
                // If user is already signed-in, then a device association would occur in a callback
                // and no user lists needs to be changed.
                if (!HavePrimaryUser() || GetPrimaryUserHandle() != newUser)
                {
                    // If this is a new user, then adjust the lists of users
                    if (HavePrimaryUser())
                    {
                        SignOutUser(0u);
                    }

                    // The primary user should not already be an extra user
                    // If the primary user is lost, then all extra users should be removed as well upon returning to main menu
                    assert(!HaveUser(newUser));

                    InitUserData(newUser, true);
                }
            }

            // Regardless of success/fail, notify game screen of result
            GetCurrentGameScreen()->NotifySignInResult(result, newUser, true, false);
        }, false);
}

bool GameScreenManager::SignInExtraUserWithUI(bool allowGuests)
{
    assert(HavePrimaryUser());

    return m_sample->GetGameUserManager()->SignInUser(
        [&](HRESULT result, XUserHandle newUser)
        {
            if (SUCCEEDED(result))
            {
                // For pairing purposes, this function can be run even when at max use count for the title.
                // However, if we try to sign in more than max, cancel the sign-in
                if (GetUserCount() == s_maxUsers &&
                    !HaveUser(newUser))
                {
                    // Since we're not initializing data for this game screen manager, the user manager needs to close the handle
                    m_sample->GetGameUserManager()->SignOutUser(newUser);
                    result = E_ABORT;
                }
                else
                {
                    // If user is already signed-in, then a device association would occur in a callback
                    // and no user lists needs to be changed.
                    // If this is a new user, then adjust the lists of users
                    if (!HaveUser(newUser))
                    {
                        InitUserData(newUser, false);
                    }
                }
            }

            // Regardless of success/fail, notify game screen of result
            GetCurrentGameScreen()->NotifySignInResult(result, newUser, false, false);
        }, allowGuests);
}

void GameScreenManager::SignOutUser(unsigned int userIndex)
{
    if (!m_userData[userIndex].m_userHandle)
    {
        return;
    }

    if (m_userData[userIndex].m_gamerpicAsyncInProgress)
    {
        m_sample->GetGameUserManager()->CancelGamerPicRequest(m_userData[userIndex].m_gamerpicAsyncHandle);
    }
    if (m_userData[userIndex].m_signOutDeferralHandle)
    {
        XUserCloseSignOutDeferralHandle(m_userData[userIndex].m_signOutDeferralHandle);
    }
    m_sample->GetGameUserManager()->SignOutUser(m_userData[userIndex].m_userHandle);
    m_userData[userIndex] = UserData();
}

void GameScreenManager::SignOutUser(XUserHandle userHandle)
{
    unsigned int userIndex = 0;
    if (GetUserIndex(userHandle, &userIndex))
    {
        SignOutUser(userIndex);
    }
}

void GameScreenManager::SignOutAllUsers()
{
    for (unsigned int userIndex = 0; userIndex < s_maxUsers; ++userIndex)
    {
        SignOutUser(userIndex);
    }
}

void GameScreenManager::MakeUserPrimary(unsigned int userIndex)
{
    if (userIndex == 0)
    {
        return;
    }

    UserData oldPrimaryUserData = m_userData[0];
    m_userData[0] = m_userData[userIndex];
    m_userData[userIndex] = oldPrimaryUserData;
    UpdateGamerPic(0);
    UpdateGamerPic(userIndex);
}

void GameScreenManager::MakeUserPrimary(XUserHandle userHandle)
{
    unsigned int userIndex = 0;
    if (GetUserIndex(userHandle, &userIndex))
    {
        MakeUserPrimary(userIndex);
    }
}

bool GameScreenManager::HaveUser(XUserHandle userHandle) const
{
    unsigned int UserIndex = 0;
    return GetUserIndex(userHandle, &UserIndex);
}

bool GameScreenManager::HaveUser(unsigned int userIndex) const
{
    return m_userData[userIndex].m_userHandle != nullptr;
}

bool GameScreenManager::HavePrimaryUser() const
{
    return HaveUser(0u);
}

bool GameScreenManager::GetUserIndex(XUserHandle userHandle, unsigned int* outUserIndex) const
{
    assert(outUserIndex);

    for (unsigned int userIndex = 0; userIndex < s_maxUsers; ++userIndex)
    {
        if (m_userData[userIndex].m_userHandle == userHandle)
        {
            (*outUserIndex) = userIndex;
            return true;
        }
    }

    return false;
}

unsigned int GameScreenManager::GetUserCount() const
{
    unsigned int userCount = 0;
    for (unsigned int userIndex = 0; userIndex < s_maxUsers; ++userIndex)
    {
        if (HaveUser(userIndex))
        {
            ++userCount;
        }
    }

    return userCount;
}

unsigned int GameScreenManager::GetPrimaryUserIndex() const
{
    return 0;
}

XUserHandle GameScreenManager::GetPrimaryUserHandle() const
{
    return m_userData[GetPrimaryUserIndex()].m_userHandle;
}

XUserHandle GameScreenManager::GetUserHandle(unsigned int userIndex) const
{
    // User handle can be null if there is no user. Use HaveUser() to get a bool on whether user is valid or not.
    return m_userData[userIndex].m_userHandle;
}

bool GameScreenManager::GetUserData(XUserHandle userHandle, UserData* outUserData) const
{
    assert(outUserData);

    unsigned int userIndex = 0;
    if (GetUserIndex(userHandle, &userIndex))
    {
        return GetUserData(userIndex, outUserData);
    }

    return false;
}

bool GameScreenManager::GetUserData(unsigned int userIndex, UserData* outUserData) const
{
    assert(outUserData);

    if (HaveUser(userIndex))
    {
        (*outUserData) = m_userData[userIndex];
        return true;
    }

    return false;
}

std::string GameScreenManager::GetUserCombinedInputString(XUserHandle userHandle)
{
    std::string outString = "";
    auto& devices = m_sample->GetGameUserManager()->GetUserDevices(userHandle);
    for (auto& deviceId : devices)
    {
        outString += GetInputDeviceGlyphsString(deviceId);
    }
    return outString;
}

std::string GameScreenManager::GetInputDeviceGlyphsString(const APP_LOCAL_DEVICE_ID& deviceId)
{
    std::string outString;
    GamePad::ButtonStateTracker state;
    if (m_sample->GetGameInputCollection()->GetDeviceState(deviceId, &state))
    {
        if (state.dpadUp == GamePad::ButtonStateTracker::HELD)
        {
            outString += u8"[DPad]Up";
        }
        if (state.dpadDown == GamePad::ButtonStateTracker::HELD)
        {
            outString += u8"[DPad]Down";
        }
        if (state.dpadRight == GamePad::ButtonStateTracker::HELD)
        {
            outString += u8"[DPad]Right";
        }
        if (state.dpadLeft == GamePad::ButtonStateTracker::HELD)
        {
            outString += u8"[DPad]Left";
        }
        if (state.a == GamePad::ButtonStateTracker::HELD)
        {
            outString += u8"[A]";
        }
        if (state.b == GamePad::ButtonStateTracker::HELD)
        {
            outString += u8"[B]";
        }
        if (state.x == GamePad::ButtonStateTracker::HELD)
        {
            outString += u8"[X]";
        }
        if (state.y == GamePad::ButtonStateTracker::HELD)
        {
            outString += u8"[Y]";
        }
        if (state.leftShoulder == GamePad::ButtonStateTracker::HELD)
        {
            outString += u8"[LB]";
        }
        if (state.rightShoulder == GamePad::ButtonStateTracker::HELD)
        {
            outString += u8"[RB]";
        }
        if (state.leftStick == GamePad::ButtonStateTracker::HELD)
        {
            outString += u8"[LThumb]";
        }
        if (state.rightStick == GamePad::ButtonStateTracker::HELD)
        {
            outString += u8"[RThumb]";
        }
        if (state.menu == GamePad::ButtonStateTracker::HELD)
        {
            outString += u8"[Menu]";
        }
        if (state.view == GamePad::ButtonStateTracker::HELD)
        {
            outString += u8"[View]";
        }
    }
    return outString;
}

std::string GameScreenManager::GetInputDeviceIdString(const APP_LOCAL_DEVICE_ID& deviceId, bool shortDeviceId)
{
    char deviceIdStringBuffer[512] = {};
    if (shortDeviceId)
    {
        sprintf_s(deviceIdStringBuffer, 512, u8"{%08x...%08x}",
            *reinterpret_cast<const unsigned int*>(&deviceId.value[0]),
            *reinterpret_cast<const unsigned int*>(&deviceId.value[28])
        );
    }
    else
    {
        sprintf_s(deviceIdStringBuffer, 512, u8"DeviceId {%08x-%08x-%08x-%08x-%08x-%08x-%08x-%08x}",
            *reinterpret_cast<const unsigned int*>(&deviceId.value[0]),
            *reinterpret_cast<const unsigned int*>(&deviceId.value[4]),
            *reinterpret_cast<const unsigned int*>(&deviceId.value[8]),
            *reinterpret_cast<const unsigned int*>(&deviceId.value[12]),
            *reinterpret_cast<const unsigned int*>(&deviceId.value[16]),
            *reinterpret_cast<const unsigned int*>(&deviceId.value[20]),
            *reinterpret_cast<const unsigned int*>(&deviceId.value[24]),
            *reinterpret_cast<const unsigned int*>(&deviceId.value[28])
        );
    }
    return std::string(deviceIdStringBuffer);
}

std::vector<APP_LOCAL_DEVICE_ID> GameScreenManager::GetUnpairedDevices(std::vector<XUserHandle> knownUsers) const
{
    std::vector<APP_LOCAL_DEVICE_ID> outDevices = m_sample->GetGameInputCollection()->GetAllDevices();
    for (unsigned int userIndex = 0; userIndex < knownUsers.size(); ++userIndex)
    {
        if (!knownUsers[userIndex])
        {
            continue;
        }

        // Remove all paired devices as reported by users
        std::vector<APP_LOCAL_DEVICE_ID> userDevices = m_sample->GetGameUserManager()->GetUserDevices(knownUsers[userIndex]);
        for (unsigned int userDeviceIndex = 0; userDeviceIndex < userDevices.size(); ++userDeviceIndex)
        {
            for (unsigned int outDeviceIndex = 0; outDeviceIndex < outDevices.size(); ++outDeviceIndex)
            {
                // If the user device was found, remove it
                if (userDevices[userDeviceIndex] == outDevices[outDeviceIndex])
                {
                    outDevices.erase(outDevices.begin() + outDeviceIndex);
                    break;
                }
            }
        }
    }

    return outDevices;
}

std::vector<APP_LOCAL_DEVICE_ID> GameScreenManager::GetUnpairedDevices() const
{
    std::vector<XUserHandle> knownUsers;
    for (unsigned int userIndex = 0; userIndex < s_maxUsers; ++userIndex)
    {
        if (m_userData[userIndex].m_userHandle)
        {
            knownUsers.push_back(m_userData[userIndex].m_userHandle);
        }
    }

    return GetUnpairedDevices(knownUsers);
}

void GameScreenManager::InitUserData(XUserHandle m_newUserHandle, bool primaryUser)
{
#ifdef _DEBUG
    assert(GetUserCount() < s_maxUsers);
    if (primaryUser)
    {
        assert(HavePrimaryUser() == false);
    }
#endif

    // Primary user should always go in slot 0.
    // Extra users should go in 1+ arbitrarily.
    unsigned int userIndex = primaryUser ? 0u : 1u;
    unsigned int maxUsersToCheck = primaryUser ? 1u : s_maxUsers;
    for (; userIndex < maxUsersToCheck; ++userIndex)
    {
        if (!m_userData[userIndex].m_userHandle)
        {
            m_userData[userIndex].m_userHandle = m_newUserHandle;
            DX::ThrowIfFailed(XUserGetLocalId(m_userData[userIndex].m_userHandle, &m_userData[userIndex].m_localId));
            DX::ThrowIfFailed(XUserGetIsGuest(m_userData[userIndex].m_userHandle, &m_userData[userIndex].m_guest));
            size_t gamertagSize = 0;
            DX::ThrowIfFailed(XUserGetGamertag(m_userData[userIndex].m_userHandle, XUserGamertagComponent::UniqueModern, XUserGamertagComponentUniqueModernMaxBytes,
                m_userData[userIndex].m_gamertag, &gamertagSize));

            UpdateGamerPic(userIndex);

            return;
        }
    }

    // Shouldn't ever happen
    throw std::exception(u8"GameScreenManager::InitUserData - Sample error");
}

void GameScreenManager::UpdateGamerPic(unsigned int userIndex)
{
    if (!HaveUser(userIndex) || m_userData[userIndex].m_gamerpicAsyncInProgress)
    {
        return;
    }

    m_userData[userIndex].m_gamerpicUploaded = false;

    // Start async request
    m_userData[userIndex].m_gamerpicAsyncInProgress = m_sample->GetGameUserManager()->GetGamerPicAsync(m_userData[userIndex].m_userHandle, XUserGamerPictureSize::Medium,
        [&](HRESULT result, XUserHandle userHandle, std::vector<uint8_t> gamerpicBuffer)
        {
            unsigned int userIndex = 0;
            if (!GetUserIndex(userHandle, &userIndex))
            {
                return;
            }

            m_userData[userIndex].m_gamerpicAsyncInProgress = false;

            if (FAILED(result))
            {
                char logBuffer[512] = {};
                sprintf_s(logBuffer, 512, u8"GetGamerPicAsync Callback : Failed to get gamer pic for user %llu, Result = 0x%08x\n", m_userData[userIndex].m_localId.value, result);
                OutputDebugStringA(logBuffer);

                return;
            }

            //store the gamerpic so it can be set in the UI
            m_userData[userIndex].m_gamerPic = gamerpicBuffer;
            m_userData[userIndex].m_gamerpicUploaded = true;

            //Have the active screen update any gamerpics it is rendering
            GetCurrentGameScreen()->NotifyGamerPicUpdated(userHandle);

        }, &m_userData[userIndex].m_gamerpicAsyncHandle);
}

void GameScreenManager::UpdateSignOutDeferral(double dt)
{
    for (unsigned int userIndex = 0; userIndex < s_maxUsers; ++userIndex)
    {
        if (HaveUser(userIndex) && m_userData[userIndex].m_signOutDeferralHandle)
        {
            m_userData[userIndex].m_signOutDeferralRemaining -= dt;
            if (m_userData[userIndex].m_signOutDeferralRemaining <= 0.0)
            {
                m_userData[userIndex].m_signOutDeferralRemaining = 0.0;
                XUserCloseSignOutDeferralHandle(m_userData[userIndex].m_signOutDeferralHandle);
                m_userData[userIndex].m_signOutDeferralHandle = nullptr;

                // Clear the gamerpic flag just do it doesn't chance to flicker before the final sign-out is triggered
                m_userData[userIndex].m_gamerpicUploaded = false;
            }
        }
    }
}

void GameScreenManager::UserEventCallback(void* context, XUserHandle userHandle, XUserChangeEvent event)
{
    GameScreenManager* pThis = static_cast<GameScreenManager*>(context);

    unsigned int userIndex = 0;
    if (!pThis->GetUserIndex(userHandle, &userIndex))
    {
        return;
    }

    if (event == XUserChangeEvent::Gamertag)
    {
        size_t gamertagSize = 0;
        DX::ThrowIfFailed(XUserGetGamertag(pThis->m_userData[userIndex].m_userHandle, XUserGamertagComponent::UniqueModern, XUserGamertagComponentUniqueModernMaxBytes,
            pThis->m_userData[userIndex].m_gamertag, &gamertagSize));
    }
    else if (event == XUserChangeEvent::GamerPicture)
    {
        pThis->UpdateGamerPic(userIndex);
    }
    else if (event == XUserChangeEvent::SigningOut)
    {
        HRESULT hr = XUserGetSignOutDeferral(&pThis->m_userData[userIndex].m_signOutDeferralHandle);
        if (SUCCEEDED(hr))
        {
            if (pThis->m_userData[userIndex].m_requestDeferral == false)
            {
                return;
            }

            //Maintain the deferral for 5 seconds to demonstrate
            pThis->m_userData[userIndex].m_signOutDeferralRemaining = 5.0;

            char stringBuf[256] = {};
            sprintf_s(stringBuf, 256,
                u8"Deferring sign-out of user LocalId [%llu] for 5 seconds\n",
                pThis->m_userData[userIndex].m_localId.value);
            OutputDebugStringA(stringBuf);
        }
        else
        {
            char stringBuf[256] = {};
            sprintf_s(stringBuf, 256,
                u8"Failed to get sign out deferral for user LocalId [%llu] with hr=0x%x\n",
                pThis->m_userData[userIndex].m_localId.value,
                hr
            );
            OutputDebugStringA(stringBuf);
        }
    }
    else if (event == XUserChangeEvent::SignedOut)
    {
        // Notify current game screen before signing out in the manager so that the game screen can look up information it needs before it's lost
        pThis->GetCurrentGameScreen()->NotifySignedOut(userHandle);

        pThis->SignOutUser(userIndex);
    }
}

#pragma endregion
