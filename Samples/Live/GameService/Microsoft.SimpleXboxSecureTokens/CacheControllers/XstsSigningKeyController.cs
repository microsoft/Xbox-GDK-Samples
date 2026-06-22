//-----------------------------------------------------------------------------
// XstsSigningKeyController.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

#pragma warning disable IDE0063 // Use simple 'using' statement

using Jose;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Security.Cryptography;
using System.Threading.Tasks;

namespace Microsoft.SimpleXboxSecureTokens
{
    public class XstsSigningKeyController 
    {
        /// <summary>
        /// This class requires your service to have an in-memory cache so that the
        /// signing keys can be cached and re-used rather than downloaded with each
        /// incoming XSTS Token.
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

        /// <summary>
        /// Default creator, requires your service's in-memory cache
        /// </summary>
        /// <param name="serverCache">your service's in-memory cache to store signing keys</param>
        public XstsSigningKeyController(IMemoryCache serverCache)
        {
            ServerCache = serverCache;
        }

        /// <summary>
        /// Will call the Xbl signing key endpoint and cache all keys that are currently
        /// available.  This is intended to be called when the service starts.  If a new key
        /// is encountered, then the GetSigningKey should retrieve and cache that new
        /// key as part of its flow.
        /// </summary>
        /// <returns>Array of all the keys that were encountered</returns>
        public async Task<List<XstsCachedSigningKey>> GetAndCacheCurrentSigningKeysAsync()
        {
            var keysFound = new List<string>();
            var signingKeys = await DownloadXSTSSigningKeys(XstsConstants.XblCurrentSigningKeysURL);

            return CacheSigningKeys(signingKeys);
        }

        /// <summary>
        /// Gets the Xbox Live signing key used to generate the signature of the inner-token, this key
        /// is identified with the kid and jku values in the decrypted payload token's headers.
        /// </summary>
        /// <param name="decryptedPayload">The inner-token that comes from decrypting the payload of the outer XSTS Token</param>
        /// <returns>Json Web Key used to sign the JWT</returns>
        public async Task<XstsCachedSigningKey?> GetSigningKey(string signingKeyId, string jku)
        {
            XstsCachedSigningKey? cachedSignatureKey = null;

            //  Get the kid and check if we already have the key in our cache
            if (ServerCache != null)
            {
                cachedSignatureKey = ServerCache.Get<XstsCachedSigningKey>(signingKeyId);

                if (cachedSignatureKey == null)
                {
                    //  This is not in the cache so we need to retrieve it from the XSTS signing key service
                    var newCachedKeys = await DownloadXSTSSigningKeys(jku);

                    foreach(var key in newCachedKeys)
                    {
                        if (key.KeyId == signingKeyId)
                        {
                            cachedSignatureKey = CacheXSTSSigningKey(key);
                            break;
                        }
                    }
                }
            }
            else
            {
                throw new Exception("ServerCache is not initialized");
            }

            return cachedSignatureKey;
        }

        /// <summary>
        /// Download all of the keys at the specified jku address
        /// </summary>
        /// <param name="jku">Address where the key can be downloaded</param>
        /// <returns>All XstsSigningKeys at the jku</returns>
        private static async Task<List<XstsSigningKey>> DownloadXSTSSigningKeys(string jku)
        {
            //  Verify that this cert is being downloaded and came from the trusted host.
            Uri jkuUri = new(jku);
            if (!jkuUri.Host.Equals(XstsConstants.XblSigningKeysTrustedHost) && // Rotating JWK keys
                !jkuUri.Host.Equals(XstsConstants.XblSigningCertTrustedHost))   // Xbl Signing Cert in JWK form)
            {
                //  This is not a valid XSTS signing key URL
                throw new ArgumentException("Invalid jku host that is not trusted: {0}", jku);
            }

            var signingKeys = new XstsSigningKeysResponse();

            using (var client = CreateHttpClientFunc())
            {
                using (HttpResponseMessage response = await client.GetAsync(jku))
                {
                    var responseBody = await response.Content.ReadAsStringAsync();
                    var convertedKeys = JsonConvert.DeserializeObject<XstsSigningKeysResponse>(responseBody);
                    if (convertedKeys != null)
                    {
                        signingKeys = convertedKeys;
                    }
                }
            }

            return signingKeys.Keys;
        }

        /// <summary>
        /// Helper to cache multiple signing keys either downloaded from the XBL service or loaded from a file
        /// </summary>
        /// <param name="signingKeys">List of keys to be cached</param>
        /// <returns>List of keys that were added to the cache</returns>
        public List<XstsCachedSigningKey> CacheSigningKeys(List<XstsSigningKey> signingKeys)
        {
            var cachedKeys = new List<XstsCachedSigningKey>();

            foreach (var signingKey in signingKeys)
            {
                if (string.IsNullOrEmpty(signingKey.KeyId))
                {
                    cachedKeys.Add(CacheXSTSSigningKey(signingKey));
                }
            }

            return cachedKeys;
        }

        /// <summary>
        /// Converts the specified signing key to the format our cache uses, adds it to the cache, then returns the
        /// cached object.
        /// </summary>
        /// <param name="signingKey">Signing key downloaded from the jku that we need to cache</param>
        /// <returns>Cached object of the key</returns>
        private XstsCachedSigningKey CacheXSTSSigningKey(XstsSigningKey signingKey)
        {
            var cacheExpirationOptions = new MemoryCacheEntryOptions
            {
                AbsoluteExpiration = signingKey.Expires.AddHours(720) // The key should not show up in tokens
                                                                      // after it expires, but the max time
                                                                      // that a Relying Party Xtoken can be
                                                                      // valid is 720 hours (30 days) and
                                                                      // there is a possibility of those
                                                                      // tokens still being used up until then.
                                                                      // This value can be as short as your 
                                                                      // Relying Party token lifetime.
            };

            var jwk = XstsSigningKey.GetRSAProviderForPlatform();
            jwk.ImportParameters(new RSAParameters
            {
                Modulus  = Base64Url.Decode(signingKey.Modulus),     
                Exponent = Base64Url.Decode(signingKey.Exponent)
            });

            //  Take the values and cache it as a ready-to use key
            var cachedKey = new XstsCachedSigningKey()
            {
                KeyId     = signingKey.KeyId,
                Key       = jwk,
                KeyType   = signingKey.KeyType,
                Algorithm = signingKey.Algorithm
            };

            if (ServerCache != null)
            {
                //  Cache the token so we can use it later
                ServerCache.Set<XstsCachedSigningKey>(signingKey.KeyId, cachedKey, cacheExpirationOptions);
            }

            return cachedKey;
        }
    }
}

#pragma warning restore IDE0063 // Use simple 'using' statement