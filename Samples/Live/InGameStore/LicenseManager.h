//--------------------------------------------------------------------------------------
// File: LicenseManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <XStore.h>
#include <XTaskQueue.h>
#include <XAsync.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>

namespace ATG
{
    class LicenseManager : public std::enable_shared_from_this<LicenseManager>
    {
    public:
        struct LicenseContext
        {
            std::shared_ptr<LicenseManager> pThis;
            XStoreContextHandle storeContext = nullptr;
            std::string storeId;
            bool hasDigitalDownload = false;
        };

        struct LicensedDurable
        {
            XStoreLicenseHandle licenseHandle = nullptr;
            XTaskQueueRegistrationToken callbackToken = {};
            std::unique_ptr<LicenseContext> callbackContext;
        };

        // Constructor
        explicit LicenseManager(_In_opt_ XTaskQueueHandle asyncQueue = nullptr) noexcept(false);

        LicenseManager(LicenseManager&&) = default;
        LicenseManager& operator= (LicenseManager&&) = default;
        LicenseManager(const LicenseManager&) = delete;
        LicenseManager& operator= (const LicenseManager&) = delete;

        ~LicenseManager();

        void AcquireDurableLicense(XStoreContextHandle storeContext, const char* storeId, bool hasDigitalDownload = false);
        void AcquireDurableLicenses(XStoreContextHandle storeContext, const char* const* storeIds, size_t storeIdCount, bool checkPreviewLicense = true);
        void PreviewDurableLicense(XStoreContextHandle storeContext, const char* storeId);
        void Update();
        void Shutdown();

        void SetDurableLicenseAcquiredCallback(std::function<void(const char*, bool)> callback)
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_onDurableLicenseAcquiredCallback = std::move(callback);
        }
        void SetDurableLicenseLostCallback(std::function<void(const char*)> callback)
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_onDurableLicenseLostCallback = std::move(callback);
        }
        void SetDurablePreviewResultCallback(std::function<void(const char*, bool, uint32_t, const char*)> callback)
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_onDurablePreviewResultCallback = std::move(callback);
        }
        void SetErrorHandler(std::function<void(const char*, HRESULT)> callback)
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_errorHandler = std::move(callback);
        }

        bool IsNetworkAvailable() const { return m_isNetworkAvailable; }

    private:
        friend void CALLBACK AcquireDurableLicenseCallback(XAsyncBlock* async);
        friend void CALLBACK LicenseLostCallback(void* context);

        // Event handlers
        void HandleError(const char* apiName, HRESULT hr);
        void OnDurableLicenseAcquired(const char* storeId, bool isLicenseValid);
        void OnDurableLicenseLost(const char* storeId);
        void OnDurablePreviewResult(const char* storeId, bool isLicensable, uint32_t status, const char* licensableSku);

        // Methods used by async licensing callbacks
        void AddLicensedDurable(const char* storeId, LicensedDurable durable);
        void CleanupLicense(const char* storeId);
        void EnqueueLostLicense(const std::string& storeId)
        {
            std::lock_guard<std::mutex> lock(m_licenseMutex);
            m_LostLicensesQueue.push(storeId);
        }

        XTaskQueueHandle                                                m_asyncQueue;
        XTaskQueueRegistrationToken                                     m_networkConnectivityChangedToken;
        std::atomic<bool>                                               m_isNetworkAvailable;

        // Callbacks
        std::function<void(const char*, HRESULT)>                       m_errorHandler;
        std::function<void(const char*, bool)>                          m_onDurableLicenseAcquiredCallback;
        std::function<void(const char*)>                                m_onDurableLicenseLostCallback;
        std::function<void(const char*, bool, uint32_t, const char*)>   m_onDurablePreviewResultCallback;

        // Thread safety
        std::mutex                                                      m_callbackMutex;
        std::mutex                                                      m_licenseMutex;

        // License storage
        std::unordered_map<std::string, LicensedDurable>                m_LicensedDurables;
        std::queue<std::string>                                         m_LostLicensesQueue;
    };
}
