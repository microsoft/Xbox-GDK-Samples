//-----------------------------------------------------------------------------
// Startup.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using GameService.Logging;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.HttpOverrides;       // Needed for Linux stand alone server support
using Microsoft.AspNetCore.Mvc;
using Microsoft.CorrelationVector;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using Microsoft.XboxSecureTokens;
using Microsoft.XboxSecureTokens.XstsDelegatedAuth;
using System;
using System.Net.Http;
using System.Runtime.InteropServices;
using System.Text;

//  SECTION 1 - user-secret ID note
//  If this is running locally on our development PC
//  load the user-secrets we can use as overrides
//  for our app settings in Azure
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

            //  With .NET Core 3.0, using a generic host saves us a lot of the initialization that 
            //  previous versions of the sample required by using a Web Host.  Generic Host automatically
            //  configures the appsettings locations, user-secrets, and much more.  See the following:
            //  https://docs.microsoft.com/en-us/aspnet/core/fundamentals/host/generic-host?view=aspnetcore-3.0
        }

        public IConfiguration Configuration { get; }

        /// <summary>
        /// This method gets called by the runtime when it starts.
        /// Use this method to add services to the container.
        /// </summary>
        public void ConfigureServices(IServiceCollection services)
        {
            //  SECTION 1 - in-memory cache
            services.AddMemoryCache();

            //  SECTION 6 - We can't do any logging until after this
            //  function has completed in .NET Core 3.0 see:
            //  https://docs.microsoft.com/en-us/aspnet/core/fundamentals/logging/?view=aspnetcore-3.0#create-logs
            //  https://github.com/aspnet/Announcements/issues/353

            //  So that we can use an HttpClientFactory for better performance
            //  and proper management of HttpClients see the following:
            //  https://www.stevejgordon.co.uk/introduction-to-httpclientfactory-aspnetcore
            //  https://aspnetmonsters.com/2016/08/2016-08-27-httpclientwrong/
            services.AddHttpClient();

            //  SECTION 5 - Persistent Cache database initialization
            //  Initialize our persistent cache database connection if it exists in the app settings
            //  otherwise the server will fall-back to a temporary in-memory one for token caching
            //  If we don't have the app settings value for the persistent database, skip.
            //  The code will fall back to an in-memory database that does not need EF Core
            //  migration setup
            var connectionString = GameServicePersistentDBController.GetConnectionString(Configuration);
            if (!String.IsNullOrEmpty(connectionString))
            {
                //  This is needed at startup for creating EF Core migrations for persistent Databases.
                //  Add any more contexts and connections here for other persistent databases you create.
                services.AddDbContext<XstsPersistentDBContext>
                    (options => options.UseSqlServer(connectionString));
            }
            else
            {
                services.AddDbContext<XstsPersistentDBContext>
                    (options => options.UseInMemoryDatabase("GameServiceTmpDB"));
            }

            //  Complete the rest of the needed startup and configuration for ASP.Net Core
            //  and our server

            services.AddMvc(options => options.EnableEndpointRouting = false).SetCompatibilityVersion(CompatibilityVersion.Version_3_0);

            services.AddMvc();

            // Add the configuration singleton here
            services.AddSingleton<IConfiguration>(Configuration);
         
        }

        // This method gets called by the runtime. Use this method to configure the HTTP request pipeline.
        public void Configure(IApplicationBuilder app, ILogger<Startup> logger, IMemoryCache serverCache, IHttpClientFactory httpClientFactory)
        {
            //  SECTION 6 - Startup logging and Correlation Vector
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

            //  Middleware injection order for when the calls come in
            //  Correlation Vector initialization per call
            app.UseCvMiddleware();

            //  SECTION 1 - Initialize the Relying Party secret and Signing Certs
            //  Get the current XBL signing certs and cache them so we don't have to at runtime unless we see
            //  a new cert
            var signingCertController = new XstsSigningCertController(Configuration, httpClientFactory, serverCache, mLogger, mCv.Increment());
            var xstsCertController = new XstsCertController(Configuration, serverCache, mLogger, mCv.Increment());
            try
            {
                //  Start the async task to get and cache the current XBL signing certs for signature validation
                var signingCertTask = signingCertController.GetAndCacheCurrentSigningCerts();

                //  We will handle the XSTS token and validate its authenticity before calling
                //  the target endpoint function of the incoming request.

                //  These lines added to allow the XSTS token handling in the HTTP flow
                //  Run the XSTS token handling middleware class before proceeding to the service API functionality.
                //  By default we use the recommended Asymmetric RFC7516 format, but you can switch the server to
                //  handle the other token types by adding "XSTS_ASYMMETRIC_DRAFT7" or "XSTS_SYMETRIC_RFC7516"
                //  respectively to the Conditional compilation symbols text box of the projects
                //  (Project Properties -> Build -> All Configurations) 

                string tokenFormat = Configuration.GetValue("XSTS_TOKEN_FORMAT", "");
                if (string.IsNullOrEmpty(tokenFormat) || tokenFormat.Equals("XSTS_ASYMMETRIC_RFC7516"))
                {
                    //  Using Asymmetric encryption with a Relying Party Cert
                    //  We recommend using Asymmetric tokens as they provide
                    //  and additional security and validation check to protect your
                    //  service if your shared secret key is compromised
                    mLogger.StartupInfo(mCv.Value, "Initialing Asymmetric RFC7516 Relying Party...");
                    var cert = xstsCertController.InitializeAsymmetricRelyingParty();
                    app.UseXstsAsymmetricRFC7516Middleware();
                }
                else if (tokenFormat.Equals("XSTS_ASYMMETRIC_DRAFT7"))
                {
                    //  Legacy Tokens (Asymmetric Draft 7 JWE with Relying Party certificates) these
                    //  are the default token format for XDP configured titles but we recommend the newer
                    //  RFC7516 edition of this token if at all possible.
                    mLogger.StartupInfo(mCv.Value, "Initialing Asymmetric Draft 7 Relying Party...");
                    xstsCertController.InitializeAsymmetricRelyingParty();
                    app.UseXstsAsymmetricDraft7Middleware();
                }
                else
                {
                    var message = string.Format("XSTS token handling not initialized due to unknown XSTS_TOKEN_FORMAT in App Settings : {0}", tokenFormat);
                    mLogger.StartupWarning(mCv.Value, message, null);
                }

                signingCertTask.Wait();
                var cachedCerts = signingCertTask.Result;
                var signingCertStringBuilder = new StringBuilder();
                signingCertStringBuilder.AppendFormat("Cached {0} Xbox Live signing certs: ", tokenFormat);
                foreach (string certThumprint in cachedCerts)
                {
                    signingCertStringBuilder.AppendFormat("[{0}] ", certThumprint);
                }
                mLogger.StartupInfo(mCv.Value, signingCertStringBuilder.ToString());

            }
            catch (Exception ex)
            {
                //  We will always need a Relying Party to handle tokens
                //  so throw the exception to stop the server.
                mLogger.StartupError(mCv.Value, "", ex);
                throw;
            }




            //  SECTION 2 - BP Cert and Service Token Initialization
            try
            {
                //  To get a Service Token we will need the signature policies from
                //  the endpoint cache and the Business Partner Certificate
                mLogger.StartupInfo(mCv.Value, "Initializing Endpoints Cache...");
                var endpointsController = new XblEndpointsController(mLogger, mCv.Increment());
                var endpointTask = endpointsController.InitializeEndpointCacheAsync(mCv);

                mLogger.StartupInfo(mCv.Value, "Initializing BP Cert...");
                xstsCertController.InitializeBusinessPartnerCert();

                endpointTask.Wait();

                //  Now that we have the BP cert and signature policies cached
                //  we can request an S-token that our server will cache and use
                mLogger.StartupInfo(mCv.Value, "Initializing Service Token Cache...");
                var sTokenController = new XstsServiceTokenController(Configuration, serverCache, mLogger, mCv.Increment());
                var sTokenTask = sTokenController.GetSTokenAsync();
                sTokenTask.Wait();
            }
            catch (Exception)
            {
                //  During Sections 1 and 2 we may not yet have a Business 
                //  Partner cert so log the error and continue
                mLogger.StartupWarning(mCv.Value, "Unable to initialize BP Cert or Service Token", null);
            }

            //  Run the endpoint / API functionality
            app.UseMvcWithDefaultRoute();

            mLogger.StartupInfo(mCv.Value, "Configured and ready for requests :)");
        }
    }
}
