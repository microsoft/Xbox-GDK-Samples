//-----------------------------------------------------------------------------
// AccessTokenProvider_UnitTests.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.StoreServices;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace StoreServices_UnitTests
{
    [TestClass]
    public class AccessTokenProvider_UnitTests
    {
        /// <summary>
        /// Validates the parameter checking when creating an AccessTokenProvider.
        /// </summary>
        [TestMethod]
        public void InvalidCreationParameters_Exception()
        {
            Assert.ThrowsException<System.ArgumentException>(() => new AccessTokenProvider( null, "1", "1") );
            Assert.ThrowsException<System.ArgumentException>(() => new AccessTokenProvider("1", null, "1"));
            Assert.ThrowsException<System.ArgumentException>(() => new AccessTokenProvider("1", "1", ""));
        }
    }
}
