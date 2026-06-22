//-----------------------------------------------------------------------------
// GetGDPRListController.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using Microsoft.XboxSecureTokens;
using Newtonsoft.Json;
using System;
using System.Collections.Specialized;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;

namespace GameService.GDPR
{
    /// <summary>
    /// SECTION 4 - Example of doing a server to server (b2b) call from our web service
    /// to the GDPR Erasure Request List using Service only auth (XSTS token obtained 
    /// with only a Server Token)
    /// </summary>
    [Route("api/[controller]")]
    [ApiController]
    public class GetGDPRListController : XstsControllerBase
    {

        public GetGDPRListController(IHttpClientFactory HttpClientFactory,
                                    IConfiguration Config,
                                    IMemoryCache ServerCache,
                                    ILogger<GetGDPRListController> Logger) : base(Config, HttpClientFactory, ServerCache, Logger)
        {
        }

        /// <summary>
        /// Calls the GDPR erasure list service to get a list of users who have requested their data be removed
        /// in compliance with GDPR
        /// </summary>
        /// <returns></returns>
        [HttpGet]
        public async Task<ActionResult<string>> Get([FromQuery(Name = "StartDate")] string StartDate,
                                                    [FromQuery(Name = "EndDate")] string EndDate)
        {
            //  Must call this to get the cV for this call flow
            InitializeLoggingCv();         

            //  Get the claims from the xsts auth token our middleware extracted
            var clientToken = (XstsClientToken)this.HttpContext.Items["XstsClaims"];
            var response = new StringBuilder();

            XUserClaims user = clientToken.Users.Find(x => x.UserHash == clientToken.UserHash);

            if (user == null)
            {
                mLogger.XstsUserNotFound("", clientToken.UserHash);
                response.AppendFormat("Unable to find claims for ush = {0}\n", clientToken.UserHash);
                return response.ToString();
            }

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

            response.AppendFormat("Calling GDPR erasure list with\nStartDate {0}\nEndDate {1}\n", StartDate, EndDate);

            //  Build the Request's URI, headers, and body first
            var uri = new Uri(string.Format("https://{0}/ids?StartDate={1}&EndDate={2}",
                                            XstsConstants.GDPRListService,
                                            dtStart.ToString("s", System.Globalization.CultureInfo.InvariantCulture),
                                            dtEnd.ToString("s", System.Globalization.CultureInfo.InvariantCulture)));

            Byte[] requestContent = new Byte[8];
            Array.Clear(requestContent, 0, requestContent.Length);
            //  All headers except for the Authorization and Signature headers
            //  which will be added on in the next step
            var requestHeaders = new NameValueCollection
            {
                { "x-xbl-contract-version", " 104" },
            };

            //  Make the request with service only auth
            GDPRListResponse[] listResponse = await MakeRequestWithServiceAuth<GDPRListResponse[]>( uri,
                                                                                             HttpMethod.Get,
                                                                                             requestContent,
                                                                                             requestHeaders,
                                                                                             clientToken.Sandbox);

            if(listResponse == null)
            {
                response.Append("Error calling GDPR list endpoint, see server logs");
            }
            else if ( listResponse.Length > 0)
            {
                response.Append(JsonConvert.SerializeObject(listResponse));
            }
            else
            {
                response.Append("No results or bad formatted JSON returned.\n");
            }

            FinalizeLoggingCv();
            return response.ToString();
        }
    }
}