//--------------------------------------------------------------------------------------
// NLSAndLocalization.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#define NUMLANGUAGES 7

#include "DeviceResources.h"
#include "StepTimer.h"

// UITK
#include "UIWidgets.h"
#include "UIStyleRendererD3D.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify, public ATG::UITK::D3DResourcesProvider
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const;

    // UIStyleManager::D3DResourcesProvider interface methods
    virtual ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    virtual ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    virtual ID3D12GraphicsCommandList* GetCommandList() const override { return m_deviceResources->GetCommandList(); }

private:
       
    struct LanguageInfo
    {
        const char* locale;
        const char* buttonId;        
        const char* buttonDescription;        
        const wchar_t* localizedStringFilePath;
    };
  
    const LanguageInfo c_supportedLanguageInfos[NUMLANGUAGES] =
    {
        {"en-US",       "Loc_Button_1", "en-US: English (United States)",           L"Assets/InGameLocalizationResources/localizedStrings_en_US.json"},
        {"en-GB",       "Loc_Button_2", "en-GB: English (United Kingdom)",          L"Assets/InGameLocalizationResources/localizedStrings_en_GB.json"},
        {"zh-Hans-CN",  "Loc_Button_3", "zh-Hans-CN: Chinese (Simplified, China)",  L"Assets/InGameLocalizationResources/localizedStrings_zh_Hans_CN.json"},
        {"zh-Hant",     "Loc_Button_4", "zh-Hant: Chinese (Traditional)",           L"Assets/InGameLocalizationResources/localizedStrings_zh_Hant.json"},
        {"ja-JP",       "Loc_Button_5", "ja-JP: Japanese (Japan)",                  L"Assets/InGameLocalizationResources/localizedStrings_ja_JP.json"},
        {"es",          "Loc_Button_6", "es: Spanish",                              L"Assets/InGameLocalizationResources/localizedStrings_es.json"},
        {"fr",          "Loc_Button_7", "fr: French",                               L"Assets/InGameLocalizationResources/localizedStrings_fr.json"}
    };
    
    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // UI
    void Log(const char* text);
    void InitializeUI();    
    void SetupButtonHandler(ATG::UITK::UIButton& button, std::string displayText, std::function<void()> onClicked);

    // Localization
    void ChangeLocalization(const LanguageInfo& languageInfo);
    void LoadLocalizedStrings(const wchar_t* stringFile);
    void ChangeToDefaultLocalization();
    void EnumerateCurrentLocalizationSettings();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // UITK
    ATG::UITK::UIManager                            m_uiManager;
    ATG::UITK::UIInputState                         m_uiInputState;

    // Input device.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    std::unique_ptr<DirectX::Keyboard>              m_keyboard;
    std::unique_ptr<DirectX::Mouse>                 m_mouse;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;

    // UI
    std::mutex                                      m_logMutex;
    std::vector<std::string>                        m_logQueue;
    ATG::UITK::UIElementPtr                         m_buttonStackPanel;
    std::shared_ptr<ATG::UITK::UIConsoleWindow>     m_consoleWindow;    
    std::shared_ptr<ATG::UITK::UIStaticText>        m_localizedText;
    std::shared_ptr<ATG::UITK::UIImage>             m_localizedImage;        

    // Localization
    const LanguageInfo*                             m_currentLanguageInfo = &c_supportedLanguageInfos[0];
    std::unordered_map<std::string, std::string>    m_localizedStrings;
};
