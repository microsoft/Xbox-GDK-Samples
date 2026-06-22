//-----------------------------------------------------------------------------
// XstsTokenHandler.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Microsoft.Extensions.Caching.Memory;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading.Tasks;
using static Microsoft.SimpleXboxSecureTokens.XstsConstants;

namespace Microsoft.SimpleXboxSecureTokens
{
    public class XstsTokenHandler
    {
        /// <summary>
        /// This class requires your service to have an in-memory cache so that the
        /// signing keys can be cached and re-used rather than downloaded with each
        /// incoming XSTS Token.
        /// </summary>
        public IMemoryCache ServerCache;

        /// <summary>
        /// Can be overridden with an HttpClientFactory.CreateClient if used by your service.
        /// This will greatly help with server performance.  To do this add the following to your
        /// Setup.cs or Program.cs
        ///
        /// var httpClientFactory = app.Services.GetService<System.Net.Http.IHttpClientFactory>();
        /// XstsSigningKeyController.CreateHttpClientFunc = httpClientFactory.CreateClient;
        ///
        /// </summary>
        public static Func<HttpClient> CreateHttpClientFunc = () => new HttpClient();

        public XstsTokenHandler(IMemoryCache serverCache)
        {
            ServerCache = serverCache;
        }

        /// <summary>
        /// Validates, decrypts, and provides an XstsClientToken of the XSTS token provided in an https authorization header
        /// </summary>
        /// <param name="httpsAuthHeader">Full auth header from the incoming https call</param>
        /// <returns>XstsClientToken containing the claims of the token in the auth header</returns>
        /// <exception cref="ArgumentNullException"></exception>
        /// <exception cref="Exception"></exception>
        public async Task<XstsClientToken?> ValidateAuthorizationHeaderWithCache(string? httpsAuthHeader)
        {
            if (string.IsNullOrEmpty(httpsAuthHeader))
            {
                throw new ArgumentNullException(nameof(httpsAuthHeader));
            }

            XstsClientToken? clientToken = null;

            try
            {
                //  [0] - User Hash
                //  [1] - Xsts Token / Encrypted token
                var authHeaderParts = ParseHttpsAuthHeader(httpsAuthHeader);

                if(authHeaderParts.Count != (int)XstsAuthHeaderParts.Length)
                {
                    throw new ArgumentException("httpsAuthHeader format is incorrect or unable to be parsed.  Example: \"XBL3.0 x=[ush];[JWT with encrypted payload]\"");
                }

                //  Get the headers from the X-token to find the x5t to lookup the Relying Party cert
                var xtokenHeaders = Jose.JWT.Headers(authHeaderParts[(int)XstsAuthHeaderParts.XstsToken]);

                if(xtokenHeaders == null)
                {
                    throw new Exception("Unable to retrieve xtokenHeaders");
                }

                string x5t = (string)xtokenHeaders["x5t"];
                var rpCertManager = new XstsCertController(ServerCache);
                var relyingPartyCert = rpCertManager.GetCachedCertWithX5t(x5t);

                //  Decrypt the payload to get the inner-token / jwt
                var decryptedToken = await DecryptPayloadAsync(authHeaderParts[(int)XstsAuthHeaderParts.XstsToken], relyingPartyCert);

                //  Verify and deserialize the inner-token to get the claims
                clientToken = await VerifyAndDeserializePayload(decryptedToken);

                //  Assign the userHash from the https auth header to the clientToken for later use
                clientToken.UserHash = authHeaderParts[(int)XstsAuthHeaderParts.UserHash];
            }
            catch (Exception ex)
            {
                throw new Exception("Validation of XSTS Token Failed, see inner exception", ex);
            }

            return clientToken;
        }

        /// <summary>
        /// This will verify the outer token and then do the needed work to decrypt the payload
        /// to get the inner token / jwt.
        /// </summary>
        /// <param name="encryptedToken">The X-token that was in the https Authorization header</param>
        /// <param name="relyingPartyCert">The Relying Party cert that matches the x5t in the X-token 
        /// header. Access to the private key is required</param>
        /// <returns></returns>
        /// <exception cref="ArgumentNullException">Exception thrown if no RelyingParty cert is provided</exception>
        /// <exception cref="Exception">Exception thrown if we don't have access to the Relying Party private key</exception>
        public async Task<string> DecryptPayloadAsync(string encryptedToken,
                                                      X509Certificate2 relyingPartyCert)
        {
            if (relyingPartyCert == null)
            {
                throw new ArgumentNullException(nameof(relyingPartyCert));
            }

            var tokenParts = encryptedToken.Split('.');
            //  The 5 TokenParts:
            //  [0] - JWE Protected Header       - A UTF8 byte array turned into a
            //  XstsTokenParts.Header              Base64URL string. Decode this to get
            //                                     info about which x5t was used to
            //                                     encrypt the Content Encryption Key.
            //
            //  [1] - JWE Encrypted Key          - Content Encryption Key, a byte array
            //  XstsTokenParts.ContentEncryptionKey turned into a Base64URL of the
            //                                     secret key generated and used to
            //                                     encrypt the payload. This key is
            //                                     encrypted with the Relying Party's
            //                                     public key and can be decrypted
            //                                     with the RP's private key.
            //
            //  [2] - JWE Initialization Vector  - Used for the AES decryptor to operate 
            //  XstsTokenParts.InitializationVector on the cipher text.
            //
            //  [3] - JWE Ciphertext / Payload   - This is the content encrypted with the 
            //  XstsTokenParts.Payload             Content Encryption Key representing the
            //                                     Payload or inner-token.
            //
            //  [4] - JWE Authentication Tag     - Integrity value for the outer token
            //  XstsTokenParts.AuthenticationTag   to verify it hasn't been tampered with.


            //  Verify we have access to the Relying Party's private key
            var privateKey = relyingPartyCert.GetRSAPrivateKey();
            if (privateKey == null)
            {
                var message = "Private key for " + relyingPartyCert.FriendlyName + "unavailable.";
                throw new Exception(message);
            }

            //  [1] Decrypt (unwrap) the Content Encryption Key / Secret Key
            //      that was used to encrypt the payload
            byte[] cek = privateKey.Decrypt(XstsUtilities.FromBase64Url(tokenParts[(int)XstsTokenParts.ContentEncryptionKey]),
                                                                        RSAEncryptionPadding.OaepSHA1);

            //  Extract the AES key from the Content Encryption Key
            byte[][] keys = XstsUtilities.SplitSecretKey(cek);
            byte[] hmacKey = keys[(int)XstsContentEncryptionKeyParts.HmacKey];
            byte[] aesKey = keys[(int)XstsContentEncryptionKeyParts.AesKey];

            //  [2] Get and decode the Initialization Vector
            byte[] iv = XstsUtilities.FromBase64Url(tokenParts[(int)XstsTokenParts.InitializationVector]);

            //  [3] Decode the content / cipher text that will be decrypted
            //      Our custom FromBase64Url includes the needed padding
            byte[] encryptedContent = XstsUtilities.FromBase64Url(tokenParts[(int)XstsTokenParts.Payload]);

            //  [4] Verify the authentication tag from the info in the header
            //      Before doing extra work on the token
            byte[] authTag = XstsUtilities.FromBase64Url(tokenParts[(int)XstsTokenParts.AuthenticationTag]);
            byte[] authData = Encoding.ASCII.GetBytes(tokenParts[(int)XstsTokenParts.Header]);

            XstsUtilities.VerifyAuthenticationTag(authData, iv, encryptedContent, hmacKey, authTag);

            //  Now that we have verified the outer token's integrity, we can work on
            //  decrypting the payload to get the inner token that has the claims

            //  Decrypt the payload using the AES + IV
            byte[] decryptedPayloadBytes = await XstsUtilities.DecryptAsync(encryptedContent, aesKey, iv);
            string decryptedPayload = await XstsUtilities.DecompressAsync(decryptedPayloadBytes);

            //  Tip: You can copy the decryptedPayload string and paste it into the
            //  tool at https://jwt.io/ to decode the token and view the claims

            return decryptedPayload;
        }

        /// <summary>
        /// Takes the inner-token (decrypted payload), verifies it is valid and signed by Xbox Live
        /// then returns a deserialized XstsClientToken object of the class.
        /// </summary>
        /// <param name="decryptedPayload">Inner token that is the decrypted payload of the outer token</param>
        /// <returns>Deserialized XstsClientToken object of the inner token</returns>
        /// <exception cref="Exception">Exception if the Xbl signing key for the inner-token is unable to be retrieved</exception>
        public async Task<XstsClientToken> VerifyAndDeserializePayload(string decryptedPayload)
        {
            var tokenHeaders = Jose.JWT.Headers(decryptedPayload);
            var keyController = new XstsSigningKeyController(ServerCache);
            var signingKey = await keyController.GetSigningKey((string)tokenHeaders["kid"], (string)tokenHeaders["jku"]);
            if (signingKey == null)
            {
                throw new Exception("Unable to retrieve the signing key.");
            }

            //  JKW signing key
            var serializedJsonClaims = Jose.JWT.Decode(decryptedPayload, signingKey.Key, signingKey.Algorithm);

            //  Token is authentic, now deserialize the JSON payload into our defined XstsClientToken class
            XstsClientToken? clientTokenClaims = JsonConvert.DeserializeObject<XstsClientToken>(serializedJsonClaims);

            if (clientTokenClaims == null)
            {
                throw new Exception("Payload token is invalid or unable to be parsed");
            }

            if (!TokenLifetimeIsValid(clientTokenClaims.NotBefore, clientTokenClaims.Expires))
            {
                var message = "ClientToken is invalid or expired";
                throw new Exception(message);
            }

            return clientTokenClaims;
        }

        /// <summary>
        /// Parses the https Authorization header for the userHash and X-token parts
        /// </summary>
        /// <param name="httpsAuthHeader"></param>
        /// <returns>String list of the userHash at [0] and the X-token at [1]</returns>
        /// <exception cref="ArgumentNullException">thrown if no authorization header is provided</exception>
        public static List<string> ParseHttpsAuthHeader(string httpsAuthHeader)
        {
            if (string.IsNullOrEmpty(httpsAuthHeader))
            {
                throw new ArgumentNullException(nameof(httpsAuthHeader)); 
            }

            var parsedHeaderParts = new List<string>();

            //  We have an Authorization header, we need to know where the user hash  
            //  and token start in the string to extract them.  Example:
            //      Authorization: "XBL3.0 x=[ush];[JWT with encrypted payload]"
            int ushStartPos = httpsAuthHeader.IndexOf('=') + 1;
            int tokenStartPos = httpsAuthHeader.IndexOf(';') + 1;

            //  User Hash value
            parsedHeaderParts.Add(httpsAuthHeader.Substring(ushStartPos, (tokenStartPos - ushStartPos) - 1));

            //  X-token
            parsedHeaderParts.Add(httpsAuthHeader.Substring(tokenStartPos));

            return parsedHeaderParts;
        }

        /// <summary>
        /// Providing the nbf and exp variables from the inner token, use the current UTC time to
        /// verify that the token is still valid or has expired.
        /// </summary>
        /// <param name="notBefore">nbf claim of the token</param>
        /// <param name="expires">exp claim of the token</param>
        /// <returns></returns>
        public static bool TokenLifetimeIsValid(DateTime notBefore, DateTime expires)
        {
            //  Token decrypted properly and is authentic, check that its lifetime is valid and not expired
            //  Note: XSTS claims are formatted as Unix Epoch seconds, compare them against current UTC
            var currentUTC = DateTime.Now.ToUniversalTime();
            var tokenLifetimeIsValid = false;

            if (DateTime.Compare(notBefore, currentUTC) <= 0)
            {
                if (DateTime.Compare(expires, currentUTC) >= 0)
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
