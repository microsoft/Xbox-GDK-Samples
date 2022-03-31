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

extern void ExitSample();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    template <size_t bufferSize = 2048>
    void debugPrint(std::string_view format, ...)
    {
        assert(format.size() < bufferSize && "format string is too large, split up the string or increase the buffer size");

        static char buffer[bufferSize];

        va_list args;
        va_start(args, format);
        vsprintf_s(buffer, format.data(), args);
        va_end(args);

        OutputDebugStringA(buffer);
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
    m_storeContext(nullptr),
    m_asyncQueue(nullptr),
    m_packageInstallToken{},
    m_isStoreEnumerating(false),
    m_needSetFocus(EnumFocusArea::None)
{
    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_asyncQueue)
    );

    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
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

        if (m_storeContext)
        {
            XStoreCloseContextHandle(m_storeContext);
        }
        XStoreCreateContext(user, &m_storeContext);

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
        if (error == E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED)
        {
            m_liveResources->SignInWithUI();
        }
        else if (error == E_GAMEUSER_NO_PACKAGE_IDENTITY)
        {
            ErrorMessage("LiveResource error : E_GAMEUSER_NO_PACKAGE_IDENTITY\n");
        }
        else if (error == E_GAMEUSER_NO_DEFAULT_USER)
        {
            ErrorMessage("LiveResource error : E_GAMEUSER_NO_DEFAULT_USER\n");
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

    m_storeDetailList.clear();
    m_uiManager.ClearChildren(m_storeList);

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

        if (static_cast<unsigned int>(hr) == 0x803F6107) /* IAP_E_UNEXPECTED */
        {
            pThis->ErrorMessage("Config is invalid (Sandbox / ContentIdOverride / EKBIDOverride) : 0x%08X\n", hr);
            delete async;
            pThis->m_isStoreEnumerating = false;
            return;
        }
        else if (FAILED(hr))
        {
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

                // BUG 30095480 : Currently hasDigitalDownload doesn't work.
                //if (product->hasDigitalDownload)
                {
                    StoreProductDetails storeProduct(product);

                    debugPrint("[XStoreEnumerateProductsQuery] %s %s\n",
                        product->title, product->hasDigitalDownload ? "hasDigitalDownload" : "no package");

                    pThis->m_storeDetailList.emplace_back(storeProduct);
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

        if (pThis->m_storeDetailList.size() == 0)
        {
            pThis->ErrorMessage("No package in store.\n");
        }

        for (StoreProductDetails &storePackage : pThis->m_storeDetailList)
        {
            auto storeButton = CastPtr<UIButton>(pThis->m_uiManager.InstantiatePrefab("#store_button_prefab"));
            pThis->m_uiManager.AttachTo(storeButton, pThis->m_storeList);

            storePackage.button = storeButton;

            storeButton->GetTypedSubElementById<UIStaticText>(ID("Store_Name"))->SetDisplayText(storePackage.title);
            storeButton->GetTypedSubElementById<UIStaticText>(ID("Store_StoreID"))->SetDisplayText(storePackage.storeId);

            auto installedPackage = pThis->GetPackageDetail(storePackage.storeId);
            if (installedPackage)
            {
                // This package has been installed.
                storeButton->GetTypedSubElementById<UIImage>(ID("Store_Status"))->SetStyleId(ID("Checked"));
                storeButton->GetTypedSubElementById<UIPanel>(ID("boxProgressPanel"))->
                    GetTypedSubElementById<UIProgressBar>(ID("boxprogress"))->
                    SetProgressPercentage(1.0f);
            }
            else
            {
                storeButton->GetTypedSubElementById<UIImage>(ID("Store_Status"))->SetStyleId(ID("Unchecked"));
            }

            storeButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [pThis](UIButton* button)
            {
                pThis->m_lastSelectStoreId = button->GetTypedSubElementById<UIStaticText>(ID("Store_StoreID"))->GetDisplayText();

                StoreProductDetails* package = pThis->GetStoreProductDetail(pThis->m_lastSelectStoreId);

                if (package->isInUserCollection)
                {
                    if (!package->isBusy)
                    {
                        package->isBusy = true;
                        pThis->DownloadStorePackage(*package);
                    }
                }
                else
                {
                    pThis->PurchaseStoreProduct(*package);
                }

                pThis->m_needSetFocus = EnumFocusArea::StoreProducts;
            });

            storeButton->ButtonState().AddListenerWhen(UIButton::State::Focused, [pThis](UIButton* )
            {
                pThis->m_legendText->SetDisplayText(UIDisplayString("[A] to Purchase or Download DLC, [Y] to Refresh, [VIEW] to Close Sample"));
            });

        }

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

StoreProductDetails* Sample::GetStoreProductDetail(const std::string &storeId)
{
    for (size_t index = 0; index < m_storeDetailList.size(); ++index)
    {
        if (m_storeDetailList[index].storeId == storeId.data())
        {
            return &m_storeDetailList[index];
        }
    }

    return nullptr;
}

void Sample::PurchaseStoreProduct(StoreProductDetails &package)
{
    struct Context
    {
        Sample *pThis;
        StoreProductDetails &package;
    };

    Context *context = new Context{ this, package };

    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->context = context;
    async->callback = [](XAsyncBlock *async)
    {
        auto &[pThis, package] = *reinterpret_cast<Context*>(async->context);

        HRESULT hr = XStoreShowPurchaseUIResult(async);

        if (FAILED(hr))
        {
            pThis->ErrorMessage("Failed the purchase : 0x%x\n", hr);
            delete reinterpret_cast<Context*>(async->context);
            delete async;
            return;
        }

        pThis->RefreshStoreProducts();
        delete reinterpret_cast<Context*>(async->context);
        delete async;
    };

    HRESULT hr = XStoreShowPurchaseUIAsync(
        m_storeContext,
        package.storeId.c_str(),
        nullptr,    // Can be used to override the title bar text
        nullptr,    // Can be used to provide extra details to purchase
        async);

    if (FAILED(hr))
    {
        ErrorMessage("Failed to purchase: 0x%x\n", hr);
        delete reinterpret_cast<Context*>(async->context);
        delete async;
        return;
    }

}

void Sample::DownloadStorePackage(StoreProductDetails &package)
{
    const char* storeids[] = {
        package.storeId.c_str()
    };

    struct Context
    {
        Sample *pThis;
        StoreProductDetails &package;
    };

    Context *context = new Context{ this, package };

    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->context = context;
    async->callback = [](XAsyncBlock *async)
    {
        auto &[pThis, package] = *reinterpret_cast<Context*>(async->context);

        uint32_t count;
        HRESULT hr = XStoreDownloadAndInstallPackagesResultCount(async, &count);

        if (FAILED(hr))
        {
            pThis->ErrorMessage("Failed retrieve the installing packages count : 0x%x\n", hr);
            package.isBusy = false;
            delete reinterpret_cast<Context*>(async->context);
            delete async;
            return;
        }

        if (count == 0)
        {
            pThis->ErrorMessage("There is no package that this app can use.\n");
            package.isBusy = false;
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
            package.isBusy = false;
            delete[] packageIdentifiers;
            delete reinterpret_cast<Context*>(async->context);
            delete async;
            return;
        }

        debugPrint("packageIdentifier : %s\n", packageIdentifiers[0]);

        // Start monitor
        pThis->StartInstallMonitor(packageIdentifiers[0], package);

        delete[] packageIdentifiers;
        delete reinterpret_cast<Context*>(async->context);
        delete async;
    };

    HRESULT hr = XStoreDownloadAndInstallPackagesAsync(m_storeContext, storeids, 1, async);

    if (FAILED(hr))
    {
        ErrorMessage("Failed to start download and install : 0x%x\n", hr);
        package.isBusy = false;
        delete reinterpret_cast<Context*>(context);
        delete async;
        return;
    }
}

void Sample::StartInstallMonitor(const char *identity, StoreProductDetails &package)
{
    XPackageInstallationMonitorHandle monitor;

    struct Context
    {
        Sample *pThis;
        StoreProductDetails &package;
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
        package.isBusy = false;
        return;
    }

    Context *context = new Context{ this, package };

    XTaskQueueRegistrationToken token;

    hr = XPackageRegisterInstallationProgressChanged(
        monitor,
        context,
        [](void* context, XPackageInstallationMonitorHandle monitor) -> void
        {
            auto &[pThis, package] = *reinterpret_cast<Context*>(context);

            XPackageInstallationProgress progress;
            XPackageGetInstallationProgress(monitor, &progress);

            if (package.button && progress.totalBytes != UINT64_MAX)
            {
                debugPrint("-- progress %d / %d\n", progress.installedBytes, progress.totalBytes);

                package.button->GetTypedSubElementById<UIPanel>(ID("boxProgressPanel"))->
                    GetTypedSubElementById<UIProgressBar>(ID("boxprogress"))->
                    SetProgressPercentage(static_cast<float>
                        (static_cast<double>(progress.installedBytes) / static_cast<double>(progress.totalBytes)));
            }

            if (progress.completed)
            {
                package.isBusy = false;
                package.button->GetTypedSubElementById<UIImage>(ID("Store_Status"))->SetStyleId(ID("Checked"));
                debugPrint("-- install completed.\n");

                XPackageCloseInstallationMonitorHandle(monitor);
                delete reinterpret_cast<Context*>(context);
            }

        }, &token);

    if (FAILED(hr))
    {
        package.isBusy = false;
        XPackageCloseInstallationMonitorHandle(monitor);
        delete reinterpret_cast<Context*>(context);
    }
}

#pragma endregion

#pragma region DLC Methods
void Sample::RefreshInstalledPackages()
{
    m_dlcDetailList.clear();
    m_uiManager.ClearChildren(m_dlcList);

    auto[kind, scope] = c_filterOptions[m_twistMenu->GetCurrentItemIndex()];

    HRESULT hr = XPackageEnumeratePackages(kind, scope,
        this, [](void *context, const XPackageDetails *details) -> bool
        {
            Sample* pThis = reinterpret_cast<Sample*>(context);

            debugPrint("[XPackageEnumeratePackages] %s %s\n", details->packageIdentifier, details->displayName);

            PackageDetails package(details);

            auto mountInfo = pThis->GetPackageMountInfo(package.storeId);

            if (mountInfo)
            {
                package.isMounted = true;

                if (XStoreIsLicenseValid(mountInfo->packageLicense))
                {
                    package.isLicense = true;
                }
            }

            pThis->m_dlcDetailList.emplace_back(package);

            return true;
        });

    if (FAILED(hr))
    {
        ErrorMessage("XPackageEnumeratePackages failed : 0x%08X\n", hr);
    }

    for (auto& dlc : m_dlcDetailList)
    {
        auto dlcButton = CastPtr<UIButton>(m_uiManager.InstantiatePrefab("#dlc_button_prefab"));
        m_uiManager.AttachTo(dlcButton, m_dlcList);

        dlc.button = dlcButton;

        dlcButton->GetTypedSubElementById<UIStaticText>(ID("DLC_Name"))->SetDisplayText(dlc.displayName);
        dlcButton->GetTypedSubElementById<UIStaticText>(ID("DLC_StoreID"))->SetDisplayText(dlc.storeId);

        if (dlc.isMounted)
        {
            dlcButton->GetTypedSubElementById<UIImage>(ID("DLC_Status"))->SetStyleId(ID("Checked"));
        }
        else
        {
            dlcButton->GetTypedSubElementById<UIImage>(ID("DLC_Status"))->SetStyleId(ID("Unchecked"));
        }

        if (dlc.isLicense)
        {
            dlcButton->GetTypedSubElementById<UIStaticText>(ID("DLC_License"))->SetDisplayText(UIDisplayString("(Licensed)"));
        }
        else
        {
            dlcButton->GetTypedSubElementById<UIStaticText>(ID("DLC_License"))->SetDisplayText(UIDisplayString("(No License)"));
        }

        dlcButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton* button)
        {
            m_lastSelectStoreId = button->GetTypedSubElementById<UIStaticText>(ID("DLC_StoreID"))->GetDisplayText();

            debugPrint("Selected DLC : %s\n", m_lastSelectStoreId.data());

            PackageDetails* package = GetPackageDetail(m_lastSelectStoreId);

            if (package && !package->isBusy)
            {
                package->isBusy = true;

                if (!package->isMounted)
                {
                    // This sample checks the license before mounting the package, so it needs store context.
                    if (m_storeContext)
                    {
                        MountSelectedPackage(*package);
                    }
                    else
                    {
                        ErrorMessage("You need to sign-in before mounting the package.\n");
                        package->isBusy = false;
                    }
                }
                else
                {
                    UnmountSelectedPackage(*package);
                    button->GetTypedSubElementById<UIImage>(ID("DLC_Status"))->SetStyleId(ID("Unchecked"));
                }
            }
        });

        dlcButton->ButtonState().AddListenerWhen(UIButton::State::Focused, [this](UIButton* )
        {
            m_legendText->SetDisplayText(UIDisplayString("[A] to Mount or Unmount Package, [Y] to Refresh, [LB] + [RB] to Toggle filter, [VIEW] to Close Sample"));
        });
    }
}

PackageDetails* Sample::GetPackageDetail(const std::string &storeId)
{
    for (size_t index = 0; index < m_dlcDetailList.size(); ++index)
    {
        if (m_dlcDetailList[index].storeId == storeId.data())
        {
            return &m_dlcDetailList[index];
        }
    }

    return nullptr;
}

void Sample::MountSelectedPackage(PackageDetails &package)
{
    struct Context
    {
        Sample *pThis;
        PackageDetails *package;
    };

    auto async = new XAsyncBlock{};
    async->queue = m_asyncQueue;
    async->context = new Context{ this, &package };
    async->callback = [](XAsyncBlock *async)
    {
        auto &[pThis, package] = *reinterpret_cast<Context*>(async->context);

        XStoreLicenseHandle license = {};
        auto result = XStoreAcquireLicenseForPackageResult(async, &license);

        if (SUCCEEDED(result))
        {
            bool isLicense = XStoreIsLicenseValid(license);

            debugPrint("%s %s\n", package->displayName.data(), isLicense ? "Licensed" : "No license");

            if (isLicense)
            {
                PackageEventContext *context = new PackageEventContext{ pThis, package->storeId };

                auto token = pThis->RegisterPackageEvents(license, context);

                XPackageMountHandle mountHandle = {};
                result = XPackageMount(package->packageIdentifier.data(), &mountHandle);

                if (SUCCEEDED(result))
                {
                    package->isMounted = true;
                    package->button->GetTypedSubElementById<UIImage>(ID("DLC_Status"))->SetStyleId(ID("Checked"));

                    pThis->AddNewMountedPackage(package->storeId, license, token, mountHandle, context);
                }
                else if (result == E_ACCESSDENIED)
                {
                    XStoreCloseLicenseHandle(license);
                    pThis->ErrorMessage("Mounting failed. Cannot mount an app (%s): E_ACCESSDENIED\n", package->displayName.data());
                }
                else if (result == E_GAMEPACKAGE_DLC_NOT_SUPPORTED)
                {
                    XStoreCloseLicenseHandle(license);
                    pThis->ErrorMessage("Mounting failed. This package may target another device %s: 0x%08X\n", package->displayName.data(), result);
                }
                else
                {
                    XStoreCloseLicenseHandle(license);
                    pThis->ErrorMessage("Error mounting package %s: 0x%08X\n", package->displayName.data(), result);
                }
            }
            else
            {
                pThis->ErrorMessage("You don't have a license for %s.\n", package->displayName.data());
            }
            pThis->RefreshInstalledPackages();
            pThis->m_needSetFocus = EnumFocusArea::InstalledPackages;
        }
        else if (static_cast<unsigned int>(result) == 0x87E10BC6) /* LM_E_CONTENT_NOT_IN_CATALOG */
        {
            pThis->ErrorMessage("XStoreAcquireLicenseForPackageResult failed: %s : LM_E_CONTENT_NOT_IN_CATALOG.\n", package->displayName.data());
        }
        else
        {
            pThis->ErrorMessage("XStoreAcquireLicenseForPackageResult failed: %s : 0x%08X\n", package->displayName.data(), result);
        }

        package->isBusy = false;

        delete reinterpret_cast<Context*>(async->context);
        delete async;
    };

    if (FAILED(XStoreAcquireLicenseForPackageAsync(m_storeContext, package.packageIdentifier.data(), async)))
    {
        package.isBusy = false;

        delete reinterpret_cast<Context*>(async->context);
        delete async;
    }
}

void Sample::UnmountSelectedPackage(PackageDetails &package)
{
    auto mountInfo = m_mountedPackageList.find(package.storeId);

    if (mountInfo != m_mountedPackageList.end())
    {
        auto &[license, licenseLostEvent, mountHandle, context] = mountInfo->second;

        // If it has license lost event.
        if (context)
        {
            UnregisterPackageEvents(license, licenseLostEvent);
            delete context;
        }

        // Close the handle to the DLC Package
        XPackageCloseMountHandle(mountHandle);

        // Release the license back to the store
        XStoreCloseLicenseHandle(license);

        m_mountedPackageList.erase(mountInfo);

        RefreshInstalledPackages();
        m_needSetFocus = EnumFocusArea::InstalledPackages;
    }

    package.isBusy = false;
}

XTaskQueueRegistrationToken Sample::RegisterPackageEvents(XStoreLicenseHandle license, PackageEventContext *context)
{
    XTaskQueueRegistrationToken token = {};
    HRESULT hr = XStoreRegisterPackageLicenseLost(license, m_asyncQueue, context, [](void *context)
    {
        auto &[pThis, storeId] = *reinterpret_cast<PackageEventContext*>(context);

        debugPrint("Package license lost event received: %s\n", storeId.c_str());

        auto mountInfo = pThis->GetPackageMountInfo(storeId);
            
        if (mountInfo)
        {
            // Unmounting immediately on license terminated is not required and is left to the title
            // to determine the correct time to unmount the package. For example after the current
            // round / map is completed, or immediately if desired. It is recommended to notify the
            // user with in - game UI informing them that access to the DLC has been lost and offer
            // upsell if they continue to try and use it.

            auto package = pThis->GetPackageDetail(storeId);
            if (package)
            {
                package->isLicense = false;
            }
        }

        pThis->RefreshInstalledPackages();
        delete reinterpret_cast<PackageEventContext*>(context);

        pThis->UnregisterPackageEvents(mountInfo->packageLicense, mountInfo->licenseLostEvent);
        mountInfo->context = nullptr;

    }, &token);

    if (FAILED(hr))
    {
        delete reinterpret_cast<PackageEventContext*>(context);
        ErrorMessage("XStoreRegisterPackageLicenseLost failed : 0x%08X\n", hr);
    }

    return token;
}

void Sample::UnregisterPackageEvents(XStoreLicenseHandle license, XTaskQueueRegistrationToken licenseLostEvent)
{
    XStoreUnregisterPackageLicenseLost(license, licenseLostEvent, false);
}

void Sample::AddNewMountedPackage(std::string &storeId, XStoreLicenseHandle license, XTaskQueueRegistrationToken token, XPackageMountHandle mountHandle, PackageEventContext *context)
{
    PackageMountInfo package { license, token, mountHandle, context };
    m_mountedPackageList.emplace(storeId, package);

    const char filename[] = "\\Image.jpg";

    size_t pathSize;
    XPackageGetMountPathSize(mountHandle, &pathSize);

    pathSize += sizeof(filename);
    char* path = new (std::nothrow) char[pathSize];

    XPackageGetMountPath(mountHandle, pathSize, path);

    strcat_s(path, pathSize, filename);

    debugPrint("DLC path : %s\n", path);

    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        ErrorMessage("Failed to open %s\n", path);
        delete[] path;
        return;
    }

    // Get file size
    FILE_STANDARD_INFO fileInfo;
    if (GetFileInformationByHandleEx(hFile, FileStandardInfo, &fileInfo, sizeof(fileInfo)) == FALSE)
    {
        ErrorMessage("Failed to get the file size %s\n", path);
        delete[] path;
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
        ErrorMessage("ReadFile Failed %s\n", path);
        delete[] path;
        CloseHandle(hFile);
        return;
    }

    delete[] path;
    CloseHandle(hFile);

    m_backgroundImage->UseTextureData(buffer.get(), fileSize.LowPart);
}

PackageMountInfo* Sample::GetPackageMountInfo(const std::string &storeId)
{
    auto package = m_mountedPackageList.find(storeId);

    if (package != m_mountedPackageList.end())
    {
        return &package->second;
    }

    return nullptr;
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
    m_storeList       = m_uiManager.FindTypedById<UIVerticalStack>(ID("StoreList"));
    m_dlcList         = m_uiManager.FindTypedById<UIVerticalStack>(ID("DlcList"));
    m_twistMenu       = m_uiManager.FindTypedById<UITwistMenu>(ID("Filter"));

    m_twistMenu->SelectedItemState().AddListener([this](UITwistMenu* twistMenu)
    {
        this->m_selectedEnumFilter = (EnumFilterType)twistMenu->GetCurrentItemIndex();
        this->RefreshInstalledPackages();
    });
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

    OutputDebugStringA(buffer);
}

#pragma endregion

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

    // update our UI input state and managed layout

    m_uiInputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_uiInputState);

    // check if the "view" button was pressed and exit the sample if so

    auto buttons = m_uiInputState.GetGamePadButtons(0);

    if (buttons.view == GamePad::ButtonStateTracker::PRESSED)
    {
        ExitSample();
    }

    // ... also exit if the Sample user presses the [ESC] key

    auto keys = m_uiInputState.GetKeyboardKeys();

    if (keys.IsKeyPressed(DirectX::Keyboard::Keys::Escape))
    {
        ExitSample();
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

    if (buttons.y == GamePad::ButtonStateTracker::PRESSED ||
        keys.IsKeyPressed(DirectX::Keyboard::Keys::Y))
    {
        RefreshStoreProducts();
        RefreshInstalledPackages();
    }

    // Set focus to last pressed button. This needs to call outside button press event.
    if (m_needSetFocus == EnumFocusArea::InstalledPackages)
    {
        for (size_t index = 0; index < m_dlcList->GetChildCount(); ++index)
        {
            auto element = m_dlcList->GetChildByIndex(index);

            if (element->IsFocusable() && m_lastSelectStoreId == element->GetTypedSubElementById<UIStaticText>(ID("DLC_StoreID"))->GetDisplayText())
            {
                m_uiManager.SetFocus(element);
                m_needSetFocus = EnumFocusArea::None;
                break;
            }
        }
    }
    else if (m_needSetFocus == EnumFocusArea::StoreProducts)
    {
        for (size_t index = 0; index < m_storeList->GetChildCount(); ++index)
        {
            auto element = m_storeList->GetChildByIndex(index);

            if (element->IsFocusable() && m_lastSelectStoreId == element->GetTypedSubElementById<UIStaticText>(ID("Store_StoreID"))->GetDisplayText())
            {
                m_uiManager.SetFocus(element);
                m_needSetFocus = EnumFocusArea::None;
                break;
            }
        }
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
    auto os = m_deviceResources->GetOutputSize();
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

