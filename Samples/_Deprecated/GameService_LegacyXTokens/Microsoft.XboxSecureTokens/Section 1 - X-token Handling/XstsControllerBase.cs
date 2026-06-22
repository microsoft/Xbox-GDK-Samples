//-----------------------------------------------------------------------------
// XstsControllerBase.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using Microsoft.XboxSecureTokens.XstsDelegatedAuth;
using Newtonsoft.Json;
using System;
using System.Collections.Specialized;
using System.Net.Http;
using System.Threading.Tasks;

namespace Microsoft.XboxSecureTokens
{
    /// <summary>
    /// SECTION 1 - Common functions that an API controller will use with XSTS tokens
    /// </summary>
    public class XstsControllerBase : ControllerBase
    {
        protected readonly IConfiguration mConfig; 
        protected readonly IHttpClientFactory mHttpClientFactory;        
        protected IMemoryCache mServerCache;
        protected ILogger mLogger;
        protected CorrelationVector.CorrelationVector mCv;

        public XstsControllerBase(IConfiguration Config,
                                  IHttpClientFactory HttpClientFactory,
                                  IMemoryCache ServerCache,
                                  ILogger Logger)
        {
            mConfig = Config; 
            mServerCache = ServerCache;
            mHttpClientFactory = HttpClientFactory;
            mLogger = Logger;
        }

        public XstsControllerBase()
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
            this.HttpContext.Response.Headers.Add("MS-CV", mCv.Value);
        }

        protected async Task<HttpResponseMessage> MakeRequestWithAuth(string UserId,
                                                                   Uri TargetUri,
                                                                   HttpMethod Method,
                                                                   byte[] RequestContent,
                                                                   NameValueCollection RequestHeaders,
                                                                   string Sandbox,
                                                                   string DelegationToken)
        {
            XstsDelegatedAuthRequest delegatedAuthRequestor = new XstsDelegatedAuthRequest(mHttpClientFactory,
                                                                                           mConfig,
                                                                                           mServerCache,
                                                                                           mLogger,
                                                                                           mCv.Increment(),
                                                                                           DelegationToken);

            using (HttpRequestMessage httpRequest = await delegatedAuthRequestor.CreateRequestWithAuthAndSignature(
                                                                UserId,
                                                                TargetUri,
                                                                Method,
                                                                RequestContent,
                                                                RequestHeaders,
                                                                Sandbox))
            {
                //  call the service b2b
                HttpClient httpClient = mHttpClientFactory.CreateClient();
                return await httpClient.SendAsync(httpRequest);
            }
        }

        protected async Task<T> MakeRequestWithAuth<T>(string UserId,
                                                    Uri TargetUri,
                                                    HttpMethod Method,
                                                    byte[] RequestContent,
                                                    NameValueCollection RequestHeaders,
                                                    string Sandbox,
                                                    string DelegationToken)
        {
            using (var httpResponse = await MakeRequestWithAuth(UserId,
                                                                TargetUri,
                                                                Method,
                                                                RequestContent,
                                                                RequestHeaders,
                                                                Sandbox,
                                                                DelegationToken))
            {

                string responseBody = await httpResponse.Content.ReadAsStringAsync();

                if (!httpResponse.IsSuccessStatusCode)
                {
                    //  Something happened in this call, let's log the important info about it so we can investigate later
                    mLogger.XblFailedRequest(   mCv.Value,
                                                UserId,
                                                DelegationToken,
                                                httpResponse,
                                                responseBody,
                                                System.Text.Encoding.UTF8.GetString(RequestContent));

                }

                return JsonConvert.DeserializeObject<T>(responseBody);
            }
        }

        protected async Task<T> MakeRequestWithServiceAuth<T>(
                                                    Uri TargetUri,
                                                    HttpMethod Method,
                                                    byte[] RequestContent,
                                                    NameValueCollection RequestHeaders,
                                                    string Sandbox)
        {
            using (var httpResponse = await MakeRequestWithServiceAuth(
                                                                TargetUri,
                                                                Method,
                                                                RequestContent,
                                                                RequestHeaders,
                                                                Sandbox))
            {
                string responseBody = await httpResponse.Content.ReadAsStringAsync();

                if (!httpResponse.IsSuccessStatusCode)
                {
                    mLogger.XblFailedRequest( mCv.Value,
                                              "ServiceAuth",
                                              "",
                                              httpResponse,
                                              responseBody,
                                              System.Text.Encoding.UTF8.GetString(RequestContent));
                }

                return JsonConvert.DeserializeObject<T>(responseBody);
            }
        }

        protected async Task<HttpResponseMessage> MakeRequestWithServiceAuth(
                                                                   Uri TargetUri,
                                                                   HttpMethod Method,
                                                                   byte[] RequestContent,
                                                                   NameValueCollection RequestHeaders,
                                                                   string Sandbox)
        {
            XstsServiceAuthRequest delegatedAuthRequestor = new XstsServiceAuthRequest(mHttpClientFactory, mConfig, mServerCache, mLogger, mCv.Increment());

            using (HttpRequestMessage httpRequest = await delegatedAuthRequestor.CreateRequestWithAuthAndSignature(  
                                                                TargetUri,
                                                                Method,
                                                                RequestContent,
                                                                RequestHeaders,
                                                                Sandbox))
            {
                //  call the service b2b
                HttpClient httpClient = mHttpClientFactory.CreateClient();
                return await httpClient.SendAsync(httpRequest);
            }
        }
    }
}
