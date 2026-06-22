//-----------------------------------------------------------------------------
// FilesController.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

using System.Net.Mime;
using Microsoft.AspNetCore.Mvc;

namespace XboxAgentServerSample.Controllers
{
    /// <summary>
    /// For use with the file jobs <see cref="Models.DownloadFilesJob"/> and
    /// <see cref="Models.UploadFilesJob"/>.
    /// </summary>
    [Route("api/[controller]")]
    [ApiController]
    public class FilesController : ControllerBase
    {
        private readonly string _rootFilePath = Path.GetFullPath("Files");

        /// <summary>
        /// An endpoint to support FileDownloadJob examples.
        /// </summary>
        /// <param name="filePath"></param>
        /// <returns>The file requested, or 404 Not Found if the file can't be located.</returns>
        [HttpGet("{filePath}")]
        [ProducesResponseType(StatusCodes.Status200OK)]
        [ProducesResponseType(StatusCodes.Status404NotFound)]
        public IActionResult FilesDownload(string filePath)
        {
            // Validate the input so it is only looks for files under the root directory
            if (filePath.Contains(".."))
            {
                return BadRequest("Invalid path");
            }

            string fileName = Path.GetFileName(filePath);

            string realFilePath = Path.Combine(_rootFilePath, fileName);

            if (!System.IO.File.Exists(realFilePath))
            {
                return NotFound();
            }

            FileStream fileStream = new(realFilePath, FileMode.Open, FileAccess.Read);
            return File(fileStream, MediaTypeNames.Application.Octet, Path.GetFileName(filePath));
        }

        /// <summary>
        /// An endpoint to support FileUploadJob examples.
        /// </summary>
        /// <param name="file">The file being uploaded.</param>
        /// <returns>200 OK</returns>
        [HttpPost]
        [ProducesResponseType(StatusCodes.Status200OK)]
        public async Task<IActionResult> FilesUpload(IFormFile file)
        {
            using FileStream fileStream = new(Path.Combine(_rootFilePath, file.FileName), FileMode.Create, FileAccess.Write);
            await file.CopyToAsync(fileStream);

            return Ok();
        }

        /// <summary>
        /// An endpoint that actually supports FileUploadJob examples.
        /// </summary>
        /// <param name="fileName">The name of the file being uploaded.</param>
        /// <returns>200 OK</returns>
        [HttpPost("{fileName}")]
        [ProducesResponseType(StatusCodes.Status200OK)]
        public async Task<IActionResult> FilesUpload(string fileName)
        {
            using FileStream fileStream = new(Path.Combine(_rootFilePath, fileName), FileMode.Create, FileAccess.Write);
            await Request.Body.CopyToAsync(fileStream);

            return Ok();
        }
    }
}
