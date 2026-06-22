//-----------------------------------------------------------------------------
// XstsCertController.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using System;
using System.Security.Cryptography.X509Certificates;

namespace Microsoft.XboxSecureTokens
{
    //  Section 1 - Stores and loads Certs and Secrets needed for token handling and requests
    public class XstsCertController : Controller
    {
        protected readonly IConfiguration mConfig;
        private readonly IMemoryCache mServerCache;
        private ILogger mLogger;
        private CorrelationVector.CorrelationVector mCv;

        public XstsCertController(IConfiguration Config, IMemoryCache ServerCache, ILogger Logger, string CorrelationVectorString)
        {
            mConfig = Config;           // so that we can see which certs we should be loading
            mServerCache = ServerCache;
            mCv = Microsoft.CorrelationVector.CorrelationVector.Extend(CorrelationVectorString);
            mLogger = Logger;
        }

        //  Section 2 - Initialization of the Business Partner Certificate
        public string InitializeBusinessPartnerCert(string Thumprint = "")
        {
            X509Certificate2 bpCert = null;
            if (string.IsNullOrEmpty(Thumprint))
            {
                Thumprint = mConfig.GetValue(XstsConstants.BPCertThumprintKey, "");
            }

            if (string.IsNullOrEmpty(Thumprint))
            {
                //  Log the warning, during Section 1 we do not yet have a BP cert setup
                mLogger.StartupWarning(mCv.Increment(), $"Unable to get Business Partner Cert thumprint from App Settings value {XstsConstants.BPCertThumprintKey}", null);
            }
            else
            {
                bpCert = GetAndCacheCertFromStore(Thumprint);

                if (bpCert != null)
                {
                    //  We don't ever want this value removed from the cache
                    var cacheExpirationOptions = new MemoryCacheEntryOptions();
                    cacheExpirationOptions.SetPriority(CacheItemPriority.NeverRemove);
                    mServerCache.Set<X509Certificate2>("BusinessPartnerCert", bpCert, cacheExpirationOptions);
                }
                else
                {
                    //  The thumprint is defined in the settings, but we can't seem to access it.  Mark that we've already tried
                    //  so that we don't waste time trying to search the cert store again.
                    mServerCache.Set<bool>("CheckedThumprint" + Thumprint, true);
                    string message = "Business Partner cert thumprint is not accessible in the server's cert store: " + Thumprint;
                    mLogger.XstsWarning(mCv.Value, message);
                }
            }

            return Thumprint;
        }

        public X509Certificate2 GetBusinessPartnerCert()
        {
            X509Certificate2 bpCert = mServerCache.Get<X509Certificate2>("BusinessPartnerCert");

            if (bpCert == null)
            {
                //  BP cert is not in the cache, try to re-initialize
                string thumbprint = InitializeBusinessPartnerCert();

                if (string.IsNullOrEmpty(thumbprint))
                {
                    //  unable to find the BP cert
                    string message = "Trying to access a BP cert but we couldn't load one";
                    var ex = new Exception(message);
                    mLogger.XstsWarning(mCv.Increment(), message);
                    throw ex;
                }

                //  Try the call again
                bpCert = mServerCache.Get<X509Certificate2>("BusinessPartnerCert");
            }

            return bpCert;
        }

        public X509Certificate2 InitializeAsymmetricRelyingParty(string Thumbprint = "")
        {
            X509Certificate2 rpCert = null;
            if (string.IsNullOrEmpty(Thumbprint))
            {
                Thumbprint = mConfig.GetValue(XstsConstants.RPCertThumprintKey, "");
            }
            
            if (string.IsNullOrEmpty(Thumbprint))
            {
                mLogger.StartupWarning(mCv.Increment(), $"Unable to get Relying Party cert thumprint from App Settings value {XstsConstants.RPCertThumprintKey}", null);
            }
            else
            {
                rpCert = GetAndCacheCertFromStore(Thumbprint);
            }
            return rpCert;
        }

        public X509Certificate2 GetCert(string Thumbprint)
        {
            X509Certificate2 cert = mServerCache.Get<X509Certificate2>(Thumbprint);

            if (cert == null)
            {
                //  BP cert is not in the cache, try to re-initialize
                cert = GetAndCacheCertFromStore(Thumbprint);

                if (cert == null)
                {
                    //  unable to find the BP cert
                    string message = "Unable to access or cache cert: "+Thumbprint;
                    var ex = new Exception(message);
                    mLogger.XstsWarning(mCv.Increment(), message);
                    throw ex;
                }
            }

            return cert;
        }

        private X509Certificate2 GetAndCacheCertFromStore(string Thumbprint)
        {
            X509Certificate2 targetCert = null;

            //  To save time, check if we have already tried to get this cert and were unable to
            if (!mServerCache.Get<bool>("CheckedThumprint" + Thumbprint))
            {
                X509Store certStore = new X509Store(StoreName.My, StoreLocation.CurrentUser);
                certStore.Open(OpenFlags.ReadOnly);
                X509Certificate2Collection certCollection = certStore.Certificates.Find(
                                            X509FindType.FindByThumbprint,
                                            Thumbprint,
                                            false);

                // Get the first cert with the thumbprint
                if (certCollection.Count > 0)
                {
                    targetCert = certCollection[0];
                    //  We don't ever want this value removed from the cache
                    var cacheExpirationOptions = new MemoryCacheEntryOptions();
                    cacheExpirationOptions.SetPriority(CacheItemPriority.NeverRemove);
                    mServerCache.Set<X509Certificate2>(Thumbprint, targetCert, cacheExpirationOptions);
                    string message = "Caching Cert with Thumbprint: " + Thumbprint;
                    mLogger.XstsInformation(mCv.Increment(), message);
                }
                else
                {
                    //  Mark this as cached so we don't go seaching the store for it again
                    mServerCache.Set<bool>("CheckedThumbprint" + Thumbprint, true);

                    //  unable to find the RP cert
                    string message = "Unable to find cert in store for thumbprint - " + Thumbprint;
                    var e = new Exception(message);
                    mLogger.XstsWarning(mCv.Increment(), message);
                    throw e;
                }

                certStore.Close();
            }

            return targetCert;
        }
    }
}
