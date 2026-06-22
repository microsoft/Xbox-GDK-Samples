//-----------------------------------------------------------------------------
// EccJsonWebKey.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using System;
using System.Runtime.Serialization;
using System.Security.Cryptography;

namespace Microsoft.XboxSecureTokens.XstsDelegatedAuth
{
    //  SECTION 2 - Ecc JSON web keys used to request an S-token
    /// <summary>
    /// ECC key represented by two points X and Y. These are big endian 
    /// encoded unsigned integers represented as a base64 string.
    /// Follows the standard at: http://tools.ietf.org/html/draft-ietf-jose-json-web-key-08
    /// Provides the common key properties so that individual key types
    /// can derive from this class (such as RSA and ECC)
    /// </summary>
    [DataContract]
    public class EccJsonWebKey
    {
        /// <summary>
        /// Algorithm family. This is required. Supported values can be found in JsonWebKeyAlgorithms
        /// </summary>
        [JsonProperty("kty")]
        public string KeyType { get; set; }

        /// <summary>
        /// Algorithm family. This is OPTIONAL.
        /// </summary>
        [JsonProperty("alg")]
        public string Algorithm { get; set; }

        /// <summary>
        /// Curve types supported by the EccJsonWebKey class.
        /// http://tools.ietf.org/html/draft-ietf-jose-json-web-algorithms-08#section-5.2.1
        /// </summary>
        [JsonProperty("crv")]
        public string CurveType { get; set; }

        [JsonProperty("x")]
        public string XCoordinate
        {
            get
            {
                return XstsUtilities.ToBase64Url(this.X);
            }
            set
            {
                this.X = XstsUtilities.FromBase64Url(value);
            }
        }

        [JsonProperty("y")]
        public string YCoordinate
        {
            get
            {
                return XstsUtilities.ToBase64Url(this.Y);
            }
            set
            {
                this.Y = XstsUtilities.FromBase64Url(value);
            }
        }

        /// <summary>
        /// X coordinate of the key
        /// </summary>
        public byte[] X { get; set; }

        /// <summary>
        /// Y coordinate of the key
        /// </summary>
        public byte[] Y { get; set; }

        /// <summary>
        /// Default constructor.
        /// </summary>
        public EccJsonWebKey()
        {
        }

        /// <summary>
        /// Initializes an ECC JWK based on an ECDsa object. 
        /// Extracts X, Y and curve type properties.
        /// </summary>
        /// <param name="ecdsa"></param>
        public EccJsonWebKey(ECDsa ecdsa)
        {
            ECParameters ecc = ecdsa.ExportParameters(false);
            string curveName = ecc.Curve.Oid.FriendlyName;
            if (!XstsConstants.CurveNameToCurveType.TryGetValue(curveName, out var crv))
            {
                throw new InvalidOperationException($"{curveName} is not supported.");
            }

            this.CurveType = crv;
            this.X = ecc.Q.X;
            this.Y = ecc.Q.Y;
            this.KeyType = "EC";

            switch (ecdsa.KeySize)
            {
                case 256:
                    this.Algorithm = XstsConstants.ECDSASHA256;
                    break;
                case 384:
                    this.Algorithm = XstsConstants.ECDSASHA384;
                    break;
                case 512:
                    this.Algorithm = XstsConstants.ECDSASHA512;
                    break;
            }
        }
    }
}