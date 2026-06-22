#pragma once

#include "Manager.h"

namespace NetRumble
{
    class User;

    using UserCallback = std::function<void(const User &)>;

    class IUserManager : public Manager
    {
    public:
        ~IUserManager() = default;

        virtual uint32_t AddUserSignedInCallback(UserCallback callback) = 0;
        virtual uint32_t AddUserSignedOutCallback(UserCallback callback) = 0;

        virtual void RemoveUserSignedInCallback(uint32_t token) = 0;
        virtual void RemoveUserSignedOutCallback(uint32_t token) = 0;

        virtual void SignIn() = 0;
        virtual void SwitchUser() = 0;

        virtual User *GetCurrentUser() const = 0;

        virtual bool IsAnyoneSignedIn() const = 0;
    };
}
