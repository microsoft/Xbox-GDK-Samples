//-----------------------------------------------------------------------------
// XstsClientToken.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using System;
using System.Collections.Generic;

namespace Microsoft.XboxSecureTokens
{
    //  SECTION 1 - JSON formatting class for claims from a client provided XSTS token
    //  Classes were generated using http://json2csharp.com by giving the 
    //  raw JSON payload of the decrypted XSTS token from the client.
    //  
    //  Member names were then changed by using the [JsonProperty] attribute
    //  for more clear usage in code.  Example sbx -> Sandbox
    public class XDeviceClaims
    {
        [JsonProperty("ddm")] public string DeviceDebug { get; set; }
        [JsonProperty("dty")] public string DeviceType { get; set; }
        [JsonProperty("dvr")] public string DeviceVersion { get; set; }
        [JsonProperty("dgr")] public string DeviceGroups { get; set; }
        [JsonProperty("dpi")] public string DevicePwid { get; set; }
    }

    public class XTitleClaims
    {
        [JsonProperty("tid")] public string TitleId { get; set; }
        [JsonProperty("tvr")] public string TitleVersion { get; set; }
        [JsonProperty("tgr")] public string TitleGroups { get; set; }
    }

    public class Jwk
    {
        public string alg { get; set; }
        public string kty { get; set; }
        public string use { get; set; }
        public string crv { get; set; }
        public string x { get; set; }
        public string y { get; set; }
    }

    public class XProofKeyClaims
    {
        public Jwk jwk { get; set; }
    }

    public class XUserClaims
    {
        [JsonProperty("dlt")] public string DelegationToken { get; set; }
        [JsonProperty("gtg")] public string Gamertag { get; set; }
        [JsonProperty("uhs")] public string UserHash { get; set; }
        [JsonProperty("uts")] public string UserTest { get; set; }
        [JsonProperty("ugr")] public string UserGroups { get; set; }
        [JsonProperty("agg")] public string AgeGroup { get; set; }
        [JsonProperty("ctr")] public string CountryByIP { get; set; }
        [JsonProperty("prv")] public string Privileges { get; set; }
        [JsonProperty("upi")] public string UserPwid { get; set; }
        [JsonProperty("pfi")] public string PartnerUserFamilyId { get; set; }
        [JsonProperty("ptx")] public string Pxuid { get; set; }
    }

    public class XstsClientToken
    {
        //  This is the placeholder for the ush that was part of the Authorization header
        //  and identifies which user in the token we should be acting on
        public string UserHash { get; set; }

        [JsonProperty("aud")] public string Audience { get; set; }
        [JsonProperty("iss")] public string Issuer { get; set; }
        [JsonProperty("xdi")] public XDeviceClaims Device { get; set; }
        [JsonProperty("xti")] public XTitleClaims Title { get; set; }
        [JsonProperty("sbx")] public string Sandbox { get; set; }
        [JsonProperty("cnf")] public XProofKeyClaims ProofKey { get; set; }
        [JsonProperty("xui")] public List<XUserClaims> Users { get; set; }
        [JsonProperty("nbf")]
        public int EpochNotBefore
        {
            get
            {
                return this.epochNotBefore;
            }

            set
            {
                //  So that we can use a DateTime object we will
                //  just do the conversion from Unix Epoch now
                var epoch = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);
                this.NotBefore = epoch.AddSeconds(value);
                this.epochNotBefore = value;
            }
        }
        [JsonProperty("exp")]
        public int EpochExpires
        {
            get
            {
                return this.epochExpires;
            }

            set
            {
                //  So that we can use a DateTime object we will
                //  just do the conversion from Unix Epoch now
                var epoch = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);
                this.Expires = epoch.AddSeconds(value);
                this.epochExpires = value;
            }
        }

        public DateTime NotBefore { get; set; }
        public DateTime Expires { get; set; }
        private int epochNotBefore;
        private int epochExpires;
    }
}
