//--------------------------------------------------------------------------------------
// GameChatManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GameChatManager.h"
#include "InGameChat_Desktop.h"

// Suppress warnings from gamechat header until fixed
#pragma warning(push)
#pragma warning(disable : 5043)
// This file must be included in a single compilation unit per:
// https://docs.microsoft.com/en-us/gaming/xbox-live/features/multiplayer/chat/how-to/live-using-game-chat-2
#include "GameChat2Impl.h"
#pragma warning(pop)

using namespace xbox::services::game_chat_2;

GameChatManager::GameChatManager() noexcept :
    m_initialized(false)
{
}

void GameChatManager::Initialize()
{
    DebugTrace("Initialize GameChatManager");

    auto &chatManager = chat_manager::singleton_instance();

    // Hint where we'd like the STT window
    XSpeechToTextSetPositionHint(
        XSpeechToTextPositionHint::MiddleRight
        );

    // Put the audio processing thread on core 5
    chat_manager::set_thread_processor(
        game_chat_thread_id::audio,
        4
        );

    // Put the network processing thread on core 6
    chat_manager::set_thread_processor(
        game_chat_thread_id::networking,
        5
        );

    // Initialize chat manager
    chatManager.initialize(
        Sample::MAXUSERS,                                                                // Max users
        1.0f,                                                                            // Default volume
        xbox::services::game_chat_2::c_communicationRelationshipSendAndReceiveAll,       // Default to 'everyone can talk'
        game_chat_shared_device_communication_relationship_resolution_mode::restrictive, // Be restrictive for shared device (kinect)
        game_chat_speech_to_text_conversion_mode::automatic                              // Let GameChat perform perform accessibility actions
        );

    m_initialized = true;

    // Add the local users
    uint64_t xuid = 0;
    auto localUsers = Sample::Instance()->GetUserManager()->GetUsers();

    for (const auto& user : localUsers)
    {
        auto hr = XUserGetId(
            user->UserHandle,
            &xuid
            );

        if (SUCCEEDED(hr))
        {
            AddLocalUser(xuid);
        }
        else
        {
            DebugTrace("Failed to get XUID for user %lu", user->UserHandle);
        }
    }
}


void GameChatManager::Shutdown()
{
    DebugTrace("Shutdown GameChatManager");

    if (m_initialized)
    {
        std::lock_guard<std::mutex> lock(m_lock);

        chat_manager::singleton_instance().cleanup();
        m_initialized = false;
    }
}

void GameChatManager::AddRemoteUser(
    uint32_t peerId,
    uint64_t chatUserXuid
    )
{
    if (m_initialized)
    {
        std::lock_guard<std::mutex> lock(m_lock);

        DebugTrace("Adding remote user: %lu", chatUserXuid);

        chat_manager::singleton_instance().add_remote_user(
            std::to_wstring(chatUserXuid).c_str(),
            peerId
            );

        m_channels[chatUserXuid] = 1;

        RebuildRelationshipMap();
    }
}

void GameChatManager::ProcessStateChanges()
{
    if (!m_initialized)
    {
        return;
    }

    uint32_t count;
    game_chat_state_change_array changes;

    chat_manager::singleton_instance().start_processing_state_changes(
        &count,
        &changes
        );

    for (uint32_t i = 0; i < count; i++)
    {
        auto change = changes[i];

        switch (change->state_change_type)
        {
        case game_chat_state_change_type::communication_relationship_adjuster_changed:
            DebugTrace("GameChatManager::ProcessStateChanges() communication_relationship_adjuster_changed");
            break;

        case game_chat_state_change_type::text_chat_received:
            DebugTrace("GameChatManager::ProcessStateChanges() text_chat_received");
            HandleTextMessage(change, false);
            break;

        case game_chat_state_change_type::text_conversion_preference_changed:
            DebugTrace("GameChatManager::ProcessStateChanges() text_conversion_preference_changed");
            break;

        case game_chat_state_change_type::transcribed_chat_received:
            DebugTrace("GameChatManager::ProcessStateChanges() transcribed_chat_received");
            HandleTextMessage(change, true);
            break;
        }
    }

    chat_manager::singleton_instance().finish_processing_state_changes(changes);
}

void GameChatManager::ProcessDataFrames()
{
    if (m_initialized)
    {
        std::lock_guard<std::mutex> lock(m_lock);

        uint32_t dataFrameCount;
        game_chat_data_frame_array dataFrames;

        chat_manager::singleton_instance().start_processing_data_frames(
            &dataFrameCount,
            &dataFrames
            );

        for (uint32_t dataFrameIndex = 0; dataFrameIndex < dataFrameCount; ++dataFrameIndex)
        {
            auto dataFrame = dataFrames[dataFrameIndex];
            auto dataBuffer = std::vector<uint8_t>(dataFrame->packet_buffer, dataFrame->packet_buffer + dataFrame->packet_byte_count);

            for (uint32_t targetIndex = 0; targetIndex < dataFrame->target_endpoint_identifier_count; ++targetIndex)
            {
                auto targetIdentifier = dataFrame->target_endpoint_identifiers[targetIndex];

                Sample::Instance()->GetLiveManager()->SendNetworkMessage(
                    static_cast<uint32_t>(targetIdentifier),
                    dataBuffer
                    );
            }
        }

        chat_manager::singleton_instance().finish_processing_data_frames(dataFrames);
    }
}

void GameChatManager::HandleTextMessage(
    const game_chat_state_change *change,
    bool transcribed
    )
{
    uint64_t xuid = 0;
    const wchar_t* message;

    if (transcribed)
    {
        auto event = static_cast<const game_chat_transcribed_chat_received_state_change*>(change);
        xuid = std::stoull(event->speaker->xbox_user_id());
        message = event->message;
    }
    else
    {
        auto event = static_cast<const game_chat_text_chat_received_state_change*>(change);
        xuid = std::stoull(event->sender->xbox_user_id());
        message = event->message;
    }

    std::wstring name;
    auto user = GetChatUserByXboxUserId(xuid);

    if (user != nullptr)
    {
        name = Sample::Instance()->GetLiveManager()->GetGamertagForXuid(xuid);
    }
    else
    {
        name = std::to_wstring(xuid);
    }

    DebugTrace("Text Message from %ws: %ws", name.c_str(), message);

    XSpeechToTextSendString(
        DX::WideToUtf8(name).c_str(),
        DX::WideToUtf8(message).c_str(),
        transcribed ?
            XSpeechToTextType::Voice :
            XSpeechToTextType::Text
        );
}

void GameChatManager::AddLocalUser(
    uint64_t chatUserXuid
    )
{
    if (m_initialized)
    {
        std::lock_guard<std::mutex> lock(m_lock);

        DebugTrace("AddLocalUser: %lu", chatUserXuid);

        chat_manager::singleton_instance().add_local_user(std::to_wstring(chatUserXuid).c_str());

        // Set users on channel 1 by default
        m_channels[chatUserXuid] = 1;

        RebuildRelationshipMap();
    }
}

void
GameChatManager::RebuildRelationshipMap()
{
    auto users = Sample::Instance()->GetUserManager()->GetUsers();

    for (auto xboxUser : users)
    {
        uint64_t xuid = 0;
        auto hr = XUserGetId(
            xboxUser->UserHandle,
            &xuid
            );

        if (FAILED(hr))
        {
            continue;
        }

        auto localUser = GetChatUserByXboxUserId(xuid);

        uint32_t chatUserCount;
        chat_user_array chatUsers;

        chat_manager::singleton_instance().get_chat_users(
            &chatUserCount,
            &chatUsers
            );

        for (uint32_t chatUserIndex = 0; chatUserIndex < chatUserCount; ++chatUserIndex)
        {
            auto chatUser = chatUsers[chatUserIndex];

            if (chatUser != localUser)
            {
                auto currentChannel = m_channels[std::stoull(chatUser->xbox_user_id())];
                auto relationship = game_chat_communication_relationship_flags::none;

                // You can fully participate with users in the same channel
                if (m_channels[xuid] == currentChannel)
                {
                    relationship = c_communicationRelationshipSendAndReceiveAll;
                }

                localUser->local()->set_communication_relationship(
                    chatUser,
                    relationship
                    );
            }
        }
    }
}

void GameChatManager::RemoveLocalUser(
    uint64_t chatUserXuid
    )
{
    DebugTrace("RemoveLocalUser: %lu", chatUserXuid);

    if (m_initialized)
    {
        chat_manager::singleton_instance().remove_user(
            GetChatUserByXboxUserId(chatUserXuid)
            );
    }
}

void GameChatManager::RemoveRemoteUser(
    uint32_t peerId,
    uint64_t chatUserXuid
    )
{
    if (m_initialized)
    {
        std::lock_guard<std::mutex> lock(m_lock);

        DebugTrace("GameChatManager::RemoveRemoteUser(%lu, %lu)", peerId, chatUserXuid);

        uint32_t chatUserCount;
        chat_user_array chatUsers;

        chat_manager::singleton_instance().get_chat_users(
            &chatUserCount,
            &chatUsers
            );

        for (uint32_t chatUserIndex = 0; chatUserIndex < chatUserCount; ++chatUserIndex)
        {
            auto chatUser = chatUsers[chatUserIndex];

            if (chatUserXuid == std::stoull(chatUser->xbox_user_id()))
            {
                chat_manager::singleton_instance().remove_user(
                    chatUser
                    );

                DebugTrace("Removing remote user: %lu", chatUserXuid);
                break;
            }
        }
    }
}

chat_user*
GameChatManager::GetChatUserByXboxUserId(
    uint64_t chatUserXuid
    )
{
    if (m_initialized)
    {
        uint32_t count;
        chat_user_array chatUsers;

        chat_manager::singleton_instance().get_chat_users(
            &count,
            &chatUsers
            );

        for (uint32_t i = 0; i < count; ++i)
        {
            auto user = chatUsers[i];
            auto uxuid = std::stoull(user->xbox_user_id());

            if (chatUserXuid == uxuid)
            {
                return user;
            }
        }
    }

    return nullptr;
}

std::vector<uint64_t>
GameChatManager::GetChatUsersXuids()
{
    auto xuids = std::vector<uint64_t>();

    if (m_initialized)
    {
        uint32_t chatUserCount;
        chat_user_array chatUsers;

        chat_manager::singleton_instance().get_chat_users(
            &chatUserCount,
            &chatUsers
            );

        for (uint32_t chatUserIndex = 0; chatUserIndex < chatUserCount; ++chatUserIndex)
        {
            xuids.push_back(std::stoull(chatUsers[chatUserIndex]->xbox_user_id()));
        }
    }

    return xuids;
}

void
GameChatManager::ChangeChannelForUser(
    uint64_t chatUserXuid,
    uint8_t newChatChannelIndex
    )
{
    std::lock_guard<std::mutex> lock(m_lock);

    m_channels[chatUserXuid] = newChatChannelIndex;
    RebuildRelationshipMap();
}

void
GameChatManager::ToggleChatUserMuteState(
    uint64_t chatUserXuid
    )
{
    // Helper function to swap the mute state of a specific chat user
    auto chatUser = GetChatUserByXboxUserId(chatUserXuid);

    if (chatUser->local() != nullptr)
    {
        chatUser->local()->set_microphone_muted(
            !chatUser->local()->microphone_muted()
            );
    }
    else
    {
        auto users = Sample::Instance()->GetUserManager()->GetUsers();

        for (auto xboxUser : users)
        {
            uint64_t xuid = 0;
            auto hr = XUserGetId(
                xboxUser->UserHandle,
                &xuid
                );

            if (SUCCEEDED(hr))
            {
                auto localUser = GetChatUserByXboxUserId(xuid);

                localUser->local()->set_remote_user_muted(
                    chatUser,
                    !localUser->local()->remote_user_muted(chatUser)
                    );
            }
        }
    }
}

uint8_t
GameChatManager::GetChannelForUser(
    uint64_t chatUserXuid
    )
{
    std::lock_guard<std::mutex> lock(m_lock);
    return m_channels[chatUserXuid];
}

void
GameChatManager::ProcessChatPacket(
    uint32_t peerId,
    const std::vector<uint8_t> &data
    )
{
    if (m_initialized)
    {
        std::lock_guard<std::mutex> lock(m_lock);

        chat_manager::singleton_instance().process_incoming_data(
            peerId,                             // Remote console id
            static_cast<uint32_t>(data.size()), // Buffer size
            data.data()                         // Raw buffer pointer
            );
    }
}
