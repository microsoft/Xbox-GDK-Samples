//-----------------------------------------------------------------------------
// PackageCollection.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Xbox.AgentProtocol;

namespace XboxAgentServerSample.Models
{
    public class PackageCollection
    {
        public Package[] Packages { get; set; } = Array.Empty<Package>();
    }
}
