//-----------------------------------------------------------------------------
// XstsServiceTokenRequest.cs
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
using System.Security.Cryptography.X509Certificates;
using System.Threading.Tasks;

namespace Microsoft.XboxSecureTokens.XstsDelegatedAuth
{
    /// <summary>
    /// SECTION 2 - Creates and preforms a request to the XSAS service to obtain a Service Token
    /// </summary>
    public class XstsServiceTokenRequest
    {
        public byte[] ProofKeyInBytes { get; set; }
        public Uri RequestUri { get; private set; }
        public XstsServiceTokenRequestBody RequestBody { get; set; }

        public string Host { get; set; }

        private IConfiguration mConfig;
        private Microsoft.CorrelationVector.CorrelationVector mCv;
        private ILogger mLogger;
        private IMemoryCache mServerCache;

        public XstsServiceTokenRequest(IConfiguration Config, IMemoryCache ServerCache, ILogger Logger, string CorrelationVectorString)
        {
            mConfig = Config;
            mServerCache = ServerCache;
            mCv = Microsoft.CorrelationVector.CorrelationVector.Extend(CorrelationVectorString);
            mLogger = Logger;

            var ecdsa = ProofKeyUtility.Create();

            this.RequestBody = new XstsServiceTokenRequestBody();
            this.RequestBody.TokenType = "JWT";
            this.RequestBody.RelyingParty = "http://auth.xboxlive.com";
            this.Host = "service.auth.xboxlive.com";
            this.RequestUri = new Uri("https://service.auth.xboxlive.com/service/authenticate");

            var eParameters = ecdsa.ExportParameters(true);
            this.ProofKeyInBytes = eParameters.ExportToByteArray();

            // These are the JSON values that will be sent in the body of the request
            Ecc256ProofKey eccProofKey = new Ecc256ProofKey(new EccJsonWebKey(ecdsa));
            this.RequestBody.Properties = new XstsServiceTokenRequestProperties
            {
                ProofKey = eccProofKey
            };
        }

        /// <summary>
        /// Formats the request to XSAS and makes the HTTP call to get back a Service Token
        /// </summary>
        /// <param name="BusinessPartnerCertSubjectName"></param>
        /// <returns></returns>
        public async Task<XstsServiceToken> RequestTokenAsync()
        {
            //  Find the business partner certificate 
            var certController = new XstsCertController(mConfig, mServerCache, mLogger, mCv.Value);
            X509Certificate2 bpClientCert = certController.GetBusinessPartnerCert();
            mLogger.StartupInfo(mCv.Increment(), String.Format("Using BP Cert: {0}", bpClientCert.Subject));

            //  If you are debugging and don't have an Azure Key vault yet, you could use the following to load the BP 
            //  cert if it is installed locally.  But it is recommended to use a Key Vault for maintainability and
            //  Linux compatibility
            //  X509Certificate2 bpClientCert = XstsUtilities.GetCertificateFromThumbprint("[thumbprint goes here]", true);

            //  Create an HttpClientHandler and use the business partner certificate for SSL
            using (var handler = new HttpClientHandler())
            {
                handler.ClientCertificateOptions = ClientCertificateOption.Manual;
                handler.ClientCertificates.Add(bpClientCert);

                // Serialize our request body to a UTF8 byte array
                string requestBodyString = JsonConvert.SerializeObject(this.RequestBody);
                byte[] requestBodyContent = System.Text.Encoding.UTF8.GetBytes(requestBodyString);
                using (var httpContent = new ByteArrayContent(requestBodyContent))
                {
                    httpContent.Headers.ContentType = new MediaTypeHeaderValue("application/json");
                    var endpointController = new XblEndpointsController(mLogger, mCv.Increment());

                    XblEndpoint targetEndpoint = await endpointController.GetXblEndpointByFQDNAsync(Host);
                    SignaturePolicy targetSignaturePolicy = await endpointController.GetSignaturePolicyAsync(targetEndpoint);

                    httpContent.Headers.Add("MS-CV", mCv.Increment());
                    mCv.Increment();

                    // Sign our request, note that short circuiting the headers like this is only OK because the above headers 
                    // are known to be excluded from signing.
                    httpContent.Headers.Add("Signature", this.ProofKeyInBytes.GenerateSignature(targetSignaturePolicy,
                                                                                                this.RequestUri,
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
                        using (HttpResponseMessage response = await httpClient.PostAsync(this.RequestUri, httpContent))
                        {
                            string responseBody = await response.Content.ReadAsStringAsync();

                            XstsServiceToken newSToken = null;  

                            if (response.IsSuccessStatusCode)
                            {
                                newSToken = JsonConvert.DeserializeObject<XstsServiceToken>(responseBody);

                                //  Save the proof key we used to generate this SToken as we will need to use this same
                                //  proof key for signature generation when we use a Delegated Auth token that was created
                                //  from this same SToken
                                newSToken.ProofKeyInBytes = this.ProofKeyInBytes;
                            }
                            else
                            {
                                string responseCv = "";
                                IEnumerable<string> headerValues;
                                if (response.Headers.TryGetValues("MS-CV", out headerValues))
                                {
                                    responseCv = headerValues.First();
                                }
                                //  Something happened in this call, let's log the important info about it so we can investigate later
                                mLogger.XstsFailedRequest(mCv.Value,
                                                            "POST",
                                                            this.RequestUri.AbsoluteUri,
                                                            await httpContent.ReadAsStringAsync(),
                                                            responseCv,
                                                            responseBody);

                            }
                            return newSToken;

                        }
                    }
                }
            }
        }
    }
}
