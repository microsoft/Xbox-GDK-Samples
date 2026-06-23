//--------------------------------------------------------------------------------------
// Main.cs
//
// Root level sample logic.
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
// to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

using Unity.XGamingRuntime;
using UnityEngine;
using UnityEngine.Assertions;
using GdkSample_UnityRumblePlayFabMultiplayer;
using PlayFab;

public class Main : MonoBehaviour
{
    public enum MainState : int
    {
        Starting = 0,
        Running = 1,
        Suspending = 2,
        Suspended = 3,
        Resuming = 4,
    }

    public XboxLiveLogic XboxLive;
    public BaseMultiplayerLogic MultiplayerLogic;
    public SessionNetwork Networking;

    public ScreenManager Screens;
    public GameController TheGameController;
    public TMPro.TMP_Text StatusText;

    public MainState CurrentState { get; private set; }

    private void OnValidate()
    {
        Assert.IsNotNull(XboxLive);
        Assert.IsNotNull(MultiplayerLogic);
        Assert.IsNotNull(Networking);

        Assert.IsNotNull(Screens);
        Assert.IsNotNull(TheGameController);
        Assert.IsNotNull(StatusText);
    }

    void Awake()
    {
        Screens.gameObject.SetActive(true);
    }

    void Start()
    {
        Debug.Log("Main.Start()");

        CurrentState = MainState.Starting;

        TheGameController.gameObject.SetActive(false);

        Screens.Initialize();

        if (!GDKGameRuntime.Initialized)
        {
            // Initializes GDK runtime and Xbox Live
            GDKGameRuntime.TryInitialize();
        }

        Screens.UserStartupScreen.UserStartupCompleted += HandleUserStartupCompleted;
        Screens.MainMenuScreen.FindingGameCompleted += HandleFindGameCompleted;
        Screens.MainMenuScreen.HostingGameCompleted += HandleHostingGameCompleted;
        Screens.MainMenuScreen.JoiningGameCompleted += HandleJoiningGameCompleted;
        Screens.MainMenuScreen.FriendLobbiesObtained += HandleFriendLobbiesObtained;
        Screens.MainMenuScreen.LobbyBrowserOpened += HandleLobbyBrowserOpened;
        Screens.JoinFriendScreen.JoiningFriendCompleted += HandleJoiningFriendCompleted;
        Screens.JoinFriendScreen.JoiningFriendCancelled += HandleJoiningFriendCancelled;
        Screens.GameLobbyScreen.LeaveLobbyCompleted += HandleLeaveLobbyCompleted;
        Screens.GameLobbyScreen.TransitionToGameCompleted += HandleTransitionToGameCompleted;
        Screens.GamePlayScreen.GameOver += HandleGameOver;
        Screens.GamePlayScreen.QuitGameCompleted += HandleQuitGameCompleted;
        Screens.GamePlayScreen.QuitMatchCompleted += HandleQuitMatchCompleted;
        Screens.LobbyBrowserScreen.LobbyBrowserClosed += HandleLobbyBrowserClosed;
        Screens.LobbyBrowserScreen.JoinGameCompleted += HandleJoiningGameCompleted;

        HandleResume(0);

#if (UNITY_GAMECORE_XBOXONE || UNITY_GAMECORE_SCARLETT)
        // On console, PlayerPrefs must be initialized asynchronously before use
        StartCoroutine(UnityEngine.GameCore.PlayerPrefs.InitializeAsync(() => { Debug.Log("PlayerPrefs initialized"); }));
#if (UNITY_2021_3)
        UnityEngine.GameCore.GameCorePLM.OnResumingEvent += HandleResume;
        UnityEngine.GameCore.GameCorePLM.OnSuspendingEvent += HandleSuspend;
#else
        UnityEngine.WindowsGames.WindowsGamesPLM.OnResumingEvent += HandleResume;
        UnityEngine.WindowsGames.WindowsGamesPLM.OnSuspendingEvent += HandleSuspend;
#endif
#endif
    }

    private void OnDestroy()
    {
        HandleSuspend();

        SDK.CloseDefaultXTaskQueue();
        SDK.XGameRuntimeUninitialize();
    }

    private void HandleResume(double secondsSuspended)
    {
        CurrentState = MainState.Resuming;

        XboxLive.Initialize();
        Screens.SwitchTo<PressStartScreen>();

        CurrentState = MainState.Running;
    }

    private void HandleSuspend()
    {
        CurrentState = MainState.Suspending;

        Screens.SwitchTo<PressStartScreen>();
        Networking.StopNetworking();
        MultiplayerLogic.CleanupMultiplayer();
        XboxLive.Cleanup();

        CurrentState = MainState.Suspended;
    }

    private void HandleUserStartupCompleted()
    {
        Screens.SwitchTo<MainMenuScreen>();
    }

    private void HandleFindGameCompleted()
    {
        Screens.SwitchTo<GameLobbyScreen>();
    }

    private void HandleHostingGameCompleted()
    {
        Screens.SwitchTo<GameLobbyScreen>();
    }

    private void HandleFriendLobbiesObtained()
    {
        Screens.SwitchTo<JoinFriendScreen>();
    }

    private void HandleJoiningGameCompleted()
    {
        Screens.SwitchTo<GameLobbyScreen>();
    }

    private void HandleJoiningFriendCompleted()
    {
        Screens.SwitchTo<GameLobbyScreen>();
    }

    private void HandleJoiningFriendCancelled()
    {
        Screens.SwitchTo<MainMenuScreen>();
    }

    private void HandleLeaveLobbyCompleted()
    {
        Screens.SwitchTo<MainMenuScreen>();
    }

    private void HandleTransitionToGameCompleted()
    {
        Screens.SwitchTo<GamePlayScreen>();
    }

    private void HandleGameOver()
    {
        Screens.SwitchTo<MainMenuScreen>();
    }

    private void HandleQuitGameCompleted()
    {
        Screens.SwitchTo<MainMenuScreen>();
    }

    private void HandleQuitMatchCompleted()
    {
        Screens.SwitchTo<MainMenuScreen>();
    }

    private void HandleLobbyBrowserClosed()
    {
        Screens.SwitchTo<MainMenuScreen>();
    }
    
    private void HandleLobbyBrowserOpened()
    {
        Screens.SwitchTo<LobbyBrowserScreen>();
    }
}
