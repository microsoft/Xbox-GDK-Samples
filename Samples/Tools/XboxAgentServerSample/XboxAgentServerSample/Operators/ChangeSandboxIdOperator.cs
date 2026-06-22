//-----------------------------------------------------------------------------
// ChangeSandboxIdOperator.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Xbox.AgentProtocol;
using XboxAgentServerSample.JobManagement;
using XboxAgentServerSample.Operators.Infrastructure;

namespace XboxAgentServerSample.Operators
{
    /// <summary>
    /// This operator demonstrates changing the sandbox id for a specified device.
    /// </summary>
    public class ChangeSandboxIdOperator : IOperator
    {
        private readonly ILogger<InstallApplicationOperator> _logger;
        private readonly ICommandProcessor _commandProcessor;
        private readonly IOperatorHelpers _operatorHelpers;

        public ChangeSandboxIdOperator(ILogger<InstallApplicationOperator> logger, ICommandProcessor commandProcessor, IOperatorHelpers operatorHelpers)
        {
            _logger = logger;
            _commandProcessor = commandProcessor;
            _operatorHelpers = operatorHelpers;
        }

        public async Task Run(string deviceId, string desiredSandboxId)
        {
            // Check if the device is already set with the desired sandbox id
            string currentSandboxId = await _operatorHelpers.GetCurrentSandboxId(deviceId);

            // if the device is already set with the desired sandbox id, then fail successfully.
            if (currentSandboxId == desiredSandboxId)
            {
                throw new OperatorException($"Device is already set to the desired SandboxId {desiredSandboxId}", nameof(ChangeSandboxIdOperator));
            }

            // Change the sandbox id
            string setSandboxIdCommand = $"WdConfig.exe set SandboxId={desiredSandboxId}";

            ExecuteCommandJobResult setSandboxIdResult = await _commandProcessor.ExecuteCommand(deviceId, setSandboxIdCommand);

            if (!setSandboxIdResult.IsSuccessful())
            {
                throw new OperatorException($"Failed to change the SandboxId to {desiredSandboxId}", setSandboxIdResult, nameof(ChangeSandboxIdOperator));
            }

            await _operatorHelpers.Reboot(deviceId);

            // Check that the device is now set with the desired sandbox id
            currentSandboxId = await _operatorHelpers.GetCurrentSandboxId(deviceId);

            if (currentSandboxId != desiredSandboxId)
            {
                throw new OperatorException($"Failed to set device to SandboxId {desiredSandboxId}", nameof(ChangeSandboxIdOperator));
            }

            // Helper method for getting the current sandbox id
        }
    }
}
