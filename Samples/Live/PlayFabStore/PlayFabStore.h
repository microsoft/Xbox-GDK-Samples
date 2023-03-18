//--------------------------------------------------------------------------------------
// PlayFabStore.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "LiveResources.h"
#include "PlayFabResources.h"
#include "LiveInfoHUD.h"
#include "StepTimer.h"
#include "UITK.h"

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
    void InitializeUI();
    void InitializePlayFabProductsUI();
    void InitializeMicrosoftStoreProductsUI();
    void AddPlayFabProductButton(std::string itemId, std::string imageStyleId);
    void AddStoreProductButton(std::string storeId, std::string imageStyleId);

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

    // UI Methods
    std::shared_ptr<ATG::UITK::UIConsoleWindow> GetConsole() const
    {
        return (m_console) ?
            m_console->GetTypedSubElementById<ATG::UITK::UIConsoleWindow>(ATG::UITK::ID("ConsoleWindow")) :
            nullptr;
    }

    std::shared_ptr<ATG::UITK::UIPanel> GetPopup() const
    {
        return m_uiManager.FindTypedById<ATG::UITK::UIPanel>(ATG::UITK::ID("OkPopup"));
    }

    // Sample methods

    void PlayFabSignIn();

    void QueryStoreProducts();
    void PurchaseStoreProduct(const char* storeId);
    void RedeemStoreProducts();

    void QueryPlayFabProducts(const char** itemIds, const uint32_t itemIdsCount);
    void QueryPlayFabInventory(const char* continuationToken = nullptr);
    void PurchasePlayFabItem(const char* itemId, const int32_t amount, const int32_t price);
    void SubtractPlayFabItem(const char* itemId, const int32_t amount);
    void UpdatePlayFabItem(const char* itemId, const char* type, const int32_t amount);

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    XTaskQueueHandle m_asyncQueue;

    // UITK
    ATG::UITK::UIManager                        m_uiManager;
    ATG::UITK::UIInputState                     m_inputState;

    ATG::UITK::UIElementPtr                     m_mainLayout;
    ATG::UITK::UIElementPtr                     m_menuLayout;

    std::shared_ptr<ATG::UITK::UIStackPanel>    m_itemMenu;
    std::shared_ptr<ATG::UITK::UIStackPanel>    m_itemMenuPF;
    std::shared_ptr<ATG::UITK::UIStackPanel>    m_actionMenu;

    std::shared_ptr<ATG::UITK::UIPanel>         m_contentPanel;
    std::shared_ptr<ATG::UITK::UIPanel>         m_console;
    bool                                        m_showConsole;

    std::vector<std::string>                    m_logQueue;
    std::shared_ptr<ATG::UITK::UIConsoleWindow> m_consoleWindow;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_gold;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_potion;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_shield;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_arrow;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_sword;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_bow;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_gem;
    std::shared_ptr<ATG::UITK::UIImage>         m_selectedItemImage;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_selectedItemGold;    
    std::shared_ptr<ATG::UITK::UIStaticText>    m_items[6];
    std::shared_ptr<ATG::UITK::UIStaticText>    m_aButtonText;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_statusText;
    
    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    std::unique_ptr<DirectX::Mouse>             m_mouse;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;

    // Xbox Live objects.
    std::unique_ptr<ATG::LiveResources>         m_liveResources;
    std::unique_ptr<ATG::LiveInfoHUD>           m_liveInfoHUD;

    std::unique_ptr<ATG::PlayFabResources>      m_playFabResources;

    XStoreContextHandle                         m_xStoreContext;

    // Product UI
    bool                                        m_pfCatalogInitialized;
    bool                                        m_msProductsInitialized;

    std::map<std::string, std::pair<std::string, std::string>>  m_productDetails; // key is storeId, value is <title, price>
    std::map<std::string, std::pair<std::string, int32_t>>      m_pfCatalogDetails; // key is Id, value is <title, price>
    std::map<std::string, int32_t>                              m_pfInventoryCounts; // key is Id, value is amount

    enum Descriptors
    {
        Reserve,
        Count = 32,
    };
};
