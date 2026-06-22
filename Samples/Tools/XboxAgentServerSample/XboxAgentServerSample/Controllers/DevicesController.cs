//-----------------------------------------------------------------------------
// DevicesController.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using XboxAgentServerSample.JobManagement;
using XboxAgentServerSample.StateManagement;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.ModelBinding;
using Xbox.AgentProtocol;

namespace XboxAgentServerSample.Controllers
{
    /// <summary>
    /// This controller is used to demonstrate the example server.
    /// </summary>
    [ApiController]
    public class DevicesController : ControllerBase
    {
        private readonly IDeviceStateManager _deviceStateManager;
        private readonly IDesiredStateController _desiredStateController;

        public DevicesController(IDeviceStateManager deviceStateManager, IDesiredStateController desiredStateController)
        {
            _deviceStateManager = deviceStateManager;
            _desiredStateController = desiredStateController;
        }

        /// <summary>
        /// Set the desired state for devices. Send an empty body
        /// to clear the current desired state.
        /// </summary>
        /// <param name="desiredState">The desired state devices should be configured to.</param>
        [HttpPost("/api/devices/DefaultDeviceState")]
        [ProducesResponseType(StatusCodes.Status200OK)]
        public IActionResult SetDefaultDesiredState([FromBody(EmptyBodyBehavior = EmptyBodyBehavior.Allow)] DesiredState desiredState)
        {
            _desiredStateController.DesiredState = desiredState;
            return Ok();
        }

        /// <summary>
        /// Get the currently set desired state for devices.
        /// </summary>
        /// <returns><see cref="DesiredState"/></returns>
        [HttpGet("/api/devices/DefaultDeviceState")]
        [ProducesResponseType(typeof(DesiredState), StatusCodes.Status200OK)]
        public ActionResult<DesiredState> GetDefaultDesiredState()
        {
            return Ok(_desiredStateController.DesiredState);
        }

        /// <summary>
        /// This action show the latest state from the device's agent.
        /// </summary>
        /// <param name="deviceId">The device id of the device you want to see the state from.</param>
        /// <returns><see cref="KitHeartbeatRequest"/></returns>
        [HttpGet("api/devices/{deviceId}/State")]
        [ProducesResponseType(typeof(KitHeartbeatRequest), StatusCodes.Status200OK)]
        [ProducesResponseType(StatusCodes.Status404NotFound)]
        public ActionResult<KitHeartbeatRequest> State(string deviceId)
        {
            KitHeartbeatRequest? status = _deviceStateManager.GetLatestKitState(deviceId);

            if (status is null)
            {
                return NotFound($"Device {deviceId} hasn't contacted us yet.");
            }

            return Ok(status);
        }
    }
}
