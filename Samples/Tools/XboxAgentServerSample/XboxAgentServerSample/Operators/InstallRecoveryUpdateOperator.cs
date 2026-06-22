//-----------------------------------------------------------------------------
// InstallRecoveryUpdateOperator.cs
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
    /// Installs the specified recovery update and optionally performs a factory reset
    /// and/or change the sandbox id.
    /// </summary>
    public class InstallRecoveryUpdateOperator : IOperator
    {
        private readonly ILogger<InstallRecoveryUpdateOperator> _logger;
        private readonly ICommandProcessor _commandProcessor;
        private readonly IOperatorHelpers _operatorHelpers;
        private readonly IDeviceStateManager _deviceStateManager;

        public InstallRecoveryUpdateOperator(ILogger<InstallRecoveryUpdateOperator> logger, ICommandProcessor commandProcessor, IOperatorHelpers operatorHelpers, IDeviceStateManager deviceStateManager)
        {
            _logger = logger;
            _commandProcessor = commandProcessor;
            _operatorHelpers = operatorHelpers;
            _deviceStateManager = deviceStateManager;
        }

        public async Task Run(string deviceId, string versionId, bool factoryReset = false, string? desiredSandboxId = null)
        {
            _logger.LogInformation("Installing recovery update on device {DeviceId}", deviceId);

            string factoryResetString = factoryReset ? "factoryReset" : string.Empty;

            string installRecoveryUpdateCommand = $@"J:\unattendedsetuphelper.exe updatelive {versionId} {factoryResetString} {desiredSandboxId}";

            ExecuteCommandJobResult installResult = await _commandProcessor.ExecuteCommand(deviceId, installRecoveryUpdateCommand);

            if (!installResult.IsSuccessful())
            {
                throw new OperatorException($"Failed to install recovery update, Exit Code: {installResult.ExitCode} Output: {installResult.CommandOutput}", nameof(InstallRecoveryUpdateOperator));
            }

            _logger.LogInformation("Waiting for update to complete on device {DeviceId}", deviceId);

            bool restarted = _operatorHelpers.WaitForRebootCompletion(deviceId, 120);

            if (!restarted)
            {
                throw new OperatorException("Failed to receive agent response after recovery update.", nameof(InstallRecoveryUpdateOperator));
            }

            if (_deviceStateManager != null)
            {
                var latestKitState = _deviceStateManager.GetLatestKitState(deviceId);
                if (latestKitState != null)
                {
                    string newVersion = latestKitState.Kit.OsVersion;

                    if (string.IsNullOrEmpty(newVersion))
                    {
                        throw new OperatorException("Failed to identify current version");
                    }

                    _logger.LogInformation("Device {DeviceId} is now at version {Version}", deviceId, newVersion);
                }
            }
        }
    }
}
