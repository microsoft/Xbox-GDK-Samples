//--------------------------------------------------------------------------------------
// XboxLiveLogic.cs
//
// Logic for Xbox Live that includes the user login process.
//
// MIT License

// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
// to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.

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
using Unity.XGamingRuntime;
using HR = Unity.XGamingRuntime.Interop.HR;
using GdkSample_UnityRumble;

public partial class XboxLiveLogic : MonoBehaviour
{
    // this case is when silent authentication has failed because there is some issue
    // with silently logging in a default user
    public const int E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED = -1994108670;//0x89245102

    // this case is when silent authentication has failed because there is currently no
    // default user that has been logged in...
    public const int E_GAMEUSER_NO_DEFAULT_USER = -1994108666;//0x89245106

    // this case is when the title has not been fully made Live enabled (it is not configured
    // to be "full trust" and has not been deployed to an environment as such...
    public const int E_GAMEUSER_UNKNOWN_GAME_IDENTITY = -1994108663;//0x89245109

    // this case is when the title has not been packaged and deployed to the 
    // environment store properly...
    public const int E_GAMEUSER_NO_PACKAGE_IDENTITY = -1994108656;//0x89245110

    // this case is when the wrong environment is attempted, or is unpublished (check the 
    // current sandbox against the sandboxes authorized for the product)...
    public const int E_CONTENT_ISOLATION = -2146051054;//0x8015DC12

    // this case is when the user has elected to not sign into Live and
    // therefore the app is left in an unlogged-in state...
    public const int E_ABORT = -2147467260;//0x80004004

    private XUserChangeRegistrationToken _registrationToken = null;
    private bool _isInitialized = false;

    public enum LoginError : int
    {
        FullAuthImmediateFailure,
        TitleIsNotLiveEnabled,
        TitleIsInWrongEnvironment,
        TitleIsNotDeployedProperly,
        AuthWasAborted,
        UnhandledOrUnknownHresultError
    }

    public event Action OnUserTokenReceived;
    public event Action OnUserLoggedIn;
    public event Action OnUserLoggedOut;
    // parameters are: LoginError, hresult
    public event Action<LoginError, int> OnUserLoginError;
    // parameters are: error message, hresult
    public event Action<string, int> OnGeneralError;

    public XUserHandle MyUserHandle { get; private set; }
    public ulong MyXUID { get; private set; }
    public string MyGamerTag { get; private set; }
    public string MyUserToken { get; private set; }
    public XblContextHandle MyContextHandle { get; private set; }

    public void Initialize()
    {
        Debug.LogFormat("XboxLiveLogic.Initialize()");

        // XblInitialize is called by GDKGameRuntime.cpp
        // NOTE: when running in editor, may return: 0x89235207 (AlreadyInitialized)
        //var hresult = SDK.XBL.XblInitialize(Configuration.SCID);
        //Debug.LogFormat("XblInitialize() returned {0}", hresult.ToString("X8"));

        RegisterForLoginChangeEvents();
        _isInitialized = GDKGameRuntime.Initialized;
    }

    public void ClearUserEventHandlers()
    {
        OnUserTokenReceived = null;
        OnUserLoggedIn = null;
        OnUserLoggedOut = null;
        OnUserLoginError = null;
    }

    public void ClearEventHandlers()
    {
        Debug.LogFormat("XboxLiveLogic.ClearEventHandlers()");

        // user login related
        ClearUserEventHandlers();
        OnGeneralError = null;

        // multiplayer/session related
        ClearMultiplayerEventHandlers();

        // social related
        ClearSocialEventHandlers();
}

public void LoginLiveUser(bool SupportSilentLogin)
    {
        Debug.LogFormat("XboxLiveLogic.LoginLiveUser({0})", SupportSilentLogin);

        if (SupportSilentLogin)
        {
            SDK.XUserAddAsync(XUserAddOptions.AddDefaultUserSilently, HandleSilentUserAddCompleted);
        }
        else
        {
            SDK.XUserAddAsync(XUserAddOptions.None, HandleUserAddCompleted);
        }
    }

    public void RequestLiveUserToken(bool forceRefresh)
    {
        Debug.LogFormat("XboxLiveLogic.RequestLiveUserToken({0})", forceRefresh);

        XUserGetTokenAndSignatureUtf16HttpHeader[] headers = new XUserGetTokenAndSignatureUtf16HttpHeader[1];
        headers[0] = new XUserGetTokenAndSignatureUtf16HttpHeader
        {
            Name = "Header1",
            Value = "Value1"
        };
        byte[] bodyBuffer = new byte[1];

        SDK.XUserGetTokenAndSignatureUtf16Async(
            MyUserHandle,
            forceRefresh ? XUserGetTokenAndSignatureOptions.ForceRefresh : XUserGetTokenAndSignatureOptions.None,
            @"GET",
            Configuration.PLAYFAB_TOKEN_PROVIDER_URL,
            headers,
            bodyBuffer,
            HandleUserGetTokenCompleted);
    }

    public void Cleanup()
    {
        Debug.LogFormat("XboxLiveLogic.Cleanup()");

        CleanupSocial();
        CleanupMultiplayer();

        UnregisterForLoginChangeEvents();
        FreeUserHandle();

        ClearEventHandlers();

        _isInitialized = false;
        HasMultiplayerPrivileges = false;
        ReceivedInviteSessionHandleId = null;
    }

    private void Update()
    {
        UpdateSocial();
        UpdateMultiplayer();
    }

    private void OnDestroy()
    {
        Cleanup();
    }

    private void OnApplicationPause(bool pause)
    {
        if (!_isInitialized)
        {
            return;
        }

        if (pause)
        {
            UnregisterForLoginChangeEvents();
        }
        else
        {
            RegisterForLoginChangeEvents();
        }
    }

    private void FreeUserHandle()
    {
        if (null != MyContextHandle)
        {
            SDK.XBL.XblContextCloseHandle(MyContextHandle);
            MyContextHandle = null;
        }

        if (null != MyUserHandle)
        {
            SDK.XUserCloseHandle(MyUserHandle);
            MyUserHandle = null;
            MyXUID = 0;
            MyGamerTag = string.Empty;
        }
    }

    private void RegisterForLoginChangeEvents()
    {
        if (null == _registrationToken)
        {
            SDK.XUserRegisterForChangeEvent(LoginStatusChanged, out _registrationToken);
        }
    }

    private void UnregisterForLoginChangeEvents()
    {
        if (null != _registrationToken)
        {
            SDK.XUserUnregisterForChangeEvent(_registrationToken);
            _registrationToken = null;
        }
    }

    private void LoginStatusChanged(IntPtr context, XUserLocalId userLocalId, XUserChangeEvent eventType)
    {
        Debug.LogFormat("XboxLiveLogic.__LoginStatusChanged()");

        int hresult = 0;

        switch (eventType)
        {
            case XUserChangeEvent.SignedInAgain:
                FreeUserHandle();
                XUserHandle userHandle;
                hresult = SDK.XUserFindUserByLocalId(userLocalId, out userHandle);
                if (HR.SUCCEEDED(hresult))
                {
                    MyUserHandle = userHandle;
                    GetProfileAndIssueLoginSuccessCallback();
                }
                else
                {
                    OnUserLoginError?.Invoke(LoginError.UnhandledOrUnknownHresultError, hresult);
                }
                break;

            case XUserChangeEvent.SignedOut:
                OnUserLoggedOut?.Invoke();
                FreeUserHandle();
                break;

            // note: for now we do not really acknowledge other change event types for
            // the purposes of this sample.  for privileges in particular, making sure
            // that privilege related changes are robustly handled is always a good
            // practice for the user experience.
            case XUserChangeEvent.SigningOut:
            case XUserChangeEvent.Privileges:
            default:
                break;
        }
    }

    private void GetProfileAndIssueLoginSuccessCallback()
    {
        Debug.LogFormat("XboxLiveLogic.__IssueLoginSuccessCallback()");

        // close any existing handle

        if (null != MyContextHandle)
        {
            SDK.XBL.XblContextCloseHandle(MyContextHandle);
            MyContextHandle = null;
        }

        var hresult = SDK.XBL.XblContextCreateHandle(MyUserHandle, out XblContextHandle xblContextHandle);
        if (HR.SUCCEEDED(hresult))
        {
            MyContextHandle = xblContextHandle;
        }
        else
        {
            OnGeneralError?.Invoke(@"Getting the user live context failed.", hresult);
        }

        hresult = SDK.XUserGetId(MyUserHandle, out ulong xuid);
        if (HR.SUCCEEDED(hresult))
        {
            MyXUID = xuid;
        }
        else
        {
            OnGeneralError?.Invoke(@"Getting the user ID failed.", hresult);
        }

        hresult = SDK.XUserGetGamertag(MyUserHandle, XUserGamertagComponent.Classic, out string gamerTag);
        if (HR.SUCCEEDED(hresult))
        {
            MyGamerTag = gamerTag;
        }
        else
        {
            OnGeneralError?.Invoke(@"Getting the user gamertag failed.", hresult);
        }

        OnUserLoggedIn?.Invoke();
    }

    private void HandleUserGetTokenCompleted(int hresult, XUserGetTokenAndSignatureUtf16Data tokenAndSignature)
    {
        Debug.LogFormat("XboxLiveLogic.__HandleUserGetTokenCompleted({0})", hresult.ToString("X8"));

        if (HR.FAILED(hresult))
        {
            OnGeneralError?.Invoke(@"RequestLiveUserToken failed.", hresult);
        }
        else
        {
            MyUserToken = tokenAndSignature.Token;
            OnUserTokenReceived?.Invoke();
        }
    }

    private void HandleSilentUserAddCompleted(int hresult, XUserHandle userHandle)
    {
        Debug.LogFormat("XboxLiveLogic.__HandleSilentUserAddCompleted({0})", hresult.ToString("X8"));

        if (HR.FAILED(hresult))
        {
            switch (hresult)
            {
                case E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED:
                case E_GAMEUSER_NO_DEFAULT_USER:
                    LoginLiveUser(false);
                    break;

                case E_GAMEUSER_UNKNOWN_GAME_IDENTITY:
                    OnUserLoginError?.Invoke(LoginError.TitleIsNotLiveEnabled, hresult);
                    break;

                case E_GAMEUSER_NO_PACKAGE_IDENTITY:
                    OnUserLoginError?.Invoke(LoginError.TitleIsNotDeployedProperly, hresult);
                    break;

                case E_CONTENT_ISOLATION:
                    OnUserLoginError?.Invoke(LoginError.TitleIsInWrongEnvironment, hresult);
                    break;

                default:
                    OnUserLoginError?.Invoke(LoginError.UnhandledOrUnknownHresultError, hresult);
                    break;
            }
        }
        else
        {
            MyUserHandle = userHandle;
            GetProfileAndIssueLoginSuccessCallback();
        }
    }

    private void HandleUserAddCompleted(int hresult, XUserHandle userHandle)
    {
        Debug.LogFormat("XboxLiveLogic.__HandleUserAddCompleted({0})", hresult.ToString("X8"));

        if (HR.FAILED(hresult))
        {
            switch (hresult)
            {
                case E_ABORT:
                    OnUserLoginError?.Invoke(LoginError.AuthWasAborted, hresult);
                    break;

                case E_GAMEUSER_UNKNOWN_GAME_IDENTITY:
                    OnUserLoginError?.Invoke(LoginError.TitleIsNotLiveEnabled, hresult);
                    break;

                case E_GAMEUSER_NO_PACKAGE_IDENTITY:
                    OnUserLoginError?.Invoke(LoginError.TitleIsNotDeployedProperly, hresult);
                    break;

                case E_CONTENT_ISOLATION:
                    OnUserLoginError?.Invoke(LoginError.TitleIsInWrongEnvironment, hresult);
                    break;

                default:
                    OnUserLoginError?.Invoke(LoginError.UnhandledOrUnknownHresultError, hresult);
                    break;
            }
        }
        else
        {
            MyUserHandle = userHandle;
            GetProfileAndIssueLoginSuccessCallback();
        }
    }
}
