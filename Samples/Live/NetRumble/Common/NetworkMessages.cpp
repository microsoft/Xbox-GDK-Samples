//--------------------------------------------------------------------------------------
// NetworkMessages.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "NetworkMessages.h"

using namespace NetRumble;

GameMessage::GameMessage(GameMessageType type, uint32_t data) :
    m_type{type}
{
    m_data.resize(sizeof(unsigned));
    memcpy(m_data.data(), &data, sizeof(data));
}

GameMessage::GameMessage(GameMessageType type, std::string_view data) :
    m_type{type}
{
    m_data.resize(data.length() * sizeof(char));
    memcpy(m_data.data(), reinterpret_cast<const uint8_t*>(data.data()), m_data.size());
}

GameMessage::GameMessage(GameMessageType type, const std::vector<uint8_t> &data) :
    m_type{type},
    m_data{data}
{
}

GameMessage::GameMessage(const std::vector<uint8_t> &data)
{
    if (data.size() < (sizeof(GameMessageType) + sizeof(uint8_t)))
    {
        // Invalid message
        return;
    }

    memcpy(&m_type, data.data(), sizeof(GameMessageType));

    m_data.resize(data.size() - sizeof(GameMessageType));
    memcpy(m_data.data(), data.data() + sizeof(GameMessageType), m_data.size());
}

std::vector<uint8_t> GameMessage::Serialize() const
{
    if (m_type == GameMessageType::Unknown || m_data.empty())
    {
        return std::vector<uint8_t>();
    }

    std::vector<uint8_t> packet(sizeof(GameMessageType) + m_data.size());

    memcpy(packet.data(), &m_type, sizeof(GameMessageType));
    memcpy(packet.data() + sizeof(GameMessageType), m_data.data(), m_data.size());

    return packet;
}

std::string GameMessage::StringValue() const
{
    if (!m_data.empty())
    {
        auto stringData = reinterpret_cast<const char *>(m_data.data());
        return std::string(stringData, stringData + (m_data.size() / sizeof(char)));
    }

    return "";
}

uint32_t GameMessage::UnsignedValue() const
{
    if (!m_data.empty())
    {
        return *(reinterpret_cast<const uint32_t *>(m_data.data()));
    }

    return 0;
}

