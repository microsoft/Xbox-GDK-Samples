//-----------------------------------------------------------------------------
// XstsCertController.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Jose;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using System;
using System.Linq;
using System.Net.Http;
using System.Security.Cryptography.X509Certificates;

namespace Microsoft.SimpleXboxSecureTokens
{
    public class XstsCertController
    {
        /// <summary>
        /// This class requires your service to have an in-memory cache so that the
        /// certs can be cached and re-used rather than downloaded each time.
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

        public XstsCertController(IMemoryCache serverCache)
        {
            ServerCache = serverCache;
        }

        /// <summary>
        /// Retrieves the target certificate matching the provided x5t from the 
        /// in-memory cache.  If it is not in the cache it will be retrieved from the 
        /// current user certificate store and added to the cache.
        /// </summary>
        /// <param name="x5t">x5t of the cert from the token header retrieve</param>
        /// <returns>Certificate matching the x5t</returns>
        public X509Certificate2 GetCachedCertWithX5t(string x5t)
        {
            var x5tThumbprint = BitConverter.ToString(Base64Url.Decode(x5t)).Replace("-", string.Empty);
            return GetCachedCertWithThumbprint(x5tThumbprint);
        }

        /// <summary>
        /// Retrieves the target certificate matching the provided thumbprint from the 
        /// in-memory cache.  If it is not in the cache it will be retrieved from the 
        /// current user certificate store and added to the cache.
        /// </summary>
        /// <param name="thumbprint">Thumbprint of the cert to lookup and retrieve</param>
        /// <returns>Certificate matching the thumbprint</returns>
        public X509Certificate2 GetCachedCertWithThumbprint(string thumbprint)
        {
            X509Certificate2? cert;
            if (ServerCache != null)
            {
                cert = ServerCache.Get<X509Certificate2>(thumbprint);

                if (cert == null)
                {
                    //  Cert is not in the cache, try to re-initialize
                    cert = GetCertFromUserCertStoreWithThumbprint(thumbprint);
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
        /// <param name="cert">Cert to be added to the in-memory cache</param>
        /// <returns></returns>
        private X509Certificate2 CacheCert(X509Certificate2 cert)
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
        /// Places the provided cert into the in-memory cache as the default Business Partner
        /// Cert.  For use if your server is using the SimpleXboxDelegatedAuth library for
        /// service to service calls to Xbox Live.
        /// </summary>
        /// <param name="cert">Cert to be added to the in-memory cache</param>
        /// <returns></returns>
        public X509Certificate2 SetDefaultBPCert(X509Certificate2 cert)
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
            ServerCache.Set<X509Certificate2>(XstsConstants.DefaultBPCertCacheKey, cert, cacheExpirationOptions);

            return cert;
        }

        /// <summary>
        /// Retrieves the target certificate matching the provided thumbprint from the 
        /// in-memory cache.  If it is not in the cache it will be retrieved from the 
        /// current user certificate store and added to the cache.
        /// </summary>
        /// <param name="thumbprint">Thumbprint of the cert to lookup and retrieve</param>
        /// <returns>Default BP Cert set with CacheDefaultBPCert</returns>
        public X509Certificate2 GetCachedDefaultBPCert()
        {
            X509Certificate2? cert;
            if (ServerCache != null)
            {
                cert = ServerCache.Get<X509Certificate2>(XstsConstants.DefaultBPCertCacheKey);
                if (cert  == null)
                {
                    throw new Exception("Default BP Cert not initialized in the cache");
                }
            }
            else
            {
                throw new Exception("ServerCache is not initialized");
            }

            return cert;
        }

        /// <summary>
        /// Checks the current user cert store for a certificate that matches the provided x5t
        /// </summary>
        /// <param name="x5t">x5t of the cert to retrieve from the cert store</param>
        /// <returns>Cert that matches the x5t</returns>
        public static X509Certificate2 GetCertFromUserCertStoreWithX5t(string x5t)
        {
            var x5tThumbprint = BitConverter.ToString(Base64Url.Decode(x5t)).Replace("-", string.Empty);
            return GetCertFromUserCertStoreWithThumbprint(x5tThumbprint);
        }

        /// <summary>
        /// Checks the current user cert store for a certificate that matches the provided thumbprint
        /// </summary>
        /// <param name="thumbprint">thumbprint of the cert to retrieve from the cert store</param>
        /// <returns>Cert that matches the thumbprint</returns>
        public static X509Certificate2 GetCertFromUserCertStoreWithThumbprint(string thumbprint)
        {
            X509Certificate2 targetCert;

            var certStore = new X509Store(StoreName.My, StoreLocation.CurrentUser);
            certStore.Open(OpenFlags.ReadOnly);
            X509Certificate2Collection certCollection = certStore.Certificates.Find(
                                        X509FindType.FindByThumbprint,
                                        thumbprint,
                                        false);

            certStore.Close();

            // Get the first cert with the thumbprint
            if (certCollection.Count > 0)
            {
                targetCert = certCollection.First();
            }
            else
            {
                //  unable to find the RP cert
                string message = "Unable to find cert in store for thumbprint - " + thumbprint;
                var e = new Exception(message);
                throw e;
            }

            return targetCert;
        }
    }
}
