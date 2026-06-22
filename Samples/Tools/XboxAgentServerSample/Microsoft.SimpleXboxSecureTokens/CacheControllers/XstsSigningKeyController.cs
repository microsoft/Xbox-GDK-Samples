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
    //  Section 1 - Stores and loads signing keys frequently used for Xsts token handling and requests
    public class XstsSigningKeyController : XstsControllerBase
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

        public XstsSigningKeyController() 
        {
        }

        /// <summary>
        /// Will call the Xbl signing key endpoint and cache all the keys that are currently
        /// available.  This is intended to be called when the service starts.  If a new key
        /// is encountered, then the GetSigningKey should retrieve and cache that new
        /// key as part of its flow.
        /// </summary>
        /// <returns>Array of all the keys that were encountered</returns>
        public static async Task<string[]> GetAndCacheCurrentSigningKeys()
        {
            var keysFound = new List<string>();
            var signingKeys = await DownloadXSTSSigningKeys(XstsConstants.XblCurrentSigningKeysURL);

            foreach (var signingKey in signingKeys)
            {
                if (string.IsNullOrEmpty(signingKey.KeyId))
                {
                    keysFound.Add(signingKey.KeyId);
                    CacheXSTSSigningKey(signingKey);
                }
            }
            
            return keysFound.ToArray();
        }

        /// <summary>
        /// Gets the Xbox Live signing key used to generate the signature of the inner-token, this key
        /// is identified with the kid and jku values in the decrypted payload token's headers.
        /// /// </summary>
        /// <param name="decryptedPayload">The inner-token that comes from decrypting the payload of the outer XSTS Token</param>
        /// <returns>Json Web Key used to sign the JWT</returns>
        public static async Task<XstsCachedSigningKey?> GetSigningKey(string decodedPayload)
        {
            var tokenHeaders = Jose.JWT.Headers(decodedPayload);
            XstsCachedSigningKey? cachedSignatureKey = null;

            //  Get the kid and check if we already have the key in our cache that was used on the token.
            string signingKeyId = (string)tokenHeaders["kid"];

            if (ServerCache != null)
            {
                cachedSignatureKey = ServerCache.Get<XstsCachedSigningKey>(signingKeyId);

                if (cachedSignatureKey == null)
                {
                    //  This is not in the cache so we need to retrieve it from the XSTS signing key service
                    //signatureKey = await GetAndCacheSigningKey((string)tokenHeaders["jku"], signingKeyId);

                    var signatureKey = await XstsCore.DownloadXblSigningKey(decodedPayload);

                    cachedSignatureKey = CacheXSTSSigningKey(signatureKey);
                }
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
            //  Verify that this cert is being downloaded and came from the trusted
            //  host.
            Uri jkuUri = new(jku);
            if (!jkuUri.Host.Equals("xsts-keys.auth.xboxlive.com"))
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
        /// Converts the specified signing key to the format our cache uses, adds it to the cache, then returns the
        /// cached object.
        /// </summary>
        /// <param name="signingKey">Signing key downloaded from the jku that we need to cache</param>
        /// <returns>Cached object of the key</returns>
        private static XstsCachedSigningKey CacheXSTSSigningKey(XstsSigningKey signingKey)
        {
            var cacheExpirationOptions = new MemoryCacheEntryOptions
            {
                AbsoluteExpiration = signingKey.Expires.AddDays(1) // The key should not show up in tokens
                                                                   // after it expires, but there are some
                                                                   // outstanding tokens that may still 
                                                                   // have it so we keep it in the cache
                                                                   // for one extra day.
            };

            RSACryptoServiceProvider jwk = new();
            jwk.ImportParameters(new RSAParameters
            {
                Modulus = Base64Url.Decode(signingKey.Modulus),     //Base64Url.Decode(jwk["n"]),
                Exponent = Base64Url.Decode(signingKey.Exponent),    //Base64Url.Decode(jwk["e"])
            });

            //  Take the values and cache it as a ready-to use key
            var cachedKey = new XstsCachedSigningKey()
            {
                KeyId = signingKey.KeyId,
                Key = jwk,
                KeyType = signingKey.KeyType,
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