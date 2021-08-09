#pragma once

#if defined (PLAYFAB_PLATFORM_WINDOWS) || defined (PLAYFAB_PLATFORM_XBOX)
#include <cstdint>

namespace PlayFab
{
    namespace QoS
    {
        /// <summary>
        /// A ping result for a datacenter.
        /// </summary>
        struct PingResult
        {
            int latencyMs; // INT32_MAX value means that the latency cannot be determined in the given timeout
            int errorCode;
            int pingCount;

            PingResult() : PingResult(INT32_MAX, 0, 0)
            {
            }

            PingResult(int latencyMs, int errorCode, int pingCount) :
                latencyMs(latencyMs), errorCode(errorCode), pingCount(pingCount)
            {
            }
        };
    }
}
#endif // defined (PLAYFAB_PLATFORM_WINDOWS) || defined (PLAYFAB_PLATFORM_XBOX)