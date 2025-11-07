using UnityEngine;
using Unity.XGamingRuntime;

#if (UNITY_GAMECORE_XBOXONE || UNITY_GAMECORE_SCARLETT)
#if (UNITY_2021_3)
using XboxPLM = UnityEngine.GameCore.GameCorePLM;
#else
using XboxPLM = UnityEngine.WindowsGames.WindowsGamesPLM;
#endif
#endif //#if (UNITY_GAMECORE_XBOXONE || UNITY_GAMECORE_SCARLETT)

namespace GdkSample_InGameStore
{
    /// <summary>
    /// XStorePLM class for handling application focus changes.
    /// </summary>
    public sealed class XStorePLM : MonoBehaviour
    {
        private void Start()
        {
#if (UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN)
            Application.focusChanged += OnFocusChanged;

#elif (UNITY_GAMECORE_XBOXONE || UNITY_GAMECORE_SCARLETT)
            XboxPLM.OnSuspendingEvent += OnSuspending;
            XboxPLM.OnResumingEvent += OnResuming;
            XboxPLM.OnResourceAvailabilityChangedEvent += OnResourceAvailabilityChanged;
#endif
        }

        /// <summary>
        /// Checks for new entitlements when the title regains focus.
        /// </summary>
        private void CheckEntitlements()
        {
            if (XStoreManager.Instance.IsStoreReady)
            {
                // If the title has products available for purchase in the Microsoft Store, then whenever
                // the game loses focus (PC) or is constrained (console), the user has the ability to
                // purchase content from the Store, Xbox App or xbox.com.
                // To keep the game in-sync with external purchases, query entitlements after focus is restored.
                // Note: If a title does not offer products for purchase outside of their game,
                // then this entitlement check can be moved to XStoreShowPurchaseUIAsync's callback instead.
                XStoreManager.Instance.QueryEntitledProducts();

                // (Optional) This sample will show the base game offer whenever the player does not have a full license to the game.
                // If the base game offer is available to the current player, check if an update is needed.
                if (XStoreManager.Instance.ShowBaseGameOffer() && XStoreManager.Instance.AllProducts.ContainsKey(XStoreManager.Instance.BaseGame))
                {
                    // Update the base game offer if the user does not own it, or they are running with a trial license.
                    if (!XStoreManager.Instance.AllProducts[XStoreManager.Instance.BaseGame].IsInUserCollection || XStoreManager.Instance.IsTrialGame())
                    {
                        XStoreManager.Instance.CheckForGamePurchase();
                    }
                }
            }
        }

        /// <summary>
        /// Handles application focus change events for the in-game store.
        /// </summary>
        /// <param name="isFocused">True, if the title currently has focus.</param>
        private void OnFocusChanged(bool isFocused)
        {
            if (!isFocused)
            {
                Logger.Instance.Log("OnFocusChanged: Focus lost", LogColor.System);
            }
            else
            {
                Logger.Instance.Log("OnFocusChanged: Focus restored", LogColor.System);
                CheckEntitlements();
            }
        }

#if (UNITY_GAMECORE_XBOXONE || UNITY_GAMECORE_SCARLETT)

        private void OnResourceAvailabilityChanged(bool isConstrained)
        {
            if(isConstrained)
            {
                Logger.Instance.Log("OnResourceAvailabilityChanged: Constrained", LogColor.System);
            }
            else
            {
                Logger.Instance.Log("OnResourceAvailabilityChanged: Full", LogColor.System);
                CheckEntitlements();
            }
        }

        private void OnResuming(double secondsSuspended)
        {
            Logger.Instance.Log("OnResuming", LogColor.System);

            // Store context/details might not be valid after title suspension.
            // Note: If the original user is no longer signed-in, the account picker appears
            // and the user change will trigger full store re-initialization.

            // Check if the original user is still signed in.
            SDK.XUserGetState(XboxManager.Instance.UserHandle, out XUserState userState);        
            if (userState == XUserState.SignedIn && XStoreManager.Instance.IsStoreReady)
            {
                // Refresh the store context and game license.
                XStoreManager.Instance.CreateStoreContext();
                XStoreManager.Instance.QueryGameLicense();
            }
        }

        private void OnSuspending()
        {
            // Save game state here, or do it across multiple frames if required.
            // DO NOT call XblCleanup; this is done automatically

            // You must call this before Unity times out. 
            // This tells Unity that the app has completed all work 
            // and it is now safe to suspend the title.
            // The app has less than 1 second to call this method.

            Logger.Instance.Log("OnSuspending", LogColor.System);
            XboxPLM.AmReadyToSuspendNow();
            Debug.Log("Deferral Complete");
        }

#endif //#if (UNITY_GAMECORE_XBOXONE || UNITY_GAMECORE_SCARLETT)

    }
}
