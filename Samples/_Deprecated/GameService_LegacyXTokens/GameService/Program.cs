//-----------------------------------------------------------------------------
// Program.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.AspNetCore.Hosting;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;

namespace GameService
{
    public class Program
    {
        public static void Main(string[] args)
        {
            CreateHostBuilder(args).Build().Run();
        }

        public static IHostBuilder CreateHostBuilder(string[] args) =>
            Host.CreateDefaultBuilder(args)
                //  SECTION 6 - .NET Core 3.0 logging initialization
                .ConfigureLogging( logging =>
                {
                    //  the HostBuilder by default adds the Console, Debug, EventSource
                    //  and EventLog providers.  We need to add the following to see the
                    //  log output in the Azure log streams                    
                    logging.AddAzureWebAppDiagnostics();

                })
                .ConfigureWebHostDefaults(webBuilder =>
                {
                    webBuilder.UseStartup<Startup>();
                });
    }
}
