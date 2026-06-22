//-----------------------------------------------------------------------------
// EndpointAndSignatureContext.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.EntityFrameworkCore;
using Microsoft.XboxSecureTokens.XstsDelegatedAuth;

namespace Microsoft.XboxSecureTokens
{
    public class EndpointAndSignatureContext : DbContext
    {
        public EndpointAndSignatureContext(DbContextOptions<EndpointAndSignatureContext> options)
            : base(options)
        {
        }

        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
        }

        //  Endpoints, Signature Policies, and Service token that
        //  we will need for B2B Xbox Live Service calls
        public DbSet<XblEndpoint> Endpoints { get; set; }
        public DbSet<ServiceSignaturePolicy> SignaturePolicies { get; set; }
    }
}
