//-----------------------------------------------------------------------------
// XblSignatureInfo.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

namespace Microsoft.SimpleXboxSecureTokens
{
    public enum XblSignatureType
    {
        XblSigningKey = 0,
        XblSigningCert,
        Unknown
    }

    public class XblSignatureInfo
    {
        public XblSignatureType SignatureType { get; set; }
        public required string Id { get; set; }
    }
}
