//-----------------------------------------------------------------------------
// CollectionsQueryResponse.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System.Collections.Generic;

namespace Microsoft.StoreServices
{
    /// <summary>
    /// JSON response body from the Collections service
    /// </summary>
    public class CollectionsQueryResponse
    {
        /// <summary>
        /// List of CollectionsItems returned from the request
        /// </summary>
        public List<CollectionsItem> Items { get; set; }
    }
}
