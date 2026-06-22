//-----------------------------------------------------------------------------
// XstsServiceToken.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;

namespace Microsoft.XboxSecureTokens.XstsDelegatedAuth
{
    /// SECTION 2 - Class formatted for json service token responses
    //  Classes were generated using http://json2csharp.com from example
    //  JSON request and response
    public class XstsServiceToken
    {
        public DateTime IssueInstant { get; set; }
        public DateTime NotAfter { get; set; }
        [Key] public string Token { get; set; }
        public byte[] ProofKeyInBytes { get; set; }
    }

    public class XstsRequestProperties
    {
        public string ServiceToken { get; set; }
        public string SandboxId { get; set; }
        public string DelegationToken { get; set; }
    }

    public class XstsRequestBody
    {
        public XstsRequestBody()
        {
            this.Properties = new XstsRequestProperties();
        }
        public string RelyingParty { get; set; }
        public string TokenType { get; set; }
        public XstsRequestProperties Properties { get; set; }
    }

    public class XstsServiceAuthRequestBody
    {
        public string RelyingParty { get; set; }
        public string TokenType { get; set; }
        public XstsServerAuthRequestProperties Properties { get; set; } = new XstsServerAuthRequestProperties();
    }

    public class XstsServerAuthRequestProperties
    {
        public string ServiceToken { get; set; }
        public string SandboxId { get; set; }
    }

    public class XstsServiceTokenRequestProperties
    {
        public Ecc256ProofKey ProofKey { get; set; }
    }

    public class XstsServiceTokenRequestBody
    {
        public XstsServiceTokenRequestProperties Properties { get; set; }
        public string RelyingParty { get; set; }
        public string TokenType { get; set; }
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

    public class B2BXToken
    {
        public DateTime IssueInstant { get; set; }
        public DateTime NotAfter { get; set; }
        public string Token { get; set; }
        public DisplayClaims DisplayClaims { get; set; }
        public byte[] ProofKeyInBytes { get; set; }
        public string SandboxId { get; set; }
        public string RelyingParty { get; set; }
    }
}
