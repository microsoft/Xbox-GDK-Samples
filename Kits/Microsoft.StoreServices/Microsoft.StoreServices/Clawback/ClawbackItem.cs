//-----------------------------------------------------------------------------
// ClawbackItem.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using System;
using System.Collections.Generic;

namespace Microsoft.StoreServices
{
    /// <summary>
    /// Item representing a result from the purchase/query service (aka Clawback).  Depending on the 
    /// OrderLineItems[].LineItemState values the item may be a refunded or revoked item from the 
    /// user's owned items.
    /// </summary>
    public class ClawbackItem
    {
        /// <summary>
        /// OrderId represents a unique identifier for the purchase transaction this information is tied to.
        /// </summary>
        [JsonProperty("orderId")] public string OrderId { get; set; }

        /// <summary>
        /// List of items that were purchased with the purchase transaction that the OrderId represents.
        /// </summary>
        [JsonProperty("orderLineItems")] public List<OrderLineItem> OrderLineItems { get; set; }

        /// <summary>
        /// UTC date time when the purchase transaction was made.
        /// </summary>
        [JsonProperty("orderPurchasedDate")] public DateTime OrderPurchaseDate { get; set;}

        /// <summary>
        /// UTC date time when the purchase was refunded (if in a refunded state).
        /// </summary>
        [JsonProperty("orderRefundedDate")] public DateTime OrderRefundedDate { get; set;}

    }
    
    /// <summary>
    /// Represents a single product that was included in the purchase transaction made by the user.
    /// </summary>
    public class OrderLineItem
    {
        /// <summary>
        /// Unique Id representing this product within the purchase transaction.
        /// </summary>   
        [JsonProperty("lineItemId")] public string LineItemId { get; set; }

        /// <summary>
        /// Current state of the item indicating if it is active, revoked, or refunded.
        /// </summary>
        [JsonProperty("lineItemState")] public string LineItemState { get; set; }

        /// <summary>
        /// ProductId (StoreId) of the item offered within the store.
        /// </summary>
        [JsonProperty("productId")] public string ProductId { get; set; }

        /// <summary>
        /// Quantity granted with the purchase. Usually 1 but can be more if this is a consumable.
        /// </summary>
        [JsonProperty("quantity")] public int Quantity { get; set; }

        /// <summary>
        /// SkuId representing the specific sub-offering of the product that was purchased.
        /// </summary>
        [JsonProperty("skuId")] public string SkuId { get; set; }
    }
}
