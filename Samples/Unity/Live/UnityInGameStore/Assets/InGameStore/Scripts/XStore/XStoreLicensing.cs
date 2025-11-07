using System;
using UnityEngine;
using Unity.XGamingRuntime;

namespace GdkSample_InGameStore
{
    /// <summary>
    /// XStoreLicensing class for acquiring and previewing licenses.
    /// </summary>
    public sealed class XStoreLicensing
    {

        #region AcquireLicense
        /// <summary>
        /// Retrieves the license for a durable add-on by calling XStoreAcquireLicenseForPackageAsync()
        /// if the durable has a package, or XStoreAcquireLicenseForDurablesAsync() if the durable does not have a package.
        /// XStoreAcquireLicense* APIs are considered the 'source of truth' for license verification.
        /// Note: This is required for games using the restrictive licensing policy.
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreacquirelicenseforpackageasync
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreacquirelicensefordurablesasync
        /// </summary>
        /// <param name="storeContext">Store context for the current user.</param>
        /// <param name="storeId">StoreId of the product.</param>
        /// <param name="hasDigitalDownload">True, if the durable has a package.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public static void AcquireLicense(XStoreContext storeContext, string storeId, bool hasDigitalDownload, Action<int, string, XStoreLicense> completedCallback = null)
        {
            if (hasDigitalDownload)
            {
                // Durable has a package, so retrieve the package identifier.
                string packageId = XStoreQueries.QueryPackageIdentifier(storeId);

                if (string.IsNullOrEmpty(packageId))
                {
                    // No package identifier indicates the package has not been installed.
                    if (completedCallback != null)
                    {
                        completedCallback?.Invoke((int)E_XBOX_ERROR_CODES.E_GAMEPACKAGE_NO_PACKAGE_IDENTIFIER, storeId, null);
                    }

                    return;
                }

                Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreAcquireLicenseForPackageAsync)}()");

                // Durable with installed package, call XStoreCanAcquireLicenseForPackageAsync().
                SDK.XStoreAcquireLicenseForPackageAsync(
                    storeContext,
                    packageId,
                    (int hr, XStoreLicense license) =>
                    {
                        if (HR.FAILED(hr))
                        {
                            LogCommonLicensingErrors(hr, storeId);
                            Debug.LogWarning($"{nameof(SDK.XStoreAcquireLicenseForPackageAsync)} for {storeId} failed - 0x{hr:X8}.");
                        }

                        if (completedCallback != null)
                        {
                            completedCallback?.Invoke(hr, storeId, license);
                        }
                    });
            }
            else
            {
                Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreAcquireLicenseForDurablesAsync)}() called for {storeId}");

                // Durable without a package, call XStoreAcquireLicenseForDurablesAsync().
                SDK.XStoreAcquireLicenseForDurablesAsync(
                    storeContext,
                    storeId,
                    (int hr, XStoreLicense license) =>
                    {
                        if (HR.FAILED(hr))
                        {
                            LogCommonLicensingErrors(hr, storeId);
                            Debug.LogWarning($"{nameof(SDK.XStoreAcquireLicenseForDurablesAsync)} for {storeId} failed - 0x{hr:X8}.");
                        }

                        if (completedCallback != null)
                        {
                            completedCallback?.Invoke(hr, storeId, license);
                        }
                    });
            }
        }
        #endregion AcquireLicense

        #region PreviewLicense
        /// <summary>
        /// Retrieves a preview license for a durable add-on by calling XStoreCanAcquireLicenseForPackageAsync()
        /// if the durable has a package, or XStoreCanAcquireLicenseForStoreIdAsync() if the durable does not have a package.
        /// Allows the game to check if a durable is licensable to the current user without acquiring a license.
        /// Note: XStoreCanAcquireLicense* APIs do not work offline, instead call XStoreAcquireLicense* directly 
        /// to check for cached licenses.
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorecanacquirelicenseforpackageasync
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorecanacquirelicenseforstoreidasync
        /// </summary>
        /// <param name="storeContext">Store context for the current user.</param>
        /// <param name="storeId">StoreId of the product.</param>
        /// <param name="hasDigitalDownload">True, if the durable has a package.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public static void PreviewLicense(XStoreContext storeContext, string storeId, bool hasDigitalDownload, Action<int, string, XStoreCanAcquireLicenseResult> completedCallback = null)
        {
            if (hasDigitalDownload)
            {
                // Durable has a package, so retrieve the package identifier.
                string packageId = XStoreQueries.QueryPackageIdentifier(storeId);

                if (string.IsNullOrEmpty(packageId))
                {
                    // No package identifier indicates the package has not been installed.
                    if (completedCallback != null)
                    {
                        completedCallback?.Invoke((int)E_XBOX_ERROR_CODES.E_GAMEPACKAGE_NO_PACKAGE_IDENTIFIER, storeId, null);
                    }

                    return;
                }

                Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreCanAcquireLicenseForPackageAsync)}()");

                // Durable with installed package, call XStoreCanAcquireLicenseForPackageAsync().
                SDK.XStoreCanAcquireLicenseForPackageAsync(
                    storeContext,
                    packageId,
                    (int hr, XStoreCanAcquireLicenseResult result) =>
                    {
                        if (HR.FAILED(hr))
                        {
                            LogCommonLicensingErrors(hr, storeId);
                            Debug.LogWarning($"{nameof(SDK.XStoreCanAcquireLicenseForPackageAsync)} for {storeId} failed - 0x{hr:X8}.");
                        }
                    
                        if (HR.SUCCEEDED(hr))
                        {
                            LogPreviewLicenseStatus(hr, storeId, result);
                        }

                        if (completedCallback != null)
                        {
                            completedCallback?.Invoke(hr, storeId, result);
                        }
                    });
            }
            else
            {          
                Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreCanAcquireLicenseForStoreIdAsync)}()");

                // Durable without a package, call XStoreCanAcquireLicenseForStoreIdAsync().
                SDK.XStoreCanAcquireLicenseForStoreIdAsync(
                    storeContext,
                    storeId,
                    (int hr, XStoreCanAcquireLicenseResult result) =>
                    {
                        if (HR.FAILED(hr))
                        {
                            LogCommonLicensingErrors(hr, storeId);
                            Debug.LogWarning($"{nameof(SDK.XStoreCanAcquireLicenseForStoreIdAsync)} for {storeId} failed - 0x{hr:X8}.");
                        }
                    
                        if (HR.SUCCEEDED(hr))
                        {
                            LogPreviewLicenseStatus(hr, storeId, result);
                        }

                        if (completedCallback != null)
                        {
                            completedCallback?.Invoke(hr, storeId, result);
                        }
                    });
            }
        }
        #endregion PreviewLicense

        #region QueryAddOnLicenses
        /// <summary>
        /// Enumerates available licenses for any durable addons attached to the digital game license.
        /// Does not work for durables with packages.
        /// Results are an indication of which items *might* be licensable to the current user.
        /// To get accurate licensing state, call XStoreCanAcquireLicense* and XStoreAcquireLicense* instead.
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorequeryaddonlicensesasync
        /// </summary>
        /// <param name="storeContext">Store context for the current user.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public static void QueryAddOnLicenses(XStoreContext storeContext, Action<int, XStoreAddonLicense[]> completedCallback = null)
        {
            Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreQueryAddOnLicensesAsync)}()");

            SDK.XStoreQueryAddOnLicensesAsync(
                storeContext,
                (int hr, XStoreAddonLicense[] licenses) =>
                {
                    if (HR.FAILED(hr))
                    {
                        Debug.LogWarning($"{nameof(SDK.XStoreQueryAddOnLicensesAsync)} for failed - 0x{hr:X8}.");
                    }
                
                    if (HR.SUCCEEDED(hr))
                    {
                        Debug.Log($"Number of digital addon licenses = {licenses.Length}");

                        foreach (XStoreAddonLicense license in licenses)
                        {
                            string details = "Addon license sku: " + license.SkuStoreId;
                            details += " is active: " + license.IsActive.ToString();
                            Debug.Log(details);
                        }
                    }

                    if (completedCallback != null)
                    {
                        completedCallback?.Invoke(hr, licenses);
                    }
                });
        }
        #endregion QueryAddOnLicenses

        #region QueryGameLicense
        /// <summary>
        /// Retrieves information about the license acquired for the current game.
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorequerygamelicenseasync
        /// </summary>
        /// <param name="storeContext">Store context for the current user.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public static void QueryGameLicense(XStoreContext storeContext, Action<int, XStoreGameLicense> completedCallback = null)
        {
            Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreQueryGameLicenseAsync)}()");

            SDK.XStoreQueryGameLicenseAsync(
                storeContext,
                (int hr, XStoreGameLicense license) =>
                {
                    if (HR.FAILED(hr))
                    {
                        Debug.LogWarning($"{nameof(SDK.XStoreQueryGameLicenseAsync)} failed - 0x{hr:X8}.");
                    }

                    if (completedCallback != null)
                    {
                        completedCallback?.Invoke(hr, license);
                    }
                });
        }
        #endregion QueryGameLicense

        #region QueryLicenseToken
        /// <summary>
        /// Retrieves a JSON Web Token that can be used to verify license integrity from a custom game service.
        /// This is typically used by PC games, but provided here for demonstration.
        /// https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/commerce/pc-specific-considerations/xstore-license-tokens
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorequerylicensetokenasync
        /// </summary>
        /// <param name="storeContext">Store context for the current user.</param>
        /// <param name="storeId">StoreId of the product.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public static void QueryLicenseToken(XStoreContext storeContext, string storeId, Action<int, string> completedCallback = null)
        {
            Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreQueryLicenseTokenAsync)}()");

            string[] storeIds = { storeId };

            SDK.XStoreQueryLicenseTokenAsync(
                storeContext,
                storeIds,
                "customdeveloperstring-atgingamestore",
                (int hr, string token) =>
                {
                    if (HR.FAILED(hr))
                    {
                        Debug.LogWarning($"{nameof(SDK.XStoreQueryLicenseTokenAsync)} for {storeId} failed - 0x{hr:X8}.");
                    }
                
                    if (HR.SUCCEEDED(hr))
                    {
                        if (!string.IsNullOrEmpty(token))
                        {
                            string tokenSubstring = token.Substring(0, 30);
                            Debug.Log($"License token: {tokenSubstring}...");
                        }
                    }

                    if (completedCallback != null)
                    {
                        completedCallback?.Invoke(hr, token);
                    }
                });
        }
        #endregion  QueryLicenseToken 

        #region Helper Methods

        /// <summary>
        /// Helper method for logging license information retrieved by PreviewLicense().
        /// </summary>
        private static void LogPreviewLicenseStatus(int hr, string storeId, XStoreCanAcquireLicenseResult result)
        {
            if (HR.SUCCEEDED(hr))
            {
                if (result.Status == XStoreCanLicenseStatus.Licensable || result.Status == XStoreCanLicenseStatus.NotLicensableToUser)
                {
                    if (!string.IsNullOrEmpty(result.LicensableSku))
                    {
                        Debug.Log($"License sku for {storeId}: {result.LicensableSku}");
                    }
                
                    string status = $"License status for {storeId}: {result.Status}";

                    if (result.Status == XStoreCanLicenseStatus.Licensable)
                    { 
                        Debug.Log(status); 
                    }
                    else
                    { 
                        Debug.LogWarning(status);
                    }
                }
                else // status == 2
                {
                    Debug.LogWarning($"License status for {storeId}: LicenseActionNotApplicableToProduct");
                }
            }
        }

        /// <summary>
        /// Helper method for logging common errors encountered by PreviewLicense() or AcquireLicense().
        /// </summary>
        private static void LogCommonLicensingErrors(int hr, string storeId)
        {
            if (hr == (int)E_XBOX_ERROR_CODES.E_GAMEPACKAGE_NO_PACKAGE_IDENTIFIER 
                || hr == (int)E_XBOX_ERROR_CODES.IAP_E_BAD_LICENSE || hr == (int)E_XBOX_ERROR_CODES.E_NOTSUPPORTED)
            {
                Debug.LogWarning($"The package for {storeId} is not installed.");
            }
            else if (hr == (int)E_XBOX_ERROR_CODES.LM_E_CONTENT_NOT_IN_CATALOG)
            {
                Debug.LogWarning($"Product {storeId} is not an appropriate durable type.");
            }
            else if (hr == (int)E_XBOX_ERROR_CODES.LM_E_ENTITLED_USER_SIGNED_OUT)
            {
                Debug.LogWarning($"Owner of {storeId} is not signed in.");
            }
        }

        #endregion Helper Methods

    }
}
