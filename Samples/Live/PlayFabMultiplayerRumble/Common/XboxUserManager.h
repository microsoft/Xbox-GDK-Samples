#pragma once

#include "XboxUser.h"
#include "IUserManager.h"

namespace PlayFabMultiplayerRumble
{
    class XboxUserManager : public IUserManager
    {
    public:
        XboxUserManager() = default;
        ~XboxUserManager() = default;

        void Initialize();
        void Shutdown();

        uint32_t AddUserSignedInCallback(UserCallback callback) override;
        uint32_t AddUserSignedOutCallback(UserCallback callback) override;

        void RemoveUserSignedInCallback(uint32_t token) override;
        void RemoveUserSignedOutCallback(uint32_t token) override;

        void SignIn() override;
        void ResolveUserIssueWithUI(ATG::XboxHandle<XUserHandle> user);
        void SwitchUser() override;

        XboxUser *GetCurrentUser() const override
        {
            return m_currentUser;
        }

        bool IsAnyoneSignedIn() const override
        {
            return m_currentUser != nullptr;
        }

        void ClearCurrentUser()
        {
            m_currentUser = nullptr;
        }

    private:
        void HandleError(const char* functionName, HRESULT error);

        void CallUserCallbacks(const User &user, const std::unordered_map<uint32_t, UserCallback> &callbacks);

        // Event Handlers
        void OnSignOutCompleted(ATG::XboxHandle<XUserHandle> userHandle);
        void OnSignInCompleted(ATG::XboxHandle<XUserHandle> userHandle);

        XTaskQueueRegistrationToken m_userChangedEventToken{};

        uint32_t m_nextChangedId = 0;
        std::unordered_map<uint32_t, UserCallback> m_userSignedInCallbacks;

        uint32_t m_nextSignoutId = 0;
        std::unordered_map<uint32_t, UserCallback> m_userSignedOutCallbacks;

        XboxUser *m_currentUser = nullptr;
    };
}
