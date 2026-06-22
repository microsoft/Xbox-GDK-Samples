//-----------------------------------------------------------------------------
// DesiredState.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using XboxAgentServerSample.StateManagement;

namespace XboxAgentServerSample
{
    /// <summary>
    /// Represents the state a device should be in.
    /// </summary>
    public class DesiredState
    {
        public DesiredState() 
        {
            SandboxId = "";
            OSVersion = "";
            DesiredApplications = new List<DesiredApplication>();
        }

        public DesiredState(string sandboxId, string osVersion, List<DesiredApplication> desiredApplications)
        {
            SandboxId = sandboxId;
            OSVersion = osVersion;
            DesiredApplications = desiredApplications;
        }

        /// <summary>
        /// The desired sandbox Id.
        /// </summary>
        public string SandboxId { get; }

        /// <summary>
        /// The desired recovery version for the device's OS.
        /// </summary>
        public string OSVersion { get; }

        /// <summary>
        /// The list of applications to be installed on the device.
        /// </summary>
        public List<DesiredApplication> DesiredApplications { get; }
    }
}
