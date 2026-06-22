#pragma once

#include "XboxUser.h"
#include "UserManager.h"

namespace NetRumble
{
    class XboxUserManager : public IUserManager
    {
    public:
        XboxUserManager();
        ~XboxUserManager();

        void Initialize();
        void Shutdown();

        virtual uint32_t AddUserSignedInCallback(UserCallback callback) override;
        virtual uint32_t AddUserSignedOutCallback(UserCallback callback) override;

        virtual void RemoveUserSignedInCallback(uint32_t token) override;
        virtual void RemoveUserSignedOutCallback(uint32_t token) override;

        virtual void SignIn() override;
        void ResolveUserIssueWithUI(ATG::XboxHandle<XUserHandle> user);
        virtual void SwitchUser() override;

        virtual XboxUser *GetCurrentUser() const override
        {
            return m_currentUser;
        }

        virtual bool IsAnyoneSignedIn() const override
        {
            return m_currentUser != nullptr;
        }

        void ClearCurrentUser()
        {
            m_currentUser = nullptr;
        }

    private:
        void HandleError(HRESULT error);

        void CallUserCallbacks(const User &user, const std::unordered_map<uint32_t, UserCallback> &callbacks);

        // Event Handlers
        void OnSignOutCompleted(ATG::XboxHandle<XUserHandle> userHandle);
        void OnSignInCompleted(ATG::XboxHandle<XUserHandle> userHandle);

        XTaskQueueRegistrationToken m_userChangedEventToken;
        
        uint32_t m_nextChangedId = 0;
        std::unordered_map<uint32_t, UserCallback> m_userSignedInCallbacks;

        uint32_t m_nextSignoutId = 0;
        std::unordered_map<uint32_t, UserCallback> m_userSignedOutCallbacks;

        ATG::XboxHandle<XTaskQueueHandle> m_asyncQueue;

        XboxUser *m_currentUser;
    };
}
