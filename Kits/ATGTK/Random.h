//--------------------------------------------------------------------------------------
// Random.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <random>

namespace ATG
{
    template<typename randomSize>
    randomSize GetRandomValue(randomSize range)
    {
        if (range == 0)
            return range;
        static std::random_device randomDevice;
        static std::mt19937_64 randomEngine(randomDevice());

        using distribution = typename std::conditional< std::is_floating_point_v<randomSize>, std::uniform_real_distribution<randomSize>, std::uniform_int_distribution<randomSize>>::type;
        static distribution randomValue(0, std::numeric_limits<randomSize>::max());

        randomSize toret = randomValue(randomEngine);
        if (std::is_floating_point_v<randomSize>)
        {
            toret /= std::numeric_limits<randomSize>::max();
            toret *= range;
        }
        else
        {
            randomSize temp = toret / range;
            toret -= (temp * range);
        }
        return toret;
    }
}
