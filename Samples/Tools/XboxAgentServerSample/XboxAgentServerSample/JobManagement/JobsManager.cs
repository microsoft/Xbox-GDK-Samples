//-----------------------------------------------------------------------------
// JobsManager.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

// Ignore Spelling: tcs

using Microsoft.SimpleXboxSecureTokens;
using Xbox.AgentProtocol;
using XboxAgentServerSample.Data;
using XboxAgentServerSample.Models;

namespace XboxAgentServerSample.JobManagement
{
    /// <summary>
    /// This class handles job queuing, result posting, and overall state tracking for devices.
    /// It implements 3 different interfaces for each type of stakeholder needed. This class could
    /// easily implement a database for the state back-end instead of the existing Dictionary.
    /// </summary>
    public class JobsManager : IJobConsumptionManager, IJobProductionManager, IDeviceStateManager
    {
        //private readonly Dictionary<string, XboxAgentDevkitState> _deviceStates = new();

        private readonly AutoResetEvent _nextJobAutoResetEvent = new(false);

        #region IJobProductionManager

        /// <summary>
        /// Queue up the next job to be processed by the device.
        /// </summary>
        /// <param name="deviceId">The id for the device to process the job.</param>
        /// <param name="requestedJob">The job to be processed by the device.</param>
        /// <param name="tcs">The <see cref="TaskCompletionSource{TResult}"/> used to asynchronously relay back the job result.</param>
        public async Task<int> SetNextJob(string deviceId, RequestedJob requestedJob, TaskCompletionSource<LastProcessedJobResult> tcs)
        {
            using var dbContext = SampleDBController.CreateDbContext();
            var deviceDbEntry = dbContext.LinkedDevkits.Find(deviceId);
            if (deviceDbEntry != null)
            {
                var deviceState = deviceDbEntry.DeserializeDbEntry();
                deviceState.NextJob = requestedJob;

                _nextJobAutoResetEvent.Set();
                deviceDbEntry.UpdateDbEntry(deviceState);
            }
            else
            {
                //  This Devkit is not in our registered Devkit database yet
                //  so we can't get all the info, but we will create an entry
                //  for it so that if that specific DeveloperDeviceId does 
                //  give us a heartbeat we will be able to send it the next
                //  job passed into this API
                var devkitInfo = new XboxAgentDevkit()
                {
                    DeveloperDeviceId = deviceId
                };

                var newDevkitState = new XboxAgentDevkitState()
                {
                    DevkitInfo = devkitInfo,
                    NextJob = requestedJob,
                };

                var newDevkitDbEntry = new XboxAgentDBEntry(newDevkitState);

                dbContext.LinkedDevkits.Add(newDevkitDbEntry);
            }
            await dbContext.SaveChangesAsync();

            return 1;
        }

        public async Task<int> SetCancelCurrentJob(string deviceId, RequestedJob requestedJob)
        {
            using var dbContext = SampleDBController.CreateDbContext();
            var deviceDbEntry = dbContext.LinkedDevkits.Find(deviceId);
            if (deviceDbEntry != null)
            {
                var deviceState = deviceDbEntry.DeserializeDbEntry();
                deviceState.CancelCurrentJob = requestedJob;

                deviceDbEntry.UpdateDbEntry(deviceState);
                await dbContext.SaveChangesAsync();
            }
            return 1;
        }

        #endregion IJobProductionManager

        #region IJobConsumptionManager

        /// <summary>
        /// Get the job the device is currently meant to be processing.
        /// </summary>
        /// <param name="deviceId">The id for the device to process the job.</param>
        /// <returns>The job that the device should be processing.</returns>
        public RequestedJob? GetPendingJob(string deviceId)
        {
            RequestedJob? requestedJob = null;

            using (var dbContext = SampleDBController.CreateDbContext())
            {
                var deviceDbEntry = dbContext.LinkedDevkits.Find(deviceId);
                if (deviceDbEntry != null)
                {
                    var deviceState = deviceDbEntry.DeserializeDbEntry();
                    requestedJob = deviceState.PendingJob;
                }
            }

            return requestedJob;
        }

        public async Task<RequestedJob?> GetCancelCurrentJob(string deviceId)
        {
            RequestedJob? requestedJob = null;

            using (var dbContext = SampleDBController.CreateDbContext())
            {
                var deviceDbEntry = await dbContext.LinkedDevkits.FindAsync(deviceId);
                if (deviceDbEntry != null)
                {
                    var deviceState = deviceDbEntry.DeserializeDbEntry();
                    requestedJob = deviceState.CancelCurrentJob;
                }
            }

            return requestedJob;
        }

        /// <summary>
        /// Used to set the job's result and signal back to the requesting code the job has completed.
        /// </summary>
        /// <param name="deviceId">The id of the device that processed the job.</param>
        /// <param name="lastProcessedJobResult">The job result.</param>
        public async Task<int> SetResult(string deviceId, LastProcessedJobResult lastProcessedJobResult)
        {
            using var dbContext = SampleDBController.CreateDbContext();
            var deviceDbEntry = dbContext.LinkedDevkits.Find(deviceId);
            if (deviceDbEntry != null)
            {
                var deviceState = deviceDbEntry.DeserializeDbEntry();

                // Clear the CancelCurrentJob job to indicate the original job has returned.
                if (deviceState.CancelCurrentJob != null)
                {
                    deviceState.CancelCurrentJob = null;
                }

                //deviceState.NextJobResultTcs.SetResult(lastProcessedJobResult);
                deviceState.PendingJob = null;
                //deviceState.NextJobResultTcs = null;

                deviceState.LastJobResult = lastProcessedJobResult;

                deviceDbEntry.UpdateDbEntry(deviceState);

                await dbContext.SaveChangesAsync();
            }

            return 1;
        }

        /// <summary>
        /// Moves the queued job to the pending job position and returns it as the new
        /// job to be processed by the device.
        /// </summary>
        /// <param name="deviceId">The id of the device to process the job.</param>
        /// <returns>The new job to be processed.  If no job is queued, returns null</returns>
        public async Task<RequestedJob?> GetNextJob(string deviceId)
        {
            RequestedJob? nextJob = null;

            using (var dbContext = SampleDBController.CreateDbContext())
            {
                var deviceDbEntry = dbContext.LinkedDevkits.Find(deviceId);

                // Wait a maximum time for the next job to be requested to avoid
                // waiting to send on the next heartbeat.
                if (deviceDbEntry != null)
                {
                    var deviceState = deviceDbEntry.DeserializeDbEntry();
                    if(deviceState.NextJob != null)
                    {
                        if(deviceState.PendingJob == null)
                        {
                            //  There is no pending job so we can move the NextJob to the pending job
                            nextJob = deviceState.PendingJob = deviceState.NextJob;
                            deviceState.NextJob = null;
                            deviceDbEntry.UpdateDbEntry(deviceState);
                            await dbContext.SaveChangesAsync();
                        }
                        else
                        {
                            //  there is still a pending job so we need to wait for that to cancel first
                        }
                    }
                }
            }

            return nextJob;
        }

        /// <summary>
        /// Stores the last returned status & state from the device.
        /// </summary>
        /// <param name="deviceId">The id of the device the state is for.</param>
        /// <param name="kitHeartbeatRequest">The last heartbeat received from the device.</param>
        /// <param name="clientToken">The last XToken received from the device. Can be <c>null</c> if XToken wasn't used.</param>
        public async Task<int> SetLatestKitState(string deviceId, KitHeartbeatRequest kitHeartbeatRequest, XstsClientToken clientToken)
        {
            using var dbContext = SampleDBController.CreateDbContext();
            var deviceDbEntry = await dbContext.LinkedDevkits.FindAsync(deviceId);
            if (deviceDbEntry != null)
            {
                var deviceState = deviceDbEntry.DeserializeDbEntry();
                deviceState.LatestAgentState = kitHeartbeatRequest;
                deviceDbEntry.UpdateDbEntry(deviceState);
            }
            else
            {
                var devkitInfo = new XboxAgentDevkit()
                {
                    DeveloperDeviceId = deviceId,
                    ConsoleType = kitHeartbeatRequest.Kit.DeviceType,
                    HostName = kitHeartbeatRequest.Kit.Hostname,
                    IpAddress = kitHeartbeatRequest.Kit.IpAddress,
                    OsVersion = kitHeartbeatRequest.Kit.OsVersion,
                    RunningApplication = kitHeartbeatRequest.Kit.RunningApplication,
                    RunningApplicationId = kitHeartbeatRequest.Kit.RunningApplicationId,
                    Uptime = kitHeartbeatRequest.Kit.Uptime
                };

                var newDevkitState = new XboxAgentDevkitState()
                {
                    DeveloperDeviceId = devkitInfo.DeveloperDeviceId,
                    DevkitInfo = devkitInfo,
                    LatestAgentState = kitHeartbeatRequest,
                };

                deviceDbEntry = new XboxAgentDBEntry(newDevkitState);

                await dbContext.LinkedDevkits.AddAsync(deviceDbEntry);
            }
            await dbContext.SaveChangesAsync();

            return 1;
        }

        #endregion IJobConsumptionManager

        #region IDeviceStateManager

        /// <summary>
        /// Returns the latest heartbeat request from the device.
        /// </summary>
        /// <param name="deviceId">The id of the device you want the heartbeat payload from.</param>
        /// <returns>The heartbeat payload.</returns>
        public KitHeartbeatRequest? GetLatestKitState(string deviceId)
        {
            KitHeartbeatRequest? kitHeartbeatRequest = null;

            using (var dbContext = SampleDBController.CreateDbContext())
            {
                var deviceDbEntry = dbContext.LinkedDevkits.Find(deviceId);
                if (deviceDbEntry != null)
                {
                    var deviceState = deviceDbEntry.DeserializeDbEntry(); 
                    kitHeartbeatRequest = deviceState.LatestAgentState;
                }
            }

            return kitHeartbeatRequest;
        }

        #endregion IDeviceStateManager
    }
}
