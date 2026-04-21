using System;
using Unity.XGamingRuntime;
using UnityEngine;

namespace GdkSample_InGameStore
{
    public sealed class XNetworkManager : MonoBehaviour
    {
        public static XNetworkManager Instance { get; private set; }

        public event Action NetworkConnectionEstablished;

        public event Action NetworkConnectionLost;

        /// <summary>
        /// True, if network connectivity is available.
        /// </summary>
        public bool IsNetworkAvailable { get; private set; }

        /// <summary>
        /// True, if network has been initialized.
        /// This value is reset on console during a suspend/resume cycle.
        /// </summary>
        public bool IsNetworkInitialized
        {
            get
            {
                int hr = SDK.XNetworkingGetConnectivityHint(out XNetworkingConnectivityHint hint);
                if (HR.FAILED(hr))
                {
                    Debug.Log($"FAILED to get network connectivity hint: HResult: 0x{hr:x} ({HR.NameOf(hr)})");
                    return false;
                }

                return hint.NetworkInitialized;
            }
        }

        private XNetworkingRegisterConnectivityHintChangedCallbackToken _networkConnectivityToken;

        private void Awake()
        {
            InitializeOrDestroyInstance(this);
        }

        private static bool InitializeOrDestroyInstance(XNetworkManager newInstance)
        {
            if (Instance != newInstance && Instance != null)
            {
                Debug.LogWarning($"An instance of {newInstance.GetType().Name} already exist. Destroying {newInstance}...");
                Destroy(newInstance.gameObject);
                return false;
            }

            Instance = newInstance;
            DontDestroyOnLoad(Instance.gameObject);
            return GDKGameRuntime.Initialized; // GDKGameRuntime must be initialized for network monitor to work
        }

        void Start()
        {
            GDKGameRuntime.GameRuntimeInitialized += RegisterForConnectivityChanges;

            if (GDKGameRuntime.Initialized)
            {
                RegisterForConnectivityChanges();

                // set initial network availability based on current connectivity hint
                int hr = SDK.XNetworkingGetConnectivityHint(out XNetworkingConnectivityHint hint);
                if (HR.FAILED(hr))
                {
                    Debug.Log($"FAILED to get network connectivity hint: HResult: 0x{hr:x} ({HR.NameOf(hr)})");
                    IsNetworkAvailable = false;
                    return;
                }
                IsNetworkAvailable = hint.ConnectivityLevel == XNetworkingConnectivityLevelHint.InternetAccess;
            }
        }

        private void RegisterForConnectivityChanges()
        {
            // Register for network connectivity changes
            int hResult = SDK.XNetworkingRegisterConnectivityHintChanged(OnNetworkConnectivityHintChanged, out _networkConnectivityToken);
            if (HR.FAILED(hResult))
            {
                Debug.Log($"FAILED to register for network connectivity changes: HResult: 0x{hResult:x} ({HR.NameOf(hResult)})");
            }
        }

        private static void OnNetworkConnectivityHintChanged(IntPtr context, XNetworkingConnectivityHint hint)
        {
            if (!Instance.IsNetworkAvailable && hint.NetworkInitialized && hint.ConnectivityLevel == XNetworkingConnectivityLevelHint.InternetAccess)
            {
                Logger.Instance.Log($"OnNetworkConnectivityHintChanged: {hint.ConnectivityLevel}", LogColor.System);
                Instance.IsNetworkAvailable = true;
                Instance.NetworkConnectionEstablished?.Invoke();
            }
            else if(Instance.IsNetworkAvailable && (!hint.NetworkInitialized || hint.ConnectivityLevel != XNetworkingConnectivityLevelHint.InternetAccess))
            {
                Logger.Instance.Log($"OnNetworkConnectivityHintChanged: {hint.ConnectivityLevel}", LogColor.System);
                Instance.IsNetworkAvailable = false;
                Instance.NetworkConnectionLost?.Invoke();
            }
        }

        private void OnDestroy()
        {
            GDKGameRuntime.GameRuntimeInitialized -= RegisterForConnectivityChanges;

            if (_networkConnectivityToken != null)
            {
                SDK.XNetworkingUnregisterConnectivityHintChanged(_networkConnectivityToken, false);
                _networkConnectivityToken = null;
            }

            Instance = null;
        }
    }
}