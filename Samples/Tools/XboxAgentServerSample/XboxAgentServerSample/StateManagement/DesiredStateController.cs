//-----------------------------------------------------------------------------
// DesiredStateControllers.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using XboxAgentServerSample.JobManagement;
using XboxAgentServerSample.Operators;
using XboxAgentServerSample.Operators.Infrastructure;
using Xbox.AgentProtocol;

namespace XboxAgentServerSample.StateManagement
{
    /// <summary>
    /// Handles checking a device to see if it has the desired state and if not,
    /// handles getting the device into the desired state.
    /// </summary>
    public class DesiredStateController : IDesiredStateController
    {
        private readonly ILogger<DesiredStateController> _logger;
        private readonly IOperatorHelpers _operatorHelpers;
        private readonly IOperatorFactory _operatorFactory;
        private readonly IDeviceStateManager _deviceStateManager;
        private readonly HashSet<string> _devicesBeingProcessed = new();

        private readonly object _devicesBeingProcessedLock = new();

        public DesiredStateController(  ILogger<DesiredStateController> logger,
                                        IOperatorHelpers operatorHelpers,
                                        IOperatorFactory operatorFactory,
                                        IDeviceStateManager deviceStateManager)
        {
            _logger = logger;
            _operatorHelpers = operatorHelpers;
            _operatorFactory = operatorFactory;
            _deviceStateManager = deviceStateManager;
            DesiredState = new DesiredState();
        }

        public DesiredState DesiredState { get; set; }

        public async void CheckAndSetDeviceState(string deviceId)
        {
            // Only do the state check & changes if we have a desired state and
            // the device isn't in the middle of a state check & changed.
            lock (_devicesBeingProcessedLock)
            {
                if (DesiredState != null && !_devicesBeingProcessed.Contains(deviceId))
                {
                    _devicesBeingProcessed.Add(deviceId);
                }
                else
                {
                    return;
                }
            }

            _logger.LogInformation("Checking device {DeviceId} is at the desired state", deviceId);

            //  If any fields are blank, just ignore those
            try
            {
                bool needSandboxChange = false;
                bool needVersionUpdate = false;

                if (!string.IsNullOrEmpty(DesiredState.SandboxId))
                {
                    // Check sandbox id
                    var currentSandboxId = await _operatorHelpers.GetCurrentSandboxId(deviceId);
                    needSandboxChange = currentSandboxId != DesiredState.SandboxId;

                    if (needSandboxChange)
                    {
                        _logger.LogInformation("Device {DeviceId} is set to sandbox {SandboxId} and will be changed", deviceId, currentSandboxId);
                    }
                    else
                    {
                        _logger.LogInformation("Device {DeviceId} is set to the correct sandbox", deviceId);
                    }
                }

                string currentVersion = "";
                // check recovery version
                if (!string.IsNullOrEmpty(DesiredState.OSVersion))
                {
                    if (_deviceStateManager != null)
                    {
                        var latestKitState = _deviceStateManager.GetLatestKitState(deviceId);
                        if (latestKitState != null)
                        {
                            currentVersion = latestKitState.Kit.OsVersion;
                        }
                    }

                    string desiredVersion = DesiredState.OSVersion;
                    needVersionUpdate = currentVersion != desiredVersion;

                    if (needVersionUpdate)
                    {
                        _logger.LogInformation("Device {DeviceId} is at {Version} and will be updated", deviceId, currentVersion);
                    }
                    else
                    {
                        _logger.LogInformation("Device {DeviceId} is up-to-date", deviceId);
                    }
                }

                await SetRecoveryVersionAndSandboxIdState(deviceId, needVersionUpdate, needSandboxChange);

                await SetApplicationInstallState(deviceId);

                _logger.LogInformation("Device {DeviceId} is now at the desired state", deviceId);
            }
            catch (OperatorException e)
            {
                _logger.LogError(e, "Encountered an Operator Exception for device {DeviceId}", deviceId);
            }
            catch (Exception e)
            {
                _logger.LogError(e, "Encountered an Exception for device {DeviceId}", deviceId);
            }

            lock (_devicesBeingProcessedLock)
            {
                _devicesBeingProcessed.Remove(deviceId);
            }
        }

        private async Task SetRecoveryVersionAndSandboxIdState(string deviceId, bool needVersionUpdate, bool needSandboxChange)
        {
            // Since the sandbox id can be changed during a recovery update, let's be smart about it.
            // If an update is needed, do the update with the sandbox id specified if needed.
            // Otherwise, change the sandbox id directly, if needed.
            if (needVersionUpdate)
            {
                InstallRecoveryUpdateOperator installRecoveryUpdateOperator = _operatorFactory.GetOperator<InstallRecoveryUpdateOperator>();
                string sandboxId = "";

                if (needSandboxChange)
                {
                    sandboxId = DesiredState.SandboxId;
                    _logger.LogInformation("Installing OS version {Version} and changing sandbox to {SandboxId}", DesiredState.OSVersion, sandboxId);
                }
                else
                {
                    _logger.LogInformation("Installing OS version {Version}", DesiredState.OSVersion);
                }

                await installRecoveryUpdateOperator.Run(deviceId, DesiredState.OSVersion, false, sandboxId);
            }
            else if (needSandboxChange)
            {
                _logger.LogInformation("Changing sandbox to {SandboxId}", DesiredState.SandboxId);
                ChangeSandboxIdOperator changeSandboxIdOperator = _operatorFactory.GetOperator<ChangeSandboxIdOperator>();
                await changeSandboxIdOperator.Run(deviceId, DesiredState.SandboxId);
            }
        }

        private async Task SetApplicationInstallState(string deviceId)
        {
            Package[] installedApplications = await _operatorHelpers.GetInstalledPackages(deviceId);

            // Figure out what apps need to be installed. This includes apps that are
            // at a different version since the "FullName" includes the version number
            var desiredAppsFullNames = DesiredState
                .DesiredApplications
                .Select(da => da.FullName);

            var installedAppsFullNames = installedApplications
                .Select(ia => ia.FullName);

            var appsToInstallFullNames = desiredAppsFullNames
                .Except(installedAppsFullNames);

            List<DesiredApplication> appsToInstall = DesiredState
                .DesiredApplications
                .Where(da => appsToInstallFullNames
                    .Contains(da.FullName))
                .ToList();

            if (appsToInstall.Any())
            {
                _logger.LogInformation("Device {DeviceId} will have the following applications installed: {ApplicationsToInstall}", deviceId, string.Join(", ", appsToInstall.Select(a => a.InstallPackageUrl.Segments.Last())));
            }

            // Now install/update the apps that need installing
            InstallApplicationOperator installApplicationOperator = _operatorFactory.GetOperator<InstallApplicationOperator>();

            foreach (DesiredApplication desiredApplication in appsToInstall)
            {
                await installApplicationOperator.Run(deviceId, desiredApplication.InstallPackageUrl, desiredApplication.DependencyPackageUrls);
            }
        }
    }
}
