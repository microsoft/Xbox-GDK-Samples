//--------------------------------------------------------------------------------------
// File: LicenseManager.cpp
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright(c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "LicenseManager.h"
#include "DebugLog.h"

namespace ATG
{
    // Forward declarations for callbacks defined later in this file
    void CALLBACK AcquireDurableLicenseCallback(XAsyncBlock* async);
    void CALLBACK LicenseLostCallback(void* context);

    _Use_decl_annotations_
    LicenseManager::LicenseManager(XTaskQueueHandle asyncQueue) noexcept(false) :
        m_asyncQueue{},
        m_networkConnectivityChangedToken{},
        m_isNetworkAvailable(false),
        m_LicensedDurables{},
        m_LostLicensesQueue{}        
    {
        HRESULT hr;

        if (asyncQueue)
        {
            XTaskQueueDuplicateHandle(asyncQueue, &m_asyncQueue);
        }
        else
        {
            hr = XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::ThreadPool, &m_asyncQueue);
            DX::ThrowIfFailed(hr);
        }

        hr = XNetworkingRegisterConnectivityHintChanged(
            m_asyncQueue,
            this,
            [](void* context, const XNetworkingConnectivityHint* connectivityHint)
            {
                auto* pThis = static_cast<LicenseManager*>(context);
                pThis->m_isNetworkAvailable = connectivityHint->connectivityLevel == XNetworkingConnectivityLevelHint::InternetAccess;

                ConsoleWriteLine("[LM] IsNetworkAvailable: %s", pThis->IsNetworkAvailable() ? "Yes" : "No");
            },
            &m_networkConnectivityChangedToken);
        DX::ThrowIfFailed(hr);

        XNetworkingConnectivityHint hint{};
        XNetworkingGetConnectivityHint(&hint);
        m_isNetworkAvailable = hint.connectivityLevel == XNetworkingConnectivityLevelHint::InternetAccess;
    }

    LicenseManager::~LicenseManager()
    {
        Shutdown();
    }

    /// <summary>
    /// Attempts to acquire a license for the specified durable product.
    /// https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/system/xstore/functions/xstoreacquirelicensefordurablesasync
    /// https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/system/xstore/functions/xstoreacquirelicenseforpackageasync
    /// </summary>
    /// <param name="storeContext">XStoreContextHandle for the current user.</param>
    /// <param name="storeId">Store ID of the durable product to license.</param>
    /// <param name="hasDigitalDownload">True, if the durable has a package and XStoreAcquireLicenseForPackage should be called.</param>
    void LicenseManager::AcquireDurableLicense(XStoreContextHandle storeContext, const char* storeId, bool hasDigitalDownload)
    {
        const char* apiName = hasDigitalDownload ? "XStoreAcquireLicenseForPackageAsync" : "XStoreAcquireLicenseForDurablesAsync";

        ConsoleWriteLine("[LM] Calling %s for %s", apiName, storeId);

        char packageId[XPACKAGE_IDENTIFIER_MAX_LENGTH] = {};

        if (hasDigitalDownload)
        {
            XStoreQueryPackageIdentifier(storeId, XPACKAGE_IDENTIFIER_MAX_LENGTH, packageId);
        }

        auto async = new XAsyncBlock{};
        async->queue = m_asyncQueue;
        async->context = new LicenseContext{ shared_from_this(), storeContext, storeId, hasDigitalDownload };
        async->callback = AcquireDurableLicenseCallback;

        HRESULT hr = hasDigitalDownload ?
            XStoreAcquireLicenseForPackageAsync(storeContext, packageId, async) :
            XStoreAcquireLicenseForDurablesAsync(storeContext, storeId, async);
        if (FAILED(hr))
        {
            HandleError(apiName, hr);

            if (hr == E_GAMEPACKAGE_NO_PACKAGE_IDENTIFIER)
            {
                ConsoleWriteLine("[LM] Error calling %s: 0x%08x (Check if DLC is installed)", apiName, hr);
            }
            else
            {
                ConsoleWriteLine("[LM] Error calling %s: 0x%08x", apiName, hr);
            }

            delete static_cast<LicenseContext*>(async->context);
            delete async;
            return;
        }
    }

    /// <summary>
    /// Asynchronously acquires licenses for multiple durable products by first checking if they are licensable
    /// and then calling AcquireDurableLicense to acquire a license.
    /// https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/system/xstore/functions/xstorecanacquirelicenseforstoreidasync
    /// https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/system/xstore/functions/xstoreacquirelicensefordurablesasync
    /// </summary>
    /// <param name="storeContext">Handle to the Xbox Store context used for license operations.</param>
    /// <param name="storeIds">Array of store identifiers (durable storeIds) for which to acquire licenses.</param>
    /// <param name="storeIdCount">The number of store identifiers in the storeIds array.</param>
    void LicenseManager::AcquireDurableLicenses(XStoreContextHandle storeContext, const char* const* storeIds, size_t storeIdCount, bool checkPreviewLicense)
    {
        if (!IsNetworkAvailable() || !checkPreviewLicense)
        {
            for (size_t i = 0; i < storeIdCount; ++i)
            {
                // XStoreAcquireLicenseForDurablesAsync can get licenses for both durables with or without packages,
                // so we set hasDigitalDownload to false to make bulk license acquisition easier.
                const char* storeId = storeIds[i];
                AcquireDurableLicense(storeContext, storeId, false);
            }

            return;
        }

        // We can check XStoreCanAcquireLicense* first to avoid potential issues with the license cache.
        // The preview license API calls the service, so it's slower than calling XStoreAcquireLicense* directly (which queries the license cache first).
        for (size_t i = 0; i < storeIdCount; ++i)
        {
            const char* storeId = storeIds[i];
            ConsoleWriteLine("[LM] Calling XStoreCanAcquireLicenseForStoreIdAsync for %s", storeId);

            auto async = new XAsyncBlock{};
            async->queue = m_asyncQueue;
            async->context = new LicenseContext{ shared_from_this(), storeContext, storeId, false };
            async->callback = [](XAsyncBlock* async)
                {
                    auto* ctx = static_cast<LicenseContext*>(async->context);
                    XStoreCanAcquireLicenseResult result;
                    HRESULT hr = XStoreCanAcquireLicenseForStoreIdResult(async, &result);
                    if (SUCCEEDED(hr))
                    {
                        if (result.status == XStoreCanLicenseStatus::Licensable)
                        {
                            ctx->pThis->AcquireDurableLicense(ctx->storeContext, ctx->storeId.c_str(), false);
                        }
                        else
                        {
                            ConsoleWriteLine("[LM] Durable %s is not licensable.", ctx->storeId.c_str());
                        }
                    }
                    else
                    {
                        ConsoleWriteLine("[LM] XStoreCanAcquireLicenseForStoreIdResult failed with error: 0x%08X", hr);
                    }

                    delete ctx;
                    delete async;
                };

            HRESULT hr = XStoreCanAcquireLicenseForStoreIdAsync(storeContext, storeId, async);
            if (FAILED(hr))
            {
                ConsoleWriteLine("[LM] XStoreCanAcquireLicenseForStoreIdAsync for %s failed with error: 0x%08X", storeId, hr);
                delete static_cast<LicenseContext*>(async->context);
                delete async;
                continue; // continue trying to acquire other licenses even if one fails
            }
        }
    }

    /// <summary>
    /// Checks whether the player should be able to acquire a durable license for a specified store product.
    /// Does not acquire a license, but can be used to check license status before attempting acquisition.
    /// When offline, only cached licenses can be queried.
    /// If the license is not cached, XStoreCanAcquireLicenseForStoreId returns error 0x803f8001.
    /// We recommend skipping this call when offline and go straight to calling AcquireDurableLicense instead.
    /// https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/system/xstore/functions/xstorecanacquirelicenseforstoreidasync
    /// https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/system/xstore/functions/xstorecanacquirelicenseforstoreidresult
    /// </summary>
    /// <param name="storeContext">Handle to the store context used for the license query.</param>
    /// <param name="storeId">The store identifier to check for license availability.</param>
    void LicenseManager::PreviewDurableLicense(XStoreContextHandle storeContext, const char* storeId)
    {
        ConsoleWriteLine("[LM] Calling XStoreCanAcquireLicenseForStoreIdAsync for %s", storeId);

        auto async = new XAsyncBlock{};
        async->queue = m_asyncQueue;
        async->context = new LicenseContext{ shared_from_this(), storeContext, storeId, false };
        async->callback = [](XAsyncBlock* async)
            {
                auto* ctx = static_cast<LicenseContext*>(async->context);
                XStoreCanAcquireLicenseResult result;

                HRESULT hr = XStoreCanAcquireLicenseForStoreIdResult(async, &result);
                if (FAILED(hr))
                {
                    ConsoleWriteLine("[LM] Error calling XStoreCanAcquireLicenseForStoreIdResult: 0x%08X", hr);
                    ctx->pThis->HandleError("XStoreCanAcquireLicenseForStoreIdResult", hr);
                }
                else
                {
                    uint32_t statusValue = static_cast<uint32_t>(result.status);

                    if (result.status == XStoreCanLicenseStatus::Licensable)
                    {
                        ConsoleWriteLine("[LM] Durable %s is licensable!", ctx->storeId.c_str());
                    }
                    else if (result.status == XStoreCanLicenseStatus::NotLicensableToUser)
                    {
                        ConsoleWriteLine("[LM] Durable %s is not licensable to current user", ctx->storeId.c_str());
                    }
                    else if (result.status == XStoreCanLicenseStatus::LicenseActionNotApplicableToProduct)
                    {
                        ConsoleWriteLine("[LM] License action not applicable to product %s", ctx->storeId.c_str());
                    }

                    ConsoleWriteLine("[LM] Status: %u LicensableSku: %s", statusValue, result.licensableSku);

                    bool isLicensable = (result.status == XStoreCanLicenseStatus::Licensable);
                    ctx->pThis->OnDurablePreviewResult(
                        ctx->storeId.c_str(),
                        isLicensable,
                        statusValue,
                        result.licensableSku);

                }

                delete static_cast<LicenseContext*>(async->context);
                delete async;
            };

        HRESULT hr = XStoreCanAcquireLicenseForStoreIdAsync(storeContext, storeId, async);
        if (FAILED(hr))
        {
            ConsoleWriteLine("[LM] Error calling XStoreCanAcquireLicenseForStoreIdAsync: 0x%08X", hr);
            HandleError("XStoreCanAcquireLicenseForStoreIdAsync", hr);

            delete static_cast<LicenseContext*>(async->context);
            delete async;
            return;
        }
    }

    /// <summary>
    /// Callback invoked when an asynchronous durable license acquisition operation completes.
    /// Handles the result of acquiring a license for either a package or non-packaged durable, validates the license, and registers for license lost events.
    /// https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/system/xstore/functions/xstoreacquirelicensefordurablesresult
    /// https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/system/xstore/functions/xstoreacquirelicenseforpackageresult
    /// https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/system/xstore/functions/xstoreislicensevalid
    /// https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/system/xstore/functions/xstoreregisterpackagelicenselost
    /// </summary>
    /// <param name="async">Pointer to the XAsyncBlock that represents the completed asynchronous operation.
    /// Contains context data pointing to a LicenseManager::LicenseContext structure with store information.</param>
    void CALLBACK AcquireDurableLicenseCallback(XAsyncBlock* async)
    {
        auto* ctx = static_cast<LicenseManager::LicenseContext*>(async->context);
        XStoreLicenseHandle licenseHandle = {};

        HRESULT hr = (ctx->hasDigitalDownload) ?
            XStoreAcquireLicenseForPackageResult(async, &licenseHandle) :
            XStoreAcquireLicenseForDurablesResult(async, &licenseHandle);
        if (SUCCEEDED(hr) && licenseHandle)
        {
            bool isValid = XStoreIsLicenseValid(licenseHandle);
            ConsoleWriteLine("[LM] License for %s is valid? %s", ctx->storeId.c_str(), isValid ? "Yes" : "No");
            ctx->pThis->OnDurableLicenseAcquired(ctx->storeId.c_str(), isValid);

            if (isValid)
            {
                // Register the new license for LicenseLost events.
                // Note: XStoreRegisterPackageLicenseLost works for durables with and without a package.
                ConsoleWriteLine("[LM] Registering durable %s for license lost events", ctx->storeId.c_str());

                auto licenseContext = new LicenseManager::LicenseContext{ ctx->pThis, ctx->storeContext, ctx->storeId, ctx->hasDigitalDownload };
                XTaskQueueRegistrationToken token = {};
                hr = XStoreRegisterPackageLicenseLost(licenseHandle, async->queue, licenseContext, LicenseLostCallback, &token);
                if (SUCCEEDED(hr))
                {
                    ConsoleWriteLine("[LM] XStoreRegisterPackageLicenseLost succeeded for %s", ctx->storeId.c_str());
                    ctx->pThis->AddLicensedDurable(
                        ctx->storeId.c_str(),
                        LicenseManager::LicensedDurable{ licenseHandle, token, std::unique_ptr<LicenseManager::LicenseContext>(licenseContext) });
                }
                else
                {
                    // If registering for license lost events fails, close the license handle and clean up
                    ConsoleWriteLine("[LM] XStoreRegisterPackageLicenseLost failed for %s with error code: 0x%08x", ctx->storeId.c_str(), hr);
                    ctx->pThis->HandleError("XStoreRegisterPackageLicenseLost", hr);
                    XStoreCloseLicenseHandle(licenseHandle);
                    licenseHandle = nullptr;
                    delete licenseContext;
                }
            }
            else
            {
                // If the license is not valid, close the handle
                XStoreCloseLicenseHandle(licenseHandle);
                licenseHandle = nullptr;
            }
        }
        else
        {
            const char* apiName = ctx->hasDigitalDownload ? "XStoreAcquireLicenseForPackageResult" : "XStoreAcquireLicenseForDurablesResult";
            ConsoleWriteLine("[LM] Error calling %s for storeId %s: 0x%08x", apiName, ctx->storeId.c_str(), hr);
            ctx->pThis->HandleError(apiName, hr);

            if (static_cast<uint32_t>(hr) == 0x87e10bc6 /*LM_E_CONTENT_NOT_IN_CATALOG*/)
            {
                ConsoleWriteLine("[LM] XStoreAcquireLicense only works for base durable types. Bundles and subscriptions are not supported.");
            }

            if (static_cast<uint32_t>(hr) == 0x803f9006 /*LM_E_ENTITLED_USER_SIGNED_OUT*/)
            {
                ConsoleWriteLine("[LM] Account that owns %s is signed out", ctx->storeId.c_str());
            }

            if (licenseHandle)
            {
                XStoreCloseLicenseHandle(licenseHandle);
                licenseHandle = nullptr;
            }
        }

        delete ctx;
        delete async;
    }

    void CALLBACK LicenseLostCallback(void* context)
    {
        auto* ctx = static_cast<LicenseManager::LicenseContext*>(context);
        std::string storeId = ctx->storeId;
        ctx->pThis->EnqueueLostLicense(storeId);
        ConsoleWriteLine("[LM] License lost callback triggered for storeId: %s", storeId.c_str());
    }

    /// <summary>
    /// Should be called regularly on the main thread to process any license lost events and trigger callbacks.
    /// </summary> 
    void LicenseManager::Update()
    {
        std::vector<std::string> lostLicenses;

        // Drain the queue under lock
        {
            std::lock_guard<std::mutex> lock(m_licenseMutex);

            // Process lost licenses queue on main thread
            while (!m_LostLicensesQueue.empty())
            {
                lostLicenses.push_back(m_LostLicensesQueue.front());
                m_LostLicensesQueue.pop();
            }
        } // lock is automatically released when exiting scope

        for (const auto& storeId : lostLicenses)
        {
            CleanupLicense(storeId.c_str());
            OnDurableLicenseLost(storeId.c_str());
        }
    }

    ///<summary>
    /// Clean up all licenses and resources. Should be called on title shutdown to avoid leaks.
    /// </summary>
    void LicenseManager::Shutdown()
    {
        auto toUnregister = std::vector<LicensedDurable>{};

        {
            std::lock_guard<std::mutex> lock(m_licenseMutex);
            for (auto& pair : m_LicensedDurables)
            {
                toUnregister.push_back({ pair.second.licenseHandle, pair.second.callbackToken, std::move(pair.second.callbackContext) });
            }

            m_LicensedDurables.clear();
            while (!m_LostLicensesQueue.empty())
            {
                m_LostLicensesQueue.pop();
            }
        }

        for (auto& entry : toUnregister)
        {
            if (entry.licenseHandle && entry.callbackToken.token != 0)
            {
                XStoreUnregisterPackageLicenseLost(
                    entry.licenseHandle,
                    entry.callbackToken,
                    true);
            }

            if (entry.licenseHandle)
            {
                XStoreCloseLicenseHandle(entry.licenseHandle);
                entry.licenseHandle = nullptr;
            }
        }

        XNetworkingUnregisterConnectivityHintChanged(m_networkConnectivityChangedToken, true);

        if (m_asyncQueue)
        {
            // Drain all pending callbacks before closing the handle.
            // Uses the non-blocking terminate + dispatch loop to avoid deadlock
            // if Shutdown is called from a thread that also dispatches completions.
            bool terminated = false;
            XTaskQueueTerminate(m_asyncQueue, false, &terminated, [](void* context)
                {
                    *static_cast<bool*>(context) = true;
                });

            while (!terminated)
            {
                XTaskQueueDispatch(m_asyncQueue, XTaskQueuePort::Work, 5);
                XTaskQueueDispatch(m_asyncQueue, XTaskQueuePort::Completion, 5);
            }

            XTaskQueueCloseHandle(m_asyncQueue);
            m_asyncQueue = nullptr;
        }
    }

    void LicenseManager::AddLicensedDurable(const char* storeId, LicensedDurable durable)
    {
        CleanupLicense(storeId);

        // Insert the new license entry if no concurrent callback has already inserted one for the same storeId.
        // If an entry already exists, it means a license lost event was triggered and the existing entry is being tracked for cleanup,
        // so we should not insert the new one.
        {
            std::lock_guard<std::mutex> lock(m_licenseMutex);
            auto current = m_LicensedDurables.find(storeId);
            if (current == m_LicensedDurables.end())
            {
                m_LicensedDurables[storeId] = std::move(durable);
                return;
            }
        } // lock is automatically released when exiting scope

        // A concurrent callback already inserted an entry for this storeId.
        // The entry in the map continues tracking license lost events.
        // This duplicate registration is redundant and must be cleaned up,
        // leaving it alive would result in a leaked LicenseContext and a
        // dangling context pointer in the XStore runtime when LicenseLostCallback fires.
        if (durable.licenseHandle && durable.callbackToken.token != 0)
        {
            XStoreUnregisterPackageLicenseLost(durable.licenseHandle, durable.callbackToken, true);
        }
        if (durable.licenseHandle)
        {
            XStoreCloseLicenseHandle(durable.licenseHandle);
        }
        // unique_ptr callbackContext destructs safely here, after unregister has completed
    }

    void LicenseManager::CleanupLicense(const char* storeId)
    {
        LicensedDurable entry{};
        bool found = false;

        {
            std::lock_guard<std::mutex> lock(m_licenseMutex);
            auto it = m_LicensedDurables.find(storeId);
            if (it != m_LicensedDurables.end())
            {
                entry = std::move(it->second);
                m_LicensedDurables.erase(it);
                found = true;
            }
        } // release lock before blocking unregister call

        if (found)
        {
            if (entry.licenseHandle && entry.callbackToken.token != 0)
            {
                XStoreUnregisterPackageLicenseLost(entry.licenseHandle, entry.callbackToken, true);
            }

            if (entry.licenseHandle)
            {
                XStoreCloseLicenseHandle(entry.licenseHandle);
            }
        }
    }

    void LicenseManager::HandleError(const char* apiName, HRESULT hr)
    {
        std::function<void(const char*, HRESULT)> cb;
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            cb = m_errorHandler;
        }
        if (cb) cb(apiName, hr);
    }

    void LicenseManager::OnDurableLicenseAcquired(const char* storeId, bool isLicenseValid)
    {
        std::function<void(const char*, bool)> cb;
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            cb = m_onDurableLicenseAcquiredCallback;
        }
        if (cb) cb(storeId, isLicenseValid);
    }

    void LicenseManager::OnDurableLicenseLost(const char* storeId)
    {
        std::function<void(const char*)> cb;
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            cb = m_onDurableLicenseLostCallback;
        }
        if (cb) cb(storeId);
    }

    void LicenseManager::OnDurablePreviewResult(const char* storeId, bool isLicensable, uint32_t status, const char* licensableSku)
    {
        std::function<void(const char*, bool, uint32_t, const char*)> cb;
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            cb = m_onDurablePreviewResultCallback;
        }
        if (cb) cb(storeId, isLicensable, status, licensableSku);
    }
}
