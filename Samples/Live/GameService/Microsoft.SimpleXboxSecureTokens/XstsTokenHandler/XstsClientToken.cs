//-----------------------------------------------------------------------------
// XstsClientToken.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

// Ignore Spelling: Pwid

using Newtonsoft.Json;
using System;
using System.Collections.Generic;

namespace Microsoft.SimpleXboxSecureTokens
{
#pragma warning disable IDE1006 // Naming Styles
    public class XDeviceClaims
    {
        [JsonProperty("ddi")] public string? DeveloperDeviceId { get; set; }
        [JsonProperty("dca")] public string? DeviceCapabilities { get; set; }
        [JsonProperty("ddm")] public string? DeviceDebug { get; set; }
        [JsonProperty("dgr")] public string? DeviceGroups { get; set; }
        [JsonProperty("dpi")] public string? DevicePwid { get; set; }
        [JsonProperty("dty")] public string? DeviceType { get; set; }
        [JsonProperty("dvr")] public string? DeviceVersion { get; set; }
    }

    public class XTitleClaims
    {
        [JsonProperty("tid")] public string? TitleId { get; set; }
        [JsonProperty("tgr")] public string? TitleGroups { get; set; }
        [JsonProperty("tvr")] public string? TitleVersion { get; set; }
    }

    public class Jwk
    {
        public string? alg { get; set; }
        public string? kty { get; set; }
        public string? use { get; set; }
        public string? crv { get; set; }
        public string? x { get; set; }
        public string? y { get; set; }
    }

    public class XProofKeyClaims
    {
        public Jwk? jwk { get; set; }
    }

    public class XUserClaims
    {
        [JsonProperty("agg")] public string? AgeGroup { get; set; }
        [JsonProperty("ctr")] public string? CountryByIP { get; set; }
        [JsonProperty("lng")] public string? Language { get; set; }
        [JsonProperty("dlt")] public string? DelegationToken { get; set; }
        [JsonProperty("gtg")] public string? Gamertag { get; set; }
        [JsonProperty("mgt")] public string? ModernGamertag { get; set; }
        [JsonProperty("mgs")] public string? ModernGamertagSuffix { get; set; }
        [JsonProperty("pfi")] public string? PartnerUserFamilyId { get; set; }
        [JsonProperty("prv")] public string? Privileges { get; set; }
        [JsonProperty("ptx")] public string? Pxuid { get; set; }
        [JsonProperty("ufi")] public string? UserFamilyId { get; set; }
        [JsonProperty("ugr")] public string? UserGroups { get; set; }
        [JsonProperty("ugs")] public string? UserGuest { get; set; }
        [JsonProperty("uhs")] public string? UserHash { get; set; }
        [JsonProperty("upi")] public string? UserPwid { get; set; }
        [JsonProperty("uts")] public string? UserTest { get; set; }
    }

    public class XstsClientToken
    {
        //  This is the placeholder for the ush that was part of the Authorization header
        //  and identifies which user in the token we should be acting on
        public string? UserHash { get; set; }

        [JsonProperty("aud")] public required string Audience { get; set; } = "NoAudience";
        [JsonProperty("iss")] public required string Issuer { get; set; } = "Noissuer";
        [JsonProperty("xdi")] public XDeviceClaims? Device { get; set; }
        [JsonProperty("xti")] public XTitleClaims? Title { get; set; }
        [JsonProperty("sbx")] public string? Sandbox { get; set; }
        [JsonProperty("cnf")] public XProofKeyClaims? ProofKey { get; set; }
        [JsonProperty("xui")] public List<XUserClaims>? Users { get; set; }

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

#pragma warning restore IDE1006 // Naming Styles
}
