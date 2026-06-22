//-----------------------------------------------------------------------------
// OperatorsController.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

// Ignore Spelling: Aum

using Microsoft.AspNetCore.Mvc;
using System.ComponentModel;
using System.ComponentModel.DataAnnotations;
using XboxAgentServerSample.Operators;
using XboxAgentServerSample.Operators.Infrastructure;

namespace XboxAgentServerSample.Controllers
{
    /// <summary>
    /// This controller is used to test/debug individual operators.
    /// </summary>
    [ApiController]
    public class OperatorsController : ControllerBase
    {
        private readonly ILogger<OperatorsController> _logger;
        private readonly IOperatorFactory _operatorFactory;

        public OperatorsController(ILogger<OperatorsController> logger, IOperatorFactory operatorFactory)
        {
            _logger = logger;
            _operatorFactory = operatorFactory;
        }

        /// <summary>
        /// This action executes the Reboot Operator.
        /// </summary>
        /// <param name="deviceId">The device id of the device to reboot.</param>
        [HttpPost("api/devices/{deviceId}/Reboot")]
        [ProducesResponseType(StatusCodes.Status200OK)]
        public async Task<IActionResult> Reboot(string deviceId)
        {
            var rebootOperator = _operatorFactory.GetOperator<RebootOperator>();

            try
            {
                await rebootOperator.Run(deviceId);
            }
            catch (OperatorException e)
            {
                LogResult("Reboot", e);
                throw;
            }

            LogResult("Reboot");

            return Ok();
        }

        /// <summary>
        /// This action executes the Change SandboxId Operator.
        /// </summary>
        /// <param name="deviceId">The device id of the device to change the sandbox id for.</param>
        /// <param name="changeSandboxIdParameters">The sandbox id to change to.</param>
        [HttpPost("api/devices/{deviceId}/ChangeSandboxId")]
        [ProducesResponseType(StatusCodes.Status200OK)]
        public async Task<IActionResult> ChangeSandboxId(string deviceId, [FromBody] ChangeSandboxIdParameters changeSandboxIdParameters)
        {
            ChangeSandboxIdOperator changeSandboxIdOperator = _operatorFactory.GetOperator<ChangeSandboxIdOperator>();

            try
            {
                await changeSandboxIdOperator.Run(deviceId, changeSandboxIdParameters.SandboxId);
            }
            catch (OperatorException e)
            {
                LogResult("ChangeSandboxId", e);
                throw;
            }

            LogResult("ChangeSandboxId");

            return Ok();
        }

        /// <summary>
        /// This action executes the Install Application Operator.
        /// </summary>
        /// <param name="deviceId">The device id of the device to install the application on.</param>
        /// <param name="installApplicationParameters">The parameters used to specify the application to be installed.</param>
        [HttpPost("api/devices/{deviceId}/InstallApplication")]
        [ProducesResponseType(StatusCodes.Status200OK)]
        public async Task<IActionResult> InstallApplication([FromRoute] string deviceId, [FromBody] InstallApplicationParameters installApplicationParameters)
        {
            InstallApplicationOperator installAppOperator = _operatorFactory.GetOperator<InstallApplicationOperator>();

            try
            {
                await installAppOperator.Run(deviceId, installApplicationParameters.InstallationPackage, installApplicationParameters.DependencyPackages, installApplicationParameters.AumId);
            }
            catch (OperatorException e)
            {
                LogResult("InstallApplication", e);
                throw;
            }

            LogResult("InstallApplication");

            return Ok();
        }

        /// <summary>
        /// This action will install a recovery update from the given Windows file share.
        /// </summary>
        /// <param name="deviceId">The device id of the device to install the recovery update on.</param>
        /// <param name="installRecoveryUpdateParameters">The parameters used to specify the recovery update.</param>
        [HttpPost("api/devices/{deviceId}/InstallRecoveryUpdate")]
        [ProducesResponseType(StatusCodes.Status200OK)]
        public async Task<IActionResult> InstallRecoveryUpdate(string deviceId, [FromBody] InstallRecoveryUpdateParameters installRecoveryUpdateParameters)
        {
            InstallRecoveryUpdateOperator installRecoveryUpdateOperator = _operatorFactory.GetOperator<InstallRecoveryUpdateOperator>();

            try
            {
                await installRecoveryUpdateOperator.Run(deviceId, installRecoveryUpdateParameters.VersionId, installRecoveryUpdateParameters.FactoryReset, installRecoveryUpdateParameters.DesiredSandboxId);
            }
            catch (OperatorException e)
            {
                LogResult("InstallRecoveryUpdate", e);
                throw;
            }

            LogResult("InstallRecoveryUpdate");

            return Ok();
        }

        /// <summary>
        /// This action will download any files from a directory specified.
        /// </summary>
        /// <param name="deviceId">The device id of the device to download log files from.</param>
        [HttpPost("api/devices/{deviceId}/FileRetrieval")]
        [ProducesResponseType(StatusCodes.Status200OK)]
        public async Task<IActionResult> FileRetrieval(string deviceId, FileRetrievalParameters fileRetrievalParameters)
        {
            FileRetrievalOperator fileRetrievalOperator = _operatorFactory.GetOperator<FileRetrievalOperator>();

            try
            {
                await fileRetrievalOperator.Run(deviceId, fileRetrievalParameters.DirectoryPath);
            }
            catch (OperatorException e)
            {
                LogResult("FileRetrieval", e);
                throw;
            }

            LogResult("FileRetrieval");

            return Ok();
        }

        private void LogResult(string operationName, OperatorException? exception = null)
        {
            if (exception == null)
            {
                _logger.LogInformation("{OperationName} succeeded", operationName);
            }
            else
            {
                _logger.LogWarning(exception, "{OperationName} failed", operationName);
            }
        }

        public class ChangeSandboxIdParameters
        {
            /// <summary>
            /// The sandbox id you wish to switch the device to.
            /// </summary>
            [Required]
            public required string SandboxId { get; set; }
        }

        public class InstallApplicationParameters
        {
            /// <summary>
            /// The Url path to download the application to be installed. The filename is assumed to be the final segment.
            /// </summary>
            [Required]
            public Uri InstallationPackage { get; set; }

            /// <summary>
            /// The list of dependent packages needed for the application.
            /// Leave empty or null if there are none. The filename is assumed to be the final segment.
            /// </summary>
            public List<Uri> DependencyPackages { get; set; }

            /// <summary>
            /// If you want the application to launch once installed,
            /// then provide the Application User Model ID (AumId)
            /// for the application.
            /// </summary>
            public string AumId { get; set; }

            public InstallApplicationParameters()
            {
                DependencyPackages = new List<Uri>();
                AumId = string.Empty;
                InstallationPackage = new Uri("");
            }
        }

        public class InstallRecoveryUpdateParameters
        {
            /// <summary>
            /// The recovery version id ex: 10.0.22621.2215.36d7cf1c-ca6f-4341-aa47-7e3be9ced903
            /// </summary>
            [Required]
            public required string VersionId { get; set; }

            /// <summary>
            /// Specify if you wish to factory reset the device or not.
            /// Note: Updating to an older version than the device already is on
            /// may invoke a factory reset regardless.
            /// </summary>
            [DefaultValue(false)]
            public bool FactoryReset { get; set; } = false;

            /// <summary>
            /// Used to specify which sandbox to switch the device to if
            /// desired.
            /// </summary>
            [DefaultValue("")]
            public string DesiredSandboxId { get; set; } = string.Empty;
        }

        public class FileRetrievalParameters
        {
            /// <summary>
            /// The directory path to download files from. Path must start with D:\Titles.
            /// </summary>
            public required string DirectoryPath { get; set; }
        }
    }
}
