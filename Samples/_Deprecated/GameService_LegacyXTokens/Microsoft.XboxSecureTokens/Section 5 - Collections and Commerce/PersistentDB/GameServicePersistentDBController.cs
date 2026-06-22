//-----------------------------------------------------------------------------
// XstsPersistentDBController.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;

namespace Microsoft.XboxSecureTokens
{
    public static class GameServicePersistentDBController
    {
        public static string GetConnectionString(IConfiguration Configuration)
        {
            return Configuration.GetConnectionString("GameServbicePersistentDB");
        }

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
        public static XstsPersistentDBContext CreateDbContext(IConfiguration Config, CorrelationVector.CorrelationVector mCv, ILogger mLogger)
        {
            var optionsBuilder = new DbContextOptionsBuilder<XstsPersistentDBContext>();

            //  Using a persistent database is recommended for the 
            //  data types stored in this controller's context.  
            //  The following gets the connection string to the DB
            //  from the appsettings.json, if not there it will default
            //  to use an in-memory database only for simplicity of setup
            //  of the sample.  See the Quick Start Guide for information
            //  on how to setup an Azure DB.  For more info on accessing 
            //  Azure web app settings within code see:
            //  https://blogs.msdn.microsoft.com/cjaliaga/2016/08/10/working-with-azure-app-services-application-settings-and-connection-strings-in-asp-net-core/ 

            string connectionString = GetConnectionString(Config);
            if (string.IsNullOrEmpty(connectionString))
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
                optionsBuilder.UseInMemoryDatabase("GameServiceTmpDB");
                mLogger.StartupInfo(mCv.Value, "Using in -memory database for pending consume transactions, these should be saved in a real database");
            }
            else
            {
                optionsBuilder.UseSqlServer(connectionString);
            }

            return new XstsPersistentDBContext(optionsBuilder.Options);
        }
    }
}
