//-----------------------------------------------------------------------------
// Program.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------


using Microsoft.AspNetCore.HttpOverrides;
using Microsoft.CorrelationVector;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.FileProviders;
using Microsoft.SimpleXboxSecureTokens;
using System.Runtime.InteropServices;
using XboxAgentServerSample;
using XboxAgentServerSample.Data;
using XboxAgentServerSample.JobManagement;
using XboxAgentServerSample.Operators;
using XboxAgentServerSample.Operators.Infrastructure;
using XboxAgentServerSample.StateManagement;


var builder = WebApplication.CreateBuilder(args);

// Add services to the container.
builder.Services.AddRazorPages();

//  In-memory cache for the XSTS token certs
builder.Services.AddMemoryCache();

//  So that we can use an HttpClientFactory for better performance
//  and proper management of HttpClients see the following:
//  https://www.stevejgordon.co.uk/introduction-to-httpclientfactory-aspnetcore
//  https://aspnetmonsters.com/2016/08/2016-08-27-httpclientwrong/
builder.Services.AddHttpClient();


builder.Services.AddSingleton<JobsManager>();
builder.Services.AddSingleton<IJobProductionManager>(sp => sp.GetRequiredService<JobsManager>());
builder.Services.AddSingleton<IJobConsumptionManager>(sp => sp.GetRequiredService<JobsManager>());
builder.Services.AddSingleton<IDeviceStateManager>(sp => sp.GetRequiredService<JobsManager>());
builder.Services.AddSingleton<ICommandProcessor, CommandProcessor>();
builder.Services.AddSingleton<IDesiredStateController, DesiredStateController>();
builder.Services.AddSingleton<IOperatorFactory, OperatorFactory>();
builder.Services.AddTransient<IOperatorHelpers, OperatorHelpers>();
builder.Services.AddTransient<InstallApplicationOperator>();
builder.Services.AddTransient<RebootOperator>();
builder.Services.AddTransient<ChangeSandboxIdOperator>();
builder.Services.AddTransient<InstallRecoveryUpdateOperator>();
//builder.Services.AddTransient<FileRetrievalOperator>();

//  https://learn.microsoft.com/en-us/aspnet/core/web-api/advanced/formatting?view=aspnetcore-7.0
builder.Services.AddControllers()
    .AddNewtonsoftJson();

//  Initialize our persistent database for pending consume transactions
//  if the connection string is not in the settings, then we will fall back to
//  an in-memory cache but that would not be safe for a production deployment
var connectionString = builder.Configuration["SampleDBContext"];

if (!String.IsNullOrEmpty(connectionString))
{
    //  This is needed at startup for creating EF Core migrations for persistent Databases.
    //  Add any more contexts and connections here for other persistent databases you create.
    SampleDBController.ConnectionString = connectionString;
    builder.Services.AddDbContext<SampleDBContext>
        (options => options.UseSqlServer(connectionString));
}
else
{

    builder.Services.AddDbContext<SampleDBContext>
        (options => options.UseInMemoryDatabase(SampleConstants.InMemoryDB));
}

var app = builder.Build();

// Configure the HTTP request pipeline.
if (!app.Environment.IsDevelopment())
{
    app.UseExceptionHandler("/Error");
    // The default HSTS value is 30 days. You may want to change this for production scenarios, see https://aka.ms/aspnetcore-hsts.
    app.UseHsts();
}

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
    //mLogger.StartupWarning(mCv.Value, message, null);
}

//app.UseHttpsRedirection();

//  static file hosting for agents to download
app.UseStaticFiles();
var fileProvider = new PhysicalFileProvider(Path.Combine(builder.Environment.ContentRootPath, "MyStaticFiles"));
var requestPath = "/StaticFiles";

app.UseStaticFiles(new StaticFileOptions
{
    FileProvider = fileProvider,
    RequestPath = requestPath
});

app.UseRouting();

app.UseHttpsRedirection()
               .UseRouting()
               .UseEndpoints(endpoints =>
               {
                   endpoints.MapControllers();
               });

app.UseAuthorization();
//  XSTS - Initialize the Relying Party secret and Signing Certs
//  Get the current XBL signing certs and cache them so we don't have to at runtime unless we see
//  a new cert

var mLogger = app.Services.GetService<ILogger<SampleDBContext>>();
var mCv = new CorrelationVector();

var httpClientFactory = app.Services.GetService<System.Net.Http.IHttpClientFactory>();
if (httpClientFactory != null)
{
    XstsCore.CreateHttpClientFunc = httpClientFactory.CreateClient;
}

//  In-memory cache for the XSTS token certs
builder.Services.AddMemoryCache();
var memoryServerCache = app.Services.GetService<IMemoryCache>();
if (memoryServerCache != null)
{
    XstsControllerBase.ServerCache = memoryServerCache;
}

app.MapRazorPages();

app.Run();
