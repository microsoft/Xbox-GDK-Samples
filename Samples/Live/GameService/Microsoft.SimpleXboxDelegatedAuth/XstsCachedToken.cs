//-----------------------------------------------------------------------------
// XstsCachedToken.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System.ComponentModel.DataAnnotations;

namespace Microsoft.SimpleXboxDelegatedAuth
{
    //  In memory cache for tokens
    public class XstsCachedToken
    {
        [MaxLength(2750)]
        public string Token { get; set; }
        public byte[] ProofKeyInBytes { get; set; }
    }
}
