#pragma once

namespace NetRumble
{
    class User
    {
    public:
        virtual std::string Name() const = 0;
        virtual uint64_t    Id() const = 0;
    };
}