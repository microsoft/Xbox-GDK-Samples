//--------------------------------------------------------------------------------------
// PlayFabStore.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PlayFabStore.h"
#include "ATGColors.h"

// PlayFab Title Id
constexpr const char* TITLE_ID = "4E29";
constexpr const char* STACK_ID = "default";
constexpr const char* ITEM_TYPE = "Item";
constexpr const char* WALLET_TYPE = "Wallet";

// PlayFab Item Ids
constexpr const char* WALLET_GOLD = "171b97e2-cb8f-47a8-9815-9de0a11ebf41";
constexpr const char* ITEM_POTION = "44bee96d-9d99-49e1-bb8a-f0c86d47cd7c";
constexpr const char* ITEM_POTIONS5 = "7ae7359c-d709-4dbf-a5dd-0c4a8459a858";
constexpr const char* ITEM_POTIONS25 = "16e08694-5af4-4c4f-a938-d3972a55f901";
constexpr const char* ITEM_SHIELD = "0e605b6a-3340-4dd3-9480-4e002be006a5";
constexpr const char* ITEM_SWORD = "779dce58-af79-439e-9332-81ba19d0ca20";
constexpr const char* ITEM_BOW = "a39351a7-e899-4091-adee-e873d7bf3660";
constexpr const char* ITEM_GEM = "14c35058-920b-41fd-b469-e94c6cf9f7a2";
constexpr const char* ITEM_ARROW = "095234fb-7ffc-4c16-9713-5a02bb31d043";
constexpr const char* ITEM_ARROWS5 = "e4ffdf9f-7cb2-41e7-81a9-c335b3d61999";
constexpr const char* ITEM_ARROWS25 = "0f555fe9-700b-4d0b-ae52-035457a4018d";
constexpr const char* ITEM_ARROWS100 = "ad4b8626-95c5-4b19-ab08-72d7587ab3d5";

// Microsoft Store Ids (dev-managed consumables)
constexpr const char* ITEM_GOLD100 = "9PB60ZPT6G8G";
constexpr const char* ITEM_GOLD500 = "9PF4M9X6X2RN";
constexpr const char* ITEM_GOLD1000 = "9NFTP878QG2W";

// PlayFab Catalog Ids
constexpr const uint32_t c_pfCatalogItemsCount = 9;

const char* c_pfCatalogItemIds[] =  {
    ITEM_GEM, ITEM_BOW, ITEM_SWORD,
    ITEM_SHIELD, ITEM_ARROWS5, ITEM_ARROWS25,
    ITEM_ARROWS100, ITEM_POTIONS5, ITEM_POTIONS25
};

// PlayFab Inventory Ids
constexpr const uint32_t c_pfInventoryItemsCount = 6;

const char* c_pfInventoryItemIds[] = {
    ITEM_GEM, ITEM_SHIELD, ITEM_BOW,
    ITEM_SWORD, ITEM_ARROW, ITEM_POTION
};

// Sample definitions
extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

namespace
{
    Sample* s_sample = nullptr;

    template <size_t bufferSize = 2048>
    void ConsoleWriteLine(std::string_view format, ...)
    {
        assert(format.size() < bufferSize && "format string is too large, split up the string or increase the buffer size");

        static char buffer[bufferSize] = "";

        va_list args;
        va_start(args, format);
        vsprintf_s(buffer, format.data(), args);
        va_end(args);

        OutputDebugStringA(buffer);
        OutputDebugStringA("\n");

        if (s_sample)
        {
            auto console = s_sample->GetConsole();
            if (console)
            {
                console->AppendLineOfText(buffer);
            }
        }
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_asyncQueue(nullptr),
    m_showConsole(false)
{
    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_asyncQueue)
    );

    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);

    m_liveResources = std::make_unique<ATG::LiveResources>(m_asyncQueue);
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("");
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    if (m_xStoreContext)
    {
        XStoreCloseContextHandle(m_xStoreContext);
        m_xStoreContext = nullptr;
    }

    if (m_asyncQueue)
    {
        XTaskQueueCloseHandle(m_asyncQueue);
        m_asyncQueue = nullptr;
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    s_sample = this;

    m_msProductsInitialized = false;
    m_pfCatalogInitialized = false;

    m_pfInventoryCounts[WALLET_GOLD] = 0;
    for (auto item : c_pfInventoryItemIds)
    {
        m_pfInventoryCounts[item] = 0;
    }

    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // This sample does not properly handle user changes during Suspend/Constrain
    // https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xbox-game-life-cycle
    m_liveResources->SetUserChangedCallback([this](XUserHandle user)
        {
            m_liveInfoHUD->SetUser(user, m_asyncQueue);

            ConsoleWriteLine("Xbox Live: gamertag \"%s\" signed in", m_liveResources->GetGamertag().c_str());

#ifdef _GAMING_XBOX
            HRESULT hr = XStoreCreateContext(m_liveResources->GetUser(), &m_xStoreContext);
#else
            HRESULT hr = XStoreCreateContext(nullptr, &m_xStoreContext);
#endif
            ConsoleWriteLine("XStoreCreateContext 0x%x", hr);

            QueryStoreProducts();

            PlayFabSignIn();
        });

    m_liveResources->SetUserSignOutCompletedCallback([this](XUserHandle /*user*/)
        {
            m_liveInfoHUD->SetUser(nullptr, m_asyncQueue);
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

    m_liveResources->Initialize();
    m_liveInfoHUD->Initialize();

    InitializeUI();
}

// Sign into PlayFab with the same account that is signed into Xbox Live
void Sample::PlayFabSignIn()
{
    if (m_playFabResources)
    {
        m_playFabResources->Cleanup();
        m_playFabResources = nullptr;
    }

    m_playFabResources = std::make_unique<ATG::PlayFabResources>(TITLE_ID, m_liveResources->GetUser());
    m_playFabResources->LoginToPlayFab();

    QueryPlayFabProducts(c_pfCatalogItemIds, c_pfCatalogItemsCount);
    QueryPlayFabInventory();
}

// Get products from the Microsoft Store
void Sample::QueryStoreProducts()
{
    ConsoleWriteLine("Calling XStoreQueryProductsAsync");

    auto async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;

    async->callback = [](XAsyncBlock* async)
    {
        auto pThis = static_cast<Sample*>(async->context);

        XStoreProductQueryHandle queryHandle;
        HRESULT hr = XStoreQueryProductsResult(async, &queryHandle);
        if (SUCCEEDED(hr))
        {
            hr = XStoreEnumerateProductsQuery(queryHandle, async->context,
                [](const XStoreProduct* product, void* context) -> bool
                {
                    auto pThis = static_cast<Sample*>(context);

                    pThis->m_productDetails[product->storeId] =
                        std::pair<std::string, std::string>(product->title, product->price.formattedBasePrice);
                    ConsoleWriteLine("Store product %s %s %s", product->storeId, product->title, product->price.formattedBasePrice);

                    return true;
                });

            if (SUCCEEDED(hr))
            {
                // This sample doesn't handle paging as it should when calling XStoreQueryProducts
                assert(XStoreProductsQueryHasMorePages(queryHandle) == false);
            }
        }
        else
        {
            ConsoleWriteLine("Error calling XStoreEnumerateProductsQuery : 0x%08X", hr);
        }

        if (!pThis->m_msProductsInitialized)
        {
            pThis->InitializeMicrosoftStoreProductsUI();
        }

        delete async;
    };

    XStoreProductKind typeFilter =
        XStoreProductKind::UnmanagedConsumable;

    const char* storeIds[] = { ITEM_GOLD100, ITEM_GOLD500, ITEM_GOLD1000 };
    HRESULT hr = XStoreQueryProductsAsync(
        m_xStoreContext,
        typeFilter,     // Product filter types
        storeIds,
        ARRAYSIZE(storeIds),
        nullptr,
        0,
        async);

    if (FAILED(hr))
    {
        delete async;
        ConsoleWriteLine("Error calling XStoreQueryProductsAsync : 0x%08X", hr);
    }
}

// Purchase a product from the Microsoft Store
void Sample::PurchaseStoreProduct(const char* storeId)
{
    // https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowpurchaseuiasync
    ConsoleWriteLine("Calling XStoreShowPurchaseUIAsync");

    auto async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock *async)
    {
        auto pThis = static_cast<Sample*>(async->context);

        HRESULT hr = XStoreShowPurchaseUIResult(async);

        ConsoleWriteLine("XStoreShowPurchaseUIResult 0x%08X", hr);

        if (SUCCEEDED(hr))
        {
            pThis->RedeemStoreProducts();
        }

        delete async;
    };

    HRESULT hr = XStoreShowPurchaseUIAsync(
        m_xStoreContext,
        storeId,
        nullptr,
        nullptr,
        async);

    if (FAILED(hr))
    {
        delete async;
        m_statusText->SetDisplayText("Microsoft Store purchase failed");
        ConsoleWriteLine("Error calling XStoreShowPurchaseUIAsync : 0x%08X", hr);
    }
}

// Get all catalog items in the specified list
void Sample::QueryPlayFabProducts(const char** itemIds, const uint32_t itemIdsCount)
{
    // https://learn.microsoft.com/en-us/rest/api/playfab/economy/catalog/get-items
    ConsoleWriteLine("Calling PFCatalogGetItemsAsync");

    PFCatalogGetItemsRequest request{};
    request.entity = m_playFabResources->GetPlayerEntityKey();
    request.ids = itemIds;
    request.idsCount = itemIdsCount;

    auto async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock* async)
    {
        auto pThis = static_cast<Sample*>(async->context);

        size_t resultSize;
        HRESULT hr = PFCatalogGetItemsGetResultSize(async, &resultSize);

        if (SUCCEEDED(hr))
        {
            std::vector<char> resultBuffer(resultSize);
            PFCatalogGetItemsResponse* responseResult{ nullptr };
            hr = PFCatalogGetItemsGetResult(
                async,
                resultBuffer.size(),
                resultBuffer.data(),
                &responseResult,
                nullptr
            );

            if (SUCCEEDED(hr) && responseResult)
            {
                uint32_t itemsCount = responseResult->itemsCount;
                ConsoleWriteLine("Catalog items (%d):", itemsCount);

                for (uint32_t i = 0; i < itemsCount; ++i)
                {
                    std::string itemInfo;
                    std::string itemId;
                    std::string itemTitle;
                    int32_t itemPrice;

                    PFCatalogCatalogItem item = *responseResult->items[i];

                    itemId = std::string(item.id);
                    ConsoleWriteLine(" Id: %s", item.id);
                    ConsoleWriteLine("\tType: %s", item.type);

                    for (uint32_t j = 0; j < item.alternateIdsCount; ++j)
                    {
                        PFCatalogCatalogAlternateId altId = *item.alternateIds[j];
                        itemInfo = "\t" + std::string(altId.type) + ": " + std::string(altId.value) + " ";
                        ConsoleWriteLine(itemInfo);
                    }

                    for (uint32_t j = 0; j < item.titleCount; ++j)
                    {
                        PFStringDictionaryEntry title{};
                        title.key = item.title[j].key;
                        title.value = item.title[j].value;
                        itemInfo = "\tTitle (" + std::string(title.key) + "): " + std::string(title.value);

                        if (j == 0)
                        {
                            // Display the 'neutral' title in UI
                            itemTitle = std::string(title.value);
                        }

                        ConsoleWriteLine(itemInfo);
                    }

                    for(uint32_t j = 0; j < item.priceOptions->pricesCount; ++j)
                    {
                        PFCatalogCatalogPrice price = *item.priceOptions->prices[j];
                        for(uint32_t k = 0; k < price.amountsCount; ++k)
                        {
                            PFCatalogCatalogPriceAmount priceAmount = *price.amounts[k];
                            itemPrice = priceAmount.amount;
                            std::string currencyId = priceAmount.itemId;

                            itemInfo = "\tPrice: " + std::to_string(itemPrice);

                            if (k == 0 && currencyId.compare(WALLET_GOLD) == 0)
                            {
                                // Display the base currency in UI
                                itemPrice = priceAmount.amount;
                                itemInfo += " Gold";
                            }
                            else
                            {
                                itemInfo += " Currency ID: " + currencyId;
                            }

                            ConsoleWriteLine(itemInfo);
                        }
                    }

                    // Save item's title and price for display in UI
                    pThis->m_pfCatalogDetails[itemId] = std::pair<std::string, int32_t>(itemTitle, itemPrice);
                }

                if (!pThis->m_pfCatalogInitialized)
                {
                    pThis->InitializePlayFabProductsUI();
                }

                pThis->m_statusText->SetDisplayText(std::to_string(itemsCount) + " catalog items found");
            }
            else
            {
                pThis->m_statusText->SetDisplayText("Query Catalog failed");
                ConsoleWriteLine("Error calling PFCatalogGetItemsGetResult : 0x%08X", hr);
            }
        }
        else
        {
            pThis->m_statusText->SetDisplayText("Query Catalog failed");
            ConsoleWriteLine("Error calling PFCatalogGetItemsGetResultSize : 0x%08X", hr);
        }

        delete async;
    };

    HRESULT hr = PFCatalogGetItemsAsync(m_playFabResources->GetEntityHandle(), &request, async);

    if (FAILED(hr))
    {
        delete async;
        m_statusText->SetDisplayText("Query Catalog failed");
        ConsoleWriteLine("Error calling PFCatalagGetItemsAsync : 0x%08X", hr);
    }
}

// Get all items in the player's inventory
void Sample::QueryPlayFabInventory(const char* continuationToken)
{
    // https://learn.microsoft.com/en-us/rest/api/playfab/economy/inventory/get-inventory-items
    ConsoleWriteLine("Calling PFInventoryGetInventoryItemsAsync");

    PFInventoryGetInventoryItemsRequest request{};
    request.entity = m_playFabResources->GetPlayerEntityKey();
    request.count = 25;
    request.continuationToken = continuationToken;

    auto async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock* async)
    {
        auto pThis = static_cast<Sample*>(async->context);

        size_t resultSize{};
        HRESULT hr = PFInventoryGetInventoryItemsGetResultSize(async, &resultSize);

        if(SUCCEEDED(hr))
        {
            std::vector<char> resultBuffer(resultSize);
            PFInventoryGetInventoryItemsResponse* responseResult{ nullptr };
            hr = PFInventoryGetInventoryItemsGetResult(
                async,
                resultBuffer.size(),
                resultBuffer.data(),
                &responseResult,
                nullptr
            );

            if (SUCCEEDED(hr) && responseResult)
            {
                uint32_t itemsCount = responseResult->itemsCount;
                ConsoleWriteLine("Inventory items (%d):", itemsCount);

                for (uint32_t i = 0; i < itemsCount; ++i)
                {
                    PFInventoryInventoryItem item = *responseResult->items[i];
                    std::string itemId = item.id;
                    std::string itemAmount = std::to_string(*item.amount);

                    pThis->m_pfInventoryCounts[itemId] = *item.amount;

                    ConsoleWriteLine(" Id: %s Type: %s Amount: %s, StackId: %s",
                        item.id,
                        item.type,
                        itemAmount.c_str(),
                        item.stackId);

                    for (uint32_t j = 0; j < c_pfInventoryItemsCount; ++j)
                    {
                        if (itemId == c_pfInventoryItemIds[j])
                        {
                            pThis->m_items[j]->SetDisplayText(itemAmount);
                        };
                    }

                    if (itemId.compare(WALLET_GOLD) == 0)
                    {
                        pThis->m_gold->SetDisplayText(itemAmount + std::string(" Gold"));
                    }
                }

                // Handle paging
                // TODO: verify this works, currently we don't have enough content to trigger paging
                if (responseResult->continuationToken)
                {
                    pThis->QueryPlayFabInventory(responseResult->continuationToken);
                }
            }
            else
            {
                pThis->m_statusText->SetDisplayText("Query Inventory failed");
                ConsoleWriteLine("Error calling PFInventoryGetInventoryItemsGetResult : 0x%08X", hr);
            }
        }
        else
        {
            pThis->m_statusText->SetDisplayText("Query Inventory failed");
            ConsoleWriteLine("Error calling PFInventoryGetInventoryItemsGetResultSize : 0x%08X", hr);
        }

        delete async;        
    };

    HRESULT hr = PFInventoryGetInventoryItemsAsync(m_playFabResources->GetEntityHandle(), &request, async);

    if (FAILED(hr))
    {
        delete async;
        m_statusText->SetDisplayText("Query Inventory failed");
        ConsoleWriteLine("Error calling PFInventoryGetInventoryItemsAsync : 0x%08X", hr);
    }
}

// Purchase a single item or bundle, paying the associated price
// Susceptible to throttle (client limited to 10 write requests over 30 sec)
void Sample::PurchasePlayFabItem(const char* itemId, const int32_t amount, const int32_t price)
{
    // https://learn.microsoft.com/en-us/rest/api/playfab/economy/inventory/purchase-inventory-items
    ConsoleWriteLine("Calling PFInventoryPurchaseInventoryItemsAsync for item: %s", itemId);

    // Verify user has enough funds before calling service
    if ((m_pfInventoryCounts.at(WALLET_GOLD) - (amount * price)) < 0)
    {
        ConsoleWriteLine("Insufficient funds to purchase item: %s", itemId);
        m_statusText->SetDisplayText("Not enough funds");
        return;
    }

    PFInventoryPurchaseInventoryItemsRequest request{};
    request.entity = m_playFabResources->GetPlayerEntityKey();
    request.amount = &amount;

    PFInventoryInventoryItemReference item{};
    item.id = itemId;
    item.stackId = STACK_ID;
    request.item = &item;

    const PFInventoryPurchasePriceAmount purchasePrice[1] =
    {
        {price, WALLET_GOLD, STACK_ID}
    };

    PFInventoryPurchasePriceAmount const* pPriceAmounts = purchasePrice;
    request.priceAmountsCount = 1;
    request.priceAmounts = &pPriceAmounts;

    auto async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock* async)
    {
        auto pThis = static_cast<Sample*>(async->context);

        size_t resultSize{};
        HRESULT hr = PFInventoryPurchaseInventoryItemsGetResultSize(async, &resultSize);

        if (SUCCEEDED(hr))
        {
            std::vector<char> resultBuffer(resultSize);
            PFInventoryPurchaseInventoryItemsResponse* responseResult{ nullptr };
            hr = PFInventoryPurchaseInventoryItemsGetResult(
                async,
                resultBuffer.size(),
                resultBuffer.data(),
                &responseResult,
                nullptr
            );

            if (SUCCEEDED(hr) && responseResult && responseResult->transactionIdsCount > 0)
            {
                pThis->m_statusText->SetDisplayText("Purchase succeeded");
                ConsoleWriteLine("Purchase succeeded");
                pThis->QueryPlayFabInventory();
            }
            else
            {
                pThis->m_statusText->SetDisplayText("Purchase failed");
                ConsoleWriteLine("Error calling PFInventoryPurchaseInventoryItemsGetResult : 0x%08X", hr);
            }
        }
        else
        {
            pThis->m_statusText->SetDisplayText("Purchase failed");
            ConsoleWriteLine("Error calling PFInventoryPurchaseInventoryItemsGetResultSize : 0x%08X", hr);
        }

        delete async;
    };

    HRESULT hr = PFInventoryPurchaseInventoryItemsAsync(m_playFabResources->GetEntityHandle(), &request, async);

    if (FAILED(hr))
    {
        delete async;
        m_statusText->SetDisplayText("Purchase failed");
        ConsoleWriteLine("Error calling PFInventoryPurchaseInventoryItemsAsync : 0x%08X", hr);
    }
}

// Decrease inventory item by the specified amount
// Susceptible to throttle (client limited to 10 write requests over 30 sec)
void Sample::SubtractPlayFabItem(const char* itemId, const int32_t amount)
{
    // https://learn.microsoft.com/en-us/rest/api/playfab/economy/inventory/subtract-inventory-items
    ConsoleWriteLine("Calling PFInventorySubtractInventoryItemsAsync for item: %s", itemId);

    // Verify user has enough items before calling service
    if ((m_pfInventoryCounts.at(itemId) - amount) < 0)
    {
        ConsoleWriteLine("Insufficient inventory to subtract from item: %s", itemId);
        m_statusText->SetDisplayText("Not enough items");
        return;
    }

    PFInventorySubtractInventoryItemsRequest request{};
    request.entity = m_playFabResources->GetPlayerEntityKey();
    request.amount = &amount;

    PFInventoryInventoryItemReference item{};
    item.id = itemId;
    item.stackId = STACK_ID;
    request.item = &item;

    auto async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock* async)
    {
        auto pThis = static_cast<Sample*>(async->context);

        size_t resultSize;
        HRESULT hr = PFInventorySubtractInventoryItemsGetResultSize(async, &resultSize);

        if (SUCCEEDED(hr))
        {
            std::vector<char> resultBuffer(resultSize);
            PFInventorySubtractInventoryItemsResponse* responseResult{ nullptr };
            hr = PFInventorySubtractInventoryItemsGetResult(
                async,
                resultBuffer.size(),
                resultBuffer.data(),
                &responseResult,
                nullptr
            );

            if (SUCCEEDED(hr) && responseResult && responseResult->transactionIdsCount > 0)
            {
                pThis->m_statusText->SetDisplayText("Subtract succeeded");
                ConsoleWriteLine("Subtract succeeded");
                pThis->QueryPlayFabInventory();
            }
            else
            {
                pThis->m_statusText->SetDisplayText("Subtract failed");
                ConsoleWriteLine("Error calling PFInventorySubtractInventoryItemsGetResult : 0x%08X", hr);
            }
        }
        else
        {
            pThis->m_statusText->SetDisplayText("Subtract failed");
            ConsoleWriteLine("Error calling PFInventorySubtractInventoryItemsGetResultSize : 0x%08X", hr);
        }

        delete async;
    };

    HRESULT hr = PFInventorySubtractInventoryItemsAsync(m_playFabResources->GetEntityHandle(), &request, async);

    if (FAILED(hr))
    {
        delete async;
        m_statusText->SetDisplayText("Subtract failed");
        ConsoleWriteLine("Error calling PFInventorySubtractInventoryItemsAsync : 0x%08X", hr);
    }
}

// Consume entitlements purchased via the Microsoft Store
void Sample::RedeemStoreProducts()
{
    // https://learn.microsoft.com/en-us/rest/api/playfab/economy/inventory/redeem-microsoft-store-inventory-items
    ConsoleWriteLine("Calling PFInventoryRedeemMicrosoftStoreInventoryItemsAsync");
    m_statusText->SetDisplayText("Redeeming Microsoft Entitlements ...");

    PFInventoryRedeemMicrosoftStoreInventoryItemsRequest request{};
    request.entity = m_playFabResources->GetPlayerEntityKey();
    request.xboxToken = m_playFabResources->GetXboxToken();

    auto async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock* async)
    {
        auto pThis = static_cast<Sample*>(async->context);

        size_t resultSize;
        HRESULT hr = PFInventoryRedeemMicrosoftStoreInventoryItemsGetResultSize(async, &resultSize);

        if (SUCCEEDED(hr))
        {
            std::vector<char> resultBuffer(resultSize);
            PFInventoryRedeemMicrosoftStoreInventoryItemsResponse* responseResult{ nullptr };
            hr = PFInventoryRedeemMicrosoftStoreInventoryItemsGetResult(
                async,
                resultBuffer.size(),
                resultBuffer.data(),
                &responseResult,
                nullptr
            );

            if (SUCCEEDED(hr) && responseResult)
            {
                uint32_t succeededCount = responseResult->succeededCount;
                if (succeededCount > 0)
                {
                    for (uint32_t i = 0; i < succeededCount; ++i)
                    {
                        PFInventoryRedemptionSuccess item = *responseResult->succeeded[i];
                        ConsoleWriteLine("Store ID: %s redeemed", item.offerId);
                    }

                    pThis->QueryPlayFabInventory();
                    ConsoleWriteLine("Gold balance updated");
                    pThis->m_statusText->SetDisplayText(std::string("Gold balance updated"));
                }
                else
                {
                    ConsoleWriteLine("No items to redeem");
                    pThis->m_statusText->SetDisplayText(std::string("No items to redeem"));
                }
            }
            else
            {
                pThis->m_statusText->SetDisplayText("Redeem failed");
                ConsoleWriteLine("Error calling PFInventoryRedeemMicrosoftStoreInventoryItemsGetResult : 0x%08X", hr);
            }
        }
        else
        {
            pThis->m_statusText->SetDisplayText("Redeem failed");
            ConsoleWriteLine("Error calling PFInventoryRedeemMicrosoftStoreInventoryItemsGetResultSize : 0x%08X", hr);
        }

        delete async;
    };

    HRESULT hr = PFInventoryRedeemMicrosoftStoreInventoryItemsAsync(m_playFabResources->GetEntityHandle(), &request, async);

    if (FAILED(hr))
    {
        delete async;
        m_statusText->SetDisplayText("Redeem failed");
        ConsoleWriteLine("Error calling PFInventoryRedeemMicrosoftStoreInventoryItemsAsync : 0x%08X", hr);
    }
}

// Update the player's inventory item to the specified amount
// Susceptible to throttle (client limited to 10 write requests over 30 sec)
// Note: Default policy settings don't support calling Update from client, must enable for 'Player' in Game Manager under Title > Settings > Policy
// Recommended: Don't use Update in client. Instead, use Purchase for increases and Subtract for decreases. Used here for ease of testing.
void Sample::UpdatePlayFabItem(const char* itemId, const char* type, const int32_t amount)
{
    // https://learn.microsoft.com/en-us/rest/api/playfab/economy/inventory/update-inventory-items
    ConsoleWriteLine("Calling PFInventoryUpdateInventoryItemsAsync for item: %s", itemId);

    // verify update is possible before calling service
    if (m_pfInventoryCounts.at(itemId) == amount)
    {
        ConsoleWriteLine("No update required for item: %s", itemId);
        m_statusText->SetDisplayText("No update required");
        return;
    }

    PFInventoryUpdateInventoryItemsRequest request{};
    request.entity = m_playFabResources->GetPlayerEntityKey();

    PFInventoryInventoryItem item{};
    item.id = itemId;
    item.type = type;
    item.amount = &amount;
    item.stackId = STACK_ID;
    request.item = &item;

    auto async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock* async)
    {
        auto pThis = static_cast<Sample*>(async->context);

        size_t resultSize;
        HRESULT hr = PFInventoryUpdateInventoryItemsGetResultSize(async, &resultSize);

        if (SUCCEEDED(hr))
        {
            std::vector<char> resultBuffer(resultSize);
            PFInventoryUpdateInventoryItemsResponse* responseResult{ nullptr };
            hr = PFInventoryUpdateInventoryItemsGetResult(
                async,
                resultBuffer.size(),
                resultBuffer.data(),
                &responseResult,
                nullptr
            );

            if (SUCCEEDED(hr) && responseResult && responseResult->transactionIdsCount > 0)
            {
                pThis->m_statusText->SetDisplayText("Update succeeded");
                ConsoleWriteLine("Update succeeded");
                pThis->QueryPlayFabInventory();
            }
            else
            {
                ConsoleWriteLine("Error calling PFInventoryUpdateInventoryItemsGetResult : 0x%08X", hr);
            }
        }
        else
        {
            ConsoleWriteLine("Error calling PFInventoryUpdateInventoryItemsGetResultSize : 0x%08X", hr);
        }

        delete async;
    };

    HRESULT hr = PFInventoryUpdateInventoryItemsAsync(m_playFabResources->GetEntityHandle(), &request, async);

    if (FAILED(hr))
    {
        delete async;
        m_statusText->SetDisplayText("Update failed");
        ConsoleWriteLine("Error calling PFInventoryUpdateInventoryItemsAsync : 0x%08X", hr);
    }
}

#pragma region UI Methods

#ifdef _GAMING_DESKTOP
auto highlightState = UIButton::State::Hovered;
#else
auto highlightState = UIButton::State::Focused;
#endif

std::vector<std::shared_ptr<ATG::UITK::UIButton>> g_msProductButtons;
std::vector<std::shared_ptr<ATG::UITK::UIButton>> g_pfCatalogButtons;

// Initializes the PlayFab catalog items
void Sample::InitializePlayFabProductsUI()
{
    ConsoleWriteLine("Initialize PlayFabStore products");

    if (m_pfCatalogDetails.empty())
    {
        ConsoleWriteLine("PlayFab products are not ready!");
        return;
    }

    AddPlayFabProductButton(ITEM_ARROWS100, "ArrowStyle");
    AddPlayFabProductButton(ITEM_ARROWS25, "ArrowStyle");
    AddPlayFabProductButton(ITEM_ARROWS5, "ArrowStyle");
    AddPlayFabProductButton(ITEM_BOW, "BowStyle");
    AddPlayFabProductButton(ITEM_GEM, "GemStyle");
    AddPlayFabProductButton(ITEM_POTIONS25, "PotionStyle");
    AddPlayFabProductButton(ITEM_POTIONS5, "PotionStyle");
    AddPlayFabProductButton(ITEM_SHIELD, "ShieldStyle");
    AddPlayFabProductButton(ITEM_SWORD, "SwordStyle");

    m_pfCatalogInitialized = true;
    ConsoleWriteLine("PlayFab Catalog items ready for purchase");

    // PlayFabStore is now ready for user interaction
    m_contentPanel->SetVisible(true);

    if (m_msProductsInitialized)
    {
        auto toFocus = g_msProductButtons.front();
        m_uiManager.SetFocus(toFocus);
    }
}

// Initializes the Microsoft Store catalog items
void Sample::InitializeMicrosoftStoreProductsUI()
{
    ConsoleWriteLine("Initialize Microsoft Store products");

    if (m_productDetails.empty())
    {
        ConsoleWriteLine("Microsoft Store products are not ready!");
        return;
    }

    AddStoreProductButton(ITEM_GOLD100, "GoldSImageStyle");
    AddStoreProductButton(ITEM_GOLD500, "GoldMImageStyle");
    AddStoreProductButton(ITEM_GOLD1000, "GoldLImageStyle");

    m_msProductsInitialized = true;
    ConsoleWriteLine("Microsoft Store products ready for purchase");
}

// Creates a button to enable purchase of the specified PlayFab catalog item
void Sample::AddPlayFabProductButton(std::string itemId, std::string imageStyleId)
{
    if (m_pfCatalogDetails.find(itemId) != m_pfCatalogDetails.end())
    {
        std::string title = m_pfCatalogDetails[itemId].first;
        int32_t price = m_pfCatalogDetails[itemId].second;

        auto button = CastPtr<UIButton>(m_itemMenuPF->AddChildFromPrefab("#menu_item_prefab"));
        button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText(title);
        button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this, itemId, price](UIButton*)
            {
                m_statusText->SetDisplayText("");
                PurchasePlayFabItem(itemId.c_str(), 1, price);
            });
        button->ButtonState().AddListenerWhen(highlightState,
            [this, itemId, price, imageStyleId](UIButton*)
            {
                m_selectedItemImage->SetStyleId(ID(imageStyleId));
                m_selectedItemGold->SetDisplayText(std::to_string(price) + " Gold");
                m_aButtonText->SetDisplayText("[A] Purchase with Gold");
            });

        g_pfCatalogButtons.push_back(button);      
    }
}

// Creates a button to enable purchase of the specified Microsoft Store product
void Sample::AddStoreProductButton(std::string storeId, std::string imageStyleId)
{
    if (m_productDetails.find(storeId) != m_productDetails.end())
    {
        std::string title = m_productDetails[storeId].first;
        std::string price = m_productDetails[storeId].second;

        auto button = CastPtr<UIButton>(m_itemMenu->AddChildFromPrefab("#menu_item_prefab"));
        button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText(title);
        button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this, storeId, price](UIButton*)
            {
                m_statusText->SetDisplayText("");
                PurchaseStoreProduct(storeId.c_str());
            });
        button->ButtonState().AddListenerWhen(highlightState,
            [this, storeId, price, imageStyleId](UIButton*)
            {
                m_selectedItemImage->SetStyleId(ID(imageStyleId));
                m_selectedItemGold->SetDisplayText(price);
                m_aButtonText->SetDisplayText("[A] Purchase with real money");
            });

        g_msProductButtons.push_back(button);
    }
}

// Initializes basic UI layout, buttons and menus
void Sample::InitializeUI()
{
    m_mainLayout = m_uiManager.LoadLayoutFromFile("Assets/UILayout.json");
    m_uiManager.AttachTo(m_mainLayout, m_uiManager.GetRootElement());

    // log window for debug
    m_console = m_uiManager.FindTypedById<UIPanel>(ID("ConsolePanel"));
    ConsoleWriteLine("PlayFabStore sample start");
    m_console->SetVisible(false);

    // main content panel
    m_contentPanel = m_uiManager.FindTypedById<UIPanel>(ID("ContentPanel"));
    m_contentPanel->SetVisible(false);
    
    // menus
    m_itemMenu = m_uiManager.FindTypedById<UIStackPanel>(ID("ItemMenu"));
    m_itemMenuPF = m_uiManager.FindTypedById<UIStackPanel>(ID("ItemMenuPF"));
    m_actionMenu = m_uiManager.FindTypedById<UIStackPanel>(ID("ActionMenu"));

    // labels
    m_gold = m_uiManager.FindTypedById<UIStaticText>(ID("GoldBalance"));
    m_gem = m_uiManager.FindTypedById<UIStaticText>(ID("GemBalance"));
    m_shield = m_uiManager.FindTypedById<UIStaticText>(ID("ShieldBalance"));
    m_bow = m_uiManager.FindTypedById<UIStaticText>(ID("BowBalance"));
    m_sword = m_uiManager.FindTypedById<UIStaticText>(ID("SwordBalance"));
    m_arrow = m_uiManager.FindTypedById<UIStaticText>(ID("ArrowBalance"));
    m_potion = m_uiManager.FindTypedById<UIStaticText>(ID("PotionBalance"));
    m_selectedItemImage = m_uiManager.FindTypedById<UIImage>(ID("SelectedItemImage"));
    m_selectedItemGold = m_uiManager.FindTypedById<UIStaticText>(ID("SelectedItemGold"));

    m_items[0] = m_uiManager.FindTypedById<UIStaticText>(ID("GemBalance"));
    m_items[1] = m_uiManager.FindTypedById<UIStaticText>(ID("ShieldBalance"));
    m_items[2] = m_uiManager.FindTypedById<UIStaticText>(ID("BowBalance"));
    m_items[3] = m_uiManager.FindTypedById<UIStaticText>(ID("SwordBalance"));
    m_items[4] = m_uiManager.FindTypedById<UIStaticText>(ID("ArrowBalance"));
    m_items[5] = m_uiManager.FindTypedById<UIStaticText>(ID("PotionBalance"));

    m_aButtonText = m_uiManager.FindTypedById<UIStaticText>(ID("AButtonText"));
    m_statusText = m_uiManager.FindTypedById<UIStaticText>(ID("StatusText"));

    // Action Menu items
    auto button = CastPtr<UIButton>(m_actionMenu->AddChildFromPrefab("#menu_item_prefab"));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText(" Query PlayFab Catalog");
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetStyleId(ID("MenuItemLeftTextStyle"));
    button->SetRelativeSizeInRefUnits(Vector2(400, button->GetRelativeSizeInRefUnits().y));
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
        {
            m_statusText->SetDisplayText("");
            QueryPlayFabProducts(c_pfCatalogItemIds, c_pfCatalogItemsCount);
        });
    button->ButtonState().AddListenerWhen(highlightState,
        [this](UIButton*)
        {
            m_selectedItemImage->SetStyleId(ID("CatalogStyle"));
            m_selectedItemGold->SetDisplayText("");
            m_aButtonText->SetDisplayText("[A] Execute operation");
        });

    button = CastPtr<UIButton>(m_actionMenu->AddChildFromPrefab("#menu_item_prefab"));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText(" Query PlayFab Inventory");
    button->SetRelativeSizeInRefUnits(Vector2(400, button->GetRelativeSizeInRefUnits().y));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetStyleId(ID("MenuItemLeftTextStyle"));
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
        {
            m_statusText->SetDisplayText("");
            QueryPlayFabInventory();
        });
    button->ButtonState().AddListenerWhen(highlightState,
        [this](UIButton*)
        {
            m_selectedItemImage->SetStyleId(ID("CatalogStyle"));
            m_selectedItemGold->SetDisplayText("");
            m_aButtonText->SetDisplayText("[A] Execute operation");
        });

    button = CastPtr<UIButton>(m_actionMenu->AddChildFromPrefab("#menu_item_prefab"));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText(" Redeem Entitlements");
    button->SetRelativeSizeInRefUnits(Vector2(400, button->GetRelativeSizeInRefUnits().y));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetStyleId(ID("MenuItemLeftTextStyle"));
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
        {
            m_statusText->SetDisplayText("");
            RedeemStoreProducts();
        });
    button->ButtonState().AddListenerWhen(highlightState,
        [this](UIButton*)
        {
            m_selectedItemImage->SetStyleId(ID("CatalogStyle"));
            m_selectedItemGold->SetDisplayText("");
            m_aButtonText->SetDisplayText("[A] Execute operation");
        });

    button = CastPtr<UIButton>(m_actionMenu->AddChildFromPrefab("#menu_item_prefab"));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText(" Reset Inventory");
    button->SetRelativeSizeInRefUnits(Vector2(400, button->GetRelativeSizeInRefUnits().y));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetStyleId(ID("MenuItemLeftTextStyle"));
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
        {
            m_statusText->SetDisplayText("");

            UpdatePlayFabItem(ITEM_GEM, ITEM_TYPE, 0);
            UpdatePlayFabItem(ITEM_ARROW, ITEM_TYPE, 0);
            UpdatePlayFabItem(ITEM_BOW, ITEM_TYPE, 0);
            UpdatePlayFabItem(ITEM_POTION, ITEM_TYPE, 0);
            UpdatePlayFabItem(ITEM_SHIELD, ITEM_TYPE, 0);
            UpdatePlayFabItem(ITEM_SWORD, ITEM_TYPE, 0);
        });
    button->ButtonState().AddListenerWhen(highlightState,
        [this](UIButton*)
        {
            m_selectedItemImage->SetStyleId(ID("CatalogStyle"));
            m_selectedItemGold->SetDisplayText("");
            m_aButtonText->SetDisplayText("[A] Execute operation");
        });

    button = CastPtr<UIButton>(m_actionMenu->AddChildFromPrefab("#menu_item_prefab"));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText(" Reset Gold Balance");
    button->SetRelativeSizeInRefUnits(Vector2(400, button->GetRelativeSizeInRefUnits().y));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetStyleId(ID("MenuItemLeftTextStyle"));
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
        {
            m_statusText->SetDisplayText("");
            UpdatePlayFabItem(WALLET_GOLD, WALLET_TYPE, 0);
        });
    button->ButtonState().AddListenerWhen(highlightState,
        [this](UIButton*)
        {
            m_selectedItemImage->SetStyleId(ID("CatalogStyle"));
            m_selectedItemGold->SetDisplayText("");
            m_aButtonText->SetDisplayText("[A] Execute operation");
        });

    button = m_uiManager.FindTypedById<UIButton>(ID("LTButton"));
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
        {
            m_statusText->SetDisplayText("");
            SubtractPlayFabItem(ITEM_ARROW, 1);
        });

#ifdef _GAMING_DESKTOP
    button->SetFocusable(true);
#endif

    button = m_uiManager.FindTypedById<UIButton>(ID("RTButton"));
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
        {
            m_statusText->SetDisplayText("");
            SubtractPlayFabItem(ITEM_POTION, 1);
        });

#ifdef _GAMING_DESKTOP
    button->SetFocusable(true);
#endif

    button = m_uiManager.FindTypedById<UIButton>(ID("ViewButton"));
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
        {
            if (m_console)
            {
                m_console->SetVisible(m_showConsole = !m_showConsole);
            }
        });

#ifdef _GAMING_DESKTOP
    button->SetFocusable(true);
#endif

    //set initial inventory counts
    for (uint32_t i = 0; i < c_pfInventoryItemsCount; ++i)
    {
        m_items[i]->SetDisplayText("");
    }

    m_itemMenu->SetVisible(true);
    m_itemMenuPF->SetVisible(true);
    m_actionMenu->SetVisible(true);
}
#pragma endregion

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

            // TODO: Avoid throttle by tracking inventory amount changes locally.
            // Space calls to PFInventorySubtract and PFInventoryUpdate to work
            // within throttle limits instead of calling every button press.
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

    // Push logs to UI
    for (size_t i = 0; i < m_logQueue.size(); ++i)
    {
        m_consoleWindow->AppendLineOfText(m_logQueue[i]);
    }
    m_logQueue.clear();

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

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

        // consume items
        if (m_gamePadButtons.leftTrigger == GamePad::ButtonStateTracker::PRESSED)
        {
            m_statusText->SetDisplayText("");
            SubtractPlayFabItem(ITEM_ARROW, 1);
        }
        if (m_gamePadButtons.rightTrigger == GamePad::ButtonStateTracker::PRESSED)
        {
            m_statusText->SetDisplayText("");
            SubtractPlayFabItem(ITEM_POTION, 1);
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

    const auto& gamepadButtons = m_inputState.GetGamePadButtons(0);
    const auto& keys = m_inputState.GetKeyboardKeys();

    if (gamepadButtons.view == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyReleased(Keyboard::Keys::OemTilde))
    {
        if (m_console)
        {
            m_console->SetVisible(m_showConsole = !m_showConsole);
        }
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

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_liveInfoHUD->Render(commandList);

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
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
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
    auto const r = m_deviceResources->GetOutputSize();
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
    width = 1280;
    height = 720;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        size_t(Descriptors::Count),
        size_t(Descriptors::Reserve)
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
    m_liveInfoHUD->SetViewport(m_deviceResources->GetScreenViewport());
    auto const size = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(size.right, size.bottom);
}

void Sample::OnDeviceLost()
{
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
