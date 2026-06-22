//-----------------------------------------------------------------------------
// XstsServiceAuth.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using System;
using System.Collections.Specialized;
using System.Linq;
using System.Net.Http;
using System.Threading.Tasks;

namespace Microsoft.XboxSecureTokens.XstsDelegatedAuth
{
    /// SECTION 2 - S-token supporting functions
    public class XstsServiceAuthRequest : XstsDelegatedAuthRequest
    {
        internal new XstsServiceAuthRequestBody RequestBody { get; set; }

        // Place to store the Config object and use in this controller
        public XstsServiceAuthRequest(IHttpClientFactory HttpClientFactory, 
                                      IConfiguration config,
                                      IMemoryCache ServerCache,
                                      ILogger Logger, 
                                      string CorrelationVectorString)
            : base(HttpClientFactory, config, ServerCache, Logger, CorrelationVectorString, "")
        {
            this.mServerCache = ServerCache;
            this.RequestBody = new XstsServiceAuthRequestBody();
        }

        /// <summary>
        /// Provides a cached Service Auth XSTS token for b2b communication with an XBL endpoint
        /// </summary>
        /// <param name="TargetRelyingParty"></param>
        /// <param name="SandboxId"></param>
        /// <returns></returns>
        public async Task<XstsCachedToken> GetCachedAuthTokenAsync(
            string TargetRelyingParty,
            string SandboxId)
        {
            XstsCachedToken cachedToken = null;
            DateTime currentUTC = DateTime.Now.ToUniversalTime();

            string cacheKey = string.Format(XstsConstants.CacheKeyFormat, 
                                            XstsConstants.ServiceName,
                                            TargetRelyingParty,
                                            SandboxId);

            cachedToken = mServerCache.Get<XstsCachedToken>(cacheKey);

            //  if it isn't cached create one
            if (cachedToken == null)
            {
                var sTokenController = new XstsServiceTokenController(mConfig, mServerCache, mLogger, mCv.Value);
                mCv.Increment();
                var sToken = await sTokenController.GetSTokenAsync();

                //  Token is not in the cache, try to re-initialize
                B2BXToken token = await RequestServerAuthTokenAsync(TargetRelyingParty, SandboxId, sToken);
                token.SandboxId = SandboxId;
                token.RelyingParty = TargetRelyingParty;

                //  now cache the token for other requests
                cachedToken = CacheServiceAuthToken(token, cacheKey);
            }

            return cachedToken;
        }

        /// <summary>
        /// Provides a cached Delegated Auth XSTS token for b2b communication with an XBL endpoint
        /// </summary>
        /// <param name="UserId"></param>
        /// <param name="TargetRelyingParty"></param>
        /// <param name="SandboxId"></param>
        /// <returns></returns>
        public XstsCachedToken CacheServiceAuthToken(
            B2BXToken Token,
            string CacheKey)
        {
            if (Token == null)
            {
                throw new ArgumentNullException("Token");
            }
            if (CacheKey == null)
            {
                throw new ArgumentNullException("CacheKey");
            }
            
            //  create the cached token object, which only has the proof key and 
            //  token itself, we don't need the rest so we trim it off to save memory
            //  in the cache
            var tokenToCache = new XstsCachedToken()
            {
                Token = Token.Token,
                ProofKeyInBytes = Token.ProofKeyInBytes
            };

            //  set the expiration time to the lifetime of the token from right now.
            var cacheExpirationOptions = new MemoryCacheEntryOptions();
            cacheExpirationOptions.SetPriority(CacheItemPriority.Normal);
            cacheExpirationOptions.SetAbsoluteExpiration(DateTimeOffset.UtcNow.AddTicks(Token.NotAfter.Ticks - Token.IssueInstant.Ticks));

            mServerCache.Set<XstsCachedToken>(CacheKey, tokenToCache, cacheExpirationOptions);

            return tokenToCache;
        }

        internal async Task<HttpRequestMessage> CreateRequestWithAuthAndSignature(
            Uri TargetUri,
            HttpMethod Method,
            byte[] RequestContent,
            NameValueCollection RequestHeaders,
            string Sandbox)
        {
            await InitializeSTokenAndEndpoint(TargetUri);

            var cachedToken = await GetCachedAuthTokenAsync( mTargetEndpoint.RelyingParty, Sandbox);

            //  Add the Authorization header with the Service Auth XSTS token
            RequestHeaders.Add("Authorization", cachedToken.Token);

            return CreateRequestWithSignatureInternal(
                TargetUri,
                Method,
                cachedToken.ProofKeyInBytes,
                RequestContent,
                RequestHeaders);
        }

        public async Task<B2BXToken> RequestServerAuthTokenAsync(
            string TargetRelyingParty,
            string SandboxId,
            XstsServiceToken ServiceToken)
        {
            XstsServiceAuthRequestBody requestBody = new XstsServiceAuthRequestBody()
            {
                //  Construct the request body
                RelyingParty = TargetRelyingParty,
                Properties = new XstsServerAuthRequestProperties()
                {
                    SandboxId = SandboxId,
                    ServiceToken = ServiceToken.Token
                },
                TokenType = "JWT"
            };

            // Serialize our request body to a UTF8 byte array
            string requestBodyString = JsonConvert.SerializeObject(requestBody);
            byte[] requestBodyContent = System.Text.Encoding.UTF8.GetBytes(requestBodyString);

            //  All headers except for the Authorization and Signature headers
            //  which will be added on in the next step
            var requestHeaders = new NameValueCollection
            {
                { "x-xbl-contract-version", " 1" },
                { XstsConstants.ContentTypeHeaderKey, "application/json" }
            };

            //  Now pass these values to get the correct Delegated Auth and Signature headers for
            //  the request
            HttpRequestMessage httpRequest = CreateRequestWithSignatureInternal(
                this.RequestUri,
                HttpMethod.Post,
                ServiceToken.ProofKeyInBytes,
                requestBodyContent,
                requestHeaders);

            // Create an HttpClient instance for the request from our factory
            HttpClient httpClient = mHttpClientFactory.CreateClient();

            
            // Post the request and wait for the response
            using (HttpResponseMessage httpResponse = await httpClient.SendAsync(httpRequest))
            {
                string responseBody = await httpResponse.Content.ReadAsStringAsync();
                B2BXToken token = null;

                if (httpResponse.IsSuccessStatusCode)
                {
                    token = JsonConvert.DeserializeObject<B2BXToken>(responseBody);
                    token.ProofKeyInBytes = ServiceToken.ProofKeyInBytes;

                    //  Format the delegated auth token into a header format
                    //  Authorization: "XBL3.0 x=[ush];[JWT with encrypted payload]"
                    token.Token = string.Format("XBL3.0 x=-;{0}",
                                            token.Token);

                }
                else
                {
                    string responseCv = "BADCV";
                    if (httpResponse.Headers.TryGetValues("MS-CV", out var headerValues))
                    {
                        responseCv = headerValues.First();
                    }
                    //  Something happened in this call, let's log the important info about it so we can investigate later
                    mLogger.XstsFailedRequest(  mCv.Value,
                                                httpRequest.Method.ToString(),
                                                httpRequest.RequestUri.AbsoluteUri.ToString(),
                                                await httpRequest.Content.ReadAsStringAsync(),
                                                responseCv,
                                                responseBody);

                }
                return token;
                
            }
        }
    }
}
