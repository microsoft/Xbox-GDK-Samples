#pragma once

#if defined (PLAYFAB_PLATFORM_WINDOWS) || defined (PLAYFAB_PLATFORM_XBOX)
#include <string>
#include <playfab/PlayFabMultiplayerDataModels.h>

namespace PlayFab
{
    namespace QoS
    {
        /// <summary>
        /// The result of pinging a datacenter.
        /// </summary>
        struct RegionResult
        {
        public:
            RegionResult(const std::string& region, int latencyMs, int errorCode);
            RegionResult() = delete;

            // The datacenter region
            std::string region;

            // Average latency to reach the data center
            int latencyMs;

            // Last error code recieved while pinging
            int errorCode;
        };
    }
}
#endif // defined (PLAYFAB_PLATFORM_WINDOWS) || defined (PLAYFAB_PLATFORM_XBOX)