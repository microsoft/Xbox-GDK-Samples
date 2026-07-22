$scriptRoot = Get-Location
$dropDir = "$scriptRoot\drop"
New-Item -ItemType Directory -Force -Path $dropDir

dotnet publish $scriptRoot\wrapper\wrapper.csproj -c release -o $dropDir\w10x64 --runtime win-x64 /p:PublishSingleFile=true

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msbuildPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe
$echoServerSampleProj = "C:\Users\emilymattlin\gx_dev\Samples\Live\MsQuicEcho\EchoServerSample.vcxproj"

& $msbuildPath $echoServerSampleProj `
  "/p:Configuration=Release" `
  "/p:OutDir=$dropDir\w10x64\" `
  "/p:Platform=x64"

$vcpkgRoot = "C:\PATH\TO\vcpkg"
Copy-Item "$vcpkgRoot\packages\msquic_x64-windows\bin\msquic.dll" -Destination $dropDir\w10x64

Compress-Archive -Path $dropDir\w10x64\* -DestinationPath $dropDir\gameassets.zip -Force
echo "game assets successfully zipped at $dropDir\gameassets.zip"

# open folder
Invoke-Item $PSScriptRoot\drop







