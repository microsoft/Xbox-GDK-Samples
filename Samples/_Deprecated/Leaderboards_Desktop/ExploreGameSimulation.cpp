//--------------------------------------------------------------------------------------
// ExploreGameSimulation.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ExploreGameSimulation.h"
#include "Json.h"

using hrclock = std::chrono::high_resolution_clock;

static const char *s_environments[] = { u8"Maze", u8"Cave", u8"Void", u8"Museum" };

class GameImpl
{
public:
    GameImpl() : m_rand((uint32_t)hrclock::now().time_since_epoch().count()) { }

    uint32_t CalculateDistance()
    {
        static std::array<double, 4> intervals{ 600.0, 1250.0, 2200.0, 2600.0 };
        static std::array<double, 4> weights{ 0.002, 1.5, 2.5, 0.3 };
        static std::piecewise_linear_distribution<double> distanceDist(
            intervals.begin(), intervals.end(), weights.begin());

        return (uint32_t)distanceDist(m_rand);
    }

    uint32_t CalculateTimesLost(uint32_t distance)
    {
        static std::exponential_distribution<double> lostDist(2.2);
        double lostRate;
        do
        {
            lostRate = lostDist(m_rand);
            lostRate *= (distance / 100);
        } while (lostRate < 1.0 && distance < 700);
        return (uint32_t)lostRate;
    }

    uint32_t CalculateItemsFound(uint32_t distance)
    {
        static std::exponential_distribution<double> itemsDist(1);

        double itemRate = itemsDist(m_rand);
        itemRate *= (distance / 100);
        return (uint32_t)itemRate;
    }

    const char *GetRadomnEnvironment()
    {
        static std::uniform_int_distribution<uint32_t> envDist(0, 3);

        return s_environments[envDist(m_rand)];
    }

private:
    std::default_random_engine m_rand;

};

GameResult ExploreGameSimulation::PlayGame()
{
    static GameImpl Game;

    GameResult result;

    result.distanceTraveled = Game.CalculateDistance();
    result.timesLost = Game.CalculateTimesLost(result.distanceTraveled);
    result.itemsFound = Game.CalculateItemsFound(result.distanceTraveled);
    result.environment = Game.GetRadomnEnvironment();

    return result;
}

ExploreGameSimulation::ExploreGameSimulation()
{
}

ExploreGameSimulation::~ExploreGameSimulation()
{
}

std::string GameResult::ToEventJson()
{
    json j;
    j["Environment"] = environment;
    j["DistanceTraveled"] = distanceTraveled;
    j["GotLostCount"] = timesLost;
    j["ItemsFoundCount"] = itemsFound;

    return j.dump();
}
