//-----------------------------------------------------------------------------
// XstsPersistentDBContext.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using GameService.Collections;
using Microsoft.EntityFrameworkCore;

namespace Microsoft.XboxSecureTokens
{
    //  SECTION 5 - Persistent Database Context
    public class XstsPersistentDBContext : DbContext
    {
        public XstsPersistentDBContext(DbContextOptions<XstsPersistentDBContext> options)
            : base(options)
        {}

        //  Both of these dbsets will contain data that we want to be shared across our
        //  servers, so we store them in a persistent SQL database in Azure.  The sample
        //  by default looks for the appsettings ConnectionString "GameServicePersistentDB"
        //  if not found, then it will default to use an in-memory database for simplicity
        //  But all deployed code should be using a real DB for these tables to prevent
        //  data loss and unnecessary network traffic.
        public DbSet<ConsumeRequest> PendingConsumeRequests { get; set; }
    }
}
