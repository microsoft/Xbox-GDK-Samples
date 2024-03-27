//--------------------------------------------------------------------------------------
// SocialManagerIntegration.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"
#include <time.h>
#include "SocialManager.h"
#include "StringUtil.h"

void Sample::AddUserToSocialManager(_In_ XUserHandle user)
{
    uint64_t xuid = GetXuidForXUserHandle(user);
    if(xuid == 0)
        return;

    m_console->Format(L"Adding user %llu to SocialManager\n", xuid);

    // Add the local user, no extra details
    HRESULT hr = XblSocialManagerAddLocalUser(user, XblSocialManagerExtraDetailLevel::NoExtraDetail, m_asyncQueue);
    if (FAILED(hr))
    {
        m_console->Format(L" Failed adding user: 0x%08X\n", hr);
        return;
    }

    // create 4 different groups with various filters.
    // users in these groups will be filled in by the XblSocialManagerDoWork call below.
    CreateSocialGroupFromFilters(user, XblPresenceFilter::All, XblRelationshipFilter::Friends);
    CreateSocialGroupFromFilters(user, XblPresenceFilter::AllOnline, XblRelationshipFilter::Friends);
    CreateSocialGroupFromFilters(user, XblPresenceFilter::TitleOnline, XblRelationshipFilter::Friends);
    CreateSocialGroupFromFilters(user, XblPresenceFilter::All, XblRelationshipFilter::Favorite);
}

void Sample::RemoveUserFromSocialManager(_In_ XUserHandle user)
{
    uint64_t xuid = GetXuidForXUserHandle(user);
    if(xuid == 0)
        return;

    m_console->Format(L"Removing user %llu from SocialManager\n", xuid);

    auto it = m_userSocialGroups.begin();
    while (it != m_userSocialGroups.end())
    {
        XblSocialManagerDestroySocialUserGroup(*it);
        it = m_userSocialGroups.erase(it);
    }

    // Remove local user
    XblSocialManagerRemoveLocalUser(user);
}

void Sample::CreateSocialGroupFromList(
    _In_ XUserHandle user,
    _In_ std::vector<uint64_t>& xuidList)
{
    if(!xuidList.empty())
    {
        XblSocialManagerUserGroup* socialGroupCustom;

        HRESULT hr = XblSocialManagerCreateSocialUserGroupFromList(user, xuidList.data(), static_cast<uint32_t>(xuidList.size()), &socialGroupCustom);
        if (FAILED(hr))
        {
            m_console->Format(L" Error creating social group from list: 0x%08X\n", hr);
        }
        else
        {
            m_userSocialGroups.push_back(socialGroupCustom);
        }
    }
}

void Sample::CreateSocialGroupFromFilters(
    _In_ XUserHandle user,
    _In_ XblPresenceFilter presenceFilter,
    _In_ XblRelationshipFilter relationshipFilter
    )
{
    m_console->WriteLine(L"Creating Social Group.");

    XblSocialManagerUserGroup* group;
    HRESULT hr = XblSocialManagerCreateSocialUserGroupFromFilters(user, presenceFilter, relationshipFilter, &group);
    if(FAILED(hr))
    {
        m_console->Format(L" Error creating social group from filters: 0x%08X\n", hr);
    }
    else
    {
        m_userSocialGroups.push_back(group);
    }
}

void Sample::DestroySocialGroup(_In_ XblSocialManagerUserGroup* group)
{
    HRESULT hr = XblSocialManagerDestroySocialUserGroup(group);
    if (FAILED(hr))
    {
        m_console->Format(L" Error destroying social group: 0x%08X\n", hr);
    }
}

void Sample::LoadGamerPics(uint64_t* xuids, size_t count)
{
    auto async = new XAsyncBlock();
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock* asyncProf)
    {
        auto sample = reinterpret_cast<Sample*>(asyncProf->context);

        size_t profilesCount;
        HRESULT hr = XblProfileGetUserProfilesResultCount(asyncProf, &profilesCount);
        if (FAILED(hr))
        {
            sample->m_console->Format(L"Failed to get user profile count: 0x%08X\n", static_cast<unsigned int>(hr));
        }
        else
        {
            XblUserProfile* profiles = new XblUserProfile[profilesCount];
            hr = XblProfileGetUserProfilesResult(asyncProf, profilesCount, profiles);
            if (FAILED(hr))
            {
                sample->m_console->Format(L"Failed to get user profiles: 0x%08X\n", static_cast<unsigned int>(hr));
            }
            else
            {
                sample->m_profilesToLoadCount = profilesCount;
                for (size_t i = 0; i < profilesCount; i++)
                {
                    // Load gamerpic to cache
                    auto profile = profiles[i];

                    HCCallHandle httpHandle;
                    DX::ThrowIfFailed(HCHttpCallCreate(&httpHandle));
                    DX::ThrowIfFailed(HCHttpCallRequestSetUrl(httpHandle, "GET", profile.gameDisplayPictureResizeUri));

                    PictureData* picData = new PictureData(profile.xboxUserId, httpHandle, &sample->m_profilePicCache, &sample->m_profilesLoadedCount);

                    auto async = new XAsyncBlock{};
                    async->context = picData;
                    async->queue = nullptr;
                    async->callback = [](XAsyncBlock* asyncImg)
                    {
                        auto picData = reinterpret_cast<PictureData*>(asyncImg->context);
                        static size_t bufferSize = 1024 * 16;
                        uint8_t* pictureData = new uint8_t[bufferSize];

                        size_t responseSize;
                        HRESULT hr = HCHttpCallResponseGetResponseBodyBytesSize(picData->httpHandle, &responseSize);
                        DX::ThrowIfFailed(hr);

                        uint8_t* responseBuffer = new uint8_t[responseSize];
                        size_t responseBufferUsed;

                        hr = HCHttpCallResponseGetResponseBodyBytes(picData->httpHandle, responseSize, responseBuffer, &responseBufferUsed);
                        DX::ThrowIfFailed(hr);

                        if (SUCCEEDED(hr))
                        {
                            pictureData = responseBuffer;

                            // Write to cache
                            std::pair<uint8_t*, size_t> bufferPair;
                            bufferPair.first = pictureData;
                            bufferPair.second = responseBufferUsed;
                            (*picData->profileCache)[picData->xuid] = bufferPair;
                            (*picData->profilesLoadedCount)++;
                        }
                        else
                        {
                            pictureData = 0;
                            pictureData = nullptr;
                        }

                        DX::ThrowIfFailed(HCHttpCallCloseHandle(picData->httpHandle));

                        delete asyncImg;
                    };

                    DX::ThrowIfFailed(HCHttpCallPerformAsync(httpHandle, async));
                }
            }
        }
        delete asyncProf;
    };

    DX::ThrowIfFailed(XblProfileGetUserProfilesAsync(m_liveResources->GetLiveContext(), xuids, count, async));
}

void Sample::UpdateSocialManager()
{
    // Process events from the social manager
    // This should be called each frame update

#if PERF_COUNTERS
    auto perfInstance = performance_counters::get_singleton_instance();
    perfInstance->begin_capture(L"updates");
#endif

    const XblSocialManagerEvent* socialManagerEvents;
    size_t socialManagerEventCount;

    HRESULT hr = XblSocialManagerDoWork(&socialManagerEvents, &socialManagerEventCount);
    if (FAILED(hr))
    {
        m_console->Format(L"Failed to DoWork: 0x%08X\n", hr);
        return;
    }

    for (uint32_t i=0; i<socialManagerEventCount; i++)
    {
        std::wstring text;

        DisplayUsersAffected(socialManagerEvents->usersAffected);

        switch (socialManagerEvents[i].eventType)
        {
            case XblSocialManagerEventType::LocalUserAdded:
                text = L"LocalUserAdded";
                break;
            case XblSocialManagerEventType::PresenceChanged:
                text = L"PresenceChanged";
                RefreshUserList();
                break;
            case XblSocialManagerEventType::ProfilesChanged:
                text = L"ProfilesChanged";
                break;
            case XblSocialManagerEventType::SocialRelationshipsChanged:
                text = L"SocialRelationshipsChanged";
                break;
            case XblSocialManagerEventType::SocialUserGroupLoaded:
                text = L"SocialUserGroupLoaded";
                RefreshUserList();
                break;
            case XblSocialManagerEventType::SocialUserGroupUpdated:
                text = L"SocialUserGroupUpdated";
                RefreshUserList();
                break;
            case XblSocialManagerEventType::UnknownEvent:
                text = L"UnknownEvent";
                break;
            case XblSocialManagerEventType::UsersAddedToSocialGraph:
                text = L"UsersAddedToSocialGraph";
                RefreshUserList();
                break;
            case XblSocialManagerEventType::UsersRemovedFromSocialGraph:
                text = L"UsersRemovedFromSocialGraph";
                RefreshUserList();
                break;
        }

        m_console->Format(L"SocialManager event: %ws\n", text.c_str());
        if (FAILED(socialManagerEvents[i].hr))
        {
            m_console->Format(L" Error: 0x%08X\n", socialManagerEvents[i].hr);
        }
    }
#if PERF_COUNTERS
    perfInstance->end_capture(L"updates");
#endif
}

void Sample::DisplayUsersAffected(XblSocialManagerUser* const* users)
{
    for(uint32_t i = 0; i < XBL_SOCIAL_MANAGER_MAX_AFFECTED_USERS_PER_EVENT; i++)
    {
        XblSocialManagerUser* user = users[i];
        if(user == nullptr)
            continue;

        m_console->Format(L" Affected: %ws\n", DX::Utf8ToWide(user->displayName).c_str());
    }
}

uint64_t Sample::GetXuidForXUserHandle(XUserHandle user)
{
    uint64_t xuid = 0;
    HRESULT hr = XUserGetId(user, &xuid);
    if(FAILED(hr))
    {
        m_console->Format(L" Failed getting xuid for user: 0x%08X\n", hr);
    }
    return xuid;
}
