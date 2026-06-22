#include "pch.h"
#include "XboxUser.h"

using namespace PlayFabMultiplayerRumble;

XboxUser::XboxUser(ATG::XboxHandle<XUserHandle> user) : m_user{user}
{
}

std::string XboxUser::Name() const
{
    char buffer[XUserGamertagComponentClassicMaxBytes] = {};
    XUserGetGamertag(m_user.get(), XUserGamertagComponent::Classic, XUserGamertagComponentClassicMaxBytes, buffer, nullptr);

    return buffer;
}

uint64_t XboxUser::Id() const
{
    uint64_t id = {};
    XUserGetId(m_user.get(), &id);

    return id;
}
