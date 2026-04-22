using System;
using System.Collections.Generic;
using Unity.XGamingRuntime;
using UnityEngine;

namespace GdkSample_InGameStore
{
    public sealed class XStoreLicenseManager : MonoBehaviour
    {
        // Collection of durable add-ons to always check licensing for.
        // Licenses can be acquired without an online connection, but only if the StoreId of the product is already known.
        // If running as a different title, replace with StoreIds for durables relevant to your title.
        public readonly List<string> DurablesToLicense = new()
        {
            "9N30KZZF4BR9", // DWOB1
            "9P23V43P0XZZ", // DWOB2
            "9P8S15PJTB0P", // DWOB3
            "9PLRFWZWWF91", // DWOB4
            "9NC3H6CSGCPK", // DWOB5
            "9PLNMXRKNM4C"  // DurableContent1 (durable with package)
        };

        public static XStoreLicenseManager Instance { get; private set; }

        /// <summary>
        /// Signals when a durable license has been acquired.
        /// Returns the storeId for the newly acquired durable.
        /// </summary>
        public event Action<string> DurableLicenseAcquired;

        /// <summary>
        /// Signals when a durable license has been lost.
        /// Returns the storeId for the previously licensed durable.
        /// </summary>
        public event Action<string> DurableLicenseLost;

        /// <summary>
        /// Contains all durables with an active license.
        /// Persists between user sign-in/sign-out events.
        /// </summary>
        public Dictionary<string, Tuple<XStoreLicense, PackageLicenseLostCallbackToken>> LicensedDurables;

        /// <summary>
        /// Tracks license lost events for durables that have acquired a valid license.
        /// Persists between user sign-in/sign-out events.
        /// Processed on the main thread in Update() to avoid calling XStoreUnregisterPackageLicenseLost from a time-sensitive thread.
        /// </summary>
        public Queue<string> LostLicensesQueue;

        private void Awake()
        {
            InitializeOrDestroyInstance(this);
        }

        private static bool InitializeOrDestroyInstance(XStoreLicenseManager newInstance)
        {
            if (Instance != newInstance && Instance != null)
            {
                Debug.LogWarning($"An instance of {newInstance.GetType().Name} already exist. Destroying {newInstance}...");
                Destroy(newInstance.gameObject);
                return false;
            }

            Instance = newInstance;
            DontDestroyOnLoad(Instance.gameObject);
            Instance.LicensedDurables = new();
            Instance.LostLicensesQueue = new();
            return true;
        }

        private void Update()
        {
            // Process license lost events on the main thread.
            // After the license is lost, XStoreUnregisterPackageLicenseLost cannot be called from a time-sensitive thread.
            while (LostLicensesQueue.Count > 0)
            {
                string storeId = LostLicensesQueue.Dequeue();

                if (!string.IsNullOrEmpty(storeId))
                {
                    Debug.Log($"Processing license lost event for {storeId} on main thread.");

                    CleanupLicense(storeId);

                    // Invoke the event so other systems can respond.
                    DurableLicenseLost?.Invoke(storeId);

                    // Attempt to reacquire the lost license.
                    Debug.Log($"Attempting to reacquire the license for {storeId}.");

                    if (XStoreManager.Instance.AllProducts.ContainsKey(storeId) || DurablesToLicense.Contains(storeId))
                    {
                        AcquireDurableLicense(storeId);
                    }
                    else
                    {
                        Debug.LogWarning($"Cannot reacquire license for {storeId}. Product not found in AllProducts or DurablesToLicense.");
                    }
                }
            }
        }

        /// <summary>
        /// Attempts to acquire licenses for all durables in the 'DurablesToLicense' collection.
        /// </summary>
        public void AcquireDurableLicenses()
        {
            Debug.Log($"{nameof(XStoreLicenseManager)}.{nameof(AcquireDurableLicenses)}() called to acquire licenses for all durables in the hardcoded list.");

            foreach (var durable in DurablesToLicense)
            {
                // Network connection is required by XStoreCanAcquireLicense, but XStoreAcquireLicense can succeed without a connection if the license is cached on the device.
                if (XNetworkManager.Instance.IsNetworkAvailable)
                {
                    XStoreLicensing.PreviewLicense(
                    XStoreManager.Instance.StoreContext,
                    durable,
                    false,
                    (int hr, string storeId, XStoreCanAcquireLicenseResult result) =>
                    {
                        if (HR.SUCCEEDED(hr))
                        {
                            if (result.LicensableSku != null && result.Status == XStoreCanLicenseStatus.Licensable)
                            {
                                Logger.Instance.Log($"Durable {storeId} is licensable.", LogColor.Success);
                                AcquireDurableLicense(storeId);
                            }
                            else
                            {
                                Debug.Log($"Durable {storeId} is not licensable. Status: {result.Status}");
                            }
                        }
                        else
                        {
                            Debug.LogWarning($"Failed to preview license for durable {storeId} - 0x{hr:X8}");
                        }
                    });
                }
                else
                {
                    Debug.Log($"Network not available. Checking license cache for {durable}.");
                    AcquireDurableLicense(durable);
                }
            }
        }

        /// <summary>
        /// Attempts to acquire a license for the specified durable StoreId.
        /// </summary>
        /// <param name="storeId">The StoreId of the durable for which to acquire a license.</param>
        /// <param name="hasDigitalDownload">Whether the durable has a digital download associated with it. This is only relevant for durables that are also packages, such as 'DurableContent1' in this sample.</param>
        public void AcquireDurableLicense(string storeId, bool hasDigitalDownload = false)
        {
            XStoreLicensing.AcquireLicense(
                XStoreManager.Instance.StoreContext,
                storeId,
                hasDigitalDownload,
                (int hr, string acquiredStoreId, XStoreLicense license) =>
                {
                    if (HR.SUCCEEDED(hr) && license != null)
                    {
                        // Check if the license is valid.
                        bool isValid = SDK.XStoreIsLicenseValid(license);
                        string licenseState = $"License for {storeId} is valid: {isValid}";

                        if (isValid)
                        {
                            Logger.Instance.Log($"{licenseState}", LogColor.Success);

                            // If a license has previously been acquired for this StoreId, unregister the old license before registering the new one.
                            CleanupLicense(storeId);

                            // Register the durable license for license lost events.
                            Logger.Instance.Log($"Registering durable {storeId} for license lost events.", LogColor.Event);

                            // Register the new license for LicenseLost events.
                            // Note: XStoreRegisterPackageLicenseLost works for durables with and without a package.
                            // https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreregisterpackagelicenselost
                            hr = SDK.XStoreRegisterPackageLicenseLost(
                                license,
                                (IntPtr context) =>
                                {
                                    // Queue the license lost event to be processed on the main thread
                                    // XStoreUnregisterPackageLicenseLost cannot be called from a time-sensitive thread
                                    Logger.Instance.Log($"License lost callback triggered for durable {storeId}. Queuing for main thread processing.", LogColor.Event);
                                    
                                    if (!LostLicensesQueue.Contains(storeId))
                                    {
                                        LostLicensesQueue.Enqueue(storeId);
                                    }
                                },
                                out PackageLicenseLostCallbackToken token);

                            if (HR.SUCCEEDED(hr))
                            {
                                Debug.Log($"Successfully registered durable {storeId} for license lost events.");
                                if (!LicensedDurables.ContainsKey(storeId))
                                {
                                    LicensedDurables.Add(storeId, new Tuple<XStoreLicense, PackageLicenseLostCallbackToken>(license, token));
                                }
                                else
                                {
                                    LicensedDurables[storeId] = new Tuple<XStoreLicense, PackageLicenseLostCallbackToken>(license, token);
                                }

                                DurableLicenseAcquired?.Invoke(storeId);
                            }
                            else
                            {
                                Debug.Log($"Failed to register durable {storeId} for license lost events.");
                                Debug.Log($"{nameof(SDK.XStoreRegisterPackageLicenseLost)} for {storeId} returned: 0x{hr:X8}");
                                SDK.XStoreCloseLicenseHandle(license);
                            }
                        }
                        else
                        {
                            Debug.LogWarning(licenseState);
                            SDK.XStoreCloseLicenseHandle(license);
                        }
                    }
                    else
                    {
                        Logger.Instance.Log($"Failed to acquire license for durable {acquiredStoreId} - 0x{hr:X8}", LogColor.Warning);
                        if (license != null)
                        {
                            SDK.XStoreCloseLicenseHandle(license);
                        }
                    }
                });
        }

        public void CleanupLicenses()
        {
            foreach (var durable in LicensedDurables)
            {
                SDK.XStoreUnregisterPackageLicenseLost(durable.Value.Item1, durable.Value.Item2);
                SDK.XStoreCloseLicenseHandle(durable.Value.Item1);
            }

            LicensedDurables.Clear();
        }

        private void CleanupLicense(string storeId)
        {
            if (LicensedDurables.ContainsKey(storeId))
            {
                Debug.Log($"Cleaning up previous license for durable {storeId}.");
                var durable = LicensedDurables[storeId];

                // Remove from dictionary FIRST to prevent re-entry
                LicensedDurables.Remove(storeId);

                // Check if the license handle is valid before unregistering
                if (durable.Item1 != null)
                {
                    SDK.XStoreUnregisterPackageLicenseLost(durable.Item1, durable.Item2);
                    SDK.XStoreCloseLicenseHandle(durable.Item1);
                }
            }
        }

        private void OnDestroy()
        {
            if (LicensedDurables != null)
            {
                CleanupLicenses();
            }

            Instance = null;
        }
    }
}
