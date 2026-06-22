//--------------------------------------------------------------------------------------
// InGameStore_Desktop.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "InGameStore_Desktop.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "StringUtil.h"

extern void ExitSample();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

void DEBUGLOG(const char *format, ...)
{
    static char buffer[2048];

    va_list args;
    va_start(args, format);
    vsprintf_s(buffer, format, args);
    va_end(args);

    OutputDebugStringA(buffer);
}

Sample::Sample() noexcept(false) :
    m_xStoreContext(nullptr),
    m_currentButton(nullptr),
    m_uiUpdate(0),
    m_listViewPage(0),
    m_pauseKbInput(false),
    m_asyncQueue(nullptr)
{
    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_asyncQueue)
    );

    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->RegisterDeviceNotify(this);

    m_liveResources = std::make_shared<ATG::LiveResources>(m_asyncQueue);
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("In-Game Store Desktop Sample");

    ATG::UIConfig uiconfig;
    uiconfig.colorBackground = DirectX::XMFLOAT4(0, 0, 0, 1);
    uiconfig.colorFocus = DirectX::XMFLOAT4(0, 153.f / 255.f, 1.f / 255.f, 1);
    m_ui = std::make_shared<ATG::UIManager>(uiconfig);

}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
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
    // NOTE: When running the app from the Start Menu (required for
    //	Store API's to work) the Current Working Directory will be
    //	returned as C:\Windows\system32 unless you overwrite it.
    //	The sample relies on the font and image files in the .exe's
    //	directory and so we do the following to set the working
    //	directory to what we want.
    char dir[_MAX_PATH] = {};
    (void)GetModuleFileNameA(NULL, dir, _MAX_PATH);
    m_ExePath = dir;
    m_ExePath = m_ExePath.substr(0, m_ExePath.find_last_of("\\"));
    SetCurrentDirectoryA(m_ExePath.c_str());

    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

    m_ui->LoadLayout(L".\\Assets\\SampleUI.csv", L".\\Assets");

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    //  Custom Sample Code
    ProductListViewRow::s_ui = m_ui;
    //ProductListViewRow::s_images = m_imageManager;
    ProductListViewRow::s_sample = this;

    m_liveResources->SetUserChangedCallback([this](XUserHandle user)
    {
        m_liveInfoHUD->SetUser(user, m_asyncQueue);
        m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel)->Show();
    });

    m_liveResources->SetUserSignOutCompletedCallback([this](XUserHandle /*user*/)
    {
        m_liveInfoHUD->SetUser(nullptr, m_asyncQueue);

        // Add code to handle last user signing out.
        m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel)->Close();
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

    //	Because this is on Desktop we don't need to pass in a user,
    //	It defaults to who is signed into the Windows Store App
    HRESULT hr = XStoreCreateContext(nullptr, &m_xStoreContext);
    if (FAILED(hr))
    {
        DEBUGLOG("\tExtended Error creating XStoreContext: 0x%08x\n", static_cast<unsigned int>(hr));
    }
    QueryProducts();

}

#pragma region In-Game Store
UIProductDetails Sample::CopyToUIProduct(const XStoreProduct* Product)
{
    UIProductDetails copy;

    copy.storeId = DX::Utf8ToWide(Product->storeId);
    copy.title = DX::Utf8ToWide(Product->title);
    copy.description = DX::Utf8ToWide(Product->description);

    copy.productKind = Product->productKind;
    copy.price = CopyToUIPrice(Product->price);
    copy.hasDigitalDownload = Product->hasDigitalDownload;
    copy.isInUserCollection = Product->isInUserCollection;

    copy.skusCount = Product->skusCount;
    for (uint32_t i = 0; i < Product->skusCount; i++)
    {
        copy.skus.push_back(CopyToUISku(Product->skus[i]));
    }

    return copy;
}

UIProductSku Sample::CopyToUISku(XStoreSku Sku)
{
    UIProductSku copy;

    copy.skuId = DX::Utf8ToWide(Sku.skuId);
    copy.title = DX::Utf8ToWide(Sku.title);

    std::wstring description;

    copy.price = CopyToUIPrice(Sku.price);
    copy.isTrial = Sku.isTrial;
    copy.isInUserCollection = Sku.isInUserCollection;
    copy.isSubscription = Sku.isSubscription;
    copy.collectionData = Sku.collectionData;

    copy.bundledSkusCount = Sku.bundledSkusCount;
    for (uint32_t i = 0; i < Sku.bundledSkusCount; i++)
    {
        copy.bundledSkus.push_back(DX::Utf8ToWide(Sku.bundledSkus[i]));
    }

    copy.availabilitiesCount = Sku.availabilitiesCount;
    for (uint32_t i = 0; i < Sku.availabilitiesCount; i++)
    {
        copy.availabilities.push_back(CopyToUIAvailability(Sku.availabilities[i]));
    }

    return copy;
}

UIProductAvailability Sample::CopyToUIAvailability(XStoreAvailability Availability)
{
    UIProductAvailability copy;
    copy.availabilityId = DX::Utf8ToWide(Availability.availabilityId);
    copy.endDate = Availability.endDate;
    copy.price = CopyToUIPrice(Availability.price);

    return copy;
}

UIProductPrice Sample::CopyToUIPrice(XStorePrice Price)
{
    UIProductPrice copy;
    copy.basePrice = Price.basePrice;
    copy.price = Price.price;
    copy.recurrencePrice = Price.recurrencePrice;
    copy.isOnSale = Price.isOnSale;
    copy.saleEndDate = Price.saleEndDate;

    copy.currencyCode = DX::Utf8ToWide(Price.currencyCode);
    copy.formattedBasePrice = DX::Utf8ToWide(Price.formattedBasePrice);;
    copy.formattedPrice = DX::Utf8ToWide(Price.formattedPrice);;
    copy.formattedRecurrencePrice = DX::Utf8ToWide(Price.formattedRecurrencePrice);;

    return copy;
}

void Sample::QueryProducts()
{

    DEBUGLOG("Calling XStoreQueryAssociatedProductsAsync");

    XAsyncBlock *async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock *async)
    {
        XStoreProductQueryHandle queryHandle = nullptr;

        HRESULT hr = XStoreQueryProductsResult(async, &queryHandle);
        if (SUCCEEDED(hr))
        {
            auto pThis = reinterpret_cast<Sample*>(async->context);

            //Update the list of products that came from the Query
            auto callback = [](_In_ const XStoreProduct* product, _In_ void* context) -> bool
            {
                auto pThis = reinterpret_cast<Sample*>(context);

                //  We create a copy of the product because
                UIProductDetails productCopy = pThis->CopyToUIProduct(product);

                DEBUGLOG("\tProduct:\n");
                DEBUGLOG("\t\tstoreId    : %S\n", productCopy.storeId.c_str());
                DEBUGLOG("\t\tproductKind: %S\n", pThis->ProductKindToString(productCopy.productKind).c_str());
                DEBUGLOG("\t\tisInUserCollection: %S\n", productCopy.isInUserCollection ? "true" : "false");

                // categorize catalog based on ProductKind
                if (productCopy.productKind == XStoreProductKind::UnmanagedConsumable)
                {
                    //  Put the Unmanaged Consumables and consumables together for our UI
                    pThis->m_catalog[XStoreProductKind::Consumable].insert(productCopy.storeId);
                }
                else
                {
                    pThis->m_catalog[productCopy.productKind].insert(productCopy.storeId);
                }
                pThis->m_catalogDetails.insert_or_assign(productCopy.storeId, productCopy);

                return true;
            };

            DEBUGLOG("\nEnumerating...");
            hr = XStoreEnumerateProductsQuery(queryHandle, async->context, callback);

            pThis->UpdateProductList();

            if (XStoreProductsQueryHasMorePages(queryHandle))
            {
                // TODO: Demo GetNextPageStoreProductsQueryAsync
                DEBUGLOG("\nHas more pages!");
            }
            DEBUGLOG("\n");
        }
        delete async;
    };

    XStoreProductKind typeFilter =
        XStoreProductKind::Consumable |
        XStoreProductKind::Durable |
        XStoreProductKind::Game |
        XStoreProductKind::UnmanagedConsumable |
        XStoreProductKind::Pass;

    HRESULT hr = XStoreQueryAssociatedProductsAsync(
        m_xStoreContext,// XStoreContext
        typeFilter,     // Product filter types
        25,             // Products per page
        async);         // XAsyncBlock

    if (FAILED(hr))
    {
        delete async;
        DEBUGLOG("\nError calling XStoreQueryAssociatedProductsAsync : 0x%08X\n", static_cast<unsigned int>(hr));
    }

}

void Sample::QueryGameLicense()
{
    DEBUGLOG("Calling XStoreQueryGameLicenseAsync");

    XAsyncBlock *async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock *async)
    {
        XStoreGameLicense license;
        HRESULT hr = XStoreQueryGameLicenseResult(async, &license);
        if (SUCCEEDED(hr))
        {
            //	UI output
            std::wstring info = L"";
            info.append(L"Game License Info\n");
            info.append(L"isActive: ");
            info.append(license.isActive ? L"true" : L"false");
            info.append(L"\nisDiscLicense: ");
            info.append(license.isDiscLicense ? L"true" : L"false");
            info.append(L"\nisTrial: ");
            info.append(license.isTrial ? L"true" : L"false");
            info.append(L"\nisTrialOwnedByThisUser: ");
            info.append(license.isTrialOwnedByThisUser ? L"true" : L"false");

            auto pThis = reinterpret_cast<Sample*>(async->context);
            pThis->m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_actionResult)->SetText(info.c_str());
            pThis->m_ui->FindControl<ATG::IControl>(c_sampleUIPanel, c_includedIn)->SetVisible(false);
            pThis->m_ui->FindControl<ATG::IControl>(c_sampleUIPanel, c_bundleName)->SetVisible(false);

            //	Additional infor and formatting for debug output
            DEBUGLOG("\tXStoreGameLicense info:\n");
            DEBUGLOG("\t\tskuStoreId:             %s\n", license.skuStoreId);

            DEBUGLOG("\t\tisActive:               %s\n", license.isActive ? "true" : "false");
            DEBUGLOG("\t\tisTrialOwnedByThisUser: %s\n", license.isTrialOwnedByThisUser ? "true" : "false");
            DEBUGLOG("\t\tisDiscLicense:          %s\n", license.isDiscLicense ? "true" : "false");
            DEBUGLOG("\t\tisTrial:                %s\n", license.isTrial ? "true" : "false");

            if (license.isTrial)
            {
                DEBUGLOG("\t\ttrialTimeRemainingInSeconds: %zu\n", license.trialTimeRemainingInSeconds);
                DEBUGLOG("\t\ttrialUniqueId:               %s\n", license.trialUniqueId);

                char buff[32];
                struct tm  timeinfo = {};
                ::localtime_s(&timeinfo, &license.expirationDate);
                strftime(buff, 32, "%m.%d.%Y %H:%M:%S", &timeinfo);
                DEBUGLOG("\t\texpirationDate:              %s\n", buff);
            }
            DEBUGLOG("\n");
        }
        else
        {
            DEBUGLOG("\nError calling XStoreQueryGameLicenseResult : 0x%08X\n", static_cast<unsigned int>(hr));
        }

        delete async;
    };

    HRESULT hr = XStoreQueryGameLicenseAsync(
        m_xStoreContext,
        async);

    if (FAILED(hr))
    {
        delete async;
        DEBUGLOG("\nError calling XStoreQueryGameLicenseAsync : 0x%08X\n", static_cast<unsigned int>(hr));
    }
}

void Sample::QueryCollections()
{
    DEBUGLOG("Calling XStoreQueryEntitledProductsAsync");

    XAsyncBlock *async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock *async)
    {
        XStoreProductQueryHandle queryHandle = nullptr;

        if (SUCCEEDED(XStoreQueryProductsResult(async, &queryHandle)))
        {
            auto pThis = reinterpret_cast<Sample*>(async->context);

            //Update the list of products that came from the Query
            auto callback = [](_In_ const XStoreProduct* product, _In_ void* context) -> bool
            {
                auto pThis = reinterpret_cast<Sample*>(context);
                UIProductDetails productCopy = pThis->CopyToUIProduct(product);

                DEBUGLOG("\tProduct:\n");
                DEBUGLOG("\t\tstoreId    : %S\n", productCopy.storeId.c_str());
                DEBUGLOG("\t\tproductKind: %S\n", pThis->ProductKindToString(productCopy.productKind).c_str());
                DEBUGLOG("\t\tisInUserCollection: %s\n", productCopy.isInUserCollection ? "true" : "false");
                DEBUGLOG("\n");

                // categorize catalog based on ProductKind
                XStoreProductKind targetProductKind = productCopy.productKind;
                if (productCopy.productKind == XStoreProductKind::UnmanagedConsumable)
                {
                    //  Put the Unmanaged Consumables and consumables together for our UI
                    targetProductKind = XStoreProductKind::Consumable;
                }

                pThis->m_catalogDetails.insert_or_assign(productCopy.storeId, productCopy);

                return true;
            };

            DEBUGLOG("\nEnumerating...");
            HRESULT hr = XStoreEnumerateProductsQuery(queryHandle, async->context, callback);

            if (SUCCEEDED(hr) && XStoreProductsQueryHasMorePages(queryHandle))
            {
                // TODO: Demo GetNextPageStoreProductsQueryAsync
                DEBUGLOG("Has more pages!");
            }
            DEBUGLOG("\n");

            //  We may have new entitlements so refresh the UI to show that
            pThis->UpdateProductList();

        }

        delete async;
    };

    XStoreProductKind typeFilter =
        XStoreProductKind::Consumable |
        XStoreProductKind::Durable |
        XStoreProductKind::Game |
        XStoreProductKind::UnmanagedConsumable |
        XStoreProductKind::Pass;

    HRESULT hr = XStoreQueryEntitledProductsAsync(
        m_xStoreContext,
        typeFilter,     // Product filter types
        25,             // Products per page
        async);

    if (FAILED(hr))
    {
        delete async;
        DEBUGLOG("\nError calling XStoreQueryEntitledProductsAsync : 0x%08X\n", static_cast<unsigned int>(hr));
    }
}

void Sample::QueryLicenseToken()
{
    DEBUGLOG("Calling XStoreQueryLicenseTokenAsync");

    XAsyncBlock *async = new XAsyncBlock{};
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
            DEBUGLOG("Failed retrieve the license token size: 0x%08X\r\n", static_cast<unsigned int>(hr));
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
            DEBUGLOG("Failed retrieve the license token result: 0x%08X\r\n", static_cast<unsigned int>(hr));
            return;
        }

        DEBUGLOG("License Token result: %s\r\n", result.data());
        delete async;
    };

    const char * ids[] = { "9NN4ZHKML55R"};	//	StoreID of the InGameStoreSample

    HRESULT hr = XStoreQueryLicenseTokenAsync(
        m_xStoreContext,
        ids,     // Product filter types
        1,       // Products per page
        "a",
        async);

    if (FAILED(hr))
    {
        delete async;
        DEBUGLOG("\nError calling XStoreQueryLicenseTokenAsync : 0x%08X\n", static_cast<unsigned int>(hr));
    }
}


XStoreProductKind Sample::StringToProductKind(const std::wstring& type)
{
    if (type == L"Consumable")
    {
        return XStoreProductKind::Consumable;
    }
    else if (type == L"Durable")
    {
        return XStoreProductKind::Durable;
    }
    else if (type == L"Game")
    {
        return XStoreProductKind::Game;
    }
    else if (type == L"Pass")
    {
        return XStoreProductKind::Pass;
    }
    else
    {
        return  XStoreProductKind::Consumable |
            XStoreProductKind::Durable |
            XStoreProductKind::Game |
            XStoreProductKind::UnmanagedConsumable |
            XStoreProductKind::Pass;
    }
}

std::wstring Sample::ProductKindToString(XStoreProductKind kind)
{
    switch (kind)
    {
    case XStoreProductKind::UnmanagedConsumable:
        return L"UnmanagedConsumable";
    case XStoreProductKind::Consumable:
        return L"Consumable";
    case XStoreProductKind::Durable:
        return L"Durable";
    case XStoreProductKind::Game:
        return L"Game";
    case XStoreProductKind::Pass:
        return L"Pass";
    case XStoreProductKind::None:
        return L"Pass";
    default:
        return L"Unknown";
    }
}

void Sample::MakePurchase()
{
    //  Convert the wstring to a char* to be used with the API
    std::string tmp = DX::WideToUtf8(m_currentProduct);
    const char* storeId = tmp.c_str();

    XAsyncBlock *async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock *async)
    {
        HRESULT hr = XStoreShowPurchaseUIResult(async);

        if (SUCCEEDED(hr))
        {
            DEBUGLOG("\n\tPurchase Succeeded\n");

            //	Update the collections info on what is owned
            auto pThis = reinterpret_cast<Sample*>(async->context);
            pThis->QueryCollections();
        }
        else
        {
            DEBUGLOG("\n\tPurchase Failed or canceled\n");
            DEBUGLOG("\tExtended Error : 0x%08x\n", static_cast<unsigned int>(hr));
        }

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
        printf("\nError calling XStoreShowPurchaseUIAsync : 0x%08X\r\n", static_cast<unsigned int>(hr));
        return;
    }
}

void Sample::CompleteCatalogItemsRefresh(const std::wstring& type)
{
    auto count = (type == L"All") ? m_catalogDetails.size() : m_catalog[StringToProductKind(type)].size();
    DEBUGLOG("** GetAssociatedStoreProductsAsync: \"%s\" %u items\n", type.c_str(), count);

    // no need to retrieve any additional details (cf. get_catalog_item_details), all info already returned in StoreProducts

    if (type == L"All" && count == 0)
    {
        ShowPopup(L"No products returned for %s", m_storeId.c_str());
    }

    /*  Todo: Images not yet ported for sample
    for (auto& product : m_catalogDetails)
    {

        for (auto image : product.second->Images)
        {
            // initiate downloads for images of these types to be used for this sample
            if (image->ImagePurposeTag == L"Logo" || image->ImagePurposeTag == L"BoxArt" || image->ImagePurposeTag == L"Poster")
            {
                std::wstring imageID = product.first;
                imageID += L"\\";
                imageID += image->ImagePurposeTag->Data();

                m_imageManager->DownloadImage(image->Uri->AbsoluteUri->Data(), imageID.c_str());
            }
        }

    }
    */

    m_uiUpdate |= ProductList | ProductDetails;
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = static_cast<float>(timer.GetElapsedSeconds());

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

        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            m_pauseKbInput = true;
            QueryProducts();
        }
        else if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Y))
        {
            m_pauseKbInput = true;
            QueryGameLicense();
        }
        else if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_pauseKbInput = true;
            QueryCollections();
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

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::A))
    {
        auto button = dynamic_cast<ATG::Button*>(m_currentButton);

        // Make sure the button is actually enabled before calling its callback.
        if (button && button->IsEnabled())
        {
            auto panel = m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel);
            button->OnSelected(panel);
        }
    }
    else if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::X))
    {
        m_pauseKbInput = true;
        QueryProducts();
    }
    else if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Y))
    {
        m_pauseKbInput = true;
        QueryGameLicense();
    }
    else if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::B))
    {
        m_pauseKbInput = true;
        QueryCollections();
    }
    else if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::L))
    {
        m_pauseKbInput = true;
        QueryLicenseToken();
    }
    else if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::LeftControl))
    {
        m_twist->MovePrevious();
        m_listViewPage = 0;
        auto panel = m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel);
        UpdateProductList();
        auto control = panel->Find(c_item00);
        if (control->IsVisible())
        {
            panel->SetFocus(control);
        }
        TriggerUpdateDetails();
    }
    else if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::RightControl))
    {
        m_twist->MoveNext();
        m_listViewPage = 0;
        auto panel = m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel);
        UpdateProductList();
        auto control = panel->Find(c_item00);
        if (control->IsVisible())
        {
            panel->SetFocus(control);
        }
        TriggerUpdateDetails();
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

    m_ui->Update(elapsedTime, m_mouse->Get(), m_keyboard->Get());

    while (XTaskQueueDispatch(m_asyncQueue, XTaskQueuePort::Completion, 0))
    {}

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

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);
    m_liveInfoHUD->Render(commandList);
    m_ui->Render(commandList);

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
}

void Sample::OnResuming()
{
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
    m_liveResources->Refresh();
    m_ui->Reset();
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

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        Descriptors::Count,
        Descriptors::Reserve
        );

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    m_ui->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto vp = m_deviceResources->GetScreenViewport();
    m_liveInfoHUD->SetViewport(vp);

    auto size = m_deviceResources->GetOutputSize();
    m_ui->SetWindow(size);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

#pragma region UI Methods
void Sample::UpdateProductList()
{
    auto currentProductType = c_productTypeStrings[m_twist->CurrentIndex()];
    std::vector<UIProductDetails> products;

    std::lock_guard<std::mutex> lock(m_catalogLock);

    if (currentProductType == L"All")
    {
        //  If the current tab is set to "All" then add all products
        for (auto &catalogItem : m_catalogDetails)
        {
            products.push_back(catalogItem.second);
        }
    }
    else
    {
        for (auto &product : m_catalog[StringToProductKind(currentProductType)])
        {
            products.push_back(m_catalogDetails[product]);
        }
    }

    UpdateListView(m_listViewPage, products.size());
    m_listView->UpdateRows(products, m_listViewPage * c_pageSize);
}

void Sample::UpdateDetailsView()
{
    auto currentProduct = c_productTypeStrings[m_twist->CurrentIndex()];

    m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_quantity)->SetText(L"");
    m_ui->FindControl<ATG::IControl>(c_sampleUIPanel, c_includedIn)->SetVisible(false);
    m_ui->FindControl<ATG::IControl>(c_sampleUIPanel, c_bundleName)->SetVisible(false);
    m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_actionResult)->SetText(L"");

    if ((currentProduct != L"All") && m_catalog[StringToProductKind(currentProduct)].empty())
    {
        m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_productTitle)->SetText(L"No products of this type were found.");
        m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_priceLabel)->SetText(L"");
        m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_productDescription)->SetText(L"");
        /*  Todo: Images not yet ported for sample
        //m_ui->FindControl<ATG::IControl>(c_sampleUIPanel, c_posterImage)->SetVisible(false);
        */
        m_currentProduct = L"";
    }
    else if (m_catalogDetails.find(m_currentProduct) != m_catalogDetails.end())
    {
        auto productId = m_currentProduct;
        auto &product = m_catalogDetails[m_currentProduct];

        m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_productTitle)->SetText(product.title.c_str());
        m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_productDescription)->SetText(product.description.c_str());
        m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_priceLabel)->SetText(product.price.formattedPrice.c_str());

        if (product.productKind == XStoreProductKind::Consumable)
        {
            uint32_t quantity = 0;
            for (uint32_t i = 0; i < product.skusCount; i++)
            {
                if (quantity < product.skus[i].collectionData.quantity)
                {
                    quantity = product.skus[i].collectionData.quantity;
                }
            }
            wchar_t buff[100];
            _snwprintf_s(buff, sizeof(buff), L"Current User Quantity: %" PRIu32, quantity);

            m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_quantity)->SetText(buff);
        }

        /*  Todo: Images not yet ported for sample
        auto posterImage = m_ui->FindControl<ATG::Image>(c_sampleUIPanel, c_posterImage);
        posterImage->SetVisible(true);
        auto imageId = posterImage->GetImageId();
        auto image = m_imageManager->GetImage(productId + L"\\Poster");

        if (image != nullptr)
        {
            m_ui->RegisterImage(imageId, image);
        }
        else
        {
            m_ui->RegisterImage(imageId, m_imageManager->GetImage(L"LoadingBrandedBoxArt"));
        }
        */
    }
    m_pauseKbInput = false;
}

void Sample::TriggerUpdateDetails()
{
    std::wstring *productId = reinterpret_cast<std::wstring*>(m_currentButton->GetUser());
    if (productId != nullptr)
    {
        m_currentProduct.assign(productId->begin(), productId->end());
        UpdateDetailsView();
    }
}

void Sample::SetupUI()
{
    // Needs to match order of c_productTypeStrings
    static std::vector<std::wstring> s_options = { L"ALL", L"GAMES", L"DURABLES", L"CONSUMABLES", L"PASS" };
    m_twist = std::make_unique<UITwist>(m_ui.get(), c_sampleUIPanel, c_twistElementStart, s_options);

    /*  Todo: Images not yet ported for sample
    //m_imageManager->LoadImageFromFile(L"LoadingBoxArt", L"Assets\\DownloadingBoxArt.png");
    //m_imageManager->LoadImageFromFile(L"LoadingBrandedBoxArt", L"Assets\\DownloadingBrandedBoxArt.png");
    */

    ListViewConfig config = { c_pageSize, c_item00, c_itemOffset, c_sampleUIPanel };
    m_listView = std::make_unique<ListView<UIProductDetails, ProductListViewRow>>(m_ui, config);
    m_listView->ClearAllRows();
    m_listView->SetSelectedCallback([this](ATG::IPanel*, ATG::IControl* control)
    {
        m_currentProduct = *(reinterpret_cast<std::wstring*>(control->GetUser()));
        if (!m_currentProduct.empty() && control->GetUser())
        {
            MakePurchase();
        }
    });

    m_ui->FindControl<ATG::Button>(c_sampleUIPanel, c_previousButton)->SetCallback([this](ATG::IPanel*, ATG::IControl*)
    {
        m_listViewPage--;
        UpdateProductList();
    });

    m_ui->FindControl<ATG::Button>(c_sampleUIPanel, c_nextButton)->SetCallback([this](ATG::IPanel*, ATG::IControl*)
    {
        m_listViewPage++;
        UpdateProductList();
    });

    m_listView->SetFocusedCallback([this](ATG::IPanel *, ATG::IControl *control)
    {
        m_currentProduct = *(reinterpret_cast<std::wstring*>(control->GetUser()));
        UpdateDetailsView();
    });

    UpdateDetailsView();
}

void Sample::ShowPopup(const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);

    wchar_t buff[1024] = {};
    vswprintf_s(buff, format, args);

    m_ui->FindControl<ATG::TextLabel>(c_popupPanel, c_popupLabel)->SetText(buff);
    m_ui->Find(c_popupPanel)->Show();

    m_currentButton = m_ui->FindControl<ATG::Button>(c_popupPanel, c_popupButton);

    va_end(args);
}

void Sample::UpdateListView(size_t currentPage, size_t totalProducts)
{
    if (currentPage == 0)
    {
        auto button = m_ui->FindControl<ATG::Button>(c_sampleUIPanel, c_previousButton);
        button->SetEnabled(false);
        button->SetVisible(false);
        m_ui->FindControl<ATG::Image>(c_sampleUIPanel, c_previousImage)->SetVisible(false);
        auto panel = m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel);
        auto control = panel->Find(c_item00);
        m_currentButton = control;
        if (control->IsVisible())
        {
            panel->SetFocus(control);
            TriggerUpdateDetails();
        }
    }
    else
    {
        auto button = m_ui->FindControl<ATG::Button>(c_sampleUIPanel, c_previousButton);
        button->SetEnabled(true);
        button->SetVisible(true);
        m_ui->FindControl<ATG::Image>(c_sampleUIPanel, c_previousImage)->SetVisible(true);
    }

    if ((currentPage + 1) * c_pageSize > totalProducts)
    {
        auto button = m_ui->FindControl<ATG::Button>(c_sampleUIPanel, c_nextButton);
        button->SetEnabled(false);
        button->SetVisible(false);
        m_ui->FindControl<ATG::Image>(c_sampleUIPanel, c_nextImage)->SetVisible(false);
        auto panel = m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel);
        auto control = panel->Find(c_item00);
        m_currentButton = control;
        if (control->IsVisible())
        {
            panel->SetFocus(control);
            TriggerUpdateDetails();
        }
    }
    else
    {
        auto button = m_ui->FindControl<ATG::Button>(c_sampleUIPanel, c_nextButton);
        button->SetEnabled(true);
        button->SetVisible(true);
        m_ui->FindControl<ATG::Image>(c_sampleUIPanel, c_nextImage)->SetVisible(true);
    }
}
#pragma endregion

#pragma region ListViewRow Methods
std::weak_ptr<ATG::UIManager> ProductListViewRow::s_ui;
Sample* ProductListViewRow::s_sample;

//  Todo: Images not yet ported for sample
//std::weak_ptr<ImageManager> ProductListViewRow::s_images;

void ProductListViewRow::Show()
{
    m_selectBtn->SetVisible(true);
    m_selectBtn->SetEnabled(true);
    m_productImage->SetVisible(true);
}

void ProductListViewRow::Hide()
{
    m_selectBtn->SetVisible(false);
    m_selectBtn->SetEnabled(false);
    m_productImage->SetVisible(false);
    m_productOwned->SetVisible(false);
    m_productLicensed->SetVisible(false);
}

void ProductListViewRow::SetControls(ATG::IPanel *parent, int rowStart)
{
    m_selectBtn = dynamic_cast<ATG::Button*>(parent->Find(unsigned(rowStart)));
    m_selectBtn->SetUser(reinterpret_cast<void*>(&m_productId));

    m_productImage = dynamic_cast<ATG::Image*>(parent->Find(unsigned(rowStart + 1)));
    m_productOwned = dynamic_cast<ATG::Image*>(parent->Find(unsigned(rowStart + 2)));
    m_productLicensed = dynamic_cast<ATG::Image*>(parent->Find(unsigned(rowStart + 3)));

    /*  Todo: Images not yet ported for sample
    auto images = s_images.lock();
    auto ui = s_ui.lock();
    ui->RegisterImage(rowStart, images->GetImage(L"LoadingBoxArt"));
    m_productImage->SetImageId(rowStart);
    */
}

void ProductListViewRow::Update(UIProductDetails item)
{
    m_productId = item.storeId;
    auto productId = m_productId;

    if (s_ui.expired()) //|| s_images.expired())
    {
        return;
    }

    // IsInUserCollection should be equivalent to the results of GetUserCollectionAsync
    m_productOwned->SetVisible(item.isInUserCollection);
    //m_productOwned->SetVisible(s_sample->ProductIsOwned(m_productId));
    //m_productLicensed->SetVisible(s_sample->ProductIsLicensed(m_productId));

    auto ui = s_ui.lock();
    /* No images yet
    auto images = s_images.lock();

    unsigned int imageId = m_productImage->GetImageId();

    std::string imageName = m_productId;
    imageName += L"\\Logo";

    auto image = images->GetImage(imageName);

    if (image == nullptr)
    {
        imageName = m_productId;
        imageName += L"\\BoxArt";

        image = images->GetImage(imageName);
    }

    // If the image was found register it
    if (image != nullptr && m_productId == productId)
    {
        ui->RegisterImage(imageId, image);
    }
    else
    {
        ui->RegisterImage(imageId, images->GetImage(L"LoadingBoxArt"));
    }
    */
}

void ProductListViewRow::SetSelectedCallback(ATG::IControl::callback_t callback)
{
    m_selectBtn->SetCallback(callback);
}

void ProductListViewRow::SetFocusedCallback(ATG::IControl::callback_t callback)
{
    m_selectBtn->SetFocusCb(callback);
}
#pragma endregion
