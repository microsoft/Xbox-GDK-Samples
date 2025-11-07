using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using Unity.XGamingRuntime;

namespace GdkSample_SimpleWebSockets
{
    public enum WebSocketStatus
    {
        Uninitialized,
        Connecting,
        Connected,
        Closed
    }

    public sealed class SimpleWebSocketsSceneManager : MonoBehaviour
    {
        [SerializeField] GameObject MainMenu;
        [SerializeField] Button FirstSelectedButton;
        [SerializeField] Button ExitButton;
        [SerializeField] Button ConnectButton;
        [SerializeField] Button DisconnectButton;
        [SerializeField] Button SendBinaryButton;
        [SerializeField] Button SendStringButton;

        private List<Button> _buttonList;

        private HCWebsocketHandle _webSocket;
        private WebSocketStatus _lastWebSocketStatus = WebSocketStatus.Uninitialized;
        private byte[] _binaryMessage = null;
        private string _message = null;

        private bool _closedFlag = false;
        private int _closedHr = 0;
        private HCWebSocketCloseStatus _webSocketCloseStatus = HCWebSocketCloseStatus.Normal;

        private readonly string _address = "https://ws.ifelse.io";
        private readonly string _protocol = "";
        private readonly string _sendBytesMessage = "Hello World (bytes)!";
        private readonly string _sendStringMessage = "Hello World (string)!";

        public static SimpleWebSocketsSceneManager Instance { get; private set; }

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
            ConnectButton.onClick.AddListener(() => ConnectWebSocket(_address, _protocol));
            DisconnectButton.onClick.AddListener(() => DisconnectWebSocket());
            SendBinaryButton.onClick.AddListener(() => SendWebSocketBinaryMessage(System.Text.Encoding.UTF8.GetBytes(_sendBytesMessage), (uint)_sendBytesMessage.Length));
            SendStringButton.onClick.AddListener(() => SendWebSocketMessage(_sendStringMessage));

            _buttonList = new List<Button>(MainMenu.GetComponentsInChildren<Button>());
            DisableButtons();

            // Initialize GDK runtime and Xbox Live
            if (GDKGameRuntime.TryInitialize())
            {
                XboxManager.Instance.InitializeAndAddUser();
            }
        }

        private void Update()
        {
            // Provide log output based on web socket events

            if (_binaryMessage != null)
            {
                Debug.Log($"Received binary message: '{System.Text.Encoding.UTF8.GetString(_binaryMessage)}'.");
                _binaryMessage = null;
            }
            if (_message != null)
            {
                Debug.Log($"Received message: '{_message}'.");
                _message = null;
            }
            if (_closedFlag)
            {
                Debug.Log("Web socket successfully closed.");
                _closedFlag = false;
            }
            if (HR.FAILED(_closedHr))
            {
                Debug.LogWarning($"{nameof(SDK)}.{nameof(SDK.HCWebSocketCloseHandle)} failed - 0x{_closedHr:X8}.");
                _closedHr = 0;
            }
            if (_webSocketCloseStatus != HCWebSocketCloseStatus.Normal)
            {
                Debug.LogWarning($"Web socket closed with abnormal status: {Enum.GetName(typeof(HCWebSocketCloseStatus), _webSocketCloseStatus)}.");
                _webSocketCloseStatus = HCWebSocketCloseStatus.Normal;
            }
        }

        public void ConnectWebSocket(string address, string protocol)
        {
            Debug.Log($"Calling ConnectWebSocket()");

            if (_webSocket == null || 
                _lastWebSocketStatus == WebSocketStatus.Uninitialized || 
                _lastWebSocketStatus == WebSocketStatus.Closed)
            {
                Debug.Log($"Attempting to create the web socket...");

                // Create the http client web socket
                int hr = SDK.HCWebSocketCreate(
                    out _webSocket,            // web socket that will be connected to
                    OnStringMessageReceived,   // incoming message handler
                    OnBinaryMessageReceived,   // incoming binary message handler
                    OnWebSocketClosed);        // closing web socket handler

                if (HR.FAILED(hr))
                {
                    Debug.LogError($"{nameof(SDK)}.{nameof(SDK.HCWebSocketCreate)} failed - 0x{hr:X8}.");
                    return;
                }

                Debug.Log($"Web socket successfully created.");
                Debug.Log($"Attempting to connect the web socket to '{address}'...");

                // Connect to the web endpoint
                hr = SDK.HCWebSocketConnectAsync(
                    address,        // web address endpoint
                    protocol,       // security protocol
                    _webSocket,     // created web socket
                    (HCWebsocketHandle webSocket, int errorCode, uint platformErrorCode) =>
                    {
                        Debug.Log($"{nameof(SDK)}.{nameof(SDK.HCWebSocketConnectAsync)}.Callback()");

                        if (HR.FAILED(errorCode))
                        {
                            _lastWebSocketStatus = WebSocketStatus.Uninitialized;
                            Debug.LogError($"{nameof(SDK)}.{nameof(SDK.HCWebSocketCreate)} failed - errorCode: 0x{errorCode:X8} & platformErrorCode: 0x{platformErrorCode:X8}.");
                            return;
                        }

                        // web socket has been successfully connected
                        _lastWebSocketStatus = WebSocketStatus.Connected;
                        
                        Debug.Log($"Web socket successfully connected to '{address}'.");
                    });

                if (HR.FAILED(hr))
                {
                    Debug.LogError($"{nameof(SDK)}.{nameof(SDK.HCWebSocketConnectAsync)} failed - 0x{hr:X8}.");
                    return;
                }

                _lastWebSocketStatus = WebSocketStatus.Connecting;
            }
            else
            {
                Debug.LogWarning($"Web socket has been already been created & connected. Close the web socket to re-create.");
            }
        }

        public void SendWebSocketBinaryMessage(byte[] bytes, uint length)
        {
            Debug.Log($"Calling SendWebSocketBinaryMessage()");

            if (_lastWebSocketStatus != WebSocketStatus.Connected)
            {
                Debug.LogWarning($"Web socket status is currently set to '{Enum.GetName(typeof(WebSocketStatus), _lastWebSocketStatus)}'. Please ensure the web socket is connected. Binary message was not sent.");
                return;
            }

            Debug.Log($"Sending binary message '{System.Text.Encoding.UTF8.GetString(bytes)}' to the connected web socket...");

            // Send a binary message to the connected web socket
            int hr = SDK.HCWebSocketSendBinaryMessageAsync(
                _webSocket,    // connected web socket
                bytes,          // message, in bytes, to be sent
                length,         // message length
                (HCWebsocketHandle webSocket, int errorCode, uint platformErrorCode) =>
                {
                    Debug.Log($"{nameof(SDK)}.{nameof(SDK.HCWebSocketSendMessageAsync)}.Callback()");

                    if (HR.FAILED(errorCode))
                    {
                        Debug.LogError($"{nameof(SDK)}.{nameof(SDK.HCWebSocketSendMessageAsync)} failed - errorCode: 0x{errorCode:X8} & platformErrorCode: 0x{platformErrorCode:X8}.");
                        return;
                    }

                    Debug.Log($"Successfully sent binary message '{System.Text.Encoding.UTF8.GetString(bytes)}' to the connected web socket.");
                });

            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK)}.{nameof(SDK.HCWebSocketSendBinaryMessageAsync)} failed - 0x{hr:X8}.");
            }
        }

        public void SendWebSocketMessage(string message)
        {
            Debug.Log($"Calling SendWebSocketMessage()");

            if (_lastWebSocketStatus != WebSocketStatus.Connected)
            {
                Debug.LogWarning($"Web socket status is currently set to '{Enum.GetName(typeof(WebSocketStatus), _lastWebSocketStatus)}'. Please ensure the web socket is connected. Message was not sent.");
                return;
            }

            Debug.Log($"Sending message '{message}' to the connected web socket...");

            // Send a message to the connected web socket
            int hr = SDK.HCWebSocketSendMessageAsync(
                _webSocket,  // connected web socket
                message,     // message to be sent
                (HCWebsocketHandle webSocket, int errorCode, uint platformErrorCode) => 
                {
                    Debug.Log($"{nameof(SDK)}.{nameof(SDK.HCWebSocketSendMessageAsync)}.Callback()");

                    if (HR.FAILED(errorCode))
                    {
                        Debug.LogError($"{nameof(SDK)}.{nameof(SDK.HCWebSocketSendMessageAsync)} failed - errorCode: 0x{errorCode:X8} & platformErrorCode: 0x{platformErrorCode:X8}.");
                        return;
                    }

                    Logger.Instance.Log($"Successfully sent message '{message}' to the connected web socket.", LogColor.Success);
                });

            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK)}.{nameof(SDK.HCWebSocketSendMessageAsync)} failed - 0x{hr:X8}.");
            }
        }

        public void DisconnectWebSocket()
        {
            Debug.Log($"Calling DisconnectWebSocket()");

            if (_webSocket != null && _lastWebSocketStatus == WebSocketStatus.Connected)
            {
                Debug.Log($"Attempting to disconnect the web socket...");

                // Disconnect the existing web socket
                int hr = SDK.HCWebSocketDisconnect(_webSocket);

                if (HR.FAILED(hr))
                {
                    Debug.LogError($"{nameof(SDK)}.{nameof(SDK.HCWebSocketDisconnect)} failed - 0x{hr:X8}.");
                }
            }
            else
            {
                Debug.LogWarning($"Web socket failed to disconnect. Web socket status is currently set to '{Enum.GetName(typeof(WebSocketStatus), _lastWebSocketStatus)}'.");
            }
        }

        private void OnSignInDetected()
        {
            EnableButtons();
            EventSystem.current.SetSelectedGameObject(FirstSelectedButton.gameObject);

            //if (_lastWebSocketStatus == WebSocketStatus.Uninitialized)
            //{
            //    SDK.HCInitialize();
            //}
        }

        private void OnUserChangeDetected()
        {
            DisableButtons();
        }

        private void OnBinaryMessageReceived(HCWebsocketHandle websocket, byte[] payloadBytes)
        {
            Debug.Log($"OnBinaryMessageReceived()");
            _binaryMessage = payloadBytes;
        }

        private void OnStringMessageReceived(HCWebsocketHandle websocket, string incomingBodyString)
        {
            Debug.Log($"OnStringMessageReceived()");
            _message = incomingBodyString;
        }

        private void OnWebSocketClosed(HCWebsocketHandle websocket, HCWebSocketCloseStatus closeStatus)
        {
            Debug.Log($"OnWebSocketClosed()");

            if (_webSocket != null)
            {
                _closedHr = SDK.HCWebSocketCloseHandle(_webSocket);
                _webSocket = null;
            }

            _lastWebSocketStatus = WebSocketStatus.Closed;
            _closedFlag = true;
            _webSocketCloseStatus = closeStatus;
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
