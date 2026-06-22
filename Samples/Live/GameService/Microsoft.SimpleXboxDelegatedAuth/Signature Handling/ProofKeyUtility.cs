//-----------------------------------------------------------------------------
// ProofKeyUtility.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma warning disable IDE0063 // Use simple 'using' statement

using Microsoft.SimpleXboxSecureTokens;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Net.Http;
using System.Runtime.Serialization;
using System.Security.Cryptography;
using System.Text;

namespace Microsoft.SimpleXboxDelegatedAuth
{
    //  Proof keys used in request of an S-Token
    [DataContract]
    public class Ecc256ProofKey
    {
        [JsonProperty("alg", Order = 1)]
        public string Algorithm { get; set; }

        [JsonProperty("kty", Order = 1)]
        public string KeyType { get; set; }

        [JsonProperty("use", Order = 2)]
        public string Use { get; set; }

        [JsonProperty("crv", Order = 3)]
        public string CurveType { get; set; }

        [JsonProperty("x", Order = 4)]
        public string X { get; set; }

        [JsonProperty("y", Order = 5)]
        public string Y { get; set; }

        public Ecc256ProofKey()
        {
        }

        public Ecc256ProofKey(Dictionary<string, string> values)
        {
            this.KeyType = values["kty"];
            this.Algorithm = "ES256";
            this.CurveType = values["crv"];
            this.X = values["x"];
            this.Y = values["y"];
            this.Use = "sig";
        }

        public Ecc256ProofKey(EccJsonWebKey Jwk)
        {
            this.KeyType = Jwk.KeyType;
            this.Algorithm = "ES256";
            this.CurveType = Jwk.CurveType;
            this.X = Jwk.XCoordinate;
            this.Y = Jwk.YCoordinate;
            this.Use = "sig";
        }

    }

    /// <summary>
    /// Useful helper function for handling and creating proof keys needed by the XSAS service
    /// </summary>
    public static class ProofKeyUtility
    {
        /// <summary>
        /// Create's a ECDsaP256 ProofKey and exports the Public/Private Key Pair to a byte array
        /// </summary>
        /// <returns>Byte Array which can be converted to an ECDsa instance</returns>
        public static byte[] CreateProofKey()
        {
            using (ECDsa key = Create())
            {
                return key.ExportParameters(true).ExportToByteArray();
            }
        }

        /// <summary>
        /// Creates a ECDsaP256 ProofKey
        /// </summary>
        /// <returns>ECDsa instance</returns>
        public static ECDsa Create()
        {
            //  On Windows you can directly pass the ECParameters into the ECDsa.Create()
            //  Function like so: Create(ECCurve.NamedCurves.nistP256);
            //
            //  HOWEVER, on Linux that will result in the following exception:
            //      The specified curve 'ecdsa_p256' or its parameters are not valid for 
            //      this platform.
            //  For more information on this issue see the following:
            //      https://github.com/dotnet/corefx/issues/32323
            //  
            //  To work around this on Linux we create the ECDsa from an Oid created from
            //  the friendly name rather than passing in the friendly name to ECDsa.Create()
            //  directly.  This code also works on Windows.
            var oid = Oid.FromFriendlyName(XstsConstants.ECDSAP256OidFriendlyName, OidGroup.All);
            ECCurve curve = ECCurve.CreateFromOid(oid);
            return Create(curve);
        }

        /// <summary>
        /// Creates a ECDsa instance with the specified curve
        /// </summary>
        /// <param name="Curve">ECCurve value</param>
        /// <returns>ECDsa instance</returns>
        public static ECDsa Create(ECCurve Curve)
        {
                return ECDsa.Create(Curve);
        }

        /// <summary>
        /// Generates the signature for the passed in HTTP request using the exported proofKey in bytes from the matching
        /// Delegated Auth X-token.
        /// </summary>
        /// <param name="ProofKeyInBytes"></param>
        /// <param name="Policy"></param>
        /// <param name="HttpRequest"></param>
        /// <returns></returns>
        public static string GenerateSignature(this byte[] ProofKeyInBytes,
                                               SignaturePolicy Policy,
                                               HttpRequestMessage HttpRequest)
        {
            // Serialize our request body to a UTF8 byte array
            byte[] requestBodyContent = null;
            if (HttpRequest.Method != HttpMethod.Get && HttpRequest.Content != null)
            {
                var requestBodyString = JsonConvert.SerializeObject(HttpRequest.Content.ToString());
                requestBodyContent = System.Text.Encoding.UTF8.GetBytes(requestBodyString);
            }

            var headers = new NameValueCollection();
            foreach (var header in HttpRequest.Headers)
            {
                headers.Add(header.Key, header.Value.ToString());
            }

            return GenerateSignature(ProofKeyInBytes,
                                     Policy,
                                     HttpRequest.RequestUri,
                                     HttpRequest.Method,
                                     requestBodyContent,
                                     headers);
        }

        /// <summary>
        /// Generates the signature for the passed in HTTP request parameters using the exported proofKey in bytes
        /// </summary>
        /// <param name="ProofKeyInBytes"></param>
        /// <param name="Policy"></param>
        /// <param name="RequestUri"></param>
        /// <param name="HttpMethod"></param>
        /// <param name="RequestBody"></param>
        /// <param name="Headers"></param>
        /// <returns>String value to be set as the Signature header of the HTTP request</returns>
        public static string GenerateSignature(this byte[] ProofKeyInBytes,
                                               SignaturePolicy Policy,
                                               Uri RequestUri,
                                               HttpMethod HttpMethod,
                                               byte[] RequestBody,
                                               NameValueCollection Headers)
        {
            //  If there is no body, we still need a blank array for the signature generation
            if (RequestBody == null)
            {
                RequestBody = new Byte[8];
                Array.Clear(RequestBody, 0, RequestBody.Length);
            }

            // perform applicable signature action
            var SigningProofKeyParameters = ProofKeyInBytes.ECParametersFromByteArray();
            
            using (var signingContext = new SigningContext(HashAlgorithmName.SHA256, SigningProofKeyParameters))
            {
                long timestmap = DateTime.UtcNow.ToFileTimeUtc();
                SignRequest(
                    signingContext,
                    Policy,
                    timestmap,
                    HttpMethod.ToString().ToUpperInvariant(),
                    RequestUri.GetComponents(UriComponents.PathAndQuery, UriFormat.SafeUnescaped),
                    Headers,
                    RequestBody,
                    0,
                    RequestBody.Length);

                return CreateSignatureHeader(signingContext.GetSignature(), Policy.Version, timestmap);
                    
            }
        }

        private static readonly long MaxFileTime = DateTime.MaxValue.ToFileTimeUtc();

        /// <summary>
        /// Creates the signature header value from the signature bytes, Policy version, and timestamp.
        /// </summary>
        /// <param name="signature">The signature.</param>
        /// <param name="version">The Policy version.</param>
        /// <param name="timestamp">The timestamp.</param>
        /// <returns>The signature header.</returns>
        public static string CreateSignatureHeader(byte[] signature, int version, long timestamp)
        {
            if (signature == null)
            {
                throw new ArgumentNullException("signature");
            }
            if (!IsValidFileTime(timestamp))
            {
                throw new ArgumentOutOfRangeException("timestamp", "Not a valid Windows file time.");
            }

            byte[] versionBytes = BitConverter.GetBytes(version);
            byte[] timestampBytes = BitConverter.GetBytes(timestamp);

            if (BitConverter.IsLittleEndian)
            {
                Array.Reverse(versionBytes);
                Array.Reverse(timestampBytes);
            }

            byte[] headerBytes = new byte[signature.Length + versionBytes.Length + timestampBytes.Length];
            Buffer.BlockCopy(versionBytes, 0, headerBytes, 0, versionBytes.Length);
            Buffer.BlockCopy(timestampBytes, 0, headerBytes, versionBytes.Length, timestampBytes.Length);
            Buffer.BlockCopy(signature, 0, headerBytes, versionBytes.Length + timestampBytes.Length, signature.Length);

            return Convert.ToBase64String(headerBytes);
        }

        /// <summary>
        /// Signs everything but the body. Note that even if there is no request body,
        /// a null byte still must be added. Use the other overload if you can load the
        /// request body into memory.
        /// </summary>
        /// <param name="context">The signing context.</param>
        /// <param name="policy">The signature Policy.</param>
        /// <param name="timestamp">The timestamp.</param>
        /// <param name="method">The HTTP method (verb).</param>
        /// <param name="pathAndQuery">The path and query string of the request URL.</param>
        /// <param name="headers">The request Headers.</param>
        public static void SignPrologue(
            SigningContext context,
            SignaturePolicy policy,
            long timestamp,
            string method,
            string pathAndQuery,
            NameValueCollection headers)
        {
            if (context == null)
            {
                throw new ArgumentNullException("context");
            }
            if (policy == null)
            {
                throw new ArgumentNullException("policy");
            }
            if (!IsValidFileTime(timestamp))
            {
                throw new ArgumentOutOfRangeException("timestamp", "Not a valid Windows file time.");
            }
            if (string.IsNullOrEmpty(method))
            {
                throw new ArgumentNullException("method");
            }
            if (pathAndQuery == null)
            {
                throw new ArgumentNullException("pathAndQuery");
            }
            if (headers == null)
            {
                throw new ArgumentNullException("headers");
            }

            context.SignVersion(policy.Version);
            context.SignTimestamp(timestamp);
            context.SignElement(method.ToUpperInvariant());
            context.SignElement(pathAndQuery);
            SignHeaders(context, headers, policy);
        }

        /// <summary>
        /// Signs the entire request.
        /// </summary>
        /// <param name="context">The signing context.</param>
        /// <param name="policy">The signature Policy.</param>
        /// <param name="timestamp">The timestamp.</param>
        /// <param name="method">The HTTP method (verb).</param>
        /// <param name="pathAndQuery">The path and query string of the request URL.</param>
        /// <param name="headers">The request Headers.</param>
        /// <param name="body">The buffer containing the request body.</param>
        /// <param name="index">An offset into the buffer marking the start of request body.</param>
        /// <param name="count">The length in bytes of the request body in the buffer.</param>
        public static void SignRequest(
            SigningContext context,
            SignaturePolicy policy,
            long timestamp,
            string method,
            string pathAndQuery,
            NameValueCollection headers,
            byte[] body,
            int index,
            int count)
        {
            if (context == null)
            {
                throw new ArgumentNullException("context");
            }
            if (policy == null)
            {
                throw new ArgumentNullException("policy");
            }
            if (!IsValidFileTime(timestamp))
            {
                throw new ArgumentOutOfRangeException("timestamp", "Not a valid Windows file time.");
            }
            if (string.IsNullOrEmpty(method))
            {
                throw new ArgumentNullException("method");
            }
            if (pathAndQuery == null)
            {
                throw new ArgumentNullException("pathAndQuery");
            }
            if (headers == null)
            {
                throw new ArgumentNullException("headers");
            }
            if (body == null)
            {
                throw new ArgumentNullException("body");
            }

            SignPrologue(
                context,
                policy,
                timestamp,
                method,
                pathAndQuery,
                headers);

            int numBytes = (int)Math.Min(count, policy.MaxBodyBytes);
            context.AddBytes(body, index, numBytes);
            context.AddNullByte();
        }

        /// <summary>
        /// Adds the Headers to the signature calculation according to the 
        /// signature Policy.
        /// </summary>
        /// <param name="context">The signing context.</param>
        /// <param name="headers">The collection containing the request Headers.</param>
        /// <param name="policy">The signature Policy.</param>
        public static void SignHeaders(SigningContext context, NameValueCollection headers, SignaturePolicy policy)
        {
            if (context == null)
            {
                throw new ArgumentNullException("context");
            }
            if (policy == null)
            {
                throw new ArgumentNullException("policy");
            }
            if (headers == null)
            {
                throw new ArgumentNullException("headers");
            }

            context.SignElement(headers["Authorization"] ?? string.Empty);

            if (policy.ExtraHeaders == null || policy.ExtraHeaders.Length == 0)
            {
                return;
            }

            foreach (string header in policy.ExtraHeaders)
            {
                // If the header isn't present we treat it as an
                // empty string so that the null byte gets added.

                string headerValue = headers[header] ?? string.Empty;
                context.SignElement(headerValue);
            }
        }

        /// <summary>
        /// Checks if the timestamp is a valid Windows file time.
        /// </summary>
        /// <param name="timestamp">The timestamp.</param>
        /// <returns>True if valid, otherwise false.</returns>
        public static bool IsValidFileTime(long timestamp)
        {
            return timestamp >= 0 && timestamp <= MaxFileTime;
        }

        public static HashAlgorithm CreateHashAlgorithm(this HashAlgorithmName hashAlg)
        {
            HashAlgorithm returnAlg = null;
            switch (hashAlg.Name)
            {
                case "SHA256":
                    returnAlg = SHA256.Create();
                    break;
                case "SHA384":
                    returnAlg = SHA384.Create();
                    break;
                case "SHA512":
                    returnAlg = SHA512.Create();
                    break;
                default:
                    throw new InvalidOperationException("Unsupported HashAlgorithm type");
            }

            return returnAlg;
        }

        /// <summary>
        /// Exports the ECParameters for a ProofKey to a byte array.  This allows us to keep
        /// the proofKey in a database with the S-Token as a byte[], then convert it back
        /// to an actionable ECParameters object with ECParametersFromByteArray().
        /// </summary>
        /// <param name="Parameters"></param>
        /// <returns>Byte array that can be converted back to ECParamaters object with ECParametersFromByteArray()</returns>
        public static byte[] ExportToByteArray(this ECParameters Parameters)
        {
            if (Parameters.Q.X == null || Parameters.Q.Y == null)
            {
                throw new InvalidOperationException("Invalid key");
            }

            //  Get the friendly name string so we can use it to re-construct with
            //  ECCurve.CreateFromFriendlyName
            byte[] oid = Encoding.ASCII.GetBytes(Parameters.Curve.Oid.FriendlyName);

            //  4 bytes (1 for each part size), length of each part (oid, D, Q.X, Q.Y)
            var blob = new byte[ 4                       
                               + oid.Length
                               + Parameters.D.Length
                               + Parameters.Q.X.Length
                               + Parameters.Q.Y.Length];
            int blobWriteOffset = 0;

            //  Curve Type
            blob[blobWriteOffset] = (byte)oid.Length;
            ++blobWriteOffset;
            Buffer.BlockCopy(oid, 0, blob, blobWriteOffset, oid.Length);
            blobWriteOffset += oid.Length;

            //  D
            blob[blobWriteOffset] = (byte)Parameters.D.Length;
            ++blobWriteOffset;
            Buffer.BlockCopy(Parameters.D, 0, blob, blobWriteOffset, Parameters.D.Length);
            blobWriteOffset += Parameters.D.Length;

            //  Q.X
            blob[blobWriteOffset] = (byte)Parameters.Q.X.Length;
            ++blobWriteOffset;
            Buffer.BlockCopy(Parameters.Q.X, 0, blob, blobWriteOffset, Parameters.Q.X.Length);
            blobWriteOffset += Parameters.Q.X.Length;

            //  Q.Y
            blob[blobWriteOffset] = (byte)Parameters.Q.Y.Length;
            ++blobWriteOffset;
            Buffer.BlockCopy(Parameters.Q.Y, 0, blob, blobWriteOffset, Parameters.Q.Y.Length);
            //blobWriteOffset += Parameters.Q.Y.Length;

            return blob;
        }

        /// <summary>
        /// Converts a byte[] from ExportToByteArray used to store in a database back to an 
        /// ECParameters object that can be used for creating signatures.
        /// </summary>
        /// <param name="ByteBlob"></param>
        /// <returns>ECParameters that can be used for creating signatures</returns>
        public static ECParameters ECParametersFromByteArray(this byte[] ByteBlob)
        {
            if (ByteBlob == null)
            {
                throw new InvalidOperationException("ByteBlob");
            }

            ECParameters returnParameters = new ECParameters();
            int blobReadOffset = 0;

            //  Curve Type
            int partLength = ByteBlob[blobReadOffset];
            ++blobReadOffset;
            var curveTypeBlob = new byte[partLength];
            Buffer.BlockCopy(ByteBlob, blobReadOffset, curveTypeBlob, 0, partLength);
            blobReadOffset += partLength;
            string curveType = Encoding.ASCII.GetString(curveTypeBlob);
            returnParameters.Curve = ECCurve.CreateFromFriendlyName(curveType);

            //  D
            partLength = ByteBlob[blobReadOffset];
            ++blobReadOffset;
            var d = new byte[partLength];
            Buffer.BlockCopy(ByteBlob, blobReadOffset, d, 0, partLength);
            blobReadOffset += partLength;
            returnParameters.D = d;

            //  Q.X
            partLength = ByteBlob[blobReadOffset];
            ++blobReadOffset;
            var x = new byte[partLength];
            Buffer.BlockCopy(ByteBlob, blobReadOffset, x, 0, partLength);
            blobReadOffset += partLength;
            returnParameters.Q.X = x;

            //  Q.Y
            partLength = ByteBlob[blobReadOffset];
            ++blobReadOffset;
            var y = new byte[partLength];
            Buffer.BlockCopy(ByteBlob, blobReadOffset, y, 0, partLength);
            //blobReadOffset += partLength;
            returnParameters.Q.Y = y;

            return returnParameters;
        }
    }
}
#pragma warning restore IDE0063 // Use simple 'using' statement