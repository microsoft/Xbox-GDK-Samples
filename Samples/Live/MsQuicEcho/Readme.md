This sample is to demonstrate how to implement MsQuic using a simple Echo Server as an example. 

// Here is how the echo server works:
// 1. Initialize MsQuic
// 2. Create a listener to accept incoming connections
// 3. For each incoming connection, create a session
// 4. For each session, create a stream to handle incoming data
// 5. Read data from the stream and echo it back
// 6. Clean up resources when done
// 7. Handle errors and connection closures gracefully

// To run the MsQuic Echo Server sample, follow these steps:

// The latest version of PowerShell is needed to run the executable 
Install the latest version of PowerShell and use it for running the builkd on the command line:
https://learn.microsoft.com/powershell/scripting/install/installing-powershell-on-windows?view=powershell-7.5

// This project uses vcpkg to manage dependencies. 
Set up vcpkg with the instructions here: https://learn.microsoft.com/vcpkg/get_started/get-started-msbuild?pivots=shell-powershell
The project is already set up with vcpkg, so there is no need to change any settings. You will find the vcpkg file is already there.
Make sure to set the VCPKG_ROOT.
You can also use vcpkg as a component in Visual Studio: https://devblogs.microsoft.com/cppblog/vcpkg-is-now-included-with-visual-studio/

// Open the solution in Visual Studio and build
1. Open Visual Studio and load the `EchoServerSample.sln` solution file in gx_dev\Samples\Live\MsQuicEcho.
2. Ensure platform is set to x64 and configuration is set to Release or Debug.
3. Build the solution! This will restore the necessary dependencies as well as build the MsQuic Echo Server sample.

// Generate the certificate
// In 'Generate-MsQuicCert.ps1', you can set the certificate name and the password, which you will use when attaching the certifate to your server build in PlayFab.
// Use the 'Generate-MsQuicCert.ps1' script to generate a self-signed certificate for the server. 
// Check your certificates folder (%appdata%\Microsoft\SystemCertificates\My\Certificates) to see the certificate, named by its cert-hash.
// To test the Sample locally, run the following in two separate PowerShell prompts:
1. .\EchoServerSample.exe  -server -cert_hash:<cert-hash>
2. .\EchoServerSample.exe -client -unsecure -target:127.0.0.1

// Upload the certificate to PlayFab
// To upload your certificate to your title in PlayFab, go to the PlayFab title to get the Title ID and Secret Key.
// The Title ID is in the URL or listed on the My Studios and Titles page.
// Go to the Settings for the PlayFab title get the Secret Key to properly reference it.
// Edit the upload-playfab-cert.bat file to contain your title's ID and Secret Key.
// Run the upload-playfab-cert.bat file to upload the certificate to PlayFab.

// Building the sample to upload to PlayFab.
// You will need to build the sample and zip the assets before uploading them to PlayFab.
1. In build.ps1, change "C:\PATH\TO\vcpkg" to the filepath to your vcpkg folder.
2. Run the build.ps1 file in wrappingGsdk to package all these files in wrappingGsdk\drop\gameassets.zip.

// When making a new server build for the PlayFab title set the following options:
// In Server details, set the server type to container.
// Upload the zipped assets file gameassets.zip to the Assets section.
// Set the start command: C:\Assets\wrapper.exe -g C:\Assets\EchoServerSample.exe -server -cert_hash:<cert-hash>
// In Network, set the Port to 4567, the Name to gameport, and the Protocol to UDP.
// Upload the pfx file that was generated for the certificate from the Temp folder named with the name chosen in Generate-MsQuicCert.ps1.
// Save the server build and deploy it to the PlayFab title.

// Testing with a client
// Once the build is deployed, request a server instance from PlayFab for it.
// In the session details, you will find the IP address and port for the server.
// Use the following command to run the client through PowerShell:
1. .\EchoServerSample.exe -client -unsecure -target:<IP-Address> -port:<port>

// A successful readout will look like this:
Argument 0: D:\source\EchoServerSample\gx_dev\Samples\Live\MsQuicEcho\x64\Release\EchoServerSample.exe
Argument 1: -client
Argument 2: -unsecure
Argument 3: -target:<IP-Address>
Argument 4: -port:<port>
Beginning
EchoBegin
Running Client
[conn][00000252A76BD840] Connecting...
Using external port 30002
Client Running.

[conn][00000252A76BD840] Connected
[strm][00000252A8F26BC0] Starting...
Enter your message: Hello World
[strm][00000252A8F26BC0] Sending data...
[strm][00000252A8F26BC0] Data sent
[conn][00000252A76BD840] Resumption ticket received (56 bytes):
0100000001310001026710030245C00404810000000504800100000604800100000704800100000801010E0104C0000000FF03DE1A027E80
[strm][00000252A8F26BC0] Data received
Echo: Hello World

RTT: 100450 microseconds
[strm][00000252A8F26BC0] Peer shut down
[strm][00000252A8F26BC0] All done
[conn][0000026E1DF8D840] Successfully shut down on idle.
[conn][0000026E1DF8D840] All done
Exited MsQuic