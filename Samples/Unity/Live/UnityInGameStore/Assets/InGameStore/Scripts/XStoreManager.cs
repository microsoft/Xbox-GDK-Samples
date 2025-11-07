using System;
using System.Collections.Generic;
using System.Text;
using UnityEngine;
using Unity.XGamingRuntime;

namespace GdkSample_InGameStore
{
    public sealed class XStoreManager : MonoBehaviour
    {
        public static XStoreManager Instance { get; private set; }

        /// <summary>
        /// Store ID of the base game.
        /// Set by QueryCurrentGame() during store initialization.
        /// </summary>
        public string BaseGame { get; private set; }

        /// <summary>
        /// True, if the store is fully initialized and ready for
        /// user interaction.
        /// </summary>
        public bool IsStoreReady { get; private set; }

        /// <summary>
        /// Signals when a durable license has been lost.
        /// </summary>
        public event Action DurableLicenseLost;

        /// <summary>
        /// Signals when changes are made to the 'AllProducts' list.
        /// </summary>
        public event Action ProductsUpdated;

        /// <summary>
        /// Signals when store intialization fails.
        /// </summary>
        public event Action StoreInitializationFailed;

        /// <summary>
        /// Signals when store initialization is complete and the
        /// store is ready for user interaction.
        /// </summary>
        public event Action StoreInitializationSucceeded;
  
        /// <summary>
        /// Game License retrieved for the current user.
        /// Set during initialization.
        /// </summary>
        public XStoreGameLicense GameLicense { get; private set; }

        /// <summary>
        /// Store context associated with the current user.
        /// This is required by the majority of XStore* APIs.
        /// Set during initialization.
        /// </summary>
        public XStoreContext StoreContext { get; private set; }

        /// <summary>
        /// Contains all products available to the current user (catalog + collections).
        /// Recreated whenever a new account is signed into the game.
        /// </summary>
        public Dictionary<string, XStoreProduct> AllProducts;
    
        /// <summary>
        /// Contains all products available for purchase.
        /// Recreated whenever a new account is signed into the game.
        /// </summary>
        public Dictionary<string, XStoreProduct> CatalogProducts;
    
        /// <summary>
        /// Contains all durables with an active license.
        /// Persists between user sign-in/sign-out events.
        /// </summary>
        public Dictionary<string, Tuple<XStoreLicense, PackageLicenseLostCallbackToken>> LicensedDurables;
    
        /// <summary>
        /// Tracks license lost events for durables that have acquired a license.
        /// Persists between user sign-in/sign-out events.
        /// </summary>
        public Queue<string> LostLicensesQueue;

        /// <summary>
        /// Registration token for game license change events.
        /// </summary>
        private GameLicenseChangedCallbackToken _gameLicenseChangedToken;

        private void Awake()
        {
            if (Instance != null)
            {
                Destroy(this);
                return;
            }

            Instance = this;

            AllProducts = new();
            CatalogProducts = new();
            LicensedDurables = new();
            LostLicensesQueue = new();

            IsStoreReady = false;
        }

        // Start is called before the first frame update.
        void Start()
        {
            XboxManager.Instance.UserSignedIn += OnSignInDetected;
            XboxManager.Instance.UserSignedOut += OnUserChangeDetected;
            XboxManager.Instance.UserSignInStarted += OnUserChangeDetected;
            XboxManager.Instance.UserSignOutStarted += OnUserChangeDetected;

            // Initialize GDK runtime and Xbox Live
            if (GDKGameRuntime.TryInitialize())
            {
                XboxManager.Instance.InitializeAndAddUser();
            }
        }

        private void Update()
        {
            // It's up to the game to decide how to respond to license lost events.
            // Typically, a game will allow the player to finish their current session and/or save
            // before losing access to durable content. For this sample, we'll close the lost license handle
            // and remove it from the 'LicensedDurables' list.

            // Check if any durables have lost their license.
            LostLicensesQueue.TryPeek(out string storeId);

            if (!string.IsNullOrEmpty(storeId) && IsStoreReady)
            {
                if (LicensedDurables.ContainsKey(storeId))
                {
                    Debug.Log($"Removing license for durable {storeId}.");
                    var durable = LicensedDurables[storeId];
                
                    // Unregister license for LicenseLost events and close the handle
                    SDK.XStoreUnregisterPackageLicenseLost(durable.Item1, durable.Item2);
                    SDK.XStoreCloseLicenseHandle(durable.Item1);
                
                    // Remove the license and send an event for the UI to respond to
                    LicensedDurables.Remove(storeId);
                    DurableLicenseLost?.Invoke();
                }

                LostLicensesQueue.Dequeue();
            }
        }

        #region Event Handlers

        private void OnSignInDetected()
        {
            // Once a user has signed into the game, we can create a store
            // context and query licensing/catalog/collections.
            InitializeStore();
        }

        private void OnUserChangeDetected()
        {
            // If the active user signs out, or a new user is in the process of
            // signing in, the store context is no longer valid.
            IsStoreReady = false;
            ClearStoreContext();
        }

        #endregion Event Handlers

        #region InitializeStore
        /// <summary>
        /// Creates the store context, queries for the game license, and
        /// retrieves product information for the base game, and products
        /// returned form catalog and collections.
        /// Called whenever a new player is signed into the game or when
        /// the title resumes from a suspend event.
        /// </summary>
        public void InitializeStore()
        {
            Logger.Instance.Log($"{nameof(XStoreManager)} {nameof(InitializeStore)} started...", LogColor.System);

            IsStoreReady = false;
            ClearStore();

            // Check if the current user is signed into the Micrsoft Store.
            // On console, this is always true. On PC, the user must be signed into the Microsoft Store for XStore APIs to work.
            // It is possible to have a different user signed into the Microsoft Store than the Xbox Live user, but all purchases
            // will be associated with the Microsoft Store account.
            // See 'Handling mismatched store account scenarios on PC' in the GDK documentation.
            if (!SDK.XUserIsStoreUser(XboxManager.Instance.UserHandle))
            {
                Debug.LogError($"{nameof(XStoreManager)} {nameof(InitializeStore)} failed. Current user does not match Microsoft Store user.");
                StoreInitializationFailed?.Invoke();
                return;
            }

            // Create the store context for the current user.
            int hr = CreateStoreContext();
            if (HR.FAILED(hr))
            {
                StoreInitializationFailed?.Invoke();
                return;
            }

            // Get base game information to add as an offer in the store.
            // (Optional) This sample will show the base game offer
            // to players that do not have a full license to the game.
            QueryCurrentGame();

            // Check if the player has a license to the game.
            QueryGameLicense(false, (hr, gameLicense) =>
            {
                // Note: you can prevent users from playing the game or accessing
                // the store when they don't have a game license (restrictive licensing).
                // For this sample, we won't require a game license, but we will
                // show the base game offer in the catalog when the user doesn't have
                // a full game license.

                // Check for catalog items available for purchase within the game.
                QueryAssociatedProducts(false, (hr, products) =>
                {
                    if (HR.SUCCEEDED(hr))
                    {
                        foreach (XStoreProduct product in products)
                        {
                            if (!CatalogProducts.ContainsKey(product.StoreId))
                            {
                                CatalogProducts.Add(product.StoreId, product);
                            }
                        }

                        // Check collections for any additional products that
                        // the player might own but are not available in the catalog.
                        // (Optional) This sample will show a combination of catalog and collection
                        // results in the UI product list. If a title only shows purchasable items,
                        // then this call is not required during initialization.
                        QueryEntitledProducts(false, (hr, products) =>
                        {
                            if (HR.FAILED(hr))
                            {
                                // Sandbox or account configuration is wrong. Can't recover from this without user intervention.
                                Debug.LogError($"{nameof(XStoreManager)} {nameof(InitializeStore)} failed. Check sandbox and user configuration.");
                                StoreInitializationFailed?.Invoke();
                                return;
                            }

                            if (HR.SUCCEEDED(hr))
                            {
                                Logger.Instance.Log($"{nameof(XStoreManager)} {nameof(InitializeStore)} succeeded.", LogColor.System);
                            }

                            IsStoreReady = true;
                            StoreInitializationSucceeded?.Invoke();
                        });
                    }
                });
            });
        }
        #endregion InitializeStore

        #region CreateStoreContext
        /// <summary>
        /// Creates a store context for the current user.
        /// This is called whenever a new user is signed-in or after the game resumes from a suspended state.
        /// </summary>
        public int CreateStoreContext()
        {
            Debug.Log($"{nameof(XStoreManager)}.{nameof(CreateStoreContext)}()");

            if (StoreContext != null)
            {
                ClearStoreContext();
            }

            int hr = SDK.XStoreCreateContext(XboxManager.Instance.UserHandle, out XStoreContext storeContext);
        
            if (HR.FAILED(hr) || storeContext == null)
            {
                Debug.LogError($"{nameof(SDK.XStoreCreateContext)} failed - 0x{hr:X8}.");
                return hr;
            }
        
            StoreContext = storeContext;

            int hresult = SDK.XStoreRegisterGameLicenseChanged(
                storeContext,
                (IntPtr context)=>
                {
                    // It's up to the game to decide how to respond to license change events.
                    // Typically, a game license check occurs after sign-in and when resuming from a suspend event.
                    // For this sample, we'll log the change event and query for an updated license.
                    Debug.Log($"Game license change detected.");
                
                    if (IsStoreReady)
                    {
                        QueryGameLicense();
                    }
                },
                out _gameLicenseChangedToken);

            if (HR.FAILED(hresult))
            {
                Debug.LogWarning($"{nameof(SDK.XStoreRegisterGameLicenseChanged)} failed - 0x{hresult:X8}.");
            }

            return hr;
        }
        #endregion CreateStoreContext

        #region QueryCurrentGame
        /// <summary>
        /// Retrieve's the current title's product information.
        /// This can be used as an upsell offer when the user does not have a license.
        /// </summary>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public void QueryCurrentGame(Action<int, XStoreProduct> completedCallback = null)
        {
            if (StoreContext == null)
            { return; }

            Debug.Log($"{nameof(XStoreManager)}.{nameof(QueryCurrentGame)}()");

            XStoreQueries.QueryProductForCurrentGame(
                StoreContext,
                (int hr, XStoreProduct game) =>
                {
                    if (HR.SUCCEEDED(hr))
                    {
                        Logger.Instance.Log($"StoreId of base game = {game.StoreId}", LogColor.Success);
                        BaseGame = game.StoreId;

                        // Add the current game to the list of purchasable content.
                        if (!CatalogProducts.ContainsKey(game.StoreId))
                        {
                            CatalogProducts.Add(game.StoreId, game);
                        }

                        List<XStoreProduct> baseGameList = new();
                        baseGameList.Add(game);
                        UpdateProducts(baseGameList);
                    }

                    if(completedCallback != null)
                    {
                        completedCallback?.Invoke(hr, game);
                    }
                });
        }
        #endregion QueryCurrentGame

        #region QueryGameLicense 
        /// <summary>
        /// Requests the active game license. Will return a valid license if an owning
        /// account is signed into the console or has designated it as their 'Home' console
        /// for sharing licenses. Checking license tokens with your game service is recommended when
        /// running on PC but not necessary on Xbox consoles.
        /// </summary>
        /// <param name="getToken">True, if a license token should be created to pass to your game service for verification.</param>
        /// <param name="completedCallback">Optional callback to enable additional processing by caller.</param>
        public void QueryGameLicense(bool getToken = false, Action<int, XStoreGameLicense> completedCallback = null)
        {
            if (StoreContext == null)
            { return; }

            Debug.Log($"{nameof(XStoreManager)}.{nameof(QueryGameLicense)}()");

            XStoreLicensing.QueryGameLicense(StoreContext, (hr, license) =>
            {
                if (HR.SUCCEEDED(hr) && license != null)
                {
                    GameLicense = license;

                    Logger.Instance.Log($"Game license is active: {license.IsActive}", LogColor.Success);

                    if (!license.IsActive)
                    {
                        Debug.LogWarning("No active game license found.");
                    }
                    else
                    {
                        StringBuilder licenseInfo = new();
                        licenseInfo.Append($"Game license sku: {license.SkuStoreId}");

                        if (license.IsDiscLicense)
                        {
                            licenseInfo.Append(", running from disc.");
                        }
                        else if (license.IsTrial)
                        {
                            licenseInfo.Append($"\nTrial: {license.TrialTimeRemainingInSeconds} seconds remaining | ");

                            System.DateTime now = System.DateTime.Now;
                            System.DateTime expiration = DateTime.FromOADate(license.ExpirationDate);
                            if (expiration > now)
                            {
                                licenseInfo.Append($"Expiration date: {expiration.ToLongTimeString()} | ");
                            }
                            licenseInfo.Append(license.IsTrialOwnedByThisUser ? "Owned by current account | " : "Owned by another account | ");
                            licenseInfo.Append($"Unique ID: {license.TrialUniqueId}");
                        }

                        if (license.IsTrial)
                        {
                            Debug.LogWarning(licenseInfo.ToString());
                        }
                        else
                        {
                            Debug.Log(licenseInfo.ToString());
                        }

                        if (getToken)
                        {
                            // Retrieve the license token for the game.
                            XStoreLicensing.QueryLicenseToken(StoreContext, BaseGame);
                        }
                    }
                }

                if (completedCallback != null)
                {
                    completedCallback?.Invoke(hr, license);
                }
            });
        }
        #endregion QueryGameLicense

        #region QueryAssociatedProducts
        /// <summary>
        /// Retrieves all add-on products currently available for purchase.
        /// Called whenever an account is signed into the game as part
        /// of the store initialization process.
        /// </summary>
        /// <param name="printProducts">True, if catalog details should be printed to the console.</param>
        public void QueryAssociatedProducts(bool printProducts = false, Action<int, List<XStoreProduct>> completedCallback = null)
        {
            if (StoreContext == null)
            { return; }

            Debug.Log($"{nameof(XStoreManager)}.{nameof(QueryAssociatedProducts)}()");

            XStoreQueries.QueryAssociatedProducts(
                StoreContext, 
                (hr, products) =>
                {
                    if (HR.SUCCEEDED(hr))
                    {
                        Logger.Instance.Log($"Number of catalog items = {products.Count}", LogColor.Success);

                        if (printProducts)
                        {
                            string productsInfo = GetProductStrings(products);
                            Debug.Log(productsInfo);
                        }

                        UpdateProducts(products);
                    }

                    if (completedCallback != null)
                    {
                        completedCallback?.Invoke(hr, products);
                    }
                });
        }
        #endregion QueryAssociatedProducts

        #region QueryEntitledProducts
        /// <summary>
        /// Retrieves all add-on products that the user has an active entitlement to.
        /// Results may include previously purchased products that are no longer availabe for purchase within the store,
        /// or products that cannot be purchased outside of a bundle/pass.
        /// Called as the final step of store initialization or whenever the title is unconstrained after purchase/visiting the store
        /// to update ownership details in the UI.
        /// </summary>
        /// <param name="printProducts">True, if collection details should be printed to the console.</param>
        public void QueryEntitledProducts(bool printProducts = false, Action<int, List<XStoreProduct>> completedCallback = null)
        {
            if (StoreContext == null)
            { return; }

            Debug.Log($"{nameof(XStoreManager)}.{nameof(QueryEntitledProducts)}()");

            XStoreQueries.QueryEntitledProducts(
                StoreContext, 
                (hr, products) =>
                {
                    if (HR.SUCCEEDED(hr))
                    {
                        Logger.Instance.Log($"Number of collection items = {products.Count}", LogColor.Success);

                        if (printProducts)
                        {
                            string productsInfo = GetProductStrings(products);
                            Debug.Log(productsInfo);
                        }
                
                        UpdateProducts(products);
                    }

                    if (completedCallback != null)
                    {
                        completedCallback?.Invoke(hr, products);
                    }
                });
        }
        #endregion QueryEntitledProducts

        #region Helper Methods

        /// <summary>
        /// When a game no longer needs access to content, it's good practice to close
        /// active license handles to help avoid hitting concurrency limits.
        /// For this sample, we'll close any active licenses and the store context before exiting the game.
        /// </summary>
        public void CloseAllStoreHandles()
        {
            Debug.Log($"{nameof(XStoreManager)}.{nameof(CloseAllStoreHandles)}()");

            // Close the StoreContext and unregister for game license changes
            ClearStoreContext();

            // Close any durable license handles still open and unregister for license lost events
            foreach (KeyValuePair<string, Tuple<XStoreLicense, PackageLicenseLostCallbackToken>> durable in LicensedDurables)
            {
                SDK.XStoreUnregisterPackageLicenseLost(durable.Value.Item1, durable.Value.Item2);
                SDK.XStoreCloseLicenseHandle(durable.Value.Item1);
            }

            LicensedDurables.Clear();
        }

        /// <summary>
        /// Checks for ownership/license changes related to the base game offer.
        /// </summary>
        public void CheckForGamePurchase()
        {
            if (StoreContext == null)
            { return; }

            Debug.Log($"{nameof(XStoreManager)}.{nameof(CheckForGamePurchase)}()");

            QueryCurrentGame((int hr, XStoreProduct game) =>
            {
                if (HR.SUCCEEDED(hr) && game != null && game.IsInUserCollection)
                {
                    // If the user owns the base game, check for an updated license.
                    // To get the latest license (not cached), recreate the store context.
                    hr = CreateStoreContext();                    
                    if (HR.SUCCEEDED(hr) && StoreContext != null)
                    {
                        QueryGameLicense(false, (int hr, XStoreGameLicense license) =>
                        {
                            if (HR.SUCCEEDED(hr) && license != null)
                            {
                                // If the game is operating with the wrong license type, a restart might be required.
                                // This can happen when the game is configured with an unlimited trial and the player
                                // purchases the upgraded license outside of the game (via Microsoft Store).
                                if (license.IsTrial && OwnsFullGameSku(game))
                                {
                                    Debug.LogWarning($"License refresh required. Relaunch or suspend/resume game to upgrade license.");
                                }

                                // Update UI to reflect latest details.
                                ProductsUpdated?.Invoke();
                            }
                        });
                    }
                }
            });
        }

        /// <summary>
        /// Checks if the current player owns the full game sku.
        /// </summary>
        /// <param name="game">Game to check.</param>
        /// <returns>True if the player owns the full game sku, false otherwise.</returns>
        public bool OwnsFullGameSku(XStoreProduct game)
        {
            for(int i = 0; i < game.Skus.Length; ++i)
            {
                if (game.Skus[i].IsInUserCollection && !game.Skus[i].IsTrial)
                {
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Checks the game license to determine if the user is in trial mode.
        /// </summary>
        /// <returns>True, if the user is running the game with a trial license.</returns>
        public bool IsTrialGame()
        {
            if (GameLicense == null)
            {
                return false;
            }
            else if (GameLicense.IsTrial)
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Checks the game license to determine if the base game
        /// offer should be shown in the catalog.
        /// </summary>
        /// <returns>True, if the user does not have a full license to the game.</returns>
        public bool ShowBaseGameOffer()
        {
            if (GameLicense == null)
            {
                return true;
            }
            else if (GameLicense.IsTrial)
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Resets most store globals/variables for a clean start.
        /// Does not clear durable license data, as that should persist across users.
        /// </summary>
        private void ClearStore()
        {
            ClearStoreContext();

            if (AllProducts != null)
            {
                AllProducts.Clear();
            }

            if (CatalogProducts != null)
            {
                CatalogProducts.Clear();
            }

            GameLicense = null;
        }

        /// <summary>
        /// Closes the store context handle and unregisters for game license changes.
        /// </summary>
        private void ClearStoreContext()
        {
            if (StoreContext != null)
            {
                if (_gameLicenseChangedToken != null)
                {
                    // Unregister the game license changed callback for the current context.
                    SDK.XStoreUnregisterGameLicenseChanged(StoreContext, _gameLicenseChangedToken);
                }

                // Close the current context handle.
                SDK.XStoreCloseContextHandle(StoreContext);
            }

            StoreContext = null;
            _gameLicenseChangedToken = null;
        }

        /// <summary>
        /// Adds/updates a list of all products available (catalog + collections) for the current user.
        /// After updating, it will fire an event for the UI to respond to.
        /// </summary>
        /// <param name="products">List of products to update.</param>
        private void UpdateProducts(List<XStoreProduct> products)
        {
            foreach (XStoreProduct product in products)
            {            
                if (AllProducts.ContainsKey(product.StoreId))
                {
                    AllProducts[product.StoreId] = product;
                }
                else
                {
                    AllProducts.Add(product.StoreId, product);
                }
            }

            // Inform UI that product updates are available.
            ProductsUpdated?.Invoke();
        }

        /// <summary>
        /// Prints basic XStoreProduct details to the console.
        /// </summary>
        /// <param name="products">List of products to print details for.</param>
        /// <returns>A string that can be printed to console.</returns>
        private string GetProductStrings(List<XStoreProduct> products)
        {
            string productsInfo = "";

            foreach (XStoreProduct product in products)
            {
                productsInfo += "\n" + product.StoreId + " (" + product.ProductKind + ") " + product.Title
                    + (product.IsInUserCollection == true ? " [Owned]" : "");
            }

            return productsInfo;
        }

        #endregion Helper Methods

    }
}
