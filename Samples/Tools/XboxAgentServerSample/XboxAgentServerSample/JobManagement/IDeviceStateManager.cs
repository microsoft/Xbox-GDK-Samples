//-----------------------------------------------------------------------------
// IDeviceStateManager.cs
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
    /// This interface is meant for <see cref="XboxAgentServerSample.Operators.IOperator"/> classes that need to
    /// inspect the state of a device.
    /// </summary>
    public interface IDeviceStateManager
    {
        KitHeartbeatRequest? GetLatestKitState(string deviceId);
    }
}
