//-----------------------------------------------------------------------------
// Index.cshtml.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.RazorPages;
using Microsoft.EntityFrameworkCore;
using XboxAgentServerSample.Data;
using XboxAgentServerSample.Models;

namespace XboxAgentServerSample.Pages
{
    public class IndexModel : PageModel
    {
        private readonly ILogger<IndexModel> _logger;
        private readonly XboxAgentServerSample.Data.SampleDBContext _context;

        public IList<XboxAgentDevkitState> XboxAgentDevkitStates { get; set; } = default!;
        public XboxAgentDevkitState? SelectedDevkit { get; set; }

        public IndexModel(ILogger<IndexModel> logger, XboxAgentServerSample.Data.SampleDBContext context)
        {
            _logger = logger;
            _context = context;
        }

        public async Task OnGet(string deviceId)
        {
            if (_context.XboxAgentDevkitState != null)
            {
                using (var dbContext = SampleDBController.CreateDbContext())
                {
                    var jsonData = await dbContext.LinkedDevkits.ToListAsync();
                    var jsonDataEntries = new List<XboxAgentDevkitState>();
                    foreach (var jsonEntry in jsonData)
                    {
                        jsonDataEntries.Add(jsonEntry.DeserializeDbEntry());
                    }

                    XboxAgentDevkitStates = jsonDataEntries;
                }
            }
        }

        public async Task<IActionResult> OnPostAsync(string deviceID)
        {
            if (_context.XboxAgentDevkitState != null)
            {
                using (var dbContext = SampleDBController.CreateDbContext())
                {
                    var jsonData = await dbContext.LinkedDevkits.ToListAsync();
                    var jsonDataEntries = new List<XboxAgentDevkitState>();
                    foreach (var jsonEntry in jsonData)
                    {
                        var devkitState = jsonEntry.DeserializeDbEntry();
                        jsonDataEntries.Add(devkitState);
                        if(devkitState.DeveloperDeviceId.Equals(deviceID))
                        {
                            SelectedDevkit = devkitState;
                        }
                    }

                    XboxAgentDevkitStates = jsonDataEntries;
                }
            }
            return RedirectToPage("./Index");
        }
    }
}