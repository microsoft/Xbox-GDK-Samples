//-----------------------------------------------------------------------------
// HeartbeatController.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using Microsoft.AspNetCore.Mvc;
using Microsoft.SimpleXboxSecureTokens;
using Xbox.AgentProtocol;
using XboxAgentServerSample.JobManagement;
using XboxAgentServerSample.StateManagement;

namespace XboxAgentServerSample.Controllers
{
    [ApiController]
    public class HeartbeatController : ControllerBase
    {
        private readonly ILogger<HeartbeatController> _logger;

        private readonly IJobConsumptionManager _consumerJobManager;
        private readonly IDesiredStateController _desiredStateController;

        public HeartbeatController(IJobConsumptionManager consumerJobManager,
                                    IDesiredStateController desiredStateController,
                                    ILogger<HeartbeatController> logger)
        {
            _consumerJobManager = consumerJobManager;
            _desiredStateController = desiredStateController;
            _logger = logger;
        }

        /// <summary>
        /// Receives the Agents current state and returns a job for the Agent to process.
        /// </summary>
        /// <param name="kitHeartbeatRequest">Sent by the agent to provide the current state of the devkit, and request a job to process.</param>
        /// <returns><see cref="KitHeartbeatResponse"/></returns>
        [HttpPost("/api/KitHeartbeat")]
        [ProducesResponseType(StatusCodes.Status200OK)]
        [ProducesResponseType(StatusCodes.Status401Unauthorized)]
        public ActionResult<KitHeartbeatResponse> SendHeartbeat([FromBody] KitHeartbeatRequest kitHeartbeatRequest)
        {
            string deviceId;

            // First try to get the device id from the XToken.
            // Otherwise, get it from the 'DeviceId' header.
            // If neither are present, then return 401 UNAUTHORIZED.
            var xstsToken = this.HttpContext.Request.Headers["Authorization"];
            if (string.IsNullOrEmpty(xstsToken))
            {
                xstsToken = this.HttpContext.Request.Headers["XBOX-AGENT-XTOKEN"];
            }

            if (string.IsNullOrEmpty(xstsToken))
            {
                var message = "No XSTS token in Authorization or XBOX-AGENT-XTOKEN headers";
                throw new Exception(message);
            }

            XstsClientToken? clientToken = null;

            try
            {
                var clientTokenTask = XstsTokenHandler.ValidateAuthorizationHeaderWithCache(xstsToken);
                clientTokenTask.Wait();

                clientToken = clientTokenTask.Result;
            }
            catch (Exception ex)
            {
                _logger.LogWarning("Client attempting to connect with an XSTS token that is invalid", ex);

            }

            if (clientToken == null)
            {
                _logger.LogWarning("Client attempting to connect without an XSTS token.");
                return Unauthorized();
            }
            else if (clientToken.Device == null ||
                     clientToken.Device.DeveloperDeviceId == null)
            {
                _logger.LogWarning("Client attempting to connect with an XSTS token that doesn't have a DeveloperDeviceID.  Relying Party: {clientToken.Audience}", clientToken.Audience);
                return Unauthorized();
            }
            else
            {
                deviceId = clientToken.Device.DeveloperDeviceId;
            }

            _logger.LogInformation("Handling heartbeat for device id: {DeviceId}", deviceId);

            // store the status from the device
            _consumerJobManager.SetLatestKitState(deviceId, kitHeartbeatRequest, clientToken);

            _desiredStateController.CheckAndSetDeviceState(deviceId);

            // Default state for a requested job is null.
            RequestedJob? requestedJob;

            // get the pending job
            RequestedJob? pendingJob = _consumerJobManager.GetPendingJob(deviceId);

            if (pendingJob is not null)
            {
                _logger.LogInformation("Found a pending job.");

                var cancelCurrentJobTask = _consumerJobManager.GetCancelCurrentJob(deviceId);

                // Check we have a job result and process it otherwise send the pending job or cancel job if it exists.
                if (kitHeartbeatRequest.LastProcessedJobResult is not null &&
                    pendingJob.Id == kitHeartbeatRequest.LastProcessedJobResult.Id)
                {
                    _logger.LogInformation("Job Id {JobId} finished", kitHeartbeatRequest.LastProcessedJobResult.Id);
                    _consumerJobManager.SetResult(deviceId, kitHeartbeatRequest.LastProcessedJobResult);
                    var requestJobTask = _consumerJobManager.GetNextJob(deviceId);

                    requestJobTask.Wait();
                    requestedJob = requestJobTask.Result;
                }
                else
                {
                    // If canceling, then send the cancel job otherwise resend the pending job.
                    _logger.LogInformation("Current job is still being processed.");
                    cancelCurrentJobTask.Wait();
                    var cancelCurrentJob = cancelCurrentJobTask.Result;
                    if (cancelCurrentJob is not null)
                    {
                        _logger.LogInformation("Canceling current job");
                        requestedJob = cancelCurrentJob;
                    }
                    else
                    {
                        requestedJob = pendingJob;
                    }
                }
            }
            else
            {
                var requestJobTask = _consumerJobManager.GetNextJob(deviceId);
                requestJobTask.Wait();
                requestedJob = requestJobTask.Result;
            }

            KitHeartbeatResponse kitHeartbeatResponse = new()
            {
                RequestedJob = requestedJob
            };

            var returnObject = Ok(kitHeartbeatResponse);
            return returnObject;
        }
    }
}
