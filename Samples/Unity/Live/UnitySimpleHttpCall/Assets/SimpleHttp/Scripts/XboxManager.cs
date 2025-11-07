using System;
using Unity.XGamingRuntime;
using UnityEngine;

namespace GdkSample_SimpleHttp
{
    public sealed class XboxManager : MonoBehaviour
    {
        public static XboxManager Instance { get; private set; }

        public event Action GamerPictureRetrieved;
        public event Action GamerTagRetrieved;
        public event Action UserSignInStarted;
        public event Action UserSignOutStarted;
        public event Action UserSignedIn;
        public event Action UserSignedOut;

        public XblContextHandle ContextHandle { get; private set; }
        public XUserHandle UserHandle { get; private set; }
        public Texture2D GamerPicture { get; private set; }
        public string GamerTag { get; private set; }
        public ulong UserId { get; private set; }

        private XUserChangeRegistrationToken _registrationToken;

        private void Awake()
        {
            InitializeOrDestroyInstance(this);
        }

        private static bool InitializeOrDestroyInstance(XboxManager newInstance)
        {
            if (Instance != newInstance && Instance != null)
            {
                Debug.LogWarning($"An instance of {newInstance.GetType().Name} already exist. Destroying {newInstance}...");
                Destroy(newInstance.gameObject);
                return false;
            }

            Instance = newInstance;
            DontDestroyOnLoad(Instance.gameObject);
            return GDKGameRuntime.Initialized; // Xbox Live is initialize by GDKGameRuntime
        }

        public void InitializeAndAddUser()
        {
            if (!GDKGameRuntime.Initialized)
            {
                GDKGameRuntime.TryInitialize();
            }

            SignIn(false);
        }

        public void SignIn(bool showUI = true)
        {
            Debug.Log($"{nameof(XboxManager)}.{nameof(SignIn)}()");

            ResetUserDetails();
            UserSignInStarted?.Invoke();

            // Sign player into Xbox Live.
            SDK.XUserAddAsync(showUI ? XUserAddOptions.None : XUserAddOptions.AddDefaultUserAllowingUI, AddUserComplete);
        }

        private void AddUserComplete(int hResult, XUserHandle userHandle)
        {
            if (HR.FAILED(hResult))
            {
                Debug.LogWarning($"{nameof(SDK.XUserAddAsync)} failed - 0x{hResult:X8}.");
                return;
            }

            ClearUser();
            UserHandle = userHandle;

            hResult = SDK.XBL.XblContextCreateHandle(UserHandle, out XblContextHandle xblContextHandle);
            if (HR.FAILED(hResult))
            {
                Debug.LogError($"{nameof(SDK.XBL.XblContextCreateHandle)} failed - 0x{hResult:X8}.");
            }
            ContextHandle = xblContextHandle;

            hResult = SDK.XUserGetId(UserHandle, out ulong xuid);
            if (HR.FAILED(hResult))
            {
                Debug.LogError($"{nameof(SDK.XUserGetId)} failed - 0x{hResult:X8}.");
            }
            UserId = xuid;

            // Retrieve the Xbox user's gamer tag.
            hResult = SDK.XUserGetGamertag(UserHandle, XUserGamertagComponent.Classic, out string gamertag);
            if (HR.FAILED(hResult))
            {
                Debug.LogError($"{nameof(SDK.XUserGetGamertag)} failed - 0x{hResult:X8}.");
            }
            GamerTag = gamertag;
            GamerTagRetrieved?.Invoke();

            SDK.XUserGetGamerPictureAsync(UserHandle, XUserGamerPictureSize.Small, GetGamerPictureCallback);

            SDK.XUserRegisterForChangeEvent(UserChangeEventCallback, out _registrationToken);

            Debug.Log($"User sign in detected.");
            UserSignedIn?.Invoke();
        }

        private void GetGamerPictureCallback(int hResult, byte[] buffer)
        {
            Debug.Log($"{nameof(SDK)}.{nameof(SDK.XUserGetGamerPictureAsync)}.Callback()");

            if (HR.FAILED(hResult))
            {
                Debug.LogWarning($"{nameof(SDK.XUserGetGamerPictureAsync)} failed - 0x{hResult:X8}.");
                return;
            }

            GamerPicture = new Texture2D(64, 64);
            GamerPicture.LoadImage(buffer);
            GamerPicture.Apply();

            GamerPictureRetrieved?.Invoke();
        }

        private void UserChangeEventCallback(IntPtr _, XUserLocalId userLocalId, XUserChangeEvent eventType)
        {
            if (eventType == XUserChangeEvent.SignedOut)
            {
                OnUserSignOutCompleted(UserHandle);
            }
            else if (eventType == XUserChangeEvent.SigningOut)
            {
                OnUserSigningOut(UserHandle);
            }
        }

        private void OnUserSignOutCompleted(XUserHandle userHandle)
        {
            if (userHandle !=  null && UserHandle != null)
            {
                // Check if the signed-out user was the active game user.
                SDK.XUserCompare(userHandle, UserHandle, out int result);
                if (result == 0) // users are the same
                {
                    UserSignedOut?.Invoke();
                    ResetUserDetails();

                    // This sample requires that an account always be signed-into the game,
                    // so if the active user is lost, show the account picker to select a new account.
                    Debug.Log($"Active user signed out, select a user to continue.");
                    SignIn(true);
                }
            }
        }

        void OnUserSigningOut(XUserHandle user)
        {
            if (user != null && UserHandle != null)
            {
                // Check if the user signing out is the active game user.
                SDK.XUserCompare(user, UserHandle, out int result);

                if (result == 0) // users are the same
                {
                    Debug.Log($"Active user signing out.");
                    UserSignOutStarted?.Invoke();
                }
            }
        }

        private void ResetUserDetails()
        {
            GamerTag = "";
            GamerTagRetrieved?.Invoke();

            GamerPicture = new Texture2D(64, 64);
            GamerPicture.Apply();
            GamerPictureRetrieved?.Invoke();
        }

        private void ClearUser()
        {
            if (_registrationToken != null)
            {
                SDK.XUserUnregisterForChangeEvent(_registrationToken);
                _registrationToken = null;
            }

            if (ContextHandle != null)
            {
                SDK.XBL.XblContextCloseHandle(ContextHandle);
                ContextHandle = null;
            }

            if (UserHandle != null)
            {
                SDK.XUserCloseHandle(UserHandle);
                UserHandle = null;
            }

            if (_registrationToken != null)
            {
                SDK.XUserUnregisterForChangeEvent(_registrationToken);
            }

            GamerTag = "";
            UserId = 0;
        }

        private void OnDestroy()
        {
            ClearUser();
        }
    }
}
