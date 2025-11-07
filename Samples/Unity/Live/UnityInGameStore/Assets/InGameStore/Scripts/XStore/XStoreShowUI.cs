using System;
using UnityEngine;
using Unity.XGamingRuntime;

namespace GdkSample_InGameStore
{
    /// <summary>
    /// XStoreShowUI class for showing various product UI via the Microsoft Store.
    /// </summary>
    public sealed class XStoreShowUI
    {

        #region ShowAssociatedProductsUI
        /// <summary>
        /// Opens the Microsoft Store app and shows a set of add-ons associated with the game that
        /// are available for purchase.
        /// Note: This API does not support product kinds 'Pass' and 'UnmanagedConsumable'.
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowassociatedproductsuiasync
        /// Bug: The Microsoft Store will show 'No results found' when running in sandbox or private audience on PC.
        /// </summary>
        /// <param name="storeContext">The store context for the current user.</param>
        /// <param name="storeId">StoreId of the game.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public static void ShowAssociatedProductsUI(XStoreContext storeContext, string storeId, Action<int> completedCallback = null)
        {
#if (UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN)
            Debug.LogWarning($"{nameof(SDK.XStoreShowAssociatedProductsUIAsync)}: Microsoft Store will show 'No results found' when running in sandbox or private audience on PC.");
#endif
            Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreShowAssociatedProductsUIAsync)}() called for {storeId}");

            XStoreProductKind productKinds = XStoreProductKind.Consumable | XStoreProductKind.Durable | XStoreProductKind.Game;
        
            int hresult = SDK.XStoreShowAssociatedProductsUIAsync(
                storeContext,
                storeId,
                productKinds,
                (int hr) =>
                {
                    if (HR.FAILED(hr))
                    {
                        Debug.LogWarning($"{nameof(SDK.XStoreShowAssociatedProductsUIAsync)} for {storeId} failed - 0x{hr:X8}.");
                    }

                    if (completedCallback != null)
                    {
                        completedCallback?.Invoke(hr);
                    }
                });
        }
        #endregion ShowAssociatedProductsUI

        #region ShowProductPageUI
        /// <summary>
        /// Opens the Microsoft Store app directly to the Product Details Page of the requested product.
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowproductpageuiasync
        /// </summary>
        /// <param name="storeContext">Store context for the current user.</param>
        /// <param name="storeId">StoreId of the product.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public static void ShowProductPageUI(XStoreContext storeContext, string storeId, Action<int> completedCallback = null)
        {
            Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreShowProductPageUIAsync)}() called for {storeId}");

            int hresult = SDK.XStoreShowProductPageUIAsync(
                storeContext,
                storeId,
                (int hr) =>
                {
                    if (HR.FAILED(hr))
                    {
                        Debug.LogWarning($"{nameof(SDK.XStoreShowProductPageUIAsync)} for {storeId} failed - 0x{hr:X8}.");
                    }

                    if (completedCallback != null)
                    {
                        completedCallback?.Invoke(hr);
                    }
                });
        }
        #endregion ShowProductPageUI

        #region ShowPurchaseUI
        /// <summary>
        /// Shows the purchase UI overlay for the specified product.
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowpurchaseuiasync
        /// </summary>
        /// <param name="storeContext">Store context for the current user.</param>
        /// <param name="storeId">StoreId of the product.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public static void ShowPurchaseUI(XStoreContext storeContext, string storeId, Action<int> completedCallback = null)
        {
            Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreShowPurchaseUIAsync)}() called for {storeId}");

            SDK.XStoreShowPurchaseUIAsync(
                storeContext,
                storeId,
                null,   // Can be used to provide the title bar text
                null,   // Can be used to provide extra details to purchase
                (int hr) =>
                {
                    if (HR.FAILED(hr))
                    {
                        if (hr == (int)E_XBOX_ERROR_CODES.E_ABORT)
                        {
                            Debug.LogWarning($"{storeId} purchase canceled by user.");
                        }
                        else if (hr == ((int)E_XBOX_ERROR_CODES.E_GAMESTORE_ALREADY_PURCHASED))
                        {
                            Debug.LogWarning($"{storeId} already purchased by user.");
                        }
                        else
                        {
                            Debug.LogWarning($"{nameof(SDK.XStoreShowPurchaseUIAsync)} for {storeId} failed - 0x{hr:X8}.");
                        }
                    }

                    if (completedCallback != null)
                    {
                        completedCallback?.Invoke(hr);
                    }
                });
        }
        #endregion ShowPurchaseUI

        #region ShowRateAndReviewUI
        /// <summary>
        /// Displays a system dialog to allow the user to review the current game.
        /// Note: Relies on production content, title must be propped to RETAIL in order for this to work in sandbox.
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowrateandreviewuiasync
        /// </summary>
        /// <param name="storeContext">Store context for the current user.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public static void ShowRateAndReviewUI(XStoreContext storeContext, Action<int> completedCallback = null)
        {
            Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreShowRateAndReviewUIAsync)}()");

            SDK.XStoreShowRateAndReviewUIAsync(
                storeContext,
                (int hr, XStoreRateAndReviewResult result) =>
                {               
                    if (HR.FAILED(hr))
                    {
                        if (hr == (int)E_XBOX_ERROR_CODES.E_ABORT)
                        {
                            Debug.LogWarning("Rate and review canceled by user.");
                        }
                        else
                        {
                            Debug.LogWarning($"{nameof(SDK.XStoreShowRateAndReviewUIAsync)} failed - 0x{hr:X8}.");
                        }
                    }
                
                    if (HR.SUCCEEDED(hr))
                    {
                        Debug.Log($"Ratings updated: {result.WasUpdated}");
                    }

                    if (completedCallback != null)
                    {
                        completedCallback?.Invoke(hr);
                    }
                });
        }
        #endregion ShowRateAndReviewUI

        #region ShowRedeemTokenUI
        /// <summary>
        /// Displays a system dialog to allow the user to redeem a token in the Microsoft Store.
        /// https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowredeemtokenuiasync
        /// </summary>
        /// <param name="storeContext">Store context for the current user.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public static void ShowRedeemTokenUI(XStoreContext storeContext, Action<int> completedCallback = null)
        {
            Debug.Log($"{nameof(SDK)}.{nameof(SDK.XStoreShowRedeemTokenUIAsync)}()");
            string[] allowedStoreIds = { };
        
            SDK.XStoreShowRedeemTokenUIAsync(
                storeContext,
                " ",    // Empty 5x5, user must enter token
                allowedStoreIds,   // Can be used to restrict redemption to specific store Ids
                false,  // Can be used to prevent CSV (cash gift cards)
                (int hr) =>
                {
                    if (HR.FAILED(hr))
                    {
                        if (hr == (int)E_XBOX_ERROR_CODES.E_ABORT)
                        {
                            Debug.LogWarning("Redeem token canceled by user.");
                        }
                        else
                        {
                            Debug.LogWarning($"{nameof(SDK.XStoreShowRedeemTokenUIAsync)} failed - 0x{hr:X8}.");
                        }
                    }

                    if (completedCallback != null)
                    {
                        completedCallback?.Invoke(hr);
                    }
                });
        }
        #endregion ShowRedeemTokenUI

    }
}
