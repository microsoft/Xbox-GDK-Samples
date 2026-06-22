//-----------------------------------------------------------------------------
// FileRetrievalOperator.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

// Ignore Spelling: Accessor

using System.Text.RegularExpressions;
using Xbox.AgentProtocol;
using XboxAgentServerSample.JobManagement;
using XboxAgentServerSample.Operators.Infrastructure;

namespace XboxAgentServerSample.Operators
{
    /// <summary>
    /// This operator demonstrates uploading all files from the _logFilePath path.
    /// </summary>
    public class FileRetrievalOperator : IOperator
    {
        private readonly ICommandProcessor _commandProcessor;
        private readonly Uri _fileUrl;

        private readonly Regex _fileRegex = new(@"^\d{2}/\d{2}/\d{4}\s\s\d{2}:\d{2}\s(?:AM|PM)\s+\d+\s(?<FileName>.+)$", RegexOptions.Multiline);

        public FileRetrievalOperator(ICommandProcessor commandProcessor, IHttpContextAccessor hostContextAccessor)
        {
            _commandProcessor = commandProcessor;
            if(hostContextAccessor != null &&
                hostContextAccessor.HttpContext != null)
            {
                var request = hostContextAccessor.HttpContext.Request;
                _fileUrl = new Uri($"{request.Scheme}://{request.Host}/api/Files/");
            }
            else
            {
                throw new InvalidOperationException("ContextAccessor HttpContext or Request are null.");
            }
        }

        public async Task Run(string deviceId, string directoryPath)
        {
            var fullPath = Path.GetFullPath(directoryPath);

            if (!Path.IsPathRooted(fullPath))
            {
                throw new OperatorException($"Specified directory path '{fullPath}' isn't rooted.", nameof(FileRetrievalOperator));
            }

            // Get a list of files
            const string listFilesCommand = "cmd /c dir /-c";
            ExecuteCommandJobResult listFilesResult = await _commandProcessor.ExecuteCommand(deviceId, listFilesCommand, fullPath);

            if (!listFilesResult.IsSuccessful())
            {
                throw new OperatorException("File listing failed", listFilesResult, nameof(FileRetrievalOperator));
            }

            // Parse dir for the actual file names
            var fileMatches = _fileRegex.Matches(listFilesResult.CommandOutput);

            List<FileUpload> fileUploads = fileMatches.AsEnumerable().Select(fileMatch =>
            {
                var filename = fileMatch.Groups["FileName"].Value.Trim();
                return new FileUpload
                {
                    Source = $@"{fullPath}\{filename}",
                    Destination = new Uri(_fileUrl, filename)
                };
            }).ToList();

            // Request the upload of files
            UploadFilesJobResult uploadResult = await _commandProcessor.UploadFiles(deviceId, fileUploads);

            if (!uploadResult.IsSuccessful())
            {
                throw new OperatorException(uploadResult, nameof(FileRetrievalOperator));
            }
        }
    }
}
