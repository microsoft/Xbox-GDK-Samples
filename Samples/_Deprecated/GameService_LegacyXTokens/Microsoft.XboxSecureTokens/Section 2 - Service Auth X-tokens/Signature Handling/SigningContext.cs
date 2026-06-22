//-----------------------------------------------------------------------------
// SigningContext.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
using System;
using System.Security.Cryptography;
using System.Text;

namespace Microsoft.XboxSecureTokens.XstsDelegatedAuth
{
    /// SECTION 2 - Generates and validates signatures on tokens
    /// SECTION 1 - Generates and validates signatures on tokens
    /// <summary>
    /// An abstraction to provide a consistent interface for the different 
    /// signing algorithms. This class can be used for both creating and
    /// verifying signatures.
    /// </summary>
    public class SigningContext : IDisposable
    {
        private static readonly byte[] finalBlock = new byte[0];
        private static readonly byte[] nullByte = new byte[] { 0 };

        private readonly Func<byte[], byte[], bool> verifyHash;
        private readonly Func<byte[], byte[]> signHash;
        private readonly HashAlgorithm hashAlg;
        private readonly AsymmetricAlgorithm cryptoAlg;

        /// <summary>
        /// Creates a signing context using RSA and the supplied hashing algorithm.
        /// </summary>
        /// <param name="hashAlg">The hashing algorithm to use. This should be SHA256.</param>
        /// <param name="rsaAlg">The RSA provider.</param>
        public SigningContext(HashAlgorithmName hashAlg, RSACryptoServiceProvider rsaAlg)
        {
            if (rsaAlg == null)
            {
                throw new ArgumentNullException(nameof(rsaAlg));
            }

            //hashAlg.ValidateHashAlgorithmName();

            this.verifyHash = (hash, sig) => rsaAlg.VerifyHash(hash, sig, hashAlg, RSASignaturePadding.Pkcs1);
            this.signHash = (hash) => rsaAlg.SignHash(hash, hashAlg, RSASignaturePadding.Pkcs1);

            this.cryptoAlg = rsaAlg;
            this.hashAlg = hashAlg.CreateHashAlgorithm();
        }

        /// <summary>
        /// Creates a signing context using ECC and the supplied hashing algorithm.
        /// </summary>
        /// <param name="hashAlg">The hashing algorithm to use. This should be SHA256.</param>
        /// <param name="eccAlg">The ECC provider.</param>
        public SigningContext(HashAlgorithmName hashAlg, ECParameters Parameters)
        {
            //  On Windows you can directly pass the ECParameters into the ECDsa.Create()
            //  Function.  
            //  
            //  HOWEVER, on Linux that will result in the following exception:
            //      The specified curve 'ecdsa_p256' or its parameters are not valid for 
            //      this platform.
            //  For more information on this issue see the following:
            //      https://github.com/dotnet/corefx/issues/32323
            //  
            //  To work around this on Linux we create a new ECParameters object using
            //  a curve friendly name that will work on ASP.NET Core on Linux.  This code
            //  works just fine on Windows as well, but is not needed.
            var oid = Oid.FromFriendlyName(Parameters.Curve.Oid.FriendlyName, OidGroup.All);
            var linuxCompatibleParameters = new ECParameters
            {
                Curve = ECCurve.CreateFromOid(oid),
                D = Parameters.D,
                Q = Parameters.Q
            };

            var eccAlg = ECDsa.Create(linuxCompatibleParameters);

            this.hashAlg = hashAlg.CreateHashAlgorithm();
            this.verifyHash = eccAlg.VerifyHash;
            this.signHash = eccAlg.SignHash;
            this.cryptoAlg = eccAlg;
        }

        /// <summary>
        /// A generic method for adding bytes to the signature calculation.
        /// </summary>
        /// <param name="buffer">The buffer to add bytes from.</param>
        /// <param name="index">The start index into the buffer.</param>
        /// <param name="count">The number of bytes to take from the buffer.</param>
        public void AddBytes(byte[] buffer, int index, int count)
        {
            this.hashAlg.TransformBlock(buffer, index, count, null, 0);
        }

        /// <summary>
        /// Adds a null (0x00) byte to the signature calculation.
        /// </summary>
        public void AddNullByte()
        {
            this.hashAlg.TransformBlock(nullByte, 0, 1, null, 0);
        }

        /// <summary>
        /// Adds the signature policy version to the signature calculation.
        /// This function will handle the conversion to big-endian and add
        /// the trailing null byte to the signature calculation.
        /// </summary>
        /// <param name="version">The policy version.</param>
        public void SignVersion(int version)
        {
            byte[] bytes = BitConverter.GetBytes(version);
            if (BitConverter.IsLittleEndian)
            {
                Array.Reverse(bytes);
            }

            this.AddBytes(bytes, 0, bytes.Length);
            this.AddNullByte();
        }

        /// <summary>
        /// Adds the Windows file time to the signature calculation.
        /// This function will handle the conversion to big-endian and add
        /// the trailing null byte to the signature calculation.
        /// </summary>
        /// <param name="timestamp">The Windows file time.</param>
        public void SignTimestamp(long timestamp)
        {
            byte[] bytes = BitConverter.GetBytes(timestamp);
            if (BitConverter.IsLittleEndian)
            {
                Array.Reverse(bytes);
            }

            this.AddBytes(bytes, 0, bytes.Length);
            this.AddNullByte();
        }

        /// <summary>
        /// Adds a string element to the signature calculation. This is used
        /// for adding text elements like the HTTP method, URI elements, and
        /// HTTP headers.
        /// This function will add the trailing null byte to the signature calculation.
        /// </summary>
        /// <param name="element"></param>
        public void SignElement(string element)
        {
            byte[] buffer = Encoding.ASCII.GetBytes(element);
            this.AddBytes(buffer, 0, buffer.Length);
            this.AddNullByte();
        }

        /// <summary>
        /// Verifies the signature matches.
        /// </summary>
        /// <param name="sig">The signature to verify against.</param>
        /// <returns>True if the signature matches. False otherwise.</returns>
        public bool VerifyHash(byte[] sig)
        {
            if (sig == null)
            {
                throw new ArgumentNullException("sig");
            }

            this.hashAlg.TransformFinalBlock(finalBlock, 0, 0);

            return this.verifyHash(this.hashAlg.Hash, sig);
        }

        /// <summary>
        /// Calculates the final signature.
        /// </summary>
        /// <returns>The calculated signature.</returns>
        public byte[] GetSignature()
        {
            this.hashAlg.TransformFinalBlock(finalBlock, 0, 0);
            return this.signHash(this.hashAlg.Hash);
        }

        /// <summary>
        /// Part of the IDisposable implementation.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Disposes the hashAlg and cryptoAlg objects.
        /// </summary>
        /// <param name="disposing"></param>
        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                this.hashAlg.Dispose();
                this.cryptoAlg.Dispose();
            }
        }
    }
}