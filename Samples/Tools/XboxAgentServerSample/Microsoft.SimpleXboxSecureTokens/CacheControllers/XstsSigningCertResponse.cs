//-----------------------------------------------------------------------------
// XstsSigningCertResponse.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;

namespace Microsoft.SimpleXboxSecureTokens
{
    //  SECTION 1 - Asymmetric signature validation cert retrieval and caching
    public class XstsSigningCertResponse
    {
        [JsonExtensionData]
        public required IDictionary<string, JToken> signingCerts;
    }

    public class LicenseTokenSignatureCertResponse
    {
        public required string Certificate { get; set; }
    }
}
