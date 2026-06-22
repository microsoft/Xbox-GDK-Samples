//-----------------------------------------------------------------------------
// RecurrenceQueryResponse.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System.Collections.Generic;

namespace Microsoft.StoreServices
{
    /// <summary>
    /// JSON response body from a query to the Recurrence service
    /// </summary>
    public class RecurrenceQueryResponse
    {
        /// <summary>
        /// List of ReucrrenceItems returned from the request
        /// </summary>
        public List<RecurrenceItem> Items { get; set; }
    }
}
