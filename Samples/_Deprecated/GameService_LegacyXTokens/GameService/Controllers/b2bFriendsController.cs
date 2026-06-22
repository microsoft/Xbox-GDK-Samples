//-----------------------------------------------------------------------------
// b2bfriendsController.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using GameService.Models;
using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using Microsoft.XboxSecureTokens;
using System;
using System.Collections.Specialized;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;

namespace GameService.Controllers
{
    /// <summary>
    /// SECTION 4 - Example of doing a server to server (b2b) call from our web service
    /// to an Xbox Live endpoint.  For this we will get the user's friends
    /// list
    /// </summary>
    [Route("api/[controller]")]
    [ApiController]
    public class b2bFriendsController : XstsControllerBase
    {

        public b2bFriendsController(IHttpClientFactory HttpClientFactory,
                                    IConfiguration Config,
                                    IMemoryCache ServerCache,
                                    ILogger<b2bFriendsController> Logger) : base(Config, HttpClientFactory, ServerCache, Logger)
        {
        }

        /// <summary>
        /// Calls the People service to get a user's friends list using Delegated Auth
        /// This API has some async operations in it, so we define it as async and use
        /// the await command to help improve server performance and load ability
        /// </summary>
        /// <returns></returns>
        [HttpGet]
        public async Task<ActionResult<string>> Get()
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
            }
            else
            {
                response.AppendFormat("Calling Friends service for user {0}\n", user.Gamertag);

                //  Build the Request's URI, headers, and body first
                var uri = new Uri(string.Format("https://{0}/users/me/people", XstsConstants.XblSocialService));
                Byte[] requestContent = new Byte[8];
                Array.Clear(requestContent, 0, requestContent.Length);

                //  All headers except for the Authorization and Signature headers
                //  which will be added on in the next step
                var requestHeaders = new NameValueCollection
                {
                    { "Example", "Example Header" }
                };

                //  Now pass these values to get the correct Delegated Auth and Signature headers for
                //  the request
                XblPeopleResponse XblPeopleResponse = 
                    await MakeRequestWithAuth<XblPeopleResponse>( user.Pxuid,
                                                                  uri,                     
                                                                  HttpMethod.Get,                  
                                                                  requestContent,                 
                                                                  requestHeaders,                                
                                                                  clientToken.Sandbox,                        
                                                                  user.DelegationToken);

                //  Because we will be appending multiple strings we are using a StringBuilder to improve
                //  performance as outlined in this article
                //  https://www.c-sharpcorner.com/article/tips-and-best-practices-to-improve-asp-net-web-application-performance/
                response.AppendFormat("{0} Friends found for {1}\n", XblPeopleResponse.people.Count, user.Gamertag);

                //  Parse the results to get the friends list
                foreach (Person friend in XblPeopleResponse.people)
                {
                    response.AppendFormat("  Xuid:{0}\n", friend.xuid);
                }
            }

            var finalResponse = response.ToString();
            mLogger.B2bFriendsResponse(mCv.Increment(), user.Pxuid, finalResponse);

            FinalizeLoggingCv();
            return finalResponse;
        }
    }
}