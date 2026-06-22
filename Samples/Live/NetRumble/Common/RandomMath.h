//--------------------------------------------------------------------------------------
// RandomMath.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace RandomMath
{
    inline int32_t RandomBetween(int32_t minimum, int32_t maximum)
    {
        return rand() % (maximum - minimum + 1) + minimum;
    }

    inline uint32_t RandomBetween(uint32_t minimum, uint32_t maximum)
    {
        return rand() % (maximum - minimum + 1) + minimum;
    }

    inline float RandomBetween(float minimum, float maximum)
    {
        float f = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        return minimum + f * (maximum - minimum);
    }

    inline DirectX::SimpleMath::Vector2 RandomDirection()
    {
        float angle = RandomBetween(0.0f, DirectX::XM_2PI);
        return DirectX::SimpleMath::Vector2(std::cosf(angle), std::sinf(angle));
    }

    inline DirectX::SimpleMath::Vector2 RandomDirection(float minimumAngleInDegrees, float maximumAngleInDegrees)
    {
        float angle = DirectX::XMConvertToRadians(RandomBetween(minimumAngleInDegrees, maximumAngleInDegrees));
        return DirectX::SimpleMath::Vector2(std::cosf(angle), std::sinf(angle));
    }
}