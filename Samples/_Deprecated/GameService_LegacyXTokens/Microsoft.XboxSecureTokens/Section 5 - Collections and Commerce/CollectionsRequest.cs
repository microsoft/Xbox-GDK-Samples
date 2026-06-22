//-----------------------------------------------------------------------------
// CollectionsRequest.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.Runtime.Serialization;

namespace GameService.Collections
{
    //  SECTION 5 - Collections request class for json formatting
    //  Classes were generated using http://json2csharp.com from example
    //  JSON request and response
    public class CollectionsRequest
    {
        [JsonProperty("maxPageSize")] public int MaxPageSize { get; set; }
        [JsonProperty("excludeDuplicates")] public bool ExcludeDuplicates { get; set; }
        [JsonProperty("EntitlementFilters")] public List<string> EntitlementFilters { get; set; }
        [JsonProperty("productSkuIds")] public List<ProductSkuId> ProductSkuIds { get; set; }
        [JsonProperty("market")] public string Market { get; set; }
        [JsonProperty("expandSatisfyingItems")] public bool ExpandSatisfyingItems { get; set; }
    }

    public class ProductSkuId
    {
        [JsonProperty("productId")] public string ProductId { get; set; }
        [JsonProperty("skuId")] public string SkuId { get; set; }
    }

    public static class CollectionsFilters
    {
        public const string Game                = "*:Game";
        public const string Application         = "*:Application";
        public const string Durable             = "*:Durable";
        public const string Consumable          = "*:Consumable";
        public const string UnmanagedConsumable = "*:UnmanagedConsumable";
    }

    public class ConsumeRequest
    {
        [JsonProperty("productId")] public string ProductId { get; set; }
        [Key]
        [JsonProperty("trackingId")] public string TrackingId { get; set; }
        [JsonProperty("removeQuantity")] public int RemoveQuantity { get; set; }
        [DataMember]
        [JsonProperty("user")] public string User { get; set; }
    }


}
