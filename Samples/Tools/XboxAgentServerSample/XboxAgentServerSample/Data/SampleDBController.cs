//-----------------------------------------------------------------------------
// SampleDBController.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Microsoft.EntityFrameworkCore;
using XboxAgentServerSample.Models;

namespace XboxAgentServerSample.Data
{
    public class SampleDBController
    {
        public static string? ConnectionString { get; set; }

        static DbContextOptionsBuilder<SampleDBContext>? optionsBuilder = null;

        //  For better performance and best practice we do not hold the context or auto create
        //  a context to the db when the API is called.  Instead we create the context, use it
        //  and then release it as quick as possible so that our server can handle more load
        //  and incoming calls are not stuck waiting for a locked, but not used context.
        //
        //  using (var dbContext = CreateDbContext())
        //  {
        //      DB read / write requests
        //  }
        //
        //  See the following articles:
        //  Multi-threading and the Entity Framework - https://social.msdn.microsoft.com/Forums/en-US/e5cb847c-1d77-4cd0-abb7-b61890d99fae/multithreading-and-the-entity-framework?forum=adodotnetentityframework
        //  C# working with Entity Framework in a multi threaded server - https://stackoverflow.com/questions/9415955/c-sharp-working-with-entity-framework-in-a-multi-threaded-server
        //  One DbContext per web request... why? - https://stackoverflow.com/questions/10585478/one-dbcontext-per-web-request-why
        public static SampleDBContext CreateDbContext()
        {
            if (optionsBuilder == null)
            {
                optionsBuilder = new DbContextOptionsBuilder<SampleDBContext>();

                //  Using a persistent database is recommended for the 
                //  data types stored in this controller's context.  
                //  The following gets the connection string to the DB
                //  from the appsettings.json, if not there it will default
                //  to use an in-memory database only for simplicity of setup
                //  of the sample.  See the Quick Start Guide for information
                //  on how to setup an Azure DB.  For more info on accessing 
                //  Azure web app settings within code see:
                //  https://blogs.msdn.microsoft.com/cjaliaga/2016/08/10/working-with-azure-app-services-application-settings-and-connection-strings-in-asp-net-core/ 

                if (string.IsNullOrEmpty(ConnectionString))
                {
                    ////////////////////////////////////////////////////////
                    //  NOTE! FOR PRODUCTION DEPLOYMENT USING 
                    //  AN IN-MEMORY DATABASE IS INSUFFICIENT!!!
                    //  
                    //  This code is provided just as an easy starting
                    //  point for the sample to use an in-memory database.
                    //  In deployed code you should be using a real database
                    //  that all servers can access and share this data. 
                    //  Also to prevent data loss in case of a server outage
                    //  See above code for persistent DB connections.
                    /////////////////////////////////////////////////////////
                    optionsBuilder.UseInMemoryDatabase(SampleConstants.InMemoryDB);
                }
                else
                {
                    optionsBuilder.UseSqlServer(ConnectionString);
                }

                optionsBuilder.EnableSensitiveDataLogging(true);
            }

            return new SampleDBContext(optionsBuilder.Options);
        }

        public static async Task<List<XboxAgentDevkit>> GetDevkitsInfo()
        {
            var devkits = new List<XboxAgentDevkit>();

            using (var dbContext = CreateDbContext())
            {
                var jsonData = await dbContext.LinkedDevkits.ToListAsync();
                var jsonDataEntries = new List<XboxAgentDevkitState>();
                foreach (var jsonEntry in jsonData)
                {
                    var devkitState = jsonEntry.DeserializeDbEntry();
                    devkits.Add(devkitState.DevkitInfo);
                }
            }
            return devkits;
        }

        public static async Task<XboxAgentDevkitState?> GetDevkitState(string id)
        {
            XboxAgentDevkitState? devkit = null;

            using (var dbContext = CreateDbContext())
            {
                var jsonData = await dbContext.LinkedDevkits.FindAsync(id);
                if (jsonData != null)
                { 
                    devkit = jsonData.DeserializeDbEntry();
                }
            }
            return devkit;
        }
    }
}
