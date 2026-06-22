//-----------------------------------------------------------------------------
// StoreServicesClient.Clawback.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using System;
using System.Threading.Tasks;

namespace Microsoft.StoreServices
{
    public sealed partial class StoreServicesClient : IStoreServicesClient
    {
        /// <summary>
        /// Query for the user's refunded products from the Clawback service based on the 
        /// parameters of the request.
        /// </summary>
        /// <param name="request"></param>
        /// <returns></returns>
        public async Task<ClawbackQueryResponse> ClawbackQueryAsync(ClawbackQueryRequest queryParameters)
        {
            if (string.IsNullOrEmpty(queryParameters.UserPurchaseId))
            {
                throw new ArgumentException($"{nameof(queryParameters.UserPurchaseId)} must be provided", nameof(queryParameters.UserPurchaseId));
            }

            //  Post the request and wait for the response
            var userClawback = await IssueRequestAsync<ClawbackQueryResponse>(
                "https://purchase.mp.microsoft.com/v8.0/b2b/orders/query",
                JsonConvert.SerializeObject(queryParameters),
                null);

            return userClawback;
        }
    }
}
