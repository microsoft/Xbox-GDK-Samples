using UnityEngine;


#if (UNITY_GAMECORE_XBOXONE || UNITY_GAMECORE_SCARLETT)
#if (UNITY_2021_3)
using XboxPLM = UnityEngine.GameCore.GameCorePLM;
#else
using XboxPLM = UnityEngine.WindowsGames.WindowsGamesPLM;
#endif
#endif //#if (UNITY_GAMECORE_XBOXONE || UNITY_GAMECORE_SCARLETT)

namespace GdkSample_SimplePLM
{
    public sealed class GamePLM : MonoBehaviour
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
            }
        }

        private void OnResuming(double secondsSuspended)
        {
            Logger.Instance.Log("OnResuming", LogColor.System);
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
