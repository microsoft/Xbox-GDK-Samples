//-----------------------------------------------------------------------------
// RebootOperator.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using XboxAgentServerSample.Operators.Infrastructure;

namespace XboxAgentServerSample.Operators
{
    /// <summary>
    /// This Operator demonstrates rebooting a device.
    /// </summary>
    public class RebootOperator : IOperator
    {
        private readonly IOperatorHelpers _operatorHelpers;

        public RebootOperator(IOperatorHelpers operatorHelpers)
        {
            _operatorHelpers = operatorHelpers;
        }

        public async Task Run(string deviceId)
        {
            await _operatorHelpers.Reboot(deviceId);
        }
    }
}
