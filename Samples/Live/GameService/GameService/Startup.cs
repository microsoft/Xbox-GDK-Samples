//-----------------------------------------------------------------------------
// Startup.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using GameService.Logging;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.HttpOverrides;       // Needed for Linux stand alone server support
using Microsoft.CorrelationVector;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using Microsoft.SimpleXboxDelegatedAuth;
using Microsoft.SimpleXboxSecureTokens;
using System;
using System.Net.Http;
using System.Runtime.InteropServices;
using System.Text;


//  User-secret ID note
//  If this is running locally on your development PC,
//  use the secrets.json file to specify your Relying Party Cert
//  and Business Partner Cert thumbprints.
//
//  Added to binary in GameService.csproj file <UserSecretsId>
//  See this article for more info on using user-secrets
//  https://docs.microsoft.com/en-us/aspnet/core/security/app-secrets?tabs=windows&view=aspnetcore-2.2#SecretManager  

namespace GameService
{
    public class Startup
    {
        private ILogger mLogger;
        private CorrelationVector mCv;

        public Startup(IConfiguration configuration)
        {
            Configuration = configuration;

            //  With .NET Core, using a generic host saves us a lot of the initialization that 
            //  previous versions of the sample required by using a Web Host.  Generic Host automatically
            //  configures the app settings locations, user-secrets, and much more.  See the following:
            //  https://learn.microsoft.com/en-us/aspnet/core/fundamentals/host/generic-host
        }

        public IConfiguration Configuration { get; }

        /// <summary>
        /// This method gets called by the runtime when it starts.
        /// Use this method to add services to the container.
        /// </summary>
        public void ConfigureServices(IServiceCollection services)
        {
            //  In-memory cache
            services.AddMemoryCache();

            //  So that we can use an HttpClientFactory for better performance
            //  and proper management of HttpClients see the following:
            //  https://www.stevejgordon.co.uk/introduction-to-httpclientfactory-aspnetcore
            //  https://aspnetmonsters.com/2016/08/2016-08-27-httpclientwrong/
            services.AddHttpClient();

            //  Persistent Cache database initialization
            //  Initialize our persistent cache database connection if it exists in the app settings
            //  otherwise the server will fall-back to a temporary in-memory one for token caching
            var connectionString = GameServicePersistentDBController.GetConnectionString(Configuration);
            if (!String.IsNullOrEmpty(connectionString))
            {
                //  This is needed at startup for creating EF Core migrations for persistent Databases.
                //  Add any more contexts and connections here for other persistent databases you create.
                services.AddDbContext<GameServicePersistentDBContext>
                    (options => options.UseSqlServer(connectionString));
            }
            else
            {
                services.AddDbContext<GameServicePersistentDBContext>
                    (options => options.UseInMemoryDatabase("GameServiceTmpDB"));
            }

            //  Complete the rest of the needed startup and configuration for ASP.Net Core
            //  and our server
            services.AddMvc(options => options.EnableEndpointRouting = false);

            services.AddMvc();

            // Add the configuration singleton here
            services.AddSingleton<IConfiguration>(Configuration);
         
        }

        // This method gets called by the runtime. Use this method to configure the HTTP request pipeline.
        public void Configure(IApplicationBuilder app, ILogger<Startup> logger, IMemoryCache serverCache, IHttpClientFactory httpClientFactory)
        {
            //  Startup logging and Correlation Vector
            mLogger = logger;
            mCv = new CorrelationVector();
            mLogger.StartupInfo(mCv.Value, "Starting Initialization of server");

            //  First handle any incoming calls and re-direct / forward as needed depending on server platform
            if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            {
                //  To enable common Linux ASP.NET functionality not used on Windows, add "LINUX" to the 
                //  Conditional compilation symbols text box (Project Properties -> Build -> All Configurations) 

                //  When running on Linux ASP.Net core runs on Kestrel and needs a reverse proxy to take
                //  the incoming HTTP calls and then fwd them to the Kestrel service.  This sets up the
                //  service for handling this forward process. 
                //  See: https://docs.microsoft.com/en-us/aspnet/core/host-and-deploy/linux-nginx?view=aspnetcore-2.1&tabs=aspnetcore2x
                app.UseForwardedHeaders(new ForwardedHeadersOptions
                {
                    ForwardedHeaders = ForwardedHeaders.XForwardedFor | ForwardedHeaders.XForwardedProto
                });
            }
            else if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            {
                //  Forces any HTTP traffic to go to HTTPS for Windows Servers
                //  On Linux this will be done by the Reverse Proxy and we want HTTP to be coming in
                //  from the Reverse Proxy to the running service
                app.UseHttpsRedirection();
            }
            else
            {
                var message = string.Format("Unsupported or untested OS.  GameService sample has been tested on Windows and Linux only. {0}", RuntimeInformation.OSDescription);
                mLogger.StartupWarning(mCv.Value, message, null);
            }

            try
            {
                //  Initialize the XSTS handling controllers to use the HttpClientFactory
                //  This allows for better management of HttpClients.
                //
                //  See the following for more information:
                //  https://www.stevejgordon.co.uk/introduction-to-httpclientfactory-aspnetcore
                //  https://aspnetmonsters.com/2016/08/2016-08-27-httpclientwrong/
                XstsCertController.CreateHttpClientFunc = httpClientFactory.CreateClient;
                XstsSigningKeyController.CreateHttpClientFunc = httpClientFactory.CreateClient;

                //  Initialize the controllers to retrieve and cache the current certs and keys
                //  so we don't have to at runtime unless we see a new one
                var xstsCertController = new XstsCertController(serverCache);
                var signingKeyController = new XstsSigningKeyController(serverCache);

                mLogger.StartupInfo(mCv.Value, "Initialing Relying Party cert...");
                string relyingPartyThumbprint = Configuration.GetValue(GameServiceConstants.RPCertThumbprintKey, "");
                var relyingPartyCert = xstsCertController.GetCachedCertWithThumbprint(relyingPartyThumbprint);

                mLogger.StartupInfo(mCv.Value, "Initialing Xbl JKU Signing Keys and Certs...");
                var signingKeyTask = signingKeyController.GetAndCacheCurrentSigningKeysAsync();

                var cachedKeys = signingKeyTask.Result;
                var signingKeysStringBuilder = new StringBuilder();
                signingKeysStringBuilder.AppendFormat("Cached Xbox Live signing keys: ");
                foreach (var key in cachedKeys)
                {
                    signingKeysStringBuilder.AppendFormat("[{0}] ", key.KeyId);
                }
                mLogger.StartupInfo(mCv.Value, signingKeysStringBuilder.ToString());

                signingKeyTask.Wait();
            }
            catch (Exception ex)
            {
                //  We will always need a Relying Party to handle tokens
                //  so throw the exception to stop the server.
                mLogger.StartupError(mCv.Value, "", ex);
                throw;
            }

            try
            {
                //  Initialize the delegated auth XSTS handling controllers to use the HttpClientFactory
                //  This allows for better management of HttpClients. See the following for more information:
                //  https://www.stevejgordon.co.uk/introduction-to-httpclientfactory-aspnetcore
                //  https://aspnetmonsters.com/2016/08/2016-08-27-httpclientwrong/
                XstsDelegatedAuthHandler.CreateHttpClientFunc = httpClientFactory.CreateClient;

                //  To get a Service Token we will need the signature policies from
                //  the endpoint cache and the Business Partner Certificate
                mLogger.StartupInfo(mCv.Value, "Initializing Endpoints Cache...");
                var endpointTask = XblEndpointsController.InitializeEndpointCacheAsync();

                mLogger.StartupInfo(mCv.Value, "Initializing BP Cert...");
                var xstsCertController = new XstsCertController(serverCache);
                string businessPartnerThumbprint = Configuration.GetValue(GameServiceConstants.BPCertThumbprintKey, "");
                var businessPartnerCert = xstsCertController.GetCachedCertWithThumbprint(businessPartnerThumbprint);
                xstsCertController.SetDefaultBPCert(businessPartnerCert);

                endpointTask.Wait();

                //  Now that we have the BP cert and signature policies cached
                //  we can request an S-token that our server will cache and use
                mLogger.StartupInfo(mCv.Value, "Initializing Service Token Cache...");
                var sTokenController = new XstsDelegatedAuthHandler(serverCache);
                var sTokenTask = sTokenController.RequestServiceTokenAsync();
                sTokenTask.Wait();

            }
            catch (Exception)
            {
                //  During Startup we may not yet have a Business Partner cert,
                //  but that is OK for just client->server x-token handling.
                //  So, log it as a warning and continue
                mLogger.StartupWarning(mCv.Value, "Unable to initialize BP Cert or Service Token", null);
            }

            //  Middleware injection order for when the calls come in
            //-------------------------------------------------------------

            //  Correlation Vector initialization per call
            app.UseCvMiddleware();

            //  This runs the XSTS token handling and validation code before entering the
            //  target endpoint / function of the incoming request.  
            mLogger.StartupInfo(mCv.Value, "Initialing Asymmetric RFC7516 X-token handling middleware...");

            // Apply the X-token handling middleware only to paths other than the Service-only auth example
            // for getting deleted accounts.  That controller does not require client identity validation.
            app.UseWhen(context =>
                !context.Request.Path.StartsWithSegments("/api/GetDeletedAccounts"),
                appBuilder =>
                {
                    appBuilder.UseXstsAsymmetricRFC7516Middleware();
                });

            //  Run the endpoint / API functionality
            app.UseMvcWithDefaultRoute();

            mLogger.StartupInfo(mCv.Value, "Configured and ready for requests :)");
        }
    }
}
