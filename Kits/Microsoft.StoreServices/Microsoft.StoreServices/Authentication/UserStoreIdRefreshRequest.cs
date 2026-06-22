//-----------------------------------------------------------------------------
// UserStoreIdRefreshRequest.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;

namespace Microsoft.StoreServices
{
    /// <summary>
    /// Used to generate the JSON body of a request to refresh a UserStoreId.
    /// </summary>
    class UserStoreIdRefreshRequest
    {
        [JsonProperty("serviceTicket")] public string ServiceToken { get; set; }
        [JsonProperty("key")] public string UserStoreId { get; set; }
    }
}
