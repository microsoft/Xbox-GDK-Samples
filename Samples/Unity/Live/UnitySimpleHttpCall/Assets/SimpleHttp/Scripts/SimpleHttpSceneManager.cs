using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using Unity.XGamingRuntime;
using XBL = Unity.XGamingRuntime.SDK.XBL;

namespace GdkSample_SimpleHttp
{
    public sealed class SimpleHttpSceneManager : MonoBehaviour
    {
        [SerializeField] GameObject MainMenu;
        [SerializeField] Button FirstSelectedButton;
        [SerializeField] Button ExitButton;
        [SerializeField] Button SendXblRequestAsBytesButton;
        [SerializeField] Button SendXblRequestAsStringButton;

        private readonly string _xblWebAddress = "https://profile.xboxlive.com/users/me/profile/settings?settings=GameDisplayName";
        private readonly string _method = "GET";
        private readonly string _headerName = "X-XBL-Contract-Version";
        private readonly string _headerValue = "3";

        private List<Button> _buttonList;

        public static SimpleHttpSceneManager Instance { get; private set; }

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

            // Handle button onclick events
            SendXblRequestAsBytesButton.onClick.AddListener(() =>
            {
                string body = "This is a request body as bytes.";
                SendHttpCallRequestBytes(_xblWebAddress, _method, _headerName, _headerValue, System.Text.Encoding.UTF8.GetBytes(body));
            });

            SendXblRequestAsStringButton.onClick.AddListener(() =>
            {
                string body = "This is a request body as a string.";
                SendHttpCallRequestString(_xblWebAddress, _method, _headerName, _headerValue, body);
            });

            _buttonList = new List<Button>(MainMenu.GetComponentsInChildren<Button>());
            DisableButtons();

            // Initialize GDK runtime and Xbox Live
            if (GDKGameRuntime.TryInitialize())
            {
                XboxManager.Instance.InitializeAndAddUser();
            }
        }

        public void SendHttpCallRequestBytes(string address, string method, string headerName, string headerValue, byte[] body)
        {
            Logger.Instance.Log($"Calling SendHttpCallRequestBytes()", LogColor.Event);

            Debug.Log($"Attempting to create the HTTP request for '{address}' using method type '{method}'.");

            // Create the HTTP request
            int hr = XBL.XblHttpCallCreate(
               XboxManager.Instance.ContextHandle,  // Xbox content handle
               method,                              // method (GET, PUT, POST, etc)
               address,                             // http web address
               out XblHttpCallHandle httpHandle);   // handle for sending

            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK)}.{nameof(XBL)}.{nameof(XBL.XblHttpCallCreate)} failed - 0x{hr:X8}.");
                return;
            }

            Debug.Log($"Attempting to set header name as '{headerName}' & header value as '{headerValue}'.");

            // Set the HTTP request header
            hr = XBL.XblHttpCallRequestSetHeader(
                httpHandle,                         // existing http handle
                headerName,                         // http header name
                headerValue,                        // http header value
                true);                              // allow tracing

            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK)}.{nameof(XBL)}.{nameof(XBL.XblHttpCallRequestSetHeader)} failed - 0x{hr:X8}.");
                XBL.XblHttpCallCloseHandle(httpHandle);
                return;
            }

            Debug.Log($"Attempting to set body to '{System.Text.Encoding.UTF8.GetString(body)}'.");

            // Set the HTTP request's body using bytes
            hr = XBL.XblHttpCallRequestSetRequestBodyBytes(
                httpHandle,                         // existing http handle
                body);                              // body request, in bytes

            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK)}.{nameof(XBL)}.{nameof(XBL.XblHttpCallRequestSetRequestBodyBytes)} failed - 0x{hr:X8}.");
                XBL.XblHttpCallCloseHandle(httpHandle);
                return;
            }

            Debug.Log($"Sending the request...");

            // Send the HTTP request
            XBL.XblHttpCallPerformAsync(
                httpHandle,                         // existing http handle
                XblHttpCallResponseBodyType.Vector, // signify request is in bytes
                (int hr) =>                         // completion callback
                {
                    Debug.Log($"{nameof(SDK)}.{nameof(XBL)}.{nameof(XBL.XblHttpCallPerformAsync)}.Callback()");

                    if (HR.FAILED(hr))
                    {
                        Debug.LogError($"{nameof(SDK)}.{nameof(XBL)}.{nameof(XBL.XblHttpCallPerformAsync)} failed - 0x{hr:X8}.");
                        XBL.XblHttpCallCloseHandle(httpHandle);
                        return;
                    }

                    Logger.Instance.Log($"{method} request sent to address '{address}'.", LogColor.Success);

                    hr = XBL.XblHttpCallGetResponseString(httpHandle, out string response);

                    if (HR.FAILED(hr))
                    {
                        Debug.LogError($"{nameof(SDK)}.{nameof(XBL)}.{nameof(XBL.XblHttpCallGetResponseString)} failed - 0x{hr:X8}.");
                        XBL.XblHttpCallCloseHandle(httpHandle);
                        return;
                    }

                    Logger.Instance.Log($"Response recieved from address '{address}'.", LogColor.Success);
                    Debug.Log($"Response: {response}");

                    XBL.XblHttpCallCloseHandle(httpHandle);
                });
        }

        public void SendHttpCallRequestString(string address, string method, string headerName, string headerValue, string body)
        {
            Logger.Instance.Log($"Calling SendHttpCallRequestString()", LogColor.Event);

            Debug.Log($"Attempting to create the HTTP request for '{address}' using method type '{method}'.");

            // Create the HTTP request
            int hr = XBL.XblHttpCallCreate(
               XboxManager.Instance.ContextHandle,  // Xbox content handle
               method,                              // method (GET, PUT, POST, etc)
               address,                             // http web address
               out XblHttpCallHandle httpHandle);   // handle for sending

            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK)}.{nameof(XBL)}.{nameof(XBL.XblHttpCallCreate)} failed - 0x{hr:X8}.");
                return;
            }

            Debug.Log($"Attempting to set header name as '{headerName}' & header value as '{headerValue}'.");

            // Set the HTTP request header
            hr = XBL.XblHttpCallRequestSetHeader(
                httpHandle,                         // existing http handle
                headerName,                         // http header name
                headerValue,                        // http header value
                true);                              // allow tracing

            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK)}.{nameof(XBL)}.{nameof(XBL.XblHttpCallRequestSetHeader)} failed - 0x{hr:X8}.");
                XBL.XblHttpCallCloseHandle(httpHandle);
                return;
            }

            Debug.Log($"Attempting to set body to '{body}'.");

            // Set the HTTP request's body using bytes
            hr = XBL.XblHttpCallRequestSetRequestBodyString(
                httpHandle,                         // existing http handle
                body);                              // body request

            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK)}.{nameof(XBL)}.{nameof(XBL.XblHttpCallRequestSetRequestBodyString)} failed - 0x{hr:X8}.");
                XBL.XblHttpCallCloseHandle(httpHandle);
                return;
            }

            Debug.Log($"Sending request...");

            // Send the HTTP request
            XBL.XblHttpCallPerformAsync(
                httpHandle,                         // existing http handle
                XblHttpCallResponseBodyType.String, // signify request is a string
                (int hr) =>                         // completion callback
                {
                    Debug.Log($"{nameof(SDK)}.{nameof(XBL)}.{nameof(XBL.XblHttpCallPerformAsync)}.Callback()");

                    if (HR.FAILED(hr))
                    {
                        Debug.LogError($"{nameof(SDK)}.{nameof(XBL)}.{nameof(XBL.XblHttpCallPerformAsync)} failed - 0x{hr:X8}.");
                        XBL.XblHttpCallCloseHandle(httpHandle);
                        return;
                    }

                    Logger.Instance.Log($"{method} request sent to address '{address}'.", LogColor.Success);

                    hr = XBL.XblHttpCallGetResponseString(httpHandle, out string response);

                    if (HR.FAILED(hr))
                    {
                        Debug.LogError($"{nameof(SDK)}.{nameof(XBL)}.{nameof(XBL.XblHttpCallGetResponseString)} failed - 0x{hr:X8}.");
                        XBL.XblHttpCallCloseHandle(httpHandle);
                        return;
                    }

                    Logger.Instance.Log($"Response recieved from address '{address}'.", LogColor.Success);
                    Debug.Log($"Response: {response}");

                    XBL.XblHttpCallCloseHandle(httpHandle);
                });
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
