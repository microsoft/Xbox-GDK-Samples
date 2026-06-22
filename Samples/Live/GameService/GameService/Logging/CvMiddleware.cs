//-----------------------------------------------------------------------------
// CvMiddleware.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Http;
using Microsoft.CorrelationVector;
using System.Threading.Tasks;

namespace GameService.Logging
{
    // Extension method used to add the middleware to the HTTP request pipeline.
    public static class CvMiddlewareExtensions
    {
        public static IApplicationBuilder UseCvMiddleware(this IApplicationBuilder builder)
        {
            return builder.UseMiddleware<CvMiddleware>();
        }
    }

    /// <summary>
    /// Correlation Vector middleware
    /// The purpose of this middleware is to add a correlation vector (cV) to the response.
    /// The cV will be used in logging so that we can easily trace the logs for a single
    /// response through the system.  The cV is also helpful as Xbox Live services use this
    /// method for logging as well.  This makes tracking a request through your server and
    /// the Xbox Live services much easier and consistent.  More information about 
    /// Correlation Vectors and the Git repository can be found at the following link:
    /// https://github.com/Microsoft/CorrelationVector
    /// </summary>

    public class CvMiddleware
    {
        private readonly RequestDelegate mNext;

        public CvMiddleware(RequestDelegate next)
        {
            mNext = next;
        }

        /// <summary>
        /// This is the middleware entry point in the HTTP call processing
        /// </summary>
        /// <param name="httpContext"></param>
        /// <returns></returns>
        public async Task Invoke(HttpContext httpContext)
        {
            string cvHeader = httpContext.Request.Headers["MS-CV"];
            CorrelationVector cV;
            if (!string.IsNullOrEmpty(cvHeader))
            {
                //  This request has a cV header on it,
                //  we now extend the cV for this service's logging
                //  Ex: pDWfNQcD7Eqdr74xjZa0mg.0 -> pDWfNQcD7Eqdr74xjZa0mg.0.0
                cV = CorrelationVector.Extend(cvHeader);
            }
            else
            {
                //  This request does not have a cV header, create one for logging
                cV = new CorrelationVector(CorrelationVectorVersion.V2);
            }

            //  This allows the API controllers (or subsequent delegates in the flow) to access it.
            //  We also stamp our response with the cV here in case something goes wrong to ensure
            //  it gets back to the client for lookup later
            httpContext.Items["MS-CV"] = cV;
            httpContext.Response.Headers.Append("MS-CV", cV.Value);
            await mNext(httpContext);
        }
    }
}
