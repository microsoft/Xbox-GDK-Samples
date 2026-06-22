//-----------------------------------------------------------------------------
// XblPeopleResponse.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System;
using System.Collections.Generic;

namespace GameService.Models
{
    //  SECTION 4 - People response class for json formatting
    //  Classes were generated using http://json2csharp.com from example
    //  JSON request and response
    public class Person
    {
        public string xuid { get; set; }
        public DateTime addedDateTimeUtc { get; set; }
        public bool isFavorite { get; set; }
        public bool isKnown { get; set; }
        public List<object> socialNetworks { get; set; }
        public bool isFollowedByCaller { get; set; }
        public bool isFollowingCaller { get; set; }
        public bool isUnfollowingFeed { get; set; }
    }

    public class XblPeopleResponse
    {
        public int totalCount { get; set; }
        public List<Person> people { get; set; }
    }
}
