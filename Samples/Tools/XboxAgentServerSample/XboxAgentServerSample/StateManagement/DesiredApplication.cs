//-----------------------------------------------------------------------------
// DesiredApplication.cs
//
// Xbox Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License file under the project root for
// license information.
//-----------------------------------------------------------------------------

namespace XboxAgentServerSample.StateManagement
{
    /// <summary>
    /// Represents the application that should be on a device.
    /// </summary>
    public class DesiredApplication
    {
        public DesiredApplication()
        {
            FullName = "";
            DisplayName = "";
            InstallPackageUrl = new Uri("");
            DependencyPackageUrls = new List<Uri>();
        }

        public DesiredApplication(string fullName, string displayName, Uri installPackageUrl, List<Uri> dependencyPackageUrls)
        {
            FullName = fullName;
            DisplayName = displayName;
            InstallPackageUrl = installPackageUrl;
            DependencyPackageUrls = dependencyPackageUrls;
        }

        /// <summary>
        /// The name to compare against to ensure the application is installed.
        /// </summary>
        public string FullName { get; }

        /// <summary>
        /// The friendly name of the application.
        /// </summary>
        public string DisplayName { get; }

        /// <summary>
        /// The URL path to download the installation appx package.
        /// </summary>
        public Uri InstallPackageUrl { get; }

        /// <summary>
        /// The list of URL Paths to download the dependency packages.
        /// </summary>
        public List<Uri> DependencyPackageUrls { get; }
    }
}
