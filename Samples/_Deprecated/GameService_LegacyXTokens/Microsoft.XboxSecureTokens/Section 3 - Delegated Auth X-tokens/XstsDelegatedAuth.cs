//-----------------------------------------------------------------------------
// XstsDelegatedAuth.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Threading.Tasks;

namespace Microsoft.XboxSecureTokens.XstsDelegatedAuth
{
    //  SECTION 3 - XSTS Delegated Auth main controller for retrieval, caching, and appending to b2b calls
    public class XstsDelegatedAuthRequest
    {
        internal readonly IHttpClientFactory mHttpClientFactory;
        internal XstsServiceTokenController mServiceTokenController;
        internal XblEndpointsController mXblEndpointsController;

        public Uri RequestUri { get; private set; }
        public XstsRequestBody RequestBody { get; set; }
        internal XstsServiceToken mSToken;
        internal XblEndpoint mTargetEndpoint;
        internal SignaturePolicy mTargetSignaturePolicy;

        // Place to store the Config object and use in this controller
        internal readonly IConfiguration mConfig;
        internal Microsoft.CorrelationVector.CorrelationVector mCv;

        internal ILogger mLogger;
        internal IMemoryCache mServerCache;
        internal string mDelegationToken;

        public XstsDelegatedAuthRequest(IHttpClientFactory HttpClientFactory,
                                        IConfiguration config,
                                        IMemoryCache ServerCache,
                                        ILogger Logger,
                                        string CorrelationVectorString,
                                        string DelegationToken)
        {
            this.mServerCache = ServerCache;
            this.mCv = Microsoft.CorrelationVector.CorrelationVector.Extend(CorrelationVectorString);
            this.mLogger = Logger;
            this.mServiceTokenController = new XstsServiceTokenController(mConfig, mServerCache, mLogger, mCv.Increment());
            this.mXblEndpointsController = new XblEndpointsController(mLogger, mCv.Increment());
            this.mHttpClientFactory = HttpClientFactory;
            this.mConfig = config;
            this.mDelegationToken = DelegationToken;

            this.RequestBody = new XstsRequestBody();
            this.RequestBody.TokenType = "JWT";
            this.RequestUri = new Uri("https://xsts.auth.xboxlive.com/xsts/authorize");
        }

        /// <summary>
        /// Provides a Delegated Auth XSTS token for b2b communication with an XBL endpoint
        /// </summary>
        /// <param name="TargetRelyingParty"></param>
        /// <param name="SandboxId"></param>
        /// <param name="DelegationToken"></param>
        /// <param name="ServiceToken"></param>
        /// <returns></returns>
        public async Task<B2BXToken> RequestDelegatedAuthTokenAsync(
            XstsServiceToken ServiceToken,
            string DelegationToken,
            string TargetRelyingParty,
            string SandboxId)
        {
            //  Construct the request body
            this.RequestBody.RelyingParty = TargetRelyingParty;
            this.RequestBody.Properties.DelegationToken = DelegationToken;
            this.RequestBody.Properties.SandboxId = SandboxId;
            this.RequestBody.Properties.ServiceToken = ServiceToken.Token;

            // Serialize our request body to a UTF8 byte array
            string requestBodyString = JsonConvert.SerializeObject(this.RequestBody);
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

            using (var httpContent = new ByteArrayContent(requestBodyContent))
            {
                // Post the request and wait for the response
                using (HttpResponseMessage httpResponse = await httpClient.SendAsync(httpRequest))
                {
                    string responseBody = await httpResponse.Content.ReadAsStringAsync();
                    B2BXToken token = null;

                    if (httpResponse.IsSuccessStatusCode)
                    {    
                        token = JsonConvert.DeserializeObject<B2BXToken>(responseBody);
                        token.ProofKeyInBytes = ServiceToken.ProofKeyInBytes;
                        token.SandboxId = SandboxId;
                        token.RelyingParty = TargetRelyingParty;

                        //  Format the delegated auth token into a header format
                        //  Authorization: "XBL3.0 x=[ush];[JWT with encrypted payload]"
                        token.Token = string.Format("XBL3.0 x={0};{1}",
                                                    token.DisplayClaims.Users[0].UserHash,
                                                    token.Token);
                        
                    }
                    else
                    {
                        string responseCv = "";
                        IEnumerable<string> headerValues;
                        if(httpResponse.Headers.TryGetValues("MS-CV", out headerValues))
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

        /// <summary>
        /// SECTION 2 - Getting and caching an S-Token along with endpoints
        /// Used to get the currently cached Service Token and lookup the target endpoint's
        /// Relying Party and Signature Policy
        /// </summary>
        /// <param name="TargetUri"></param>
        /// <returns></returns>
        internal async Task<byte[]> InitializeSTokenAndEndpoint(Uri TargetUri)
        {
            mSToken = await mServiceTokenController.GetSTokenAsync();
            if (mSToken == null)
            {
                throw new InvalidOperationException("Unable to obtain an S Token");
            }

            mTargetEndpoint = await mXblEndpointsController.GetXblEndpointByFQDNAsync(TargetUri.Host);
            mTargetSignaturePolicy = await mXblEndpointsController.GetSignaturePolicyAsync(mTargetEndpoint);

            return mSToken.ProofKeyInBytes;
        }

        /// <summary>
        /// Creates an HttpRequestMessage with a Delegated Auth XSTS token and signature to
        /// talk to XBL endpoints b2b.  Some endpoints include headers and body into the 
        /// signature policy, so all the call's information must be provided to generate
        /// the right signature.
        /// </summary>
        /// <param name="TargetUri"></param>
        /// <param name="Method"></param>
        /// <param name="RequestContent"></param>
        /// <param name="RequestHeaders"></param>
        /// <param name="Sandbox"></param>
        /// <param name="DelegationToken"></param>
        /// <returns></returns>
        internal async Task<HttpRequestMessage> CreateRequestWithAuthAndSignature(
            string UserId,
            Uri TargetUri,
            HttpMethod Method,
            byte[] RequestContent,
            NameValueCollection RequestHeaders,
            string Sandbox)
        {
            await InitializeSTokenAndEndpoint(TargetUri);

            var cachedToken = await GetCachedAuthTokenAsync(UserId, mTargetEndpoint.RelyingParty, Sandbox);
            
            if(cachedToken == null)
            {
                mLogger.XstsWarning(mCv.Value, "Unable to obtain a cached Delegated Auth X-Token");
                mCv.Increment();
                throw new InvalidOperationException("Unable to obtain a cached Delegated Auth X-Token");
            }

            //  Add the Authorization header with the Delegated Auth XSTS token
            RequestHeaders.Add("Authorization", cachedToken.Token);

            return CreateRequestWithSignatureInternal(
                TargetUri,
                Method,
                cachedToken.ProofKeyInBytes,
                RequestContent,
                RequestHeaders);
        }

        /// <summary>
        /// Used by XstsDelegatedAuth to call the XSTS service to get a Delegated Auth Token where
        /// The request will only have a signature (no auth header)
        /// </summary>
        /// <param name="TargetUri"></param>
        /// <param name="Method"></param>
        /// <param name="RequestContent"></param>
        /// <param name="RequestHeaders"></param>
        /// <returns></returns>
        internal async Task<HttpRequestMessage> CreateRequestWithSignature(
            Uri TargetUri,
            HttpMethod Method,
            byte[] RequestContent,
            NameValueCollection RequestHeaders)
        {
            #region Parameter Validation
            if (TargetUri == null)
            {
                throw new ArgumentNullException("TargetUri");
            }
            if (Method == null)
            {
                throw new ArgumentNullException("Method");
            }
            if (RequestHeaders == null)
            {
                throw new ArgumentNullException("RequestHeaders");
            }
            #endregion

            byte[] proofKeyInBytes = await InitializeSTokenAndEndpoint(TargetUri);

            return CreateRequestWithSignatureInternal(
                TargetUri,
                Method,
                RequestContent,
                proofKeyInBytes,
                RequestHeaders);
        }

        /// <summary>
        /// Creates the HttpRequestMessage and adds the Signature Header to it.  Internal API called by 
        /// CreateRequestWithSignature and CreateRequestWithAuthAndSignature
        /// </summary>
        /// <param name="TargetUri"></param>
        /// <param name="Method"></param>
        /// <param name="RequestContent"></param>
        /// <param name="RequestHeaders"></param>
        /// <returns>HttpRequestMessage</returns>
        internal HttpRequestMessage CreateRequestWithSignatureInternal(
            Uri TargetUri,
            HttpMethod Method,
            byte[] TokensProofKeyInBytes,
            byte[] RequestContent,
            NameValueCollection RequestHeaders)
        {
            #region Parameter Validation
            //  All other parameters should have already been verified by the caller
            if (mSToken == null)
            {
                throw new ArgumentNullException("mSToken");
            }
            if (mTargetEndpoint == null)
            {
                throw new ArgumentNullException("mTargetEndpoint");
            }
            if (mTargetSignaturePolicy == null)
            {
                throw new ArgumentNullException("mTargetSignaturePolicy");
            }

            if (RequestContent == null)
            {
                //  If there is no body, we still need a blank array for the signature 
                //  generation
                RequestContent = new Byte[8];
                Array.Clear(RequestContent, 0, RequestContent.Length);
            }
            #endregion

            var httpRequest = new HttpRequestMessage(Method, TargetUri);

            //  This adds the MS-CV header to the request.  If the server being called
            //  supports correlation vectors it will use this request's cV for its own
            //  logging after expanding the cV with a new .0.  This helps sync up logs
            //  from the Microsoft servers and your own when investigating issues.
            httpRequest.Headers.Add("MS-CV", mCv.Increment());

            //  Calculate the signature with the ProofKey from the S Token which might
            //  require the method, uri, headers, and body of the request depending on  
            //  the Signature Policy.
            try
            {
                httpRequest.Headers.Add("Signature",
                    TokensProofKeyInBytes.GenerateSignature(mTargetSignaturePolicy,
                                                      TargetUri,
                                                      Method,
                                                      RequestContent,
                                                      RequestHeaders));
            }
            catch (Exception e)
            {
                //  Unexpected result - enter into your server logs to track this here    
                mLogger.XstsException(mCv.Value, "Signature Generation Error", e);
            }

            if (Method == HttpMethod.Post)
            {
                httpRequest.Content = new ByteArrayContent(RequestContent);

                //  Look for the Content-Type header that we may have previously set when building the call
                //  we need to move that header to the Content.Headers list or else we will get exceptions
                try
                {
                    string contentTypeValue = RequestHeaders[XstsConstants.ContentTypeHeaderKey];
                    RequestHeaders.Remove(XstsConstants.ContentTypeHeaderKey);
                    httpRequest.Content.Headers.ContentType = new MediaTypeHeaderValue(contentTypeValue);
                }
                catch (Exception e)
                {
                    mLogger.XstsException(mCv.Value, "Content-Type header expected, but not found", e);
                    throw;
                }
            }

            //  Add the rest of the headers from the NameValueCollection and then pass back the
            //  now configured request
            foreach (string key in RequestHeaders.AllKeys)
            {
                httpRequest.Headers.Add(key, RequestHeaders[key]);
            }

            return httpRequest;
        }

        /// <summary>
        /// Provides a cached Delegated Auth XSTS token for b2b communication with an XBL endpoint
        /// </summary>
        /// <param name="UserId"></param>
        /// <param name="TargetRelyingParty"></param>
        /// <param name="SandboxId"></param>
        /// <returns></returns>
        public async Task<XstsCachedToken> GetCachedAuthTokenAsync(
            string UserId,
            string TargetRelyingParty,
            string SandboxId)
        {
            DateTime currentUTC = DateTime.Now.ToUniversalTime();

            string cacheKey = string.Format(XstsConstants.CacheKeyFormat, 
                                            UserId,
                                            TargetRelyingParty,
                                            SandboxId);

            XstsCachedToken cachedToken = mServerCache.Get<XstsCachedToken>(cacheKey);

            if(cachedToken == null)
            {
                //  Token isn't cached so lets go get one and then cache it
                //  We don't have a valid delegated auth XSTS token cached,
                //  so we need to get a new one
                B2BXToken xToken = await this.RequestDelegatedAuthTokenAsync(mSToken,
                                                                             mDelegationToken,
                                                                             mTargetEndpoint.RelyingParty,
                                                                             SandboxId);

                cachedToken = CacheDelegatedAuthTokenAsync(xToken,
                                                           UserId);
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
        public XstsCachedToken CacheDelegatedAuthTokenAsync(
            B2BXToken Token,
            string UserId)
        {
            if (Token == null)
            {
                throw new ArgumentNullException("Token");
            }
            if (UserId == null)
            {
                throw new ArgumentNullException("UserId");
            }
            if (Token.RelyingParty == null)
            {
                throw new ArgumentNullException("RelyingParty");
            }
            if (Token.SandboxId == null)
            {
                throw new ArgumentNullException("SandboxId");
            }

            XstsCachedToken tokenToCache = new XstsCachedToken()
            {
                Token = Token.Token,
                ProofKeyInBytes = Token.ProofKeyInBytes
            };

            //  set the expiration time to the lifetime of the token from right now.
            var cacheExpirationOptions = new MemoryCacheEntryOptions();
            cacheExpirationOptions.SetPriority(CacheItemPriority.Normal);
            cacheExpirationOptions.SetAbsoluteExpiration(DateTimeOffset.UtcNow.AddTicks(Token.NotAfter.Ticks - Token.IssueInstant.Ticks));

            //  CacheService tokens based on UserId:RelyingParty:Sandbox
            string cacheKey = string.Format(XstsConstants.CacheKeyFormat,
                                            UserId,
                                            Token.RelyingParty,
                                            Token.SandboxId);
            mServerCache.Set<XstsCachedToken>(cacheKey, tokenToCache, cacheExpirationOptions);
            
            return tokenToCache;
        }
    }
}
