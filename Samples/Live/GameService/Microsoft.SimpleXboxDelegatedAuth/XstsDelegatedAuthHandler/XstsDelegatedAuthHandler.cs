//-----------------------------------------------------------------------------
// XstsDelegatedAuthHandler.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Microsoft.Extensions.Caching.Memory;
using Microsoft.SimpleXboxSecureTokens;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading.Tasks;

namespace Microsoft.SimpleXboxDelegatedAuth
{
    public class XstsDelegatedAuthHandler
    {
        /// <summary>
        /// This class requires your service to have an in-memory cache so that the
        /// delegated Xsts Tokens can be cached and re-used rather than created with
        /// each incoming request.
        /// </summary>
        public IMemoryCache ServerCache;

        /// <summary>
        /// Can be overridden with an HttpClientFactory.CreateClient if used by your service.
        /// This will greatly help with server performance.  To do this add the following to your
        /// Setup.cs or Program.cs
        ///
        /// var httpClientFactory = app.Services.GetService<System.Net.Http.IHttpClientFactory>();
        /// XstsSigningKeyController.CreateHttpClientFunc = httpClientFactory.CreateClient;
        ///
        /// </summary>
        public static Func<HttpClient> CreateHttpClientFunc = () => new HttpClient();
        public static string BPThumbprint = "";

        /// <summary>
        /// Default constructor
        /// </summary>
        /// <param name="serverCache"></param>
        public XstsDelegatedAuthHandler(IMemoryCache serverCache)
        {
            ServerCache = serverCache;
        }

        /// <summary>
        /// Retrieve the cached XSTS Service Token, if one does not exist, request a new one and cache it.
        /// </summary>
        /// <returns></returns>
        public async Task<XstsServiceToken> GetCachedServiceTokenAsync()
        {
            var serviceToken = await Task.FromResult(ServerCache.Get<XstsServiceToken>("ServiceToken"));

            if (serviceToken == null)
            {
                //  Add the new token so other calls can get it
                //  We will set the cache expiration for 12 days, the token should only be valid for
                //  14 so it will get removed from the cache before it does and a process will kick
                //  off to get a new one.
                serviceToken = await RequestServiceTokenAsync();
                ServerCache.Set("ServiceToken", serviceToken, TimeSpan.FromDays(12));
            }

            return serviceToken;
        }

        /// <summary>
        /// Requests a Service Token from XSAS using the default Business Partner Certificate
        /// in the cache.
        /// </summary>
        /// <returns></returns>
        public async Task<XstsServiceToken> RequestServiceTokenAsync()
        {
            //  Find the business partner certificate 
            var bpCertManager = new XstsCertController(ServerCache);
            var businessPartnerCert = bpCertManager.GetCachedDefaultBPCert();

            return await RequestServiceTokenAsync(businessPartnerCert);
        }

        /// <summary>
        /// Requests a Service Token from XSAS using the provided thumbprint of the Business Partner Certificate
        /// in the cache.
        /// </summary>
        /// <param name="BusinessPartnerThumbprint">Thumbprint of the BP cert to look for and use in the cache</param>
        /// <returns></returns>
        public async Task<XstsServiceToken> RequestServiceTokenAsync(string BusinessPartnerThumbprint)
        {
            //  Find the business partner certificate 
            var bpCertManager = new XstsCertController(ServerCache);
            var businessPartnerCert = bpCertManager.GetCachedCertWithThumbprint(BusinessPartnerThumbprint);

            return await RequestServiceTokenAsync(businessPartnerCert);
        }

        /// <summary>
        /// Requests a Service Token from XSAS using the provided Business Partner Certificate
        /// </summary>
        /// <param name="BusinessPartnerCert"></param>
        /// <returns></returns>
        public async Task<XstsServiceToken> RequestServiceTokenAsync(X509Certificate2 BusinessPartnerCert)
        {
            //  Create an HttpClientHandler and use the business partner certificate for SSL
            using (var handler = new HttpClientHandler())
            {
                handler.ClientCertificateOptions = ClientCertificateOption.Manual;
                handler.ClientCertificates.Add(BusinessPartnerCert);

                // Serialize our request body to a UTF8 byte array
                var stokenRequest = new XstsServiceTokenRequest();
                string requestBodyString = JsonConvert.SerializeObject(stokenRequest.RequestBody);
                byte[] requestBodyContent = System.Text.Encoding.UTF8.GetBytes(requestBodyString);
                using (var httpContent = new ByteArrayContent(requestBodyContent))
                {
                    httpContent.Headers.ContentType = new MediaTypeHeaderValue("application/json");
                    var endpointController = new XblEndpointsController();

                    XblEndpoint targetEndpoint = await endpointController.GetXblEndpointByFQDNAsync(stokenRequest.Host);
                    SignaturePolicy targetSignaturePolicy = await endpointController.GetSignaturePolicyAsync(targetEndpoint);

                    // Sign our request, note that short circuiting the headers like this is only OK because the above headers 
                    // are known to be excluded from signing.
                    httpContent.Headers.Add("Signature",
                                             stokenRequest.ProofKeyInBytes.GenerateSignature(targetSignaturePolicy,
                                                                                             stokenRequest.RequestUri,
                                                                                             HttpMethod.Post,
                                                                                             requestBodyContent,
                                                                                             new NameValueCollection()));

                    //  We don't use the IHttpClientFactory as we do everywhere else in the server
                    //  for the following reasons.  
                    //  1 - We need to override the SSL certificate with the BP cert for this call only
                    //  2 - We will only make this call on server startup or once every 2 weeks when
                    //      a new SToken would be needed and never per-request from clients.
                    using (var httpClient = new HttpClient(handler))
                    {
                        // Post the request and wait for the response
                        using (HttpResponseMessage response = await httpClient.PostAsync(stokenRequest.RequestUri,
                                                                                          httpContent))
                        {
                            XstsServiceToken newSToken = null;
                            string responseBody = await response.Content.ReadAsStringAsync();

                            if (response.IsSuccessStatusCode)
                            {
                                newSToken = JsonConvert.DeserializeObject<XstsServiceToken>(responseBody);

                                //  Save the proof key we used to generate this SToken as we will need to use this same
                                //  proof key for signature generation when we use a Delegated Auth token that was created
                                //  from this same SToken
                                newSToken.ProofKeyInBytes = stokenRequest.ProofKeyInBytes;

                                //  Add the Business Partner ID that is part of the Subject on the cert
                                int bpIdStartPos = BusinessPartnerCert.Subject.IndexOf("O=") + 2;
                                var businessPartnerId = BusinessPartnerCert.Subject.Substring(bpIdStartPos, 36);
                                newSToken.BusinessPartner = businessPartnerId;
                            }
                            else
                            {
                                //  The call failed, in order to help look up on the Microsoft Side what happened, get
                                //  the ms-cv header value and pass that back to the caller.
                                string responseCv = "";
                                IEnumerable<string> headerValues;
                                if (response.Headers.TryGetValues("MS-CV", out headerValues))
                                {
                                    responseCv = headerValues.First();
                                }

                                throw new Exception("Failed to get a Service Token from XSAS.  " +
                                                    $"Response Code: {response.StatusCode}, " +
                                                    $"Response MS-CV: {responseCv}, " +
                                                    $"Response Body: {responseBody}");
                            }
                                
                            return newSToken;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Get a Delegated Auth X-token for the target URI and Sandbox ID using the default BP cert's cached Service Token
        /// </summary>
        /// <param name="TargetUri"></param>
        /// <param name="SandboxId"></param>
        /// <returns></returns>
        /// <exception cref="Exception"></exception>
        public async Task<XstsAuthToken> GetCachedServerAuthXToken(Uri TargetUri, string SandboxId)
        {
            var endpointController = new XblEndpointsController();
            var targetEndpoint = await endpointController.GetXblEndpointByFQDNAsync(TargetUri.Host);

            if (targetEndpoint == null)
            {
                throw new Exception("TargetUri is not linked to a Relying Party.  No token required");
            }

            //  Create the key to search for an existing Server Auth X-token for this endpoint and sandbox
            var cacheKey = string.Format("{0}_{1}_{2}", XstsTokenType.ServiceAuth, targetEndpoint.RelyingParty, SandboxId);
            var xstsToken = await Task.FromResult(ServerCache.Get<XstsAuthToken>(cacheKey));

            if (xstsToken == null)
            {
                //  We don't have a token for this endpoint and sandbox so get a new one and cache it
                var serviceToken = await GetCachedServiceTokenAsync();

                xstsToken = await CreateXstsAuthToken(TargetUri, SandboxId, serviceToken, XstsTokenType.ServiceAuth);

                //  Cache the token and set the expiration for 5 minutes before its NotAfter time
                ServerCache.Set(cacheKey, xstsToken, xstsToken.NotAfter.Subtract(TimeSpan.FromMinutes(5)));
            }

            return xstsToken;
        }

        /// <summary>
        /// Creates the value for the signature header of an HTTPS request with an X-token using the provided ServiceToken and SignaturePolicy.
        /// </summary>
        /// <param name="HttpRequest"></param>
        /// <param name="ServiceToken"></param>
        /// <param name="Policy"></param>
        /// <returns></returns>
        /// <exception cref="Exception"></exception>
        public string CreateRequestSignature(HttpRequestMessage HttpRequest,
                                             XstsServiceToken ServiceToken,
                                             SignaturePolicy Policy)
        {
            string signature;

            //  Calculate the signature with the ProofKey from the S Token which might
            //  require the method, uri, headers, and body of the request depending on  
            //  the Signature Policy.
            try
            {
                // Generate the signature header
                signature = ServiceToken.ProofKeyInBytes.GenerateSignature(Policy, HttpRequest);
            }
            catch (Exception e)
            {
                //  Unexpected result - enter into your server logs to track this here    
                throw new Exception("Signature Generation Error", e);
            }

            return signature;
        }

        /// <summary>
        /// Creates a delegated or service XSTS authentication token for the specified target endpoint and sandbox using
        /// the provided service token and token type.
        /// </summary>
        /// <remarks>The returned XSTS token is formatted for use in Xbox Live authentication headers. For
        /// delegated authentication, the token includes the user hash and the provided user ID. For service
        /// authentication, a default user hash is used. The method retrieves endpoint and signature policy information
        /// dynamically based on the target URI.</remarks>
        /// <param name="TargetUri">The URI of the target Xbox Live endpoint for which the XSTS token is requested. The host portion is used to
        /// determine the relying party.</param>
        /// <param name="SandboxId">The identifier of the Xbox Live sandbox environment. Specifies the environment context for the token.</param>
        /// <param name="ServiceToken">The service token used to authenticate the request and generate the XSTS token. Must not be null.</param>
        /// <param name="TokenType">The type of XSTS token to create. Determines whether a delegated or service token is generated.</param>
        /// <param name="DelegationToken">An optional delegation token to include in the request. Used for delegated authentication scenarios. If not
        /// required, leave as an empty string.</param>
        /// <param name="UserId">An optional user identifier to associate with the token. For delegated tokens, this should be the
        /// service-specific user ID. If not required, leave as an empty string.</param>
        /// <returns>A task that represents the asynchronous operation. The task result contains an XstsAuthToken instance with
        /// the generated token and associated metadata.</returns>
        /// <exception cref="Exception">Thrown if the target URI is not linked to a relying party, if the signature policy for the endpoint cannot
        /// be retrieved, if the service token is null, or if the token request fails.</exception>
        public async Task<XstsAuthToken> CreateXstsAuthToken( Uri TargetUri,
                                                              string SandboxId,
                                                              XstsServiceToken ServiceToken,
                                                              XstsTokenType TokenType,
                                                              string DelegationToken = "",
                                                              string UserId = "")
        {
            //  Get the Relying Party for the target URI's endpoint
            var endpointController = new XblEndpointsController();
            var targetEndpoint = await endpointController.GetXblEndpointByFQDNAsync(TargetUri.Host);
            var targetSignaturePolicy = await endpointController.GetSignaturePolicyAsync(targetEndpoint);

            if (targetEndpoint == null)
            {
                throw new Exception("TargetUri is not linked to a Relying Party.  No token required");
            }
            else if (targetSignaturePolicy == null)
            {
                throw new Exception("Unable to get the Signature Policy for the target endpoint");
            }
            else if (ServiceToken == null)
            {
                throw new Exception("Service Token is required to get a Delegated Auth X-token");
            }

            var requestBody = new XstsAuthTokenRequest()
            {
                //  Construct the request body
                RelyingParty = targetEndpoint.RelyingParty,
                Properties = new XstsAuthTokenRequestProperties()
                {
                    SandboxId = SandboxId,
                    ServiceToken = ServiceToken.Token,
                    DelegationToken = DelegationToken
                },
                TokenType = "JWT"
            };

            var httpRequest = new HttpRequestMessage(HttpMethod.Post, "https://xsts.auth.xboxlive.com/xsts/authorize")
            {
                Content = new StringContent(JsonConvert.SerializeObject(requestBody), Encoding.UTF8, "application/json")
            };    

            httpRequest.Headers.Add("x-xbl-contract-version", " 1");                      
            httpRequest.Headers.Add("Signature", ServiceToken.ProofKeyInBytes.GenerateSignature(targetSignaturePolicy, httpRequest));
            httpRequest.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));

            // Create an HttpClient instance for the request from our factory
            HttpClient httpClient = CreateHttpClientFunc();

            // Post the request and wait for the response
            using (HttpResponseMessage httpResponse = await httpClient.SendAsync(httpRequest))
            {
                string responseBody = await httpResponse.Content.ReadAsStringAsync();
                XstsAuthToken token = null;

                if (httpResponse.IsSuccessStatusCode)
                {
                    token = JsonConvert.DeserializeObject<XstsAuthToken>(responseBody);
                    token.SignaturePolicy = targetSignaturePolicy;
                    token.ProofKeyInBytes = ServiceToken.ProofKeyInBytes;
                    token.SandboxId = SandboxId;
                    token.RelyingParty = requestBody.RelyingParty;
                    token.TokenType = TokenType;

                    // Format the delegated auth token into a header format
                    
                    if (token.TokenType == XstsTokenType.DelegatedAuth)
                    {
                        //  Authorization: "XBL3.0 x=[ush];[JWT with encrypted payload]"
                        //  Since this is a delegated auth token, we need to include the user hash
                        //  which we can get from the first user in the DisplayClaims as there should
                        //  only be one user for delegated auth tokens.
                        token.Token = string.Format("XBL3.0 x={0};{1}",
                                                    token.DisplayClaims.Users.FirstOrDefault().UserHash,
                                                    token.Token);
                        
                        //  Id passed in by the caller to add to the token, should be an
                        //  Id that the service uses to identify the user.
                        token.UserID = UserId;
                    }
                    else
                    {
                        //  Service Auth X-tokens have the default user hash of "-" meaning 'no user'
                        token.Token = string.Format("XBL3.0 x=-;{0}", token.Token);
                        token.UserID = ServiceToken.BusinessPartner;
                    }
                }
                else
                {
                    //  The call failed, in order to help look up on the Microsoft Side what happened, get
                    //  the ms-cv header value and pass that back to the caller.
                    string responseCv = "";
                    IEnumerable<string> headerValues;
                    if (httpResponse.Headers.TryGetValues("MS-CV", out headerValues))
                    {
                        responseCv = headerValues.First();
                    }

                    throw new Exception($"Failed to get {TokenType} X-token." +
                                        $"Response Code: {httpResponse.StatusCode}, " +
                                        $"Response MS-CV: {responseCv}, " +
                                        $"Response Body: {responseBody}");
                }
                return token;
            }
        }
    }
}
