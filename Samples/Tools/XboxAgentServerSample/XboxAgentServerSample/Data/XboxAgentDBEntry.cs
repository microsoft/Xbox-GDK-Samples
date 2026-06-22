//-----------------------------------------------------------------------------
// XboxAgentDevkitState.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using System.ComponentModel.DataAnnotations;
using Xbox.AgentProtocol;

namespace XboxAgentServerSample.Models
{
    public class XboxAgentDBEntry
    {
        public XboxAgentDBEntry()
        { }

        public XboxAgentDBEntry(XboxAgentDevkitState devkitState)
        {
            UpdateDbEntry(devkitState);
        }

        public void UpdateDbEntry(XboxAgentDevkitState devkitState)
        {
            //  DeveloperDeviceID
            DeveloperDeviceId = devkitState.DeveloperDeviceId;

            //  DevkitInfo
            if (devkitState.DevkitInfo != null)
            {
                DevkitInfo = JsonConvert.SerializeObject(devkitState.DevkitInfo);
            }
            else
            {
                DevkitInfo = "";
            }

            //  PendingJob
            if (devkitState.PendingJob != null)
            {
                PendingJob = JsonConvert.SerializeObject(devkitState.PendingJob);
            }
            else
            {
                PendingJob = "";
            }

            //  CancelCurrentJob
            if (devkitState.CancelCurrentJob != null)
            {
                CancelCurrentJob = JsonConvert.SerializeObject(devkitState.CancelCurrentJob);
            }
            else
            {
                CancelCurrentJob = "";
            }

            //  LatestAgentState
            if (devkitState.LatestAgentState != null)
            {
                LatestAgentState = JsonConvert.SerializeObject(devkitState.LatestAgentState);
            }
            else
            {
                LatestAgentState = "";
            }


            //  NextJob
            if (devkitState.NextJob != null)
            {
                NextJob = JsonConvert.SerializeObject(devkitState.NextJob);
            }
            else
            {
                NextJob = "";
            }

            //  LastJobResult
            if (devkitState.LastJobResult != null)
            {
                LastJobResult = JsonConvert.SerializeObject(devkitState.LastJobResult);
            }

        }

        public XboxAgentDevkitState DeserializeDbEntry()
        {
            var devkitState = new XboxAgentDevkitState
            {
                //  DeveloperDeviceID
                DeveloperDeviceId = DeveloperDeviceId
            };

            //  DevkitInfo
            if (!string.IsNullOrEmpty(DevkitInfo))
            {
                var devkitInfo = JsonConvert.DeserializeObject<XboxAgentDevkit>(DevkitInfo);
                if (devkitInfo != null)
                {
                    devkitState.DevkitInfo = devkitInfo;
                }
            }

            //  PendingJob
            if (!string.IsNullOrEmpty(PendingJob))
            {
                devkitState.PendingJob = JsonConvert.DeserializeObject<RequestedJob>(PendingJob);
            }

            //  CancelCurrentJob
            if (!string.IsNullOrEmpty(CancelCurrentJob))
            {
                devkitState.CancelCurrentJob = JsonConvert.DeserializeObject<RequestedJob>(CancelCurrentJob);
            }

            //  LatestAgentState
            if (!string.IsNullOrEmpty(LatestAgentState))
            {
                devkitState.LatestAgentState = JsonConvert.DeserializeObject<KitHeartbeatRequest>(LatestAgentState);
            }

            //  NextJob
            if (!string.IsNullOrEmpty(NextJob))
            {
                devkitState.NextJob = JsonConvert.DeserializeObject<RequestedJob>(NextJob);
            }

            //  LastJobResult
            if (!string.IsNullOrEmpty(LastJobResult))
            {
                devkitState.LastJobResult = JsonConvert.DeserializeObject<LastProcessedJobResult>(LastJobResult);
            }

            return devkitState;

        }

        [Key]
        public string DeveloperDeviceId { get; set; } = string.Empty;

        /// <summary>
        /// The Devkit's information
        /// </summary>
        public string DevkitInfo { get; set; } = string.Empty;

        /// <summary>
        /// The current job the device is meant to process.
        /// </summary>
        public string PendingJob { get; set; } = string.Empty;

        /// <summary>
        /// The next job to be processed by the device.
        /// </summary>
        public string NextJob { get; set; } = string.Empty;

        /// <summary>
        /// Populated when the current job needs to be canceled.
        /// </summary>
        public string CancelCurrentJob { get; set; } = string.Empty;

        /// <summary>
        /// The last heartbeat request object received from the device.
        /// </summary>
        public string LatestAgentState { get; set; } = string.Empty;

        /// <summary>
        /// The last heartbeat request object received from the device.
        /// </summary>
        public string LastJobResult { get; set; } = string.Empty;

        // <summary>
        /// Used to handle asynchronously receiving the job result.
        /// </summary>
        //public TaskCompletionSource<LastProcessedJobResult> NextJobResultTcs { get; set; }
    }
}
