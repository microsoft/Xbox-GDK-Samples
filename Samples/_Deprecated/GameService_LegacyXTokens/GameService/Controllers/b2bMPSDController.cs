//-----------------------------------------------------------------------------
// b2bMPSDController.cs
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

namespace GameService.MPSD
{
    /// <summary>
    /// SECTION 4 - Example of doing a server to server (b2b) call from our web service
    /// to the MPSD (Multilayer Session Directory using Service only auth
    /// (XSTS token obtained with only a Server Token)
    /// </summary>
    [Route("api/[controller]")]
    [ApiController]
    public class b2bMPSDController : XstsControllerBase
    {

        public b2bMPSDController(IHttpClientFactory HttpClientFactory,
                                    IConfiguration Config,
                                    IMemoryCache ServerCache,
                                    ILogger<b2bMPSDController> Logger) : base(Config, HttpClientFactory, ServerCache, Logger)
        {
        }

        /// <summary>
        /// Calls the MPSD service to get information about a session template
        /// </summary>
        /// <returns></returns>
        [HttpGet]
        public async Task<ActionResult<string>> Query([FromQuery(Name = "scid")] string Scid,
                                                      [FromQuery(Name = "sessionName")] string SessionName,
                                                      [FromQuery(Name = "templateName")] string TemplateName)
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

            //  Validate that we have the right parameters to make this call
            if (Scid == null)
            {
                response.AppendFormat("Missing query parameter {{scid}}\n");
                Response.StatusCode = (int)HttpStatusCode.BadRequest;
            }
            if (SessionName == null)
            {
                response.AppendFormat("Missing query parameter {{sessionName}}\n");
                Response.StatusCode = (int)HttpStatusCode.BadRequest;
            }
            if (TemplateName == null)
            {
                response.AppendFormat("Missing query parameter {{templateName}}\n");
                Response.StatusCode = (int)HttpStatusCode.BadRequest;
            }

            //  We had a bad request so exit here
            if (Response.StatusCode == (int)HttpStatusCode.BadRequest)
            {
                mLogger.MPSDInvalidRequest(mCv.Increment(), user.Pxuid, "Invalid MPSD request", Request.QueryString.Value);
                response.AppendFormat("Correct format is  api/b2bMPSD/query?scid=[SCID]&sessionName=[sessionName]&templateName=[templateName]\n");
                return response.ToString();
            }

            response.AppendFormat("Calling MPSD to get info on sessionTemplate {0} of SCID {1}\n", TemplateName, Scid);

            //  Build the Request's URI, headers, and body first
            var uri = new Uri(string.Format("https://{0}/serviceconfigs/{1}/sessionTemplates/{2}",
                XstsConstants.XblMPSDService,
                Scid,
                TemplateName,
                SessionName));

            Byte[] requestContent = new Byte[8];
            Array.Clear(requestContent, 0, requestContent.Length);
            //  All headers except for the Authorization and Signature headers
            //  which will be added on in the next step
            var requestHeaders = new NameValueCollection
            {
                { "x-xbl-contract-version", " 104" },
            };

            //  Make the request with service only auth
            MPSDResponse XblPeopleResponse = await MakeRequestWithServiceAuth<MPSDResponse>( uri,
                                                                                             HttpMethod.Get,
                                                                                             requestContent,
                                                                                             requestHeaders,
                                                                                             clientToken.Sandbox);

            
            response.Append(JsonConvert.SerializeObject(XblPeopleResponse));


            FinalizeLoggingCv();
            return response.ToString();
        }
    }
}