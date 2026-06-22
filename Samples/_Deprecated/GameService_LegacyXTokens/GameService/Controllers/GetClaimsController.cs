//-----------------------------------------------------------------------------
// GetClaimsController.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using Microsoft.XboxSecureTokens;
using System.Net.Http;
using System.Text;

namespace GameService.Controllers
{
    //  SECTION 1 - GetClaims API controller and initial endpoint to be used with sample
    //  Validates and extracts the claims in an XSTS Token sent up as part of the Authorization
    //  header and then replies back to the caller with the claims from within the token
    [Route("api/[controller]")]
    [ApiController]
    public class GetClaimsController : XstsControllerBase
    {
        public GetClaimsController( IHttpClientFactory HttpClientFactory,
                                    IConfiguration Config,
                                    IMemoryCache ServerCache,
                                    ILogger<GetClaimsController> Logger)
            : base(Config, HttpClientFactory, ServerCache, Logger)
        {
            //  We initialize this here instead of in the XstsControllerBase so that we can specify the
            //  specific name of each service to help sort out the logs
            mLogger = Logger;
        }

        // GET: api/<controller>
        [HttpGet]
        public ActionResult<string> Get()
        {
            //  Must call this to get the cV for this call flow
            InitializeLoggingCv();

            var clientToken = (XstsClientToken)this.HttpContext.Items["XstsClaims"];
            StringBuilder response = new StringBuilder();

            XUserClaims user = clientToken.Users.Find( x => x.UserHash == clientToken.UserHash);
            XDeviceClaims device = clientToken.Device;

            if(user == null)
            {
                response.AppendFormat("Unable to find claims for ush = {0}\n", clientToken.UserHash);
                mLogger.XstsUserNotFound("", clientToken.UserHash);
            }
            else
            { 
                response.AppendFormat("Claims found for ush = {0}\n", clientToken.UserHash);
                if (!string.IsNullOrEmpty(user.Pxuid))
                {
                    response.AppendFormat("Pxuid={0}\n", user.Pxuid);
                }
                if (!string.IsNullOrEmpty(user.UserPwid))
                {
                    response.AppendFormat("UserPwid={0}\n", user.UserPwid);
                }
                if (!string.IsNullOrEmpty(user.AgeGroup))
                {
                    response.AppendFormat("AgeGroup={0}\n", user.AgeGroup);
                }
                if (!string.IsNullOrEmpty(user.CountryByIP))
                {
                    response.AppendFormat("CountryByIP={0}\n", user.CountryByIP);
                }
                if (!string.IsNullOrEmpty(user.DelegationToken))
                {
                    response.AppendFormat("DelegationToken={0}\n", user.DelegationToken);
                }
                if (!string.IsNullOrEmpty(user.Gamertag))
                {
                    response.AppendFormat("Gamertag={0}\n", user.Gamertag);
                }
                if (!string.IsNullOrEmpty(user.PartnerUserFamilyId))
                {
                    response.AppendFormat("PartnerUserFamilyId={0}\n", user.PartnerUserFamilyId);
                }
                if (!string.IsNullOrEmpty(user.Privileges))
                {
                    response.AppendFormat("Privileges={0}\n", user.Privileges);
                }
                if (!string.IsNullOrEmpty(user.UserGroups))
                {
                    response.AppendFormat("UserGroups={0}\n", user.UserGroups);
                }
                if (!string.IsNullOrEmpty(user.UserHash))
                {
                    response.AppendFormat("UserHash={0}\n", user.UserHash);
                }
                if (!string.IsNullOrEmpty(user.UserTest))
                {
                    response.AppendFormat("UserTest={0}\n", user.UserTest);
                }
                if (!string.IsNullOrEmpty(device.DeviceType))
                {
                    response.AppendFormat("DeviceType={0}\n", device.DeviceType);
                }
                if (!string.IsNullOrEmpty(device.DeviceVersion))
                {
                    response.AppendFormat("DeviceVersion={0}\n", device.DeviceVersion);
                }
                if (!string.IsNullOrEmpty(device.DeviceGroups))
                {
                    response.AppendFormat("DeviceGroups={0}\n", device.DeviceGroups);
                }
                if (!string.IsNullOrEmpty(device.DevicePwid))
                {
                    response.AppendFormat("DevicePwid={0}\n", device.DevicePwid);
                }
            }

            var finalResponse = response.ToString();
            mLogger.ClaimsResponse(mCv.Increment(), user.Pxuid, finalResponse);

            FinalizeLoggingCv();
            return finalResponse;
        }
    }
}
