# Parameters
$certName = "MsQuicCertSample"
$certPassword = "CreatePassword"
$certPasswordSecure = ConvertTo-SecureString -String $certPassword -Force -AsPlainText
$pfxPath = "$env:TEMP\$certName.pfx"
$base64Path = "$env:TEMP\$certName.pfx.b64"
$jsonPath = "$env:TEMP\upload-cert.json"

# 1. Create a self-signed certificate with multiple DNS names used for testing
$cert = New-SelfSignedCertificate `
    -DnsName $env:computername,localhost,"msquic.test" `
    -CertStoreLocation "Cert:\CurrentUser\My" `
    -KeyExportPolicy Exportable `
    -KeyUsage DigitalSignature `
    -KeyUsageProperty Sign `
    -FriendlyName $certName `
    -NotAfter (Get-Date).AddYears(1)

# 2. Export the certificate to a PFX file
Export-PfxCertificate `
    -Cert $cert `
    -FilePath $pfxPath `
    -Password $certPasswordSecure

# 3. Convert the PFX to Base64 for PlayFab upload
[Convert]::ToBase64String([IO.File]::ReadAllBytes($pfxPath)) | Out-File -Encoding ascii $base64Path

# 4. Create JSON file for PlayFab upload with password
$jsonContent = @{
    GameCertificate = @{
        Name = $certName
        Password = $certPassword
        Base64EncodedValue = ([System.IO.File]::ReadAllText($base64Path)).Trim()
    }
} | ConvertTo-Json

$jsonContent | Out-File -Encoding ascii $jsonPath

Write-Host "Certificate created and exported:"
Write-Host "PFX file: $pfxPath"
Write-Host "Base64 file: $base64Path"
Write-Host "JSON file: $jsonPath"
