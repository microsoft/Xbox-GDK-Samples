//-----------------------------------------------------------------------------
// XstsCertController.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Jose;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using System;
using System.Security.Cryptography.X509Certificates;
using System.Threading.Tasks;

namespace Microsoft.SimpleXboxSecureTokens
{
    //  Section 1 - Stores and loads Certs frequently used for Xsts token handling and requests
    public class XstsCertController : XstsControllerBase
    {
        /// NOTE:   This class requires your service to have an in-memory cache.
        ///         To do this, please add the following to your program.cs or startup.cs:
        ///         
        ///         builder.Services.AddMemoryCache();
        ///         XstsControllerBase2.ServerCache = app.Services.GetService<IMemoryCache>();

        /// NOTE:   This class uses the XstsCore API's and to improve server performance your
        ///         service should use an HttpClientFactory to create HttpClients.  To set
        ///         The XstsCore API's to use your HttpClientFactory use the following override
        ///         in your program.cs or startup.cs:
        ///         
        ///         var httpClientFactory = app.Services.GetService<System.Net.Http.IHttpClientFactory>();
        ///         XstsCore.CreateHttpClientFunc = httpClientFactory.CreateClient;

        public XstsCertController()
        {
        }

        /// <summary>
        /// Retrieves the target certificate matching the provided thumbprint from the 
        /// in-memory cache.  If it is not in the cache it will be retrieved from the 
        /// current user certificate store and added to the cache.
        /// </summary>
        /// <param name="Thumbprint"></param>
        /// <returns></returns>
        public static X509Certificate2 GetCert(string Thumbprint)
        {
            X509Certificate2? cert;
            if (ServerCache != null)
            {
                cert = ServerCache.Get<X509Certificate2>(Thumbprint);

                if (cert == null)
                {
                    //  BP cert is not in the cache, try to re-initialize
                    cert = XstsCore.GetCertFromCurrentUserStore(Thumbprint);
                    CacheCert(cert);
                }
            }
            else
            {
                throw new Exception("ServerCache is not initialized");
            }

            return cert;
        }

        /// <summary>
        /// Places the provided cert into the in-memory cache so that it can be quickly 
        /// looked up and used again in future calls.
        /// </summary>
        /// <param name="cert"></param>
        /// <returns></returns>
        private static X509Certificate2 CacheCert(X509Certificate2 cert)
        {
            //  Now lets put this in the cache
            //  We don't ever want this value removed from the cache
            var cacheExpirationOptions = new MemoryCacheEntryOptions();
            cacheExpirationOptions.SetPriority(CacheItemPriority.NeverRemove);

            if (ServerCache == null)
            {
                throw new Exception("ServerCache is not initialized");
            }

            ServerCache.Set<X509Certificate2>(cert.Thumbprint, cert, cacheExpirationOptions);

            return cert;
        }

        /// <summary>
        /// Gets the Xbox Live certificate used to generate the signature of the inner-token, this cert
        /// is identified with the x5t and x5u values in the decrypted payload token's headers.
        /// </summary>
        /// <param name="decryptedPayload">The inner-token that comes from decrypting the payload of the outer XSTS Token</param>
        /// <returns>x509 cert used to sign the provided token</returns>
        public static async Task<X509Certificate2> GetXblSigningCert(string decryptedPayload)
        {
            var tokenHeaders = Jose.JWT.Headers(decryptedPayload);

            //  Get the x5t (thumbprint) and check if we already have this cert in our cache.
            //  The header gives us info on the cert used to sign the token.
            string x5tEncoded = (string)tokenHeaders["x5t"];
            string x5tThumbprint = BitConverter.ToString(Base64Url.Decode(x5tEncoded)).Replace("-", string.Empty);

            if(ServerCache == null)
            {
                throw new Exception("ServerCache is not initialized");
            }

            X509Certificate2? signatureCert = ServerCache.Get<X509Certificate2>(x5tThumbprint);
            if (signatureCert == null)
            {
                //  This is not in the cache so we need to retrieve it from the XSTS service
                signatureCert = await XstsCore.DownloadXblSigningCert(decryptedPayload);
                CacheCert(signatureCert);
            }

            return signatureCert;
        }
    }
}
