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
using PlayFab;
using PlayFab.ClientModels;

public class UserStartupScreen : BaseUiScreen
{
    public event Action UserStartupCompleted;

    public Button StartButton;
    public GamerProfile GamerProfile;

    public void HandleStartButtonPressed()
    {
        Debug.LogFormat("UserStartupScreen.HandleStartButtonPressed()");

        Disable();
        
        // sign in to Xbox Live
        XboxLive.OnUserLoggedIn += HandleLiveUserSignedIn;
        XboxLive.OnUserLoginError += (error, hresult)=> { HandleError("Logging into Xbox Live failed: {0}", hresult.ToString("X8")); };
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
        Debug.LogFormat("UserStartupScreen.__HandleLiveUserSignedIn()");

        AsyncOpUI.Finished();

        // get the social manager started
        XboxLive.InitializeSocial();

        // get the Live user token
        XboxLive.OnUserTokenReceived += HandleUserTokenReceived;
        XboxLive.OnGeneralError += (message, hresult) => { HandleError("Getting the Xbox Live user token failed: {0}", hresult.ToString("X8")); };
        AsyncOpUI.Started(@"Requesting Xbox Live user token...");
        XboxLive.RequestLiveUserToken(false);
    }

    private void HandleUserTokenReceived()
    {
        Debug.LogFormat("UserStartupScreen.__HandleUserTokenReceived()");

        AsyncOpUI.Finished();

        // log into PlayFab with the Xbox token
        var request = new LoginWithXboxRequest { TitleId = Configuration.PLAYFAB_TITLE_ID, XboxToken = XboxLive.MyUserToken, CreateAccount = true };
        PlayFabClientAPI.LoginWithXbox(request, HandlePlayFabUserLoggedIn, HandlePlayFabError);
    }

    private void HandlePlayFabUserLoggedIn(LoginResult result)
    {
        Debug.LogFormat("UserStartupScreen.__HandlePlayFabUserLoggedIn()");

        AsyncOpUI.Finished();

        // notify that we should switch away from this screen
        XboxLive.OnMultiplayerInitialized += HandleMultiplayerInitialized;
        XboxLive.OnGeneralError += (message, hresult) => { HandleError("Initializing Xbox Live mutiplayer failed: {0}", hresult.ToString("X8")); };
        AsyncOpUI.Started(@"Initializing Xbox Live Multiplayer...");
        XboxLive.InitializeMultiplayer();
    }

    private void HandlePlayFabError(PlayFabError error)
    {
        AsyncOpUI.Finished();
        Debug.LogErrorFormat(error.ErrorMessage);
        StatusBarText.text = string.Format(error.ErrorMessage);

        Enable();
    }

    private void HandleMultiplayerInitialized()
    {
        Debug.LogFormat("UserStartupScreen.__HandleMultiplayerInitialized()");

        AsyncOpUI.Finished();
        XboxLive.OnMultiplayerInitialized -= HandleMultiplayerInitialized;

        GamerProfile.Show();

        UserStartupCompleted?.Invoke();
    }

    private void HandleError(string errorMessage, params object[] args)
    {
        AsyncOpUI.Finished();

        Debug.LogErrorFormat(errorMessage, args);
        StatusBarText.text = string.Format(errorMessage, args);

        Enable();
    }
}
