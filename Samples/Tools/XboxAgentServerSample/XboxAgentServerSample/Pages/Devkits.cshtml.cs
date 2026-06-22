//-----------------------------------------------------------------------------
// Devkits.cshtml.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Microsoft.AspNetCore.Mvc.RazorPages;
using Microsoft.EntityFrameworkCore;
using XboxAgentServerSample.Data;
using XboxAgentServerSample.Models;

namespace XboxAgentServerSample.Pages
{
    public class DevkitsModel : PageModel
    {
        private readonly XboxAgentServerSample.Data.SampleDBContext _context;

        public DevkitsModel(XboxAgentServerSample.Data.SampleDBContext context)
        {
            _context = context;
        }

        public IList<XboxAgentDevkitState> LinkedDevkits { get;set; } = default!;

        public async Task OnGetAsync()
        {
            if (_context.XboxAgentDevkitState != null)
            {
                using (var dbContext = SampleDBController.CreateDbContext())
                {
                    var jsonData = await dbContext.LinkedDevkits.ToListAsync();
                    var devkits = new List<XboxAgentDevkitState>();
                    foreach (var jsonEntry in jsonData)
                    {
                        devkits.Add(jsonEntry.DeserializeDbEntry());
                    }

                    LinkedDevkits = devkits;
                }
            }
        }
    }
}
