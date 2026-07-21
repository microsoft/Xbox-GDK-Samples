<#

.SYNOPSIS
Generates a header containing a string with all the APIs from an xml file

.DESCRIPTION
This script is used to generate the GameAPIHeader.h for the xbdepends tool.

THIS SCRIPT IS NOT FOR DISTRIBUTION. IT'S ONLY USED FOR SAMPLE MAINTAINENCE

.PARAMETER PathToXML
Provides the path to the XML file containing the APIs

.PARAMETER OutputHeader
Provides the path to the output C++ header file to create

.EXAMPLE
GameAPIHeaders.ps1 gamecoreapis.xml GameOSAPIs.h

#>

param (
    [Parameter(
        Mandatory,
        Position = 0
    )]
    [string]$PathToXML,
    [Parameter(
        Position = 1
    )]
    [string]$OutputHeader = "GameOSAPIs.h")


if (-not (Test-Path $PathToXML)) {
    Write-Error -Message "ERROR: Input XML file not found" -ErrorAction Stop
}

Write-Host "Reading: " $PathToXML
[xml]$gcxml = Get-Content -Path $PathToXML

$apilist = $gcxml.Apis.ApiGroup.Exports.Export.Name | Sort-Object | Get-Unique

Write-Host "Found " $apilist.count " APIs"

Write-Host "Writing: " $OutputHeader

New-Item $OutputHeader -ItemType File -Force -Value ("//--------------------------------------------------------------------------------------" + [Environment]::NewLine)
Add-Content $OutputHeader ("// File: " + $OutputHeader)
Add-Content $OutputHeader "//"
Add-Content $OutputHeader "// Microsoft Xbox Binary Dependencies Tool - Game APIs data"
Add-Content $OutputHeader "//"
Add-Content $OutputHeader "// Copyright (C) Microsoft Corporation. All rights reserved."
Add-Content $OutputHeader "//--------------------------------------------------------------------------------------"
Add-Content $OutputHeader ""
Add-Content $OutputHeader "#pragma once"
Add-Content $OutputHeader ""
Add-Content $OutputHeader "namespace KnownAPIs"
Add-Content $OutputHeader "{"
Add-Content $OutputHeader "    const char* c_GameOSAPIs[] ="
Add-Content $OutputHeader "    {"
ForEach ($api in $apilist) { Add-Content $OutputHeader ('        "' + $api + '",') }
Add-Content $OutputHeader "    };"
Add-Content $OutputHeader "}"

