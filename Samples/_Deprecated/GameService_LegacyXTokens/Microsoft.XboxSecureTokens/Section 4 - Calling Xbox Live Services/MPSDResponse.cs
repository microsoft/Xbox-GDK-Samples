//-----------------------------------------------------------------------------
// MPSDResponse.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

namespace GameService.MPSD
{
    //  SECTION 4 - MPSD response class for json formatting
    //  Classes were generated using http://json2csharp.com from example
    //  JSON request and response
    public class Capabilities
    {
        public bool connectivity { get; set; }
        public bool connectionRequiredForActiveMembers { get; set; }
        public bool gameplay { get; set; }
        public bool crossPlay { get; set; }
        public bool userAuthorizationStyle { get; set; }
    }

    public class System
    {
        public int version { get; set; }
        public int maxMembersCount { get; set; }
        public string visibility { get; set; }
        public string inviteProtocol { get; set; }
        public Capabilities capabilities { get; set; }
    }

    public class Custom
    {
    }

    public class Constants
    {
        public System system { get; set; }
        public Custom custom { get; set; }
    }

    public class Fixed
    {
        public Constants constants { get; set; }
    }

    public class MPSDResponse
    {
        public int contractVersion { get; set; }
        public Fixed @fixed { get; set; }
    }

}
