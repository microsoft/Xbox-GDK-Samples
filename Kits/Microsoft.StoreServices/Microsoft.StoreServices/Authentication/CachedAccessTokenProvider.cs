//-----------------------------------------------------------------------------
// CachedAccessTokenProvider.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.Extensions.Caching.Memory;
using System;
using System.Threading.Tasks;

namespace Microsoft.StoreServices
{
    /// <summary>
    /// An IAccessTokenProvider that generates, caches, retrieves, and manages the expiration of the 
    /// access tokens for your service required for Microsoft Store Services authentication.
    /// </summary>
    public class CachedAccessTokenProvider : AccessTokenProvider
    {
        /// <summary>
        /// Cache used to store and retrieve access tokens
        /// </summary>
        private readonly IMemoryCache _serverCache;

        /// <summary>
        /// Generates an access token provider that will manage a cache of the access tokens based 
        /// on your AAD credentials provided.
        /// <param name="serverCache">IMemoryCache to be used to cache and retrieve the tokens</param>
        /// <param name="tenantId">Registered AAD Tenant Id for your service</param>
        /// <param name="clientId">Registered AAD Client Id for your service</param>
        /// <param name="clientSecret">Registered AAD Client secret for your service</param>
        public CachedAccessTokenProvider(IMemoryCache serverCache,
                                         string tenantId,
                                         string clientId,
                                         string clientSecret) : base (tenantId, clientId, clientSecret)
        {
            if (serverCache == null)
            {
                throw new ArgumentException($"{nameof(serverCache)} required", nameof(serverCache));
            }
            _serverCache = serverCache;
        }

        /// <summary>
        /// Gets the currently cached Service access token or generates a new one if not cached.
        /// </summary>
        /// <returns></returns>
        public override Task<AccessToken> GetServiceAccessTokenAsync()
        {
            return GetTokenAsync(AccessTokenAudienceTypes.Service);
        }

        /// <summary>
        /// Gets the currently cached Collections access token or generates a new one if not cached.
        /// </summary>
        /// <returns></returns>
        public override Task<AccessToken> GetCollectionsAccessTokenAsync()
        {
            return GetTokenAsync(AccessTokenAudienceTypes.Collections);
        }

        /// <summary>
        /// Gets the currently cached Purchase access token or generates a new one if not cached.
        /// </summary>
        /// <returns></returns>
        public override Task<AccessToken> GetPurchaseAccessTokenAsync()
        {
            return GetTokenAsync(AccessTokenAudienceTypes.Purchase);
        }

        /// <summary>
        /// Retrieves a valid cached access token based on the audience provided. If not cached,
        /// a new token is created and cached.
        /// </summary>
        /// <param name="audience"></param>
        /// <returns></returns>
        protected async Task<AccessToken> GetTokenAsync(string audience)
        {
            var currentUTC = DateTime.Now.ToUniversalTime();

            //  If we are unable to acquire a token, it is expired, or expiring
            //  in less than 5 minutes we create a new one and cache it
            if(!_serverCache.TryGetValue(audience, out AccessToken token) ||
               DateTime.Compare(token.ExpiresOn.AddMinutes(-5), currentUTC) <= 0)
            {
                token = await CreateAccessTokenAsync(audience);
                CacheAccessToken(audience, token);
            }

            return token;
        }

        /// <summary>
        /// Adds this token to the cache so that future calls for the same audience do not
        /// require generating a new one.
        /// </summary>
        /// <param name="audience"></param>
        /// <param name="token"></param>
        private void CacheAccessToken(string audience, AccessToken token)
        {
            //  Add the new token so other calls can get it
            //  We will set the cache expiration for an hour less than the token is valid for.
            //  This will remove it from the cache before it expires and will get a new one
            //  for the next process
            var cacheExpirationOptions = new MemoryCacheEntryOptions
            {
                AbsoluteExpiration = token.ExpiresOn,
                Priority = CacheItemPriority.High
            };

            _serverCache.Set<AccessToken>(audience, token, cacheExpirationOptions);
        }
    }
}
