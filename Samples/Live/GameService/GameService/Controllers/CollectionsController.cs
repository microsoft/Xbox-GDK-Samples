//-----------------------------------------------------------------------------
// CollectionsController.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using GameService.Logging;
using GameService.Models;
using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using Microsoft.SimpleXboxDelegatedAuth;
using Microsoft.SimpleXboxSecureTokens;
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
    /// Example of doing a server to server (b2b) call from a web service
    /// to the collections service to query a user's purchased products.
    /// </summary>
    [Route("api/[controller]/[action]")]
    [ApiController]
    public class CollectionsController : GameServiceControllerBase
    {
        
        public CollectionsController( IHttpClientFactory HttpClientFactory,
                                      IConfiguration Config,
                                      IMemoryCache CertCache,
                                      ILogger<CollectionsController> Logger) : base(Config, HttpClientFactory, CertCache, Logger)
        {
        }

        /// <summary>
        /// Calls the v8 Collections service (b2bLicensePreview) to get information on the products that the user owns.
        /// </summary>
        /// <param name="TargetProductSkuIds">list of specific products to request in a '.' separated string of [ProductId]:[SkuId]</param>
        /// <param name="Market">Market results should be scoped to (E.g. US, FR, CA - https://dev.maxmind.com/geoip/legacy/codes/iso3166/ )</param>
        /// <returns></returns>
        [HttpGet]
        public async Task<ActionResult<string>> Query([FromQuery(Name = "ids")] string TargetProductSkuIds)
        {
            //  Must call this to get the cV for this call flow
            InitializeLoggingCv();
            //  Get the claims from the xsts auth token the middleware extracted
            var clientToken = (XstsClientToken)this.HttpContext.Items["XstsClaims"];
            var response = new StringBuilder();

            if (clientToken == null)
            {
                response.Append("Invalid XSTS token");
                return Unauthorized(response.ToString());
            }

            if (clientToken.Users.Count == 0)
            {
                response.Append("No users in XSTS token");
                return Unauthorized(response.ToString());
            }

            XUserClaims user = clientToken.Users.Find(x => x.UserHash == clientToken.UserHash);

            if (user == null)
            {
                response.AppendFormat("Unable to find claims for ush = {0}\n", clientToken.UserHash);
            }
            else
            {
                response.AppendFormat("Calling Collections service for user {0}\n", user.Gamertag);

                //  Build the Request's URI, headers, and body first
                var targetUri = new Uri("https://collections.mp.microsoft.com/v8.0/collections/b2bLicensePreview");

                //  Build the request's body
                var requestJson = new CollectionsRequest
                {
                    //  Page size in the sample is defaulted to 100, however you may want to adjust 
                    //  this in the XstsConstants.cs to better fit your own needs and performance
                    MaxPageSize = 100,
                    ExpandSatisfyingItems = true,        //  This expands the results to include any products that
                                                         //  are included in a bundle the user owns.
                    ExcludeDuplicates = true             //  Only include one result (entitlement) per item.
                };

                //  Default value for Market in the request is "neutral" to get all markets
                requestJson.Market = "neutral";

                //  Check if the caller wants us to look for specific products
                if (!string.IsNullOrEmpty(TargetProductSkuIds))
                {
                    response.AppendFormat("Looking for these specific products:\n");
                    var productSkuIds = new List<ProductSkuId>();
                    string[] formattedProducts = TargetProductSkuIds.Split('.');

                    foreach (string productSkuId in formattedProducts)
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
                    { "User-Agent", GameServiceConstants.ServiceName }, //  unique name to identify your service in logging
                    { XstsConstants.ContentTypeHeaderKey, "application/json" }
                };

                //  We need to get the Delegated Auth token to talk to the people service on-behalf-of the user
                var delegatedAuthHandler = new XstsDelegatedAuthHandler(mServerCache);

                //  Service Tokens (S-token) should be cached as they have a 14 day lifetime
                var sToken = await delegatedAuthHandler.GetCachedServiceTokenAsync();

                //  Get the Delegated Auth token for the user using the information in the x-token from the client and
                //  the S-token from the cache
                var delegatedAuthXtoken = await delegatedAuthHandler.CreateXstsAuthToken(targetUri,
                                                                                          clientToken.Sandbox,
                                                                                          sToken,
                                                                                          XstsTokenType.DelegatedAuth,
                                                                                          user.DelegationToken);

                //  Build the request and add all the headers before generating the signature and adding it to the request                

                var collectionsQueryRequest = new HttpRequestMessage(HttpMethod.Post, targetUri);
                collectionsQueryRequest.Headers.Add("Authorization", delegatedAuthXtoken.Token);
                collectionsQueryRequest.Headers.Add("User-Agent", GameServiceConstants.ServiceName);
                collectionsQueryRequest.Content = new ByteArrayContent(requestBodyContent);
                collectionsQueryRequest.Content.Headers.ContentType = new System.Net.Http.Headers.MediaTypeHeaderValue("application/json");

                var signature = delegatedAuthHandler.CreateRequestSignature(collectionsQueryRequest, sToken, delegatedAuthXtoken.SignaturePolicy);
                collectionsQueryRequest.Headers.Add("Signature", signature);

                //  Now make the request to the Collections service
                HttpResponseMessage httpResponse = null;
                using (var httpClient = mHttpClientFactory.CreateClient())
                {
                    httpResponse = await httpClient.SendAsync(collectionsQueryRequest);
                }

                //  Verify we got a response and parse it
                if (httpResponse != null)
                {
                    string responseBody = await httpResponse.Content.ReadAsStringAsync();
                    var userCollections = Newtonsoft.Json.JsonConvert.DeserializeObject<CollectionsResponse>(responseBody);

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
                        response.AppendFormat("  {0}  {1}  {2} acquired by {3}{4}\n",
                            item.productKind,
                            item.productId,
                            satisfyingEntitlements.ToString(),
                            AcquisitionTypeFriendlyName(item.acquisitionType),
                            consumableInfo.ToString());
                    }
                }
                else
                {
                    //  We got a response back, even if it was a failure it should give us the info
                    //  we need to log the error
                    response.AppendFormat("Error calling Collections service: {0}\n", httpResponse.StatusCode);
                }
            }

            var finalResponse = response.ToString();
            FinalizeLoggingCv();
            return finalResponse;
        }

        string AcquisitionTypeFriendlyName(string acquisitionType)
        {
            return (acquisitionType.ToLower()) switch
            {
                "single" => "purchase",
                "recurring" => "subscription",
                _ => acquisitionType,
            };
        }

        //  For additional functionality to call the Collections service, please see the 
        //  Microsoft.Store.Services sample on Github - 
        //  https://github.com/microsoft/Microsoft-Store-Services-Sample

    }
}
