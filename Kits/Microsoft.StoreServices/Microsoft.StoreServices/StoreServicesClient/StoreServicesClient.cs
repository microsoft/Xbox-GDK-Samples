//-----------------------------------------------------------------------------
// StoreServicesClient.Clawback.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;

namespace Microsoft.StoreServices
{
    /// <summary>
    /// Client to manage the access tokens, authentication, calls, and requests to the Microsoft Store Services. 
    /// </summary>
    public sealed partial class StoreServicesClient : IStoreServicesClient
    {
        /// <summary>
        /// Can be overridden with an HttpClientFactory.CreateClient() if used by your service.
        /// Ex: StoreServicesClient.CreateHttpClientFunc = httpClientFactory.CreateClient;
        /// </summary>
        public static Func<HttpClient> CreateHttpClientFunc = () => new HttpClient();

        /// <summary>
        /// Identification string of your service for logging purposes on the calls to the Microsoft
        /// Store Services.
        /// </summary>
        public string ServiceIdentity { get; private set; }

        /// <summary>
        /// Manages the access tokens required for authenticating our calls to the Microsoft Store Services.
        /// </summary>
        private readonly IAccessTokenProvider _accessTokenProvider;

        /// <summary>
        /// Used for disposing of the item.
        /// </summary>
        private bool _isDisposed;

        /// <summary>
        /// Creates a client that will manage the auth and calls to the Microsoft Store Services.
        /// </summary>
        /// <param name="serviceIdentity">Identification string of your service for logging purposes on the calls to
        /// the Microsoft Store Services.</param>
        /// <param name="accessTokenProvider">IAccessTokenProvider created to manages the access tokens required for 
        /// authenticating our calls to the Microsoft Store Services.</param>
        public StoreServicesClient(string serviceIdentity, IAccessTokenProvider accessTokenProvider) 
        {
            _accessTokenProvider = accessTokenProvider;
            ServiceIdentity = serviceIdentity ?? "UnspecifiedService-Microsoft.StoreServices";
            _isDisposed = false;
        }

        /// <summary>
        /// Creates and executes the HTTP request to the target Microsoft Store Service then parses the JSON 
        /// response based on the specified response type.
        /// </summary>
        /// <typeparam name="T">JSON response type to expect</typeparam>
        /// <param name="uri">URI of the target Microsoft Store Service</param>
        /// <param name="requestBodyString">JSON request body</param>
        /// <param name="additionalHeaders">Any additional headers that are needed more than the default headers</param>
        /// <returns><typeparamref name="T"/> object from the JSON response of the HTTP request</returns>
        private async Task<T> IssueRequestAsync<T>(string uri, string requestBodyString, IDictionary<string, string> additionalHeaders = null)
        {
            var client = CreateHttpClientFunc();

            var httpRequest = new HttpRequestMessage(HttpMethod.Post, uri)
            {
                Content = new StringContent(requestBodyString, Encoding.UTF8, "application/json")
            };

            //  Add the Authorization header for AAD / StoreID keys
            var serviceToken = await _accessTokenProvider.GetServiceAccessTokenAsync();
            httpRequest.Headers.Add("Authorization", $"Bearer {serviceToken.Token}");
            httpRequest.Headers.Add("User-Agent", ServiceIdentity); //  unique name to identify your service in logging

            if (additionalHeaders != null)
            {
                //  Add the rest of the headers the caller wants 
                foreach (var header in additionalHeaders)
                {
                    httpRequest.Headers.Add(header.Key, header.Value);
                }
            }

            try
            {
                //  issue the request to the service
                var httpResponse = await client.SendAsync(httpRequest);

                if (!httpResponse.IsSuccessStatusCode)
                {
                    throw new StoreServicesHttpResponseException($"HTTP request failed with status code {httpResponse.StatusCode}.", httpResponse);
                }

                string responseBody = await httpResponse.Content.ReadAsStringAsync();

                //  De-serialize the JSON response and pass it back.  All responses from the 
                //  Microsoft Store Services use UTC time so we make sure to specify that.
                return JsonConvert.DeserializeObject<T>(responseBody, new JsonSerializerSettings
                {
                    DateTimeZoneHandling = DateTimeZoneHandling.Utc
                });
            }
            catch (HttpRequestException httpReqEx)
            {
                throw new StoreServicesClientException($"HTTP request for type {typeof(T)} failed.", httpReqEx);
            }
        }

        /// <summary>
        /// Provides a Service Access Token from the IAccessTokenProvider.
        /// </summary>
        /// <returns></returns>
        public Task<AccessToken> GetServiceAccessTokenAsync()
        {
            return _accessTokenProvider.GetServiceAccessTokenAsync();
        }

        /// <summary>
        /// Provides a Service Access Token from the IAccessTokenProvider.
        /// </summary>
        /// <returns></returns>
        public Task<AccessToken> GetCollectionsAccessTokenAsync()
        {
            return _accessTokenProvider.GetCollectionsAccessTokenAsync();
        }

        /// <summary>
        /// Provides a Service Access Token from the IAccessTokenProvider.
        /// </summary>
        /// <returns></returns>
        public Task<AccessToken> GetPurchaseAccessTokenAsync()
        {
            return _accessTokenProvider.GetPurchaseAccessTokenAsync();
        }

        /// <summary>
        /// Dispose of this object.
        /// </summary>
        public void Dispose()
        {
            if (_isDisposed) return;
            _isDisposed = true;
            GC.SuppressFinalize(this);
        }
    }
}