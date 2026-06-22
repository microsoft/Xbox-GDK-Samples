//-----------------------------------------------------------------------------
// XstsAuthToken.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma warning disable IDE0063 // Use simple 'using' statement

using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.ComponentModel;

namespace Microsoft.SimpleXboxDelegatedAuth
{
    /// <summary>
    /// This can either be a Delegated Auth XSTS token (calling on-behalf-of a user)
    /// or a Service Auth XSTS token (calling on-behalf-of a service)
    /// </summary>
    public class XstsAuthToken
    {
        public DateTime IssueInstant { get; set; }
        public DateTime NotAfter { get; set; }
        public string Token { get; set; }
        public DisplayClaims DisplayClaims { get; set; }
        public byte[] ProofKeyInBytes { get; set; }
        public string SandboxId { get; set; }
        public string RelyingParty { get; set; }
        public SignaturePolicy SignaturePolicy { get; set; }
        public XstsTokenType TokenType { get; set; }
        public string UserID { get; set; }
    }

    public enum XstsTokenType
    {
        DelegatedAuth,  // On-behalf-of a user
        ServiceAuth     // On-behalf-of a service
    }

    public class DelegatedUserClaims
    {
        [JsonProperty("agg")] public string AgeGroup { get; set; }
        [JsonProperty("gtg")] public string Gamertag { get; set; }
        [JsonProperty("prv")] public string Privileges { get; set; }
        [JsonProperty("xid")] public string Xuid { get; set; }
        [JsonProperty("uhs")] public string UserHash { get; set; }
    }

    public class DisplayClaims
    {
        [JsonProperty("xui")] public List<DelegatedUserClaims> Users { get; set; }
    }

    public class XstsAuthTokenRequest
    {
        public string RelyingParty { get; set; }
        public string TokenType { get; set; }
        public XstsAuthTokenRequestProperties Properties { get; set; }
    }

    public class XstsAuthTokenRequestProperties
    {
        public string ServiceToken { get; set; }
        public string SandboxId { get; set; }

        [DefaultValue("")]
        [JsonProperty("DelegationToken", NullValueHandling = NullValueHandling.Ignore, DefaultValueHandling = DefaultValueHandling.Ignore)]
        public string DelegationToken { get; set; }
    }
}

#pragma warning restore IDE0063 // Use simple 'using' statement
