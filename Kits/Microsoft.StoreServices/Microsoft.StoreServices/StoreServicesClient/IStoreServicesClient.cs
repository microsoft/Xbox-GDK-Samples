//-----------------------------------------------------------------------------
// StoreServicesClient.Clawback.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System;
using System.Threading.Tasks;

namespace Microsoft.StoreServices
{
    /// <summary>
    /// Provides an interface to create a client that will manage the requests and responses from the
    /// Microsoft Store Services.
    /// </summary>
    public interface IStoreServicesClient : IDisposable
    {
        /// <summary>
        /// Query the user's collections information and returns any items that meet the 
        /// query parameters.  These items are directly owned by the user who's 
        /// UserCollectionsId we are passing with the QueryParameters.
        /// </summary>
        /// <param name="request"></param>
        /// <returns></returns>
        Task<CollectionsQueryResponse> CollectionsQueryAsync(CollectionsQueryRequest request);

        /// <summary>
        /// Consumes a specified quantity of a product from a user's active balance in the store.
        /// </summary>
        /// <param name="request"></param>
        /// <returns></returns>
        Task<CollectionsConsumeResponse> CollectionsConsumeAsync(CollectionsConsumeRequest request);

        /// <summary>
        /// Query's the Clawback information for the user based on the parameters object.
        /// </summary>
        /// <param name="request"></param>
        /// <returns></returns>
        Task<ClawbackQueryResponse> ClawbackQueryAsync(ClawbackQueryRequest request);

        /// <summary>
        /// Query's the Recurrence information for the user based on the parameters object
        /// </summary>
        /// <param name="request"></param>
        /// <returns></returns>
        Task<RecurrenceQueryResponse> RecurrenceQueryAsync(RecurrenceQueryRequest request);

        /// <summary>
        /// Allows the caller to change the billing state of an existing user's
        /// subscription.  Extend its date, cancel, enable/disable auto-renew, or
        /// refund the subscription.
        /// </summary>
        /// <param name="request"></param>
        /// <returns></returns>
        Task<RecurrenceChangeResponse> RecurrenceChangeAysnc(RecurrenceChangeRequest request);

        /// <summary>
        /// Provides the Service Access Token from the IAccessTokenControler used when the
        /// Client was configured.
        /// </summary>
        /// <returns></returns>
        Task<AccessToken> GetServiceAccessTokenAsync();
        /// <summary>
        /// Provides the Collections Access Token from the IAccessTokenControler used when the
        /// Client was configured.
        /// </summary>
        /// <returns></returns>
        Task<AccessToken> GetCollectionsAccessTokenAsync();
        /// <summary>
        /// Provides the Purchase Access Token from the IAccessTokenControler used when the
        /// Client was configured.
        /// </summary>
        /// <returns></returns>
        Task<AccessToken> GetPurchaseAccessTokenAsync();       
    }
}
