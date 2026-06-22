//-----------------------------------------------------------------------------
// XstsServiceTokenController.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using System;
using System.Threading.Tasks;

namespace Microsoft.XboxSecureTokens.XstsDelegatedAuth
{
    /// SECTION 2 - Requests and caches Service Tokens
    public class XstsServiceTokenController 
    {
        private ILogger mLogger;
        private Microsoft.CorrelationVector.CorrelationVector mCv;
        private IMemoryCache mServerCache;
        private IConfiguration mConfig;

        public XstsServiceTokenController (IConfiguration Config, IMemoryCache ServerCache, ILogger Logger, string CorrelationVectorString)
        {
            mConfig = Config;
            mServerCache = ServerCache;
            mLogger = Logger;
            mCv = Microsoft.CorrelationVector.CorrelationVector.Extend(CorrelationVectorString);
        }

        public async Task<XstsServiceToken> GetSTokenAsync()
        {
            XstsServiceToken sToken = mServerCache.Get<XstsServiceToken>("ServiceToken");

            if (sToken == null)
            {
                sToken = await RequestNewSTokenAsync();
            }

            return sToken;
        }

        private async Task<XstsServiceToken> RequestNewSTokenAsync()
        {
            var sTokenRequest = new XstsServiceTokenRequest(mConfig, mServerCache, mLogger, mCv.Increment());
            XstsServiceToken newSToken = await sTokenRequest.RequestTokenAsync();

            //  Add the new token so other calls can get it
            //  We will set the cache expiration for 12 days, the token should only be valid for
            //  14 so it will get removed from the cache before it does and a proccess will kick off to get a new one
            var cacheExpirationOptions = new MemoryCacheEntryOptions();
            cacheExpirationOptions.AbsoluteExpiration = DateTime.Now.AddDays(12);
            cacheExpirationOptions.SetPriority(CacheItemPriority.High);
            mServerCache.Set<XstsServiceToken>("ServiceToken", newSToken, cacheExpirationOptions);

            mLogger.StartupInfo(mCv.Increment(), String.Format("GameService - Service Token cached {0}...", newSToken.Token.Substring(0, 20)));
            return newSToken;
        }
    }
}