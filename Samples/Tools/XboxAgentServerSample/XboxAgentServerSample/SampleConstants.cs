//-----------------------------------------------------------------------------
// SampleConstants.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

namespace XboxAgentServerSample
{
    public class SampleConstants
    {
        public const string InMemoryDB = "InMemoryDB";

        public static void InitializeXSTS(IServiceProvider serviceProvider)
        {
            serviceProvider.GetService<IHttpClientFactory>();

        }
    }
}