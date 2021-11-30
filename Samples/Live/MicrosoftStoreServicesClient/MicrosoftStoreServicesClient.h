//--------------------------------------------------------------------------------------
// MicrosoftStoreServicesClient.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "LiveResources.h"
#include "LiveInfoHUD.h"
#include "StepTimer.h"
#include "UITK.h"
#include <XStore.h>
#include "StepTimer.h"
#include "HttpManager.h"

constexpr size_t c_uiResultRowMax = 4;//c_uiResultRowMax (4)

struct RecurrenceItem {
    bool        autoRenew = false;
    std::string beneficiary;
    std::string expirationTime;
    std::string expirationTimeWithGrace;
    std::string id;
    bool        isTrial = false;
    std::string lastModified;
    std::string market;
    std::string productId;
    std::string recurrenceState;
    std::string skuId;
    std::string startTime;
    std::string cancellationDate;
};

struct CollectionsItem {
    std::string acquisitionType;
    std::string endDate;
    std::string id;
    std::string modifiedDate;
    std::string productId;
    std::string productKind;
    std::string skuId;
    std::string status;
    uint64_t    quantity = 0;
};

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
    void SetupButtonHandler(ATG::UITK::UIButton& button, std::function<void()> onClicked);

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

    void HttpManagerLog(const std::string& s) const
    {
        OutputDebugStringA(s.c_str());

        if (m_consoleWindow)
        {
            std::vector<std::string> tokens;
            std::string intermediate;
            std::stringstream check1(s);

            while (getline(check1, intermediate, '\n'))
            {
                tokens.emplace_back(intermediate);
            }

            for (const std::string& str : tokens)
            {
                m_consoleWindow->AppendLineOfText(str);
            }
        }
    }

    template<typename ... Args>
    void HttpManagerLog(const std::string& format, Args ... args) const
    {
        const size_t size = (size_t)std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
        const auto buffer = std::make_unique<char[]>(size);

        std::snprintf(buffer.get(), size, format.c_str(), args...);
        std::string formatted = std::string(buffer.get(), buffer.get() + size - 1);

        OutputDebugStringA(formatted.c_str());

        if (m_consoleWindow)
        {
            std::vector<std::string> tokens;
            std::string intermediate;
            std::stringstream check1(formatted);

            while (getline(check1, intermediate, '\n'))
            {
                tokens.emplace_back(intermediate);
            }

            for (const std::string& str : tokens)
            {
                m_consoleWindow->AppendLineOfText(str);
            }
        }
    }

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

    // UITK
    ATG::UITK::UIManager                        m_uiManager;
    ATG::UITK::UIInputState                     m_inputState;

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
    std::shared_ptr<ATG::LiveResources>         m_liveResources;
    std::unique_ptr<ATG::LiveInfoHUD>           m_liveInfoHUD;

    enum Descriptors
    {
        // Put your static descriptors here
        Reserve,
        Count = 64,
    };

    ATG::UITK::UIElementPtr                     m_menuScreen;
    ATG::UITK::UIElementPtr                     m_collectionScreen;
    ATG::UITK::UIElementPtr                     m_subscriptionScreen;
    ATG::UITK::UIElementPtr                     m_clawbackScreen;

    //UI Button
    std::shared_ptr<ATG::UITK::UIButton>        m_authScreenButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_showCollectionsButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_showSubscriptionsButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_showClawbackButton;

    std::shared_ptr<ATG::UITK::UIPanel>         m_subscriptionProductIdPanels[c_uiResultRowMax];
    std::shared_ptr<ATG::UITK::UIPanel>         m_subscriptionStartPanels[c_uiResultRowMax];
    std::shared_ptr<ATG::UITK::UIPanel>         m_subscriptionEndPanels[c_uiResultRowMax];
    std::shared_ptr<ATG::UITK::UIPanel>         m_subscriptionStatePanels[c_uiResultRowMax];
    std::shared_ptr<ATG::UITK::UIButton>        m_subscriptionPostponeOrCancelButton[c_uiResultRowMax];
    std::shared_ptr<ATG::UITK::UIButton>        m_subscriptionNextPageButton;
    std::shared_ptr<ATG::UITK::UIPanel>         m_subscriptionPageNumber;

    std::shared_ptr<ATG::UITK::UIPanel>         m_collectionsProductIdPanels[c_uiResultRowMax];
    std::shared_ptr<ATG::UITK::UIPanel>         m_collectionsTypePanel[c_uiResultRowMax];
    std::shared_ptr<ATG::UITK::UIPanel>         m_collectionsQuantityPanels[c_uiResultRowMax];
    std::shared_ptr<ATG::UITK::UIButton>        m_collectionsConsumeOrPurchaseButtons[c_uiResultRowMax];
    std::shared_ptr<ATG::UITK::UIButton>        m_collectionsNextPageButton;
    std::shared_ptr<ATG::UITK::UIPanel>         m_collectionsPageNumber;

    std::shared_ptr<ATG::UITK::UIButton>        m_clawbackViewClawbackQueueButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_clawbackViewUsersClawbackButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_clawbackViewUserBalancesButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_clawbackRunValidationButton;

    XTaskQueueHandle                            m_asyncQueue;

    std::shared_ptr<ATG::UITK::UIConsoleWindow> m_consoleWindow;

    //-------------------------------------------------------------------
    // Sample Specific variables and APIs for showcasing Store Services
    //-------------------------------------------------------------------
    XStoreContextHandle m_xStoreContext;
    HttpManager*        m_httpManager = nullptr;
    std::string         m_currentUIPage;

    //----------------------------------------------------
    //  Collections, Consume, and Purchase specific items
    void ShowCollections();
    void ShowCollectionNextPage(uint64_t StartPos);
    void ConsumeOrPurchaseButton(uint8_t);
    void PurchaseItem(std::string ProductId);
    void ConsumeItem(CollectionsItem Item);

    std::vector<CollectionsItem> m_collectionsItems;
    uint64_t                     m_CollectionsUIStart;
    uint64_t                     m_CollectionsUIEnd;

    //----------------------------------------------------
    //  Recurrence / Subscriptions specific items
    void ShowSubscriptions();
    void ShowSubscriptionNextPage(uint64_t StartPos);
    void PostponeOrCancelButton(uint8_t);

    std::vector<RecurrenceItem>  m_recurrenceItems;
    uint64_t                     m_subscriptionUIStart;
    uint64_t                     m_subscriptionUIEnd;

    //----------------------------------------------------
    //  Clawback / Refund specific functions
    void ShowClawback();
    void QueryClawbackForUser();
    void SimpleServiceGetCall(std::string URL);

    //----------------------------------------------------
    //  AAD Authentication related variables and APIs
    void InitStore();
    void SetUserAndSandboxIdentifiers();
    void RefreshUserStoreIds();
    void CacheAccessTokensFromService(std::string ServerAccessTokenURL);
    std::string GetUserCollectionsId(const char* serviceTicket, const char* publisherUserId);
    std::string GetUserPurchaseId(const char* serviceTicket, const char* publisherUserId);
    std::string GetCachedUserCollectionsId(bool ForceRefresh);
    std::string GetCachedUserPurchaseId(bool ForceRefresh);

    std::string m_cachedCollectionsAccessToken;
    std::string m_cachedPurchaseAccessToken;
    std::string m_cachedUserCollectionsId;
    std::string m_cachedUserPurchaseId;
    std::string m_cachedUserId;
    std::string m_gamerTag;
    std::string m_currentSandbox;
    bool        m_waitingForTokens;
    bool        m_cachingTokens;

    void CacheUserCollectionsId(const std::string& UserCollectionsId)
    {
        m_cachedUserCollectionsId = UserCollectionsId;
    };

    void CacheUserPurchaseId(const std::string& UserPurchaseId)
    {
        m_cachedUserPurchaseId = UserPurchaseId;
    };

    void CacheCollectionsAccessToken(const std::string& AccessToken)
    {
        m_cachedCollectionsAccessToken = AccessToken;
    };

    void CachePurchaseAccessToken(const std::string& AccessToken)
    {
        m_cachedPurchaseAccessToken = AccessToken;
    };

    void CacheUserId(const std::string& PublisherUserId)
    {
        m_cachedUserId = PublisherUserId;
    };
};
