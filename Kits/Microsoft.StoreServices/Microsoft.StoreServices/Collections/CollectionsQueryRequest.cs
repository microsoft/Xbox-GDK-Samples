//-----------------------------------------------------------------------------
// CollectionsQueryRequest.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using System.Collections.Generic;

namespace Microsoft.StoreServices
{
    /// <summary>
    /// JSON request body for a query to the Collections service
    /// </summary>
    public class CollectionsQueryRequest
    {
        /// <summary>
        /// The maximum number of products to return in one response. The default and maximum value is 100.
        /// </summary>
        [JsonProperty("maxPageSize")] public int MaxPageSize { get; set; }

        /// <summary>
        /// Removes duplicate entitlements where the user might be entitled to a single product from multiple sources.
        /// </summary>
        [JsonProperty("excludeDuplicates")] public bool ExcludeDuplicates { get; set; }

        /// <summary>
        /// Specifies which product types to return in the query results. For a list of valid values, see EntitlementFilterTypes.]
        /// </summary>
        [JsonProperty("EntitlementFilters")] public List<string> EntitlementFilters { get; set; }

        /// <summary>
        /// If specified, the service only returns products applicable to the provided product/SKU pairs. Recommended for all service-to-service queries for faster and more reliable results.
        /// </summary>
        [JsonProperty("productSkuIds")] public List<ProductSkuId> ProductSkuIds { get; set; }

        /// <summary>
        /// The country/region/market that you want to check the entitlement for. Using “neutral” (recommended) searches all markets. Otherwise, use the two-character ISO 3166 country/region code, for example, US.
        /// </summary>
        [JsonProperty("market")] public string Market { get; set; }

        /// <summary>
        /// Include items that are entitled through bundles or subscriptions in the results. If set to false, the results only contain the items the user has purchased, such as the parent bundle’s product information. If you’re using this parameter, always specify which products you want results for to avoid long or timed-out requests.
        /// </summary>
        [JsonProperty("expandSatisfyingItems")] public bool ExpandSatisfyingItems { get; set; }

        /// <summary>
        /// The user for which this item is entitled. You will use the UserCollectionsId in this property to define the user you want the results for.
        /// </summary>
        [JsonProperty("beneficiaries")] public List<CollectionsRequestBeneficiary> Beneficiaries { get; set; }

        /// <summary>
        /// Filter the results to this validity type which is based off the Status values of the items to be returned.
        /// </summary>
        [JsonProperty("validityType")] public string ValidityType { get; set; }

        public CollectionsQueryRequest()
        {
            //  default values most commonly used
            Market = "neutral";
            ExpandSatisfyingItems = true; //  This expands the results to include any products that
                                          //  are included in a bundle the user owns.
            ExcludeDuplicates = true;     //  Only include one result (entitlement) per item.

            MaxPageSize = 100;            //  Default Max is 100

            //  Default Game related product types
            //  Filter our results to include these product types
            EntitlementFilters = new List<string>() {
                EntitlementFilterTypes.Game,
                EntitlementFilterTypes.Consumable,
                EntitlementFilterTypes.Durable
            };

            Beneficiaries = new List<CollectionsRequestBeneficiary>();
        }
    }

    /// <summary>
    /// JSON structure to provide the UserCollectionsId to identify which user we are making the request for.
    /// </summary>
    public class CollectionsRequestBeneficiary 
    {
        /// <summary>
        /// Must be set to "b2b".
        /// </summary>
        [JsonProperty("identitytype")] public string Identitytype { get; set; }

        /// <summary>
        /// The User Store ID key that represents the identity of the user for whom you want to report a consumable product as fulfilled.
        /// </summary>
        [JsonProperty("identityValue")] public virtual string UserCollectionsId { get; set; }

        /// <summary>
        /// The requested identifier for the returned response. We recommend that you use the same value as the userId claim in the User Store ID key.
        /// </summary>
        [JsonProperty("localTicketReference")] public string LocalTicketReference { get; set; }

        public CollectionsRequestBeneficiary()
        {
            Identitytype         = "b2b";
            UserCollectionsId    = "";
            LocalTicketReference = "";
        }
    }

    /// <summary>
    /// JSON structure to denote the ProductId and the Sku
    /// </summary>
    public class ProductSkuId
    {
        /// <summary>
        /// ProductId (StoreId) of the item offered within the store.
        /// </summary>
        [JsonProperty("productId")] public string ProductId { get; set; }

        /// <summary>
        /// SkuId representing the specific sub-offering of the product that was purchased.  For query requests, this value can be blank.
        /// </summary>
        [JsonProperty("skuId")] public string SkuId { get; set; }
    }

    /// <summary>
    /// String values that can be added to a query request to filter the results to these specific
    /// product types.
    /// </summary>
    public static class EntitlementFilterTypes
    {
        /// <summary>
        /// Games products.
        /// </summary>
        public const string Game                = "*:Game";

        /// <summary>
        /// Apps that are not games.
        /// </summary>
        public const string Application         = "*:Application";

        /// <summary>
        /// Game content such as DLC and most single-time purchase items
        /// </summary>
        public const string Durable             = "*:Durable";

        /// <summary>
        /// Store-managed consumable products (recommended consumable type for most scenarios).
        /// </summary>
        public const string Consumable          = "*:Consumable";

        /// <summary>
        /// Developer-managed consumables that must be fulfilled before being able to be purchased by the user again.
        /// </summary>
        public const string UnmanagedConsumable = "*:UnmanagedConsumable";
    }

    /// <summary>
    /// Filter options for Collections query requests.  Results will be based on specific Status values.
    /// </summary>
    public static class ValidityTpes
    {
        /// <summary>
        /// Everything will be returned.
        /// </summary>
        public const string All           = "All";

        /// <summary>
        /// Items that have a state of Active
        /// </summary>
        public const string Valid         = "Valid";

        /// <summary>
        /// Items that are Expired, Revoked, or in other non-valid states.
        /// </summary>
        public const string Invalid       = "Invalid";

        /// <summary>
        /// Items that are active but not yet reached their end date
        /// </summary>
        public const string NotYetEnded   = "NotYetEnded";

        /// <summary>
        /// Items that have not yet been activated such as pre-orders
        /// </summary>
        public const string NotYetStarted = "NotYetStarted";
    }
}
