//--------------------------------------------------------------------------------------
// GameSaveCombo.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "UITK.h"
#include "XGameSave.h"

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

    // Initialization
    void Initialize(HWND window, int width, int height);
    void RegisterUIEventHandlers();
    void LoadLayout();

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

    // UI
    void Log(const char* text);
    void DisableProviderReqButtons();
    void EnableProviderReqButtons();

    // User Load
    HRESULT GetUserHandle(XUserAddOptions xUserAddOptions);

    // GameSave
    HRESULT GetProviderHandle();
    HRESULT InitializeData();
    HRESULT DeleteContainer();
    void ParseBlobData(XGameSaveBlob* blobData, uint32_t blobCount);

    void SetUserRequiresUpdate() { m_userUIRequiresUpdate = true; }

    struct CharacterStatData {
        std::string name;
        uint8_t level;
        std::string combatClass;
    };

    struct WorldData {
        int32_t xPosition;
        int32_t yPosition;
        bool isDayTime;
    };

    struct GamerPicBytes
    {
        size_t size;
        std::unique_ptr<uint8_t> data;
    };

    struct ContainerInfo {
        XGameSaveContainerHandle containerHandle;
        const char* containerName;
    };

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    //UI management
    HRESULT UpdateUserUIData();
    void UpdateView();

    HRESULT AddModifyContainer();
    HRESULT GenerateBlobData(XGameSaveUpdateHandle updateHandle);
    HRESULT LoadBlobsFromDisk();
    void SetDataDisplayable(bool value);
    void LogFailedHR(const char* functionName, HRESULT hr);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // UITK
    ATG::UITK::UIManager                        m_uiManager;
    ATG::UITK::UIInputState                     m_inputState;
    ATG::UITK::UIElementPtr                     m_optionsPanel;

    // UI
    std::mutex                                  m_logMutex;
    std::vector<std::string>                    m_logQueue;
    std::shared_ptr<ATG::UITK::UIButton>        m_loadButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_addContainerButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_deleteContainerButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_switchUserButton;
    std::shared_ptr<ATG::UITK::UIConsoleWindow> m_consoleWindow;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_gamertagText;
    std::shared_ptr<ATG::UITK::UIImage>         m_gamerpicImage;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_containerLabel;

    std::shared_ptr<ATG::UITK::UIStaticText>    m_characterDataNameLabel;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_characterDataLevelLabel;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_characterDataCombatClassLabel;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_worldDataIsDayTimeLabel;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_worldDataXPositionLabel;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_worldDataYPositionLabel;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    std::unique_ptr<DirectX::Mouse>             m_mouse;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // UserSignIn.
    XUserHandle                                 m_userHandle;
    XGameSaveProviderHandle                     m_providerHandle;
    XTaskQueueHandle                            m_taskQueue;
    bool                                        m_userAddInProgress;

    // The SCID for this title, used to identify it in the Xbox Live backend and access its storage there.
    CharacterStatData                           m_currentCharacterData;
    WorldData                                   m_currentWorldData;
    XGameSaveContainerHandle                    m_containerHandle;

    std::atomic_bool                            m_isDataDisplayable;
    bool                                        m_userUIRequiresUpdate;
    GamerPicBytes                               m_gamerPic;
};
