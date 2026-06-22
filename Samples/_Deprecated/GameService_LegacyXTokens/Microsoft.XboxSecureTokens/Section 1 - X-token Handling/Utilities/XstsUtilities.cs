//-----------------------------------------------------------------------------
// XstsUtil.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading.Tasks;

namespace Microsoft.XboxSecureTokens
{
    //  SECTION 1 - Helpful functions for decrypting the tokens
    static class XstsUtilities
    {
        internal static async Task<byte[]> DecryptAsync(byte[] cipher, byte[] key, byte[] iv)
        {
            if (cipher == null)
            {
                throw new ArgumentNullException("cipher");
            }

            return await Task.Run(() =>
            {
                using (RijndaelManaged symmetricKey = new RijndaelManaged())
                {
                    symmetricKey.Mode = CipherMode.CBC;
                    byte[] plainTextBytes = new byte[cipher.Length];
                    using (ICryptoTransform decryptor = symmetricKey.CreateDecryptor(key, iv))
                    {
                        return Decrypt(decryptor, cipher);
                    }
                }
            });
        }

        internal static byte[] Decrypt(ICryptoTransform decryptor, byte[] cipher)
        {
            if (decryptor == null)
            {
                throw new ArgumentNullException("decryptor");
            }

            if (cipher == null)
            {
                throw new ArgumentNullException("cipher");
            }

            byte[] decryptedBytes = null;

            using (var ms = new MemoryStream(cipher))
            {
                using (var cryptoStream = new CryptoStream(ms, decryptor, CryptoStreamMode.Read))
                {
                    decryptedBytes = new byte[cipher.Length];
                    int count = cryptoStream.Read(decryptedBytes, 0, decryptedBytes.Length);
                    decryptedBytes = decryptedBytes.Take(count).ToArray();
                }
            }

            return decryptedBytes;
        }

        internal static async Task<string> DecompressAsync(byte[] input)
        {
            return await Task.Run(() =>
            {
                using (var inputStream = new MemoryStream(input))
                {
                    using (var deflateStream = new DeflateStream(inputStream, CompressionMode.Decompress))
                    {
                        using (var reader = new StreamReader(deflateStream))
                        {
                            return reader.ReadToEnd();
                        }
                    }
                }
            });
        }

        internal static byte[] FromBase64Url(string toBeDecoded)
        {
            // Add padding
            toBeDecoded = toBeDecoded.PadRight(toBeDecoded.Length + (4 - toBeDecoded.Length % 4) % 4, '=');

            // Base64 URL encoded to Base 64 encoded
            toBeDecoded = toBeDecoded.Replace('-', '+').Replace('_', '/');

            // Base 64 decode
            byte[] raw = Convert.FromBase64String(toBeDecoded);
            return raw;
        }

        internal static string ToBase64Url(byte[] arg)
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
                return new byte[0];
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
            using (HMACSHA256 hmacSha = new HMACSHA256(key))
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
            if (!authTag.SequenceEqual(computedAuthTag[0]))
            {
                throw new InvalidOperationException("Authentication tag does not match with the computed hash.");
            }
        }
        internal static byte[][] SplitSecretKey(byte[] val)
        {
            // val should be even length
            if (val.Length % 2 != 0)
            {
                throw new ArgumentException();
            }

            int midpoint = val.Length / 2;

            byte[] firstHalf = new byte[midpoint];
            byte[] secondHalf = new byte[midpoint];
            Buffer.BlockCopy(val, 0, firstHalf, 0, midpoint);
            Buffer.BlockCopy(val, midpoint, secondHalf, 0, midpoint);

            return new[] { firstHalf, secondHalf };
        }

        
        //  Below functions are used for Legacy XSTS token handling (Asymmetric Draft7 JWT)
        internal static byte[] ConcatKdf(byte[] contentMasterKey, string enc, string label, int keydatalen)
        {
            int keydatalenInBytes = keydatalen / 8;

            byte[] encBytes = Encoding.UTF8.GetBytes(enc);
            byte[] labelBytes = Encoding.ASCII.GetBytes(label);

            List<byte> result = new List<byte>();

            int round = 1;

            while (result.Count < keydatalenInBytes)
            {
                byte[] hash_input = XstsUtilities.ArrayConcat(
                    ConvertToBigEndian(round),
                    contentMasterKey,
                    ConvertToBigEndian(keydatalen),
                    encBytes,
                    ConvertToBigEndian(0),
                    ConvertToBigEndian(0),
                    labelBytes);

                using (SHA256 sha = new SHA256Managed())
                {
                    result.AddRange(sha.ComputeHash(hash_input));
                }

                round++;
            }

            return result.Take(keydatalenInBytes).ToArray();
        }

        internal static byte[] ConvertToBigEndian(int val)
        {
            byte[] intBytes = BitConverter.GetBytes(val);
            if (BitConverter.IsLittleEndian)
            {
                Array.Reverse(intBytes);
            }
            return intBytes;
        }

        internal static X509Certificate2 GetCertificateFromThumbprint(string Thumbprint, bool PrivateKeyAccessRequired)
        {
            X509Certificate2 targetCert = null;

            //  Remove any hidden characters that might be in the string due to copy and paste
            //  from the MMC.exe UI that will result in a failed search  
            Thumbprint = Thumbprint.Replace(@"[^\da-fA-F]", string.Empty).ToUpper();

            //  find the cert in the store (for this example we use the Local Machine store)
            try
            {
                var store = new X509Store(StoreName.My, StoreLocation.LocalMachine);
                store.Open(OpenFlags.ReadOnly);
                X509Certificate2Collection certCollection = store.Certificates.Find(X509FindType.FindByThumbprint, Thumbprint, false);
                if(certCollection.Count > 0)
                {
                    targetCert = certCollection[0];
                }
                else
                {
                    throw new InvalidOperationException(string.Format("GetCertificateFromThumbprint Error: Unable to find certificate from thumbprint: {0}", Thumbprint));
                }
                store.Close();
            }
            catch (CryptographicException)
            {
                throw new InvalidOperationException(string.Format("GetCertificateFromThumbprint Error: Unable to find certificate from thumbprint: {0}", Thumbprint));
            }

            //  Now validate that we have access to the private key of the certificate if required

            if (PrivateKeyAccessRequired && targetCert.PrivateKey == null)
            {
                throw new InvalidOperationException(string.Format("GetCertificateFromThumbprint Error: No private key access for certificate {0}", targetCert.FriendlyName));
            }

            return targetCert;

        }
    }
}
