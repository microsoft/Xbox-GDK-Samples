//-----------------------------------------------------------------------------
// XboxAgentDevkit.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

// Ignore Spelling: Ip

using System.ComponentModel.DataAnnotations;
using Xbox.AgentProtocol;

namespace XboxAgentServerSample.Models
{
    public class XboxAgentDevkit
    {
        [Key]
        public required string DeveloperDeviceId { get; set; }
        public string ConsoleType { get; set; }
        public string HostName { get; set; }
        public string IpAddress { get; set; }
        public string OsVersion { get; set; }
        public Package RunningApplication { get; set; }
        public string RunningApplicationId { get; set; }
        public long Uptime { get; set; }

        public XboxAgentDevkit() 
        {
            DeveloperDeviceId = "";
            ConsoleType = "";
            HostName = "";
            IpAddress = "";
            OsVersion = "";
            RunningApplication = new Package();
            RunningApplicationId = "";
            Uptime = 0;
        }
    }
}
