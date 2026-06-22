//-----------------------------------------------------------------------------
// IConsumerJobManager.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Microsoft.SimpleXboxSecureTokens;
using Xbox.AgentProtocol;

namespace XboxAgentServerSample.JobManagement
{
    /// <summary>
    /// This interface is solely used by the heartbeat action in <see cref="XboxAgentServerSample.Controllers.HeartbeatController"/> to retrieve the next job and
    /// also post a job result and the kit's state.
    /// </summary>
    public interface IJobConsumptionManager
    {
        RequestedJob? GetPendingJob(string deviceId);

        Task<int> SetResult(string deviceId, LastProcessedJobResult lastProcessedJobResult);

        Task<RequestedJob?> GetNextJob(string deviceId);

        Task<int> SetLatestKitState(string deviceId, KitHeartbeatRequest kitHeartbeatRequest, XstsClientToken clientToken);
        Task<RequestedJob?> GetCancelCurrentJob(string deviceId);
    }
}
