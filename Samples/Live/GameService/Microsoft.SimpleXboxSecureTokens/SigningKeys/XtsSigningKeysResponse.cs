//-----------------------------------------------------------------------------
// XstsSigningKeysResponse.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using System.Collections.Generic;

namespace Microsoft.SimpleXboxSecureTokens
{
    /// <summary>
    /// JSON response body from the XSTS token signing keys
    /// </summary>
    public class XstsSigningKeysResponse
    {
        /// <summary>
        /// List of Json Web Keys that are active for XSTS token signature validation
        /// </summary>
        [JsonProperty("keys")]
        public List<XstsSigningKey> Keys { get; set; }

        public XstsSigningKeysResponse()
        {
            Keys = new List<XstsSigningKey>();
        }
    }
}
