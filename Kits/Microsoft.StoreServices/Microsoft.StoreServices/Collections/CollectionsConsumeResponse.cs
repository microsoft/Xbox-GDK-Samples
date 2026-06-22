//-----------------------------------------------------------------------------
// CollectionsConsumeResponse.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using System.Collections.Generic;

namespace Microsoft.StoreServices
{
    /// <summary>
    /// JSON response from a successful consume request
    /// </summary>
    public class CollectionsConsumeResponse
    {
        /// <summary>
        /// ID that identifies this collection item from other items that the user owns. This ID is unique per product.
        /// </summary>
        [JsonProperty("itemId")] public string ItemId { get; set; }

        /// <summary>
        /// The remaining balance of this consumable that the user owns.
        /// </summary>
        [JsonProperty("newQuantity")] public int NewQuantity { get; set; }

        /// <summary>
        /// The unique tracking ID that was provided in the request to track and validate that the fulfillment succeeded.
        /// </summary>
        [JsonProperty("trackingId")] public string TrackingId { get; set; }

        /// <summary>
        /// The ProductId / StoreId of the consumable that was fulfilled.
        /// </summary>
        [JsonProperty("productId")] public string ProductId { get; set; }
    }

    /// <summary>
    /// Error data from the consume service if the consume request failed
    /// </summary>
    public class ConsumeError
    {
        /// <summary>
        /// Error code related to the consume service
        /// </summary>
        [JsonProperty("code")] public string Code { get; set; }

        /// <summary>
        /// Additional data about the error
        /// </summary>
        [JsonProperty("data")] public List<string> Data { get; set; }

        /// <summary>
        /// Additional details about the error
        /// </summary>
        [JsonProperty("details")] public List<object> Details { get; set; }

        /// <summary>
        /// Message describing the error
        /// </summary>
        [JsonProperty("message")] public string Message { get; set; }

        /// <summary>
        /// Source of the error being reported
        /// </summary>
        [JsonProperty("source")] public string Source { get; set; }
    }

    /// <summary>
    /// JSON response body if there was an error executing the consume request
    /// </summary>
    public class CollectionsConsumeErrorResponse
    {
        /// <summary>
        /// Error code related to the consume service
        /// </summary>
        [JsonProperty("code")] public string Code { get; set; }

        /// <summary>
        /// Additional data about the error
        /// </summary>
        [JsonProperty("data")] public List<string> Data { get; set; }

        /// <summary>
        /// Additional details about the error
        /// </summary>
        [JsonProperty("details")] public List<object> Details { get; set; }

        /// <summary>
        /// Error data from the consume service if the consume request failed
        /// </summary>
        [JsonProperty("innererror")] public ConsumeError InnerError { get; set; }

        /// <summary>
        /// Message describing the error
        /// </summary>
        [JsonProperty("message")] public string Message { get; set; }

        /// <summary>
        /// Source of the error being reported
        /// </summary>
        [JsonProperty("source")] public string Source { get; set; }
    }
}
