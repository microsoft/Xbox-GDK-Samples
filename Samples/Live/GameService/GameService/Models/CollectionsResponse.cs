//-----------------------------------------------------------------------------
// CollectionsResponse.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System;
using System.Collections.Generic;

namespace GameService.Models
{
#pragma warning disable IDE1006 // Naming Styles

    //  Collections response class for json formatting
    //  Classes were generated using http://json2csharp.com from example
    //  JSON request and response
    public class RecurrenceData
    {
        public string recurrenceId { get; set; }
    }

    public class TrialData
    {
        public bool isInTrialPeriod { get; set; }
        public bool isTrial { get; set; }
        public string trialTimeRemaining { get; set; }
    }

    public class CollectionItem
    {
        public DateTime acquiredDate { get; set; }
        public string acquisitionType { get; set; }
        public DateTime endDate { get; set; }
        public string id { get; set; }
        public string legacyOfferInstanceId { get; set; }
        public string legacyProductId { get; set; }
        public string localTicketReference { get; set; }
        public DateTime modifiedDate { get; set; }
        public string purchasedCountry { get; set; }
        public string productFamily { get; set; }
        public string productId { get; set; }
        public string productKind { get; set; }
        public RecurrenceData recurrenceData { get; set; }
        public List<object> satisfiedByProductIds { get; set; }
        public string sharingSource { get; set; }
        public string skuId { get; set; }
        public DateTime startDate { get; set; }
        public string status { get; set; }
        public List<object> tags { get; set; }
        public TrialData trialData { get; set; }
        public string devOfferId { get; set; }
        public int quantity { get; set; }
        public string transactionId { get; set; }
    }

    public class CollectionsResponse
    {
        public List<CollectionItem> Items { get; set; }
    }

    public class ConsumeResponse
    {
        public string itemId { get; set; }
        public int newQuantity { get; set; }
        public string trackingId { get; set; }  
        public string productId { get; set; }
    }

    public class Innererror
    {
        public string code { get; set; }
        public List<string> data { get; set; }
        public List<object> details { get; set; }
        public string message { get; set; }
        public string source { get; set; }
    }

    public class ConsumeError
    {
        public string code { get; set; }
        public List<ConsumeErrorData> data { get; set; }
        public List<object> details { get; set; }
        public Innererror innererror { get; set; }
        public string message { get; set; }
        public string source { get; set; }
    }

    public class ConsumeErrorData
    {
        public string QuantityAvailable { get; set; }
    }
#pragma warning restore IDE1006 // Naming Styles
}
