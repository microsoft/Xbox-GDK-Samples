//-----------------------------------------------------------------------------
// XstsServiceTokenRequest.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma warning disable IDE0063 // Use simple 'using' statement

using System;

namespace Microsoft.SimpleXboxDelegatedAuth
{
    /// <summary>
    /// Creates and preforms a request to the XSAS service to obtain a Service Token
    /// </summary>
    public class XstsServiceTokenRequest
    {
        public byte[] ProofKeyInBytes { get; set; }
        public Uri RequestUri { get; private set; }
        public XstsServiceTokenRequestBody RequestBody { get; set; }

        public string Host { get; set; }

        public XstsServiceTokenRequest()
        {
            var ecdsa = ProofKeyUtility.Create();

            this.RequestBody = new XstsServiceTokenRequestBody
            {
                TokenType = "JWT",
                RelyingParty = "http://auth.xboxlive.com"
            };
            this.Host = "service.auth.xboxlive.com";
            this.RequestUri = new Uri("https://service.auth.xboxlive.com/service/authenticate");

            var eParameters = ecdsa.ExportParameters(true);
            this.ProofKeyInBytes = eParameters.ExportToByteArray();

            // These are the JSON values that will be sent in the body of the request
            Ecc256ProofKey eccProofKey = new Ecc256ProofKey(new EccJsonWebKey(ecdsa));
            this.RequestBody.Properties = new XstsServiceTokenRequestProperties
            {
                ProofKey = eccProofKey
            };
        }
    }
}
#pragma warning restore IDE0063 // Use simple 'using' statement
