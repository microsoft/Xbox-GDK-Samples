//-----------------------------------------------------------------------------
// ICommandProcessor.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Xbox.AgentProtocol;

namespace XboxAgentServerSample.JobManagement
{
    public interface ICommandProcessor
    {
        Task<DownloadFilesJobResult> DownloadFiles(string deviceId, List<FileDownload> files);

        Task<ExecuteCommandJobResult> ExecuteCommand(string deviceId, string commandAndArguments, string workingDirectory = "", bool waitForExit = true);

        Task<UploadFilesJobResult> UploadFiles(string deviceId, List<FileUpload> files);

        void CancelCurrentJob(string deviceId);
    }
}
