//-----------------------------------------------------------------------------
// XblEndpointsController.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Logging;
using System;
using System.Linq;
using System.Threading.Tasks;

namespace Microsoft.XboxSecureTokens.XstsDelegatedAuth
{
    //  SECTION 2 - Endpoints initialization and controller
    public class XblEndpointsController
    {
        private ILogger mLogger;
        private Microsoft.CorrelationVector.CorrelationVector mCv;

        public XblEndpointsController(ILogger Logger, string CorrelationVectorString)
        {
            mCv = CorrelationVector.CorrelationVector.Extend(CorrelationVectorString);
            mLogger = Logger;
        }

        //  In-memory database cache for endpoints and signature policies data
        //  For better performance and best practice we do not hold the context or auto create
        //  a context to the db when the API is called.  Instead we create the context, use it
        //  and then release it as quick as possible so that our server can handle more load
        //  and incoming calls are not stuck waiting for a locked, but not used context.
        //
        //  using (var dbContext = CreateDbContext())
        //  {
        //      DB read / write requests
        //  }
        //
        //  See the following articles:
        //  Multi-threading and the Entity Framework - https://social.msdn.microsoft.com/Forums/en-US/e5cb847c-1d77-4cd0-abb7-b61890d99fae/multithreading-and-the-entity-framework?forum=adodotnetentityframework
        //  C# working with Entity Framework in a multi threaded server - https://stackoverflow.com/questions/9415955/c-sharp-working-with-entity-framework-in-a-multi-threaded-server
        //  One DbContext per web request… why? - https://stackoverflow.com/questions/10585478/one-dbcontext-per-web-request-why

        //  Create the dbContext for the Relying Party needed to decrypt the token
        private static EndpointAndSignatureContext CreateEndpointDbContext()
        {
            var optionsBuilder = new DbContextOptionsBuilder<EndpointAndSignatureContext>();
            optionsBuilder.UseInMemoryDatabase("EndpointsDb");
            return new EndpointAndSignatureContext(optionsBuilder.Options);
        }

        public async Task<SignaturePolicy> GetSignaturePolicyAsync(XblEndpoint Target)
        {
            ServiceSignaturePolicy result = null;
            using (var dbContext = CreateEndpointDbContext())
            {
                result = await dbContext.SignaturePolicies
                                //  NOTE - We are checking for the PolicyIndex +1 because when writing the 
                                //  Signature policies to the DB the it was always overriding the index of "0"
                                //  to be 1, so we had to shift their IDs all by 1 vs the index that came 
                                //  down from the endpoints server in JSON.
                                .FirstOrDefaultAsync(b => b.SignatureID == Target.SignaturePolicyIndex + 1);
            }
            return result.SignaturePolicy;
        }

        public async Task<XblEndpoint> GetXblEndpointByFQDNAsync(string Domain)
        {
            XblEndpoint result = null;

            using (var dbContext = CreateEndpointDbContext())
            {
               result = await dbContext.Endpoints
                                .FirstOrDefaultAsync(b => Domain == b.Host);
            }

            if (result == null)
            {
                result = await FindWildcardEndpoint(Domain);
            }
            return result;
        }

        private async Task<XblEndpoint> FindWildcardEndpoint(string Domain)
        {
            XblEndpoint result = null;

            //  We didn't find an endpoint for this specific host, so check for
            //  a wildcard entry in the endpoints (most often *.xboxlive.com)
            string[] splitString = Domain.Split('.');
            int i = splitString.Count();
            if (i >= 2)
            {
                string wildcardHost = string.Format("*.{0}.{1}", splitString[i - 2], splitString[i - 1]);
                using (var dbContext = CreateEndpointDbContext())
                {
                    result = await dbContext.Endpoints
                    .FirstOrDefaultAsync(b => b.Host == wildcardHost);
                }
            }
            return result;
        }


        public async Task<int> InitializeEndpointCacheAsync(Microsoft.CorrelationVector.CorrelationVector Cv)
        {
            try
            {
                using (var dbContext = CreateEndpointDbContext())
                {
                    if (dbContext.Endpoints.Count() == 0)
                    {
                        //  We don't have endpoints yet so get the list and populate the cache for future calls
                        XblEndpointsAndPolicies EndpointsAndSignaturePolicies = await XblEndpointsAndPolicies.GetXblEndpointsAndPoliciesAsync();

                        //  When trying to save the index value of 0 into the db it is being changed to 1, so 
                        //  to compensate for this we start our index at 1 instead of 0 based and have code
                        //  to do the same when searching for the correct signature policy index
                        int signatureIndex = 1;
                        foreach (ServiceSignaturePolicy signaturePolicy in EndpointsAndSignaturePolicies.SignaturePolicies)
                        {

                            signaturePolicy.SupportedalgorithmsString = String.Join('.', signaturePolicy.SupportedAlgorithms);                          
                            signaturePolicy.SignatureID = signatureIndex++;
                            dbContext.SignaturePolicies.Add(signaturePolicy);
                        }

                        foreach (XblEndpoint endpoint in EndpointsAndSignaturePolicies.EndPoints)
                        {
                            //  Because we are using an in-memory database for this we need a unique key for each
                            //  endpoint.  But some endpoints in the returned JSON have the same host name and protocol
                            //  so we need to use the full URL including the path (if there is one) for a unique identifier
                            endpoint.FullURI = string.Format("{0}://{1}{2}", endpoint.Protocol, endpoint.Host, endpoint.Path);
                            dbContext.Endpoints.Add(endpoint);
                        }

                        dbContext.SaveChanges();
                        mLogger.StartupInfo(Cv.Value, string.Format("{0} Endpoints cached", EndpointsAndSignaturePolicies.EndPoints.Count));
                    }
                }
                return 1;   //  It's not good practice to return a Task<void> so we are returning a Task<int>
            }
            catch(Exception e)
            {
                mLogger.XstsException(Cv.Increment(), "Unable to initialize the EndpointCache", e);
                throw;
            }
        }
    }
}