//-----------------------------------------------------------------------------
// CachedAccessTokenProvider_UnitTests.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.Extensions.Caching.Memory;
using Microsoft.StoreServices;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Net.Http;

namespace StoreServices_UnitTests
{
    [TestClass]
    public class CachedAccessTokenProvider_UnitTests
    {
        /// <summary>
        /// Validates the parameter checking when creating a CachedAccessTokenProvider.
        /// </summary>
        [TestMethod]
        public void InvalidCreationParameters_Exception()
        {
            var cacheOptions = new MemoryCacheOptions();
            var memoryCache = new MemoryCache(cacheOptions);

            Assert.ThrowsException<System.ArgumentException>(() => new CachedAccessTokenProvider( null, "1", "1", "1") );
            Assert.ThrowsException<System.ArgumentException>(() => new CachedAccessTokenProvider(memoryCache, null, "1", "1"));
            Assert.ThrowsException<System.ArgumentException>(() => new CachedAccessTokenProvider(memoryCache, "1", null, "1"));
            Assert.ThrowsException<System.ArgumentException>(() => new CachedAccessTokenProvider(memoryCache, "1", "1", ""));
        }

        /// <summary>
        /// Verifies that when the cache doesn't have a token of the asked type that it retrieves a token and the
        /// cache grows as they are added.
        /// </summary>
        [TestMethod]
        public void TokenNotCached_TokenCreatedAndCached()
        {
            var cacheOptions = new MemoryCacheOptions();
            var memoryCache = new MemoryCache(cacheOptions);
            var cachedTokenProvider = new CachedAccessTokenProvider(memoryCache, "tenantId", "clientId", "clientSecret");
            
            CachedAccessTokenProvider.CreateHttpClientFunc = () => new HttpClient(new TestServiceTokenHttpMessageHandler());
            Assert.AreEqual(0, memoryCache.Count);
            var serviceTokenTask = cachedTokenProvider.GetServiceAccessTokenAsync();
            var serviceToken = serviceTokenTask.Result;
            Assert.AreEqual(1, memoryCache.Count);

            CachedAccessTokenProvider.CreateHttpClientFunc = () => new HttpClient(new TestCollectionsTokenHttpMessageHandler());
            var collectionsTokenTask = cachedTokenProvider.GetCollectionsAccessTokenAsync();
            var collectionsToken = collectionsTokenTask.Result;
            Assert.AreEqual(2, memoryCache.Count);
            
            CachedAccessTokenProvider.CreateHttpClientFunc = () => new HttpClient(new TestPurchaseTokenHttpMessageHandler());
            var purchaseTokenTask = cachedTokenProvider.GetPurchaseAccessTokenAsync();
            var purchaseToken = purchaseTokenTask.Result;
            Assert.AreEqual(3, memoryCache.Count);
        }

        /// <summary>
        /// Verifies that if the cache is populated, that it returns those tokens and does not retrieve new ones
        /// </summary>
        [TestMethod]
        public void TokenCached_CachedTokenReturned()
        {
            var cacheOptions = new MemoryCacheOptions();
            var memoryCache = new MemoryCache(cacheOptions);
            var cachedTokenProvider = new CachedAccessTokenProvider(memoryCache, "tenantId", "clientId", "clientSecret");


            CachedAccessTokenProvider.CreateHttpClientFunc = () => new HttpClient(new TestServiceTokenHttpMessageHandler());
            Assert.AreEqual(0, memoryCache.Count);
            var serviceTokenTask = cachedTokenProvider.GetServiceAccessTokenAsync();
            var serviceToken = serviceTokenTask.Result;

            CachedAccessTokenProvider.CreateHttpClientFunc = () => new HttpClient(new TestCollectionsTokenHttpMessageHandler());
            var collectionsTokenTask = cachedTokenProvider.GetCollectionsAccessTokenAsync();
            var collectionsToken = collectionsTokenTask.Result;

            CachedAccessTokenProvider.CreateHttpClientFunc = () => new HttpClient(new TestPurchaseTokenHttpMessageHandler());
            var purchaseTokenTask = cachedTokenProvider.GetPurchaseAccessTokenAsync();
            var purchaseToken = purchaseTokenTask.Result;

            serviceTokenTask = cachedTokenProvider.GetServiceAccessTokenAsync();
            collectionsTokenTask = cachedTokenProvider.GetCollectionsAccessTokenAsync();
            purchaseTokenTask = cachedTokenProvider.GetPurchaseAccessTokenAsync();

            var cachedServiceToken = serviceTokenTask.Result;
            var cachedCollectionsToken = collectionsTokenTask.Result;
            var cachedPurchaseToken = purchaseTokenTask.Result;

            Assert.AreEqual(serviceToken, cachedServiceToken);
            Assert.AreEqual(collectionsToken, cachedCollectionsToken);
            Assert.AreEqual(purchaseToken, cachedPurchaseToken);
        }

        /// <summary>
        /// Verifies that when an item expires in the cache that a new token is retrieved and cached
        /// </summary>
        [TestMethod]
        public void TokenExpired_TokenCreatedAndCached()
        {
            var cacheOptions = new MemoryCacheOptions();
            var memoryCache = new MemoryCache(cacheOptions);
            var cachedTokenProvider = new CachedAccessTokenProvider(memoryCache, "tenantId", "clientId", "clientSecret");

            CachedAccessTokenProvider.CreateHttpClientFunc = () => new HttpClient(new TestExpiringServiceTokenHttpMessageHandler());
            Assert.AreEqual(0, memoryCache.Count);
            var expiringServiceTokenTask = cachedTokenProvider.GetServiceAccessTokenAsync();
            var expiringServiceToken = expiringServiceTokenTask.Result;
            Assert.AreEqual(1, memoryCache.Count);
            CachedAccessTokenProvider.CreateHttpClientFunc = () => new HttpClient(new TestServiceTokenHttpMessageHandler());
            var newServiceTokenTask = cachedTokenProvider.GetServiceAccessTokenAsync();
            Assert.AreEqual(1, memoryCache.Count);  //  size should be one because the old expired token should no longer
                                                    //  be there and the new token should overwrite the old one.
            var newServiceToken = newServiceTokenTask.Result;

            Assert.AreNotEqual(expiringServiceToken, newServiceToken);
        }
    }
}
