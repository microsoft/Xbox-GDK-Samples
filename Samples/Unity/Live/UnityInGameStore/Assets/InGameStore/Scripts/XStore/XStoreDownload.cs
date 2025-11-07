using System;
using UnityEngine;
using Unity.XGamingRuntime;

namespace GdkSample_InGameStore
{
    /// <summary>
    /// XStoreDownload class for downloading and installing packages.
    /// </summary>
    public sealed class XStoreDownload
    {
        #region DownloadAndInstallPackage
        /// <summary>
        /// Adds a single product (durable with package) to the download queue.
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoredownloadandinstallpackagesasync
        /// </summary>
        /// <param name="storeContext">Store context for the current user.</param>
        /// <param name="storeId">StoreId of the durable with package to download.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public static void DownloadAndInstallPackage(XStoreContext storeContext, string storeId, Action<int, string> completedCallback = null)
        {
            Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreDownloadAndInstallPackagesAsync)}()");

            string[] storeIds = { storeId };

            SDK.XStoreDownloadAndInstallPackagesAsync(
                storeContext,
                storeIds,
                (int hr, string[] packageIdentifiers) =>
                {
                    if (HR.FAILED(hr))
                    {
                        Debug.LogWarning($"{nameof(SDK.XStoreDownloadAndInstallPackagesAsync)} for {storeIds[0]} failed - 0x{hr:X8}.");

                        if (completedCallback != null)
                        {
                            completedCallback?.Invoke(hr, null);
                        }

                        return;
                    }

                    if (HR.SUCCEEDED(hr))
                    {
                        if (packageIdentifiers.Length > 0)
                        {
                            Debug.Log($"{storeId} added to download queue.");

                            if (completedCallback != null)
                            {
                                completedCallback?.Invoke(hr, packageIdentifiers[0]);
                            }
                        }
                    }
                });
        }
        #endregion DownloadAndInstallPackage

    }
}
