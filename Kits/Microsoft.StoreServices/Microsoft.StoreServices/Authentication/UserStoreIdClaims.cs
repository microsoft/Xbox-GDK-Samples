//-----------------------------------------------------------------------------
// UserStoreIdClaims.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using System;

namespace Microsoft.StoreServices
{
    /// <summary>
    /// Claims contained within a UserStoreId (general term for either UserCollectionsId or UserPurchaseId)
    /// </summary>
    public class UserStoreIdClaims
    {
        /// <summary>
        /// Azure Active Directory App Client ID used for the Access Token that generated this
        /// UserStoreId
        /// </summary>
        [JsonProperty("http://schemas.microsoft.com/marketplace/2015/08/claims/key/clientId")]
        public string ClientId { get; set; }

        /// <summary>
        /// The PublisherUserId string that was used when generating the UserStoreId on the
        /// Client.  NOTE: This is controlled by the client and does not represent an actual 
        /// identity value of the user account this UserStoreId represents.
        /// </summary>
        [JsonProperty("http://schemas.microsoft.com/marketplace/2015/08/claims/key/userId")]
        public string UserId { get; set; }

        /// <summary>
        /// Payload used by the Microsoft Store Services as part of authentication
        /// </summary>
        [JsonProperty("http://schemas.microsoft.com/marketplace/2015/08/claims/key/payload")]
        public string Payload { get; set; }

        /// <summary>
        /// The URI that can be used to refresh this UserStoreId once it has expired without 
        /// generating a new one from the client.
        /// </summary>
        [JsonProperty("http://schemas.microsoft.com/marketplace/2015/08/claims/key/refreshUri")]
        public string RefreshUri { get; set; }

        /// <summary>
        /// Service or identity of who created and signed the JWT representing the UserStoreId
        /// </summary>
        [JsonProperty("iss")] public string Issuer { get; set; }

        /// <summary>
        /// Represents if this is a UserCollectionsId or UserPurchaseId.  See UserStoreIdAudiences.
        /// </summary>
        [JsonProperty("aud")] public string Audience { get; set; }

        /// <summary>
        /// Seconds from the Unix Epoc that represents the UTC datetime the UserStoreId was generated
        /// </summary>
        [JsonProperty("iat")] public uint EpochIssuedOn { get; set; }

        /// <summary>
        /// Seconds from the Unix Epoc that represents the UTC datetime the UserStoreId will expire
        /// </summary>
        [JsonProperty("exp")] public uint EpochExpiresOn { get; set; }

        /// <summary>
        /// Seconds from the Unix Epoc that represents the UTC datetime the UserStoreId starts being valid and can be used.
        /// </summary>
        [JsonProperty("nbf")] public uint EpochValidAfter { get; set; }

        /// <summary>
        /// The UTC date and time when the UserStoreId becomes valid and can be used
        /// </summary>
        public DateTime ValidAfter => DateTime.UnixEpoch.AddSeconds(EpochValidAfter);

        /// <summary>
        /// The UTC date and time when the UserStoreId expires
        /// </summary>
        public DateTime ExpiresOn => DateTime.UnixEpoch.AddSeconds(EpochExpiresOn);

        /// <summary>
        /// The UTC date and time when the UserStoreId was created
        /// </summary>
        public DateTime IssuedOn => DateTime.UnixEpoch.AddSeconds(EpochIssuedOn);
    }
}
