#pragma once

namespace PlayFabMultiplayerRumble
{
    class User
    {
    public:
        virtual std::string Name() const = 0;
        virtual uint64_t    Id() const = 0;
    };
}