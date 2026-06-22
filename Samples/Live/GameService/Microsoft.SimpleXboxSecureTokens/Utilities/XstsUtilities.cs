//-----------------------------------------------------------------------------
// XstsUtilities.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

#pragma warning disable IDE0063 // Use simple 'using' statement

using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Security.Cryptography;
using System.Threading.Tasks;

namespace Microsoft.SimpleXboxSecureTokens
{
    //  Helpful functions for decrypting the tokens
    public static class XstsUtilities
    {
        internal static async Task<byte[]> DecryptAsync(byte[] cipher, byte[] key, byte[] iv)
        {
            if (cipher == null)
            {
                throw new ArgumentNullException("cipher");
            }

            return await Task.Run(() =>
            {
                using (Aes aesAlg = Aes.Create())
                {
                    aesAlg.Key = key;
                    aesAlg.IV = iv;

                    //  Create a decryptor to preform the stream transform
                    using (ICryptoTransform decryptor = aesAlg.CreateDecryptor(aesAlg.Key, aesAlg.IV))
                    {
                        return Decrypt(decryptor, cipher);
                    }
                }
            });
        }

        internal static byte[] DecryptToBytes(ICryptoTransform decryptor, byte[] cipher)
        {
            if (decryptor == null)
            {
                throw new ArgumentNullException(nameof(decryptor));
            }

            if (cipher == null)
            {
                throw new ArgumentNullException(nameof(cipher));
            }

            using (MemoryStream ms = new(cipher))
            {
                using (CryptoStream cryptoStream = new(ms, decryptor, CryptoStreamMode.Read))
                using (MemoryStream byteStream = new())
                {
                    cryptoStream.CopyTo(byteStream);
                    return byteStream.ToArray();
                }
            }
        }

        internal static byte[] Decrypt(ICryptoTransform decryptor, byte[] cipher)
        {
            if (decryptor == null)
            {
                throw new ArgumentNullException(nameof(decryptor));
            }

            if (cipher == null)
            {
                throw new ArgumentNullException(nameof(cipher));
            }

            using (MemoryStream msDecrypt = new(cipher))
            {
                using (CryptoStream csDecrypt = new(msDecrypt, decryptor, CryptoStreamMode.Read))
                {
                    using (StreamReader srDecrypt = new(csDecrypt))
                    {
                        
                        // Read the decrypted bytes from the decrypting stream
                        int bufferBytesWritten = 0;
                        var outputBuffer = new byte[cipher.Length];

                        //  As part of the update to .NET 6.0 we need to now loop through the stream due to the following
                        //  change in Stream.Read and Stream.ReadAsync for CryptoStreams:
                        //  https://docs.microsoft.com/en-us/dotnet/core/compatibility/core-libraries/6.0/partial-byte-reads-in-streams
                        while (bufferBytesWritten < outputBuffer.Length)
                        {
                            var bytesRead = csDecrypt.Read(
                                            buffer: outputBuffer,
                                            offset: bufferBytesWritten,
                                            count:  outputBuffer.Length - bufferBytesWritten);

                            bufferBytesWritten += bytesRead;
                            if (bytesRead == 0)
                            {
                                break;
                            }

                        }

                        return outputBuffer;
                    }
                }
            }
        }

        internal static async Task<string> DecompressAsync(byte[] input)
        {
            return await Task.Run(() =>
            {
                using (MemoryStream inputStream = new(input))
                {
                    using (DeflateStream deflateStream = new(inputStream, CompressionMode.Decompress))
                    {
                        using (StreamReader reader = new(deflateStream))
                        {
                            return reader.ReadToEnd();
                        }
                    }
                }
            });
        }

        public static byte[] FromBase64Url(string toBeDecoded)
        {
            // Add padding
            toBeDecoded = toBeDecoded.PadRight(toBeDecoded.Length + (4 - toBeDecoded.Length % 4) % 4, '=');

            // Base64 URL encoded to Base 64 encoded
            toBeDecoded = toBeDecoded.Replace('-', '+').Replace('_', '/');

            // Base 64 decode
            byte[] raw = Convert.FromBase64String(toBeDecoded);
            return raw;
        }

        public static string ToBase64Url(byte[] arg)
        {
            if (arg == null)
            {
                throw new ArgumentNullException("arg");
            }
            string s = Convert.ToBase64String(arg);
            s = s.Split(XstsConstants.Base64PadCharacter)[0]; // Remove any trailing padding
            s = s.Replace(XstsConstants.Base64Character62, XstsConstants.Base64UrlCharacter62); // 62nd char of encoding
            s = s.Replace(XstsConstants.Base64Character63, XstsConstants.Base64UrlCharacter63); // 63rd char of encoding

            return s;
        }

        internal static byte[] ArrayConcat(params byte[][] args)
        {
            if (args == null)
            {
                return Array.Empty<byte>();
            }

            var nonNullArgs = args.Where(a => a != null);

            int resultLen = nonNullArgs.Sum(a => a.Length);
            byte[] res = new byte[resultLen];

            int index = 0;
            foreach (byte[] bArr in nonNullArgs)
            {
                Buffer.BlockCopy(bArr, 0, res, index, bArr.Length);
                index += bArr.Length;
            }

            return res;
        }

        internal static byte[] SignHmac(byte[] data, byte[] key)
        {
            using (HMACSHA256 hmacSha = new(key))
            {
                byte[] sig = hmacSha.ComputeHash(data);
                return sig;
            }
        }

        internal static void VerifyAuthenticationTag(byte[] aad, byte[] iv, byte[] cipherText, byte[] hmacKey, byte[] authTag)
        {
            byte[] aadBitLength = BitConverter.GetBytes((ulong)(aad.Length * 8));

            // AL value must be in Big Endian
            if (BitConverter.IsLittleEndian)
            {
                Array.Reverse(aadBitLength);
            }

            byte[] dataToSign = XstsUtilities.ArrayConcat(aad, iv, cipherText, aadBitLength);
            byte[] hash = XstsUtilities.SignHmac(dataToSign, hmacKey);
            byte[][] computedAuthTag = SplitSecretKey(hash);

            // Check if the auth tag is equal
            // The authentication tag is the first half of the hmac result
            if (!authTag.FixedTimeEquals(computedAuthTag[0]))
            {
                throw new InvalidOperationException("Authentication tag does not match with the computed hash.");
            }
        }
        internal static byte[][] SplitSecretKey(byte[] val)
        {
            // val should be even length
            if (val.Length % 2 != 0)
            {
                throw new Exception("Parameter byte[] val should be even length");
            }

            int midpoint = val.Length / 2;

            byte[] firstHalf = new byte[midpoint];
            byte[] secondHalf = new byte[midpoint];
            Buffer.BlockCopy(val, 0, firstHalf, 0, midpoint);
            Buffer.BlockCopy(val, midpoint, secondHalf, 0, midpoint);

            return new[] { firstHalf, secondHalf };
        }

        /// <summary>
        /// Determine the equality of two byte sequences in an amount of time which depends on
        /// the length of the sequences, but not the values. This helps prevent timing attacks.
        /// inspired by CryptographicOperations
        /// https://github.com/dotnet/runtime/blob/master/src/libraries/System.Security.Cryptography.Primitives/src/System/Security/Cryptography/CryptographicOperations.cs
        /// </summary>
        /// <param name="left">First buffer to compare</param>
        /// <param name="right">Second buffer to compare</param>
        /// <returns>true if left and right are the same length and contents</returns>
        [MethodImpl(MethodImplOptions.NoInlining | MethodImplOptions.NoOptimization)]
        public static bool FixedTimeEquals(this byte[] left, byte[] right)
        {
            // NoOptimization because we want this method to be exactly as non-short-circuiting
            // as written.
            //
            // NoInlining because the NoOptimization would get lost if the method got in-lined.
            if (left == null && right == null)
            {
                return true;
            }
            if (left == null || right == null || left.Length != right.Length)
            {
                return false;
            }
            int length = left.Length;
            int accum = 0;
            for (int i = 0; i < length; i++)
            {
                accum |= left[i] - right[i];
            }
            return accum == 0;
        }
    }
}

#pragma warning restore IDE0063 // Use simple 'using' statement
