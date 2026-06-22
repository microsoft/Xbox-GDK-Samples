//-----------------------------------------------------------------------------
// GDPRListResponse.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System.Collections.Generic;

namespace GameService.GDPR
{
    //  SECTION 4 - MPSD response class for json formatting
    //  Classes were generated using http://json2csharp.com from example
    //  JSON request and response
    public class Publisher
    {
        public string Id { get; set; }
        public string Name { get; set; }
    }

    public class GDPRListResponse
    {
        public Publisher publisher { get; set; }
        public List<string> Ids { get; set; }
    }

}
