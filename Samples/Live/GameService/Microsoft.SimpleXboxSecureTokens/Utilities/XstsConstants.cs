//-----------------------------------------------------------------------------
// XstsConstants.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

// Ignore Spelling: NISTP ECDSAP ECDSASHA

using System;
using System.Collections.Generic;

namespace Microsoft.SimpleXboxSecureTokens
{
    /// <summary>
    /// Specifies the individual components of an XSTS authorization header used in secure data transmission and authentication.
    /// based on the format of "XBL3.0 x=<userhash>;<xststoken>"
    /// </summary>
    public enum XstsAuthHeaderParts
    {
        UserHash = 0,
        XstsToken = 1,
        Length
    }

    /// <summary>
    /// Specifies the individual components of an XSTS token used in secure data transmission and authentication.
    /// </summary>
    /// <remarks>Use this enumeration to identify and access specific parts of an XSTS token, such as the
    /// header, encryption key, initialization vector, payload, and authentication tag. The values correspond to
    /// distinct sections commonly required for cryptographic operations and token validation.</remarks>
    public enum XstsTokenParts
    {
        Header = 0,
        ContentEncryptionKey = 1,
        InitializationVector = 2,
        Payload = 3,
        AuthenticationTag = 4,
        Length
    }

    /// <summary>
    /// Specifies the individual parts of a content encryption key used in XSTS operations.
    /// </summary>
    /// <remarks>This enumeration identifies the components of a composite key used for content encryption in
    /// XSTS, such as the HMAC key and AES key. The values can be used to select or reference specific key parts when
    /// performing cryptographic operations or managing key material.</remarks>
    public enum XstsContentEncryptionKeyParts
    {
        HmacKey = 0,
        AesKey = 1,
        Length
    }

    static public class XstsConstants
    {
        public const string Version = "SimpleXboxSecureTokens_v2510";
        
        public const string XblCurrentSigningCertsURL = "https://xsts.auth.xboxlive.com/xsts/signingkeys";
        public const string XblCurrentSigningKeysURL = "https://xsts-keys.auth.xboxlive.com/keys";

        //  Trusted host names for the XBL signing keys (including the XBL signing cert in JWK form)
        public const string XblSigningKeysTrustedHost = "xsts-keys.auth.xboxlive.com";
        public const string XblSigningCertTrustedHost = "xsts.auth.xboxlive.com";

        public const string ContentTypeHeaderKey = "Content-type";

        public const string NISTP256 = "P-256";
        public const string NISTP384 = "P-384";
        public const string NISTP521 = "P-521";
        public const string NISTP256OidFriendlyName = "nistP256";
        public const string NISTP384OidFriendlyName = "nistP384";
        public const string NISTP521OidFriendlyName = "nistP521";

        /// <summary>
        /// On Linux the Oid Friendly Name is different than windows
        /// </summary>
        public const string ECDSAP256OidFriendlyName = "ECDSA_P256";
        public const string ECDSAP384OidFriendlyName = "ECDSA_P384";
        public const string ECDSAP512OidFriendlyName = "ECDSA_P512";

        public static readonly IReadOnlyDictionary<string, string> CurveNameToCurveType = new Dictionary<string, string>()
        {
            { NISTP256OidFriendlyName, NISTP256 },
            { NISTP384OidFriendlyName, NISTP384 },
            { NISTP521OidFriendlyName, NISTP521 },
            { ECDSAP256OidFriendlyName, NISTP256 },
            { ECDSAP384OidFriendlyName, NISTP384 },
            { ECDSAP512OidFriendlyName, NISTP521 }
        };

        public const char Base64PadCharacter = '=';
        public const char Base64Character62 = '+';
        public const char Base64Character63 = '/';
        public const char Base64UrlCharacter62 = '-';
        public const char Base64UrlCharacter63 = '_';
        public const string ECDSASHA256 = "ES256";
        public const string ECDSASHA384 = "ES384";
        public const string ECDSASHA512 = "ES512";

        //---------------------------------------------------------------------------------------------
        //  Constants for the in-memory cert cache
        public const string DefaultBPCertCacheKey = "DEFAULT_BP_CERT";
        //----------------------------------------------------------------------------------------------

        //  no reference now:
        public const string CacheKeyFormat = "{0}:{1}:{2}";

        public const string ProductionSandbox = "RETAIL";
    }
}
