//--------------------------------------------------------------------------------------
// GameChatManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

class ChatUser;

class GameChatManager
{
public:
    GameChatManager() noexcept;

    GameChatManager(GameChatManager&&) = delete;
    GameChatManager& operator=(GameChatManager&&) = delete;

    GameChatManager(GameChatManager const&) = delete;
    GameChatManager& operator=(GameChatManager const&) = delete;

    void Initialize();
    void Shutdown();

    // Call to add a local user to gamechat
    void AddLocalUser(
        uint64_t chatUserXuid
        );

    // Call to remove a local user from gamechat
    void RemoveLocalUser(
        uint64_t chatUserXuid
        );

    // Handles a new user connection
    void AddRemoteUser(
        uint64_t chatUserXuid,
        uint64_t deviceId
        );

    // Remove a remote user from gamechat
    void RemoveRemoteUser(
        uint64_t chatUserXuid
        );

    // Handles incoming chat messages from the game's network layer
    void ProcessChatPacket(
        uint64_t chatUserXuid,
        const std::vector<uint8_t> &data
        );

    // Return a vector of xuids for all gamechat users
    std::vector<uint64_t> GetChatUsersXuids();
    
    // Get the channel a user is in
    uint8_t GetChannelForUser(
        uint64_t chatUserXuid
        );

    // Toggle the mute state of a gamechat user
    void ToggleChatUserMuteState(
        uint64_t chatUserXuid
        );

    // Set the channel for a chat user
    void ChangeChannelForUser(
        uint64_t chatUserXuid,
        uint8_t newChatChannelIndex
        );

    // Call in update loop to pump chat events
    void ProcessStateChanges();

    // Call to process chat packets
    void ProcessDataFrames();

    // Return the raw gamechat user by xuid
    xbox::services::game_chat_2::chat_user* GetChatUserByXboxUserId(
        uint64_t xuid
        );

    // The number of voice channels available
    static constexpr int c_numberOfChannels = 3;

private:
    void HandleTextMessage(
        const xbox::services::game_chat_2::game_chat_state_change *change,
        bool transcribed
        );

    void HandleRelationshipAdjusterChange(
        const xbox::services::game_chat_2::game_chat_state_change *change
        );

    void RebuildRelationshipMap();

    bool m_initialized;
    std::mutex m_lock;
    std::map<uint64_t, uint8_t> m_channels;
};
