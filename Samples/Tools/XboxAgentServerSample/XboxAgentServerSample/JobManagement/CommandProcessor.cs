//-----------------------------------------------------------------------------
// CommandProcessor.cs
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
    /// This class provides a clean high level interface to sending
    /// jobs to the device and receiving the results of the jobs. Each
    /// job must be awaited before executing the next job.
    /// </summary>
    public class CommandProcessor : ICommandProcessor
    {
        private readonly ILogger<CommandProcessor> _logger;
        private readonly IJobProductionManager _jobProductionManager;

        public CommandProcessor(ILogger<CommandProcessor> logger, IJobProductionManager jobProductionManager)
        {
            _logger = logger;
            _jobProductionManager = jobProductionManager;
        }

        /// <summary>
        /// Sends the <see cref="ExecuteCommandJob"/> to the specified device.
        /// </summary>
        /// <param name="deviceId">The device id of the device to send this job to.</param>
        /// <param name="commandAndArguments">The command and arguments to execute on the device.</param>
        /// <param name="workingDirectory">The working directory to execute the command in.</param>
        /// <param name="waitForExit">Whether to wait for the command to exit or not. Default is <c>true</c></param>
        /// <returns><see cref="ExecuteCommandJobResult"/></returns>
        public async Task<ExecuteCommandJobResult> ExecuteCommand(string deviceId, string commandAndArguments, string workingDirectory = "", bool waitForExit = true)
        {
            RequestedJob requestedJob = new()
            {
                Id = NewId(),
                JobType = JobType.ExecuteCommandJob,
                ExecuteCommandJob = new ExecuteCommandJob
                {
                    CommandLine = commandAndArguments,
                    WorkingDirectory = workingDirectory,
                    WaitForExit = waitForExit
                }
            };

            _logger.LogInformation("Sending execution command {Command}", commandAndArguments);

            LastProcessedJobResult result = await ProcessJob(deviceId, requestedJob);
            return result.ExecuteCommandJobResult;
        }

        /// <summary>
        /// Sends the <see cref="DownloadFilesJob"/> to the specified device.
        /// </summary>
        /// <param name="deviceId">The device id of the device to send this job to.</param>
        /// <param name="files">The list of files to download from and save to on the device.
        /// See <see cref="FileDownload"/> for more information.</param>
        /// <returns><see cref="DownloadFilesJobResult"/></returns>
        public async Task<DownloadFilesJobResult> DownloadFiles(string deviceId, List<FileDownload> files)
        {
            RequestedJob requestedJob = new()
            {
                Id = NewId(),
                JobType = JobType.DownloadFilesJob,
                DownloadFilesJob = new DownloadFilesJob
                {
                    Files = files
                }
            };

            LastProcessedJobResult result = await ProcessJob(deviceId, requestedJob);
            return result.DownloadFilesJobResult;
        }

        /// <summary>
        /// Sends the <see cref="UploadFilesJob"/> to the specified device.
        /// </summary>
        /// <param name="deviceId">The device id of the device to send this job to.</param>
        /// <param name="files">The list of files to upload from the device. See <see cref="FileUpload"/> for more information.</param>
        /// <returns></returns>
        public async Task<UploadFilesJobResult> UploadFiles(string deviceId, List<FileUpload> files)
        {
            RequestedJob requestedJob = new()
            {
                Id = NewId(),
                JobType = JobType.DownloadFilesJob,
                UploadFilesJob = new UploadFilesJob
                {
                    Files = files
                }
            };

            LastProcessedJobResult result = await ProcessJob(deviceId, requestedJob);
            return result.UploadFilesJobResult;
        }

        /// <summary>
        /// Sends the <see cref="Xbox.AgentProtocol.CancelCurrentJob"/> to the specified device.
        /// </summary>
        /// <param name="deviceId">The device id of the device to send this job to.</param>
        public void CancelCurrentJob(string deviceId)
        {
            RequestedJob requestedJob = new()
            {
                Id = NewId(),
                JobType = JobType.CancelCurrentJob,
                CancelCurrentJob = new CancelCurrentJob()
            };
            _jobProductionManager.SetCancelCurrentJob(deviceId, requestedJob);
        }

        private static string NewId() => Guid.NewGuid().ToString("D");

        /// <summary>
        /// This method creates a <see cref="TaskCompletionSource{TResult}"/> to receive the job's result and queues said job
        /// for the specified device.
        /// </summary>
        /// <param name="deviceId">The device id of the device to queue the job for.</param>
        /// <param name="requestedJob">The job to be queued.</param>
        /// <returns><see cref="LastProcessedJobResult"/></returns>
        private async Task<LastProcessedJobResult> ProcessJob(string deviceId, RequestedJob requestedJob)
        {
            TaskCompletionSource<LastProcessedJobResult> tcs = new(TaskCreationOptions.RunContinuationsAsynchronously);

            await _jobProductionManager.SetNextJob(deviceId, requestedJob, tcs);

            return await tcs.Task;
        }
    }

    /// <summary>
    /// Helper methods to quickly check if a job succeeded or not. For use
    /// by consumers of <see cref="CommandProcessor"/>.
    /// </summary>
    public static class ResultExtensions
    {
        public static bool IsSuccessful(this ExecuteCommandJobResult result) => result.ExitCode == 0;

        public static bool IsSuccessful(this DownloadFilesJobResult result) => !result.FailedFileDownloadResults.Any();

        public static bool IsSuccessful(this UploadFilesJobResult result) => !result.FailedFileUploadResults.Any();
    }
}
