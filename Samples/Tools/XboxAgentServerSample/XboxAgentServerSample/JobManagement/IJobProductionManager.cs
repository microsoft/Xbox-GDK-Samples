//-----------------------------------------------------------------------------
// IJobProductionManager.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Xbox.AgentProtocol;

namespace XboxAgentServerSample.JobManagement
{
    /// <summary>
    /// This interface is used by <see cref="CommandProcessor"/> to set the next job or clear a stale job.
    /// </summary>
    public interface IJobProductionManager
    {
        Task<int> SetCancelCurrentJob(string deviceId, RequestedJob requestedJob);

        Task<int> SetNextJob(string deviceId, RequestedJob requestedJob, TaskCompletionSource<LastProcessedJobResult> tcs);
    }
}
