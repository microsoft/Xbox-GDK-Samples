using System.Collections.Generic;
using Unity.XGamingRuntime;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace GdkSample_SimplePLM
{
    public sealed class SimplePLMSceneManager : MonoBehaviour
    {
        [SerializeField] GameObject MainMenu;
        [SerializeField] Button FirstSelectedButton;
        [SerializeField] Button ExitButton;
        [SerializeField] Button ShowPlayerProfileButton;
        [SerializeField] Button ShowMessageDialogButton;
        [SerializeField] Button LaunchSettingsButton;

        private List<Button> _buttonList;

        public static SimplePLMSceneManager Instance { get; private set; }

        private void Awake()
        {
            if (Instance != null)
            {
                Destroy(this);
                return;
            }

            Instance = this;
        }

        // Start is called before the first frame update.
        void Start()
        {
            XboxManager.Instance.UserSignedIn += OnSignInDetected;
            XboxManager.Instance.UserSignedOut += OnUserChangeDetected;
            XboxManager.Instance.UserSignInStarted += OnUserChangeDetected;
            XboxManager.Instance.UserSignOutStarted += OnUserChangeDetected;

            // Button handlers for onClick events
            ShowPlayerProfileButton.onClick.AddListener(() => ShowPlayerProfile());
            ShowMessageDialogButton.onClick.AddListener(() => ShowMessageDialog());
            LaunchSettingsButton.onClick.AddListener(() => LaunchSettings());

            _buttonList = new List<Button>(MainMenu.GetComponentsInChildren<Button>());
            DisableButtons();

            // Initialize GDK runtime
            if (GDKGameRuntime.TryInitialize())
            {
                XboxManager.Instance.InitializeAndAddUser();
            }
        }

        private void ShowPlayerProfile()
        {
            string messageText = $"Calling XGameUiShowPlayerProfileCardAsync(). Your game will lose focus.";
#if (UNITY_GAMECORE_XBOXONE || UNITY_GAMECORE_SCARLETT)
            messageText = messageText = $"Calling XGameUiShowPlayerProfileCardAsync(). Your game will be constained and mostly visible.";
#endif
            Debug.Log(messageText);

            int hr = SDK.XGameUiShowPlayerProfileCardAsync(
                XboxManager.Instance.UserHandle,
                XboxManager.Instance.UserId,
                (int hResult) =>
                {
                    if (hResult == -2147467260) // E_ABORT occurs when the user closes the profile card
                    {
                        Debug.Log("Player profile card was dismissed by the user.");
                    }
                    else if (hResult == -2147024567) // E_OPERATIONINPROGRESS
                    {
                        Debug.Log("Opening player profile card...");
                    }
                    else if (HR.FAILED(hResult))
                    {
                        Debug.LogWarning($"Failed to show player profile card. Error: 0x{hResult:X8}");
                    }
                });
        }

        private void ShowMessageDialog()
        {
            string messageText = $"Calling XGameUiShowMessageDialogAsync(). Your game will lose focus.";
            string dialogMessage = "Your game has lost focus.";
#if (UNITY_GAMECORE_XBOXONE || UNITY_GAMECORE_SCARLETT)
            messageText = messageText = $"Calling XGameUiShowPlayerProfileCardAsync(). Your game will be constrained and partially visible.";
            dialogMessage = "Your game is now constrained.";
#endif
            Debug.Log(messageText);

            int hr = SDK.XGameUiShowMessageDialogAsync(
                "Message Title",
                dialogMessage,
                "Button1",
                "Button2",
                "Button3",
                XGameUiMessageDialogButton.First,
                XGameUiMessageDialogButton.Third,
                (int hResult, XGameUiMessageDialogButton resultButton) =>
                {
                    if (hResult == -2147467260) // E_ABORT occurs when the user closes the dialog
                    {
                        Debug.Log("Message dialog was dismissed by the user.");
                    }
                    else if (HR.FAILED(hResult))
                    {
                        Debug.LogWarning($"Failed to show message dialog. Error: 0x{hResult:X8}");
                    }
                    else
                    {
                        Debug.Log($"Message dialog closed. Button '{resultButton}' selected");
                    }
                });
        }

        private void LaunchSettings()
        {
            string settingsUri = "ms-settings:";
            string messageText = $"Calling XLaunchUri() to launch into settings. Your game will lose focus.";
#if (UNITY_GAMECORE_XBOXONE || UNITY_GAMECORE_SCARLETT)
            settingsUri = "settings:";
            messageText = $"Calling XLaunchUri() to launch into settings. Your game will be constrained and not visible. If the game is not visible for 10 minutes it will be suspended.";
#endif
            Debug.Log(messageText);
            int hr = SDK.XLaunchUri(XboxManager.Instance.UserHandle, settingsUri);
            if (HR.FAILED(hr))
            {
                Debug.LogWarning($"Failed to launch settings. Error: 0x{hr:X8}");
            }
        }

        private void OnSignInDetected()
        {
            EnableButtons();
            EventSystem.current.SetSelectedGameObject(FirstSelectedButton.gameObject);
        }

        private void OnUserChangeDetected()
        {
            DisableButtons();
        }

        private void EnableButtons()
        {
            foreach(Button button in _buttonList)
            {
                button.interactable = true;
            }   
        }

        private void DisableButtons()
        {
            foreach(Button button in _buttonList)
            {
                button.interactable = false;
            }

            // Exit button should always be enabled
            ExitButton.interactable = true;
        }

    }
}
