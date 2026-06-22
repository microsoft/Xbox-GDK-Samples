//-----------------------------------------------------------------------------
// XstsConstants.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Globalization;

namespace Microsoft.XboxSecureTokens
{
    //  SECTION 1 - change the ServiceName value to your own unique value
    static public class XstsConstants
    {
        public const int CollectionsPageSize = 100;

        //  Change this value so that it identifies your
        //  service or title from other servers when
        //  making calls to XSTS and Xbox Live services
        public const string ServiceName = "GameServiceSamplev1.8";
        public const string CacheKeyFormat = "{0}:{1}:{2}";
        public const string BPCertThumprintKey = "BP_CERT_THUMBPRINT";
        public const string RPCertThumprintKey = "RP_CERT_THUMBPRINT";

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

        public const string P256 = "P-256";
        public const string P384 = "P-384";
        public const string P521 = "P-521";
        public const int Ecc256PublicBlobMagic = 0x31534345; // "ECS1"
        public const int Ecc384PublicBlobMagic = 0x33534345; // "ECS3"
        public const int Ecc521PublicBlobMagic = 0x35534345; // "ECS5"
        public const char Base64PadCharacter = '=';
        public static string DoubleBase64PadCharacter = String.Format(CultureInfo.InvariantCulture, "{0}{0}", Base64PadCharacter);
        public const char Base64Character62 = '+';
        public const char Base64Character63 = '/';
        public const char Base64UrlCharacter62 = '-';
        public const char Base64UrlCharacter63 = '_';
        public const string ECDSASHA256 = "ES256";
        public const string ECDSASHA384 = "ES384";
        public const string ECDSASHA512 = "ES512";

        //  Endpoints that we will be using with our services
        public const string XblSocialService = "social.xboxlive.com";
        public const string XblMPSDService = "sessiondirectory.xboxlive.com";
        public const string GDPRListService = "deletedaccounts.xboxlive.com";
        public const string XblCurrentSigningCertsURL = "https://xsts.auth.xboxlive.com/xsts/signingkeys";

        //  Pre-defined list of just the endpoints that our services actually need
        //  so that we don't spend cycles searching through the full endpoint list
        //  on sites that we will never use.
        public static string[] TargetXblEndpoints = 
        {
                //  The most used XBL endpoints and its wildcard 
                //  which will be our default DB entry 
                "*.xboxlive.com",
                "privileges.xboxlive.com",
                "privileges.xboxlive.com",

                //  Commerce / store related endpoints
                "inventory.xboxlive.com",
                "licensing.xboxlive.com",
                "licensing.mp.microsoft.com",
                "collections.mp.microsoft.com"
         };

        public const string ContentTypeHeaderKey = "Content-type";
    }
}
