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
using Microsoft.SimpleXboxDelegatedAuth;
using Microsoft.SimpleXboxSecureTokens;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;

namespace GameService.Controllers
{
    /// <summary>
    /// Example of doing a server to server (b2b) call from our web service
    /// to an Xbox Live endpoint.  For this we will get the user's friends
    /// list
    /// </summary>
    [Route("api/[controller]")]
    [ApiController]
    public class B2bFriendsController : GameServiceControllerBase
    {
        public B2bFriendsController(IHttpClientFactory HttpClientFactory,
                                    IConfiguration Config,
                                    IMemoryCache ServerCache,
                                    ILogger<B2bFriendsController> Logger) : base(Config, HttpClientFactory, ServerCache, Logger)
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

            //  Get the claims from the xsts auth token the middleware extracted
            var clientToken = (XstsClientToken)this.HttpContext.Items["XstsClaims"];
            var response = new StringBuilder();

            if(clientToken == null)
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
                response.AppendFormat("Calling Friends service for user {0}\n", user.Gamertag);

                var targetUri = new System.Uri("https://social.xboxlive.com/users/me/people");

                //  We need to get the Delegated Auth token to talk to the people service on-behalf-of the user
                var delegatedAuthHandler = new XstsDelegatedAuthHandler(mServerCache);

                //  Service Tokens (S-token) should be cached as they have a 14 day lifetime
                var sToken = await delegatedAuthHandler.GetCachedServiceTokenAsync();
                var test = await delegatedAuthHandler.GetCachedServerAuthXToken(targetUri, clientToken.Sandbox);

                //  Get the Delegated Auth token for the user using the information in the x-token from the client and
                //  the S-token from the cache
                var delegatedAuthXtoken = await delegatedAuthHandler.CreateXstsAuthToken( targetUri,
                                                                                          clientToken.Sandbox, 
                                                                                          sToken, 
                                                                                          XstsTokenType.DelegatedAuth, 
                                                                                          user.DelegationToken );



                //  Build the request and add all the headers before generating the signature and adding it to the request                
                var friendsRequest = new HttpRequestMessage(HttpMethod.Get, targetUri);
                friendsRequest.Headers.Add("Authorization", delegatedAuthXtoken.Token);
                friendsRequest.Headers.Add("MS-CV", mCv.Increment());
                mCv.Increment();
                var signature = delegatedAuthHandler.CreateRequestSignature(friendsRequest, sToken, delegatedAuthXtoken.SignaturePolicy);
                friendsRequest.Headers.Add("Signature", signature);

                //  Now make the request to the People service
                HttpResponseMessage httpResponse = null;
                using (var httpClient = mHttpClientFactory.CreateClient())
                {
                    httpResponse = await httpClient.SendAsync(friendsRequest);
                }

                //  Check the response and then parse it
                if (httpResponse != null)
                {
                    string responseBody = await httpResponse.Content.ReadAsStringAsync();
                    var peopleResponse = Newtonsoft.Json.JsonConvert.DeserializeObject<XblPeopleResponse>(responseBody);

                    //  Because we will be appending multiple strings we are using a StringBuilder to improve
                    //  performance as outlined in this article
                    //  https://www.c-sharpcorner.com/article/tips-and-best-practices-to-improve-asp-net-web-application-performance/
                    response.AppendFormat("{0} Friends found for {1}\n", peopleResponse.people.Count, user.Gamertag);

                    //  Parse the results to get the friends list
                    foreach (Person friend in peopleResponse.people)
                    {
                        response.AppendFormat("  Xuid:{0}\n", friend.xuid);
                    }
                }
                else
                {
                    response.AppendFormat("No friends found for {0}\n", user.Gamertag);
                }             
            }

            var finalResponse = response.ToString();
            
            FinalizeLoggingCv();
            return finalResponse;
        }
    }
}