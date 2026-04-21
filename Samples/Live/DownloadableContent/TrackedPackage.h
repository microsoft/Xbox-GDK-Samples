//--------------------------------------------------------------------------------------
// TrackedPackage.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <atomic>

class TrackedPackage
{
public:

    TrackedPackage() :
        m_storeContext{ nullptr },
        m_asyncQueue{ nullptr },
        m_licenseHandle{ nullptr },
        m_licenseLostEvent{ 0 },
        m_mountHandle{ nullptr },
        m_licenseLostCallback{ nullptr },
        m_completeMountCallback{ nullptr },
        packageIdentifier{ "" },
        storeId{ "" },
        isProcessing{ false },
        isMounted{ false },
        isLicensed{ false }
    {
    };

    TrackedPackage(XStoreContextHandle storeContext, XTaskQueueHandle asyncQueue, std::string packageidentifier, std::string storeid) :
        m_storeContext{ storeContext },
        m_asyncQueue{ asyncQueue },
        m_licenseHandle{ nullptr },
        m_licenseLostEvent{ 0 },
        m_mountHandle{ nullptr },
        m_licenseLostCallback{ nullptr },
        m_completeMountCallback{ nullptr },
        packageIdentifier{ packageidentifier },
        storeId{ storeid },
        isProcessing{ false },
        isMounted{ false },
        isLicensed{ false }
    {
    };

    TrackedPackage(const TrackedPackage&) = delete;
    TrackedPackage(TrackedPackage&&) = delete;
    TrackedPackage& operator=(const TrackedPackage&) = delete;
    TrackedPackage& operator=(TrackedPackage&&) = delete;

private:
    XStoreContextHandle         m_storeContext;
    XTaskQueueHandle            m_asyncQueue;

    XStoreLicenseHandle         m_licenseHandle;
    XTaskQueueRegistrationToken m_licenseLostEvent;
    XPackageMountHandle         m_mountHandle;

    using LicenseLostCallback = std::function<void(void)>;
    using CompleteMountCallback = std::function<void(bool success, std::string mountPath)>;

    LicenseLostCallback         m_licenseLostCallback;
    CompleteMountCallback       m_completeMountCallback;

    HRESULT AcquireLicense();
    HRESULT Mount();

public:
    std::string packageIdentifier;
    std::string storeId;

    std::atomic<bool> isProcessing;
    std::atomic<bool> isMounted;
    std::atomic<bool> isLicensed;

    // Callbacks for UI updates and event handling
    using CompleteMountCallback = std::function<void(bool success, std::string mountPath)>;

    HRESULT StartAsyncMountProcess(CompleteMountCallback callback);

    void Unmount();
    void RegisterPackageEvents(LicenseLostCallback callback);
    void UnregisterPackageEvents();
};
