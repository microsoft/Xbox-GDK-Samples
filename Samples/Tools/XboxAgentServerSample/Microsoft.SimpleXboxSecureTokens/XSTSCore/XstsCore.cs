//-----------------------------------------------------------------------------
// XstsCore.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Jose;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading.Tasks;


namespace Microsoft.SimpleXboxSecureTokens
{
    public static class XstsCore
    {
        /// <summary>
        /// Can be overridden with an HttpClientFactory.CreateClient() if used by your service.
        /// This will greatly help with server performance.  To do this add the following to your
        /// Setup.cs or Program.cs
        ///
        /// var httpClientFactory = app.Services.GetService<System.Net.Http.IHttpClientFactory>();
        /// XstsCore.CreateHttpClientFunc = httpClientFactory.CreateClient;
        ///
        /// Warning, not assigning an HttpClientFactor can cause server performance issues generating
        /// lots of unmanaged HttpClients and is not recommended.
        /// </summary>
        public static Func<HttpClient> CreateHttpClientFunc = () => new HttpClient();

        public static string[] GetTokenParts(string xToken)
        {
            //  The 5 TokenParts of the token:
            //  [0] - JWE Protected Header       - This is a UTF8 byte array turned into a
            //                                     Base64URL string.  You need to decode 
            //                                     this to get info about which x5t was used
            //                                     to encrypt the Content Encryption Key.
            //  [1] - JWE Encrypted Key          - Content Encryption Key, this is a byte 
            //                                     array that is the generated secret key
            //                                     used to encrypt the payload turned into a
            //                                     Base64URL string.  This key is encrypted with
            //                                     the Relying Party's public key and decrypted
            //                                     with the RP's private key.
            //  [2] - JWE Initialization Vector  - Used for the AES decryptor to operate 
            //                                     on the cipher text
            //  [3] - JWE Ciphertext             - This is the content encrypted with the 
            //                                     Content Encryption Key
            //  [4] - JWE Authentication Tag     - Integrity value 
            return xToken.Split('.');
        }

        public static string GetRelyingPartyThumbprint(string encryptedToken)
        {
            string x5t = "";

            var tokenParts = GetTokenParts(encryptedToken);

            //  [0] The first part of the JWE tells us what cert to use for decryption
            //      and Additional Authenticated Data used for validation later below
            var xtokenHeader = tokenParts[0];

            //  decode the header
            string header = Encoding.UTF8.GetString(Base64Url.Decode(xtokenHeader));

            if (header != null)
            {
                //  create a dictionary out of the header
                Dictionary<string, string>? headerDict = JsonConvert.DeserializeObject<Dictionary<string, string>>(
                    header,
                    new JsonSerializerSettings
                    {
                        MaxDepth = 2,
                        TypeNameHandling = TypeNameHandling.None
                    });

                if(headerDict == null)
                {
                    var message = "Unable to create dictionary from the header to get the relying party thumbprint: " + header;
                    throw new Exception(message);
                }

                //  the header tells you which cert to use to decrypt the Content Encryption Key
                //  via the x5t (Base64Url). You need to decode the x5t to a byte array and 
                //  re-encode it to a hex thumbprint
                x5t = headerDict["x5t"];
                if (!string.IsNullOrEmpty(x5t))
                {
                    x5t = BitConverter.ToString(Base64Url.Decode(x5t)).Replace("-", string.Empty);
                }
            }
            return x5t;
        }

        public static async Task<string> DecryptPayloadAsync(string encryptedToken,
                                                 X509Certificate2 relyingPartyCert)
        {
            return await DecryptPayloadAsync(GetTokenParts(encryptedToken), relyingPartyCert);
        }

        public static async Task<string> DecryptPayloadAsync(string[] tokenParts,
                                                 X509Certificate2 relyingPartyCert)
        {
            if(relyingPartyCert == null)
            {
                throw new ArgumentNullException(nameof(relyingPartyCert));
            }

            //  [1] Decrypt (unwrap) the Content Encryption Key / Secret Key
            //      that was used to encrypt the payload
            var privateKey = relyingPartyCert.GetRSAPrivateKey();
            if (privateKey == null)
            {
                var message = "Private key for " + relyingPartyCert.FriendlyName + "unavailable.";
                throw new Exception(message);
            }

            byte[] cek = privateKey.Decrypt(XstsUtilities.FromBase64Url(tokenParts[1]),
                                                                       RSAEncryptionPadding.OaepSHA1);

            //  Extract the AES key from the Content Encryption Key
            byte[][] keys = XstsUtilities.SplitSecretKey(cek);
            byte[] hmacKey = keys[0];
            byte[] aesKey = keys[1];

            //  [2] Get and decode the Initialization Vector
            byte[] iv = XstsUtilities.FromBase64Url(tokenParts[2]);

            //  [3] Decode the content / cipher text that will be decrypted
            //      Our custom FromBase64Url includes the needed padding
            byte[] encryptedContent = XstsUtilities.FromBase64Url(tokenParts[3]);

            //  [4] Verify the authentication tag from the info in the header
            //      Before doing extra work on the token.
            byte[] authTag = XstsUtilities.FromBase64Url(tokenParts[4]);
            byte[] authData = Encoding.ASCII.GetBytes(tokenParts[0]);

            XstsUtilities.VerifyAuthenticationTag(authData, iv, encryptedContent, hmacKey, authTag);

            //  Now that we have verified the outer token's integrity, we can work on
            //  decrypting the payload to get the inner token that has the claims

            //  Decrypt the payload using the AES + IV
            byte[] decryptedContent = await XstsUtilities.DecryptAsync(encryptedContent, aesKey, iv);
            string encodedJwt = await XstsUtilities.DecompressAsync(decryptedContent);

            //  You can copy the encodedJwt and paste it into the
            //  tool at https://jwt.io/ to decode the token and
            //  view its TokenParts

            return encodedJwt;
        }

        public static XblSignatureInfo GetXblSignatureInfo(string encodedJwt)
        {
            var signatureInfo = new XblSignatureInfo()
            {
                //  default values
                Id = "",
                SignatureType = XblSignatureType.Unknown
            };

            var tokenHeaders = Jose.JWT.Headers(encodedJwt);

            if (tokenHeaders.ContainsKey("x5t"))
            {
                signatureInfo.SignatureType = XblSignatureType.XblSigningCert;

                //  Get the x5t (thumbprint) so we know which certificate was used to sign the token.
                string x5tEncoded = (string)tokenHeaders["x5t"];
                string x5tThumbprint = BitConverter.ToString(Base64Url.Decode(x5tEncoded)).Replace("-", string.Empty);
                signatureInfo.Id = x5tThumbprint;
            }
            else if (tokenHeaders.ContainsKey("jku"))
            {
                signatureInfo.SignatureType = XblSignatureType.XblSigningKey;
            }

            return signatureInfo;
        }

        public static XstsClientToken VerifyAndDeserializePayload( string encodedJwt,
                                                                   X509Certificate2 signatureCert)
        {
            var serializedJsonClaims = Jose.JWT.Decode(encodedJwt, signatureCert.GetRSAPublicKey(), Jose.JwsAlgorithm.RS256);

            //  Token is authentic, now deserialize the JSON payload into our defined XstsClientToken class
            XstsClientToken? clientTokenClaims = JsonConvert.DeserializeObject<XstsClientToken>(serializedJsonClaims);

            if (clientTokenClaims == null ||
               !TokenLifetimeIsValid(clientTokenClaims))
            {
                var message = "ClientToken is invalid or expired";
                throw new Exception(message);
            }

            return clientTokenClaims;
        }

        public static XstsClientToken VerifyAndDeserializePayload( string encodedJwt,
                                                                   XstsCachedSigningKey cachedSigningKey)
        {
            //  JKW signing key
            var serializedJsonClaims = Jose.JWT.Decode(encodedJwt, cachedSigningKey.Key, cachedSigningKey.Algorithm);

            //  Token is authentic, now deserialize the JSON payload into our defined XstsClientToken class
            XstsClientToken? clientTokenClaims = JsonConvert.DeserializeObject<XstsClientToken>(serializedJsonClaims);

            if( clientTokenClaims == null ||
                !TokenLifetimeIsValid(clientTokenClaims))
            {
                var message = "ClientToken is invalid or expired";
                throw new Exception(message);
            }

            return clientTokenClaims;
        }

        public static XstsClientToken VerifyAndDeserializePayload( string encodedJwt,
                                                                   XstsSigningKey signingKey)
        {
            //  Convert the raw signing key to the simplified version that can be used with Jose.JWT.Decode
            var simplifiedSigningKey = new XstsCachedSigningKey(signingKey);

            return VerifyAndDeserializePayload(encodedJwt, simplifiedSigningKey);
        }

        public static X509Certificate2 GetCertFromCurrentUserStore(string thumbprint)
        {
            X509Certificate2 targetCert;

            var certStore = new X509Store(StoreName.My, StoreLocation.CurrentUser);
            certStore.Open(OpenFlags.ReadOnly);
            X509Certificate2Collection certCollection = certStore.Certificates.Find(
                                        X509FindType.FindByThumbprint,
                                        thumbprint,
                                        false);

            certStore.Close();

            // Get the first cert with the thumbprint
            if (certCollection.Count > 0)
            {
                targetCert = certCollection[0];
            }
            else
            {
                //  unable to find the RP cert
                string message = "Unable to find cert in store for thumbprint - " + thumbprint;
                var e = new Exception(message);
                throw e;
            }

            return targetCert;
        }

        public static async Task<X509Certificate2> DownloadXblSigningCert(string encodedJwt)
        {
            var tokenHeaders = Jose.JWT.Headers(encodedJwt);

            //  Get the x5t (thumbprint) and check if we already have this cert in our cache.
            //  The header gives us info on the cert used to sign the token.
            string x5t = (string)tokenHeaders["x5t"];
            string x5u = (string)tokenHeaders["x5u"];
            string x5tThumbprint = BitConverter.ToString(Base64Url.Decode(x5t)).Replace("-", string.Empty);

            string certData;

            //  Verify that this cert is being downloaded and came from the trusted
            //  host.
            var x5uUri = new Uri(x5u);
            if (x5uUri.Host.Equals("xsts.auth.xboxlive.com"))
            {
                //  This is an XSTS signing cert
                string responseBody;

                var httpClient = CreateHttpClientFunc();
                using (HttpResponseMessage response = await httpClient.GetAsync(x5u))
                {
                    responseBody = await response.Content.ReadAsStringAsync();
                }

                var deserializedResult = JsonConvert.DeserializeObject<XstsSigningCertResponse>(responseBody);

                if (deserializedResult != null)
                {
                    try
                    {
                        certData = deserializedResult.signingCerts[x5t].ToString();
                    }
                    catch (Exception e)
                    {
                        //  This means that the x5t does not match the value that we got back
                        var ex = new Exception(string.Format("Cert downloaded from x5u ({0}) does not match the x5t requested ({1})", x5u, x5t), e);
                        throw ex;
                    }

                    //  Now we need to trim off the begin and end tags
                    certData = certData.Replace("-----BEGIN CERTIFICATE-----\n", "");
                    certData = certData.Replace("\n-----END CERTIFICATE-----\n", "");
                }
                else
                {
                    var message = "Invalid response: {0}" + responseBody;
                    throw new Exception(message);
                }
            }
            else
            {
                var message = "Invalid x5u host that is not trusted: " + x5u;
                throw new Exception(message);
            }

            var signingCert = new X509Certificate2(Convert.FromBase64String(certData));

            return signingCert;
        }

        public static async Task<XstsSigningKey> DownloadXblSigningKey(string encodedJwt)
        {
            var tokenHeaders = Jose.JWT.Headers(encodedJwt);

            //  Get the kid and jku
            string signingKeyId = (string)tokenHeaders["kid"];
            string jku = (string)tokenHeaders["jku"];

            var allXblSigningKeys = await DownloadXblSigningKeys(jku);
            XstsSigningKey? targetKey = null;

            //  Look for our specific target key based on the kid, cache, and then return it
            foreach (var signingKey in allXblSigningKeys)
            {
                if (signingKey.KeyId == signingKeyId)
                {
                    targetKey = signingKey;
                    break;
                }
            }

            if (targetKey == null)
            {
                var message = "Unable to find key for kid: " + signingKeyId + " at " + jku;
                throw new Exception(message);
            }

            return targetKey;
        }

        public static async Task<List<XstsSigningKey>> DownloadXblSigningKeys(string jku)
        {
            //  Verify that this cert is being downloaded and came from the trusted
            //  host.
            var jkuUri = new Uri(jku);
            if (!jkuUri.Host.Equals("xsts-keys.auth.xboxlive.com"))
            {
                //  This is not a valid XSTS signing key URL
                throw new ArgumentException("Invalid jku host that is not trusted: {0}", jku);
            }

            var httpClient = CreateHttpClientFunc();

            var signingKeys = new XstsSigningKeysResponse();

            using (HttpResponseMessage response = await httpClient.GetAsync(jku))
            {
                var responseBody = await response.Content.ReadAsStringAsync();
                var converted = JsonConvert.DeserializeObject<XstsSigningKeysResponse>(responseBody);
                if (converted != null)
                {
                    signingKeys = converted;
                }
            }

            return signingKeys.Keys;
        }

        public static string GetXTokenFromHeader(string authHeader)
        {
            if (string.IsNullOrEmpty(authHeader)) { return ""; }

            //  We have an Authorization header, we need to know where the user hash  
            //  and token start in the string to extract them.
            //  Example: Authorization: "XBL3.0 x=[ush];[JWT with encrypted payload]"
            int tokenStartPos = authHeader.IndexOf(';') + 1;

            var token = authHeader.Substring(tokenStartPos);

            return token;
        }

        public static string GetUserHashFromHeader(string authHeader)
        {
            if (string.IsNullOrEmpty(authHeader)) { return ""; }

            string ush;

            //  We have an Authorization header, we need t0 know where the user hash  
            //  and token start in the string to extract them.  Example:
            //      Authorization: "XBL3.0 x=[ush];[JWT with encrypted payload]"
            int ushStartPos = authHeader.IndexOf('=') + 1;
            int tokenStartPos = authHeader.IndexOf(';') + 1;

            ush = authHeader.Substring(ushStartPos, (tokenStartPos - ushStartPos) - 1);
            return ush;
        }

        public static async Task<XstsClientToken> ValidateAuthorizationHeader(string authHeader)
        {
            XstsClientToken clientToken;
            try
            {
                string userHash = GetUserHashFromHeader(authHeader);
                string encryptedToken = GetXTokenFromHeader(authHeader);

                string relyingPartyThumbprint = GetRelyingPartyThumbprint(encryptedToken);
                //var relyingPartyCert = GetCertFromCurrentUserStore(relyingPartyThumbprint);

                var relyingPartyCert = await XstsCertController.GetXblSigningCert(relyingPartyThumbprint);
                var decryptedToken = await DecryptPayloadAsync(encryptedToken, relyingPartyCert);

                var xblSignatureInfo = GetXblSignatureInfo(decryptedToken);

                if (xblSignatureInfo.SignatureType == XblSignatureType.XblSigningCert)
                {
                    var signingCert = await DownloadXblSigningCert(decryptedToken);
                    clientToken = VerifyAndDeserializePayload(decryptedToken, signingCert);
                    clientToken.UserHash = userHash;
                }
                else if (xblSignatureInfo.SignatureType == XblSignatureType.XblSigningKey)
                {
                    var signingKey = await DownloadXblSigningKey(decryptedToken);
                    clientToken = VerifyAndDeserializePayload(encryptedToken, signingKey);
                    clientToken.UserHash = userHash;
                }
                else
                {
                    throw new Exception("Unknown XblSignatureType");
                }

                if(!TokenLifetimeIsValid(clientToken))
                {
                    throw new Exception("Token lifetime is not valid");
                }

            }
            catch (Exception ex)
            {
                throw new Exception("Unable to handle Xsts Token", ex);
            }

            return clientToken;
        }

        public static bool TokenLifetimeIsValid(XstsClientToken clientToken)
        {
            //  Token decrypted properly and is authentic, check that its lifetime is valid and not expired
            //  Note: XSTS claims are formatted as Unix Epoch seconds, compare them against current UTC
            var currentUTC = DateTime.Now.ToUniversalTime();
            var tokenLifetimeIsValid = false;

            if (DateTime.Compare(clientToken.NotBefore, currentUTC) <= 0)
            {
                if (DateTime.Compare(clientToken.Expires, currentUTC) >= 0)
                {
                    //  Token is authentic and active, add the claims object to the request's Items member.
                    //  This allows the API controllers (or subsequent delegates in the flow) to access it.  
                    tokenLifetimeIsValid = true;
                }
                else
                {
                    //  This token's expire time is less than right now so it is expired.  
                }
            }
            else
            {
                //  This token's Not Before time is greater than right now, so it is valid for the future but not yet
            }

            return tokenLifetimeIsValid;
        }
    }
}
