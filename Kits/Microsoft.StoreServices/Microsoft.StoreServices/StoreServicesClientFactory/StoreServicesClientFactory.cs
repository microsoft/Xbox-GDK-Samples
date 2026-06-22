//-----------------------------------------------------------------------------
// StoreServicesClientFactory.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System;
using System.Net.Http;

namespace Microsoft.StoreServices
{
    /// <summary>
    /// Factory that will provide IStoreServicesClients on request from a shared IAccessTokenProvider.
    /// </summary>
    public class StoreServicesClientFactory : IStoreServicesClientFactory
    {
        /// <summary>
        /// Shared IAccessTokenProvider to be used with each StoreServices client created.
        /// </summary>
        private IAccessTokenProvider _accessTokenProvider;

        /// <summary>
        /// Identification string of your service for logging purposes on the calls to the Microsoft
        /// Store Services.
        /// </summary>
        public string ServiceIdentity { get; private set; }

        /// <summary>
        /// Constructor for use in the ASP.NET ConfigureServices() function before you
        /// have the Service Identity and other values to create the IAccessTokenProvider.
        /// Once you have these values and the IAccessTokenProvider, use this class' 
        /// Configure() function to set the values.
        /// </summary>
        public StoreServicesClientFactory() { }


        public void Initialize(string serviceIdentity,
                              IAccessTokenProvider accessTokenProvider)
        {
            _accessTokenProvider = accessTokenProvider;
            ServiceIdentity = serviceIdentity ?? "Microsoft.StoreServices.Default";
        }

        /// <summary>
        /// Constructor if you already have the needed service identity and IAccessTokenProvider.
        /// </summary>
        /// <param name="serviceIdentity">Identification string of your service for logging purposes on the calls to
        /// the Microsoft Store Services.</param>
        /// <param name="accessTokenProvider">IAccessTokenProvider initialized with your services information that
        /// will be shared and used by all the generated StoreServicesClients.</param>
        public StoreServicesClientFactory(string serviceIdentity,
                                          IAccessTokenProvider accessTokenProvider) 
        {
            Initialize(serviceIdentity, accessTokenProvider);
        }
        
        /// <summary>
        /// Sets the StoreServicesClient to use the provided httpClientFactory
        /// to create any HttpClients that are needed for the HTTP calls that will be made.
        /// </summary>
        /// <param name="httpClientCreateFunction"></param>
        public void SetHttpClientCreate(IHttpClientFactory httpClientFactory)
        {
            if (httpClientFactory == null)
            {
                throw new ArgumentNullException(nameof(httpClientFactory));
            }
            else
            {
                SetHttpClientCreate(httpClientFactory.CreateClient);
            }
        }

        /// <summary>
        /// Sets the StoreServicesClient to use the provided function to create any
        /// HttpClients that are needed for the HTTP calls that will be made.
        /// </summary>
        /// <param name="httpClientCreateFunction"></param>
        public void SetHttpClientCreate(Func<HttpClient> httpClientCreateFunction)
        {
            if (httpClientCreateFunction == null)
            {
                throw new ArgumentNullException(nameof(httpClientCreateFunction));
            }
            else
            {
                StoreServicesClient.CreateHttpClientFunc = httpClientCreateFunction;
            }
        }

        public IStoreServicesClient CreateClient()
        {
            return new StoreServicesClient(ServiceIdentity, _accessTokenProvider);
        }
    }
}
