//-----------------------------------------------------------------------------
// InstallApplicationOperator.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

// Ignore Spelling: aum

using Xbox.AgentProtocol;
using XboxAgentServerSample.JobManagement;
using XboxAgentServerSample.Operators.Infrastructure;

namespace XboxAgentServerSample.Operators
{
    /// <summary>
    /// This operator demonstrates how one could install an application onto a device.
    /// </summary>
    public class InstallApplicationOperator : IOperator
    {
        private const string _tempDirectory = "Temp";
        private const string _titlesPath = @"D:\Titles";
        private readonly string _tempPath = $@"{_titlesPath}\{_tempDirectory}";

        private readonly ILogger<InstallApplicationOperator> _logger;
        private readonly ICommandProcessor _commandProcessor;

        public InstallApplicationOperator(ILogger<InstallApplicationOperator> logger, ICommandProcessor commandProcessor)
        {
            _logger = logger;
            _commandProcessor = commandProcessor;
        }

        public async Task Run(string deviceId, Uri installPackage, IEnumerable<Uri>? dependencyPackages = null, string? aumId = null)
        {
            // create the download list with the installation package
            List<FileDownload> files = new()
            {
                new FileDownload
                {
                    Source = installPackage,
                    Destination = $@"{_tempDirectory}\{installPackage.Segments.Last()}"
                }
            };

            // Add the dependency packages to the download list
            if (dependencyPackages != null)
            {
                files.AddRange(dependencyPackages
                    .Select(dependency => new FileDownload
                    {
                        Source = dependency,
                        Destination = $@"{_tempDirectory}\{dependency.Segments.Last()}"
                    }));
            }

            _logger.LogInformation("Downloading Application installation files on device {DeviceId}", deviceId);

            DownloadFilesJobResult downloadResult = await _commandProcessor.DownloadFiles(deviceId, files);

            if (!downloadResult.IsSuccessful())
            {
                throw new OperatorException(downloadResult, nameof(InstallApplicationOperator));
            }

            _logger.LogInformation("Installing the application on device {DeviceId}", deviceId);

            string dependenciesParameter = string.Empty;

            if (dependencyPackages != null &&
                dependencyPackages.Any())
            {
                dependenciesParameter = $"/DependencyPackagePaths:{string.Join(",", dependencyPackages.Select(dp => dp.Segments.Last()))}";
            }

            string installCommand = $"mindeployappx.exe /Add /PackagePath:{installPackage.Segments.Last()} {dependenciesParameter}";

            ExecuteCommandJobResult installResult = await _commandProcessor.ExecuteCommand(deviceId, installCommand, _tempPath);

            if (!installResult.IsSuccessful())
            {
                throw new OperatorException("Installing the application failed.", installResult, nameof(InstallApplicationOperator));
            }

            _logger.LogInformation("Cleaning up Application installation files on device {DeviceId}", deviceId);

            string cleanupCommand = $"cmd /c rmdir /S /Q {_tempPath}";

            var cleanupResult = await _commandProcessor.ExecuteCommand(deviceId, cleanupCommand, _titlesPath);

            if (!cleanupResult.IsSuccessful())
            {
                throw new OperatorException("Failed to remove the application installation files", cleanupResult, nameof(InstallApplicationOperator));
            }

            if (!string.IsNullOrEmpty(aumId))
            {
                _logger.LogInformation("Launching Application on device {DeviceId}", deviceId);

                string launchApplicationCommand = $"wdapp launch {aumId}";

                ExecuteCommandJobResult launchApplicationResult = await _commandProcessor.ExecuteCommand(deviceId, launchApplicationCommand, _titlesPath);

                if (!launchApplicationResult.IsSuccessful())
                {
                    throw new OperatorException("Failed to launch the application", launchApplicationResult, nameof(InstallApplicationOperator));
                }
            }

            _logger.LogInformation("Installing Application on Device Id {DeviceId} has succeeded", deviceId);
        }
    }
}
