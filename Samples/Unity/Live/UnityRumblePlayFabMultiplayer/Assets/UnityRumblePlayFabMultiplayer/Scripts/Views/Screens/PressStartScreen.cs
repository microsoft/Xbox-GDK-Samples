//--------------------------------------------------------------------------------------
// ScreenManager.cs
//
// The UI screen class that handles the functionality for logging the user in.
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

using System;
using UnityEngine;
using UnityEngine.Assertions;
using UnityEngine.UI;

public class PressStartScreen : BaseUiScreen
{
    public event Action UserStartupCompleted;

    public Button StartButton;
    public GamerProfile GamerProfile;

    public void HandleStartButtonPressed()
    {
        Debug.Log("UserStartupScreen.HandleStartButtonPressed()");

        Disable();
        
        // sign in to Xbox Live
        XboxLive.OnUserLoggedIn += HandleLiveUserSignedIn;
        XboxLive.OnUserLoginError += (error, hr)=> { HandleError($"Logging into Xbox Live failed with HRESULT: {hr.ToString("X8")}"); };
        AsyncOpUI.Started(@"Logging in to Xbox Live...");
        XboxLive.LoginLiveUser(true);        
    }

    protected override void OnValidate()
    {
        base.OnValidate();

        Assert.IsNotNull(StartButton);
        Assert.IsNotNull(GamerProfile);
    }

    protected override void OnShown()
    {
        GamerProfile.Hide();

        if(Networking != null)
        {
            Networking.InitializePlayFabParty();
        }

        base.OnShown();
    }

    protected override void OnEnabled()
    {
        StartButton.interactable = true;

        base.OnEnabled();
    }

    protected override void OnDisabled()
    {
        base.OnDisabled();

        StartButton.interactable = false;
    }

    private void HandleLiveUserSignedIn()
    {
        Debug.Log("UserStartupScreen.HandleLiveUserSignedIn()");

        AsyncOpUI.Finished();
        XboxLive.ClearEventHandlers();

        // get the social manager started
        XboxLive.InitializeSocial();

        // get the Live user token
        XboxLive.OnUserTokenReceived += HandleUserTokenReceived;
        XboxLive.OnGeneralError += (message, hr) => { HandleError($"Getting the Xbox Live user token failed with HRESULT:{hr.ToString("X8")}"); };
        AsyncOpUI.Started(@"Requesting Xbox Live user token...");
        XboxLive.RequestLiveUserToken(false);
    }

    private void HandleUserTokenReceived()
    {
        Debug.Log("UserStartupScreen.HandleUserTokenReceived()");

        AsyncOpUI.Finished();
        XboxLive.ClearEventHandlers();

        // notify that we should switch away from this screen
        MultiplayerLogic.OnMultiplayerInitialized += HandleMultiplayerInitialized;
        MultiplayerLogic.OnGeneralError += (message, hr) => { HandleError($"Initializing Xbox Live mutiplayer failed with HRESULT:{hr.ToString("X8")}"); };
        AsyncOpUI.Started(@"Initializing Xbox Live Multiplayer...");
        MultiplayerLogic.InitializeMultiplayer();
    }

    private void HandleMultiplayerInitialized()
    {
        Debug.Log("UserStartupScreen.HandleMultiplayerInitialized()");

        AsyncOpUI.Finished();
        MultiplayerLogic.ClearEventHandlers();

        GamerProfile.Show();

        UserStartupCompleted?.Invoke();
    }

    private void HandleError(string errorMessage, params object[] args)
    {
        AsyncOpUI.Finished();
        XboxLive.ClearEventHandlers();
        MultiplayerLogic.ClearEventHandlers();

        Debug.LogErrorFormat(errorMessage, args);
        StatusBarText.text = string.Format(errorMessage, args);

        Enable();
    }
}
