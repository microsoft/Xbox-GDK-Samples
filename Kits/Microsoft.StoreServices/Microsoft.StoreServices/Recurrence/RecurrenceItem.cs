//-----------------------------------------------------------------------------
// RecurrenceItem.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using System;

namespace Microsoft.StoreServices
{
    /// <summary>
    /// Information related to a subscription the user has or had an entitlement to.
    /// </summary>
    public class RecurrenceItem
    {
        /// <summary>
        /// Indicates if the user is enrolled to have their subscription auto-renew at the end
        /// of the next billing cycle.
        /// </summary>
        [JsonProperty("autoRenew")] public bool AutoRenew { get; set; }

        /// <summary>
        /// Id that was used when the UserPurchaseId was created.  This will match the UserId value in the
        /// UserPurchaseId.
        /// </summary>
        [JsonProperty("beneficiary")] public string Beneficiary { get; set; }

        /// <summary>
        /// The UTC date and time that the subscription will or has expired
        /// </summary>
        [JsonProperty("expirationTime")] public DateTime ExpirationTime { get; set; }

        /// <summary>
        /// The UTC date and time that the user's Grace period will end if auto-renew fails at the 
        /// ExpirationTime.  During Grace, users should still have access and be considered valid
        /// subscribers, but notified they need to fix their auto-renew payment.
        /// </summary>
        [JsonProperty("expirationTimeWithGrace")] public DateTime ExpirationTimeWithGrace { get; set; }

        /// <summary>
        /// An ID that identifies this collection item from other items that the user owns. This ID is unique per product.
        /// </summary>
        [JsonProperty("id")] public string Id { get; set; }

        /// <summary>
        /// Indicates if the product is in a trial period, such as a subscription.
        /// </summary>
        [JsonProperty("isTrial")] public bool IsTrial { get; set; }

        /// <summary>
        /// The UTC date that this item was last modified.
        /// </summary>
        [JsonProperty("lastModified")] public DateTime LastModified { get; set; }

        /// <summary>
        /// The country the product was purchased in following the two-character ISO 3166 country/region code. EX: US.
        /// </summary>
        [JsonProperty("market")] public string Market { get; set; }

        /// <summary>
        /// Also refereed to as the Store ID for the product within the Microsoft Store catalog. An example Store ID for
        /// a product is 9NBLGGH42CFD.
        /// </summary>
        [JsonProperty("productId")] public string ProductId { get; set; }

        /// <summary>
        /// Current state of the recurrence.  See RecurrenceSates.
        /// </summary>
        [JsonProperty("recurrenceState")] public string RecurrenceState { get; set; }

        /// <summary>
        /// The specific SKU identifier if there are multiple offerings of the product in the Microsoft Store catalog. An example Store ID for a SKU is 0010.
        /// </summary>
        [JsonProperty("skuId")] public string SkuId { get; set; }

        /// <summary>
        /// The UTC date that the subscription became or will become valid.
        /// </summary>
        [JsonProperty("startTime")] public DateTime StartTime { get; set; }

        /// <summary>
        /// The UTC date that the subscription was canceled.
        /// </summary>
        [JsonProperty("cancellationDate")] public DateTime CancellationDate { get; set; }
    }

    /// <summary>
    /// Strings representing the state that a subscription can be in
    /// </summary>
    public class RecurrenceStates
    {
        /// <summary>
        /// Currently subscribed and should have access to subscription benefits
        /// </summary>
        public static string Active   = "Active";

        /// <summary>
        /// Subscription was canceled, benefits should not be active.
        /// </summary>
        public static string Canceled = "Canceled";

        /// <summary>
        /// Subscription was refunded, benefits should not be active.
        /// </summary>
        public static string Revoked  = "Revoked";
    }
}
