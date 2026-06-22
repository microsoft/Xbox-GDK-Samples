//-----------------------------------------------------------------------------
// IStoreServicesClientFactory.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

namespace Microsoft.StoreServices
{
    /// <summary>
    /// Interface to create a factory that will provide IStoreServicesClients on request from
    /// a shared IAccessTokenProvider.
    /// </summary>
    public interface IStoreServicesClientFactory
    {
        /// <summary>
        /// Generate an IStoreServicesClient to be used.
        /// </summary>
        /// <returns></returns>
        IStoreServicesClient CreateClient();

        /// <summary>
        /// Used to set the IAccessTokenProvider and ServiceIdentity of the factory if 
        /// we were unable to initialize the object with them. Ex: Factory is created in
        /// ASP.NET's ConfigureServices(), but you don't have the AAD Ids until Configure().
        /// </summary>
        /// <param name="serviceIdentity">Identification string of your service for logging purposes on the calls to
        /// the Microsoft Store Services.</param>
        /// <param name="accessTokenProvider">IAccessTokenProvider initialized with your services information that
        /// will be shared and used by all the generated StoreServicesClients.</param>
        void Initialize(string serviceIdentity, IAccessTokenProvider accessTokenProvider);
    }
}
