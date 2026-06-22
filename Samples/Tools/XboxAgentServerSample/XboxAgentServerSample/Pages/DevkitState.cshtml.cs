//-----------------------------------------------------------------------------
// DevkitState.cshtml.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

// Ignore Spelling: Devkit Devkits

using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.RazorPages;
using System;
using XboxAgentServerSample.Data;
using XboxAgentServerSample.JobManagement;
using XboxAgentServerSample.Models;

namespace XboxAgentServerSample.Pages
{
    public class DevkitStateModel : PageModel
    {
        private readonly SampleDBContext _context;
        private readonly ICommandProcessor _commandProcessor;

        public IList<XboxAgentDevkit> XboxAgentDevkits { get; set; } = default!;

        [BindProperty]
        public XboxAgentDevkitState? SelectedDevkit { get; set; }
        [BindProperty]
        public string CommandString { get; set; }
        [BindProperty]
        public bool CommandWaitForExit { get; set; }
        [BindProperty]
        public string FileDownloadURI { get; set; }
        [BindProperty]
        public string FileDestination { get; set; }

        public DevkitStateModel(SampleDBContext context,
                                ICommandProcessor commandProcessor)
        {
            _context = context;
            _commandProcessor = commandProcessor;
            CommandString = "";
            CommandWaitForExit = true;
            FileDownloadURI = "";
            FileDestination = "";
        }

        public async Task OnGet(string id)
        {
            if (_context.XboxAgentDevkitState != null)
            {
                XboxAgentDevkits = await SampleDBController.GetDevkitsInfo();
                SelectedDevkit = await SampleDBController.GetDevkitState(id);
            }
        }

        public async Task<IActionResult> OnPostAsync(string id, bool cancelJob = false)
        {
            var targetDevkit = await SampleDBController.GetDevkitState(id);

            if (targetDevkit != null)
            {
                if (cancelJob)
                {
                    _commandProcessor.CancelCurrentJob(id);
                }

                //  Check for an Execute Command
                if (!String.IsNullOrEmpty(CommandString))
                {
                    await _commandProcessor.ExecuteCommand(id, CommandString, "", CommandWaitForExit);
                }
                else if (!String.IsNullOrEmpty(FileDownloadURI))
                {
                    var downloadList = new List<Xbox.AgentProtocol.FileDownload>();


                    if (Uri.TryCreate(FileDownloadURI, UriKind.Absolute, out Uri? downloadUri))
                    {
                        var filename = Path.GetFileName(downloadUri.LocalPath);

                        if (String.IsNullOrEmpty(FileDestination))
                        {
                            FileDestination = "temp";
                        }

                        if (String.IsNullOrEmpty(filename))
                        {
                            filename = "temp.data";
                        }

                        var download = new Xbox.AgentProtocol.FileDownload
                        {
                            Source = downloadUri,
                            Destination = FileDestination + "\\" + filename
                        };

                        downloadList.Add(download);

                        await _commandProcessor.DownloadFiles(id, downloadList);
                    }
                    else
                    {
                        throw new InvalidOperationException("Invalid download URL provided: " + FileDownloadURI);
                    }
                }
            }

            return Redirect("./DevkitState?id="+id);
        }
    }
}
