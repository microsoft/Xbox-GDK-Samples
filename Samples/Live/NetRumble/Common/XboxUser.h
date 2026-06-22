#pragma once

#include "XboxHandle.h"
#include "User.h"

namespace NetRumble
{
    class XboxUser final : public User
    {
    public:
        XboxUser() = default;
        ~XboxUser() = default;

        XboxUser(const XboxUser &source) = default;
        XboxUser(XboxUser &&moveFrom) = default;

        XboxUser &operator=(const XboxUser &source) = default;
        XboxUser &operator=(XboxUser &&moveFrom) = default;

        // Takes ownership of this handle and will close the handle when done
        explicit XboxUser(ATG::XboxHandle<XUserHandle> user);

        operator XUserHandle() const { return m_user.get(); }
        operator ATG::XboxHandle<XUserHandle>() const { return m_user; }

        virtual std::string Name() const override;
        virtual uint64_t    Id() const override;
    private:
        ATG::XboxHandle<XUserHandle> m_user;
    };
}
