//-----------------------------------------------------------------------------
// StoreServicesClient.Recurrence.cs
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
        /// Query for the user's subscriptions.
        /// </summary>
        /// <param name="request"></param>
        /// <returns></returns>
        public async Task<RecurrenceQueryResponse> RecurrenceQueryAsync(RecurrenceQueryRequest request)
        {
            if (string.IsNullOrEmpty(request.UserPurchaseId))
            {
                throw new ArgumentException($"{nameof(request.UserPurchaseId)} must be provided", nameof(request.UserPurchaseId));
            }

            //  Post the request and wait for the response
            var userRecurrences = await IssueRequestAsync<RecurrenceQueryResponse>(
                "https://purchase.mp.microsoft.com/v8.0/b2b/recurrences/query",
                JsonConvert.SerializeObject(request),
                null);

            return userRecurrences;
        }

        /// <summary>
        /// Change the user's subscription based on the request parameters.
        /// </summary>
        /// <param name="request"></param>
        /// <returns></returns>
        public async Task<RecurrenceChangeResponse> RecurrenceChangeAysnc(RecurrenceChangeRequest request)
        {
            if (string.IsNullOrEmpty(request.UserPurchaseId))
            {
                throw new ArgumentException($"{nameof(request.UserPurchaseId)} must be provided", nameof(request.UserPurchaseId));
            }

            //  Post the request and wait for the response
            var userRecurrence = await IssueRequestAsync<RecurrenceChangeResponse>(
                $"https://purchase.mp.microsoft.com/v8.0/b2b/recurrences/{request.RecurrenceId}/change",
                JsonConvert.SerializeObject(request),
                null);

            return userRecurrence;
        }
    }
}
