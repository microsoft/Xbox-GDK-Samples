//-----------------------------------------------------------------------------
// IDesiredStateController.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

namespace XboxAgentServerSample.StateManagement
{
    public interface IDesiredStateController
    {
        void CheckAndSetDeviceState(string deviceId);

        DesiredState DesiredState { get; set; }
    }
}
