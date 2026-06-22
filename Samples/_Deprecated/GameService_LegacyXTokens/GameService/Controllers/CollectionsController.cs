//-----------------------------------------------------------------------------
// CollectionsController.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using Microsoft.XboxSecureTokens;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;

namespace GameService.Collections
{
    /// <summary>
    /// SECTION 5 - Collections controller and endpoint
    /// Example of doing a server to server (b2b) call from our web service
    /// to the collections service to query a user's purchased products or
    /// fulfill consumables.
    /// </summary>
    [Route("api/[controller]/[action]")]
    [ApiController]
    public class CollectionsController : XstsControllerBase
    {
        
        public CollectionsController( IHttpClientFactory HttpClientFactory,
                                      IConfiguration Config,
                                      IMemoryCache CertCache,
                                      ILogger<CollectionsController> Logger) : base(Config, HttpClientFactory, CertCache, Logger)
        {
        }

        /// <summary>
        /// Calls the Collections service to get information on the products that the
        /// user owns.
        /// </summary>
        /// <param name="TargetProductSkuIds">list of specific products to request in a '.' separated string of [ProductId]:[SkuId]</param>
        /// <param name="Market">Market results should be scoped to (Eg. US, FR, CA - https://dev.maxmind.com/geoip/legacy/codes/iso3166/ )</param>
        /// <returns></returns>
        [HttpGet]
        public async Task<ActionResult<string>> Query([FromQuery(Name = "ids")] string TargetProductSkuIds,
                                                      [FromQuery(Name = "market")] string Market)
        {
            //  Must call this to get the cV for this call flow
            InitializeLoggingCv();
            var response = new StringBuilder();

            //  Get the claims from the xsts auth token our middleware extracted
            var clientToken = this.HttpContext.Items["XstsClaims"] as XstsClientToken;
            if (clientToken == null)
            {
                response.Append("Token Error");
                mLogger.XstsWarning(mCv.Increment(), "XstsClaims object not found in HttpContext");
                return response.ToString();
            }


            XUserClaims user = clientToken.Users.Find(x => x.UserHash == clientToken.UserHash);

            if (user == null)
            {
                mLogger.XstsUserNotFound(mCv.Increment(), clientToken.UserHash);
                response.AppendFormat("Unable to find claims for user hash = {0}\n", clientToken.UserHash);
                return response.ToString();
            }
            
            response.AppendFormat("Calling Collections service for user {0}\n", user.Gamertag);

            //  Build the Request's URI, headers, and body first
            var uri = new Uri("https://collections.mp.microsoft.com/v8.0/collections/b2bLicensePreview");

            //  Build the request's body
            var requestJson = new CollectionsRequest();

            //  Page size in the sample is defaulted to 100, however you may want to adjust 
            //  this in the XstsConstants.cs to better fit your own needs and performance
            requestJson.MaxPageSize = XstsConstants.CollectionsPageSize;

            requestJson.ExpandSatisfyingItems = true;        //  This expands the results to include any products that
                                                             //  are included in a bundle the user owns.
            requestJson.ExcludeDuplicates = true;            //  Only include one result (entitlement) per item.

            //  The store market you want the results to be scoped to.
            //  Default if no value, will be "neutral"
            if (Market == null)
            {
                requestJson.Market = "neutral";
            }
            else
            {
                requestJson.Market = Market;
            }

            //  Check if the caller wants us to look for specific products
            if (!string.IsNullOrEmpty(TargetProductSkuIds))
            {
                response.AppendFormat("Looking for these specific products:\n");
                var productSkuIds = new List<ProductSkuId>();
                string[] formattedProducts = TargetProductSkuIds.Split('.');

                foreach (string productSkuId in formattedProducts )
                {
                    response.AppendFormat("    {0}\n", productSkuId);
                    string[] splitProductIds = productSkuId.Split(':');
                    if (splitProductIds.Length == 2)
                    {
                        productSkuIds.Add(new ProductSkuId()
                        {
                            ProductId = splitProductIds[0],
                            SkuId = splitProductIds[1]
                        });
                    }
                    else
                    {
                        mLogger.LogWarning(mCv.Increment(), $"Invalid ProductSkuID passed from caller {productSkuId}");
                    }
                }

                requestJson.ProductSkuIds = productSkuIds;
            }

            //  Filter our results to include these product types
            requestJson.EntitlementFilters = new List<string>() {
                                                CollectionsFilters.Game,
                                                CollectionsFilters.Consumable,
                                                CollectionsFilters.Durable
                                                };

            // Serialize our request body to a UTF8 byte array
            string requestBodyString = JsonConvert.SerializeObject(requestJson);
            byte[] requestBodyContent = System.Text.Encoding.UTF8.GetBytes(requestBodyString);

            //  All headers except for the Authorization and Signature headers
            //  which will be added on in the next step
            var requestHeaders = new NameValueCollection
            {
                { "User-Agent", XstsConstants.ServiceName }, //  unique name to identify your service in logging
                { XstsConstants.ContentTypeHeaderKey, "application/json" }
            };

            //  Now pass these values to get the correct Delegated Auth and Signature headers for
            //  the request
            //  Post the request and wait for the response
            CollectionsResponse userCollections = 
                await this.MakeRequestWithAuth<CollectionsResponse>(user.Pxuid,
                                                                    uri,
                                                                    HttpMethod.Post,
                                                                    requestBodyContent,
                                                                    requestHeaders,
                                                                    clientToken.Sandbox,
                                                                    user.DelegationToken);

            foreach (var item in userCollections.Items)
            {
                var satisfyingEntitlements = new StringBuilder();
                var consumableInfo = new StringBuilder();

                //  Check if this is enabled because of a satisfying entitlement from a bundle or subscription
                if (item.satisfiedByProductIds.Any())
                {
                    satisfyingEntitlements.Append(" enabled by satisfying entitlement(s) from ");
                    foreach (var parent in item.satisfiedByProductIds)
                    {
                        satisfyingEntitlements.Append($"{parent}, ");
                    }
                }

                if (item.productKind == "Consumable")
                {
                    consumableInfo.AppendFormat(" with a balance remaining of {0}", item.quantity);
                }

                //  [Product type] [StoreID (from Partner Center)] ([Product ID from XDP]) [Satisfying entitlements] acquired by [acquisition type]
                response.AppendFormat("  {0}  {1} ({2}) {3} acquired by {4}{5}\n",
                    item.productKind,
                    item.productId,
                    item.legacyProductId,
                    satisfyingEntitlements.ToString(),
                    AcquisitionTypeFriendlyName(item.acquisitionType),
                    consumableInfo.ToString());
                
            }
            var finalResponse = response.ToString();
            mLogger.QueryResponse(mCv.Increment(), user.Pxuid, finalResponse);
            FinalizeLoggingCv();
            return finalResponse;
        }

        string AcquisitionTypeFriendlyName(string acquisitionType)
        {
            switch (acquisitionType.ToLower())
            {
                case "single": return "purchase";
                case "recurring": return "subscription";
                default: return acquisitionType;
            }
        }

        /// <summary>
        /// Consumes the specified quantity from the user's balance of that consumable
        /// </summary>
        /// <param name="ProductId">Product to be consumed</param>
        /// <param name="Quantity">Quantity to consume</param>
        /// <returns></returns>
        [HttpGet]
        public async Task<ActionResult<string>> Consume([FromQuery(Name = "id")] string ProductId,
                                                        [FromQuery(Name = "quantity")] int Quantity)
        {
            //  Must call this to get the cV for this call flow
            InitializeLoggingCv();
            var response = new StringBuilder("");

            bool err = false;

            //  Validate that we have a productID and a quantity first
            if (ProductId == null)
            {
                response.AppendFormat("Missing query parameter {{id}}\n");
                Response.StatusCode = (int)HttpStatusCode.BadRequest;
                err = true;
            }

            if (Quantity <= 0)
            {
                response.AppendFormat("Missing or invalid query parameter {{Quantity}} of value {0}\n", Quantity);
                Response.StatusCode = (int)HttpStatusCode.BadRequest;
                err = true;
            }

            //  Get the claims from the xsts auth token our middleware extracted
            var clientToken = this.HttpContext.Items["XstsClaims"] as XstsClientToken;
            if (clientToken == null)
            {
                response.Append("Token Error");
                mLogger.XstsWarning(mCv.Increment(), "XstsClaims object not found in HttpContext");
                Response.StatusCode = (int)HttpStatusCode.BadRequest;
                err = true;
            }

            XUserClaims user = clientToken.Users.Find(x => x.UserHash == clientToken.UserHash);
            if (user == null)
            {
                mLogger.XstsUserNotFound(mCv.Increment(), clientToken.UserHash);
                response.AppendFormat("Unable to find claims for ush = {0}\n", clientToken.UserHash);
                return response.ToString();
            }

            //  We had a bad request so exit here
            if (err)
            {
                mLogger.CollectionsInvalidRequest(mCv.Increment(), user.Pxuid, "Invalid consume request", Request.QueryString.Value);
                response.AppendLine("Correct format is  api/collections/consume?id=[StoreID]&quantity=[number]");
                return response.ToString();
            }
            


            
            

            
                       
            //  Build the Request's URI, headers, and body first
            var uri = new Uri("https://collections.mp.microsoft.com/v8.0/collections/consume");

            //  Build the request's body
            var requestParams = new ConsumeRequest()
            {
                RemoveQuantity = Quantity,
                ProductId = ProductId,
                TrackingId = Guid.NewGuid().ToString(),
                //  Save this request into our pending transaction database
                User = user.Pxuid    //  Pxuid is not needed for the actual consume call,
                                     //  but since we are storing this data structure in
                                     //  the pending transaction database we will need
                                     //  to know who the transaction was on behalf of
                                     //  and when we retry give the user credit for it

            };
                                                                    
            using (var dbContext = GameServicePersistentDBController.CreateDbContext(mConfig, mCv, mLogger))
            {
                await dbContext.PendingConsumeRequests.AddAsync(requestParams);
                dbContext.SaveChanges();
            }
            mLogger.AddPendingTransaction(mCv.Increment(),
                               requestParams.User,
                               requestParams.TrackingId,
                               requestParams.ProductId,
                               requestParams.RemoveQuantity);
            response.AppendLine(await ConsumeProductForUser(requestParams, clientToken.Sandbox, user));

            var finalResponse = response.ToString();
            mLogger.ConsumeResponse(mCv.Increment(), user.Pxuid, finalResponse);

            FinalizeLoggingCv();
            return finalResponse;
        }

        /// <summary>
        /// Looks for any pending transactions of consumables that have not completed for this user
        /// then retries them
        /// </summary>
        /// <returns></returns>
        [HttpGet]
        public async Task<ActionResult<string>> RetryPendingConsumes()
        {
            //  Must call this to get the cV for this call flow
            InitializeLoggingCv();

            var response = new StringBuilder("");

            //  Get the claims from the xsts auth token our middleware extracted
            var clientToken = (XstsClientToken)this.HttpContext.Items["XstsClaims"];
            XUserClaims user = clientToken.Users.Find(x => x.UserHash == clientToken.UserHash);

            if (user == null)
            {
                mLogger.XstsUserNotFound(mCv.Increment(), clientToken.UserHash);
                response.AppendFormat("Unable to find claims for ush = {0}\n", clientToken.UserHash);
                return response.ToString();
            }

            response.AppendFormat("Finding all pending consume calls for user {0}...\n",
                                    user.Gamertag);
            List<ConsumeRequest> pendingUserConsumeRequests = null;

            using (var dbContext = GameServicePersistentDBController.CreateDbContext(mConfig, mCv, mLogger))
            {
                pendingUserConsumeRequests = await dbContext.PendingConsumeRequests
                    .Where(b => b.User == user.Pxuid).ToListAsync();
                dbContext.SaveChanges();
            }

            response.AppendFormat("Found {0} pending consume request(s) to complete or verify...\n", pendingUserConsumeRequests.Count);
            foreach (ConsumeRequest currentRequest in pendingUserConsumeRequests)
            {
                response.Append(await ConsumeProductForUser(currentRequest, clientToken.Sandbox, user)); 
            }

            var finalResponse = response.ToString();
            mLogger.RetryPendingConsumesResponse(mCv.Increment(), finalResponse);

            FinalizeLoggingCv();
            return finalResponse;
        }

        private async Task<string> ConsumeProductForUser(ConsumeRequest CurrentRequest,
                                                                      string SandboxId,
                                                                      XUserClaims User)
        {


            var response = new StringBuilder("");
            response.AppendFormat("Calling Collections service to consume {0} from user {1}'s balance of {2}...\n",
                                CurrentRequest.RemoveQuantity,
                                User.Gamertag,
                                CurrentRequest.ProductId);

            //  Build the Request's URI, headers, and body first
            var uri = new Uri("https://collections.mp.microsoft.com/v8.0/collections/consume");

            // Serialize our request body to a UTF8 byte array
            string requestBodyString = JsonConvert.SerializeObject(CurrentRequest);
            byte[] requestBodyContent = System.Text.Encoding.UTF8.GetBytes(requestBodyString);

            //  All headers except for the Authorization and Signature headers
            //  which will be added on in the next step
            var requestHeaders = new NameValueCollection
            {
                { "User-Agent", XstsConstants.ServiceName }, //  unique name to identify your service in logging
                { XstsConstants.ContentTypeHeaderKey, "application/json" },
            };

            //  Now pass these values to get the correct Delegated Auth and Signature headers for
            //  the request
            // Post the request and wait for the response
            try
            {
                var httpResponse = await MakeRequestWithAuth(   User.Pxuid,
                                                                uri,
                                                                HttpMethod.Post,
                                                                requestBodyContent,
                                                                requestHeaders,
                                                                SandboxId,
                                                                User.DelegationToken);

                //  We got a response back, even if it was a failure it should give us the info
                //  we need 
                using (var dbContext = GameServicePersistentDBController.CreateDbContext(mConfig, mCv, mLogger))
                {
                    dbContext.PendingConsumeRequests.Remove(CurrentRequest);
                    dbContext.SaveChanges();                    
                }
                mLogger.RemovePendingTransaction(mCv.Increment(),
                                                      CurrentRequest.User,
                                                      CurrentRequest.TrackingId,
                                                      CurrentRequest.ProductId,
                                                      CurrentRequest.RemoveQuantity);

                string responseBody = await httpResponse.Content.ReadAsStringAsync();
                if (httpResponse.IsSuccessStatusCode)
                {
                    ConsumeResponse consumeResponse = JsonConvert.DeserializeObject<ConsumeResponse>(responseBody);

                    //  Give the user credit for the item 
                    //  consumed within your own entitlement
                    //  or item tracking system here
                    
                    response.AppendFormat( "Consumed {0}, user's remaining balance is {1} for consumable {2}\n",
                                            CurrentRequest.RemoveQuantity,
                                            consumeResponse.newQuantity,
                                            CurrentRequest.ProductId);
                }
                else
                {
                    //  The consume request failed for some reason but we should have 
                    //  the reason in the response body
                    ConsumeError consumeError = JsonConvert.DeserializeObject<ConsumeError>(responseBody);

                    if(consumeError.code == "BadRequest" &&
                        consumeError.innererror.code.Equals("InsufficientConsumeQuantity"))
                    {
                        //  Request was good, but the user does not have enough of the consumable
                        //  left in their balance to consume the quantity from the request
                        response.AppendFormat(  "User does not have enough balance to consume {0}\n",
                                                CurrentRequest.RemoveQuantity);

                        mLogger.ConsumeError(mCv.Value,  
                                             CurrentRequest.TrackingId,
                                             CurrentRequest.User,
                                             CurrentRequest.ProductId,
                                             CurrentRequest.RemoveQuantity,
                                             "Insufficient balance",
                                             null);
                                                      
                    }
                    else
                    {
                        //  Some other error occurred
                        response.AppendFormat(  "Unexpected error during consume: {0}\n{1}\n{2}\n",
                                                consumeError.code,
                                                consumeError.innererror.code,
                                                consumeError.innererror.message);

                        mLogger.ConsumeError(mCv.Value,
                                             CurrentRequest.TrackingId,
                                             CurrentRequest.User,
                                             CurrentRequest.ProductId,
                                             CurrentRequest.RemoveQuantity,
                                             responseBody,
                                             null);
                    }
                }
            }
            catch (Exception e)
            {
                response.AppendFormat("Error trying to consume the product, please retry.\n{0}\n", e.Message);
                mLogger.ConsumeError(mCv.Value,
                                     CurrentRequest.TrackingId,
                                     CurrentRequest.User,
                                     CurrentRequest.ProductId,
                                     CurrentRequest.RemoveQuantity,
                                     "Insufficient balance",
                                     e);
            }

            FinalizeLoggingCv();
            return response.ToString();
            
        }
    }
}
