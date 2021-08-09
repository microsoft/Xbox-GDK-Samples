//--------------------------------------------------------------------------------------
// LocalStorage.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "UITK.h"

enum class LocalStorageLocation
{
    // Xbox and Windows Platforms
    PersistentLocalStorage,

#if defined(_GAMING_XBOX_SCARLETT) || defined(_GAMING_XBOX)

    // Xbox Only
    TemporaryLocalStorage,
    SystemScratch,
    InstalledGameData

#else

    // Windows only
    TempFolder,
    LocalAppData

#endif
};

const static size_t s_numTestButtons = 8;

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
    void GetDefaultSize(int& width, int& height) const noexcept;

    // ATG::UITK::D3DResourcesProvider
    ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    ID3D12GraphicsCommandList* GetCommandList() const override { return m_deviceResources->GetCommandList(); }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // UI Setup
    void SetupUI();
    void SetStorageTypeLabel(LocalStorageLocation location);
    void SetupButtonHandler(std::shared_ptr<ATG::UITK::UIButton> button, std::function<void()> onClicked);
    void SetupTestButton(size_t index, std::string buttonLabel, std::function<void()> testMethod);
    void SetupTestButtons(LocalStorageLocation location);

public:

    // Storage methods
    void InitializePersistentLocalStorage();

    // File IO Helper Methods
    std::string GetLocalStoragePath(LocalStorageLocation location);
    std::string LocalStorageLocationToString(LocalStorageLocation location);
    bool WriteFile(const char* filePath, const std::vector<uint8_t>& data, size_t numDuplicates = 1, bool append = false, bool log = true);
    bool ReadFile(const char* filePath, std::vector<uint8_t>& outData);

    // Test Management
    void InvokeTest(size_t index);
    void NextStorageType();
    void PrevStorageType();
    void SetStorageType(LocalStorageLocation location);

    // Test Cases
    void Test_GetStorageInfo(LocalStorageLocation location);
    void Test_QueryFreeSpace(LocalStorageLocation location);
    void Test_WriteFile(LocalStorageLocation location);
    void Test_ReadFile(LocalStorageLocation location);
    void Test_GetWriteStats();
    void Test_StressWriteStats(LocalStorageLocation location);
    void Test_GetPLSInfo();
    void Test_FillAvailablePLS();

    // On-going Write Testing
    void Test_StartStressWrites(LocalStorageLocation location, size_t bytesToWrite, size_t bytesPerSecond);
    void Test_StopStressWrites();
    void Test_UpdateStressWrites(float deltaSeconds);

    // Other misc sample methods
    void Log(const char* log);

private:

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // UITK
    ATG::UITK::UIManager                        m_uiManager;
    ATG::UITK::UIInputState                     m_inputState;
    std::shared_ptr<ATG::UITK::UIButton>        m_testButtons[s_numTestButtons];
    std::shared_ptr<ATG::UITK::UIButton>        m_leftButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_rightButton;
    std::shared_ptr<ATG::UITK::UIConsoleWindow> m_consoleWindow;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_storageTypeText;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    std::unique_ptr<DirectX::Mouse>             m_mouse;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // Persistent Local Storage (PLS)
    std::string                                 m_plsPath;

    // Logging
    std::mutex                                  m_logMutex;
    std::vector<std::string>                    m_logQueue;

    // State
    LocalStorageLocation                        m_currentState;
    std::function<void()>                       m_testCallbacks[s_numTestButtons];
    XTaskQueueHandle                            m_taskQueue;

    // On-Going Write Testing
    bool                                        m_writeTestEnabled;
    std::string                                 m_writeTestPath;
    float                                       m_writeTestWriteDelay;
    float                                       m_writeTestLogDelay;
    size_t                                      m_writeTestBytesRemaining;
    size_t                                      m_writeTestBytesPerSecond;
};
