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
using Microsoft.SimpleXboxSecureTokens;
using System;
using System.Net.Http;
using System.Text;

namespace GameService.Controllers
{
    //  GetClaims API controller and initial endpoint to be used with sample
    //  Validates and extracts the claims in an XSTS Token sent up as part of the Authorization
    //  header and then replies back to the caller with the claims from within the token
    [Route("api/[controller]")]
    [ApiController]
    public class GetClaimsController : GameServiceControllerBase
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

        /// <summary>
        /// Handles an HTTP GET request to validate the XSTS authorization token and returns a formatted string
        /// containing user, device, and title claims if the token is valid.
        /// </summary>
        /// <remarks>This method expects the 'Authorization' header to contain a valid XSTS token. If the
        /// token is invalid or missing, the response will include error details and a warning will be logged. The
        /// returned string is intended for diagnostic or informational purposes and may include sensitive claim
        /// information; ensure appropriate handling in production environments.</remarks>
        /// <returns>A string containing details about the validated XSTS token, including user, device, and title claims. If the
        /// token is invalid or claims are missing, the string will include error information.</returns>
        /// <exception cref="Exception">Thrown if the Authorization header is missing or does not contain a valid XSTS token.</exception>
        [HttpGet]
        public ActionResult<string> Get()
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
            XDeviceClaims device = clientToken.Device;
            XTitleClaims title = clientToken.Title;

            if (user == null)
            {
                response.AppendFormat("Unable to find claims for ush = {0}\n", clientToken.UserHash);
                mLogger.LogWarning("Unable to find claims for ush = {0}\n", clientToken.UserHash);
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
                if (!string.IsNullOrEmpty(user.ModernGamertag))
                {
                    response.AppendFormat("ModernGamertag={0}\n", user.ModernGamertag);
                }
                if (!string.IsNullOrEmpty(user.ModernGamertagSuffix))
                {
                    response.AppendFormat("ModernGamertagSuffix={0}\n", user.ModernGamertagSuffix);
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
                if (device != null)
                {
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
                    if (!string.IsNullOrEmpty(device.DeveloperDeviceId))
                    {
                        response.AppendFormat("DeveloperDeviceId={0}\n", device.DeveloperDeviceId);
                    }
                    if (!string.IsNullOrEmpty(device.DeviceCapabilities))
                    {
                        response.AppendFormat("DeviceCapabilities={0}\n", device.DeviceCapabilities);
                    }
                }
                if (title != null)
                {
                    if (!string.IsNullOrEmpty(title.TitleId))
                    {
                        response.AppendFormat("TitleId={0}\n", title.TitleId);
                    }
                    if (!string.IsNullOrEmpty(title.TitleGroups))
                    {
                        response.AppendFormat("TitleId={0}\n", title.TitleGroups);
                    }
                    if (!string.IsNullOrEmpty(title.TitleVersion))
                    {
                        response.AppendFormat("TitleId={0}\n", title.TitleVersion);
                    }
                }
            }

            var finalResponse = response.ToString();
            mLogger.LogInformation(mCv.Increment(), finalResponse);

            FinalizeLoggingCv();
            return finalResponse;
        }
    }
}
