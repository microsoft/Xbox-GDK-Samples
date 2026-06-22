//--------------------------------------------------------------------------------------
// LobbyPlayer.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// TODO: move somewhere else
class CLobbyPlayer
{
public:
    CLobbyPlayer() = default;

	CLobbyPlayer(std::string_view strDisplayName, byte shipColor, byte shipVariant, bool bIsReady) :
        m_strDisplayName{ strDisplayName },
        m_shipColor{shipColor},
        m_shipVariant{shipVariant},
        m_bIsReady{bIsReady}
	{
	}

	const std::string& GetDisplayName() const
	{
		return m_strDisplayName;
	}

	const byte ShipColor() const
	{
		return m_shipColor;
	}

	const byte ShipVariant() const
	{
		return m_shipVariant;
	}

	const bool IsReady() const
	{
		return m_bIsReady;
	}

private:
	std::string m_strDisplayName;
	byte m_shipColor;
	byte m_shipVariant;
	bool m_bIsReady;
};