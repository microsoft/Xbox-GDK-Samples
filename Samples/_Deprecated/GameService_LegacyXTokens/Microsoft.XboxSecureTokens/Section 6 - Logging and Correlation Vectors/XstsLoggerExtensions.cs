//-----------------------------------------------------------------------------
// XstsLoggerExtensions.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using Microsoft.Extensions.Logging;
using System;
using System.Net.Http;
using System.Text;

namespace Microsoft.XboxSecureTokens
{
    //  SECTION 6 - Log formatting centralized location
    //  This sample is using LoggerMessage as outlined in the following article:
    //  https://docs.microsoft.com/en-us/aspnet/core/fundamentals/logging/loggermessage?view=aspnetcore-2.1
    //
    //  As part of this, the format of the logging strings is pre-defined in the static class below rather
    //  than at each point in the code using mLogger.LogError or mLogger.LogInformation.  This provides better
    //  performance even if it saves a small amount and allows all of the logging formatting to be managed
    //  in a single file rather than hundreds of lines spread out throughout the code.

    public class XstsLogEventIds
    {
        //  Server and services
        public const int Startup            = 1000;
        public const int GetClaims          = 1010;
        public const int b2bFriends         = 1020;
        public const int Collections        = 1030;
        public const int TransactionDB      = 1031;
        public const int CollectionsQuery   = 1032;
        public const int CollectionsConsume = 1033;
        public const int CollectionsRetry   = 1034;

        //  XSTS token handling
        public const int XstsMiddleware     = 2000;
        public const int XstsServiceToken   = 2010;
        public const int XstsDelegatedAuth  = 2020;

        //  Generic logging ids
        public const int XstsInformation    = 3000;
        public const int XstsException      = 3001;
        public const int XstsWarning        = 3002;
    }

    public static class XstsLoggerExtensions
    {
        //  Xsts specific logging
        private static readonly Action<ILogger, string, string, Exception> _xstsInformation;
        private static readonly Action<ILogger, string, string, Exception> _xstsException;
        private static readonly Action<ILogger, string, string, Exception> _xstsWarning;
        private static readonly Action<ILogger, string, string, string, string, string, string, Exception> _xstsFailedRequest;
        private static readonly Action<ILogger, string, string, string, string, string, Exception> _xblFailedRequest;
        private static readonly Action<ILogger, string, string, string, string, Exception> _xblDebugData;
        private static readonly Action<ILogger, string, string, Exception> _xstsUserNotFound;

        //  Service specific logging actions
        private static readonly Action<ILogger, string, string, Exception> _startupError;
        private static readonly Action<ILogger, string, string, Exception> _startupInfo;
        private static readonly Action<ILogger, string, string, Exception> _startupWarning;
        private static readonly Action<ILogger, string, string, string, string, Exception> _collectionsInvalidRequest;
        private static readonly Action<ILogger, string, string, string, string, Exception> _mpsdInvalidRequest;
        private static readonly Action<ILogger, string, string, string, string, int, Exception> _removePendingTransaction;
        private static readonly Action<ILogger, string, string, string, string, int, Exception> _addPendingTransaction;
        private static readonly Action<ILogger, string, string, string, Exception> _queryResponse;
        private static readonly Action<ILogger, string, string, string, Exception> _consumeResponse;
        private static readonly Action<ILogger, string, string, string, string, int, string, Exception> _consumeError;
        private static readonly Action<ILogger, string, string, Exception> _retryPendingConsumesResponse;
        private static readonly Action<ILogger, string, string, string, Exception> _b2bFriendsResponse;
        private static readonly Action<ILogger, string, string, string, Exception> _claimsResponse;

        private static string SanitizeLineEndings(string str)
        {
            //  this is so the strings we are logging are json formatting compatible
            return str.Replace("\n", "\\\\n").Replace("\r", "\\\\r").Replace("\t", "\\\\t");
        }

        private static string SanitizeQuotes(string str)
        {
            //  Used for formatting json that will be included as a string
            //  in another json structure
            var a = str.Replace("\"", "\\\"").Replace("\'", "\\\'");
            return a;
        }

        //  This is where we define the static format for each of our logging APIs
        static XstsLoggerExtensions()
        {
            _startupInfo = LoggerMessage.Define<string, string>(
                LogLevel.Information,
                new EventId(XstsLogEventIds.Startup, nameof(StartupInfo)),
                "{{\"cV\":\"{cV}\",\"info\":\"{info}\"}}");

            _xstsException = LoggerMessage.Define<string, string>(
                LogLevel.Error,
                new EventId(XstsLogEventIds.XstsException, nameof(XstsException)),
                "{{\"cV\":\"{cV}\",\"message\":\"{message}\"}}");

            _xstsInformation = LoggerMessage.Define<string, string>(
                LogLevel.Information,
                new EventId(XstsLogEventIds.XstsInformation, nameof(XstsInformation)),
                "{{\"cV\":\"{cV}\",\"message\":\"{message}\"}}");

            _xstsWarning = LoggerMessage.Define<string, string>(
                LogLevel.Warning,
                new EventId(XstsLogEventIds.XstsException, nameof(XstsException)),
                "{{\"cV\":\"{cV}\",\"message\":\"{message}\"}}");

            _xstsFailedRequest = LoggerMessage.Define<string, string, string, string, string, string>(
               LogLevel.Warning,
               new EventId(XstsLogEventIds.XstsDelegatedAuth, nameof(XstsFailedRequest)),
               "{{\"cV\":\"{cV}\",\"method\":\"{method}\",\"requestUri\":\"{uri}\",\"requestBody\":\"{body}\",\"responseCv\":\"{responseCv}\",\"responseBody\":\"{responseBody}\"}}");

            _xblFailedRequest = LoggerMessage.Define<string, string, string, string, string>(
               LogLevel.Warning,
               new EventId(XstsLogEventIds.XstsDelegatedAuth, nameof(XblFailedRequest)),
               "{{\"cV\":\"{cV}\",\"pxuid\":\"{pxuid}\",\"delegationToken\":\"{delegationToken}\",\"response\":\"{response}\",\"request\":\"{request}\"}}");

            _xblDebugData = LoggerMessage.Define<string, string, string, string>(
               LogLevel.Warning,
               new EventId(XstsLogEventIds.XstsDelegatedAuth, nameof(XblDebugData)),
               "{{\"cV\":\"{cV}\",\"pxuid\":\"{pxuid}\",\"delegationToken\":\"{delegationToken}\",\"target\":\"{target}\"}}");

            _xstsUserNotFound = LoggerMessage.Define<string, string>(
               LogLevel.Warning,
               new EventId(XstsLogEventIds.XstsWarning, nameof(XstsUserNotFound)),
               "{{\"cV\":\"{cV}\",\"ush\":\"{ush}\",\"error\":\"Unable to find claims in token for user\"");

            _startupError = LoggerMessage.Define<string, string>(
                LogLevel.Critical,
                new EventId(XstsLogEventIds.Startup, nameof(StartupError)),
                "{{\"cV\":\"{cV}\",\"error\":\"{error}\"}}");

            _startupWarning = LoggerMessage.Define<string, string>(
                LogLevel.Warning,
                new EventId(XstsLogEventIds.Startup, nameof(StartupError)),
                "{{\"cV\":\"{cV}\",\"error\":\"{error}\"}}");

            _collectionsInvalidRequest = LoggerMessage.Define<string, string, string, string>(
                LogLevel.Warning,
                new EventId(XstsLogEventIds.Collections, nameof(CollectionsInvalidRequest)),
                "{{\"cV\":\"{cV}\",\"pxuid\":\"{pxuid}\",\"error\":\"{error}\",\"urlPath\":\"{urlPath}\"}}");

            _mpsdInvalidRequest = LoggerMessage.Define<string, string, string, string>(
                LogLevel.Warning,
                new EventId(XstsLogEventIds.Collections, nameof(CollectionsInvalidRequest)),
                "{{\"cV\":\"{cV}\",\"pxuid\":\"{pxuid}\",\"error\":\"{error}\",\"urlPath\":\"{urlPath}\"}}");

            _removePendingTransaction = LoggerMessage.Define<string, string, string, string, int>(
                LogLevel.Information,
                new EventId(XstsLogEventIds.TransactionDB, nameof(RemovePendingTransaction)),
                "{{\"cV\":\"{cV}\",\"pxuid\":\"{pxuid}\",\"transaction\":\"{transaction}\",\"product\":\"{product}\",\"quantity\":{quantity},\"status\":\"removed\"}}");

            _addPendingTransaction = LoggerMessage.Define<string, string, string, string, int>(
                LogLevel.Information,
                new EventId(XstsLogEventIds.TransactionDB, nameof(AddPendingTransaction)),
                "{{\"cV\":\"{cV}\",\"pxuid\":\"{pxuid}\",\"transaction\":\"{transaction}\",\"product\":\"{product}\",\"quantity\":{quantity},\"status\":\"pending\"}}");

            _queryResponse = LoggerMessage.Define<string, string, string>(
                LogLevel.Information,
                new EventId(XstsLogEventIds.CollectionsQuery, nameof(QueryResponse)),
                "{{\"cV\":\"{cV}\",\"pxuid\":\"{pxuid}\",\"response\":\"{response}\"}}");

            _consumeResponse = LoggerMessage.Define<string, string, string>(
                LogLevel.Information,
                new EventId(XstsLogEventIds.CollectionsConsume, nameof(ConsumeResponse)),
                "{{\"cV\":\"{cV}\",\"pxuid\":\"{pxuid}\",\"response\":\"{response}\"}}");

            _consumeError = LoggerMessage.Define<string, string, string, string, int, string>(
                LogLevel.Error,
                new EventId(XstsLogEventIds.CollectionsConsume, nameof(ConsumeError)),
                "{{\"cV\":\"{cV}\",\"pxuid\":\"{pxuid}\",\"transaction\":\"{transaction}\",\"product\":\"{product}\",\"quantity\":{quantity},\"status\":\"{message}\"}}");

            _retryPendingConsumesResponse = LoggerMessage.Define<string, string>(
                LogLevel.Information,
                new EventId(XstsLogEventIds.CollectionsRetry, nameof(RetryPendingConsumesResponse)),
                "{{\"cV\":\"{cV}\",\"response\":\"{response}\"}}");

            _b2bFriendsResponse = LoggerMessage.Define<string, string, string>(
                LogLevel.Information,
                new EventId(XstsLogEventIds.b2bFriends, nameof(B2bFriendsResponse)),
                "{{\"cV\":\"{cV}\",\"pxuid\":\"{pxuid}\",\"response\":\"{response}\"}}");

            _claimsResponse = LoggerMessage.Define<string, string, string>(
                LogLevel.Information,
                new EventId(XstsLogEventIds.GetClaims, nameof(ClaimsResponse)),
                "{{\"cV\":\"{cV}\",\"pxuid\":\"{pxuid}\",\"response\":\"{response}\"}}");
        }

        public static string FormatResponseForLogs(HttpResponseMessage response, string responseBody)
        {
            var responseString = new StringBuilder();
            responseString.AppendFormat("{0}: {1}", (int)response.StatusCode, response.ReasonPhrase);
            responseString.AppendLine();
            responseString.Append(response.Headers.ToString());
            responseString.AppendLine();
            responseString.Append(SanitizeQuotes(responseBody));
            return responseString.ToString();
        }

        //  Creates a string that can be used in Fiddler's Scratchpad to replay the exact same call for debugging
        public static string FormatRequestForLogs(HttpRequestMessage request, string requestBody)
        {
            var requestString = new StringBuilder();
            requestString.AppendFormat("{0} {1} HTTP/1.1", request.Method, request.RequestUri.AbsoluteUri);
            requestString.AppendLine();
            requestString.Append(request.Headers.ToString());
            if(request.Content != null)
            {
                requestString.Append(request.Content.Headers.ToString());
            }
            if(request.Method != HttpMethod.Get)
            { 
                requestString.AppendLine();
                requestString.Append(SanitizeQuotes(requestBody));     // Had to do this for now as the request.Content object has already been disposed
                                                                       // at this point.  otherwise we would use Content.ReadAsStringAsync() as above
            }
            return requestString.ToString();
        }

        public static void XstsException(this ILogger logger, string cV, string message, Exception ex)
        {
            _xstsException(logger, cV, message, ex);
        }

        public static void XstsInformation(this ILogger logger, string cV, string message)
        {
            _xstsInformation(logger, cV, message, null);
        }

        public static void XstsWarning(this ILogger logger, string cV, string message)
        {
            _xstsWarning(logger, cV, message, null);
        }

        public static void XstsFailedRequest(this ILogger logger, string cV, string method, string uri, string body, string responseCv, string responseBody)
        {
            _xstsFailedRequest(logger, cV, method, uri, body, responseCv, responseBody, null);
        }

        public static void XblFailedRequest(this ILogger logger, string cV, string pxuid, string delegationToken, HttpResponseMessage response, string responseBody, string requestBody)
        {
            string requestLog = FormatRequestForLogs( response.RequestMessage, requestBody);
            string responseLog = FormatResponseForLogs( response, responseBody);

            _xblFailedRequest(logger,
                              cV,
                              pxuid,
                              delegationToken,
                              SanitizeLineEndings(requestLog),
                              SanitizeLineEndings(responseLog),
                              null);
        }

        public static void XblDebugData(this ILogger logger, string cV, string pxuid, string delegationToken, string target)
        {
            _xblDebugData(logger, cV, pxuid, delegationToken, target, null);
        }

        public static void XstsUserNotFound(this ILogger logger, string cV, string ush)
        {
            _xstsUserNotFound(logger, cV, ush, null);
        }

        //  Controller / service specific logging functions
        public static void StartupInfo(this ILogger logger, string cV, string info)
        {
            _startupInfo(logger, cV, info, null);
        }

        public static void StartupError(this ILogger logger, string cV, string info, Exception ex)
        {
            _startupError(logger, cV, info, ex);
        }

        public static void StartupWarning(this ILogger logger, string cV, string info, Exception ex)
        {
            _startupWarning(logger, cV, info, ex);
        }

        public static void CollectionsInvalidRequest(this ILogger logger, string cV, string pxuid, string error, string urlPath)
        {
            _collectionsInvalidRequest(logger, cV, pxuid, error, urlPath, null);
        }

        public static void MPSDInvalidRequest(this ILogger logger, string cV, string pxuid, string error, string urlPath)
        {
            _mpsdInvalidRequest(logger, cV, pxuid, error, urlPath, null);
        }

        public static void AddPendingTransaction(this ILogger logger, string cV, string pxuid, string transactionId, string productId, int quantity )
        {
            _addPendingTransaction(logger, cV, pxuid, transactionId, productId, quantity, null);
        }

        public static void RemovePendingTransaction(this ILogger logger, string cV, string pxuid, string transactionId, string productId, int quantity)
        {
            _removePendingTransaction(logger, cV, pxuid, transactionId, productId, quantity, null);
        }

        public static void QueryResponse(this ILogger logger, string cV, string pxuid, string response)
        {
            _queryResponse(logger, cV, pxuid, SanitizeLineEndings(response), null);
        }

        public static void ConsumeResponse(this ILogger logger, string cV, string pxuid, string response)
        {
            _queryResponse(logger, cV, pxuid, SanitizeLineEndings(response), null);
        }

        public static void ConsumeError(this ILogger logger, string cV, string pxuid, string transactionId, string productId, int quantity, string message, Exception ex)
        {
            _consumeError(logger, cV, pxuid, transactionId, productId, quantity, SanitizeLineEndings(message), ex);
        }

        public static void RetryPendingConsumesResponse(this ILogger logger, string cV, string response)
        {
            _retryPendingConsumesResponse(logger, cV, SanitizeLineEndings(response), null);
        }

        public static void B2bFriendsResponse(this ILogger logger, string cV, string pxuid, string response)
        {
            _b2bFriendsResponse(logger, cV, pxuid, SanitizeLineEndings(response), null);
        }

        public static void ClaimsResponse(this ILogger logger, string cV, string pxuid, string response)
        {
            _claimsResponse(logger, cV, pxuid, SanitizeLineEndings(response), null);
        }
    }
}
