//--------------------------------------------------------------------------------------
// InGameStore.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "InGameStore.h"
#include "ATGColors.h"
#include <XUser.h>

extern void ExitSample();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

using namespace ATG::UITK;

ATG::FileDownloader Sample::s_fileDownloader;

#pragma region Helpers
namespace
{
    Sample* s_sample;

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

    template <size_t bufferSize = 512>
    void ShowPopup(std::string_view format, ...)
    {
        assert(format.size() < bufferSize && "format string is too large, split up the string or increase the buffer size");

        static char buffer[bufferSize] = "";

        va_list args;
        va_start(args, format);
        vsprintf_s(buffer, format.data(), args);
        va_end(args);

        ConsoleWriteLine(buffer);

        if (s_sample)
        {
            auto popup = s_sample->GetPopup();

            if (popup->IsVisible())
            {
                auto str = popup->GetTypedSubElementById<UIStaticText>(ID("PopupText"))->GetDisplayText();
                str += "\n";
                str += buffer;
                popup->GetTypedSubElementById<UIStaticText>(ID("PopupText"))->SetDisplayText(str);
            }
            else
            {
                popup->GetTypedSubElementById<UIStaticText>(ID("PopupText"))->SetDisplayText(buffer);
            }

            auto button = popup->GetTypedSubElementById<UIButton>(ID("PopupButton"));
            button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
                [popup](UIButton*)
            {
                popup->SetVisible(false);
            });

            popup->SetVisible(true);
        }
    }

    UIProductPrice CopyToUIPrice(XStorePrice price)
    {
        UIProductPrice copy;
        copy.basePrice = price.basePrice;
        copy.price = price.price;
        copy.recurrencePrice = price.recurrencePrice;
        copy.isOnSale = price.isOnSale;
        copy.saleEndDate = price.saleEndDate;

        copy.currencyCode = price.currencyCode? price.currencyCode : "";
        copy.formattedBasePrice = price.formattedBasePrice;
        copy.formattedPrice = price.formattedPrice;
        copy.formattedRecurrencePrice = price.formattedRecurrencePrice;

        return copy;
    }

    UIProductAvailability CopyToUIAvailability(XStoreAvailability availability)
    {
        UIProductAvailability copy;
        copy.availabilityId = availability.availabilityId;
        copy.endDate = availability.endDate;
        copy.price = CopyToUIPrice(availability.price);

        return copy;
    }

    UIProductSku CopyToUISku(XStoreSku sku)
    {
        UIProductSku copy;

        copy.skuId = sku.skuId;
        copy.title = sku.title;

        copy.price = CopyToUIPrice(sku.price);
        copy.isTrial = sku.isTrial;

        for (uint32_t i = 0; i < sku.bundledSkusCount; i++)
        {
            copy.bundledSkus.push_back(sku.bundledSkus[i]);
        }

        std::vector<UIProductAvailability> availabilities;
        for (uint32_t i = 0; i < sku.availabilitiesCount; i++)
        {
            copy.availabilities.push_back(CopyToUIAvailability(sku.availabilities[i]));
        }

        copy.isInUserCollection = sku.isInUserCollection;
        copy.isSubscription = sku.isSubscription;

        copy.quantity = sku.collectionData.quantity;

        return copy;
    }

    UIProductImage CopyToUIImage(XStoreImage image)
    {
        UIProductImage copy;
        copy.height = image.height;
        copy.width = image.width;
        copy.uri = image.uri;
        copy.tag = image.imagePurposeTag;

        return copy;
    }

    UIProductDetails CopyToUIProduct(const XStoreProduct* product)
    {
        UIProductDetails copy;

        copy.storeId = product->storeId;
        copy.title = product->title;
        copy.description = product->description;
        copy.language = product->language;
        copy.productKind = product->productKind;
        copy.price = CopyToUIPrice(product->price);
        copy.hasDigitalDownload = product->hasDigitalDownload;
        copy.isInUserCollection = product->isInUserCollection;

        copy.aggregateQuantity = 0;
        for (uint32_t i = 0; i < product->skusCount; i++)
        {
            // See readme for comments regarding sku quantity and caution around
            // using quantity on the client (i.e. try to use b2b)
            copy.aggregateQuantity += product->skus[i].collectionData.quantity;

            copy.skus.push_back(CopyToUISku(product->skus[i]));
        }

        for (uint32_t i = 0; i < product->imagesCount; i++)
        {
            copy.images.push_back(CopyToUIImage(product->images[i]));
        }

        return copy;
    }
}

#pragma endregion

Sample::Sample() noexcept(false) :
    m_xStoreContext(nullptr),
    m_frame(0),
    m_asyncQueue(nullptr),
    m_closeMenu(false),
    m_showConsole(false)
{
    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_asyncQueue)
    );

    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::SerializedThreadPool, XTaskQueueDispatchMode::SerializedThreadPool, &m_imageQueue)
    );

    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);

    m_liveResources = std::make_shared<ATG::LiveResources>(m_asyncQueue);
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("");

    m_baseGameStoreId = "";
    m_selectedStoreId = "";
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    XStoreCloseContextHandle(m_xStoreContext);

    m_xStoreContext = nullptr;

    if (m_asyncQueue)
    {
        XTaskQueueCloseHandle(m_asyncQueue);
        m_asyncQueue = nullptr;
    }

    if (m_imageQueue)
    {
        XTaskQueueCloseHandle(m_imageQueue);
        m_imageQueue = nullptr;
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    s_sample = this;

    m_gamePad = std::make_unique<GamePad>();
    m_gamePadButtons = std::make_unique<GamePad::ButtonStateTracker>();
    m_keyboard = std::make_unique<Keyboard>();
    m_keyboardButtons = std::make_unique<Keyboard::KeyboardStateTracker>();
    m_mouse = std::make_unique<Mouse>();
    m_mouseButtons = std::make_unique<Mouse::ButtonStateTracker>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
    
    m_liveResources->SetUserChangedCallback([this](XUserHandle user)
    {
        m_liveInfoHUD->SetUser(user, m_asyncQueue);

#ifdef _GAMING_XBOX
        InitStore();
#endif
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

    SetupUI();

    m_liveResources->Initialize();
    m_liveInfoHUD->Initialize();


    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    GetUserDefaultLocaleName(localeName, ARRAYSIZE(localeName));
    auto localeInfo = m_uiManager.FindTypedById<UIStaticText>(ID("LocaleInfo"));
    localeInfo->SetDisplayText(DX::WideToUtf8(localeName));

#ifndef _GAMING_XBOX
    // For desktop, 
    InitStore();
#endif
}

#pragma region In-Game Store
void Sample::InitStore()
{
#ifdef _GAMING_XBOX
    HRESULT hr = XStoreCreateContext(m_liveResources->GetUser(), &m_xStoreContext);
#else
    HRESULT hr = XStoreCreateContext(nullptr, &m_xStoreContext);
#endif
    
    DX::ThrowIfFailed(hr); // Unable to create an XStoreContext

    QueryGameLicense();

    QueryCatalog();
}

void Sample::QueryGameLicense()
{
    ConsoleWriteLine("Calling XStoreQueryGameLicenseAsync");

    auto async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock *async)
    {
        XStoreGameLicense license;
        HRESULT hr = XStoreQueryGameLicenseResult(async, &license);
        if (SUCCEEDED(hr))
        {
            auto pThis = reinterpret_cast<Sample*>(async->context);

            if (!license.isActive)
            {
                auto licenseInfo1 = pThis->m_uiManager.FindTypedById<UIStaticText>(ID("LicenseInfo1"));
                std::string licenseInfo = "No license found";
                licenseInfo1->SetDisplayText(licenseInfo);
                ShowPopup(licenseInfo.c_str());
            }
            else
            {
                auto licenseInfo1 = pThis->m_uiManager.FindTypedById<UIStaticText>(ID("LicenseInfo1"));

                std::string licenseInfo = (std::strlen(license.skuStoreId) == 12) ? "Base game store ID: " : "Active license: ";
                licenseInfo += license.skuStoreId;
                licenseInfo1->SetDisplayText(licenseInfo);

                auto licenseInfo2 = pThis->m_uiManager.FindTypedById<UIStaticText>(ID("LicenseInfo2"));
                if (license.isDiscLicense)
                {
                    licenseInfo = "Running from disc";
                    licenseInfo2->SetDisplayText(licenseInfo);
                }
                else
                if (license.isTrial)
                {
                    // Display additional trial info
                    licenseInfo = "Trial: ";
                    licenseInfo += std::to_string(license.trialTimeRemainingInSeconds) + " seconds remaining | ";
                    if (license.expirationDate > 0)
                    {
                        char buff[32] = {};
                        struct tm timeinfo = {};
                        ::localtime_s(&timeinfo, &license.expirationDate);
                        strftime(buff, 32, "%m/%d/%Y %H:%M:%S", &timeinfo);
                        licenseInfo += "Expiration date: " + std::string(buff) + " | ";
                    }
                    licenseInfo += std::string(license.isTrialOwnedByThisUser ? "Owned by current account | " : "Owned by another account | ");
                    licenseInfo += "Unique ID: " + std::string(license.trialUniqueId);

                    licenseInfo2->SetDisplayText(licenseInfo);

                    // Query current product to add to product list (for upsell)
                    pThis->QueryGameProduct();
                }

                ConsoleWriteLine(licenseInfo.c_str());

                if (pThis->GetPopup()->IsVisible())
                {
                    pThis->GetPopup()->GetTypedSubElementById<UIStaticText>(ID("PopupText"))->SetDisplayText(licenseInfo);
                }

                pThis->SetBaseGameStoreId(license.skuStoreId);
            }
        }
        else
        {
            ShowPopup("Error calling XStoreQueryGameLicenseResult : 0x%08X\n", hr);
        }

        delete async;
    };

    HRESULT hr = XStoreQueryGameLicenseAsync(m_xStoreContext, async);

    if (FAILED(hr))
    {
        delete async;
        ShowPopup("Error calling XStoreQueryGameLicenseAsync : 0x%08X\n", hr);
    }
}

void Sample::QueryAddonLicenses()
{
    // This will only return durable (without package) licenses associated
    // with the game license at the time of query.
    // Game licenses will not be updated with newly-purchased durables (until
    // the game license is refreshed upon next game launch), therefore do not
    // rely on this as a means of refreshing the ownership state of durables;
    // use XStore(Can)AcquireLicenseForDurables/XStoreAcquireLicenseForStoreId

    ConsoleWriteLine("Calling XStoreQueryAddOnLicensesAsync");

    auto async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock *async)
    {
        uint32_t count;
        HRESULT hr = XStoreQueryAddOnLicensesResultCount(
            async,
            &count);

        if (FAILED(hr))
        {
            delete async;
            ShowPopup("Error calling XStoreQueryAddOnLicensesResultCount: 0x%x", hr);
            return;
        }

        ShowPopup("Number of add-on licenses: %u", count);
        
        XStoreAddonLicense* licenses = new XStoreAddonLicense[count];
        hr = XStoreQueryAddOnLicensesResult(async, count, licenses);

        if (FAILED(hr))
        {
            ShowPopup("Error calling XStoreQueryAddOnLicensesResult: 0x%x", hr);
            delete[] licenses;
            return;
        }

        for (uint32_t i = 0; i < count; ++i)
        {
            auto addOnLicense = licenses[i];

            std::string info = "AddOnLicense " + std::string(addOnLicense.skuStoreId);
            info += " active: " + std::string(addOnLicense.isActive ? "true" : "false");

            if (addOnLicense.expirationDate > 0)
            {
                char buff[32] = {};
                struct tm timeinfo = {};
                ::localtime_s(&timeinfo, &addOnLicense.expirationDate);
                strftime(buff, 32, "%m/%d/%Y %H:%M:%S", &timeinfo);
                info += " expiry: " + std::string(buff);
            }

            ConsoleWriteLine(info.c_str());
        }

        delete[] licenses;
    };

    HRESULT hr = XStoreQueryAddOnLicensesAsync(m_xStoreContext, async);

    if (FAILED(hr))
    {
        delete async;
        ShowPopup("Error calling XStoreQueryAddOnLicensesAsync : 0x%08X\n", hr);
    }
}

void Sample::QueryLicenseToken()
{
    ConsoleWriteLine("Calling XStoreQueryLicenseTokenAsync");

    auto async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock *async)
    {
        size_t size;
        HRESULT hr = XStoreQueryLicenseTokenResultSize(
            async,
            &size);

        if (FAILED(hr))
        {
            delete async;
            ShowPopup("Error calling XStoreQueryLicenseTokenResultSize: 0x%x", hr);
            return;
        }

        std::vector<char> result(size);
        hr = XStoreQueryLicenseTokenResult(
            async,
            size,
            result.data());

        if (FAILED(hr))
        {
            delete async;
            ShowPopup("Error calling XStoreQueryLicenseTokenResult: 0x%08X", static_cast<uint32_t>(hr));
            return;
        }

        ShowPopup("License Token size %u", result.size());

        // Not using ConsoleWriteLine here in case token size exceeds buffer size
        OutputDebugStringA(result.data());
        OutputDebugStringA("\n");

        delete async;
    };

    // Pass in store IDs of items to check for licenses
    const char* ids[] =
    {
        m_baseGameStoreId.c_str() // most often you are concerned with the base game
    };

    HRESULT hr = XStoreQueryLicenseTokenAsync(
        m_xStoreContext,
        ids,
        ARRAYSIZE(ids),
        "customdeveloperstring-atgingamestore",
        async);

    if (FAILED(hr))
    {
        delete async;
        ShowPopup("Error calling XStoreQueryLicenseTokenAsync : 0x%08X\n", hr);
    }
}


struct QueryContext
{
    Sample* pThis;
    bool isCatalogQuery;                 // true if query is for AssociatedProducts (i.e. catalog)
    uint32_t count;                      // used to accumulate count of products returned across pages
    XStoreProductQueryHandle handle;     // handle to reuse for paged operations
};

bool CALLBACK ProductEnumerationCallback(const XStoreProduct* product, void* context)
{
    auto&[pThis, isCatalog, count, queryHandle] = *reinterpret_cast<QueryContext*>(context);

    UIProductDetails productCopy = CopyToUIProduct(product);

    std::string quantity = {};
    if (productCopy.isInUserCollection &&
        (productCopy.productKind == XStoreProductKind::Consumable ||
         productCopy.productKind == XStoreProductKind::UnmanagedConsumable))
    {
        quantity += "Quantity: " + std::to_string(productCopy.aggregateQuantity);
    }

    ConsoleWriteLine("%10s: (%s) [%s] %s %s %s",
        ProductKindToString(productCopy.productKind).c_str(),
        productCopy.storeId.c_str(),
        productCopy.language.c_str(),
        productCopy.title.c_str(),
        productCopy.isInUserCollection ? "[OWNED]" : "",
        quantity.c_str());

    pThis->AddOrUpdateProductToCatalog(productCopy.productKind, productCopy);

    pThis->DownloadProductImage(productCopy, "Logo", nullptr);
    
    count++;

    return true;
}

void Sample::QueryGameProduct()
{
    auto async = new XAsyncBlock{};
    async->context = new QueryContext{ this, false, 0, nullptr }; // expected by ProductEnumerationCallback
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock* async)
    {
        auto&[pThis, isCatalog, count, queryHandle] = *reinterpret_cast<QueryContext*>(async->context);

        if (SUCCEEDED(XStoreQueryProductForCurrentGameResult(async, &queryHandle)))
        {
            auto hr = XStoreEnumerateProductsQuery(queryHandle, async->context, ProductEnumerationCallback);
            if (FAILED(hr))
            {
                ShowPopup("Error calling XStoreEnumerateProductsQuery : 0x%08X", hr);
            }
            else
            {
                ShowPopup("%u game product found", count);
            }
        }

        delete async;
    };

    HRESULT hr = XStoreQueryProductForCurrentGameAsync(m_xStoreContext, async);

    if (FAILED(hr))
    {
        delete async;
        ShowPopup("Error calling XStoreQueryProductForCurrentGameAsync : 0x%08X", hr);
    }
}

void CALLBACK QueryProductsCallback(XAsyncBlock* async)
{
    auto&[pThis, isCatalogQuery, count, queryHandle] = *reinterpret_cast<QueryContext*>(async->context);

    HRESULT hr = XStoreQueryProductsResult(async, &queryHandle);
    if (SUCCEEDED(hr))
    {
        hr = XStoreEnumerateProductsQuery(queryHandle, async->context, ProductEnumerationCallback);

        if (SUCCEEDED(hr))
        {
            if (XStoreProductsQueryHasMorePages(queryHandle))
            {
                ConsoleWriteLine("Has more pages!");
                pThis->QueryNextPage(async);
            }
            else
            {
                ShowPopup(isCatalogQuery ? "%u catalog products found" : "You own %u products", count);

                pThis->UpdateProductList();

                XStoreCloseProductsQueryHandle(queryHandle);
                delete async;
            }
        }
        else
        {
            delete async;
            ShowPopup("Error calling XStoreEnumerateProductsQuery : 0x%08X", hr);
        }
    }
}

void Sample::QueryCatalog()
{
    // Returns products associated to the title available for purchase

    ConsoleWriteLine("Calling XStoreQueryAssociatedProductsAsync");

    auto async = new XAsyncBlock{};
    async->context = new QueryContext{ this, true, 0, nullptr };
    async->queue = m_asyncQueue;
    async->callback = QueryProductsCallback;

    XStoreProductKind typeFilter =
        XStoreProductKind::Consumable |
        XStoreProductKind::Durable |
        XStoreProductKind::Game |
        XStoreProductKind::UnmanagedConsumable |
        XStoreProductKind::Pass;

    HRESULT hr = XStoreQueryAssociatedProductsAsync(
        m_xStoreContext,
        typeFilter,     // Product filter types
        25,             // Products per page (25 is good default)
        async);

    //// Example of how to use XStoreQueryProducts to query specific products
    //// Replace above with below

    //const char* storeIds[] = { "9MT5TGW893HV", "9NQWJKKNHF1L" };
    //HRESULT hr = XStoreQueryProductsAsync(
    //    m_xStoreContext,
    //    typeFilter,     // Product filter types
    //    storeIds,
    //    ARRAYSIZE(storeIds),
    //    nullptr,
    //    0,
    //    async);

    if (FAILED(hr))
    {
        delete async;
        ShowPopup("Error calling XStoreQueryAssociatedProductsAsync : 0x%08X", hr);
    }
}

void Sample::QueryCollections()
{
    // Returns products the store user owns

    ConsoleWriteLine("Calling XStoreQueryEntitledProductsAsync");

    auto async = new XAsyncBlock{};
    async->context = new QueryContext{ this, false, 0, nullptr };
    async->queue = m_asyncQueue;
    async->callback = QueryProductsCallback;

    XStoreProductKind typeFilter =
        XStoreProductKind::Consumable |
        XStoreProductKind::Durable |
        XStoreProductKind::Game |
        XStoreProductKind::UnmanagedConsumable |
        XStoreProductKind::Pass;

    HRESULT hr = XStoreQueryEntitledProductsAsync(
        m_xStoreContext,
        typeFilter,     // Product filter types
        25,             // Products per page (25 is good default)
        async);

    if (FAILED(hr))
    {
        delete async;
        ShowPopup("Error calling XStoreQueryEntitledProductsAsync : 0x%08X", hr);
    }
}

void Sample::QueryNextPage(XAsyncBlock *async)
{
    async->callback = [](XAsyncBlock* async)
    {
        auto&[pThis, isCatalogQuery, count, queryHandle] = *reinterpret_cast<QueryContext*>(async->context);

        if (SUCCEEDED(XStoreProductsQueryNextPageResult(async, &queryHandle)))
        {
            ConsoleWriteLine("Enumerating Page...");
            HRESULT hr = XStoreEnumerateProductsQuery(queryHandle, async->context, ProductEnumerationCallback);

            if (SUCCEEDED(hr))
            {
                // IMPORTANT! Always check if there are more pages to enumerate
                // Results may return in a single page in development environment
                // but may be split up in retail environment
                if (XStoreProductsQueryHasMorePages(queryHandle))
                {
                    ConsoleWriteLine("Has more pages!");
                    pThis->QueryNextPage(async);
                }
                else
                {
                    ShowPopup(isCatalogQuery? "%u catalog products found" : "You own %u products", count);

                    pThis->UpdateProductList();

                    XStoreCloseProductsQueryHandle(queryHandle);
                    delete async;
                }
            }
            else
            {
                delete async;
                ShowPopup("Error calling XStoreEnumerateProductsQuery : 0x%08X", hr);
            }
        }
    };

    auto&[pThis, isCatalog, count, queryHandle] = *reinterpret_cast<QueryContext*>(async->context);

    HRESULT result = XStoreProductsQueryNextPageAsync(queryHandle, async);
    if (FAILED(result))
    {
        delete async;
        ShowPopup("Error calling XStoreProductsQueryNextPageAsync : 0x%08X", result);
    }
}

void Sample::Download(const char* storeId)
{
    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->context = this;
    async->callback = [](XAsyncBlock* async)
    {
        uint32_t count = 0;
        HRESULT hr = XStoreDownloadAndInstallPackagesResultCount(async, &count);

        if (FAILED(hr))
        {
            delete async;
            ShowPopup("Error calling XStoreDownloadAndInstallPackagesResultCount : 0x%08X\n", hr);
        }
        else
        {
            std::vector<char[XPACKAGE_IDENTIFIER_MAX_LENGTH]> packageIds(count);

            hr = XStoreDownloadAndInstallPackagesResult(async, count, packageIds.data());

            if (FAILED(hr))
            {
                delete async;
                ShowPopup("Error calling XStoreDownloadAndInstallPackagesResult : 0x%08X\n", hr);
            }
            else
            {
                ShowPopup("Download queued (count %u)", count);
            }
        }
    };

    const char* storeIds[] =
    {
        storeId
    };

    HRESULT hr = XStoreDownloadAndInstallPackagesAsync(m_xStoreContext, storeIds, ARRAYSIZE(storeIds), async);
    if (FAILED(hr))
    {
        delete async;
        ShowPopup("Error calling XStoreDownloadAndInstallPackagesAsync : 0x%08X\n", hr);
    }
}

void Sample::PreviewLicense(const char* storeId)
{
    // Answers, do I have a license for this product?

    ConsoleWriteLine("Calling XStoreCanAcquireLicenseForStoreIdAsync for %s", storeId);

    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock* async)
    {
        XStoreCanAcquireLicenseResult result;

        // for unapplicable product types, result.status can be 2
        HRESULT hr = XStoreCanAcquireLicenseForStoreIdResult(
            async,
            &result);

        if (FAILED(hr))
        {
            ShowPopup("Error calling XStoreCanAcquireLicenseForStoreIdResult: 0x%x", hr);
        }
        else
        {
            ShowPopup("Status: %u LicensableSku: %s", result.status, result.licensableSku);
        }

        delete async;
    };

    HRESULT hr = XStoreCanAcquireLicenseForStoreIdAsync(
        m_xStoreContext,
        storeId,
        async);

    if (FAILED(hr))
    {
        delete async;

        ShowPopup("Error calling XStoreCanAcquireLicenseForStoreIdAsync: 0x%x", hr);
        return;
    }
}


void Sample::AcquireLicense(const char* storeId)
{
    // This switches on hasDigitalDownload which is populated if Durable has a package
    // Depending on this, different API must be used to acquire license

    auto &product = m_catalogDetails[storeId];

    auto isDlc = product.hasDigitalDownload;

    const char* apiName = isDlc ? "XStoreAcquireLicenseForPackageAsync" : "XStoreAcquireLicenseForDurablesAsync";

    ConsoleWriteLine("Calling %s for %s", apiName, storeId);

    struct Ctx
    {
        bool isDlc;
    };

    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock* async)
    {
        XStoreLicenseHandle handle = {};

        auto&[isDlc] = *reinterpret_cast<Ctx*>(async->context);

        HRESULT hr = (isDlc) ?
            XStoreAcquireLicenseForPackageResult(async, &handle) :
            XStoreAcquireLicenseForDurablesResult(async, &handle);

        if (FAILED(hr))
        {
            const char* apiName = isDlc ? "XStoreAcquireLicenseForPackageResult" : "XStoreAcquireLicenseForDurablesResult";

            ConsoleWriteLine("Error calling %s: 0x%x", apiName, hr);

            if (static_cast<uint32_t>(hr) == 0x87e10bc6 /*LM_E_CONTENT_NOT_IN_CATALOG*/)
            {
                ShowPopup("This API only works with durables without packages");
            }
            if (static_cast<uint32_t>(hr) == 0x803f9006 /*LM_E_ENTITLED_USER_SIGNED_OUT*/)
            {
                ShowPopup("User that owns this DLC is signed out");
            }
        }
        else
        {
            bool isValid = XStoreIsLicenseValid(handle);

            ShowPopup("Is valid license: %s", isValid ? "yes" : "no");

            if (isValid)
            {
                XTaskQueueRegistrationToken token = {};
                hr = XStoreRegisterPackageLicenseLost(handle, async->queue, async->context,
                    [](void *)
                    {
                        ConsoleWriteLine("License lost event received: %s");
                    },
                    &token);

                if (FAILED(hr))
                {
                    ConsoleWriteLine("XStoreRegisterPackageLicenseLost failed : 0x%08x", hr);
                }
            }
        }

        delete reinterpret_cast<Ctx*>(async->context);
        delete async;
    };

    char packageId[XPACKAGE_IDENTIFIER_MAX_LENGTH] = {};

    if (isDlc)
    {
        XStoreQueryPackageIdentifier(storeId, XPACKAGE_IDENTIFIER_MAX_LENGTH, packageId);
    }

    async->context = new Ctx{ isDlc };

    HRESULT hr = isDlc ?
        XStoreAcquireLicenseForPackageAsync(m_xStoreContext, packageId, async) :
        XStoreAcquireLicenseForDurablesAsync(m_xStoreContext, storeId, async);

    if (FAILED(hr))
    {
        if (hr == E_GAMEPACKAGE_NO_PACKAGE_IDENTIFIER)
        {
            ShowPopup("Error calling %s: 0x%x (Check if DLC is installed)", apiName, hr);
        }
        else
        {
            ShowPopup("Error calling %s: 0x%x", apiName, hr);
        }
        delete async;

        return;
    }
}

void Sample::ShowAssociatedProducts()
{
    // This opens up the Store app showing associated products filtered to the desired product types
    auto currentProductType = m_uiManager.FindTypedById<UITwistMenu>(ID("ProductTypeFilter"))->GetCurrentDisplayString();

    auto types = c_assocTypeMap.at("ALL");
    auto it = c_assocTypeMap.find(currentProductType);
    if (it != c_assocTypeMap.end())
    {
        types = it->second;
    }
    else
    {
        currentProductType = "ALL";
    }

    ConsoleWriteLine("Calling XStoreShowAssociatedProductsUIAsync for %s", currentProductType.c_str());

    auto async = new XAsyncBlock{};
    async->callback = [](XAsyncBlock* async)
    {
        HRESULT hr = XStoreShowAssociatedProductsUIResult(async);

        if (FAILED(hr))
        {
            ShowPopup("Error calling XStoreShowAssociatedProductsUIResult: 0x%08x", hr);
        }

        delete async;
    };

    auto hr = XStoreShowAssociatedProductsUIAsync(m_xStoreContext, m_baseGameStoreId.c_str(), types, async);

    if (FAILED(hr))
    {
        ShowPopup("Error calling XStoreShowAssociatedProductsUIAsync: 0x%08x", hr);
        delete async;
    }
}

void Sample::ShowProductPage(const char* storeId)
{
    // This opens up the Store app showing the PDP of the selected product
    ConsoleWriteLine("Calling XStoreShowProductPageUIAsync for %s", storeId);

    auto async = new XAsyncBlock{};
    async->callback = [](XAsyncBlock *async)
    {
        HRESULT hr = XStoreShowProductPageUIResult(async);

        if (FAILED(hr))
        {
            ShowPopup("Error calling XStoreShowProductPageUIResult: 0x%08x", hr);
        }

        delete async;
    };

    auto hr = XStoreShowProductPageUIAsync(m_xStoreContext, storeId, async);

    if (FAILED(hr))
    {
        ShowPopup("Error calling XStoreShowProductPageUIResult: 0x%08x", hr);
        delete async;
    }
}

void Sample::MakePurchase(const char* storeId)
{
    struct PurchaseContext
    {
        Sample* pThis;
        std::string storeId;
        std::string itemName;
    };

    auto async = new XAsyncBlock{};
    async->context = new PurchaseContext{ this, storeId, m_catalogDetails.at(storeId).title };
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock *async)
    {
        auto &[pThis, storeId, itemName] = *reinterpret_cast<PurchaseContext*>(async->context);

        HRESULT hr = XStoreShowPurchaseUIResult(async);
        if (SUCCEEDED(hr))
        {
            ConsoleWriteLine("Purchase succeeded (%s)", storeId.c_str());
            ShowPopup("Thank you for purchasing %s", itemName.c_str());
            pThis->QueryCollections();
        }
        else
        {
            if (hr == E_GAMESTORE_ALREADY_PURCHASED)
            {
                ShowPopup("You already own %s", itemName.c_str());
            }
            else if (hr == E_ABORT)
            {
                ConsoleWriteLine("Purchase cancelled");
            }
            else
            {
                ShowPopup("Purchase failed (%s) 0x%x", storeId.c_str(), hr);
            }
        }

        delete reinterpret_cast<PurchaseContext*>(async->context);
        delete async;
    };

    HRESULT hr = XStoreShowPurchaseUIAsync(
        m_xStoreContext,
        storeId,
        nullptr,    // Can be used to override the title bar text
        nullptr,    // Can be used to provide extra details to purchase
        async);

    if (FAILED(hr))
    {
        delete async;
        ShowPopup("Error calling XStoreShowPurchaseUIAsync: 0x%x", hr);
    }
}

void Sample::RateMyGame()
{
    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock* async)
    {
        XStoreRateAndReviewResult result{};
        HRESULT hr = XStoreShowRateAndReviewUIResult(async, &result);

        if (FAILED(hr))
        {
            if (hr == E_ABORT)
            {
                ConsoleWriteLine("Review cancelled");
            }
            else
            {
                ShowPopup("Error calling XStoreShowRateAndReviewUIResult: 0x%x", hr);
            }
        }
        else
        {
            ShowPopup("Review %s", result.wasUpdated ? "updated." : "posted.");
        }

        delete async;
    };

    HRESULT hr = XStoreShowRateAndReviewUIAsync(m_xStoreContext, async);

    if (FAILED(hr))
    {
        delete async;
        ShowPopup("Error calling XStoreShowRateAndReviewUIAsync: 0x%x", hr);
    }
}

void Sample::RedeemToken()
{
    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->context = this;
    async->callback = [](XAsyncBlock* async)
    {
        HRESULT hr = XStoreShowRedeemTokenUIResult(async);

        if (FAILED(hr))
        {
            if (hr == E_ABORT)
            {
                ConsoleWriteLine("Token entry cancelled");
            }
            else
            {
                ShowPopup("Error calling XStoreShowRedeemTokenUIResult: 0x%x", hr);
            }
        }
        else
        {
            auto pThis = reinterpret_cast<Sample*>(async->context);

            pThis->QueryCollections();
        }
    };
    
    HRESULT hr = XStoreShowRedeemTokenUIAsync(
        m_xStoreContext,
        " ",            // Using " " brings up the UI keyboard for code entry
        nullptr,        // Restrict to specific Store IDs
        0,
        false,          // disallow CSV (cash gift cards)
        async);

    if (FAILED(hr))
    {
        delete async;
        ShowPopup("Error calling XStoreShowRedeemTokenUIAsync: 0x%x", hr);
    }

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

    float elapsedTime = static_cast<float>(timer.GetElapsedSeconds());

    const bool wasMenuVisible = m_itemMenu->GetVisible();
    auto const menuY = uint32_t(m_itemMenu->GetRelativePositionInRefUnits().y);

    // update our UI input state and managed layout
    m_uiInputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_uiInputState);

    const auto& gamepadButtons = m_uiInputState.GetGamePadButtons(0);
    const auto& keys = m_uiInputState.GetKeyboardKeys();
    const auto& mouse = m_uiInputState.GetMouseState();
    const auto& mouseButtons = m_uiInputState.GetMouseButtons();

    if (gamepadButtons.view == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyReleased(Keyboard::Keys::OemTilde))
    {
        m_showConsole = !m_showConsole;
        if (m_console)
        {
            m_console->SetVisible(m_showConsole);
        }
    }

    if ((gamepadButtons.leftStick == GamePad::ButtonStateTracker::PRESSED &&
        gamepadButtons.rightStick == GamePad::ButtonStateTracker::PRESSED) ||
        keys.IsKeyPressed(DirectX::Keyboard::Keys::Escape))
    {
        ExitSample();
    }

    if (gamepadButtons.leftShoulder == GamePad::ButtonStateTracker::PRESSED)
    {
        auto productTypeFilter = m_uiManager.FindTypedById<UITwistMenu>(ID("ProductTypeFilter"));
        productTypeFilter->DecrementSelectedItem();
        UpdateProductList();
    }

    if (gamepadButtons.rightShoulder == GamePad::ButtonStateTracker::PRESSED)
    {
        auto productTypeFilter = m_uiManager.FindTypedById<UITwistMenu>(ID("ProductTypeFilter"));
        productTypeFilter->IncrementSelectedItem();
        UpdateProductList();
    }

    // Menu operations

    if (gamepadButtons.menu == GamePad::ButtonStateTracker::PRESSED)
    {
        if (!m_liveResources->IsUserSignedIn())
        {
            m_liveResources->SignInSilently();
        }
        else
        {
            if (!m_itemMenu->IsVisible())
            {
                ShowGlobalMenu();
            }
            else
            {
                CloseMenu();
            }
        }
    }

    if (gamepadButtons.b && m_itemMenu->IsVisible())
    {
        CloseMenu();
    }

    if (!GetPopup()->IsVisible() &&
        ((!wasMenuVisible && m_itemMenu->IsVisible()) ||
         (wasMenuVisible && menuY != uint32_t(m_itemMenu->GetRelativePositionInRefUnits().y))))
    {
        // new menu just appeared, set first item to focus (unless popup is up)
        m_uiManager.SetFocus(m_itemMenu->GetChildByIndex(0));
    }
    else if (mouseButtons.leftButton == Mouse::ButtonStateTracker::PRESSED)
    {
        // if clicking anywhere other than the currently open item menu, close it
        if (!m_itemMenu->HitTestPixels(mouse.x, mouse.y))
        {
            CloseMenu();
        }
    }

    if (m_itemMenu->IsVisible())
    {
        bool isAnyFocused = false;
        for (size_t i = 0; i < m_itemMenu->GetChildCount(); ++i)
        {
            if (m_itemMenu->GetChildByIndex(i)->IsFocused())
            {
                isAnyFocused = true;
                break;
            }
        }

        if (!isAnyFocused)
        {
            CloseMenu();
        }
    }

    if (m_closeMenu)
    {
        CloseMenu(true);
    }

    // Popup
    static bool popupShowing = false;
    if (!popupShowing && GetPopup()->IsVisible())
    {
        m_uiManager.SetFocus(GetPopup()->GetTypedSubElementById<UIButton>(ID("PopupButton")));
        popupShowing = true;
    }
    if (popupShowing && GetPopup()->IsVisible() == false)
    {
        popupShowing = false;
    }

    while (XTaskQueueDispatch(m_asyncQueue, XTaskQueuePort::Completion, 0))
    {
    }

    m_liveInfoHUD->Update(m_deviceResources->GetCommandQueue());

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

    // ---Live Info HUD Start---

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_liveInfoHUD->Render(commandList);
    // ---Live Info HUD End---

    // UITK
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

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

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
void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();

    m_liveResources->Refresh();
}

void Sample::OnConstrained()
{
    ConsoleWriteLine("Constrained");
}

void Sample::OnUnconstrained()
{
    ConsoleWriteLine("Unconstrained");
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
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        Descriptors::Count,
        Descriptors::Reserve
        );

    // ---Live Info HUD Start---
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    // ---Live Info HUD End---


    // create the style renderer for the UI manager to use for rendering the UI scene styles
    // 200 = bump up number of descriptor piles to accomodate many simultaneously displayed textures
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this, 200);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));

    m_showConsole = false;

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto vp = m_deviceResources->GetScreenViewport();
    m_liveInfoHUD->SetViewport(vp);
    
    // Notify the UI manager of the current window size
    auto const os = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(os.right, os.bottom);
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


#pragma region UI Methods
void Sample::SetupUI()
{
    // UITK load layout
    m_uiManager.GetRootElement()->AddChildFromLayout("Assets/UI/layout.json");

    auto productTypeFilter = m_uiManager.FindTypedById<UITwistMenu>(ID("ProductTypeFilter"));

    productTypeFilter->GetTypedSubElementById<UIButton>(ID("TwistLeft"))->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
    {
        UpdateProductList();
    });

    productTypeFilter->GetTypedSubElementById<UIButton>(ID("TwistRight"))->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
    {
        UpdateProductList();
    });
       
    m_console = m_uiManager.FindTypedById<UIPanel>(ID("ConsolePanel"));
    m_itemMenu = m_uiManager.FindTypedById<UIStackPanel>(ID("ItemMenu"));

    auto menuButton = m_uiManager.FindTypedById<UIButton>(ID("MenuButton"));
    menuButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
    {
        ShowGlobalMenu();
    });
}

void Sample::AddOrUpdateProductToCatalog(XStoreProductKind kind, UIProductDetails& product)
{
    auto& storeId = product.storeId;

    // Insert or update product
    m_catalog[kind].insert(storeId);
    m_catalogDetails.insert_or_assign(storeId, product);
}

void Sample::DownloadProductImage(const UIProductDetails& product, const char* tag, std::shared_ptr<UIImage> imageElement)
{
    // This downloads the image with the provided tag
    // If optional imageElement is passed in, apply image data when downloaded
    for (auto& image : product.images)
    {
        if (image.tag == tag)
        {
            auto async = new XAsyncBlock{};
            async->queue = m_asyncQueue;
            async->context = imageElement.get();
            async->callback = [](XAsyncBlock* async)
            {
                UIImage* imageElement = static_cast<UIImage*>(async->context);

                FileHandle file;
                HRESULT hr = Sample::GetFileDownloader().DownloadFileAsyncResult(async, &file);

                if (FAILED(hr))
                {
                    ConsoleWriteLine("Error calling DownloadFileAsyncResult: 0x%x", hr);
                }
                else
                {
                    if (imageElement)
                    {
                        imageElement->UseTextureData(file->data(), file->size());
                    }
                }

                delete async;
            };

            // image uris just start with //
            std::string uri = "https:" + image.uri;

            GetFileDownloader().DownloadFileAsync(uri, product.storeId + tag, async);
            break;
        }
    }
}

void Sample::UpdateProductList()
{
    ConsoleWriteLine("UpdateProductList");

    std::lock_guard<std::mutex> lock(m_catalogLock);

    auto itemList = m_uiManager.FindTypedById<UIStackPanel>(ID("ItemList"));
    itemList->Reset();

    m_uiManager.FindTypedById<UIStaticText>(ID("StoreIdText"))->SetDisplayText("");
    m_uiManager.FindTypedById<UIStaticText>(ID("PriceText"))->SetDisplayText("");
    m_uiManager.FindTypedById<UIStaticText>(ID("DescriptionText"))->SetDisplayText("");
    m_uiManager.FindTypedById<UIStaticText>(ID("ItemInfoText"))->SetDisplayText("");

    auto productTypeFilter = m_uiManager.FindTypedById<UITwistMenu>(ID("ProductTypeFilter"));

    for (auto &pair : m_catalogDetails)
    {
        auto item = pair.second;

        bool addToList = false;

        switch (productTypeFilter->GetCurrentItemIndex())
        {
        default:
        case 0: // All
            addToList = true;
            break;
        case 1: // Durables
            addToList = item.productKind == XStoreProductKind::Durable && item.hasDigitalDownload == false;
            break;
        case 2: // DLC
            addToList = item.productKind == XStoreProductKind::Durable && item.hasDigitalDownload;
            break;
        case 3: // Consumables
            addToList = item.productKind == XStoreProductKind::Consumable ||
                item.productKind == XStoreProductKind::UnmanagedConsumable;
            break;
        case 4: // Bundles
            for (auto& sku : item.skus)
            {
                if (sku.bundledSkus.size() > 0)
                {
                    addToList = true;
                    break;
                }
            }
            break;
        case 5: // Other
            addToList = item.productKind != XStoreProductKind::Durable &&
                item.productKind != XStoreProductKind::Consumable &&
                item.productKind != XStoreProductKind::UnmanagedConsumable;
            break;
        }

        std::string itemInfo = "";

        if ((item.productKind == XStoreProductKind::Consumable ||
            item.productKind == XStoreProductKind::UnmanagedConsumable) &&
            item.isInUserCollection)
        {
            itemInfo += "Quantity: " + std::to_string(item.aggregateQuantity);
        }
        for (auto& sku : item.skus)
        {
            if (sku.bundledSkus.size() > 0)
            {
                itemInfo += "Included items:\n";
                for (auto& bundledSku : sku.bundledSkus)
                {
                    itemInfo += bundledSku.c_str();
                    auto it = m_catalogDetails.find(bundledSku);
                    itemInfo += it != m_catalogDetails.end() ? ": " + it->second.title : "";
                    itemInfo += "\n";
                }
            }
        }

        if (addToList)
        {
            auto itemButton = CastPtr<UIButton>(itemList->AddChildFromPrefab("#item_prefab"));
            CastPtr<UIStaticText>(itemButton->GetSubElementById(ID("ItemName"), true))->SetDisplayText(item.title);

            auto itemImage = itemButton->GetTypedSubElementById<UIImage>(ID("ItemImage"));

            auto key = item.storeId + "Logo";
            auto image = Sample::GetFileDownloader().GetFile(key);
            if (image == nullptr)
            {
                DownloadProductImage(item, "Logo", itemImage);
            }
            else
            {
                itemImage->UseTextureData(image->data(), image->size());
            }

            auto OnItem = [this, item, itemInfo](UIButton*)
            {
                m_uiManager.FindTypedById<UIStaticText>(ID("StoreIdText"))->SetDisplayText(item.storeId);
                m_uiManager.FindTypedById<UIStaticText>(ID("PriceText"))->SetDisplayText(item.price.formattedPrice);
                m_uiManager.FindTypedById<UIStaticText>(ID("DescriptionText"))->SetDisplayText(item.description);
                m_uiManager.FindTypedById<UIStaticText>(ID("ItemInfoText"))->SetDisplayText(itemInfo);
                m_selectedStoreId = item.storeId;

                auto poster = m_uiManager.FindTypedById<UIImage>(ID("Poster"));

                auto key = item.storeId + "Poster";
                auto image = Sample::GetFileDownloader().GetFile(key);
                if (image == nullptr)
                {
                    DownloadProductImage(item, "Poster", poster);
                }
                else
                {
                    poster->UseTextureData(image->data(), image->size());
                }
            };

            itemButton->ButtonState().AddListenerWhen(UIButton::State::Hovered, OnItem);

            itemButton->ButtonState().AddListenerWhen(UIButton::State::Focused, OnItem);


            if (item.isInUserCollection)
            {
                CastPtr<UIImage>(itemButton->GetSubElementById(ID("OwnedIcon"), true))->SetVisible(true);
            }

            itemButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
                [this, item](UIButton* button)
            {
                auto y = button->GetScreenRectInRefUnits().y;
                ShowItemMenu(item, y);
            });
        }
    }

    auto numProducts = itemList->GetChildCount();
    std::string numResultsText = std::to_string(numProducts) + " product" + ((numProducts > 1) ? "s" : "");
    m_uiManager.FindTypedById<UIStaticText>(ID("NumResultsText"))->SetDisplayText(numProducts? numResultsText : "");
}

void Sample::ShowItemMenu(UIProductDetails item, long y)
{
    m_itemMenu->Reset();

    auto menuX = 700.f;
    auto menuY = float(y);
    m_itemMenu->SetRelativePositionInRefUnits(Vector2(menuX, menuY));

    auto storeId = item.storeId;

    if (!item.isInUserCollection ||
        item.productKind == XStoreProductKind::Consumable || item.productKind == XStoreProductKind::UnmanagedConsumable)
    {
        // Add purchase button for unpurchased items and consumables
        auto button = CastPtr<UIButton>(m_itemMenu->AddChildFromPrefab("#menu_item_prefab"));
        button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText("Purchase");
        button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this, storeId](UIButton*)
        {
            MakePurchase(storeId.c_str());
            CloseMenu();
        });
    }

    if (item.productKind == XStoreProductKind::Durable)
    {
        if (item.hasDigitalDownload)
        {
            // Add download button
            auto button = CastPtr<UIButton>(m_itemMenu->AddChildFromPrefab("#menu_item_prefab"));
            button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText("Download and install");
            button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
                [this, storeId](UIButton*)
            {
                Download(storeId.c_str());
                CloseMenu();
            });
        }

        // Add preview license button
        auto button = CastPtr<UIButton>(m_itemMenu->AddChildFromPrefab("#menu_item_prefab"));
        button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText("Preview license");
        button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this, storeId](UIButton*)
        {
            PreviewLicense(storeId.c_str());
            CloseMenu();
        });

        // Add acquire license button
        button = CastPtr<UIButton>(m_itemMenu->AddChildFromPrefab("#menu_item_prefab"));
        button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText("Acquire license");
        button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this, storeId](UIButton*)
        {
            AcquireLicense(storeId.c_str());
            CloseMenu();
        });
    }

    {
        // Add show PDP button
        auto button = CastPtr<UIButton>(m_itemMenu->AddChildFromPrefab("#menu_item_prefab"));
        button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText("Show Product Page");
        button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this, storeId](UIButton*)
        {
            ShowProductPage(storeId.c_str());
            CloseMenu();
        });
    }

    for (size_t i = 0; i < m_itemMenu->GetChildCount(); ++i)
    {
        auto button = CastPtr<UIButton>(m_itemMenu->GetChildByIndex(i));
        button->ButtonState().AddListenerWhen(UIButton::State::Normal,
                [](UIButton* button)
        {
            button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonLegend"))->SetVisible(false);
        });
        button->ButtonState().AddListenerWhen(UIButton::State::Focused,
            [](UIButton* button)
        {
            button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonLegend"))->SetVisible(true);
        });
    }

    m_itemMenu->SetVisible(true);
}

void Sample::ShowGlobalMenu()
{
    m_itemMenu->Reset();

    auto menuButton = m_uiManager.FindTypedById<UIButton>(ID("MenuButton"));

    auto menuX = menuButton->GetRelativePositionInRefUnits().x;
    auto menuY = menuButton->GetRelativePositionInRefUnits().y + menuButton->GetRelativeSizeInRefUnits().y;
    m_itemMenu->SetRelativePositionInRefUnits(Vector2(menuX, menuY));

    auto button = CastPtr<UIButton>(m_itemMenu->AddChildFromPrefab("#menu_item_prefab"));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText("Show Addons in Store");
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
    {
        ShowAssociatedProducts();
        CloseMenu();
    });

    button = CastPtr<UIButton>(m_itemMenu->AddChildFromPrefab("#menu_item_prefab"));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText("Rate and review");
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
    {
        RateMyGame();
        CloseMenu();
    });

    button = CastPtr<UIButton>(m_itemMenu->AddChildFromPrefab("#menu_item_prefab"));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText("Redeem code");
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
    [this](UIButton*)
    {
        RedeemToken();
        CloseMenu();
    });

    button = CastPtr<UIButton>(m_itemMenu->AddChildFromPrefab("#menu_item_prefab"));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText("Query addons");
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
    {
        QueryAddonLicenses();
        ShowPopup("Querying for addons licensed by digital license...");
        CloseMenu();
    });

    button = CastPtr<UIButton>(m_itemMenu->AddChildFromPrefab("#menu_item_prefab"));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText("Query catalog");
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
    {
        QueryCatalog();
        ShowPopup("Querying for all products that can be sold by this game...");
        CloseMenu();
    });

    button = CastPtr<UIButton>(m_itemMenu->AddChildFromPrefab("#menu_item_prefab"));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText("Query collections");
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
    {
        QueryCollections();
        ShowPopup("Querying collections...");
        CloseMenu();
    });

    button = CastPtr<UIButton>(m_itemMenu->AddChildFromPrefab("#menu_item_prefab"));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText("Query game product");
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
    {
        QueryGameProduct();
        ShowPopup("Querying game product...");
        CloseMenu();
    });

    button = CastPtr<UIButton>(m_itemMenu->AddChildFromPrefab("#menu_item_prefab"));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText("Query game license");
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
    {
        QueryGameLicense();
        ShowPopup("Querying game license...");
        CloseMenu();
    });

    button = CastPtr<UIButton>(m_itemMenu->AddChildFromPrefab("#menu_item_prefab"));
    button->GetTypedSubElementById<UIStaticText>(ID("MenuItemButtonText"))->SetDisplayText("Query license token");
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [this](UIButton*)
    {
        QueryLicenseToken();
        ShowPopup("Querying license token...");
        CloseMenu();
    });

    m_itemMenu->SetVisible(true);
}

void Sample::CloseMenu(bool doUpdate)
{
    if (doUpdate && m_closeMenu)
    {
        // actual update of StackPanel done in Update function only
        m_itemMenu->SetVisible(false);
        m_itemMenu->Reset();
        m_closeMenu = false;

        // restore focus to popup if it was up
        if (GetPopup()->IsVisible())
        {
            m_uiManager.SetFocus(GetPopup()->GetTypedSubElementById<UIButton>(ID("PopupButton")));
        }
    }
    else
    {
        // set to pending close
        m_closeMenu = true;
    }
}

#pragma endregion // UI Methods
