#pragma once

#if defined (PLAYFAB_PLATFORM_WINDOWS) || defined (PLAYFAB_PLATFORM_XBOX)
#include <vector>

#include <playfab/QoS/RegionResult.h>

namespace PlayFab
{
    namespace QoS
    {
        /// <summary>
        /// A class that holds a result of the whole QoS operation.
        /// </summary>
        class QoSResult
        {
        public:
            QoSResult() = default;
            QoSResult(QoSResult&&) = default;
            QoSResult(const QoSResult&) = delete;
            QoSResult& operator=(QoSResult&&) = default;
            QoSResult& operator=(const QoSResult&) = delete;

            // A list of datacenter results. Each element contains a ping result for a datacenter.
            std::vector<RegionResult> regionResults;

            // An error code of the whole QoS operation.
            int errorCode;
        };
    }
}
#endif // defined (PLAYFAB_PLATFORM_WINDOWS) || defined (PLAYFAB_PLATFORM_XBOX)