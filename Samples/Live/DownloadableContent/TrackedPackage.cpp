//--------------------------------------------------------------------------------------
// TrackedPackage.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DownloadableContent.h"
#include "TrackedPackage.h"

// It is no longer required to acquire a license before mounting since June 2024 GDK.
// The license check is performed within the Mount API.
// If the license is lost while playing the game, you will no longer be able to access
// the files within the package. If you do not use the license acquisition API,
// you will not be able to receive a license loss event, so you will only receive
// an error when you first access the files within the package.
// Since error handling becomes more complex, it is recommended to use the license API
// and license loss events if you need to access files within the package.


//#define BYPASS_LICENSE_ACQUISITION_API

HRESULT TrackedPackage::StartAsyncMountProcess(CompleteMountCallback callback)
{
    if (isProcessing)
    {
        // Already in progress.
        return E_FAIL;
    }

    m_completeMountCallback = callback;
    isProcessing = true;

#ifdef BYPASS_LICENSE_ACQUISITION_API
    return Mount();
#else
    return AcquireLicense();
#endif
}

HRESULT TrackedPackage::AcquireLicense()
{
    auto async = std::make_unique<XAsyncBlock>();
    async->queue = m_asyncQueue;
    async->context = this;
    async->callback = [](XAsyncBlock* async)
        {
            std::unique_ptr<XAsyncBlock> asyncPtr{ async };
            auto& package = *reinterpret_cast<TrackedPackage*>(async->context);

            HRESULT hr = XStoreAcquireLicenseForPackageResult(async, &package.m_licenseHandle);

            if (SUCCEEDED(hr))
            {
                package.isLicensed = XStoreIsLicenseValid(package.m_licenseHandle);
                debugPrint("%s %s\n", package.storeId.data(), package.isLicensed ? "Licensed" : "No license");

                if (package.isLicensed)
                {
                    package.Mount();
                }
                else
                {
                    package.isProcessing = false;
                    if (package.m_completeMountCallback)
                    {
                        package.m_completeMountCallback(false, "");
                    }
                }
            }
            else
            {
                if (static_cast<uint32_t>(hr) == 0x87E10BC6) /* LM_E_CONTENT_NOT_IN_CATALOG */
                {
                    ErrorMessage("AcquireLicense failed: %s : LM_E_CONTENT_NOT_IN_CATALOG.\n", package.packageIdentifier.c_str());
                }
                else if (static_cast<uint32_t>(hr) == 0x803F9006) /* LM_E_ENTITLED_USER_SIGNED_OUT */
                {
                    ErrorMessage("AcquireLicense failed: %s : LM_E_ENTITLED_USER_SIGNED_OUT.\n", package.packageIdentifier.c_str());
                }
                else
                {
                    ErrorMessage("AcquireLicense failed: %s : 0x%08X\n", package.packageIdentifier.c_str(), hr);
                }

                package.isProcessing = false;
                if (package.m_completeMountCallback)
                {
                    package.m_completeMountCallback(false, "");
                }
            }
        };

    HRESULT hr = XStoreAcquireLicenseForPackageAsync(m_storeContext, packageIdentifier.c_str(), async.get());

    if (FAILED(hr))
    {
        ErrorMessage("XStoreAcquireLicenseForPackageAsync failed : 0x%08X\n", hr);
        isProcessing = false;
    }
    else
    {
        async.release();
    }

    return hr;
}

HRESULT TrackedPackage::Mount()
{
    auto async = std::make_unique<XAsyncBlock>();
    async->queue = m_asyncQueue;
    async->context = this;
    async->callback = [](XAsyncBlock* async)
        {
            std::unique_ptr<XAsyncBlock> asyncPtr{ async };
            auto& package = *reinterpret_cast<TrackedPackage*>(async->context);

            HRESULT hr = XPackageMountWithUiResult(async, &package.m_mountHandle);

            if (SUCCEEDED(hr))
            {
                package.isMounted = true;
                std::string mountPath = "";

                size_t pathSize;
                hr = XPackageGetMountPathSize(package.m_mountHandle, &pathSize);

                if (SUCCEEDED(hr))
                {
                    char* mountPathBuffer = new (std::nothrow) char[pathSize];

                    if (mountPathBuffer)
                    {
                        hr = XPackageGetMountPath(package.m_mountHandle, pathSize, mountPathBuffer);
                        if (SUCCEEDED(hr))
                        {
                            mountPath = mountPathBuffer;
                        }
                        delete[] mountPathBuffer;
                    }
                    else
                    {
                        ErrorMessage("XPackageGetMountPath failed : 0x%08X\n", hr);
                    }
                }
                else
                {
                    ErrorMessage("XPackageGetMountPathSize failed : 0x%08X\n", hr);
                }

#ifdef BYPASS_LICENSE_ACQUISITION_API
                // Mount API performs license check internally, so if we are here, we can assume the package is licensed.
                package.isLicensed = true;
#endif
                package.isProcessing = false;
                if (package.m_completeMountCallback)
                {
                    package.m_completeMountCallback(!mountPath.empty(), mountPath);
                }
            }
            else
            {
                if (hr == E_ACCESSDENIED)
                {
                    ErrorMessage("Mounting failed. Cannot access package : %s : E_ACCESSDENIED\n", package.packageIdentifier.c_str());
                }
                else if (hr == E_GAMEPACKAGE_DLC_NOT_SUPPORTED)
                {
                    ErrorMessage("Mounting failed. This package may target another device : %s : E_GAMEPACKAGE_DLC_NOT_SUPPORTED\n", package.packageIdentifier.c_str());
                }
                else if (hr == E_ABORT)
                {
                    ErrorMessage("Mounting failed. User canceled : %s : E_ABORT.\n", package.packageIdentifier.c_str());
                }
                else if (static_cast<uint32_t>(hr) == 0x87DE2729 /* LM_E_OWNER_NOT_SIGNED_IN */)
                {
                    ErrorMessage("Mounting failed. User has no entitlement : %s", package.packageIdentifier.c_str());
                }
                else
                {
                    ErrorMessage("Mounting failed : %s : 0x%08X\n", package.packageIdentifier.c_str(), hr);
                }

                package.isProcessing = false;
                if (package.m_completeMountCallback)
                {
                    package.m_completeMountCallback(false, "");
                }
            }
        };

    HRESULT hr = XPackageMountWithUiAsync(packageIdentifier.c_str(), async.get());

    if (FAILED(hr))
    {
        ErrorMessage("XPackageMountWithUiAsync failed : 0x%08X\n", hr);
        isProcessing = false;
    }
    else
    {
        async.release();
    }

    return hr;
}

void TrackedPackage::Unmount()
{
#ifndef BYPASS_LICENSE_ACQUISITION_API
    // Unregister license lost event
    if (m_licenseHandle)
    {
        UnregisterPackageEvents();
    }
#endif

    // Close the handle to the DLC Package
    if (m_mountHandle)
    {
        XPackageCloseMountHandle(m_mountHandle);
        m_mountHandle = nullptr;
    }

    // Release the license back to the store
    if (m_licenseHandle)
    {
        XStoreCloseLicenseHandle(m_licenseHandle);
        m_licenseHandle = nullptr;
    }

    isProcessing = false;
    isLicensed = false;
    isMounted = false;
}

void TrackedPackage::RegisterPackageEvents(LicenseLostCallback callback)
{
    m_licenseLostCallback = callback;

#ifdef BYPASS_LICENSE_ACQUISITION_API
    // Skip registering license lost event.
    return;
#endif

    HRESULT hr = XStoreRegisterPackageLicenseLost(m_licenseHandle, m_asyncQueue, this, [](void* context)
        {
            auto& package = *reinterpret_cast<TrackedPackage*>(context);

            debugPrint("Package license lost event received: %s\n", package.storeId.c_str());

            // If you lose the license, you will no longer be able to access the files in the package.
            // If you need access again, you will need to mount it again.

            package.Unmount();
            
            if (package.m_licenseLostCallback)
            {
                package.m_licenseLostCallback();
            }

        }, &m_licenseLostEvent);

    if (FAILED(hr))
    {
        ErrorMessage("XStoreRegisterPackageLicenseLost failed : 0x%08X\n", hr);
    }
}

void TrackedPackage::UnregisterPackageEvents()
{
    XStoreUnregisterPackageLicenseLost(m_licenseHandle, m_licenseLostEvent, false);
}
