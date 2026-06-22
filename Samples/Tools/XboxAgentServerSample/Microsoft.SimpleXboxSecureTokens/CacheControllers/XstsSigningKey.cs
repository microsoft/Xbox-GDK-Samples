//-----------------------------------------------------------------------------
// XstsSigningKey.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Jose;
using Newtonsoft.Json;
using System;

namespace Microsoft.SimpleXboxSecureTokens
{
    /// <summary>
    /// JWK values for XSTS token signature validation
    /// </summary>
    public class XstsSigningKey
    {
        [JsonProperty("kid")]
        public required string KeyId { get; set; }

        [JsonProperty("n")]
        public required string Modulus { get; set; }

        [JsonProperty("e")]
        public required string Exponent { get; set; }

        [JsonProperty("alg")]
        public JwsAlgorithm Algorithm { get; set; }

        [JsonProperty("kty")]
        public required string KeyType { get; set; }

        [JsonProperty("use")]
        public required string KeyUse { get; set; }

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

        [JsonProperty("iat")]
        public int EpochIssuedAt
        {
            get
            {
                return this.epochIssuedAt;
            }

            set
            {
                //  So that we can use a DateTime object we will
                //  just do the conversion from Unix Epoch now
                var epoch = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);
                this.IssuedAt = epoch.AddSeconds(value);
                this.epochIssuedAt = value;
            }
        }

        public DateTime NotBefore { get; set; }
        public DateTime Expires { get; set; }
        public DateTime IssuedAt { get; set; }
        private int epochNotBefore;
        private int epochExpires;
        private int epochIssuedAt;

    }
}
