//-----------------------------------------------------------------------------
// XstsCachedSigningKey.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Jose;
using System.Security.Cryptography;

namespace Microsoft.SimpleXboxSecureTokens
{
    /// <summary>
    /// JWK values for XSTS token signature validation
    /// </summary>
    public class XstsCachedSigningKey
    {
        public string KeyId { get; set; }

        public JwsAlgorithm Algorithm { get; set; }

        public string KeyType { get; set; }

        public RSA Key { get; set; }

        public XstsCachedSigningKey()
        {
            Key       = XstsSigningKey.GetRSAProviderForPlatform();
            KeyId     = string.Empty;
            Algorithm = new JwsAlgorithm();
            KeyType   = string.Empty;
        }

        /// <summary>
        /// Converts the specified signing key to a simplified format that can be cached and used with
        /// the XSTS token signing validation helpers.
        /// </summary>
        /// <param name="signingKey">Signing key downloaded from the jku that we need to cache</param>
        /// <returns>Simplified key that can be cached and used with follow up XSTS functions</returns>
        public XstsCachedSigningKey(XstsSigningKey signingKey)
        {
            var jwk = XstsSigningKey.GetRSAProviderForPlatform();
            jwk.ImportParameters(new RSAParameters
            {
                Modulus  = Base64Url.Decode(signingKey.Modulus),
                Exponent = Base64Url.Decode(signingKey.Exponent)
            });

            //  Take the values so they are easier and ready to use with our XSTS signing validation
            KeyId     = signingKey.KeyId;
            Key       = jwk;
            KeyType   = signingKey.KeyType;
            Algorithm = signingKey.Algorithm;
        }
    }
}
