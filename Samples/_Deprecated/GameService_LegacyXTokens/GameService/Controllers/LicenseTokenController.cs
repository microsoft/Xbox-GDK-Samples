//-----------------------------------------------------------------------------
// LicenseTokenController.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Jose;
using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Caching.Memory;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using Microsoft.XboxSecureTokens;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading.Tasks;

namespace GameService.Controllers
{
    //  Classes were generated using http://json2csharp.com 
    //  Some member names were then changed by using the [JsonProperty] attribute
    //  for more clear usage in code.

    public class LicenseTokenRequest
    {
        public string LicenseToken { get; set; }
    }

    public class LicensableProduct
    {
        public DateTime endDate { get; set; }
        public bool isShared { get; set; }
        public string id { get; set; }
        public string productId { get; set; }
        public string skuId { get; set; }
        public string userId { get; set; }
    }

    public class LicenseTokenClaims
    {
        public string certificateId { get; set; }
        public string customDeveloperString { get; set; }
        public List<LicensableProduct> licensableProducts { get; set; }
        public string payload { get; set; }
        public int tokenVersion { get; set; }
    }

    public class LicenseToken
    {
        public LicenseTokenClaims Claims { get; set; }
        [JsonProperty("aud")] public string Audience { get; set; }
        [JsonProperty("iss")] public string Issuer { get; set; }
        [JsonProperty("nbf")] public int EpochNotBefore
        {
            get { return this.epochNotBefore; }

            set
            {
                //  So that we can use a DateTime object we will
                //  just do the conversion from Unix Epoch now
                var epoch = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);
                this.NotBefore = epoch.AddSeconds(value);
                this.epochNotBefore = value;
            }
        }
        [JsonProperty("exp")] public int EpochExpires
        {
            get { return this.epochExpires; }

            set
            {
                //  So that we can use a DateTime object we will
                //  just do the conversion from Unix Epoch now
                var epoch = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);
                this.Expires = epoch.AddSeconds(value);
                this.epochExpires = value;
            }
        }

        public string serializedJsonClaims { get; set; }
        [JsonProperty("LicenseTokenClaim")] public string EncodedClaims
        {
            get { return this.encodedClaims; }

            set
            {
                this.encodedClaims = value;

                //  Decode the value first and then sanatize the results
                string decodedClaims = Encoding.UTF8.GetString(Base64Url.Decode(value));

                //  There is extra data in the LicenseTokenClaim part of the json payload,
                //  we will trim those parts off and so we have serialized json to work with
                int claimsStartPos = decodedClaims.IndexOf('{');
                int claimsEndPos = decodedClaims.LastIndexOf('}') + 1;
                this.serializedJsonClaims = decodedClaims.Substring(claimsStartPos, (claimsEndPos - claimsStartPos));

                this.Claims = JsonConvert.DeserializeObject<LicenseTokenClaims>(serializedJsonClaims);
            }
        }

        public DateTime NotBefore { get; set; }
        public DateTime Expires { get; set; }
        private int epochNotBefore;
        private int epochExpires;
        private string encodedClaims;
    }

    [Route("api/[controller]")]
    [ApiController]
    public class LicenseTokenController : XstsControllerBase
    {

        public LicenseTokenController(IHttpClientFactory HttpClientFactory,
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
        [HttpPost]
        public async Task<ActionResult<string>> Post([FromBody] LicenseTokenRequest Request)
        {
            //  Must call this to get the cV for this call flow
            InitializeLoggingCv();
            StringBuilder response = new StringBuilder();

            if (!string.IsNullOrEmpty(Request.LicenseToken))
            {
                string[] TokenParts = Request.LicenseToken.Split('.');
                //  The 3 TokenParts of the token:
                //  [0] - JWE Protected Header       - This is a UTF8 byte array turned into a
                //                                     Base64URL string.  You need to decode 
                //                                     this to get info about which x5t was used
                //                                     to sign the token
                //  [1] - Payload
                //  [2] - JWE Authentication Tag     - Integrity value / signature

                //  [0] The first part of the token tells us the x5t that was used to generate
                //  the signature.
                string encodedHeader = TokenParts[0];

                //  decode the header
                string headerString = Encoding.UTF8.GetString(Base64Url.Decode(encodedHeader));

                //  create a dictionary out of the header
                Dictionary<string, object> headerDict = JsonConvert.DeserializeObject<Dictionary<string, object>>(
                    headerString,
                    new JsonSerializerSettings
                    {
                        MaxDepth = 2,
                        TypeNameHandling = TypeNameHandling.None
                    });

                //  the header tells you which cert to use to verify the signature via the x5t (Base64Url).
                string x5t = BitConverter.ToString(Base64Url.Decode((string)headerDict["x5t"])).Replace("-", string.Empty);
                string x5u = "https://licensing.mp.microsoft.com/v8.0/licenseToken/fullCertificate/";
                x5u += x5t;
                headerDict.Add("x5u", x5u);

                try
                {
                    XstsSigningCertController certController = 
                        new XstsSigningCertController(mConfig, 
                                                      mHttpClientFactory,
                                                      mServerCache,
                                                      mLogger,
                                                      mCv.Increment());
                    X509Certificate2 signingCert = 
                        await certController.GetSigningCert(headerDict);

                    string decoded = Jose.JWT.Decode(Request.LicenseToken,
                                                     signingCert.GetRSAPublicKey());

                    //  Token is authentic, otherwise we would have seen an exception.
                    //  now deserialize the JSON payload into our defined LicenseToken class
                    //  Note due to formatting there is extra work to be done on the claims inside
                    //  See the Set function for encodedClaims in LicenseToken
                    LicenseToken licenseToken = JsonConvert.DeserializeObject<LicenseToken>(decoded);

                    //  Insert your server logic for validating the other values of the license token such as the 
                    //  StoreID in licenseToken.Claims.LicensableProducts[] to valdiate it is the right
                    //  game or product.  You can also check the NotBefore and Expires DateTime values
                    //  to make sure it isn't stale.
                    response.Append(licenseToken.serializedJsonClaims);
                }
                catch (Exception e)
                {
                    mLogger.XstsException(mCv.Value, "License Token Signature invalid or unable to calculate", e);
                    throw;
                }
            }
            else
            {
                //  No authorization header, return a 403
                response.Append("No LicenseToken attached to the request.  Should be JSON form {\"LicenseToken\":\"[actualtoken]\"}");
            }

            var finalResponse = response.ToString();
            FinalizeLoggingCv();
            return finalResponse;
        }
    }
}