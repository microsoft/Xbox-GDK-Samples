//-----------------------------------------------------------------------------
// SignaturePolicy.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

namespace Microsoft.XboxSecureTokens.XstsDelegatedAuth
{
    /// <summary>
    /// SECTION 2 - Signature policy json formatting class
    /// This specifies a signature policy.
    /// </summary>
    public class SignaturePolicy
    {
        public static readonly SignaturePolicy[] DefaultSignaturePolicies = new[]
        {
            new SignaturePolicy
            {
                Version = 1,
                ClockSkewSeconds = 5*60,
                MaxBodyBytes = 1024*8,
                SupportedAlgorithms = new string[] { "ES256", "RS256" },
            }
        };

        /// <summary>
        /// Gets or sets the policy version.
        /// </summary>
        public int Version { get; set; }

        /// <summary>
        /// Gets or sets the supported signing algorithms.
        /// </summary>
        public string[] SupportedAlgorithms { get; set; }

        /// <summary>
        /// Gets or sets the additional headers to include in the signature.
        /// Note that this list is ordered.
        /// </summary>
        public string[] ExtraHeaders { get; set; }

        /// <summary>
        /// Gets or sets the maximum number of bytes from the body to include
        /// in the signature.
        /// </summary>
        public long MaxBodyBytes { get; set; }

        /// <summary>
        /// Gets or sets the maximum clock skew.
        /// </summary>
        public int ClockSkewSeconds { get; set; }

        /// <summary>
        /// Gets or sets the ignoring of clock skew enforcement
        /// </summary>
        public bool IgnoreClockSkew { get; set; }
    }
}