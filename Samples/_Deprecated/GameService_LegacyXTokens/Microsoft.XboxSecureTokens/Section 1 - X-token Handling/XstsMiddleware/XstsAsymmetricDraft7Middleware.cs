//-----------------------------------------------------------------------------
// XstsAsymmetricDraft7Middleware.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Jose;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Http;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading.Tasks;

namespace Microsoft.XboxSecureTokens
{
    // Extension method used to add the middleware to the HTTP request pipeline.
    public static class XstsAsymmetricDraft7MiddlewareExtensions
    {
        public static IApplicationBuilder UseXstsAsymmetricDraft7Middleware(this IApplicationBuilder builder)
        {
            return builder.UseMiddleware<XstsAsymmetricDraft7Middleware>();
        }
    }

    /// <summary>
    /// The purpose of this middleware is to validate an XSTS token using Asymmetric key auth and JWE Draft 7
    /// that was the default token type for titles in XDP for configuring their web services.
    /// For Partner Center configured titles we recommend using Asymmetric key auth and JWE RFC 7516 format.
    /// See the XstsSymRFCMiddleware class for handling those tokens.
    /// For information about Middleware in ASP.NET see the following
    /// https://docs.microsoft.com/en-us/aspnet/core/fundamentals/middleware/?view=aspnetcore-2.1&tabs=aspnetcore2x
    /// </summary>
    public class XstsAsymmetricDraft7Middleware
    {
        private readonly RequestDelegate mNext;
        private readonly IHttpClientFactory mHttpClientFactory;
        private ILogger mLogger;
        private Microsoft.CorrelationVector.CorrelationVector mCv;
        private readonly IMemoryCache mServerCache;
        private readonly IConfiguration mConfig;

        public XstsAsymmetricDraft7Middleware(IConfiguration Config,
                                              RequestDelegate next,
                                              IHttpClientFactory HttpClientFactory,
                                              ILogger<XstsAsymmetricDraft7Middleware> Logger,
                                              IMemoryCache ServerCache)
        {
            mConfig = Config;
            mServerCache = ServerCache;
            mNext = next;
            mHttpClientFactory = HttpClientFactory;
            mLogger = Logger;
        }

        private void InitializeLoggingCv(HttpContext httpContext)
        {
            //  This can't be set in the constructor because you only have an HttpContext
            //  during the actual service endpoint function.  So call this at the start
            //  of each incoming request.
            mCv = (Microsoft.CorrelationVector.CorrelationVector)httpContext.Items["MS-CV"];
        }

        /// <summary>
        /// This is the middleware entry point in the HTTP call processing
        /// </summary>
        /// <param name="httpContext"></param>
        /// <returns></returns>
        public async Task Invoke(HttpContext httpContext)
        {
            InitializeLoggingCv(httpContext);

            string XstsAuthHeader = httpContext.Request.Headers["Authorization"];
            if (!string.IsNullOrEmpty(XstsAuthHeader))
            {
                XstsClientToken clientToken;

                //  We have an Authorization header, we need t0 know where the user hash  
                //  and token start in the string to extract them.  Example:
                //      Authorization: "XBL3.0 x=[ush];[JWT with encrypted payload]"
                int ushStartPos = XstsAuthHeader.IndexOf('=') + 1;
                int tokenStartPos = XstsAuthHeader.IndexOf(';') + 1;

                string xToken = XstsAuthHeader.Substring(tokenStartPos);
                try
                {
                    clientToken = await ConsumeToken(xToken);
                    clientToken.UserHash = XstsAuthHeader.Substring(ushStartPos, (tokenStartPos - ushStartPos) - 1);
                }
                catch (Exception e)
                {
                    //  The token was not validated or had some other issue, deny the request
                    //  and close the connection.  For security, don't send any specific info
                    //  about why to the client.
                    mLogger.XstsException(mCv.Value, "XstsAsymmetricDraft7Middleware Error", e);
                    httpContext.Response.StatusCode = (int)HttpStatusCode.Forbidden;
                    await httpContext.Response.WriteAsync("Access Denied: XSTS token invalid");
                    return;
                }

                //  Token decrypted properly and is authentic, check that its lifetime is valid and not expired
                //  Note: XSTS claims are formatted as Unix Epoch seconds, compare them against current UTC
                var currentUTC = DateTime.Now.ToUniversalTime();

                if (DateTime.Compare(clientToken.NotBefore, currentUTC) <= 0)
                {
                    if (DateTime.Compare(clientToken.Expires, currentUTC) >= 0)
                    {
                        //  Token is authentic and active, add the claims object to the request's Items member.
                        //  This allows the API controllers (or subsequent delegates in the flow) to access it.  
                        httpContext.Items["XstsClaims"] = clientToken;

                        //  Go to the next delegate in the HTTP request pipeline
                        await mNext(httpContext);
                    }
                    else
                    {
                        //  This token's expire time is less than right now so it is expired.  
                        httpContext.Response.StatusCode = (int)HttpStatusCode.Unauthorized;
                        mLogger.XstsWarning(mCv.Value, "XstsAsymmetricDraft7Middleware - Token Expired");
                        await httpContext.Response.WriteAsync("Access Denied: XSTS token expired");
                    }
                }
                else
                {
                    //  This token's Not Before time is greater than right now, so it is valid for the future but not yet
                    httpContext.Response.StatusCode = (int)HttpStatusCode.Unauthorized;
                    await httpContext.Response.WriteAsync("Access Denied: XSTS token not valid yet");
                }
            }
            else
            {
                //  No authorization header, return a 403
                httpContext.Response.StatusCode = (int)HttpStatusCode.Forbidden;
                await httpContext.Response.WriteAsync("Access Denied: No auth header");
            }
            return;
        }

        /// <summary>
        /// Consumes and validates the token (Asymmetric Draft 7)  by decrypting the payload and extracting the claims
        /// </summary>
        /// <param name="Token"></param>
        /// <returns></returns>
        private async Task<XstsClientToken> ConsumeToken(string Token)
        {
            string[] TokenParts = Token.Split('.');
            //  The 5 TokenParts of the token:
            //  [0] - JWE Protected Header       - This is a UTF8 byte array turned into a
            //                                     Base64URL string.  You need to decode 
            //                                     this to get an AesKey that will specify
            //                                     the cert needed to decrypt the cipher text
            //  [1] - JWE Encrypted Key          - Content Master Key, this is a byte 
            //                                     array turned into a Base64URL string
            //  [2] - JWE Initialization Vector  - Used for the AES decryptor to operate 
            //                                     on the cipher text
            //  [3] - JWE Ciphertext             - This is the content encrypted with the 
            //                                     Relying Party certificate
            //  [4] - JWE Authentication Tag     - Integrity value

            //  [0] The first part of the JWE tells us what cert to use for decryption
            string encodedHeader = TokenParts[0];

            //  decode the header
            string header = Encoding.UTF8.GetString(Base64Url.Decode(encodedHeader));

            //  create a dictionary out of the header
            Dictionary<string, string> headerDict = JsonConvert.DeserializeObject<Dictionary<string, string>>(
                header, 
                new JsonSerializerSettings
                {
                    MaxDepth = 2,
                    TypeNameHandling = TypeNameHandling.None
                });

            //  the header tells you which cert to use to decrypt. x5t is Base64Url,
            //  you need to decode it to a byte array and re-encode it to a hex thumbprint
            string x5t = BitConverter.ToString(Base64Url.Decode(headerDict["x5t"])).Replace("-", string.Empty);

            //  get the Relying Party certificate to decrypt
            XstsCertController certController = new XstsCertController(mConfig, mServerCache, mLogger, mCv.Value);
            X509Certificate2 decryptCert = certController.GetCert(x5t);
            
            //  [1] Decrypt the Content Master Key
            byte[] cmk = ((RSA)decryptCert.GetRSAPrivateKey()).Decrypt(XstsUtilities.FromBase64Url(TokenParts[1]),
                                                                    RSAEncryptionPadding.OaepSHA1);

            //  Key Derivation Function - Derive the AES key from the Content Master Key
            //  Additional info on what is being done in this function:
            //  https://tools.ietf.org/html/draft-ietf-jose-json-web-algorithms-40#section-4.6.2 
            //  See section 5.8.1 Concatenation Key Derivation Function of the following:
            //  http://csrc.nist.gov/publications/nistpubs/800-56A/SP800-56A_Revision1_Mar08-2007.pdf  
            byte[] aesKey = XstsUtilities.ConcatKdf(cmk, "A128CBC+HS256", "Encryption", 128);

            //  [2] Decode the Initialization Vector
            byte[] iv = XstsUtilities.FromBase64Url(TokenParts[2]);

            //  [3] Decode the content / cipher text that will be decrypted
            //  Our custom FromBase64Url includes the needed padding
            byte[] encryptedContent = XstsUtilities.FromBase64Url(TokenParts[3]);

            //  Decrypt the payload using the CMK + IV
            byte[] decryptedContent = await XstsUtilities.DecryptAsync(encryptedContent, aesKey, iv);
            string encodedJwt = await XstsUtilities.DecompressAsync(decryptedContent);

            //  You can copy the encodedJwt and paste it into the
            //  tool at https://jwt.io/ to decode the token and
            //  view its TokenParts

            //  Validate the signature of the token. This ensures that the token 
            //  came from Xbox Live and that the integrity was not compromised
            //  by an attacker who has obtained your private key.
            string serializedJsonClaims = "";
            XstsSigningCertController signingCertController = new XstsSigningCertController(mConfig, mHttpClientFactory, mServerCache, mLogger, mCv.Increment());
            X509Certificate2 signatureCert = await signingCertController.GetSigningCert(Jose.JWT.Headers(encodedJwt));
            try
            { 
                 serializedJsonClaims = Jose.JWT.Decode(encodedJwt, signatureCert.GetRSAPublicKey(), Jose.JwsAlgorithm.RS256);
            }
            catch (Exception e)
            {
                mLogger.XstsException(mCv.Value, "Signature invalid or unable to calculate", e);
                throw;
            }

            //  Token is authentic, now deserialize the JSON payload into our defined XstsClientToken class
            XstsClientToken clientTokenClaims = JsonConvert.DeserializeObject<XstsClientToken>(serializedJsonClaims);

            return clientTokenClaims;
        }
    }
}
