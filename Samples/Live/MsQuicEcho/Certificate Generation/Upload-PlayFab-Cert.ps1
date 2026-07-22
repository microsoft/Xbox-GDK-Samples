# Parameters
$secretKey = "YOURTITLESECRETKEY"
$titleId = "YOURTITLEID"
$jsonPath = "$env:TEMP\upload-cert.json"

# 1. POST request to get the entity token
$getTokenBody = @{} | ConvertTo-Json
$getTokenHeader = @{
    "X-SecretKey" = $secretKey
    "Content-Type" = "application/json"
} 
$response = Invoke-RestMethod -Uri "https://$titleId.playfabapi.com/Authentication/GetEntityToken" -Method 'Post' -Body $getTokenBody -Headers $getTokenHeader
$entityToken = $response.data.EntityToken

# 2. POST request to upload certificate using the entity token
$uploadCertBody = Get-Content $jsonPath -Raw
$uploadCertHeader = @{
    "X-EntityToken" = $entityToken
    "Content-Type" = "application/json"
} 
Invoke-RestMethod -Uri "https://$titleId.playfabapi.com/MultiplayerServer/UploadCertificate" -Method 'Post' -Body $uploadCertBody -Headers $uploadCertHeader | ConvertTo-HTML