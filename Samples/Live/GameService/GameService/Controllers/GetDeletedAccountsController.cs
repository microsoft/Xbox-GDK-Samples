//-----------------------------------------------------------------------------
// GetDeletedAccountsController.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using GameService.Models;
using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using Microsoft.SimpleXboxDelegatedAuth;
using Microsoft.SimpleXboxSecureTokens;
using Newtonsoft.Json;
using System;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;

namespace GameService.DeletedAccounts
{
    /// <summary>
    /// Example of doing a server to server call to the DeletedAccounts Erasure Request list
    /// using Service only auth (XSTS token obtained with only a Server Token)
    /// </summary>
    [Route("api/[controller]")]
    [ApiController]
    public class GetDeletedAccountsController : GameServiceControllerBase
    {

        public GetDeletedAccountsController(IHttpClientFactory HttpClientFactory,
                                            IConfiguration Config,
                                            IMemoryCache ServerCache,
                                            ILogger<GetDeletedAccountsController> Logger) : base(Config, HttpClientFactory, ServerCache, Logger)
        {
            //  We initialize this here instead of in the XstsControllerBase so that we can specify the
            //  specific name of each service to help sort out the logs
            mLogger = Logger;
        }

        /// <summary>
        /// Calls the DeletedAccounts erasure list service to get a list of users who have requested their data be removed
        /// in compliance with DeletedAccounts
        /// </summary>
        /// <returns></returns>
        [HttpGet]
        public async Task<ActionResult<string>> Get([FromQuery(Name = "StartDate")] string StartDate,
                                                    [FromQuery(Name = "EndDate")] string EndDate)
        {
            //  Must call this to get the cV for this call flow
            InitializeLoggingCv();

            //  This call is purely server to server and doesn't need a client identity so we don't
            //  validate any client token here.  You can comment out the app.UseXstsAsymmetricRFC7516Middleware()
            //  call in Startup.cs to skip client token validation for this controller when testing it.
            StringBuilder response = new();

            DateTime currentDate = new DateTime(DateTime.Now.Ticks); 
            DateTime dtStart = DateTime.UtcNow.AddDays(-1);
            DateTime dtEnd = DateTime.Now;

            //  Validate that we have the right parameters to make this call
            if (StartDate == null || !DateTime.TryParse(StartDate, out dtStart))
            {
                response.AppendFormat("No StartDate specified, using {0}\n", dtStart.ToString());
            }
            
            if (EndDate == null || !DateTime.TryParse(EndDate, out dtEnd))
            {
                response.AppendFormat("No EndDate specified, using {0}\n", EndDate);
            }

            response.AppendFormat("Calling Deleted Accounts erasure list with\nStartDate {0}\nEndDate {1}\n", StartDate, EndDate);

            //  Build the Request's URI, headers, and body first
            var targetUri = new Uri(string.Format("https://{0}/ids?StartDate={1}&EndDate={2}",
                                    "deletedaccounts.xboxlive.com",
                                    dtStart.ToString("s", System.Globalization.CultureInfo.InvariantCulture),
                                    dtEnd.ToString("s", System.Globalization.CultureInfo.InvariantCulture)));

            //  We need to get the Service Auth token to talk to the DeletedAccounts service
            var delegatedAuthHandler = new XstsDelegatedAuthHandler(mServerCache);
            var sToken = await delegatedAuthHandler.GetCachedServiceTokenAsync();
            var serviceAuthXtoken = await delegatedAuthHandler.CreateXstsAuthToken( targetUri,
                                                                                    XstsConstants.ProductionSandbox,
                                                                                    sToken,
                                                                                    XstsTokenType.ServiceAuth);

            var deletedAccountsRequest = new HttpRequestMessage(HttpMethod.Get, targetUri);
            deletedAccountsRequest.Headers.Add("Authorization", serviceAuthXtoken.Token);
            var signature = delegatedAuthHandler.CreateRequestSignature(deletedAccountsRequest, sToken, serviceAuthXtoken.SignaturePolicy);
            deletedAccountsRequest.Headers.Add("Signature", signature);
            deletedAccountsRequest.Headers.Add("x-xbl-contract-version", "104");

            //  Now make the request to the DeletedAccounts service
            HttpResponseMessage httpResponse = null;
            using (var httpClient = mHttpClientFactory.CreateClient())
            {
                httpResponse = await httpClient.SendAsync(deletedAccountsRequest);
            }

            if (httpResponse != null)
            {
                //  Parse the response
                string responseBody = await httpResponse.Content.ReadAsStringAsync();
                var deletedAccountsResponse = Newtonsoft.Json.JsonConvert.DeserializeObject<DeletedAccountsResponse[]>(responseBody);

                if (deletedAccountsResponse == null)
                {
                    response.Append("Error calling Deleted Accounts endpoint, see server logs");
                }
                else if (deletedAccountsResponse.Length > 0)
                {
                    response.Append(JsonConvert.SerializeObject(deletedAccountsResponse));
                }
                else
                {
                    response.Append("No results or bad formatted JSON returned.\n");
                }
            }
            else
            {
                response.AppendFormat("No deleted accounts returned\n");
            }

            FinalizeLoggingCv();
            return response.ToString();
        }
    }
}