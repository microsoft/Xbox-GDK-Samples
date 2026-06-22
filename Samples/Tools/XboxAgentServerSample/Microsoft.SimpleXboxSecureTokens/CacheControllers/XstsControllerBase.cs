//-----------------------------------------------------------------------------
// XstsControllerBase.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Caching.Memory;
using System;
using System.Net.Http;

namespace Microsoft.SimpleXboxSecureTokens
{
    /// <summary>
    /// SECTION 1 - Common functions that an API controller will use with XSTS tokens
    /// </summary>
    public class XstsControllerBase : ControllerBase
    {
        /// <summary>
        /// Can be overridden with an HttpClientFactory.CreateClient() if used by your service.
        /// This will greatly help with server performance.  To do this add the following to your
        /// Setup.cs or Program.cs
        ///
        /// var httpClientFactory = app.Services.GetService<System.Net.Http.IHttpClientFactory>();
        /// XstsSigningKeyController.CreateHttpClientFunc = httpClientFactory.CreateClient;
        ///
        /// </summary>
        public static Func<HttpClient> CreateHttpClientFunc = () => new HttpClient();

        /// <summary>
        /// You can set this in your Setup.cs or Program.cs to use your WebApplication's IMemoryCache
        /// to help performance and not have to download common keys and certs each time.
        ///
        /// EX:
        ///     //  In-memory cache for the XstsTokenHandler
        ///     builder.Services.AddMemoryCache();
        ///     XstsSigningKeyController.ServerCache = app.Services.GetService<IMemoryCache>();
        ///
        /// </summary>
        
        public static IMemoryCache? ServerCache;

        public XstsControllerBase()
        {
            //ServerCache = new MemoryCache(new MemoryCacheOptions());
        }
    }
}