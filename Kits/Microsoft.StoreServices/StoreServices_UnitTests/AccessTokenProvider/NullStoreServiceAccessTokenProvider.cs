//-----------------------------------------------------------------------------
// NullStoreServiceAccessTokenProvider.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.StoreServices;
using System.Threading.Tasks;

namespace StoreServices_UnitTests
{
    internal class NullStoreServiceAccessTokenProvider : IAccessTokenProvider
    {
        public Task<AccessToken> GetCollectionsAccessTokenAsync()
        {
            return Task.FromResult(new AccessToken());
        }

        public Task<AccessToken> GetPurchaseAccessTokenAsync()
        {
            return Task.FromResult(new AccessToken());
        }

        public Task<AccessToken> GetServiceAccessTokenAsync()
        {
            return Task.FromResult(new AccessToken());
        }
    }
}
