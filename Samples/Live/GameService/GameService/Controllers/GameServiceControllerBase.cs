//-----------------------------------------------------------------------------
// GameServiceControllerBase.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma warning disable IDE0063 // Use simple 'using' statement

using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Microsoft.CorrelationVector;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using System.Net.Http;

namespace GameService
{
    /// <summary>
    /// Common functions that an API controller will use with XSTS tokens
    /// </summary>
    public class GameServiceControllerBase : ControllerBase
    {

        protected readonly IConfiguration mConfig;
        protected readonly IHttpClientFactory mHttpClientFactory;
        protected IMemoryCache mServerCache;
        protected ILogger mLogger;
        protected CorrelationVector mCv;

        public GameServiceControllerBase(IConfiguration Config,
                                         IHttpClientFactory HttpClientFactory,
                                         IMemoryCache ServerCache,
                                         ILogger Logger)
        {
            mConfig = Config;
            mServerCache = ServerCache;
            mHttpClientFactory = HttpClientFactory;
            mLogger = Logger;
        }

        public GameServiceControllerBase()
        {
        }

        protected void InitializeLoggingCv()
        {
            //  This can't be set in the constructor because you only have an HttpContext
            //  during the actual service endpoint function.  So call this at the start
            //  of each incoming request.
            mCv = (Microsoft.CorrelationVector.CorrelationVector)this.HttpContext.Items["MS-CV"];
        }

        protected void FinalizeLoggingCv()
        {
            this.HttpContext.Items["MS-CV"] = mCv.Value;
            this.HttpContext.Response.Headers.Remove("MS-CV");
            this.HttpContext.Response.Headers.Append("MS-CV", mCv.Value);
        }
    }
}

#pragma warning restore IDE0063 // Use simple 'using' statement
