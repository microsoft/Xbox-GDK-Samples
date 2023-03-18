//--------------------------------------------------------------------------------------
// NLSAndLocalization.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "NLSAndLocalization.h"

// ATG TK
#include "ATGColors.h"
#include "Texture.h"
#include "Json.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run and initialize UIs
void Sample::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    InitializeUI();

    EnumerateCurrentLocalizationSettings();
    ChangeToDefaultLocalization();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

#ifdef _GAMING_XBOX
    m_deviceResources->WaitForOrigin();
#endif

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    m_mouse->EndOfInputFrame();

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    // Push logs to UI
    for (size_t i = 0; i < m_logQueue.size(); ++i)
    {
        m_consoleWindow->AppendLineOfText(m_logQueue[i]);
    }
    m_logQueue.clear();

    m_uiInputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_uiInputState);

    auto buttons = m_uiInputState.GetGamePadButtons(0);
    if (buttons.view == GamePad::ButtonStateTracker::PRESSED)
    {
        ExitSample();
    }

    auto keys = m_uiInputState.GetKeyboardKeys();
    if (keys.IsKeyPressed(DirectX::Keyboard::Keys::Escape))
    {
        ExitSample();
    }

    PIXEndEvent();
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // Render the UI scene
    m_uiManager.Render();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnActivated()
{
}

void Sample::OnDeactivated()
{
}

void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_uiInputState.Reset();
}

void Sample::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

// Properties
void Sample::GetDefaultSize(int& width, int& height) const
{
    width = 1920;
    height = 1080;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // Create the style renderer for the UI manager to use for rendering the UI scene styles
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Notify the UI manager of the current window size
    auto os = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(os.right, os.bottom);
}

void Sample::OnDeviceLost()
{
    m_uiManager.GetStyleManager().ResetStyleRenderer();
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion

// Display log in console window UI
void Sample::Log(const char* text)
{
    std::lock_guard<std::mutex> scopeLock(m_logMutex);

    if (m_consoleWindow)
    {
        m_logQueue.push_back(text);
    }
}

// Load UI layouts and initialize UI buttons
void Sample::InitializeUI()
{
    auto root = m_uiManager.GetRootElement()->AddChildFromLayout("Assets/Layouts/layout.json");

    m_consoleWindow = root->GetChildById(ID("Output_Console_Window_Outer_Panel"))->GetTypedChildById<UIConsoleWindow>(ID("Output_Console_Window"));
    m_localizedText = root->GetTypedChildById<UIStaticText>(ID("Localization_Label"));
    m_localizedImage = root->GetTypedChildById<UIImage>(ID("Localization_Image"));

    m_buttonStackPanel = root->GetChildById(ID("Button_Stack_Panel"));    
    for (int i = 0; i < NUMLANGUAGES; i++)
    {
        SetupButtonHandler(
            *m_buttonStackPanel->GetTypedChildById<UIButton>(ID(c_supportedLanguageInfos[i].buttonId)),
            c_supportedLanguageInfos[i].buttonDescription,
            [&, i]() {ChangeLocalization(c_supportedLanguageInfos[i]); });
    }
        
    SetupButtonHandler(
        *m_buttonStackPanel->GetTypedChildById<UIButton>(ID("Enum_Button_1")),
        "Enumerate current localization settings",
        [&]() { EnumerateCurrentLocalizationSettings(); });
}

// Configure label of UI button and add event listener to UI button
void Sample::SetupButtonHandler(ATG::UITK::UIButton& button, std::string displayText, std::function<void()> onClicked)
{
    button.GetTypedSubElementById<UIStaticText>(ID("Test_Button_Label"))->SetDisplayText(displayText);
    button.ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [&, onClicked](UIButton* /*button*/)
    {
        onClicked();

        button.GetTypedSubElementById<UIStaticText>(ID("Test_Button_Label"))->SetStyleId(ID("Button_Label_Style"));
    });
    button.ButtonState().AddListenerWhen(UIButton::State::Focused,
        [&](UIButton* /*button*/)
    {
        button.GetTypedSubElementById<UIStaticText>(ID("Test_Button_Label"))->SetStyleId(ID("Button_Label_Style_Dark"));
    });
    button.ButtonState().AddListenerWhen(UIButton::State::Hovered,
        [&](UIButton* /*button*/)
    {
        button.GetTypedSubElementById<UIStaticText>(ID("Test_Button_Label"))->SetStyleId(ID("Button_Label_Style_Dark"));
    });
    button.ButtonState().AddListenerWhen(UIButton::State::Normal,
        [&](UIButton* /*button*/)
    {
        button.GetTypedSubElementById<UIStaticText>(ID("Test_Button_Label"))->SetStyleId(ID("Button_Label_Style"));
    });
}

// Retrieve default locale with XPackageGetUserLocale and change in-game localization resources to the default locale
// The best-matching locale is retrieved from the locales defined in MicrosoftGame.config
void Sample::ChangeToDefaultLocalization()
{
    // Get the best user locale
    char locale[LOCALE_NAME_MAX_LENGTH] = {};
    HRESULT hr = XPackageGetUserLocale(LOCALE_NAME_MAX_LENGTH, locale);
    if (SUCCEEDED(hr))
    {
        // Simply compare the result of XPackageGetUserLocale
        for (int i = 0; i < NUMLANGUAGES; i++)
        {
            if (strncmp(locale, c_supportedLanguageInfos[i].locale, LOCALE_NAME_MAX_LENGTH) == 0)
            {
                char buffer[256];
                sprintf_s(buffer, "In-game localization resources matching XPackageGetUserLocale are found: %s", c_supportedLanguageInfos[i].locale);
                Log(buffer);
                ChangeLocalization(c_supportedLanguageInfos[i]);

                return;
            }
        }

        char buffer[256];
        sprintf_s(buffer, "In-game localization resources matching XPackageGetUserLocale aren't found: default to %s", c_supportedLanguageInfos[0].locale);
        Log(buffer);        
        ChangeLocalization(c_supportedLanguageInfos[0]);

        return;        
    }
    else
    {        
        char errorMessage[256];
        sprintf_s(errorMessage, "XPackageGetUserLocale failed with error: 0x%08X", hr);
        Log(errorMessage);
#ifdef _GAMING_DESKTOP
        Log("Please make sure the application is not launched directly from exe file");
#endif
    }
}

// Load localized strings in the given language and change in-game resources to the localized resources
void Sample::ChangeLocalization(const LanguageInfo& languageInfo)
{
    LoadLocalizedStrings(languageInfo.localizedStringFilePath);
    
    // Change strings and image in UI to the localized resources
    m_localizedText->SetDisplayText(m_localizedStrings["string_main"]);
    m_localizedImage->SetStyleId(ID(m_localizedStrings["id_imageStyle"]));

    m_buttonStackPanel->GetTypedChildById<UIButton>(ID(m_currentLanguageInfo->buttonId))->SetStyleId(ID("Basic_Button_Style"));
    m_buttonStackPanel->GetTypedChildById<UIButton>(ID(languageInfo.buttonId))->SetStyleId(ID("Selected_Button_Style"));
    m_currentLanguageInfo = &languageInfo;

    char buffer[256];
    sprintf_s(buffer, "Changing in-game localization to: %s", languageInfo.locale);
    Log(buffer);
}

// Load localized strings from json file
void Sample::LoadLocalizedStrings(const wchar_t* stringFile)
{    
    std::ifstream fileStream;
    fileStream.open(stringFile);
    if (fileStream.is_open())
    {
        json jsonData;
        fileStream >> jsonData;

        m_localizedStrings.clear();
        for (auto iter = jsonData.begin(); iter != jsonData.end(); ++iter)
        {
            std::string key = iter.key();
            std::string value = iter.value();
            m_localizedStrings[key] = value;
        }
        fileStream.close();
    }
    else
    {
        throw std::invalid_argument("Localized string file in the path is not found.");
    }
}

// Display results of common APIs for localization
void Sample::EnumerateCurrentLocalizationSettings()
{
    Log("\n");

    char buffer[256];

    //GetUserDefaultLocaleName retrieves the locale that the console is set in
    wchar_t systemLocaleName[LOCALE_NAME_MAX_LENGTH];
    GetUserDefaultLocaleName(systemLocaleName, LOCALE_NAME_MAX_LENGTH);
    sprintf_s(buffer, "Console language setting - GetUserDefaultLocaleName: %ls", systemLocaleName);
    Log(buffer);

    // GetUserDefaultGeoName retrieves the location that the console is set in
    wchar_t systemGeoName[LOCALE_NAME_MAX_LENGTH];
    GetUserDefaultGeoName(systemGeoName, LOCALE_NAME_MAX_LENGTH);
    sprintf_s(buffer, "Console location setting - GetUserDefaultGeoName: %ls", systemGeoName);
    Log(buffer);

    // XPackageGetUserLocale retrieves the user locale that most closely matches the package locale
    char bestUserLocale[LOCALE_NAME_MAX_LENGTH];
    HRESULT hr = XPackageGetUserLocale(LOCALE_NAME_MAX_LENGTH, bestUserLocale);
    if (SUCCEEDED(hr))
    {
        sprintf_s(buffer, "Preferred language for the application - XPackageGetUserLocale: %s", bestUserLocale);
        Log(buffer);

        // Locale can be converted into LCID
        wchar_t wBestUserLocale[LOCALE_NAME_MAX_LENGTH];
        mbstowcs_s(NULL, wBestUserLocale, LOCALE_NAME_MAX_LENGTH, bestUserLocale, LOCALE_NAME_MAX_LENGTH);
        LANGID primary = 0, sublang = 0;
        unsigned long lcid = LocaleNameToLCID(wBestUserLocale, 0);
        if (lcid != 0)
        {
            primary = (LANGID)PRIMARYLANGID(lcid);
            sublang = (LANGID)SUBLANGID(lcid);
            sprintf_s(buffer, "     LocaleNameToLCID: 0x%x Primary ID: 0x%x Sublanguage: 0x%x", lcid, primary, sublang);
            Log(buffer);
        }
        else
        {
            HRESULT lastError = HRESULT_FROM_WIN32(GetLastError());
            char errorMessage[256];
            sprintf_s(errorMessage, "LocaleNameToLCID failed with error: 0x%08X\n", lastError);
            Log(errorMessage);
        }
    }
    else
    {
        char errorMessage[256];
        sprintf_s(errorMessage, "XPackageGetUserLocale failed with error: 0x%08X", hr);        
        Log(errorMessage);
#ifdef _GAMING_DESKTOP
        Log("Please make sure the application is not launched directly from exe file");
#endif
    }
   
    Log("\n");
}

