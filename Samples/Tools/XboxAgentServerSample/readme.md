  ![](./media/image1.png)

#   Xbox Agent Server Sample

## Description

This sample demonstrates a web service receiving heartbeat requests from the Devkit Agent.
The sample is designed to be simple in its functionality and display the contents of the heartbeat for the selected console.
You can send job requests to the target devkit using the UI provided and see the heartbeat request update as the jobs are processed.

Multiple devkits can target the sample server allowing you to issue jobs to each devkit one at a time. 

For more information on the devkit agent feature see [DevKit Agent overview](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/devkitagent-overview).


## Building and configuring the sample

To build the sample, load up the solution in Visual Studio 2022, then modify XboxAgentServerSample\Properties\launchSettings.json and XboxAgentServerSample\appsettings.json as follows:

 - Replace the `[server computer name]` placeholders with the local network name of the PC running the sample.

 - In HttpsInlineCertStore configure the URL and Certificate settings as appropriate for your SSL cert. 
	
> [!NOTE]
> Self signed certs are supported and in this example we use the CurrentUser/My store to store the cert.

## Required certificates

The following certificates are needed to run the sample:

 - SSL cert with public and private keys installed on the server to enable HTTPS traffic with the devkits.  This can be a self-signed certificate, but the certificate's name and subject must be the machine's network name or else it will not be trusted for SSL traffic.
 - SSL cert's public key certificate to be installed on the devkits to enable HTTPS calls to the server. Copy to xs:\Microsoft\Cert
 - Relying Party cert with public and private keys installed on the server to enable the decryption of the XSTS tokens from the devkit for your Relying Party. 
 
> [!NOTE]
> If the server is using a root trusted SSL cert, you do not need to install the SSL public cert to the devkits.

On the server, both the SSL and Relying Party public and private key certs should be installed to the `Certificates - Current User\Personal` cert store.
This allows the server while running in Visual Studio 2022 and Kestrel to access them.

If you do not already have a Relying Party configured in Partner Center, please see [this article](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/live-web-services.html#corepa).

### Creating a self signed SSL cert

From an elevated PowerShell command prompt run the following command.
Replace the placeholder text with the computer name where the sample is being hosted.

> ``` New-SelfSignedCertificate -CertStoreLocation Cert:\CurrentUser\My -DnsName "[server computer name]" -FriendlyName "[server computer name]" -NotAfter (Get-Date).AddYears(10) ```

### Exporting a self signed SSL cert

From an elevated PowerShell command prompt run the following command.
Replace the placeholder text with the ssl certificate thumbprint and the local path where you want to store the .cer file.

> ``` Export-Certificate -Cert Cert:\CurrentUser\My\[ssl certificate thumbprint] -FilePath [path to .cer file] ```

<a id="configureFirewall"></a>
### Configuring the firewall on your PC to allow traffic to the sample

Open a Powershell command prompt with Administrator access and run the following commands.
Replace the placeholder text with the thumbprint of the SSL cert you are using.

> ```netsh advfirewall firewall add rule name="XboxAgentServerSample" dir=in protocol=tcp localport=8733 action=allow```

> ```netsh http add urlacl url=https://+:8733/ user=Everyone```

>```netsh http add sslcert ipport=0.0.0.0:8733 certhash=[ssl certificate thumbprint] ```

<a id="configureAgent"></a>
### Configure your devkit to communicate with the sample

The following commands are used with a VS Gaming Command Prompt window to have your devkit target the sample for heartbeats.
Replace the placeholder text with the computer name where the sample is being hosted and the relying party name that the devkit should use to authorize with the sample (ex: rp://relyingparty.contoso.com/).

> ``` xbconfig DevkitAgentServiceUri=https://[server computer name]:8733/api ```
> ``` xbconfig DevkitAgentRelyingParty=[your relying party name] ```

To install the SSL public key cert on a devkit, it needs to be copied with the following command in a VS Gaming Command Prompt window:  

> ``` xbcp [path to .cer file] xs:\Microsoft\Cert ```


## Using the sample

Once you have properly configured the sample, devkits that are configured to send a heartbeat to your server will appear under the Xbox Development Kits header. 

Click on each devkit's name to see details about the most recent HeartbeatRequest or issue jobs to the devkit using the options and buttons in the right side panel.

> [!NOTE]
> The page does not automatically refresh when a heartbeat is sent from a devkit.  To refresh, click the refresh button in the upper left.


When issuing jobs to the console, these are console command jobs as detailed in [Console-based command-line tools](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/consolecommandlinetools). Some simple examples are [wdapp list](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/wdapp), [wdapp launch](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/wdapp), and [wdconfig sandboxid](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/wdconfig).

For more information and possible operations you can use with the devkit agent, see [DevKit Agent overview](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/devkitagent-overview).

## Troubleshooting

One of the best ways to troubleshoot issues is to set the logging level of the DevKit Agent to verbose (xbconfig DevkitAgentDesiredLogLevel=verbose) and use xbWatson to see the traffic from the devkit agents obtaining xsts tokens, sending heartbeats, and possible certificate issues in the SSL authentication flow.

### Verify the web service is running and can receive HTTPS traffic from your PC

Make sure that the server is able to compile and run in Visual Studio 2022.
Once the server is running, open up a web-browser and go to the sample's home page using your PC's name and not localhost:
> ``` https://[server computer name]:8733 ```

If the homepage of the server does not show, then it is possible Kestrel and your firewall are not configured properly.
Try running the configuration commands from [Configure your devkit agent to communicate with the sample](#configureFirewall).

You will also want to try and access the sample using a web browser from a different PC on the same network.

### Verify outgoing traffic to the server from the devkit

Enable Fiddler tracing on your devkit as outlined in [Fiddler on Xbox devkits](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/fiddler-setup-networking).
With Fiddler running, check for heartbeat traffic going from the devkit to your server.
If there are no calls shown in Fiddler for the URL of your PC running the sample, try running the commands again under [Configure your devkit agent to communicate with the sample](#configureAgent).

### Verify the devkit is able to complete HTTPS calls to the sample

If the devkit is making calls to the sample server but they are failing to complete the SSL handshake, but does work when running the devkit traffic through Fiddler, the following could possibly be the issue:

- Your SSL certificate's `subject` or `issued to` values do not match the host name of the PC in the URL that the devkits are using to send heartbeats.
- The SSL certificate being used by the server does not match the public certificate installed on the devkit under [Configure your devkit agent to communicate with the sample](#configureAgent)

First verify the certificate being used when an HTTPS call is made to the server by open up a web-browser and go to the sample's home page using your PC's name and not localhost:
> ``` https://[server computer name]:8733 ```

When the homepage shows, check the lock icon on the browser address bar or `View site information`.
Different web browsers have different methods for getting to this data, see how to look this up for the browser you are using.
When you are able to pull up the SSL certificate information being presented by the server, check the name and thumbprint.
Make sure that the thumbprint matches the one in the .cer file that you are copying over to the devkit.
Additionally make sure that Issued To value of the certificate matches the host name in the URL that the devkit agents are using.

Example:  
URL to the sample server is https://mylocalPC:8733.  Only a certificate issued to 'mylocalPC' would be accepted by the devkit for https traffic. 

### Verify that the server is able to decrypt the XSTS tokens sent in the heartbeats

Heartbeats from the devkit to the sample that result in HTTP 403 errors indicate that the server is unable to verify the XSTS Tokens being sent.

First, verify that you have provided the correct relying party name when [running the devkit agent configuration commands](#configureAgent).
You can also look through the fiddler calls from the console after a reboot to verify that calls to XSTS with your relying party name in the request body are coming back with tokens.

Second, verify that the sample is able to access the relying party's private certificate key on the PC.  
Looking at the debug output from the server running in Visual Studio, you should see logs or warnings that indicate what happened.
Alternatively you can put breakpoints in the ValidateAuthorizationHeaderWithCache API and step through it to see where the error is happening.
If the sample is complaining that it cannot find a certificate matching the thumbprint in the XSTS token header, make sure of the following:

- You have the private key of the certificate matching the thumbprint from the XSTS Token is installed on your PC
- The certificate and private key are installed to the `Certificates - Current User\Personal` cert store.

## Implementation notes

This sample is designed to be portable, written in ASP.NET, and uses the cross platform [Kestrel Web Server](https://learn.microsoft.com/en-us/aspnet/core/fundamentals/servers/kestrel?view=aspnetcore-7.0). 

## Known issues

The home page does not automatically refresh, use the refresh page button to see the latest heartbeat requests.

## Update history

| **Date**         |  **Version** |  **Description**                        |
|------------------|--------------|-----------------------------------------|
| October 18, 2023 |  1.0         |  Initial release                        |

## Privacy Statement

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).