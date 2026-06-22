//-----------------------------------------------------------------------------
// XstsAsymmetricRFC7516Middleware.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.Caching.Memory;
using System;
using System.Net;
using System.Threading.Tasks;

/// <summary>
/// NOTE -  This sample is meant to help you understand and begin working with X-tokens
///         on your own Game Services.  However, the sample may not be hardened against
///         specific attacks or threats associated with web service development.  We 
///         encourage you to do your own security and attack mitigation if using this 
///         sample as a base to begin working on your own services.
/// </summary>

namespace Microsoft.SimpleXboxSecureTokens
{
    // Extension method used to add the middleware to the HTTP request pipeline.
    public static class XstsAsymmetricRFC7516MiddlewareExtensions
    {
        public static IApplicationBuilder UseXstsAsymmetricRFC7516Middleware(this IApplicationBuilder builder)
        {
            return builder.UseMiddleware<XstsAsymmetricRFC7516Middleware>();
        }
    }

    /// <summary>
    /// The purpose of this middleware is to validate an XSTS token using Asymmetric key auth and JWE RFC7516
    /// format. This is the recommended token configuration for titles using Xbox Live.
    /// For information about Middleware in ASP.NET see the following
    /// https://docs.microsoft.com/en-us/aspnet/core/fundamentals/middleware/?view=aspnetcore-2.1&tabs=aspnetcore2x
    /// </summary>
    public class XstsAsymmetricRFC7516Middleware
    {
        private readonly RequestDelegate mNext;
        private readonly IMemoryCache mServerCache;

        public XstsAsymmetricRFC7516Middleware(RequestDelegate next,
                                               IMemoryCache ServerCache)
        {
            mServerCache = ServerCache;
            mNext = next;       
        }

        /// <summary>
        /// This is the middleware entry point in the HTTP call processing
        /// </summary>
        /// <param name="httpContext"></param>
        /// <returns></returns>
        public async Task Invoke(HttpContext httpContext)
        {
            string? httpsAuthHeader = httpContext.Request.Headers["Authorization"];
            if (string.IsNullOrEmpty(httpsAuthHeader))
            {
                var message = "No XSTS token in Authorization headers";
                throw new Exception(message);
            }

            if (!httpsAuthHeader.StartsWith("XBL3.0"))
            {
                //  No authorization header, return a 403
                httpContext.Response.StatusCode = (int)HttpStatusCode.Forbidden;
                await httpContext.Response.WriteAsync("Access Denied: No auth header");
            }
            else
            {
                XstsClientToken? clientToken = null;
                try
                {
                    XstsTokenHandler tokenHandler = new(mServerCache);

                    //  Validate the token and get the claims
                    var clientTokenTask = tokenHandler.ValidateAuthorizationHeaderWithCache(httpsAuthHeader);
                    clientTokenTask.Wait();

                    clientToken = clientTokenTask.Result;
                }
                catch (Exception)
                {
                    //  Token is invalid, return a 403
                    httpContext.Response.StatusCode = (int)HttpStatusCode.Forbidden;
                    await httpContext.Response.WriteAsync("Access Denied: XSTS token invalid");
                }

                //  Check the lifetime of the token to verify it is still valid or if a new token from the client is needed
                //  If the token is expired, send a 401 for the client to get a new token.
                if (clientToken != null &&
                    !XstsTokenHandler.TokenLifetimeIsValid(clientToken.NotBefore, clientToken.Expires))
                {
                    httpContext.Response.StatusCode = (int)HttpStatusCode.Unauthorized;
                    await httpContext.Response.WriteAsync("Access Denied: XSTS token expired");
                }

                //  Token is valid, add the claims to the HttpContext so that it can be used by the service
                httpContext.Items["XstsClaims"] = clientToken;
                
                //  Call the next middleware in the pipeline
                await mNext(httpContext);
            }
        }
    }
}
