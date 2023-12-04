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

#include <XGameErr.h>

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    Sample* s_sample;

    template <size_t bufferSize = 2048>
    void debugPrint(std::string_view format, ...)
    {
        assert(format.size() < bufferSize && "format string is too large, split up the string or increase the buffer size");

        static char buffer[bufferSize] = "";

        va_list args;
        va_start(args, format);
        vsprintf_s(buffer, format.data(), args);
        va_end(args);

        OutputDebugStringA(buffer);

        if (s_sample)
        {
            auto console = s_sample->GetConsole();
            if (console)
            {
                console->AppendLineOfText(buffer);
            }
        }
    }

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
        XPackageRegisterPackageInstalled(m_asyncQueue, this, [](void *context, const XPackageDetails *package)
            {
                auto pThis = reinterpret_cast<Sample*>(context);
                debugPrint("Package Installed event received: %s\n", package->displayName);
                pThis->RefreshInstalledPackages();

            }, &m_packageInstallToken)
    );
}

Sample::~Sample()
{
    // Unmount all packages that it have been mounted.
    for (auto pair = m_trackedPackageList.begin(); pair != m_trackedPackageList.end();)
    {
        auto package = pair->second;
        UnmountSelectedPackage(package);

        pair = m_trackedPackageList.erase(pair);
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
    s_sample = this;

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

        if (m_storeContext)
        {
            XStoreCloseContextHandle(m_storeContext);
        }
        XStoreCreateContext(user, &m_storeContext);

        if (!XUserIsStoreUser(user))
        {
            ErrorMessage("Current user doesn't match store user.\n");
        }

        this->RefreshInstalledPackages();
        this->RefreshStoreProducts();
    });

    m_liveResources->SetUserSignOutCompletedCallback([this](XalUserHandle /*user*/)
    {
        m_liveInfoHUD->SetUser(nullptr, m_asyncQueue);

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

    auto async = new XAsyncBlock();
    async->queue = m_asyncQueue;
    async->context = this;
    async->callback = [](XAsyncBlock *async)
    {
        Sample* pThis = reinterpret_cast<Sample*>(async->context);

        XStoreProductQueryHandle queryHandle = nullptr;

        HRESULT hr = XStoreQueryAssociatedProductsResult(
            async,
            &queryHandle);

        if (FAILED(hr))
        {
            // June 2023 GDK removes entitled account and override requirement for XStore API
            // to work in development, 0x803F6107 for this state should not longer be possible

            pThis->ErrorMessage("XStoreQueryAssociatedProductsResult failed : 0x%08X\n", hr);
            delete async;
            pThis->m_isStoreEnumerating = false;
            return;
        }

        hr = XStoreEnumerateProductsQuery(
            queryHandle,
            async->context,
            [](const XStoreProduct* product, void* context) -> bool
            {
                Sample* pThis = reinterpret_cast<Sample*>(context);

                if (product->hasDigitalDownload)
                {
                    ProductInfo productInfo(product);

                    debugPrint("[XStoreEnumerateProductsQuery] %s %s\n",
                        product->title, product->hasDigitalDownload ? "hasDigitalDownload" : "no package");

                    pThis->m_productList.insert_or_assign(productInfo.storeId, productInfo);
                }

                // Return true to keep enumerating, false to stop
                return true;
            }
        );

        if (FAILED(hr))
        {
            pThis->ErrorMessage("XStoreEnumerateProductsQuery failed : 0x%08X\n", hr);
            XStoreCloseProductsQueryHandle(queryHandle);
            delete async;
            pThis->m_isStoreEnumerating = false;
            return;
        }

        if (pThis->m_productList.size() == 0)
        {
            debugPrint("No package in store.\n");
        }

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

            storeButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [pThis](UIButton* button)
            {
                auto storeId = button->GetTypedSubElementById<UIStaticText>(ID("Store_StoreID"))->GetDisplayText();

                auto& product = pThis->m_productList[storeId];

                if (product.isInUserCollection)
                {
                    if (!product.isBusy)
                    {
                        pThis->UpdateDownloadProgress(product.button, 0, 100);

                        product.isBusy = true;
                        pThis->DownloadStorePackage(product);
                    }
                }
                else
                {
                    pThis->PurchaseStoreProduct(product);
                }
            });

            storeButton->ButtonState().AddListenerWhen(UIButton::State::Focused, [pThis](UIButton* )
            {
                pThis->m_legendText->SetDisplayText(UIDisplayString("[A] to Purchase or Download DLC, [Y] to Refresh, [VIEW] to Close Sample, [MENU] to Toggle logs."));
            });

            storeButton->ButtonState().AddListenerWhen(UIButton::State::Hovered, [pThis](UIButton*)
            {
                pThis->m_legendText->SetDisplayText(UIDisplayString("[A] to Purchase or Download DLC, [Y] to Refresh, [VIEW] to Close Sample, [MENU] to Toggle logs."));
            });
        }

        XStoreCloseProductsQueryHandle(queryHandle);
        delete async;
        pThis->m_isStoreEnumerating = false;
    };

    HRESULT hr = XStoreQueryAssociatedProductsAsync(
        m_storeContext,
        productKinds,
        25,
        async);

    if (FAILED(hr))
    {
        ErrorMessage("XStoreQueryAssociatedProductsResult failed : 0x%08X\n", hr);
        delete async;
        m_isStoreEnumerating = false;
    }
}

void Sample::PurchaseStoreProduct(ProductInfo& product)
{
    struct Context
    {
        Sample* pThis;
        ProductInfo& product;
    };

    Context *context = new Context{ this, product };

    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->context = context;
    async->callback = [](XAsyncBlock *async)
    {
        auto &[pThis, product] = *reinterpret_cast<Context*>(async->context);

        HRESULT hr = XStoreShowPurchaseUIResult(async);

        if (FAILED(hr))
        {
            pThis->ErrorMessage("Failed the purchase : %s 0x%x\n", product.storeId.c_str(), hr);
            delete reinterpret_cast<Context*>(async->context);
            delete async;
            return;
        }

        delete reinterpret_cast<Context*>(async->context);
        delete async;
    };

    HRESULT hr = XStoreShowPurchaseUIAsync(
        m_storeContext,
        product.storeId.c_str(),
        nullptr,    // Can be used to override the title bar text
        nullptr,    // Can be used to provide extra details to purchase
        async);

    if (FAILED(hr))
    {
        ErrorMessage("Failed to purchase : %s 0x%x\n", product.storeId.c_str(), hr);
        delete reinterpret_cast<Context*>(async->context);
        delete async;
        return;
    }

}

void Sample::DownloadStorePackage(ProductInfo &product)
{
    const char* storeids[] = {
        product.storeId.c_str()
    };

    struct Context
    {
        Sample* pThis;
        ProductInfo& product;
    };

    Context *context = new Context{ this, product };

    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->context = context;
    async->callback = [](XAsyncBlock *async)
    {
        auto &[pThis, product] = *reinterpret_cast<Context*>(async->context);

        uint32_t count;
        HRESULT hr = XStoreDownloadAndInstallPackagesResultCount(async, &count);

        if (FAILED(hr))
        {
            pThis->ErrorMessage("Failed retrieve the installing packages count : 0x%x\n", hr);
            product.isBusy = false;
            pThis->ResetStoreButton(product.button);
            delete reinterpret_cast<Context*>(async->context);
            delete async;
            return;
        }

        if (count == 0)
        {
            pThis->ErrorMessage("There is no package that this app can use.\n");
            product.isBusy = false;
            pThis->ResetStoreButton(product.button);
            delete reinterpret_cast<Context*>(async->context);
            delete async;
            return;
        }

        auto packageIdentifiers = new char[count][XPACKAGE_IDENTIFIER_MAX_LENGTH];

        hr = XStoreDownloadAndInstallPackagesResult(
            async,
            count,
            &packageIdentifiers[0]);

        if (FAILED(hr))
        {
            pThis->ErrorMessage("Failed retrieve the installing packages results : 0x%x\n", hr);
            product.isBusy = false;
            pThis->ResetStoreButton(product.button);
            delete[] packageIdentifiers;
            delete reinterpret_cast<Context*>(async->context);
            delete async;
            return;
        }

        debugPrint("packageIdentifier : %s\n", packageIdentifiers[0]);

        // Start monitor
        pThis->StartInstallMonitor(packageIdentifiers[0], product);

        delete[] packageIdentifiers;
        delete reinterpret_cast<Context*>(async->context);
        delete async;
    };

    HRESULT hr = XStoreDownloadAndInstallPackagesAsync(m_storeContext, storeids, 1, async);

    if (FAILED(hr))
    {
        ErrorMessage("Failed to start download and install : 0x%x\n", hr);
        product.isBusy = false;
        delete reinterpret_cast<Context*>(context);
        delete async;
        return;
    }
}

void Sample::StartInstallMonitor(const char *identity, ProductInfo &product)
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
            auto &[pThis, product] = *reinterpret_cast<Context*>(context);

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
    auto[kind, scope] = c_filterOptions[m_twistMenu->GetCurrentItemIndex()];

    m_packageList.clear();

    HRESULT hr = XPackageEnumeratePackages(kind, scope,
        this, [](void *context, const XPackageDetails *details) -> bool
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
            auto trackedPackage = m_trackedPackageList[package.storeId];
            isMounted = trackedPackage.isMounted;
            isLicensed = trackedPackage.isLicensed;
        }

        UpdatePackageStatus(dlcButton, isMounted,isLicensed);

        dlcButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton* button)
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

                    if (package.progress == EnumMountProgress::Idle)
                    {
                        UnmountSelectedPackage(package);
                        m_trackedPackageList.erase(storeId);

                        UpdatePackageStatus(m_packageList[storeId].button, false, false);

                        SetBackgroundImage("Assets\\Unmounted.png");
                    }
                }
                else
                {
                    auto& package = m_packageList[storeId];

                    TrackedPackage trackedPackage(package.packageIdentifier, package.storeId);
                    MountSelectedPackage(trackedPackage);
                }
            }
            else
            {
                ErrorMessage("You need to sign-in before mounting the package.\n");
            }
       });

        dlcButton->ButtonState().AddListenerWhen(UIButton::State::Focused, [this](UIButton* button)
        {
            m_legendText->SetDisplayText(UIDisplayString("[A] to Mount or Unmount Package, [X] to Uninstall, [Y] to Refresh, [LB] + [RB] to Toggle filter, [VIEW] to Close Sample, [MENU] to Toggle logs."));
            m_currentFocusStoreId = button->GetTypedSubElementById<UIStaticText>(ID("DLC_StoreID"))->GetDisplayText();
        });

        dlcButton->ButtonState().AddListenerWhen(UIButton::State::Hovered, [this](UIButton* button)
        {
            m_legendText->SetDisplayText(UIDisplayString("[A] to Mount or Unmount Package, [X] to Uninstall, [Y] to Refresh, [LB] + [RB] to Toggle filter, [VIEW] to Close Sample, [MENU] to Toggle logs."));
            m_currentFocusStoreId = button->GetTypedSubElementById<UIStaticText>(ID("DLC_StoreID"))->GetDisplayText();
        });
    }
}

HRESULT Sample::AcquireLicense(TrackedPackage& package)
{
   struct Context
    {
        Sample* pThis;
        TrackedPackage& package;
    };

    Context* context = new Context{ this, package };

    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->context = context;
    async->callback = [](XAsyncBlock* async)
    {
        auto& [pThis, package] = *reinterpret_cast<Context*>(async->context);

        HRESULT hr = XStoreAcquireLicenseForPackageResult(async, &package.licenseHandle);

        if (SUCCEEDED(hr))
        {
            package.isLicensed = XStoreIsLicenseValid(package.licenseHandle);
            debugPrint("%s %s\n", package.storeId.data(), package.isLicensed ? "Licensed" : "No license");
            if (package.isLicensed)
            {
                pThis->RegisterPackageEvents(package);
            }

            package.progress = EnumMountProgress::Mount;
            pThis->UpdatePackageStatus(pThis->m_packageList[package.storeId].button, package.isMounted, package.isLicensed);
        }
        else
        {
            if (static_cast<uint32_t>(hr) == 0x87E10BC6) /* LM_E_CONTENT_NOT_IN_CATALOG */
            {
                pThis->ErrorMessage("AcquireLicense failed: %s : LM_E_CONTENT_NOT_IN_CATALOG.\n", package.packageIdentifier.c_str());
            }
            else if (static_cast<uint32_t>(hr) == 0x803F9006) /* LM_E_ENTITLED_USER_SIGNED_OUT */
            {
                pThis->ErrorMessage("AcquireLicense failed: %s : LM_E_ENTITLED_USER_SIGNED_OUT.\n", package.packageIdentifier.c_str());
            }
            else
            {
                pThis->ErrorMessage("AcquireLicense failed: %s : 0x%08X\n", package.packageIdentifier.c_str(), hr);
            }

            package.progress = EnumMountProgress::Idle;
        }

        delete reinterpret_cast<Context*>(async->context);
        delete async;
    };

    package.progress = EnumMountProgress::WaitAcquire;

    HRESULT hr = XStoreAcquireLicenseForPackageAsync(m_storeContext, package.packageIdentifier.c_str(), async);

    if (FAILED(hr))
    {
        ErrorMessage("XStoreAcquireLicenseForPackageAsync failed : 0x%08X\n", hr);
        delete reinterpret_cast<Context*>(async->context);
        delete async;
    }

    return hr;
}

HRESULT Sample::Mount(TrackedPackage& package)
{
    struct Context
    {
        Sample* pThis;
        TrackedPackage& package;
    };

    Context* context = new Context{ this, package };

    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->context = context;
    async->callback = [](XAsyncBlock* async)
    {
        auto& [pThis, package] = *reinterpret_cast<Context*>(async->context);

        HRESULT hr = XPackageMountWithUiResult(async, &package.mountHandle);

        if (SUCCEEDED(hr))
        {
            package.isMounted = true;
            pThis->UpdatePackageStatus(pThis->m_packageList[package.storeId].button, package.isMounted, package.isLicensed);
            package.progress = EnumMountProgress::Use;
        }
        else
        {
            if (hr == E_ACCESSDENIED)
            {
                pThis->ErrorMessage("Mounting failed. Cannot access package : %s : E_ACCESSDENIED\n", package.packageIdentifier.c_str());
            }
            else if (hr == E_GAMEPACKAGE_DLC_NOT_SUPPORTED)
            {
                pThis->ErrorMessage("Mounting failed. This package may target another device : %s : E_GAMEPACKAGE_DLC_NOT_SUPPORTED\n", package.packageIdentifier.c_str());
            }
            else if (hr == E_ABORT)
            {
                pThis->ErrorMessage("Mounting failed. User canceled : %s : E_ABORT.\n", package.packageIdentifier.c_str());
            }
            else if (static_cast<uint32_t>(hr) == 0x87DE2729 /* LM_E_OWNER_NOT_SIGNED_IN */)
            {
                pThis->ErrorMessage("Mounting failed. User has no entitlement : %s", package.packageIdentifier.c_str());
            }
            else
            {
                pThis->ErrorMessage("Mounting failed : %s : 0x%08X\n", package.packageIdentifier.c_str(), hr);
            }

            package.progress = EnumMountProgress::Idle;
        }

        delete reinterpret_cast<Context*>(async->context);
        delete async;
    };

    package.progress = EnumMountProgress::WaitMount;

    HRESULT hr = XPackageMountWithUiAsync(package.packageIdentifier.c_str(), async);

    if (FAILED(hr))
    {
        ErrorMessage("XPackageMountWithUiAsync failed : 0x%08X\n", hr);
        delete reinterpret_cast<Context*>(async->context);
        delete async;
    }

    return hr;
}

HRESULT Sample::UsePackage(TrackedPackage& package)
{
    HRESULT hr = S_OK;

    size_t pathSize;
    hr = XPackageGetMountPathSize(package.mountHandle, &pathSize);

    if (SUCCEEDED(hr))
    {
        char* mountPath = new (std::nothrow) char[pathSize];

        if (mountPath)
        {
            hr = XPackageGetMountPath(package.mountHandle, pathSize, mountPath);

            if (SUCCEEDED(hr))
            {
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

                    // The debugger will disconnect here as the current process is ending and a new one is spawned
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
                else
                {
                    debugPrint("DLC doesn't have a file the sample needs.\n");
                }
            }
            else
            {
                ErrorMessage("XPackageGetMountPath failed : 0x%08X\n", hr);
            }
            delete[] mountPath;
        }
    }
    else
    {
        ErrorMessage("XPackageGetMountPathSize failed : 0x%08X\n", hr);
    }

    package.progress = EnumMountProgress::Idle;

    return hr;
}

void Sample::ProcessMountingPackage()
{
    for (auto& pair : m_trackedPackageList)
    {
        auto& package = pair.second;

        switch (package.progress)
        {
        case EnumMountProgress::Idle:
        case EnumMountProgress::WaitAcquire:
        case EnumMountProgress::WaitMount:
            break;
        case EnumMountProgress::Acquire:
            AcquireLicense(package);
            break;
        case EnumMountProgress::Mount:
            Mount(package);
            break;
        case EnumMountProgress::Use:
            UsePackage(package);
            break;
        }
    }
}

void Sample::MountSelectedPackage(TrackedPackage& package)
{
    // Just kick mounting progress. it works actually in ProcessMountingPackage.
    package.progress = EnumMountProgress::Acquire;

    m_trackedPackageList.insert_or_assign(package.storeId, package);
}

void Sample::UnmountSelectedPackage(TrackedPackage& package)
{
    // Unregister license lost event
    if (package.licenseHandle)
    {
        UnregisterPackageEvents(package);
    }

    // Close the handle to the DLC Package
    if (package.mountHandle)
    {
        XPackageCloseMountHandle(package.mountHandle);
    }

    // Release the license back to the store
    if (package.licenseHandle)
    {
        XStoreCloseLicenseHandle(package.licenseHandle);
    }

    package.isLicensed = false;
    package.isMounted = false;
}

void Sample::RegisterPackageEvents(TrackedPackage& package)
{
    struct Context
    {
        Sample* pThis;
        TrackedPackage& package;
    };

    Context* context = new Context{ this, package };

    HRESULT hr = XStoreRegisterPackageLicenseLost(package.licenseHandle, m_asyncQueue, context, [](void *context)
    {
        auto &[pThis, package] = *reinterpret_cast<Context*>(context);

        debugPrint("Package license lost event received: %s\n", package.storeId.c_str());

        // Unmounting immediately on license terminated is not required and is left to the title
        // to determine the correct time to unmount the package. For example after the current
        // round / map is completed, or immediately if desired. It is recommended to notify the
        // user with in - game UI informing them that access to the DLC has been lost and offer
        // upsell if they continue to try and use it.

        package.isLicensed = false;
        pThis->UnregisterPackageEvents(package);
        pThis->RefreshInstalledPackages();

        delete reinterpret_cast<Context*>(context);

    }, &package.licenseLostEvent);

    if (FAILED(hr))
    {
        delete reinterpret_cast<Context*>(context);
        ErrorMessage("XStoreRegisterPackageLicenseLost failed : 0x%08X\n", hr);
    }
}

void Sample::UnregisterPackageEvents(TrackedPackage& package)
{
    XStoreUnregisterPackageLicenseLost(package.licenseHandle, package.licenseLostEvent, false);
}

void Sample::UninstallPackage(PackageInfo& package)
{
    if (m_trackedPackageList.find(package.storeId) != m_trackedPackageList.end())
    {
        UnmountSelectedPackage(m_trackedPackageList[package.storeId]);
    }

    if (XPackageUninstallPackage(package.packageIdentifier.data()))
    {
        debugPrint("Package %s uninstalled.\n", package.packageIdentifier.data());
    }
    else
    {
        ErrorMessage("XPackageUninstallPackage failed : %s\n", package.packageIdentifier.data());
    }

    RefreshInstalledPackages();
    RefreshStoreProducts();
}

#pragma endregion

#pragma region UI Methods
void Sample::InitializeUI()
{
    m_selectedEnumFilter = EnumFilterType::AllDownloadableContent;

    m_uiManager.GetRootElement()->AddChildFromLayout("Assets/Layouts/layout.json");

    // get references to the other widgets we care about
    m_backgroundImage = m_uiManager.FindTypedById<UIImage>(ID("BackgroundImage"));
    m_errorMessage    = m_uiManager.FindTypedById<UIStaticText>(ID("ErrorText"));
    m_legendText      = m_uiManager.FindTypedById<UIStaticText>(ID("Legend"));
    m_uiProductList   = m_uiManager.FindTypedById<UIVerticalStack>(ID("StoreList"));
    m_uiPackageList   = m_uiManager.FindTypedById<UIVerticalStack>(ID("DlcList"));
    m_twistMenu       = m_uiManager.FindTypedById<UITwistMenu>(ID("Filter"));
    m_console         = m_uiManager.FindTypedById<UIPanel>(ID("ConsolePanel"));

    m_twistMenu->SelectedItemState().AddListener([this](UITwistMenu* twistMenu)
    {
        this->m_selectedEnumFilter = (EnumFilterType)twistMenu->GetCurrentItemIndex();
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
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        ErrorMessage("Failed to open %s\n", filename);
        return;
    }

    // Get file size
    FILE_STANDARD_INFO fileInfo;
    if (GetFileInformationByHandleEx(hFile, FileStandardInfo, &fileInfo, sizeof(fileInfo)) == FALSE)
    {
        ErrorMessage("Failed to get the file size %s\n", filename);
        CloseHandle(hFile);
        return;
    }

    LARGE_INTEGER fileSize = fileInfo.EndOfFile;

    // Allocate the buffer
    std::unique_ptr<uint8_t[]> buffer;
    buffer.reset(new uint8_t[fileSize.LowPart]);

    DWORD bytesRead;
    if (ReadFile(hFile, buffer.get(), fileSize.LowPart, &bytesRead, nullptr) == FALSE)
    {
        ErrorMessage("ReadFile Failed %s\n", filename);
        CloseHandle(hFile);
        return;
    }

    CloseHandle(hFile);

    m_backgroundImage->UseTextureData(buffer.get(), fileSize.LowPart);
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

void Sample::ErrorMessage(std::string_view format, ...)
{
    const size_t bufferSize = 2048;

    assert(format.size() < bufferSize && "format string is too large, split up the string or increase the buffer size");

    static char buffer[bufferSize];

    va_list args;
    va_start(args, format);
    vsprintf_s(buffer, format.data(), args);
    va_end(args);

    m_errorMessage->SetDisplayText(UIDisplayString(buffer));

    debugPrint(buffer);
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

    ProcessMountingPackage();

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
        if (m_currentFocusStoreId != "")
        {
            auto& package = m_packageList[m_currentFocusStoreId];
            UninstallPackage(package);
            m_currentFocusStoreId = "";
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

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        Descriptors::Count,
        Descriptors::Texture
        );

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    // Create the style renderer for the UI manager to use for rendering the UI scene styles.
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
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

