//-----------------------------------------------------------------------------
// ClawbackRequest.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using System.Collections.Generic;

namespace Microsoft.StoreServices
{
    /// <summary>
    /// JSON request body for the Clawback service
    /// </summary>
    public class ClawbackQueryRequest
    {
        /// <summary>
        /// UserPurchaseId that identifies the user we are asking about
        /// </summary>
        [JsonProperty("b2bKey")] public string UserPurchaseId { get; set; }

        /// <summary>
        /// String list that filters the Clawback query results to specific states. See LineItemStates.
        /// </summary>
        [JsonProperty("lineItemStateFilter")] public List<string> LineItemStateFilter { get; set; }

        /// <summary>
        /// Creates an object used to generate the JSON body of the request.
        /// </summary>
        public ClawbackQueryRequest()
        {
            //  default values most commonly used
            LineItemStateFilter = new List<string>() 
            { 
                LineItemStates.Revoked,
                LineItemStates.Refunded
            };

            UserPurchaseId = ""; 
        }
    }

    /// <summary>
    /// Values supported for the LineitemState and LineItemStateFilter
    /// </summary>
    public static class LineItemStates
    {
        public const string Purchased = "Purchased";
        public const string Revoked   = "Revoked";
        public const string Refunded  = "Refunded";
    }
}
