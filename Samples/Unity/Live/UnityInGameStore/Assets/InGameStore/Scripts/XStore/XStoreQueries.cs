using System;
using System.Collections.Generic;
using UnityEngine;
using Unity.XGamingRuntime;

namespace GdkSample_InGameStore
{
    /// <summary>
    /// XStoreQueries class for querying store product information.
    /// </summary>
    public sealed class XStoreQueries
    {
        const uint MAXPAGECOUNT = 25;

        #region QueryAssociatedProducts
        /// <summary>
        /// Gets store listing information for all products that are available for purchase within the current game (catalog).
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorequeryassociatedproductsasync
        /// </summary>
        /// <param name="storeContext">Store context for the current user.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public static void QueryAssociatedProducts(XStoreContext storeContext, Action<int, List<XStoreProduct>> completedCallback = null)
        {
            Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreQueryAssociatedProductsAsync)}()");

            List<XStoreProduct> products = new();
            XStoreProductKind productKinds = XStoreProductKind.Consumable | XStoreProductKind.Durable | XStoreProductKind.Game | XStoreProductKind.Pass | XStoreProductKind.UnmanagedConsumable;
            XStoreQueryComplete NextPageCallback = null;

            SDK.XStoreQueryAssociatedProductsAsync(
                storeContext,
                productKinds,
                MAXPAGECOUNT,
                NextPageCallback = (int hr, XStoreQueryResult result) =>
                {
                    if (HR.FAILED(hr))
                    {
                        Debug.LogWarning($"{nameof(SDK.XStoreQueryAssociatedProductsAsync)} failed - 0x{hr:X8}.");

                        if (completedCallback != null)
                        {
                            completedCallback?.Invoke(hr, null);
                        }

                        SDK.XStoreCloseProductsQueryHandle(result);
                        return;
                    }

                    if (HR.SUCCEEDED(hr))
                    {
                        foreach (XStoreProduct product in result.PageItems)
                        {
                            products.Add(product);
                        }

                        if (result.HasMorePages)
                        {
                            SDK.XStoreProductsQueryNextPageAsync(result, NextPageCallback);
                        }
                        else
                        {
                            if (completedCallback != null)
                            {
                                completedCallback?.Invoke(hr, products);
                            }

                            SDK.XStoreCloseProductsQueryHandle(result);
                        }
                    }
                });
        }
        #endregion QueryAssociatedProducts

        #region QueryConsumableBalance
        /// <summary>
        /// Gets the consumable balance for the specified product.
        /// https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/reference/system/xstore/functions/xstorequeryconsumablebalanceremainingasync
        /// </summary>
        /// <param name="storeContext">Store context for the current user.</param>
        /// <param name="storeId">StoreId of the consumable product.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public static void QueryConsumableBalance(XStoreContext storeContext, string storeId, Action<int, uint> completedCallback = null)
        {
            Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreQueryConsumableBalanceRemainingAsync)}()");

            SDK.XStoreQueryConsumableBalanceRemainingAsync(
                storeContext,
                storeId,
                (int hr, XStoreConsumableResult result) =>
                {
                    if (HR.FAILED(hr))
                    {
                        Debug.LogWarning($"{nameof(SDK.XStoreQueryConsumableBalanceRemainingAsync)} failed - 0x{hr:X8}.");

                        if (completedCallback != null)
                        {
                            completedCallback?.Invoke(hr, 0);
                        }

                        return;
                    }

                    if (HR.SUCCEEDED(hr))
                    {
                        Debug.Log($"Consumable balance for {storeId}: {result.Quantity}");

                        if (completedCallback != null)
                        {
                            completedCallback?.Invoke(hr, result.Quantity);
                        }
                    }
                });
        }
        #endregion QueryConsumableBalance

        #region QueryEntitledProducts
        /// <summary>
        /// Provides store product information for all add-ons related to the current game
        /// that the user has an entitlement to (collections).
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorequeryentitledproductsasync
        /// </summary>
        /// <param name="storeContext">Store context for the current user.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public static void QueryEntitledProducts(XStoreContext storeContext, Action<int, List<XStoreProduct>> completedCallback = null)
        {
            Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreQueryEntitledProductsAsync)}()");

            // If a continuation token is used, duplicate items might appear in later page results.
            // Using a dictionary to filter out potential duplicates.
            Dictionary<string, XStoreProduct> products = new();
            List<XStoreProduct> productList = new();
            XStoreProductKind productKinds = XStoreProductKind.Consumable | XStoreProductKind.Durable | XStoreProductKind.Game | XStoreProductKind.Pass | XStoreProductKind.UnmanagedConsumable;
            XStoreQueryComplete NextPageCallback = null;

            SDK.XStoreQueryEntitledProductsAsync(
                storeContext,
                productKinds,
                MAXPAGECOUNT,
                NextPageCallback = (int hr, XStoreQueryResult result) =>
                {
                    if (HR.FAILED(hr))
                    {
                        Debug.LogWarning($"{nameof(SDK.XStoreQueryEntitledProductsAsync)} failed - 0x{hr:X8}.");
                    
                        if (hr == (int)E_XBOX_ERROR_CODES.ERROR_NO_SUCH_USER)
                        {
                            // When running in Sandbox on PC, the user must be signed into both the Xbox App and Microsoft Store
                            // in order for entitlements query to succeed.
                            Debug.LogError($"ERROR_NO_SUCH_USER: Verify the account is signed into the Xbox App and Microsoft Store, then re-sign into the game to continue.");
                        }

                        if (completedCallback != null)
                        {
                            completedCallback?.Invoke(hr, null);
                        }

                        SDK.XStoreCloseProductsQueryHandle(result);
                        return;
                    }

                    if (HR.SUCCEEDED(hr))
                    {
                        foreach (XStoreProduct product in result.PageItems)
                        {
                            if (!products.ContainsKey(product.StoreId))
                            {
                                products.Add(product.StoreId, product);
                            }
                        }

                        if (result.HasMorePages)
                        {
                            SDK.XStoreProductsQueryNextPageAsync(result, NextPageCallback);
                        }
                        else
                        {
                            if (completedCallback != null)
                            {
                                // The callback expects a list of products, so
                                // convert the dictionary results before returning.
                                foreach(KeyValuePair<string, XStoreProduct> p in products)
                                {
                                    productList.Add(p.Value);
                                }

                                completedCallback?.Invoke(hr, productList);
                            }

                            SDK.XStoreCloseProductsQueryHandle(result);
                        }
                    }
                });
        }
        #endregion QueryEntitledProducts

        #region QueryPackageIdentifier
        /// <summary>
        /// Retrieves the package identifier for the specified product (durable with package).
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorequerypackageidentifier
        /// </summary>
        /// <param name="storeId">StoreId of the product.</param>
        /// <returns>Package identifier associated with the product or an empty string if the package is not available.</returns>
        public static string QueryPackageIdentifier(string storeId)
        {
            Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreQueryPackageIdentifier)}()");

            int hr = SDK.XStoreQueryPackageIdentifier(storeId, out string packageId);
        
            if (HR.FAILED(hr))
            {
                if (hr == (int)E_XBOX_ERROR_CODES.E_GAMEPACKAGE_NO_PACKAGE_IDENTIFIER)
                {
                    Debug.LogWarning($"The package for {storeId} is not installed.");
                }
                else
                {
                    Debug.LogWarning($"{nameof(SDK.XStoreQueryPackageIdentifier)} for {storeId} failed - 0x{hr:X8}.");
                }
            }
        
            if (HR.SUCCEEDED(hr))
            {
                Debug.Log($"PackageId for {storeId}: {packageId}");
            }

            return packageId;
        }
        #endregion QueryPackageIdentifier

        #region QueryProductForCurrentGame
        /// <summary>
        /// Provides store product information for the currently running game.
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorequeryproductforcurrentgameasync
        /// </summary>
        /// <param name="storeContext">Store context for the current user.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public static void QueryProductForCurrentGame(XStoreContext storeContext, Action<int, XStoreProduct> completedCallback = null)
        {
            Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreQueryProductForCurrentGameAsync)}()");

            SDK.XStoreQueryProductForCurrentGameAsync(
                storeContext,
                (int hr, XStoreQueryResult result) =>
                {
                    if (HR.FAILED(hr))
                    {
                        Debug.LogWarning($"{nameof(SDK.XStoreQueryProductForCurrentGameAsync)} failed - 0x{hr:X8}.");

                        if(completedCallback != null)
                        {
                            completedCallback?.Invoke(hr, null);
                        }

                        return;
                    }

                    if (HR.SUCCEEDED(hr))
                    {
                        if (completedCallback != null)
                        {
                            if (result.PageItems.Length > 0)
                            {
                                completedCallback?.Invoke(hr, result.PageItems[0]);
                            }
                            else
                            {
                                completedCallback?.Invoke(hr, null);
                            }
                        }
                    }
                });
        }
        #endregion QueryProductForCurrentGame

    }
}
