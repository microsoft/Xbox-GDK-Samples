//--------------------------------------------------------------------------------------
// ExploreGameSimulation.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

enum class Environment : uint32_t
{
    Maze = 0,
    Cave = 1,
    Void = 2,
    Museum = 3
};

struct GameResult
{
    uint32_t distanceTraveled;
    uint32_t timesLost;
    uint32_t itemsFound;
    const char *environment;

    std::string ToEventJson();
};

// This simulates an exploration game where players can get lost and find items
class ExploreGameSimulation
{
public:
    static GameResult PlayGame();
    ExploreGameSimulation();
    ~ExploreGameSimulation();
};
