//-----------------------------------------------------------------------------
// RecurrenceChangeRequest.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;

namespace Microsoft.StoreServices
{
    /// <summary>
    /// Types of change actions that can be done to manage a subscription.
    /// </summary>
    public enum RecurrenceChangeType
    {
        /// <summary>
        /// Cancel the subscription to prevent future renewal, users keep the remaining time from the current
        /// subscription period they have paid for.
        /// </summary>
        Cancel,

        /// <summary>
        /// Add days to an existing subscription for free. Ex: Your services are down for a day and you want
        /// to add a day to all user's subscriptions to compensate for that.
        /// </summary>
        Extend,

        /// <summary>
        /// Cancel the subscription, ends subscription benefits for the remainder of the paid time, refunds the user
        /// the remaining time left on their subscription period.
        /// </summary>
        Refund,

        /// <summary>
        /// Turns auto-renew off if it is on.  Services by default are not allowed to enable auto-renew if it is 
        /// currently disabled.
        /// </summary>
        ToggleAutoRenew
    }

    /// <summary>
    /// JSON request body to initiate a subscription management action
    /// </summary>
    public class RecurrenceChangeRequest
    {
        /// <summary>
        /// Id unique to the user and the subscription to be acted upon.
        /// </summary>
        [JsonIgnore] public string RecurrenceId { get; set; }

        /// <summary>
        /// The UserPurchaseId that identifies the user we are acting for.
        /// </summary>
        [JsonProperty("b2bKey")] public string UserPurchaseId { get; set; }

        /// <summary>
        /// Type of action to be done with the subscription.  See RecurrenceChangeTypes.
        /// </summary>
        [JsonProperty("changeType")] public string ChangeType { get; set; }

        /// <summary>
        /// If using the Extend change type, specifies how many days to add to the user's subscription.
        /// </summary>
        [JsonProperty("extensionTimeInDays")] public int ExtensionTimeInDays { get; set; }
    }
}
