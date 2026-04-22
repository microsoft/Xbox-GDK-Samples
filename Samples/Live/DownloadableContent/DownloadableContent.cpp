//--------------------------------------------------------------------------------------
// DownloadableContent.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DownloadableContent.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "FileReader.h"

#include <XGameErr.h>

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const std::pair<XPackageKind, XPackageEnumerationScope> c_filterOptions[4] = {
        std::pair{ XPackageKind::Content, XPackageEnumerationScope::ThisOnly },
        std::pair{ XPackageKind::Content, XPackageEnumerationScope::ThisAndRelated },
        std::pair{ XPackageKind::Game, XPackageEnumerationScope::ThisOnly },
        std::pair{ XPackageKind::Game, XPackageEnumerationScope::ThisAndRelated }
    };
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_showConsole(false),
    m_storeContext(nullptr),
    m_asyncQueue(nullptr),
    m_packageInstallToken{},
    m_isStoreEnumerating(false)
{
    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_asyncQueue)
    );

    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);

    m_liveResources = std::make_shared<ATG::LiveResources>(m_asyncQueue);
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("Downloadable Content Sample");

    //  Register for when a DLC package is installed
    DX::ThrowIfFailed(
        XPackageRegisterPackageInstalled(m_asyncQueue, this, [](void* context, const XPackageDetails* package)
            {
                auto pThis = reinterpret_cast<Sample*>(context);
                debugPrint("Package Installed event received: %s\n", package->displayName);
                pThis->RefreshInstalledPackages();

            }, &m_packageInstallToken)
    );
}

Sample::~Sample()
{
    if (m_backgroundImage)
    {
        UnmountTrackedPackages();
    }

    if (m_storeContext)
    {
        XStoreCloseContextHandle(m_storeContext);
    }

    XPackageUnregisterPackageInstalled(m_packageInstallToken, true);

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
    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_liveResources->SetUserChangedCallback([this](XalUserHandle user)
        {
            m_liveInfoHUD->SetUser(user, m_asyncQueue);

            std::string gamertag = m_liveResources->GetGamertag();
            debugPrint("User changed: %s\n", gamertag.c_str());

            // On Xbox, tracked package will be unmounted in the license lost event.
            // However, since it may not occur on a PC, we will unmount it here.
            UnmountTrackedPackages();

            CreateStoreContext();

            this->RefreshInstalledPackages();
            this->RefreshStoreProducts();
        });

    m_liveResources->SetUserSignOutCompletedCallback([this](XalUserHandle /*user*/)
        {
            m_liveInfoHUD->SetUser(nullptr, m_asyncQueue);

            debugPrint("User signed out.\n");

            // On Xbox, tracked package will be unmounted in the license lost event.
            // However, since it may not occur on a PC, we will unmount it here.
            UnmountTrackedPackages();

            XStoreCloseContextHandle(m_storeContext);
            m_storeContext = nullptr;

            this->RefreshInstalledPackages();
            this->RefreshStoreProducts();
        });

    m_liveResources->SetErrorHandler([this](HRESULT error)
        {
            if (error == E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED || error == E_GAMEUSER_NO_DEFAULT_USER)
            {
                m_liveResources->SignInWithUI();
            }
            else if (error == E_GAMEUSER_NO_PACKAGE_IDENTITY)
            {
                ErrorMessage("LiveResource error : E_GAMEUSER_NO_PACKAGE_IDENTITY\n");
            }
            else // Handle other error cases.
            {
                ErrorMessage("LiveResource error : 0x%08X\n", error);
            }
        });

    InitializeUI();
    RefreshInstalledPackages();

    m_liveResources->Initialize();
    m_liveInfoHUD->Initialize();
}
#pragma region Store Methods

void Sample::CreateStoreContext()
{
    if (m_storeContext)
    {
        XStoreCloseContextHandle(m_storeContext);
        m_storeContext = nullptr;
    }

#ifdef _GAMING_XBOX
    HRESULT hr = XStoreCreateContext(m_liveResources->GetUser(), &m_storeContext);
#else
    HRESULT hr = XStoreCreateContext(nullptr, &m_storeContext);
#endif

    if (FAILED(hr)) // Unable to create an XStoreContext
    {
        ErrorMessage("XStoreCreateContext failed : 0x%08X\n", hr);
        return;
    }

    if (!XUserIsStoreUser(m_liveResources->GetUser()))
    {
        ErrorMessage("Current user doesn't match store user.\n");
    }
}

void Sample::RefreshStoreProducts()
{
    if (m_isStoreEnumerating)
        return;

    m_isStoreEnumerating = true;

    m_productList.clear();
    m_uiManager.ClearChildren(m_uiProductList);

    if (!m_storeContext)
    {
        m_isStoreEnumerating = false;
        return;
    }

    XStoreProductKind productKinds = XStoreProductKind::Durable;

    auto async = std::make_unique<XAsyncBlock>();
    async->queue = m_asyncQueue;
    async->context = this;
    async->callback = [](XAsyncBlock* async)
        {
            std::unique_ptr<XAsyncBlock> asyncPtr{ async };

            Sample* pThis = reinterpret_cast<Sample*>(async->context);

            XStoreProductQueryHandle queryHandle = nullptr;

            HRESULT hr = XStoreQueryAssociatedProductsResult(
                async,
                &queryHandle);

            if (FAILED(hr))
            {
                // June 2023 GDK removes entitled account and override requirement for XStore API
                // to work in development, 0x803F6107 for this state should not longer be possible

                ErrorMessage("XStoreQueryAssociatedProductsResult failed : 0x%08X\n", hr);
                pThis->m_isStoreEnumerating = false;
                return;
            }

            hr = XStoreEnumerateProductsQuery(
                queryHandle,
                async->context,
                [](const XStoreProduct* product, void* context) -> bool
                {
                    Sample* pThis = reinterpret_cast<Sample*>(context);

                    debugPrint("[XStoreEnumerateProductsQuery] %s %s\n",
                        product->title, product->hasDigitalDownload ? "hasDigitalDownload" : "no package");

                    if (product->hasDigitalDownload)
                    {
                        ProductInfo productInfo(product);
                        pThis->m_productList.insert_or_assign(productInfo.storeId, productInfo);
                    }

                    // Return true to keep enumerating, false to stop
                    return true;
                }
            );

            if (FAILED(hr))
            {
                ErrorMessage("XStoreEnumerateProductsQuery failed : 0x%08X\n", hr);
                XStoreCloseProductsQueryHandle(queryHandle);
                pThis->m_isStoreEnumerating = false;
                return;
            }

            if (pThis->m_productList.size() == 0)
            {
                debugPrint("No package in store.\n");
            }

            // Build UI for store products.
            for (auto& pair : pThis->m_productList)
            {
                auto& product = pair.second;

                auto storeButton = CastPtr<UIButton>(pThis->m_uiManager.InstantiatePrefab("#store_button_prefab"));
                pThis->m_uiManager.AttachTo(storeButton, pThis->m_uiProductList);

                product.button = storeButton;

                storeButton->GetTypedSubElementById<UIStaticText>(ID("Store_Name"))->SetDisplayText(product.title);
                storeButton->GetTypedSubElementById<UIStaticText>(ID("Store_StoreID"))->SetDisplayText(product.storeId);

                if (pThis->m_packageList.find(product.storeId) != pThis->m_packageList.end())
                {
                    // This package has been installed.
                    storeButton->GetTypedSubElementById<UIImage>(ID("Store_Status"))->SetStyleId(ID("Checked"));

                    pThis->UpdateDownloadProgress(storeButton, 100, 100);
                }
                else
                {
                    storeButton->GetTypedSubElementById<UIImage>(ID("Store_Status"))->SetStyleId(ID("Unchecked"));
                }

                storeButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
                    std::bind(&Sample::OnStoreButtonPressed, pThis, std::placeholders::_1));

                storeButton->ButtonState().AddListenerWhen(UIButton::State::Focused,
                    std::bind(&Sample::OnStoreButtonFocused, pThis, std::placeholders::_1));

                storeButton->ButtonState().AddListenerWhen(UIButton::State::Hovered,
                    std::bind(&Sample::OnStoreButtonHovered, pThis, std::placeholders::_1));
            }

            XStoreCloseProductsQueryHandle(queryHandle);
            pThis->m_isStoreEnumerating = false;

        };

    HRESULT hr = XStoreQueryAssociatedProductsAsync(
        m_storeContext,
        productKinds,
        25,
        async.get());

    if (SUCCEEDED(hr))
    {
        // Ownership of async is transferred to the callback, release it here to avoid memory leak.
        async.release();
    }
    else
    {
        ErrorMessage("XStoreQueryAssociatedProductsResult failed : 0x%08X\n", hr);
        m_isStoreEnumerating = false;
    }
}

void Sample::OnStoreButtonPressed(UIButton* button)
{
    auto storeId = button->GetTypedSubElementById<UIStaticText>(ID("Store_StoreID"))->GetDisplayText();

    auto& product = m_productList[storeId];

    if (product.isInUserCollection)
    {
        if (!product.isBusy)
        {
            UpdateDownloadProgress(product.button, 0, 100);

            product.isBusy = true;
            DownloadStorePackage(product);
        }
    }
    else
    {
        PurchaseStoreProduct(product);
    }
}

void Sample::OnStoreButtonFocused(UIButton* button)
{
    UNREFERENCED_PARAMETER(button);
    m_legendText->SetDisplayText(UIDisplayString("[A] to Purchase or Download DLC, [Y] to Refresh, [VIEW] to Close Sample, [MENU] to Toggle logs."));
}

void Sample::OnStoreButtonHovered(UIButton* button)
{
    UNREFERENCED_PARAMETER(button);
    m_legendText->SetDisplayText(UIDisplayString("[A] to Purchase or Download DLC, [Y] to Refresh, [VIEW] to Close Sample, [MENU] to Toggle logs."));
}

void Sample::PurchaseStoreProduct(ProductInfo& product)
{
    struct Context
    {
        Sample* pThis;
        ProductInfo& product;
    };

    Context* context = new Context{ this, product };

    auto async = std::make_unique<XAsyncBlock>();
    async->queue = m_asyncQueue;
    async->context = context;
    async->callback = [](XAsyncBlock* async)
        {
            std::unique_ptr<XAsyncBlock> asyncPtr{ async };

            auto& [pThis, product] = *reinterpret_cast<Context*>(async->context);

            HRESULT hr = XStoreShowPurchaseUIResult(async);

            if (FAILED(hr))
            {
                ErrorMessage("Failed the purchase : %s 0x%x\n", product.storeId.c_str(), hr);
                delete reinterpret_cast<Context*>(async->context);
                return;
            }

            delete reinterpret_cast<Context*>(async->context);
        };

    HRESULT hr = XStoreShowPurchaseUIAsync(
        m_storeContext,
        product.storeId.c_str(),
        nullptr,    // Can be used to override the title bar text
        nullptr,    // Can be used to provide extra details to purchase
        async.get());

    if (SUCCEEDED(hr))
    {
        // Ownership of async is transferred to the callback, release it here to avoid memory leak.
        async.release();
    }
    else
    {
        ErrorMessage("Failed to purchase : %s 0x%x\n", product.storeId.c_str(), hr);
        delete reinterpret_cast<Context*>(async->context);
    }

}

void Sample::DownloadStorePackage(ProductInfo& product)
{
    const char* storeids[] = {
        product.storeId.c_str()
    };

    struct Context
    {
        Sample* pThis;
        ProductInfo& product;
    };

    Context* context = new Context{ this, product };

    auto async = std::make_unique<XAsyncBlock>();
    async->queue = m_asyncQueue;
    async->context = context;
    async->callback = [](XAsyncBlock* async)
        {
            std::unique_ptr<XAsyncBlock> asyncPtr{ async };

            auto& [pThis, product] = *reinterpret_cast<Context*>(async->context);

            uint32_t count;
            HRESULT hr = XStoreDownloadAndInstallPackagesResultCount(async, &count);

            if (FAILED(hr))
            {
                ErrorMessage("Failed retrieve the installing packages count : 0x%x\n", hr);
                product.isBusy = false;
                pThis->ResetStoreButton(product.button);
                delete reinterpret_cast<Context*>(async->context);
                return;
            }

            if (count == 0)
            {
                ErrorMessage("There is no package that this app can use.\n");
                product.isBusy = false;
                pThis->ResetStoreButton(product.button);
                delete reinterpret_cast<Context*>(async->context);
                return;
            }

            auto packageIdentifiers = new char[count][XPACKAGE_IDENTIFIER_MAX_LENGTH];

            hr = XStoreDownloadAndInstallPackagesResult(
                async,
                count,
                &packageIdentifiers[0]);

            if (FAILED(hr))
            {
                ErrorMessage("Failed retrieve the installing packages results : 0x%x\n", hr);
                product.isBusy = false;
                pThis->ResetStoreButton(product.button);
                delete[] packageIdentifiers;
                delete reinterpret_cast<Context*>(async->context);
                return;
            }

            debugPrint("Start installing, packageIdentifier : %s\n", packageIdentifiers[0]);

            // Start monitor
            pThis->StartInstallMonitor(packageIdentifiers[0], product);

            delete[] packageIdentifiers;
            delete reinterpret_cast<Context*>(async->context);
        };

    HRESULT hr = XStoreDownloadAndInstallPackagesAsync(m_storeContext, storeids, 1, async.get());

    if (SUCCEEDED(hr))
    {
        // Ownership of async is transferred to the callback, release it here to avoid memory leak.
        async.release();
    }
    else
    {
        ErrorMessage("Failed to start download and install : 0x%x\n", hr);
        product.isBusy = false;
        delete reinterpret_cast<Context*>(context);
    }
}

void Sample::StartInstallMonitor(const char* identity, ProductInfo& product)
{
    XPackageInstallationMonitorHandle monitor;

    struct Context
    {
        Sample* pThis;
        ProductInfo& product;
    };

    HRESULT hr = XPackageCreateInstallationMonitor(
        identity,      // Identity to be monitored
        0,             // Number of selectors
        nullptr,       // Selectors
        1000,          // Resolution of the monitor, in milliseconds
        m_asyncQueue,  // Queue where updates are performed
        &monitor);

    if (FAILED(hr))
    {
        ErrorMessage("Failed to create installation monitor : 0x%x\n", hr);
        product.isBusy = false;
        return;
    }

    Context* context = new Context{ this, product };

    XTaskQueueRegistrationToken token;

    hr = XPackageRegisterInstallationProgressChanged(
        monitor,
        context,
        [](void* context, XPackageInstallationMonitorHandle monitor) -> void
        {
            auto& [pThis, product] = *reinterpret_cast<Context*>(context);

            XPackageInstallationProgress progress;
            XPackageGetInstallationProgress(monitor, &progress);

            if (product.button && progress.totalBytes != UINT64_MAX)
            {
                debugPrint("-- progress %d / %d\n", progress.installedBytes, progress.totalBytes);

                pThis->UpdateDownloadProgress(product.button, progress.installedBytes, progress.totalBytes);
            }

            if (progress.completed)
            {
                product.isBusy = false;
                product.button->GetTypedSubElementById<UIImage>(ID("Store_Status"))->SetStyleId(ID("Checked"));
                debugPrint("-- install completed.\n");

                XPackageCloseInstallationMonitorHandle(monitor);
                delete reinterpret_cast<Context*>(context);
            }

        }, &token);

    if (FAILED(hr))
    {
        product.isBusy = false;
        XPackageCloseInstallationMonitorHandle(monitor);
        delete reinterpret_cast<Context*>(context);
    }
}

#pragma endregion

#pragma region DLC Methods
void Sample::RefreshInstalledPackages()
{
    auto [kind, scope] = c_filterOptions[m_twistMenu->GetCurrentItemIndex()];

    m_packageList.clear();

    HRESULT hr = XPackageEnumeratePackages(kind, scope,
        this, [](void* context, const XPackageDetails* details) -> bool
        {
            Sample* pThis = reinterpret_cast<Sample*>(context);

            debugPrint("[XPackageEnumeratePackages] %s %s\n", details->packageIdentifier, details->displayName);

            PackageInfo package(details);
            pThis->m_packageList.insert(std::make_pair(details->storeId, package));

            return true;
        });

    if (FAILED(hr))
    {
        ErrorMessage("XPackageEnumeratePackages failed : 0x%08X\n", hr);
    }

    // Build UI for packages

    m_uiManager.ClearChildren(m_uiPackageList);

    for (auto& pair : m_packageList)
    {
        auto& package = pair.second;

        auto dlcButton = CastPtr<UIButton>(m_uiManager.InstantiatePrefab("#dlc_button_prefab"));
        m_uiManager.AttachTo(dlcButton, m_uiPackageList);

        dlcButton->GetTypedSubElementById<UIStaticText>(ID("DLC_Name"))->SetDisplayText(package.displayName);
        dlcButton->GetTypedSubElementById<UIStaticText>(ID("DLC_StoreID"))->SetDisplayText(package.storeId);

        package.button = dlcButton;

        bool isMounted = false;
        bool isLicensed = false;

        if (m_trackedPackageList.find(package.storeId) != m_trackedPackageList.end())
        {
            auto& trackedPackage = m_trackedPackageList[package.storeId];
            isMounted = trackedPackage.isMounted;
            isLicensed = trackedPackage.isLicensed;
        }

        UpdatePackageStatus(dlcButton, isMounted, isLicensed);

        dlcButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            std::bind(&Sample::OnDlcButtonPressed, this, std::placeholders::_1));

        dlcButton->ButtonState().AddListenerWhen(UIButton::State::Focused,
            std::bind(&Sample::OnDlcButtonFocused, this, std::placeholders::_1));

        dlcButton->ButtonState().AddListenerWhen(UIButton::State::Hovered,
            std::bind(&Sample::OnDlcButtonHovered, this, std::placeholders::_1));
    }
}

void Sample::OnDlcButtonPressed(UIButton* button)
{
    auto storeId = button->GetTypedSubElementById<UIStaticText>(ID("DLC_StoreID"))->GetDisplayText();

    debugPrint("Selected DLC : %s\n", storeId.c_str());

    // This sample checks the license before mounting the package, so it needs store context.
    if (m_storeContext)
    {
        // has tracked
        if (m_trackedPackageList.find(storeId) != m_trackedPackageList.end())
        {
            auto& package = m_trackedPackageList[storeId];

            if (!package.isProcessing)
            {
                package.Unmount();
                m_trackedPackageList.erase(storeId);

                UpdatePackageStatus(m_packageList[storeId].button, false, false);

                SetBackgroundImage("Assets\\Unmounted.png");
            }
        }
        else
        {
            auto& package = m_packageList[storeId];

            TrackedPackage trackedPackage(m_storeContext, m_asyncQueue, package.packageIdentifier, package.storeId);
            MountPackage(trackedPackage);
        }
    }
    else
    {
        ErrorMessage("You need to sign-in before mounting the package.\n");
    }
}

void Sample::OnDlcButtonFocused(UIButton* button)
{
    m_legendText->SetDisplayText(UIDisplayString("[A] to Mount or Unmount Package, [X] to Uninstall, [Y] to Refresh, [LB] + [RB] to Toggle filter, [VIEW] to Close Sample, [MENU] to Toggle logs."));
    m_currentPackageStoreId = button->GetTypedSubElementById<UIStaticText>(ID("DLC_StoreID"))->GetDisplayText();
}

void Sample::OnDlcButtonHovered(UIButton* button)
{
    m_legendText->SetDisplayText(UIDisplayString("[A] to Mount or Unmount Package, [X] to Uninstall, [Y] to Refresh, [LB] + [RB] to Toggle filter, [VIEW] to Close Sample, [MENU] to Toggle logs."));
    m_currentPackageStoreId = button->GetTypedSubElementById<UIStaticText>(ID("DLC_StoreID"))->GetDisplayText();
}

void Sample::MountPackage(TrackedPackage& trackedPackage)
{
    auto [it, inserted] = m_trackedPackageList.try_emplace(
        trackedPackage.storeId,
        m_storeContext,
        m_asyncQueue,
        trackedPackage.packageIdentifier,
        trackedPackage.storeId
    );

    auto& package = it->second;

    package.StartAsyncMountProcess([this, &package](bool success, std::string mountPath)
        {
            if (success)
            {
                UpdatePackageStatus(m_packageList[package.storeId].button, package.isMounted, package.isLicensed);

                package.RegisterPackageEvents([this, &package]()
                    {
                        UpdatePackageStatus(m_packageList[package.storeId].button, package.isMounted, package.isLicensed);
                        RefreshInstalledPackages();
                    });

                // Mounting path
                std::filesystem::path exeFilePath(mountPath);
                exeFilePath /= "AlternateExperience.exe";
                std::filesystem::path dllFilePath(mountPath);
                dllFilePath /= "ComboDll.dll";
                std::filesystem::path jpgFilePath(mountPath);
                jpgFilePath /= "Image.jpg";

                if (std::filesystem::exists(exeFilePath))
                {
                    debugPrint("DLC path : %s\n", exeFilePath.generic_string<char>().c_str());
                    char currentPath[MAX_PATH];
                    GetCurrentDirectoryA(MAX_PATH, currentPath);
                    XLaunchNewGame(exeFilePath.generic_string<char>().c_str(), currentPath, nullptr);
                }
                else if (std::filesystem::exists(dllFilePath))
                {
                    debugPrint("DLC path : %s\n", dllFilePath.generic_string<char>().c_str());
                    ExecuteDLLFunction(dllFilePath.generic_string<char>().c_str());
                }
                else if (std::filesystem::exists(jpgFilePath))
                {
                    debugPrint("DLC path : %s\n", jpgFilePath.generic_string<char>().c_str());
                    SetBackgroundImage(jpgFilePath.generic_string<char>().c_str());
                }
            }
            else
            {
                UpdatePackageStatus(m_packageList[package.storeId].button, false, false);
            }
        });
}

void Sample::UnmountTrackedPackages()
{
    for (auto pair = m_trackedPackageList.begin(); pair != m_trackedPackageList.end();)
    {
        auto& package = pair->second;
        package.Unmount();

        pair = m_trackedPackageList.erase(pair);
    }

    SetBackgroundImage("Assets\\Unmounted.png");
}

#pragma endregion

#pragma region UI Methods
void Sample::InitializeUI()
{
    m_uiManager.GetRootElement()->AddChildFromLayout("Assets/Layouts/layout.json");

    // get references to the other widgets we care about
    m_backgroundImage = m_uiManager.FindTypedById<UIImage>(ID("BackgroundImage"));
    m_errorMessage = m_uiManager.FindTypedById<UIStaticText>(ID("ErrorText"));
    m_legendText = m_uiManager.FindTypedById<UIStaticText>(ID("Legend"));
    m_uiProductList = m_uiManager.FindTypedById<UIVerticalStack>(ID("StoreList"));
    m_uiPackageList = m_uiManager.FindTypedById<UIVerticalStack>(ID("DlcList"));
    m_twistMenu = m_uiManager.FindTypedById<UITwistMenu>(ID("Filter"));
    m_console = m_uiManager.FindTypedById<UIPanel>(ID("ConsolePanel"));

    m_twistMenu->SelectedItemState().AddListener([this](UITwistMenu* twistMenu)
        {
            UNREFERENCED_PARAMETER(twistMenu);
            this->RefreshInstalledPackages();
        });
}

void Sample::ResetStoreButton(std::shared_ptr<UIButton> button)
{
    button->GetTypedSubElementById<UIPanel>(ID("boxProgressPanel"))->
        GetTypedSubElementById<UIStaticText>(ID("progressText"))->
        SetDisplayText(UIDisplayString(""));
}

void Sample::UpdatePackageStatus(std::shared_ptr<UIButton> button, bool isMounted, bool isLicensed)
{
    button->GetTypedSubElementById<UIImage>(ID("DLC_Status"))->
        SetStyleId(ID(isMounted ? "Checked" : "Unchecked"));
    button->GetTypedSubElementById<UIStaticText>(ID("DLC_License"))->
        SetDisplayText(UIDisplayString(isLicensed ? "(Licensed)" : "(No License)"));
}

void Sample::UpdateDownloadProgress(std::shared_ptr<UIButton> button, UINT64 installedBytes, UINT64 totalBytes)
{
    button->GetTypedSubElementById<UIPanel>(ID("boxProgressPanel"))->
        GetTypedSubElementById<UIProgressBar>(ID("boxprogress"))->
        SetProgressPercentage(static_cast<float>
            (static_cast<double>(installedBytes) / static_cast<double>(totalBytes)));

    char temp[256];
    sprintf_s(temp, "%lld %%", installedBytes * 100 / totalBytes);
    button->GetTypedSubElementById<UIPanel>(ID("boxProgressPanel"))->
        GetTypedSubElementById<UIStaticText>(ID("progressText"))->
        SetDisplayText(UIDisplayString(temp));
}

void Sample::SetBackgroundImage(const char* filename)
{
    FileReader reader;
    UINT32 filesize = 0;
    std::unique_ptr<uint8_t[]> buffer;

    if (SUCCEEDED(reader.Open(filename)))
    {
        if (SUCCEEDED(reader.GetSize(&filesize)))
        {
            // Allocate the buffer
            buffer.reset(new uint8_t[filesize]);

            if (SUCCEEDED(reader.Read(buffer.get(), filesize)))
            {
                m_backgroundImage->UseTextureData(buffer.get(), filesize);
            }
        }
    }
    reader.Close();
}

void Sample::ExecuteDLLFunction(const char* filename)
{
    HMODULE hModule = LoadLibraryA(filename);

    if (hModule == nullptr)
    {
        ErrorMessage("Failed to load DLL %s\n", filename);
        return;
    }

    // The function name is a C style name. This is the name that will appear in the DLL's export table
    // after it is built. This is specified within the Dll's header file through the {extern "C"} modifier.
    std::string functionName = "GetDllInfo";

    // Create and obtain a function_pointer to the function defined in our DLL.
    // Both XBOX and Desktop DLLs in this sample define this function with the same signature for simplicity.
    using FunctionPtr = std::add_pointer<void(const char*, size_t, char*)>::type;

    // There is a compiler warning that appears when converting FARPROC_, which represents a function ptr
    // with no args, to FunctionPtr. This is can be avoided by doing the following casts.
    FunctionPtr getDllInfoPtr =
        reinterpret_cast<FunctionPtr>(
            reinterpret_cast<void*>
            (GetProcAddress(hModule, functionName.c_str())));

    if (getDllInfoPtr == nullptr)
    {
        ErrorMessage("Failed to find %s from %s export table!", functionName.c_str(), filename);
        return;
    }

    // Call the example function with a string argument
    std::string executableName = "DownloadableContent.exe";
    char result[256] = {};
    getDllInfoPtr(executableName.c_str(), 256, result);

    char buffer[256] = {};
    sprintf_s(buffer, 256, u8"Called %s:: %s", filename, functionName.c_str());
    debugPrint(std::string(buffer));
    sprintf_s(buffer, 256, u8"-- Result: %s", result);
    debugPrint(std::string(buffer));

    FreeLibrary(hModule);
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

    float elapsedTime = float(timer.GetElapsedSeconds());

    // update our UI input state and managed layout

    m_uiInputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_uiInputState);

    // check if the "view" button was pressed and exit the sample if so

    const auto& buttons = m_uiInputState.GetGamePadButtons(0);

    if (buttons.view == GamePad::ButtonStateTracker::PRESSED)
    {
        ExitSample();
    }

    // ... also exit if the Sample user presses the [ESC] key

    const auto& keys = m_uiInputState.GetKeyboardKeys();

    if (keys.IsKeyPressed(DirectX::Keyboard::Keys::Escape))
    {
        ExitSample();
    }

    // debug console

    if (buttons.menu == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyReleased(Keyboard::Keys::OemTilde))
    {
        m_showConsole = !m_showConsole;
        if (m_console)
        {
            m_console->SetVisible(m_showConsole);
        }
    }

    if (buttons.rightShoulder == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyPressed(DirectX::Keyboard::Keys::PageDown))
    {
        m_twistMenu->IncrementSelectedItem();
    }

    if (buttons.leftShoulder == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyPressed(DirectX::Keyboard::Keys::PageUp))
    {
        m_twistMenu->DecrementSelectedItem();
    }

    if (buttons.x == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyPressed(DirectX::Keyboard::Keys::X))
    {
        if (m_currentPackageStoreId != "")
        {
            auto& packageInfo = m_packageList[m_currentPackageStoreId];
            if (packageInfo.packageIdentifier != "")
            {
                if (XPackageUninstallPackage(packageInfo.packageIdentifier.data()))
                {
                    debugPrint("Package %s uninstalled.\n", packageInfo.packageIdentifier.data());
                }
                else
                {
                    ErrorMessage("XPackageUninstallPackage failed : %s\n", packageInfo.packageIdentifier.data());
                }

                RefreshInstalledPackages();
                RefreshStoreProducts();
                m_currentPackageStoreId = "";
            }
        }
    }

    if (buttons.y == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyPressed(DirectX::Keyboard::Keys::Y))
    {
        RefreshStoreProducts();
        RefreshInstalledPackages();
    }

    // Process any completed tasks
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

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_liveInfoHUD->Render(commandList);

    // Render the UI scene.
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
    m_liveResources->Refresh();

    debugPrint("Resuming, refreshing store context and packages/products...\n");

    // Whenever the title is suspended, the store context can become invalid.
    // Re-create the store context.

    CreateStoreContext();
    RefreshInstalledPackages();
    RefreshStoreProducts();
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
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        Descriptors::Count,
        Descriptors::Texture
    );

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    // Create the style renderer for the UI manager to use for rendering the UI scene styles.
    auto const os = m_deviceResources->GetOutputSize();
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this, 200, os.right, os.bottom);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    m_liveInfoHUD->SetViewport(m_deviceResources->GetScreenViewport());

    // Notify the UI manager of the current window size.
    auto const os = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(os.right, os.bottom);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_liveInfoHUD->ReleaseDevice();
    m_resourceDescriptors.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

