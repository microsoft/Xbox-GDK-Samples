//-----------------------------------------------------------------------------
// XblEndpointsAndPolicies.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;
using System.Linq;
using System.Net.Http;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace Microsoft.XboxSecureTokens.XstsDelegatedAuth
{
    /// <summary>
    /// SECTION 2 - Caching and special case rules for endpoints to generate signatures
    /// Gets the signature policies for the endpoints of the Xbox Live services
    /// </summary>
    public class XblEndpointsAndPolicies
    {

        [Key] public DateTime CacheTime { get; set; }
        public List<XblEndpoint> EndPoints { get; set; }
        public List<ServiceSignaturePolicy> SignaturePolicies { get; set; }

        public XblEndpoint Find( string relyingParty )
        {
            return this.EndPoints.Where(ep => ep.RelyingParty.Equals( relyingParty, StringComparison.OrdinalIgnoreCase ) ).FirstOrDefault();
        }

        public XblEndpoint FindByFQDN( string host )
        {
            return this.EndPoints.Where( ep => ep.Match( host ) ).FirstOrDefault();
        }

        public static async Task<XblEndpointsAndPolicies> GetAllEndpointsAndPoliciesAsync()
        {
            //  We don't use the IHttpClientFactory as we do everywhere else in the server
            //  for the following reason.  
            //  We will only make this call on server startup and not frequently when getting
            //  requests from clients.  So doing this is more simple than implementing the factory
            //  in the startup function.
            using (var client = new HttpClient())
            {
                using (HttpResponseMessage response = await client.GetAsync("https://title.mgt.xboxlive.com/titles/default/endpoints?type=1"))
                {
                    string responseBody = await response.Content.ReadAsStringAsync();

                    XblEndpointsAndPolicies result = JsonConvert.DeserializeObject<XblEndpointsAndPolicies>(responseBody);
                    result.CacheTime = DateTime.Now;
                    return result;
                }
            }
        }

        public static async Task<XblEndpointsAndPolicies> GetXblEndpointsAndPoliciesAsync()
        {
            XblEndpointsAndPolicies result = await GetAllEndpointsAndPoliciesAsync();
            var filteredEndpoints = new List<XblEndpoint>();

            //  Filter all the Endpoints down to just the ones that we need
            foreach(XblEndpoint endpoint in result.EndPoints)
            {
                bool keepEndpoint = false;
                foreach(string target in XstsConstants.TargetXblEndpoints)
                {
                    if(endpoint.Host == target && endpoint.Protocol.Equals("https"))
                    {
                        //  This is one of our targets, don't remove it
                        keepEndpoint = true;
                        break;
                    }
                }

                if(keepEndpoint)
                {
                    filteredEndpoints.Add(endpoint);
                }
            }

            //  Copy over the filtered list of endpoints, keep all the rest of
            //  the signature policies and timestamp
            result.EndPoints = filteredEndpoints;
            return result;
        }
    }

    public class XblEndpoint
    {
        //  The host will be the key for each endpoint because it is unique


        [Key] public string FullURI { get; set; }
        public string Host { get; set; }
        public string Protocol { get; set; }
        public string HostType { get; set; }
        public string RelyingParty { get; set; }
        public string TokenType { get; set; }
        public int SignaturePolicyIndex { get; set; }
        public string Path { get; set; }
        public string MinTlsVersion { get; set; }

        [NotMapped]
        public List<int> ServerCertIndex { get; set; }
        public string SubRelyingParty { get; set; }

        public bool Match( string host )
        {
            // if we have a fqdn match 
            if (String.Equals( "fqdn", this.HostType, StringComparison.OrdinalIgnoreCase ) && String.Equals( host, this.Host, StringComparison.OrdinalIgnoreCase ) )
            {
                return true; 
            }

            // otherwise we need to do a regex match
            if ( String.Equals( "wildcard" , this.HostType , StringComparison.OrdinalIgnoreCase ) )
            {
                string expression = this.Host.Replace( "." , @"\.").Replace( "*" , ".*" ) + "$";
                return Regex.IsMatch( host, expression, RegexOptions.IgnoreCase);
            }

            return false;
        }
    }

    public class ServiceSignaturePolicy
    {

        [Key] public int SignatureID { get; set; }
        public int Version { get; set; }
        [NotMapped]
        public string[] SupportedAlgorithms { get; set; }
        public string SupportedalgorithmsString {get; set;}
        public long MaxBodyBytes { get; set; }

        private SignaturePolicy signaturePolicy;

        public SignaturePolicy SignaturePolicy
        {
            get
            {
                if ( signaturePolicy == null )
                {
                    signaturePolicy = new SignaturePolicy()
                    {
                        Version = this.Version,
                        ClockSkewSeconds = 15,
                        MaxBodyBytes = this.MaxBodyBytes,
                        SupportedAlgorithms = this.SupportedalgorithmsString.Split('.')
                    };
                }
                return signaturePolicy;
            }
        }
    }
}
