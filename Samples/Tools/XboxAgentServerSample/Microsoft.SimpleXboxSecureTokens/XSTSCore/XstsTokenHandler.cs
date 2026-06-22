//-----------------------------------------------------------------------------
// XstsTokenHandler.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using System;
using System.Threading.Tasks;

namespace Microsoft.SimpleXboxSecureTokens
{
    public class XstsTokenHandler
    {
        /// NOTE:   This class requires your service to have an in-memory cache.
        ///         To do this, please add the following to your program.cs or startup.cs:
        ///         
        ///         builder.Services.AddMemoryCache();
        ///         XstsControllerBase2.ServerCache = app.Services.GetService<IMemoryCache>();

        /// NOTE:   This class uses the XstsCore API's and to improve server performance your
        ///         service should use an HttpClientFactory to create HttpClients.  To set
        ///         The XstsCore API's to use your HttpClientFactory use the following override
        ///         in your program.cs or startup.cs:
        ///         
        ///         var httpClientFactory = app.Services.GetService<System.Net.Http.IHttpClientFactory>();
        ///         XstsCore.CreateHttpClientFunc = httpClientFactory.CreateClient;

        public XstsTokenHandler()
        {

        }

        public static async Task<XstsClientToken?> ValidateAuthorizationHeaderWithCache(string? authHeader)
        {
            if (string.IsNullOrEmpty(authHeader))
            {
                throw new ArgumentNullException(nameof(authHeader));
            }

            XstsClientToken? clientToken = null;
            try
            {
                string userHash = XstsCore.GetUserHashFromHeader(authHeader);
                string encryptedToken = XstsCore.GetXTokenFromHeader(authHeader);

                string relyingPartyThumbprint = XstsCore.GetRelyingPartyThumbprint(encryptedToken);
                var relyingPartyCert = XstsCore.GetCertFromCurrentUserStore(relyingPartyThumbprint);

                var decryptedToken = await XstsCore.DecryptPayloadAsync(encryptedToken, relyingPartyCert);

                var xblSignatureInfo = XstsCore.GetXblSignatureInfo(decryptedToken);

                if (xblSignatureInfo.SignatureType == XblSignatureType.XblSigningCert)
                {
                    //var signingCert = await XstsCore.DownloadXblSigningCert(decryptedToken);
                    //var certController2 = new XstsSigningCertController2();
                    var signingCert = await XstsCertController.GetXblSigningCert(decryptedToken);
                    clientToken = XstsCore.VerifyAndDeserializePayload(decryptedToken, signingCert);
                    clientToken.UserHash = userHash;
                }
                else if (xblSignatureInfo.SignatureType == XblSignatureType.XblSigningKey)
                {
                    var keyController2 = new XstsSigningKeyController();
                    //var signingKey = await XstsCore.DownloadXblSigningKey(decryptedToken);
                    var signingKey = await XstsSigningKeyController.GetSigningKey(decryptedToken);
                    if (signingKey != null)
                    {
                        clientToken = XstsCore.VerifyAndDeserializePayload(encryptedToken, signingKey);
                        clientToken.UserHash = userHash;
                    }
                }
                else
                {
                    throw new Exception("Unknown XblSignatureType");
                }

                if (clientToken != null &&
                    !XstsCore.TokenLifetimeIsValid(clientToken))
                {
                    throw new Exception("Token lifetime is not valid");
                }
            }
            catch (Exception ex)
            {
                throw new Exception("Validation of XSTS Token Failed, see inner exception", ex);
            }

            return clientToken;
        }
    }
}
