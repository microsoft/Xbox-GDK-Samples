// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
#pragma once
#include <vector>
#include <functional>
#include <algorithm>

namespace ATG
{
    enum TimerState
    {
        Start,
        Tick,
        End
    };

    class EffectTimers
    {
    public:
        void Update(float timeElapsed)
        {
            for (auto iter = m_effects.begin(); iter < m_effects.end(); ++iter)
            {
                iter->m_timeRemaining -= timeElapsed;

                if (iter->m_timeRemaining > 0)
                {
                    iter->m_callback(Tick, iter->m_timeRemaining);
                }
                else
                {
                    iter->m_callback(End, 0);  
                }
            }

            auto end = std::remove_if(m_effects.begin(), m_effects.end(),[](TimedEffect &effect)
            {
               return effect.m_timeRemaining <= 0; 
            });

            if (end != m_effects.end())
            {
                m_effects.erase(end, m_effects.end());
            }
        }

        void Add(float duration, std::function<void(TimerState, float)> callback)
        {
            callback(Start, duration);

            m_effects.emplace_back(duration, callback);
        }

        void Reset()
        {
            m_effects.clear();
        }
    private:
        struct TimedEffect
        {
            TimedEffect(float time, std::function<void(TimerState, float)> callback) : m_timeRemaining(time), m_callback(callback) {}

            float m_timeRemaining;
            std::function<void(TimerState, float)> m_callback;
        };

        std::vector<TimedEffect> m_effects;
    };
}