#include "pch.h"
#include "XboxUserManager.h"
#include "GameScreen.h"

using namespace PlayFabMultiplayerRumble;

void XboxUserManager::Initialize()
{
    auto asyncManager = Managers::Get<AsyncTaskManager>();
    auto asyncQueue = asyncManager->GetDefaultQueue();
    XUserRegisterForChangeEvent(
        asyncQueue.get(),
        this,
        [](void *context, const XUserLocalId userLocalId, XUserChangeEvent event)
        {
            auto pThis = static_cast<XboxUserManager*>(context);

            DEBUGLOG("XUser Change Event: %d", event);

            ATG::XboxHandle<XUserHandle> user{};
            XUserFindUserByLocalId(userLocalId, user.get_address_of());

            switch (event)
            {
                // if we were put back to the sign-on screen from a sign out and we logged back in
                // the user from the Xbox shell UI, then we should move them along to the main menu
            case XUserChangeEvent::SignedInAgain:
                OutputDebugStringW(L"XUserChangeEvent::SignedInAgain");
                pThis->OnSignInCompleted(user);
                break;

                // TODO: this would be the point when we just prepare for the eventual sign-out, but
                // the sign-out has not actually completed yet.
            case XUserChangeEvent::SigningOut:
                OutputDebugStringW(L"XUserChangeEvent::SigningOut");
                break;

                // if the user has finished signing-out, then we should actually return the user
                // to the main menu
            case XUserChangeEvent::SignedOut:
                OutputDebugStringW(L"XUserChangeEvent::SignedOut");
                pThis->OnSignOutCompleted(user);
                break;

                // TODO: if the user has lost privileges for MP, then we should return the player to the
                // main menu after showing a notification
                // ALSO: it might be good to either enable/disable main menu options depending on what
                // MP privileges the user has
            case XUserChangeEvent::Privileges:
                OutputDebugStringW(L"XUserChangeEvent::Privileges");
                break;

                // we do not need to worry about a change the gamer tag since the XboxUser container
                // for the user handle always gets the user's gamer tag on demand from the API
            case XUserChangeEvent::Gamertag:
                break;

                // we purposely ignore a change to the gamer picture since the gamer picture is not
                // being used by this sample
            case XUserChangeEvent::GamerPicture:
                break;

            default:
                break;
            }
        },
        &m_userChangedEventToken);
}

void XboxUserManager::Shutdown()
{
    XUserUnregisterForChangeEvent(m_userChangedEventToken, true);
    m_userChangedEventToken = {};
}

uint32_t XboxUserManager::AddUserSignedInCallback(UserCallback callback)
{
    auto id = m_nextChangedId++;
    m_userSignedInCallbacks.emplace(id, callback);
    return id;
}

uint32_t XboxUserManager::AddUserSignedOutCallback(UserCallback callback)
{
    auto id = m_nextSignoutId++;
    m_userSignedOutCallbacks.emplace(id, callback);
    return id;
}

void XboxUserManager::RemoveUserSignedInCallback(uint32_t token)
{
    m_userSignedInCallbacks.erase(token);
}

void XboxUserManager::RemoveUserSignedOutCallback(uint32_t token)
{
    m_userSignedOutCallbacks.erase(token);
}

void XboxUserManager::SignIn()
{
    auto asyncManager = Managers::Get<AsyncTaskManager>();
    auto asyncQueue = asyncManager->GetDefaultQueue();
    auto asyncHelper = new ATG::AsyncHelper(asyncQueue.get(), [this](XAsyncBlock* async)
    {
        ATG::XboxHandle<XUserHandle> user{};

        HRESULT result = XUserAddResult(async, user.get_address_of());

        if (SUCCEEDED(result))
        {
            // This failure doesn't come up until you try to actually do something with the user
            uint64_t xuid = {};
            if (FAILED(XUserGetId(user.get(), &xuid)))
            {
                ResolveUserIssueWithUI(user);
            }
            else
            {
                OnSignInCompleted(ATG::XboxHandle<XUserHandle>{user});
            }
        }
        else if (result == E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED
            || result == E_GAMEUSER_NO_DEFAULT_USER
            || result == static_cast<HRESULT>(0x8015DC12))
        {
            SwitchUser();
        }
        else
        {
            HandleError("XUserAddResult", result);
        }
    });

    HRESULT hr = XUserAddAsync(XUserAddOptions::AddDefaultUserSilently, &asyncHelper->asyncBlock);
    if (FAILED(hr))
    {
        HandleError("XUserAddAsync", hr);
        delete asyncHelper;
    }
}

void XboxUserManager::ResolveUserIssueWithUI(ATG::XboxHandle<XUserHandle> user)
{
    auto asyncManager = Managers::Get<AsyncTaskManager>();
    auto asyncQueue = asyncManager->GetDefaultQueue();
    auto asyncHelper = new ATG::AsyncHelper(asyncQueue.get(), [this, user](XAsyncBlock* async)
    {
        HRESULT hr = XUserResolveIssueWithUiResult(async);
        if (SUCCEEDED(hr))
        {
            OnSignInCompleted(ATG::XboxHandle<XUserHandle>{user});
        }
        else
        {
            HandleError("XUserResolveIssueWithUiResult", hr);
        }
    });

    HRESULT hr = XUserResolveIssueWithUiAsync(user.get(), nullptr, &asyncHelper->asyncBlock);
    if (FAILED(hr))
    {
        HandleError("XUserResolveIssueWithUiAsync", hr);
        delete asyncHelper;
    }
}

void XboxUserManager::SwitchUser()
{
    auto asyncManager = Managers::Get<AsyncTaskManager>();
    auto asyncQueue = asyncManager->GetDefaultQueue();
    auto asyncHelper = new ATG::AsyncHelper(asyncQueue.get(), [this](XAsyncBlock* async)
    {
        ATG::XboxHandle<XUserHandle> user{};
        HRESULT hr = XUserAddResult(async, user.get_address_of());
        if (SUCCEEDED(hr))
        {
            OnSignInCompleted(ATG::XboxHandle<XUserHandle>{user});
        }
        else
        {
            HandleError("XUserAddResult", hr);
        }
    });

    HRESULT hr = XUserAddAsync(XUserAddOptions::None, &asyncHelper->asyncBlock);
    if (FAILED(hr))
    {
        HandleError("XUserAddAsync", hr);
        delete asyncHelper;
    }
}

void XboxUserManager::HandleError(const char* functionName, HRESULT hr)
{
    LogError_HRESULT(functionName, hr);

    Managers::Get<ScreenManager>()->ShowError(
        "Unable to sign in",
        []()
        {
            Managers::Get<ScreenManager>()->GetCurrentGameScreen()->Reset();
        });
}

void XboxUserManager::CallUserCallbacks(const User &user, const std::unordered_map<uint32_t, UserCallback> &callbacks)
{
    for (auto &&[id, callback] : callbacks)
    {
        if (callback)
        {
            callback(user);
        }
    }
}

void XboxUserManager::OnSignOutCompleted(ATG::XboxHandle<XUserHandle> user)
{
    // same user returns 0 for XUserCompare
    if ((user && m_currentUser) && 0 == XUserCompare(user.get(), *m_currentUser))
    {
        CallUserCallbacks(*m_currentUser, m_userSignedOutCallbacks);

        m_currentUser = nullptr;
    }
}

void XboxUserManager::OnSignInCompleted(ATG::XboxHandle<XUserHandle> user)
{
    // same user returns 0 for XUserCompare
    if ((user && !m_currentUser) || 0 != XUserCompare(user.get(), *m_currentUser))
    {
        m_currentUser = new XboxUser{ user };

        CallUserCallbacks(*m_currentUser, m_userSignedInCallbacks);
    }
}
