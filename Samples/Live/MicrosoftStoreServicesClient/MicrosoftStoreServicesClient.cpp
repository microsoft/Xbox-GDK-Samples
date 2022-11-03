//--------------------------------------------------------------------------------------
// MicrosoftStoreServicesClient.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MicrosoftStoreServicesClient.h"
#include "ATGColors.h"
#include "FindMedia.h"
#include "StringUtil.h"
#include "Json.h"
#include "HttpManager.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

#pragma message( __FILE__  ": TODO update SAMPLE_SERVICE_HOST to be your deployed Service Sample" )
// The host address where you have deployed your version of the Microsoft.StoreServices Sample
// https://github.com/microsoft/Microsoft-Store-Services-Sample
#define SAMPLE_SERVICE_HOST "atgstoreservicedev.azurewebsites.net"

/// <summary>
/// Built-in URL's and functionality of the Microsoft.StoreServices Sample service.
/// Not all of these are used within this client sample.
/// </summary>
namespace
{
    constexpr char c_accessTokenURL[] = "https://" SAMPLE_SERVICE_HOST "/collections/RetrieveAccessTokens";
    constexpr char c_collectionsQueryURL[] = "https://" SAMPLE_SERVICE_HOST "/collections/Query";
    constexpr char c_consumeURL[] = "https://" SAMPLE_SERVICE_HOST "/collections/Consume";
    //constexpr char c_addPendingConsumeURL[] = "https://" SAMPLE_SERVICE_HOST "/collections/AddPendingConsume";
    //constexpr char c_retryPendingConsumeURL[] = "https://" SAMPLE_SERVICE_HOST "/collections/RetryPendingConsume";
    //constexpr char c_viewPendingConsumeURL[] = "https://" SAMPLE_SERVICE_HOST "/collections/ViewPendingConsume";
    constexpr char c_viewUserBalancesURL[] = "https://" SAMPLE_SERVICE_HOST "/collections/ViewUserBalances";
    constexpr char c_recurrenceQueryURL[] = "https://" SAMPLE_SERVICE_HOST "/purchase/RecurrenceQuery";
    constexpr char c_recurrenceChangeURL[] = "https://" SAMPLE_SERVICE_HOST "/purchase/RecurrenceChange";
    constexpr char c_clawbackQueryURL[] = "https://" SAMPLE_SERVICE_HOST "/purchase/ClawbackQuery";
    constexpr char c_viewClawbackQueueURL[] = "https://" SAMPLE_SERVICE_HOST "/purchase/ViewClawbackQueue";
    constexpr char c_runClawbackValidationURL[] = "https://" SAMPLE_SERVICE_HOST "/purchase/RunClawbackValidation";
    //constexpr char c_viewClawbackActionItemsURL[] = "https://" SAMPLE_SERVICE_HOST "/purchase/ViewClawbackActionItems";
    //constexpr char c_viewBuildingClawbackActionItemsURL[] = "https://" SAMPLE_SERVICE_HOST "/purchase/ViewBuildingClawbackActionItems";
    //constexpr char c_viewCompletedClawbackActionitemsURL[] = "https://" SAMPLE_SERVICE_HOST "/purchase/ViewCompletedClawbackActionitems";
    //constexpr char c_viewPendingClawbackActionitemsURL[] = "https://" SAMPLE_SERVICE_HOST "/purchase/ViewPendingClawbackActionitems";

    void from_json(const json& j, RecurrenceItem& item) {
        j.at("autoRenew").get_to(item.autoRenew);
        j.at("beneficiary").get_to(item.beneficiary);
        j.at("expirationTime").get_to(item.expirationTime);
        j.at("expirationTimeWithGrace").get_to(item.expirationTimeWithGrace);
        j.at("id").get_to(item.id);
        j.at("isTrial").get_to(item.isTrial);
        j.at("lastModified").get_to(item.lastModified);
        j.at("market").get_to(item.market);
        j.at("productId").get_to(item.productId);
        j.at("recurrenceState").get_to(item.recurrenceState);
        j.at("skuId").get_to(item.skuId);
        j.at("startTime").get_to(item.startTime);
        j.at("cancellationDate").get_to(item.cancellationDate);
    }

    void from_json(const json& j, CollectionsItem& item) {
        j.at("acquisitionType").get_to(item.acquisitionType);
        j.at("endDate").get_to(item.endDate);
        j.at("id").get_to(item.id);
        j.at("modifiedDate").get_to(item.modifiedDate);
        j.at("productId").get_to(item.productId);
        j.at("productKind").get_to(item.productKind);
        j.at("skuId").get_to(item.skuId);
        j.at("status").get_to(item.status);
        j.at("quantity").get_to(item.quantity);
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_asyncQueue(nullptr),
    m_xStoreContext(nullptr)
{
    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_asyncQueue)
    );

    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("Microsoft.Store.Services Client Sample");

    //  Create a Lambda that we can pass so that the Sample's log function can be used
    //  to display any information from the HttpManager
    m_httpManager = new HttpManager([&](const std::string& str) { HttpManagerLog(str); });

    //  must be initialized after the httpManager
    m_liveResources = std::make_shared<ATG::LiveResources>();

    m_cachingTokens = false;
    m_waitingForTokens = false;

    m_cachedCollectionsAccessToken.clear();
    m_cachedPurchaseAccessToken.clear();
    m_cachedUserCollectionsId.clear();
    m_cachedUserPurchaseId.clear();
    m_cachedUserId.clear();
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    XStoreCloseContextHandle(m_xStoreContext);

    if (m_asyncQueue)
    {
        XTaskQueueCloseHandle(m_asyncQueue);
        m_asyncQueue = nullptr;
    }
}

// Initialize the Direct3D resources required to run.
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

    m_liveResources->SetUserChangedCallback([this](XUserHandle user)
    {
        m_liveInfoHUD->SetUser(user, m_liveResources->GetAsyncQueue());

#ifdef _GAMING_XBOX
        InitStore();
#endif

        SetUserAndSandboxIdentifiers();
    });

    m_liveResources->SetUserSignOutCompletedCallback([this](XUserHandle /*user*/)
    {
        m_liveInfoHUD->SetUser(nullptr, m_liveResources->GetAsyncQueue());
    });

    m_liveResources->SetErrorHandler([this](HRESULT error)
    {
        if (error == E_GAMEUSER_NO_DEFAULT_USER || error == E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED)
        {
            m_liveResources->SignInWithUI();
        }
        else // Handle other error cases
        {

        }
    });

    // Before we can make an Xbox Live call we need to ensure that the Game OS has initialized the network stack
    // For sample purposes we block user interaction with the sample.  A game should wait for the network to be
    // initialized before the main menu appears.  For samples, we will wait at the end of initialization.
    while (!m_liveResources->IsNetworkAvailable())
    {
        SwitchToThread();
    }

    InitializeUI();

    m_liveResources->Initialize();
    m_liveInfoHUD->Initialize();

#ifndef _GAMING_XBOX
    // For desktop this can be done here, on console this is done after user sign-in
    InitStore();
    SetUserAndSandboxIdentifiers();
#endif
}

void Sample::SetUserAndSandboxIdentifiers()
{
#pragma message( __FILE__  ": TODO in SetUserAndSandboxIdentifiers - Update with your own user Identifiers" )
    //  NOTE: For this sample we are using the Gamertag as the unique identifier in the
    //        databases of the Service Sample, but this is not allowed for releasing
    //        titles as the Gamertag can change at any time.    You would either use 
    //        the PXUID, XUID (with authorization), or your own unique identifier for
    //        the player if not an XBL enabled game.
    m_gamerTag = m_liveResources->GetGamertag();
    if (m_gamerTag.empty())
    {
        //  TODO: add your own userID if not using Xbox Live.  For the sample we will just use
        //        a default string
        m_gamerTag = "Non-XBL-Account";
    }

    //  When running from a Sandbox we need to provide the SandboxId to the service sample
    //  to be added to the calls to the Microsoft Store Services to scope the results to
    //  what the account owns within that specific sandbox.
    char sandboxId[XSystemXboxLiveSandboxIdMaxBytes] = {};
    XSystemGetXboxLiveSandboxId(XSystemXboxLiveSandboxIdMaxBytes, sandboxId, nullptr);
    m_currentSandbox = sandboxId;
}

/// <summary>
/// Initialize the custom UI elements for the MicrosoftStoreServicesClient sample
/// </summary>
void Sample::InitializeUI()
{
    //  Main Menu elements
    m_menuScreen = m_uiManager.LoadLayoutFromFile("Assets/UI/menu-screen.json");
    m_uiManager.AttachTo(m_menuScreen, m_uiManager.GetRootElement());

    m_consoleWindow = m_uiManager.FindTypedById<UIPanel>(ID("menu-screen"))
        ->GetTypedSubElementById<UIPanel>(ID("Output_Console_Window_Outer_Panel"))
        ->GetTypedSubElementById<UIConsoleWindow>(ID("Output_Console_Window"));

    auto buttonPanel = m_uiManager.FindTypedById<UIPanel>(ID("menu-screen"))
        ->GetTypedSubElementById<UIPanel>(ID("menu_section"));

    m_authScreenButton        = buttonPanel->GetTypedSubElementById<UIButton>(ID("authentication_button"));
    m_showCollectionsButton   = buttonPanel->GetTypedSubElementById<UIButton>(ID("collection_button"));
    m_showSubscriptionsButton = buttonPanel->GetTypedSubElementById<UIButton>(ID("subscription_button"));
    m_showClawbackButton      = buttonPanel->GetTypedSubElementById<UIButton>(ID("refund_button"));

    SetupButtonHandler(*m_authScreenButton, [&]() { RefreshUserStoreIds(); });
    SetupButtonHandler(*m_showCollectionsButton, [&]() { ShowCollections(); });
    SetupButtonHandler(*m_showSubscriptionsButton, [&]() { ShowSubscriptions(); });
    SetupButtonHandler(*m_showClawbackButton, [&]() { ShowClawback(); });

    //  Collections screen elements
    m_collectionScreen = m_uiManager.LoadLayoutFromFile("Assets/UI/show_collection.json");
    m_uiManager.AttachTo(m_collectionScreen, m_uiManager.GetRootElement());
    m_collectionScreen->SetVisible(false);

    auto collectionsPanel = m_uiManager.FindTypedById<UIPanel>(ID("collection-screen"));

    m_collectionsProductIdPanels[0] = collectionsPanel->GetTypedSubElementById<UIPanel>(ID("item_name_panel1"));
    m_collectionsProductIdPanels[1] = collectionsPanel->GetTypedSubElementById<UIPanel>(ID("item_name_panel2"));
    m_collectionsProductIdPanels[2] = collectionsPanel->GetTypedSubElementById<UIPanel>(ID("item_name_panel3"));
    m_collectionsProductIdPanels[3] = collectionsPanel->GetTypedSubElementById<UIPanel>(ID("item_name_panel4"));

    m_collectionsQuantityPanels[0] = collectionsPanel->GetTypedSubElementById<UIPanel>(ID("item_num_panel1"));
    m_collectionsQuantityPanels[1] = collectionsPanel->GetTypedSubElementById<UIPanel>(ID("item_num_panel2"));
    m_collectionsQuantityPanels[2] = collectionsPanel->GetTypedSubElementById<UIPanel>(ID("item_num_panel3"));
    m_collectionsQuantityPanels[3] = collectionsPanel->GetTypedSubElementById<UIPanel>(ID("item_num_panel4"));

    auto collectionbuttonPanel = m_uiManager.FindTypedById<UIPanel>(ID("collection-screen"))
        ->GetTypedSubElementById<UIPanel>(ID("button_section"));

    m_collectionsConsumeOrPurchaseButtons[0] = collectionbuttonPanel->GetTypedSubElementById<UIButton>(ID("consume_button1"));
    m_collectionsConsumeOrPurchaseButtons[1] = collectionbuttonPanel->GetTypedSubElementById<UIButton>(ID("consume_button2"));
    m_collectionsConsumeOrPurchaseButtons[2] = collectionbuttonPanel->GetTypedSubElementById<UIButton>(ID("consume_button3"));
    m_collectionsConsumeOrPurchaseButtons[3] = collectionbuttonPanel->GetTypedSubElementById<UIButton>(ID("consume_button4"));

    SetupButtonHandler(*m_collectionsConsumeOrPurchaseButtons[0], [&]() { ConsumeOrPurchaseButton(0); });
    SetupButtonHandler(*m_collectionsConsumeOrPurchaseButtons[1], [&]() { ConsumeOrPurchaseButton(1); });
    SetupButtonHandler(*m_collectionsConsumeOrPurchaseButtons[2], [&]() { ConsumeOrPurchaseButton(2); });
    SetupButtonHandler(*m_collectionsConsumeOrPurchaseButtons[3], [&]() { ConsumeOrPurchaseButton(3); });

    m_collectionsTypePanel[0]   = collectionbuttonPanel->GetTypedSubElementById<UIPanel>(ID("type_panel1"));
    m_collectionsTypePanel[1]   = collectionbuttonPanel->GetTypedSubElementById<UIPanel>(ID("type_panel2"));
    m_collectionsTypePanel[2]   = collectionbuttonPanel->GetTypedSubElementById<UIPanel>(ID("type_panel3"));
    m_collectionsTypePanel[3]   = collectionbuttonPanel->GetTypedSubElementById<UIPanel>(ID("type_panel4"));
    m_collectionsPageNumber     = collectionbuttonPanel->GetTypedSubElementById<UIPanel>(ID("pageNumber_panel"));
    m_collectionsNextPageButton = collectionbuttonPanel->GetTypedSubElementById<UIButton>(ID("nextpage_button"));

    SetupButtonHandler(*m_collectionsNextPageButton, [&]() { ShowCollectionNextPage(m_CollectionsUIStart + c_uiResultRowMax); });

    //  Recurrence screen elements
    m_subscriptionScreen = m_uiManager.LoadLayoutFromFile("Assets/UI/show_subscription.json");
    m_uiManager.AttachTo(m_subscriptionScreen, m_uiManager.GetRootElement());
    m_subscriptionScreen->SetVisible(false);
   
    auto subscriptionPanel = m_uiManager.FindTypedById<UIPanel>(ID("subscription-screen"));

    m_subscriptionProductIdPanels[0] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("subsucription_panel1"));
    m_subscriptionProductIdPanels[1] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("subsucription_panel2"));
    m_subscriptionProductIdPanels[2] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("subsucription_panel3"));
    m_subscriptionProductIdPanels[3] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("subsucription_panel4"));

    m_subscriptionStartPanels[0] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("start_panel1"));
    m_subscriptionStartPanels[1] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("start_panel2"));
    m_subscriptionStartPanels[2] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("start_panel3"));
    m_subscriptionStartPanels[3] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("start_panel4"));

    m_subscriptionEndPanels[0] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("end_panel1"));
    m_subscriptionEndPanels[1] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("end_panel2"));
    m_subscriptionEndPanels[2] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("end_panel3"));
    m_subscriptionEndPanels[3] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("end_panel4"));

    m_subscriptionStatePanels[0] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("state_panel1"));
    m_subscriptionStatePanels[1] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("state_panel2"));
    m_subscriptionStatePanels[2] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("state_panel3"));
    m_subscriptionStatePanels[3] = subscriptionPanel->GetTypedSubElementById<UIPanel>(ID("state_panel4"));

    auto subscriptionbuttonPanel = m_uiManager.FindTypedById<UIPanel>(ID("subscription-screen"))
        ->GetTypedSubElementById<UIPanel>(ID("button_section"));

    m_subscriptionPostponeOrCancelButton[0] = subscriptionbuttonPanel->GetTypedSubElementById<UIButton>(ID("subscription_button1"));
    m_subscriptionPostponeOrCancelButton[1] = subscriptionbuttonPanel->GetTypedSubElementById<UIButton>(ID("subscription_button2"));
    m_subscriptionPostponeOrCancelButton[2] = subscriptionbuttonPanel->GetTypedSubElementById<UIButton>(ID("subscription_button3"));
    m_subscriptionPostponeOrCancelButton[3] = subscriptionbuttonPanel->GetTypedSubElementById<UIButton>(ID("subscription_button4"));

    SetupButtonHandler(*m_subscriptionPostponeOrCancelButton[0], [&]() { PostponeOrCancelButton(0); });
    SetupButtonHandler(*m_subscriptionPostponeOrCancelButton[1], [&]() { PostponeOrCancelButton(1); });
    SetupButtonHandler(*m_subscriptionPostponeOrCancelButton[2], [&]() { PostponeOrCancelButton(2); });
    SetupButtonHandler(*m_subscriptionPostponeOrCancelButton[3], [&]() { PostponeOrCancelButton(3); });

    m_subscriptionPageNumber     = subscriptionbuttonPanel->GetTypedSubElementById<UIPanel>(ID("pageNumber_panel"));
    m_subscriptionNextPageButton = subscriptionbuttonPanel->GetTypedSubElementById<UIButton>(ID("nextpage_button"));

    SetupButtonHandler(*m_subscriptionNextPageButton, [&]() { ShowSubscriptionNextPage(m_subscriptionUIStart + c_uiResultRowMax); });

    //  Clawback screen elements
    m_clawbackScreen = m_uiManager.LoadLayoutFromFile("Assets/UI/show_clawback.json");
    m_uiManager.AttachTo(m_clawbackScreen, m_uiManager.GetRootElement());
    m_clawbackScreen->SetVisible(false);

    auto clawbackButtons = m_uiManager.FindTypedById<UIPanel>(ID("clawback-screen"))
        ->GetTypedSubElementById<UIPanel>(ID("button_section"));

    m_clawbackViewUsersClawbackButton = clawbackButtons->GetTypedSubElementById<UIButton>(ID("view_user_clawback"));
    m_clawbackViewUserBalancesButton  = clawbackButtons->GetTypedSubElementById<UIButton>(ID("view_user_balances"));
    m_clawbackViewClawbackQueueButton = clawbackButtons->GetTypedSubElementById<UIButton>(ID("view_clawback_queue_button"));
    m_clawbackRunValidationButton     = clawbackButtons->GetTypedSubElementById<UIButton>(ID("run_clawback_validation"));

    SetupButtonHandler(*m_clawbackViewUsersClawbackButton, [&]() { QueryClawbackForUser(); });
    SetupButtonHandler(*m_clawbackViewUserBalancesButton, [&]() { SimpleServiceGetCall(c_viewUserBalancesURL); });
    SetupButtonHandler(*m_clawbackViewClawbackQueueButton, [&]() { SimpleServiceGetCall(c_viewClawbackQueueURL); });
    SetupButtonHandler(*m_clawbackRunValidationButton, [&]() { SimpleServiceGetCall(c_runClawbackValidationURL); });

    //  Set the initial state of the UI elements
    m_subscriptionUIStart = 0;
    m_subscriptionUIEnd = 0;

    m_showCollectionsButton->SetEnabled(false);
    m_showSubscriptionsButton->SetEnabled(false);
    m_showClawbackButton->SetEnabled(false);

    for (size_t i = 0; i < c_uiResultRowMax; i++)
    {
        m_subscriptionProductIdPanels[i]->SetVisible(false);
        m_subscriptionStartPanels[i]->SetVisible(false);
        m_subscriptionEndPanels[i]->SetVisible(false);
        m_subscriptionStatePanels[i]->SetVisible(false);
        m_subscriptionPostponeOrCancelButton[i]->SetVisible(false);

        m_collectionsProductIdPanels[i]->SetVisible(false);
        m_collectionsTypePanel[i]->SetVisible(false);
        m_collectionsConsumeOrPurchaseButtons[i]->SetVisible(false);
        m_collectionsConsumeOrPurchaseButtons[i]->SetEnabled(false);
        m_collectionsQuantityPanels[i]->SetVisible(false);
    }
}

void Sample::InitStore()
{
#ifdef _GAMING_XBOX
    HRESULT hr = XStoreCreateContext(m_liveResources->GetUser(), &m_xStoreContext);
#else
    HRESULT hr = XStoreCreateContext(nullptr, &m_xStoreContext);
#endif

    DX::ThrowIfFailed(hr); // Unable to create an XStoreContext means we can't get the UserStoreIds for this sample.
    //  Most common issue here would be that the account has not acquired a license to the sample in the sandbox or
    //  the app identity is not properly configured in the MicrosoftGame.Config file.  See
    //  https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstore-troubleshooting
}

// Configure style of UI button and add event listener to UI button
void Sample::SetupButtonHandler(ATG::UITK::UIButton& button, std::function<void()> onClicked)
{
    button.ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [&, onClicked](UIButton* /*button*/)
    {
        onClicked();
    });
    button.ButtonState().AddListenerWhen(UIButton::State::Focused,
        [&](UIButton* /*button*/)
    {
        button.GetSubElementById(ID("button_border"))->SetVisible(true);
        button.GetSubElementById(ID("button_label"))->SetStyleId(ID("button_focused_text_style"));
    });
    button.ButtonState().AddListenerWhen(UIButton::State::Hovered,
        [&](UIButton* /*button*/)
    {
        button.GetSubElementById(ID("button_border"))->SetVisible(true);
        button.GetSubElementById(ID("button_label"))->SetStyleId(ID("button_focused_text_style"));
    });
    button.ButtonState().AddListenerWhen(UIButton::State::Normal,
        [&](UIButton* /*button*/)
    {
        button.GetSubElementById(ID("button_border"))->SetVisible(false);
        button.GetSubElementById(ID("button_label"))->SetStyleId(ID("button_default_text_style"));
    });
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.menu == GamePad::ButtonStateTracker::PRESSED)
        {
            if (!m_liveResources->IsUserSignedIn())
            {
                m_liveResources->SignInSilently();
            }
            else
            {
                m_liveResources->SignInWithUI();
            }
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
        ExitSample();
    }

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Tab))
    {
        if (!m_liveResources->IsUserSignedIn())
        {
            m_liveResources->SignInSilently();
        }
        else
        {
            m_liveResources->SignInWithUI();
        }
    }

    m_liveInfoHUD->Update(m_deviceResources->GetCommandQueue());

    m_inputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_inputState);

    if (m_httpManager)
    {
        m_httpManager->Update();
    }

    if (m_waitingForTokens)
    {
        RefreshUserStoreIds();
    }

    while (XTaskQueueDispatch(m_asyncQueue, XTaskQueuePort::Completion, 0))
    {
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

    //  Render the UI manager before the LiveInfoHUD
    m_uiManager.Render();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_liveInfoHUD->Render(commandList);
   
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
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
    m_liveResources->Refresh();
    m_inputState.Reset();
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
void Sample::GetDefaultSize(int& width, int& height) const noexcept
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

#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        Descriptors::Count,
        Descriptors::Reserve
        );

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);
    
    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Initialize windows-size dependent objects here.
    m_liveInfoHUD->SetViewport(m_deviceResources->GetScreenViewport());
    auto size = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(size.right, size.bottom);
}

void Sample::OnDeviceLost()
{
    // Add Direct3D resource cleanup here.
    m_graphicsMemory.reset();
    m_liveInfoHUD->ReleaseDevice();
    m_resourceDescriptors.reset();
    m_uiManager.GetStyleManager().ResetStyleRenderer();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion


/// Microsoft Store Services Specific Code and Functions for the sample

//--------------------------------------------------------
//  Collections, Consume, and Purchase specific functions
//--------------------------------------------------------

/// <summary>
/// Calls the Sample Service to query for the user's Collections data and displays the data in the UI of the client sample
/// </summary>
void Sample::ShowCollections()
{
    OutputDebugStringA("Show collections button is pressed\n");

    //  Show the Collections UI and hide the others
    m_subscriptionScreen->SetVisible(false);
    m_clawbackScreen->SetVisible(false);
    m_collectionScreen->SetVisible(true);
    m_currentUIPage = "Collections";

    ////  Build the JSON body
    json jRequest;
    jRequest["UserCollectionsId"] = GetCachedUserCollectionsId(false);
    jRequest["UserId"] = m_gamerTag;
    jRequest["sbx"] = m_currentSandbox; //  Required when running / testing in a sandbox

    std::string requestBody = jRequest.dump();
    size_t bodySize = requestBody.size();

    std::vector<HttpHeader> headers;
    headers.push_back(HttpHeader("Content-Type", "application/json"));
    headers.push_back(HttpHeader("User-Agent", "Microsoft.StoreServicesClientSample"));

    HRESULT hr = m_httpManager->MakeHttpRequestAsync(nullptr, "POST", c_collectionsQueryURL, headers, requestBody, bodySize, [this](HttpRequestContext* context)
    {
        if (context)
        {
            auto rawBody = context->responseBody.data();
            auto bodyLen = context->responseBody.size();
            std::string body(rawBody, rawBody + bodyLen);

            HttpManagerLog("Response status code: " + std::to_string(context->responseStatusCode) + "\n");
            HttpManagerLog("Response body size: " + std::to_string(bodyLen) + "\n");

            //  Because we are using the User-Agent header that identifies us as the client sample, we will
            //  get back the raw json from the b2b service response so that we can update our own UI and
            //  make future calls to the sample server.
            auto jsonStartPos = body.find_first_of('{', 0);
            if (jsonStartPos > 0 && jsonStartPos < bodyLen)
            {
                auto jsonString = body.substr(jsonStartPos);
                json j = json::parse(jsonString);

                std::vector<CollectionsItem> nonConsumables; //  used so we can have the consumables
                                                             //  shown first
                m_collectionsItems.clear();

                //  Go through and transfer our results to the vector we use to
                //  show the right items in the UI based on which page we are on

                for (const auto& it: j["Items"])
                {
                    CollectionsItem item;
                    from_json(it, item);

                    HttpManagerLog(item.productId + "\n");
                    if (item.productKind == "Consumable" || item.productKind == "UnmanagedConsumable")
                    {
                        m_collectionsItems.push_back(item);
                    }
                    else
                    {
                        nonConsumables.push_back(item);
                    }
                }

                for (size_t i = 0; i < nonConsumables.size(); i++)
                {
                    m_collectionsItems.push_back(nonConsumables[i]);
                }

                //  Passing 0 to this function will start us on the first page of the results in the UI
                ShowCollectionNextPage(0);
            }

            HttpManagerLog(body + "\n");
            HttpManagerLog("Request completed.\n");
        }
    });

    if (hr != E_PENDING && FAILED(hr))
    {
        HttpManagerLog("Sample::ShowCollections: MakeHttpRequestAsync failed with HRESULT = 0x%08x\n", static_cast<unsigned int>(hr));
    }
}

/// <summary>
/// Switches to the next page of results based on the position passed in
/// </summary>
/// <param name="StartPos"></param>
void Sample::ShowCollectionNextPage(uint64_t StartPos)
{
    OutputDebugStringA("Collection Next button is pressed\n");
    m_collectionScreen->SetVisible(true);

    //  If our starting position is larger than our array of items, loop back to the first page
    //  at position 0.
    auto totalItems = m_collectionsItems.size();
    if (StartPos > totalItems)
    {
        m_CollectionsUIStart = 0;
    }
    else
    {
        m_CollectionsUIStart = StartPos;
    }

    //  If we are on the last page, make sure the end doesn't go past what we have
    //  in the results.
    m_CollectionsUIEnd = m_CollectionsUIStart + c_uiResultRowMax;
    if (m_CollectionsUIEnd > totalItems)
    {
        m_CollectionsUIEnd = totalItems;
    }

    //  Calculate the current page # and total pages for the UI
    size_t totalPages = totalItems / c_uiResultRowMax;
    if (totalItems % c_uiResultRowMax > 0)
    {
        totalPages++;
    }

    size_t currentPage;
    if ( m_CollectionsUIEnd < c_uiResultRowMax
         && totalItems > 0)
    {
        currentPage = 1;
    }
    else
    {
        currentPage = m_CollectionsUIEnd / c_uiResultRowMax;
    }

    auto text = m_collectionsPageNumber->GetTypedSubElementById<UIStaticText>(ID("panel_text"));
    std::string pageNumber = "Page " + std::to_string(currentPage) + "/" + std::to_string(totalPages);
    text->SetDisplayText(pageNumber);

    m_collectionsPageNumber->SetVisible(true);

    //  Check if we need to show and enable the next page button
    if (totalPages > 1)
    {
        m_collectionsNextPageButton->SetVisible(true);
    }
    else
    {
        m_collectionsNextPageButton->SetVisible(false);
    }

    //  reset the sate of all the UI elements
    for (size_t i = 0; i < c_uiResultRowMax; i++)
    {
        m_collectionsProductIdPanels[i]->SetVisible(false);
        m_collectionsTypePanel[i]->SetVisible(false);
        m_collectionsConsumeOrPurchaseButtons[i]->SetVisible(false);
        m_collectionsConsumeOrPurchaseButtons[i]->SetEnabled(false);
        m_collectionsQuantityPanels[i]->SetVisible(false);
    }

    //  Go through the elements in the results that will fit on this page of the UI and populate the UI elements with their
    //  information.
    for (uint64_t itemPosition = m_CollectionsUIStart; itemPosition < m_CollectionsUIEnd; itemPosition++)
    {
        auto item = m_collectionsItems[itemPosition];
        auto uiRow = itemPosition - m_CollectionsUIStart;
        m_collectionsProductIdPanels[uiRow]->SetVisible(true);
        text = m_collectionsProductIdPanels[uiRow]->GetTypedSubElementById<UIStaticText>(ID("panel_text"));
        text->SetDisplayText(item.productId);

        //  If this is a type of consumable we want to show the Consume / Purchase button for this item
        if (item.productKind == "Consumable" || item.productKind == "UnmanagedConsumable")
        {
            m_collectionsQuantityPanels[uiRow]->SetVisible(true);
            text = m_collectionsQuantityPanels[uiRow]->GetTypedSubElementById<UIStaticText>(ID("item_quantity_text"));
            text->SetDisplayText(std::to_string(item.quantity));

            text = m_collectionsConsumeOrPurchaseButtons[uiRow]->GetTypedSubElementById<UIStaticText>(ID("button_label"));

            //  If the item has a non-zero quantity, consume it first.  If zero, offer the bring up the purchase UI to buy more
            if (item.quantity > 0)
            {
                text->SetDisplayText("Consume");
            }
            else
            {
                text->SetDisplayText("Buy More");
            }

            m_collectionsConsumeOrPurchaseButtons[uiRow]->SetVisible(true);
            m_collectionsConsumeOrPurchaseButtons[uiRow]->SetEnabled(true);
        }

        m_collectionsTypePanel[uiRow]->SetVisible(true);
        text = m_collectionsTypePanel[uiRow]->GetTypedSubElementById<UIStaticText>(ID("item_type_text"));
        text->SetDisplayText(item.productKind);
    }
}

/// <summary>
/// Determines if the action from the button number should be a consume or purchase based on if the there is a non-zero quantity
/// </summary>
/// <param name="button_num"></param>
void Sample::ConsumeOrPurchaseButton(uint8_t button_num)
{
    uint64_t itemUIindex = m_CollectionsUIStart + button_num;
    if (m_collectionsItems[itemUIindex].quantity == 0)
    {
        PurchaseItem(m_collectionsItems[itemUIindex].productId);
    }
    else
    {
        ConsumeItem(m_collectionsItems[itemUIindex]);
    }
}

/// <summary>
/// Takes the ProductId string and initiates the in-game purchase flow of that item for convenience in the sample
/// </summary>
/// <param name="ProductId"></param>
void Sample::PurchaseItem(std::string ProductId)
{
    OutputDebugStringA("Purchase button is pressed\n");

    struct PurchaseContext
    {
        Sample* pThis;
        std::string storeId;
    };

    auto async = new XAsyncBlock{};
    async->context = new PurchaseContext{ this, ProductId };
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock* async)
    {
        auto& [pThis, storeId] = *reinterpret_cast<PurchaseContext*>(async->context);

        HRESULT hr = XStoreShowPurchaseUIResult(async);
        if (SUCCEEDED(hr))
        {
            printf("Purchase succeeded (%s)\n", storeId.c_str());

            // Update the current UI screen's data
            if (pThis->m_currentUIPage == "Collections")
            {
                pThis->ShowCollections();
            }
            else if (pThis->m_currentUIPage == "Subscriptions")
            {
                pThis->ShowSubscriptions();
            }
        }
        else
        {
            printf("Purchase failed (%s) 0x%x\n", storeId.c_str(), static_cast<unsigned int>(hr));

            if (hr == E_GAMESTORE_ALREADY_PURCHASED)
            {
                printf("Already own this\n");
            }
        }

        delete async;
    };

    HRESULT hr = XStoreShowPurchaseUIAsync(
        m_xStoreContext,
        ProductId.c_str(),
        nullptr,    // Can be used to override the title bar text
        nullptr,    // Can be used to provide extra details to purchase
        async);

    if (FAILED(hr))
    {
        delete async;
        printf("Error calling XStoreShowPurchaseUIAsync : 0x%x\n", static_cast<unsigned int>(hr));
        return;
    }
}

/// <summary>
/// Calls the Service Sample to consume the passed in item from the user's collection
/// </summary>
/// <param name="Item"></param>
void Sample::ConsumeItem(CollectionsItem Item)
{
    OutputDebugStringA("Consume button is pressed\n");

    ////  Build the JSON body
    json jRequest;
    jRequest["UserPurchaseId"] = GetCachedUserPurchaseId(false);
    jRequest["UserCollectionsId"] = GetCachedUserCollectionsId(false);
    jRequest["UserId"] = m_gamerTag;
    jRequest["Quantity"] = Item.quantity;
    jRequest["ProductId"] = Item.productId;
    jRequest["sbx"] = m_currentSandbox;
    jRequest["IncludeOrderIds"] = true;

    if (Item.productKind == "UnmanagedConsumable")
    {
        jRequest["IsUnmanagedConsumable"] = true;
    }

    std::string requestBody = jRequest.dump();
    size_t bodySize = requestBody.size();

    std::vector<HttpHeader> headers;
    headers.push_back(HttpHeader("Content-Type", "application/json"));
    headers.push_back(HttpHeader("User-Agent", "Microsoft.StoreServicesClientSample"));

    HRESULT hr = m_httpManager->MakeHttpRequestAsync(nullptr, "POST", c_consumeURL, headers, requestBody, bodySize, [this](HttpRequestContext* context)
    {
        if (context)
        {
            auto rawBody = context->responseBody.data();
            auto bodyLen = context->responseBody.size();
            std::string body(rawBody, rawBody + bodyLen);

            HttpManagerLog("Response status code: " + std::to_string(context->responseStatusCode) + "\n");
            HttpManagerLog("Response body size: " + std::to_string(bodyLen) + "\n");
            HttpManagerLog(body + "\n");
            HttpManagerLog("Request completed.\n");
        }

        //  refresh the UI as the quantity of the item should now be changed
        ShowCollections();
    });

    if (hr != E_PENDING && FAILED(hr))
    {
        HttpManagerLog("Sample::ConsumeButton: MakeHttpRequestAsync failed with HRESULT = 0x%08x\n", static_cast<unsigned int>(hr));
    }
}

//---------------------------------------------------
//  Recurrence / Subscriptions specific functions
//---------------------------------------------------

/// <summary>
/// Calls the Sample Service to query for the user's subscriptions through the Recurrence service and displays
/// the data in the UI of the client sample.
/// </summary>
void Sample::ShowSubscriptions()
{
    OutputDebugStringA("Show subscriptions button is pressed\n");

    //  Enable the Subscriptions UI and hide the others
    m_collectionScreen->SetVisible(false);
    m_clawbackScreen->SetVisible(false);
    m_subscriptionScreen->SetVisible(true);
    m_currentUIPage = "Subscriptions";

    ////  Build the JSON body
    json jRequest;
    jRequest["UserPurchaseId"] = GetCachedUserPurchaseId(false);
    jRequest["UserId"] = m_gamerTag;
    jRequest["sbx"] = m_currentSandbox; //  Required when running / testing in a sandbox

    std::string requestBody = jRequest.dump();
    size_t bodySize = requestBody.size();

    std::vector<HttpHeader> headers;
    headers.push_back(HttpHeader("Content-Type", "application/json"));
    headers.push_back(HttpHeader("User-Agent", "Microsoft.StoreServicesClientSample"));

    HRESULT hr = m_httpManager->MakeHttpRequestAsync(nullptr, "POST", c_recurrenceQueryURL, headers, requestBody, bodySize, [this](HttpRequestContext* context)
    {
        if (context)
        {
            auto rawBody = context->responseBody.data();
            auto bodyLen = context->responseBody.size();
            std::string body(rawBody, rawBody + bodyLen);

            HttpManagerLog("Response status code: " + std::to_string(context->responseStatusCode) + "\n");
            HttpManagerLog("Response body size: " + std::to_string(bodyLen) + "\n");

            //  Because we are using the User-Agent header that identifies us as the client sample, we will
            //  get back the raw json from the b2b service response so that we can update our own UI and
            //  make future calls to the sample server.
            auto jsonStartPos = body.find_first_of('{', 0);
            if (jsonStartPos > 0 && jsonStartPos < bodyLen)
            {
                auto jsonString = body.substr(jsonStartPos);
                json j = json::parse(jsonString);

                std::vector<RecurrenceItem> expiredItems;   //  used so we can have the active
                                                            //  subscriptions shown first
                m_recurrenceItems.clear();

                //  Go through and transfer our results to the vector we use to
                //  show the right items in the UI based on which page we are on

                for (const auto& it : j["Items"])
                {
                    RecurrenceItem item;
                    from_json(it, item);
                    if (item.recurrenceState == "Active")
                    {
                        m_recurrenceItems.push_back(item);
                    }
                    else
                    {
                        expiredItems.push_back(item);
                    }
                }

                for (size_t i = 0; i < expiredItems.size(); i++)
                {
                    m_recurrenceItems.push_back(expiredItems[i]);
                }

                //  Passing 0 to this function will start us on the first page of the results in the UI
                ShowSubscriptionNextPage(0);
            }

            HttpManagerLog(body + "\n");
            HttpManagerLog("Request completed.\n");
        }
    });

    if (hr != E_PENDING && FAILED(hr))
    {
        HttpManagerLog("Sample::ShowSubscriptions: MakeHttpRequestAsync failed with HRESULT = 0x%08x\n", static_cast<unsigned int>(hr));
    }
}

/// <summary>
/// Switches to the next page of results based on the position passed in
/// </summary>
/// <param name="StartPos"></param>
void Sample::ShowSubscriptionNextPage(uint64_t StartPos)
{
    OutputDebugStringA("Subscription Next button is pressed\n");
    m_subscriptionScreen->SetVisible(true);

    //  If our starting position is larger than our array of items, loop back to the first page
    //  at position 0.
    auto totalItems = m_recurrenceItems.size();
    if (StartPos > totalItems)
    {
        m_subscriptionUIStart = 0;
    }
    else
    {
        m_subscriptionUIStart = StartPos;
    }

    //  If we are on the last page, make sure the end doesn't go past what we have
    //  in the results.
    m_subscriptionUIEnd = m_subscriptionUIStart + c_uiResultRowMax;
    if (m_subscriptionUIEnd > totalItems)
    {
        m_subscriptionUIEnd = totalItems;
    }

    //  Calculate the current page # and total pages for the UI
    size_t totalPages = totalItems / c_uiResultRowMax;
    if (totalItems % c_uiResultRowMax > 0)
    {
        totalPages++;
    }

    size_t currentPage;
    if (m_subscriptionUIEnd < c_uiResultRowMax
        && totalItems > 0)
    {
        currentPage = 1;
    }
    else
    {
        currentPage = m_subscriptionUIEnd / c_uiResultRowMax;
    }

    auto text = m_subscriptionPageNumber->GetTypedSubElementById<UIStaticText>(ID("panel_text"));
    std::string pageNumber = "Page " + std::to_string(currentPage) + "/" + std::to_string(totalPages);
    text->SetDisplayText(pageNumber);

    m_subscriptionPageNumber->SetVisible(true);

    //  Check if we need to show and enable the next page button
    if (totalPages > 1)
    {
        m_subscriptionNextPageButton->SetVisible(true);
    }
    else
    {
        m_subscriptionNextPageButton->SetVisible(false);
    }

    //  reset the sate of all the UI elements
    for (size_t i = 0; i < c_uiResultRowMax; i++)
    {
        m_subscriptionProductIdPanels[i]->SetVisible(false);
        m_subscriptionPostponeOrCancelButton[i]->SetVisible(false);
        m_subscriptionStartPanels[i]->SetVisible(false);
        m_subscriptionEndPanels[i]->SetVisible(false);
        m_subscriptionStatePanels[i]->SetVisible(false);
    }

    //  Go through the elements in the results that will fit on this page of the UI and populate the UI elements with their
    //  information.
    for (uint64_t itemPosition = m_subscriptionUIStart; itemPosition < m_subscriptionUIEnd; itemPosition++)
    {
        auto item = m_recurrenceItems[itemPosition];
        auto uiRow = itemPosition - m_subscriptionUIStart;

        m_subscriptionProductIdPanels[uiRow]->SetVisible(true);
        text = m_subscriptionProductIdPanels[uiRow]->GetTypedSubElementById<UIStaticText>(ID("panel_text"));
        text->SetDisplayText(item.productId);

        m_subscriptionPostponeOrCancelButton[uiRow]->SetVisible(true);
        text = m_subscriptionPostponeOrCancelButton[uiRow]->GetTypedSubElementById<UIStaticText>(ID("button_label"));

        //  Based on if the item is set for Auto-renew and is Active, we can turn off auto renew (postpone) or
        //  cancel the rest of the subscription
        if (item.recurrenceState == "Active")
        {
            if (item.autoRenew)
            {
                //  Disable Auto-Renew
                text->SetDisplayText("Postpone");
            }
            else
            {
                //  Cancel the rest of the subscription
                text->SetDisplayText("Cancel");
            }
        }
        //  If the item is not active, then we can provide a way to renew / repurchase the subscription
        else
        {
            text->SetDisplayText("Renew");
        }

        //  Trim the time from the start and end date (end date with Grace period) so they will
        //  fit in our UI
        std::string formattedDate;
        size_t trimPos;

        m_subscriptionStartPanels[uiRow]->SetVisible(true);
        text = m_subscriptionStartPanels[uiRow]->GetTypedSubElementById<UIStaticText>(ID("panel_text"));
        trimPos = item.startTime.find_first_of('T');
        formattedDate = item.startTime.substr(0, trimPos);
        text->SetDisplayText(formattedDate);

        m_subscriptionEndPanels[uiRow]->SetVisible(true);
        text = m_subscriptionEndPanels[uiRow]->GetTypedSubElementById<UIStaticText>(ID("panel_text"));
        trimPos = item.expirationTimeWithGrace.find_first_of('T');
        formattedDate = item.expirationTimeWithGrace.substr(0, trimPos);
        text->SetDisplayText(formattedDate);
    }
}

/// <summary>
/// Calls the Service Sample to request a change to the user's active subscription through the Recurrence service
/// </summary>
/// <param name="button_num"></param>
void Sample::PostponeOrCancelButton(uint8_t button_num)
{
    OutputDebugStringA("Postpone or Cancel button is pressed\n");

    uint64_t itemUIindex = m_subscriptionUIStart + button_num;

    //  Check if this is to renew / cancel or to purchase
    if (m_recurrenceItems[itemUIindex].recurrenceState == "Inactive" ||
        m_recurrenceItems[itemUIindex].recurrenceState == "Canceled" ||
        m_recurrenceItems[itemUIindex].recurrenceState == "Failed")
    {
        PurchaseItem(m_recurrenceItems[itemUIindex].productId);
        return;
    }

    ////  Build the JSON body
    json jRequest;
    jRequest["UserPurchaseId"] = GetCachedUserPurchaseId(false);
    jRequest["UserId"] = m_gamerTag;
    jRequest["RecurrenceId"] = m_recurrenceItems[itemUIindex].id;
    if (m_recurrenceItems[itemUIindex].autoRenew)
    {
        jRequest["ChangeType"] = "ToggleAutoRenew";
    }
    else
    {
        jRequest["ChangeType"] = "Cancel";
    }
    jRequest["sbx"] = m_currentSandbox;

    std::string requestBody = jRequest.dump();
    size_t bodySize = requestBody.size();

    std::vector<HttpHeader> headers;
    headers.push_back(HttpHeader("Content-Type", "application/json"));
    headers.push_back(HttpHeader("User-Agent", "Microsoft.StoreServicesClientSample"));

    HRESULT hr = m_httpManager->MakeHttpRequestAsync(nullptr, "POST", c_recurrenceChangeURL, headers, requestBody, bodySize, [this](HttpRequestContext* context)
    {
        if (context)
        {
            HttpManagerLog("Response status code: " + std::to_string(context->responseStatusCode) + "\n");
            HttpManagerLog("Response body size: " + std::to_string(context->responseBody.size()) + "\n");

            auto rawBody = context->responseBody.data();
            auto bodyLen = context->responseBody.size();
            std::string body(rawBody, rawBody + bodyLen);

            HttpManagerLog(body + "\n");
            HttpManagerLog("Request completed.\n");
        }

        //  refresh the UI now that changes have been requested
        ShowSubscriptions();
    });

    if (hr != E_PENDING && FAILED(hr))
    {
        HttpManagerLog("Sample::PostponeOrCancelButton: MakeHttpRequestAsync failed with HRESULT = 0x%08x\n", static_cast<unsigned int>(hr));
    }
}

//---------------------------------------------------
//  Clawback / Refund specific functions
//---------------------------------------------------

/// <summary>
/// Displays the buttons that allow the client to view the clawback information and run the reconciliation task
/// </summary>
void Sample::ShowClawback()
{
    OutputDebugStringA("Clawback button is pressed\n");

    //  Show the Clawback UI and hide the others
    m_collectionScreen->SetVisible(false);
    m_subscriptionScreen->SetVisible(false);
    m_clawbackScreen->SetVisible(true);
    m_currentUIPage = "Clawback";

    m_clawbackViewClawbackQueueButton->SetEnabled(true);
    m_clawbackViewUsersClawbackButton->SetEnabled(true);
    m_clawbackViewUserBalancesButton->SetEnabled(true);
    m_clawbackRunValidationButton->SetEnabled(true);
}

/// <summary>
/// Calls the Sample Service to query for the user's Clawback information and displays the results in the text console window.
/// </summary>
void Sample::QueryClawbackForUser()
{
    ////  Build the JSON body
    json jRequest;
    jRequest["UserPurchaseId"] = GetCachedUserPurchaseId(false);
    jRequest["UserId"] = m_gamerTag;
    jRequest["sbx"] = m_currentSandbox; //  Required when running / testing in a sandbox
    std::string requestBody = jRequest.dump();

    std::vector<HttpHeader> headers;
    headers.push_back(HttpHeader("Content-Type", "application/json"));

    size_t bodySize = requestBody.size();

    HRESULT hr = m_httpManager->MakeHttpRequestAsync(nullptr, "POST", c_clawbackQueryURL, headers, requestBody, bodySize, [this](HttpRequestContext* context)
    {
        if (context)
        {
            HttpManagerLog("Response status code: " + std::to_string(context->responseStatusCode) + "\n");
            HttpManagerLog("Response body size: " + std::to_string(context->responseBody.size()) + "\n");

            auto rawBody = context->responseBody.data();
            auto bodyLen = context->responseBody.size();
            std::string body(rawBody, rawBody + bodyLen);

            HttpManagerLog(body + "\n");
            HttpManagerLog("Request completed.\n");
        }
    });

    if (hr != E_PENDING && FAILED(hr))
    {
        HttpManagerLog("Sample::ShowRefund: MakeHttpRequestAsync failed with HRESULT = 0x%08x\n", static_cast<unsigned int>(hr));
    }
}

/// <summary>
/// Calls the Sample Service using a standard GET request with the provided URL then displays the result in the text console window.
/// </summary>
void Sample::SimpleServiceGetCall(std::string URL)
{
    std::vector<HttpHeader> headers;

    HRESULT hr = m_httpManager->MakeHttpRequestAsync(nullptr, "GET", URL, headers, nullptr, 0, [this](HttpRequestContext* context)
    {
        if (context)
        {
            HttpManagerLog("Response status code: " + std::to_string(context->responseStatusCode) + "\n");
            HttpManagerLog("Response body size: " + std::to_string(context->responseBody.size()) + "\n");

            auto rawBody = context->responseBody.data();
            auto bodyLen = context->responseBody.size();
            std::string body(rawBody, rawBody + bodyLen);

            HttpManagerLog(body + "\n");
            HttpManagerLog("Request completed.\n");
        }
    });

    if (hr != E_PENDING && FAILED(hr))
    {
        HttpManagerLog("Sample::OnHttpRequestButtonPressed: MakeHttpRequestAsync failed with HRESULT = 0x%08x", static_cast<unsigned int>(hr));
        m_cachingTokens = false;
    }
}

//---------------------------------------------------
//  AAD specific commerce authentication flow
//---------------------------------------------------
/// <summary>
/// Initiates a handshake with the game service to get the Purchase and Collections Access Tokens.
/// These are then exchanged for the UserStoreIds (UserCollectionsId, UserPurchaseId) that will be used
/// to authenticate to the Microsoft Store Services from the game service.
/// </summary>
void Sample::RefreshUserStoreIds()
{
    if (m_httpManager == nullptr)
    {
        return;
    }

    //  if the required access tokens are empty, get them first and then try this API again on the
    //  next update cycle
    if (m_cachedCollectionsAccessToken.empty() || m_cachedPurchaseAccessToken.empty())
    {
        m_waitingForTokens = true;
        m_showCollectionsButton->SetEnabled(false);
        m_showSubscriptionsButton->SetEnabled(false);
        m_showClawbackButton->SetEnabled(false);
        CacheAccessTokensFromService(c_accessTokenURL);
    }
    else
    {
        m_waitingForTokens = false;

        OutputDebugStringA("RefreshUserStoreIds button is pressed\n");

        //  Use the Collections Access Token to get the UserCollectionsId
        //  Use the Purchase Access Token to get the UserPurchaseId
        //  clear the cached access tokens
        m_cachedUserCollectionsId = "";
        m_cachedUserPurchaseId = "";

        //  Get a new UserCollectionsId and UserPurchaseId
        m_cachedUserCollectionsId = GetUserCollectionsId(m_cachedCollectionsAccessToken.c_str(), m_cachedUserId.c_str());
        m_cachedUserPurchaseId = GetUserPurchaseId(m_cachedPurchaseAccessToken.c_str(), m_cachedUserId.c_str());
       

        m_collectionScreen->SetVisible(false);
        m_subscriptionScreen->SetVisible(false);
    }
}

/// <summary>
/// Call the Sample Service, get the Access Tokens, then cache them for future use during the session
/// </summary>
/// <param name="ServerAccessTokenURL"></param>
void Sample::CacheAccessTokensFromService(std::string ServerAccessTokenURL)
{
    //  Verify we don't already have a request in the pipeline for this
    if (!m_cachingTokens)
    {
        m_cachingTokens = true;

        std::vector<HttpHeader> headers;

        HRESULT hr = m_httpManager->MakeHttpRequestAsync(nullptr, "GET", ServerAccessTokenURL, headers, nullptr, 0, [this](HttpRequestContext* context)
        {
            if (context)
            {
                HttpManagerLog("Response status code: " + std::to_string(context->responseStatusCode) + "\n");
                HttpManagerLog("Response body size: " + std::to_string(context->responseBody.size()) + "\n");

                auto rawBody = context->responseBody.data();
                auto bodyLen = context->responseBody.size();
                std::string body(rawBody, rawBody + bodyLen);

                //  get the keys from the server:
                //  { "UserID": "",
                //    "AccessTokens": [
                //        { "Audience": "", "Token" : "" },
                //        { "Audience": "", "Token" : "" }]}
                json j = json::parse(body.c_str());
                std::string userID = j["UserID"].get<std::string>();

                this->CacheUserId(userID);

                for (const auto& element : j["AccessTokens"])
                {
                    std::string token = element["Token"].get<std::string>();
                    std::string audience = element["Audience"].get<std::string>();
                    if (audience == "https://onestore.microsoft.com/b2b/keys/create/purchase")
                    {
                        this->CachePurchaseAccessToken(token);
                    }
                    else if (audience == "https://onestore.microsoft.com/b2b/keys/create/collections")
                    {
                        this->CacheCollectionsAccessToken(token);
                    }
                    else
                    {
                        HttpManagerLog("Request was unexpected: " + std::to_string(context->responseStatusCode) + "\n");
                    }
                }
                HttpManagerLog("Request completed.\n");
                this->m_cachingTokens = false;
            }
        });

        if (hr != E_PENDING && FAILED(hr))
        {
            HttpManagerLog("Sample::OnHttpRequestButtonPressed: MakeHttpRequestAsync failed with HRESULT = 0x%08x", static_cast<unsigned int>(hr));
            m_cachingTokens = false;
        }
    }
    return;
}

std::string Sample::GetUserPurchaseId(const char* serviceTicket, const char* publisherUserId)
{
    auto async = new XAsyncBlock{};
    async->context = this;

    HRESULT hr = XStoreGetUserPurchaseIdAsync(
        m_xStoreContext,
        serviceTicket,
        publisherUserId,
        async);

    //  NOTE:  ERROR_NO_SUCH_USER - 0x80070525 means that there is not a user signed
    //         into the Windows Store app or that credentials are needed to be refreshed 
    if (SUCCEEDED(hr))
    {
        //  Wait for this to complete before we continue
        hr = XAsyncGetStatus(async, true);
        if (SUCCEEDED(hr))
        {
            size_t size;
            hr = XStoreGetUserPurchaseIdResultSize(
                async,
                &size);

            if (FAILED(hr))
            {
                printf("Failed retrieve the User Purchase ID size: 0x%x\r\n", static_cast<unsigned int>(hr));
                return "";
            }

            char* result = new char[size];
            hr = XStoreGetUserPurchaseIdResult(
                async,
                size,
                result);

            if (FAILED(hr))
            {
                printf("Failed retrieve the User Purchase ID result: 0x%x\r\n", static_cast<unsigned int>(hr));
                delete[] result;
                return "";
            }

            printf("Caching UserPurchaseId\r\n");

            CacheUserPurchaseId(result);

            HttpManagerLog("UserPurchaseId cached");

            //  Now that we have the UserPurchaseId we can use the Purchase endpoints
            //  so enable those buttons in the UI
            m_showSubscriptionsButton->SetEnabled(true);
            m_showClawbackButton->SetEnabled(true);

            delete[] result;
        }
        else
        {
            printf("Failed retrieve the user purchase ID result: 0x%x\r\n", static_cast<unsigned int>(hr));
            return "";
        }
    }
    delete async;

    return m_cachedUserPurchaseId;
}

std::string Sample::GetUserCollectionsId(const char* serviceTicket, const char* publisherUserId)
{
    auto async = new XAsyncBlock{};
    async->context = this;

    HRESULT hr = XStoreGetUserCollectionsIdAsync(
        m_xStoreContext,
        serviceTicket,
        publisherUserId,
        async);

    //  NOTE:  ERROR_NO_SUCH_USER - 0x80070525 means that there is not a user signed
    //         into the Windows Store app or that credentials are needed to be refreshed 
    if (SUCCEEDED(hr))
    {
        //  Wait for this to complete before we continue
        hr = XAsyncGetStatus(async, true);
        if (SUCCEEDED(hr))
        {
            size_t size;
            hr = XStoreGetUserCollectionsIdResultSize(
                async,
                &size);

            if (FAILED(hr))
            {
                printf("Failed retrieve the user collection ID size: 0x%x\r\n", static_cast<unsigned int>(hr));
                return "";
            }

            char* result = new char[size];
            hr = XStoreGetUserCollectionsIdResult(
                async,
                size,
                result);

            if (FAILED(hr))
            {
                printf("Failed retrieve the user collection ID result: 0x%x\r\n", static_cast<unsigned int>(hr));
                delete[] result;
                return "";
            }

            printf("Caching UserCollectionsId\r\n");

            CacheUserCollectionsId(result);

            //  Now that we have the UserPurchaseId we can use the Purchase endpoints
            //  so enable those buttons in the UI
            m_showCollectionsButton->SetEnabled(true);

            HttpManagerLog("UserCollectionsId cached");

            delete[] result;
        }
        else
        {
            printf("Failed retrieve the user collection ID result: 0x%x\r\n", static_cast<unsigned int>(hr));
            return "";
        }
    }
    delete async;

    return m_cachedUserCollectionsId;
}

std::string Sample::GetCachedUserCollectionsId(bool ForceRefresh)
{
    if (ForceRefresh || m_cachedUserCollectionsId.empty())
    {
        RefreshUserStoreIds();
    }

    return m_cachedUserCollectionsId;
}

std::string Sample::GetCachedUserPurchaseId(bool ForceRefresh)
{
    if (ForceRefresh || m_cachedUserPurchaseId.empty())
    {
        RefreshUserStoreIds();
    }

    return m_cachedUserPurchaseId;
}
