//-----------------------------------------------------------------------------
// GameServicePersistentDBContext.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.EntityFrameworkCore;

namespace GameService
{
    public class GameServicePersistentDBContext : DbContext
    {
        public GameServicePersistentDBContext(DbContextOptions<GameServicePersistentDBContext> options)
            : base(options)
        {}

        //  Both of these dbsets will contain data that we want to be shared across our
        //  servers, so we store them in a persistent SQL database in Azure.  The sample
        //  by default looks for the app settings ConnectionString "GameServicePersistentDB"
        //  if not found, then it will default to use an in-memory database for simplicity
        //  But all deployed code should be using a real DB for these tables to prevent
        //  data loss and unnecessary network traffic.

        //  Todo: Recommendation -
        //        Add delegated auth x-token caching in a distributed cache between servers
        //        so that your service doesn't need to get a new token each service call.

    }
}
