//-----------------------------------------------------------------------------
// XstsSigningCertController.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Jose;
using Microsoft.AspNetCore.Mvc.Razor;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Numerics;
using System.Runtime.CompilerServices;
using System.Security.Cryptography.X509Certificates;
using System.Threading.Tasks;

namespace Microsoft.XboxSecureTokens
{
    //  SECTION 1 - Asymmetric signature validation cert retrieval and caching
    public class XstsSigningCertResponse
    {
        [JsonExtensionData]
        public IDictionary<string, JToken> signingCerts;
    }

    public class LicenseTokenSignatureCertResponse
    {
        public string certificate { get; set; }
    }


    public class XstsSigningCertController : XstsControllerBase
    {
        public XstsSigningCertController(IConfiguration Config,
                                         IHttpClientFactory HttpClientFactory,
                                         IMemoryCache ServerCache,
                                         ILogger Logger,
                                         string CorrelationVectorString) 
            : base(Config, HttpClientFactory, ServerCache, Logger)
        {
        }


        public async Task<string[]> GetAndCacheCurrentSigningCerts()
        {
            var certsFound = new List<string>();

            string responseBody;
            using (var client = mHttpClientFactory.CreateClient())
            {
                using (HttpResponseMessage response = await client.GetAsync(XstsConstants.XblCurrentSigningCertsURL))
                {
                    responseBody = await response.Content.ReadAsStringAsync();
                }

                //  Parse the format of ["Cert1Data", "Cert2Data", etc..]
                char[] charsToTrim = { '[', ']' };
                responseBody = responseBody.Trim(charsToTrim);
                responseBody = responseBody.Replace("\"", "");
                var certsData = responseBody.Split(',');

                foreach(string certData in certsData)
                {
                    X509Certificate2 signingCert = new X509Certificate2(Convert.FromBase64String(certData));
                    var cacheExpirationOptions = new MemoryCacheEntryOptions();
                    cacheExpirationOptions.SetPriority(CacheItemPriority.NeverRemove);
                    mServerCache.Set<X509Certificate2>(signingCert.Thumbprint, signingCert, cacheExpirationOptions);
                    certsFound.Add(signingCert.Thumbprint);
                }

            }
            
            return certsFound.ToArray();
        }


        /// <summary>
        /// Gets an x509 certificate that was used to sign the token identified by the token header
        /// passed in from Jose.JWT.Headers( token string )
        /// </summary>
        /// <param name="TokenHeaders">Obtained from Jose.JWT.Headers</param>
        /// <returns>x509 cert used to sign the token</returns>
        public async Task<X509Certificate2> GetSigningCert(IDictionary<string, object> TokenHeaders)
        {
            //  Get the x5t (thumbprint) and check if we already have this cert in our cache.
            //  The header gives us info on the cert used to sign the token.
            string x5tEncoded = (string)TokenHeaders["x5t"];
            string x5tThumbprint = BitConverter.ToString(Base64Url.Decode(x5tEncoded)).Replace("-", string.Empty);
            
            X509Certificate2 signatureCert = mServerCache.Get<X509Certificate2>(x5tThumbprint);
            if (signatureCert == null)
            {
                //  This is not in the cache so we need to retrieve it from the XSTS service
                signatureCert = await GetAndCacheSigningCert((string)TokenHeaders["x5u"], (string)TokenHeaders["x5t"]);
            }
            
            return signatureCert;
        }

        private async Task<X509Certificate2> GetAndCacheSigningCert(string x5u, string x5t)
        {
            string certData = "";

            //  Verify that this cert is being downloaded and came from the trusted
            //  host.
            Uri x5uUri = new Uri(x5u);
            if(x5uUri.Host.Equals("xsts.auth.xboxlive.com"))
            {
                //  This is an XSTS signing cert
                certData = await DownloadXSTSSigningCert(x5u, x5t);
            }
            else if(x5uUri.Host.Equals("licensing.mp.microsoft.com"))
            {
                //  This is a License Token signing cert
                certData = await DownloadLicenseTokenSigningCert(x5u, x5t);
            }
            else
            {
                throw new ArgumentException("Invalid x5u host that is not trusted: {0}", x5u);
            }

            X509Certificate2 signingCert = new X509Certificate2(Convert.FromBase64String(certData));

            //  Now lets put this in the cache
            //  We don't ever want this value removed from the cache
            string x5tThumbprint = BitConverter.ToString(Base64Url.Decode(x5t)).Replace("-", string.Empty);
            var cacheExpirationOptions = new MemoryCacheEntryOptions();
            cacheExpirationOptions.SetPriority(CacheItemPriority.NeverRemove);
            mServerCache.Set<X509Certificate2>(x5tThumbprint, signingCert, cacheExpirationOptions);

            return signingCert;
        }

        private async Task<string> DownloadXSTSSigningCert(string x5u, string x5t)
        {
            string responseBody;

            using (var client = mHttpClientFactory.CreateClient())
            {
                using (HttpResponseMessage response = await client.GetAsync(x5u))
                {
                    responseBody = await response.Content.ReadAsStringAsync();
                }
            }

            var deserializedResult = JsonConvert.DeserializeObject<XstsSigningCertResponse>(responseBody);

            string certData = "";
            try
            {
                certData = deserializedResult.signingCerts[x5t].ToString();
            }
            catch (Exception e)
            {
                //  This means that the x5t does not match the value that we got back
                mLogger.XstsException(mCv.Value, string.Format("Cert downloaded from x5u ({0}) does not match the x5t requested ({1})", x5u, x5t), e);
                throw;
            }

            //  Now we need to trim off the begin and end tags
            certData = certData.Replace("-----BEGIN CERTIFICATE-----\n", "");
            certData = certData.Replace("\n-----END CERTIFICATE-----\n", "");

            return certData;

        }

        private async Task<string> DownloadLicenseTokenSigningCert(string x5u, string x5t)
        {

            string responseBody;

            using (var client = mHttpClientFactory.CreateClient())
            {
                using (HttpResponseMessage response = await client.GetAsync(x5u))
                {
                    responseBody = await response.Content.ReadAsStringAsync();
                }
            }

            var deserializedResult = JsonConvert.DeserializeObject<LicenseTokenSignatureCertResponse>(responseBody);

            return deserializedResult.certificate;
        }
    }
}
