//-----------------------------------------------------------------------------
// XboxAgentDevkitState.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using System.ComponentModel.DataAnnotations;
using Xbox.AgentProtocol;

namespace XboxAgentServerSample.Models
{
    public class XboxAgentDevkitState
    {
        [Key]
        public string DeveloperDeviceId { get; set; }

        /// <summary>
        /// The Devkit's information
        /// </summary>
        public XboxAgentDevkit DevkitInfo { get; set; }

        /// <summary>
        /// The current job the device is meant to process.
        /// </summary>
        public RequestedJob? PendingJob { get; set; }

        /// <summary>
        /// The next job to be processed by the device.
        /// </summary>
        public RequestedJob? NextJob { get; set; }

        /// <summary>
        /// Populated when the current job needs to be canceled.
        /// </summary>
        public RequestedJob? CancelCurrentJob { get; set; }

        /// <summary>
        /// The last heartbeat request object received from the device.
        /// </summary>
        public KitHeartbeatRequest? LatestAgentState { get; set; }

        // <summary>
        /// Used to handle asynchronously receiving the job result.
        /// </summary>
        public LastProcessedJobResult? LastJobResult { get; set; }

        public XboxAgentDevkitState()
        {
            DeveloperDeviceId = string.Empty;
            DevkitInfo = new XboxAgentDevkit
            {
                DeveloperDeviceId = ""
            };
        }
    }
}
