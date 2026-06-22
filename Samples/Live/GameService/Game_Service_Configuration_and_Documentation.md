![Xbox Logo](./media/image1.png)

# Sample Configuration Guide and Documentation

By Cameron Goodwin  
Xbox Advanced Technology Group

**This document is confidential** and provided to you under a Microsoft non-disclosure agreement.
While we have tried to ensure the accuracy of this document, we provide no express warranties or guarantees regarding the information.
The information is subject to change.
Microsoft may have intellectual property rights in the subject matter of this paper.
This document doesn't grant you a license to those rights---it's for informational purposes only.

## Abstract

This guide walks you through configuring an Azure App Service to run the Game Service sample and connect it to your Xbox Live enabled title.
You'll learn how to handle authorization between client and server using Xbox Secure Token Service (XSTS) tokens (also called X-tokens) and call Xbox Live services from your server.
The guide also covers design decisions and best practices in game service development.

## Contents

[Update history](#update-history)  

[Introduction](#introduction)  

[Prerequisites](#prerequisites)  

[1. Handling X-tokens](#1-handling-x-tokens)  
&emsp;[1.1 Configuring the NSAL, X-token definitions, and Relying Parties in Partner Center](#11-configuring-the-nsal-x-token-definitions-and-relying-parties-in-partner-center)  
&emsp;&emsp;[1.1.1 Configuring your Relying Party to use an asymmetric certificate](#111-configuring-your-relying-party-to-use-an-asymmetric-certificate)  
&emsp;&emsp;[1.1.2 Creating a self-signed Relying Party certificate](#112-creating-a-self-signed-relying-party-certificate)  
&emsp;&emsp;[1.1.3 Defining your Web Service, Relying Party and X-token claims](#113-defining-your-web-service-relying-party-and-x-token-claims)  
&emsp;&emsp;[1.1.4 Defining the service endpoint in your App's NSAL](#114-defining-the-service-endpoint-in-your-apps-nsal)  
&emsp;[1.2 Building the sample and debugging locally](#12-building-the-sample-and-debugging-locally)  
&emsp;&emsp;[1.2.1 Building the sample in Visual Studio](#121-building-the-sample-in-visual-studio)  
&emsp;&emsp;[1.2.2 Enabling Application Settings and Certs when debugging locally](#122-enabling-application-settings-and-certs-when-debugging-locally)  
&emsp;&emsp;[1.2.3 Running and debugging the sample locally](#123-running-and-debugging-the-sample-locally)  
&emsp;&emsp;[1.2.4 Using Fiddler to debug request calls locally](#124-using-fiddler-to-debug-request-calls-locally)  
&emsp;[1.3 Deploying to an Azure App Service and debugging remotely](#13-deploying-to-an-azure-app-service-and-debugging-remotely)  
&emsp;&emsp;[1.3.1 Adding certificates with private keys to your deployed service](#131-adding-certificates-with-private-keys-to-your-deployed-service)  
&emsp;&emsp;[1.3.2 Configuring the App Settings in Azure](#132-configuring-the-app-settings-in-azure)  
&emsp;&emsp;[1.3.3 Creating a debug deployment to Azure for debugging](#133-creating-a-debug-deployment-to-azure-for-debugging)  
&emsp;&emsp;[1.3.4 Attaching the Visual Studio Debugger to your Azure App Service](#134-attaching-the-visual-studio-debugger-to-your-azure-app-service)  
&emsp;[1.4 Using the SimpleWinHttp sample to verify your service and NSAL configuration](#14-using-the-simplewinhttp-sample-to-verify-your-service-and-nsal-configuration)  
&emsp;[1.5 Viewing log output from the service](#15-viewing-log-output-from-the-service)  
&emsp;&emsp;[1.5.1 Viewing log output when running locally through Visual Studio](#151-viewing-log-output-when-running-locally-through-visual-studio)  
&emsp;&emsp;[1.5.2 Enabling service log output through Azure](#152-enabling-service-log-output-through-azure)  
&emsp;&emsp;[1.5.3 Viewing service log output real-time when running on Azure](#153-viewing-service-log-output-real-time-when-running-on-azure)  
&emsp;&emsp;[1.5.4 Viewing service log output real-time from Visual Studio](#154-viewing-service-log-output-real-time-from-visual-studio)  

[2. Service Auth X-tokens](#2-service-auth-x-tokens)  
&emsp;[2.1 Creating a Business Partner certificate](#21-creating-a-business-partner-certificate)  
&emsp;[2.2 Adding the cert's thumbprint for initialization to the App Settings](#22-adding-the-certs-thumbprint-for-initialization-to-the-app-settings)  
&emsp;[2.3 Verifying your service can obtain a Service Token](#23-verifying-your-service-can-obtain-a-service-token)  

[3. Delegated Auth X-tokens](#3-delegated-auth-x-tokens)  

[4. Calling Xbox Live Services](#4-calling-xbox-live-services)  

[5. Collections and Commerce](#5-collections-and-commerce)  
&emsp;[5.1 Configuring your products for B2B Collections in Partner Center](#51-configuring-your-products-for-b2b-collections-in-partner-center)  

[6. Logging & Correlation Vectors](#6-logging--correlation-vectors)  

[Appendix A: Setting up an Azure SQL Database](#appendix-a-setting-up-an-azure-sql-database)  
&emsp;[A.1 Creating an Azure SQL Database](#a1-creating-an-azure-sql-database)  
&emsp;[A.2 Initializing the database with Migrations and debugging locally](#a2-initializing-the-database-with-migrations-and-debugging-locally)  

[FAQs and troubleshooting](#faqs-and-troubleshooting)  

## Update history

| **Date**          |  **Version** |  **Description** |
|-------------------|----------|------------------------------------------------|
| January 25, 2019  |  1.3     |  -   Redesigned and re-organized into different sections that build on top of each other.  |
| July 30, 2019     |  1.4     |  -   Added note about License Token functionality and updated Troubleshooting answer related to SSL cert issues          |
| November 1, 2019  |  1.5     |  -   Renamed to "Game Service Sample" -   Updated to .NET Core 3.0 and Visual Studio 2019 -   Terminology and naming updated to match the Xfest 2019 talk XSTS Auth and Server to Server Made Easy -   Migrated locally cached items to in-memory cache rather than in-memory database -   Removed Azure Key Vault usage and migrated secrets and certs to App Settings -   Reordered and condensed the sections in the configuration guide -   Moved source code files to match the new sections layout |
| January 24, 2020  |  1.5     |  -   Text updates to Configuration Guide and Readme only                                |
| February 25, 2020 |  1.7     |  -   Added GDNP erasure list b2b endpoint -   Fixed bug with S-token caching that caused errors generating signature headers        |
| May 12, 2020      |  1.8     |  -   Removal of Symmetric token encryption handling -   Added XBL signing cert caching at server startup using the target URI <https://xsts.auth.xboxlive.com/xsts/signingkeys> |
| July 27, 2022     |  1.2207  |  -   Version number updated to now be format of 1.YYMM of the last update -   Update to VS 2022 -   Update to .NET 6.0 -   Updated to latest NuGet packages -   Updated Decrypt functions using Stream.Read to get the decryption buffer per .NET 6.0 change described here: <https://docs.microsoft.com/en-us/dotnet/core/compatibility/core-libraries/6.0/partial-byte-reads-in-streams>  |
| April 24, 2023    |  1.2304  |  -   Added support for JKU inner token signature handling, Update to latest NuGet and .NET 7.0 packages  |  
| October 13, 2025  | 1.2510   |  -   Refactored the token handling code to simplify the overall process and make the handling code more stand-alone and portable.  - Updated to .NET 9.0 and latest NuGet packages. |

# Introduction

The Xbox Live platform uses HTTPS communication with RESTful web services.
When you add HTTPS and RESTful services to your Xbox Live enabled title, you gain the flexibility to develop game services that are quick and reliable.
This guide shows you how to configure and run the Game Service sample on your development PC and in Azure.
You'll explore how to use Xbox Secure Token Service (XSTS) tokens and HTTPS for secure communication between your Xbox Live enabled title and your custom game services.

## Prerequisites

Before you begin, ensure you have the following software, accounts, and access:

### Required Software

- **Visual Studio 2022 or later** - [Download here](https://visualstudio.microsoft.com/downloads/)
  - Workload: ASP.NET and web development
  - Workload: .NET desktop development
- **.NET 9.0 SDK** - [Download here](https://dotnet.microsoft.com/download/dotnet/9.0)
- **Windows PowerShell** - Pre-installed on Windows (for certificate generation)
- **Microsoft Game Development Kit (GDK)** - Required for testing with Xbox Live enabled titles
  - [Download from GDK Download site](https://aka.ms/gdkdl)

### Required Accounts and Access

- **Partner Center Account** with access to:
  - Xbox Live configuration
  - Web Services management
  - Relying Party configuration
  - Your title's configuration
- **Azure Subscription** with permissions to:
  - Create App Services
  - Upload certificates
  - Configure App Settings
  - Access to a Resource Group (or ability to create one)

### Optional Tools

- **HTTP debugging tool** - Choose one:
  - [Fiddler Classic](https://www.telerik.com/fiddler) or Fiddler Everywhere
  - [Postman](https://www.postman.com/)
  - [Bruno](https://www.usebruno.com/)
  - VS Code with REST Client extension
- **Azure CLI** (optional) - [Download here](https://docs.microsoft.com/en-us/cli/azure/install-azure-cli)
- **Git** (optional) - For source control

### Development Environment

- **Xbox Development Console** or **PC with GDK** configured for your sandbox
- **Network access** to:
  - Partner Center
  - Azure Portal
  - Xbox Live services
- **Local administrator rights** (required for installing certificates and running PowerShell scripts)

### Knowledge Prerequisites

This guide assumes you have:

- Basic understanding of Xbox Live authentication concepts
- Familiarity with C# and ASP.NET Core
- Basic knowledge of Azure App Services
- Understanding of HTTPS/SSL certificates
- Familiarity with REST APIs

> **New to Xbox Live?** If you're unfamiliar with Xbox Live development, we recommend reviewing the [Xbox Live overview documentation](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/live-get-started) before proceeding.

## 1. Handling X-tokens

This section walks you through configuring Partner Center to enable your title to obtain X-tokens from the client and run the sample on your development PC.
By the end of this section, your title will be able to call the sample service and receive a reply containing all the claims from the client's X-token used for authentication.

If you're unfamiliar with X-tokens or single sign-on authentication for Xbox Live enabled titles, we recommend reviewing the Xfest 2019 talk *XSTS Auth and Server to Server Made Easy* (available under [Conference Material on the GDK Download site](https://aka.ms/gdkdl)) and the following documentation articles:

- [Xbox Live authentication
    (microsoft.com)](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/live-xbox-live-authentication)

- [Xbox Live security tokens (XSTS tokens)
    (microsoft.com)](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/live-security-tokens)

- [Xbox Live authentication for title services
    (microsoft.com)](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/live-title-service-authentication)

The sample's source code comments provide additional details about the token validation and handling process.

### 1.1 Configuring the NSAL, X-token definitions, and Relying Parties in Partner Center

Before your title can communicate with your web service using X-tokens, you need to complete three tasks in Partner Center:

1. Define a Web Service
2. Create a Relying Party (RP)
3. Publish your title's Network Security Access List (NSAL) to your sandbox via the Xbox Live Single sign-on configuration page

Let's start by understanding how asymmetric encryption works with X-tokens.

#### 1.1.1 Configuring your Relying Party to use an asymmetric certificate

Asymmetric encryption uses two keys: a public key and a private key. Here's how they work together:

- **Public key**: Encrypts the token's Content Encryption Key (CEK), which encrypts the token's payload
- **Private key**: Your service uses this to decrypt the CEK, then decrypts the token's payload
- **Digital signature**: Proves the token came from Xbox Live (verified using a public key from Xbox Live endpoints)

Even if an attacker obtains your public key and creates fake X-tokens, they can't generate a valid signature without Xbox Live's private key.
Therefore, validating the signature is an important step of validating the authenticity of a token.

The Game Service sample automatically fetches and caches the current and upcoming Xbox Live signing certificates (x5u) and keys (jku) during server startup.
The sample also includes code to fetch additional keys at runtime from the inner-token's header values.
If the X5U or JKU values don't point to an endpoint under \*.xboxlive.com, the service rejects the token, fails authentication, and logs a warning.

When Xbox Live renews its signing certificate or introduces a new JKU, all new tokens include the updated x5u and jku values.
The sample service automatically detects this change, fetches the new certificate or keys, and continues validating tokens without manual intervention.

#### 1.1.2 Creating a self-signed Relying Party certificate

Now that you understand how asymmetric encryption works, let's create the certificate you'll need for your Relying Party.

1. On your computer, open **Windows PowerShell** as Administrator.

2. Run the following command to create a self-signed certificate, replacing the example name (Contoso) with your own:

```powershell
# Create the certificate
$cert = New-SelfSignedCertificate `
    -Subject "CN=Contoso Relying Party" `
    -CertStoreLocation "Cert:\CurrentUser\My" `
    -KeyExportPolicy Exportable `
    -KeySpec KeyExchange `
    -KeyLength 2048 `
    -KeyAlgorithm RSA `
    -HashAlgorithm SHA256 `
    -NotBefore (Get-Date) `
    -NotAfter (Get-Date).AddYears(10)

# Export the public key (.cer)
Export-Certificate -Cert $cert -FilePath "RP_Cert.cer" -Type CERT

# Export the full certificate with private key (.pfx)
$password = ConvertTo-SecureString -String "YourSecurePassword" -Force -AsPlainText
Export-PfxCertificate -Cert $cert -FilePath "RP_Full_Cert.pfx" -Password $password
```

3. The certificate and private key are now in your certificate store. Use the `RP_Cert.cer` file for Partner Center configuration.

4. Hold on to the `RP_Full_Cert.pfx` file as we will upload it to Azure shortly.

**Important Notes:**

- When importing the full certificate, make sure you select the **Mark this key as exportable** option so that you can export it to other servers if needed.
- **Protect your private keys:** The `.pvk` and `.pfx` files contain private keys. Store them securely and never commit them to source control.

If you ever need to re-export the public key .cer for this certificate, select the **Base-64 encoded X.509 (.CER)** option in the Certificate Export Wizard.

**How to verify:**

- Open **Windows Certificate Manager** by pressing `Win + R`, typing `certmgr.msc`, and pressing Enter
- Navigate to **Personal** > **Certificates**
- Look for the certificate with the subject name you specified (e.g., "Contoso Relying Party")
- Double-click to verify the certificate details, expiration date, and that it has a private key (shown as "You have a private key that corresponds to this certificate" in the General tab)

#### 1.1.3 Defining your Web Service, Relying Party and X-token claims

1. In Partner Center, Select the **Settings Icon in the upper right and then Developer Settings**  
![Partner Center Settings menu with Developer Settings option](./media/image2.jpg)

2. Next select **Xbox Live-\>Relying Parties**  
![Xbox Live menu showing Relying Parties option](./media/image3.png)

3. On the Relying parties page, select **New relying party**

4. Provide an Audience URI\* (e.g., `rp://contoso.com/`)

5. Select **Asymmetric encryption -- JWE RFC 7516** for Encryption Type and enable the checkbox for JKU support (added April 2023)

6. Click browse your files and select the RP_Cert.cer file you created on step 6 above.

7. Enter how long (in hours) you want your token's lifetime to be (recommended 4 hours)

8. Add the **Partner Xbox User ID (ptx)** claim to the token (this will be the unique ID you will have for each user in your database).

9. Click **Save**

\* The Audience URI must be in the form of a host URI, but the name can be different from the URI of the actual service.
For example, `rp://contoso.com/` could be Audience URI of the Relying Party, but the service endpoint is actually https:*//game.contoso.com/action/*.

**How to verify:**

- Return to the **Relying Parties** page in Partner Center
- Locate your newly created relying party in the list
- Verify that it shows the correct Audience URI, encryption type (Asymmetric encryption -- JWE RFC 7516), and token lifetime
- Confirm that the certificate thumbprint matches your uploaded certificate
- Check that the **Partner Xbox User ID (ptx)** claim appears in the claims list

### 1.1.4 Defining the service endpoint in your App's NSAL

Now that you've created your Relying Party and defined the X-token claims, you need to configure your app's Network Security Access List (NSAL).
The NSAL tells the GetTokenAndSignature API on the client which X-token to provide when calling your service's URL.
You'll configure the NSAL from the Xbox Live Single sign-on page in Partner Center.

1. Go to your App's overview page in Dev Center and select **Services-\>Xbox Live**  
![Partner Center showing Services menu with Xbox Live option highlighted](./media/image4.jpg)

2. Check the tab towards the top of the page for the Sandbox you will be testing in.

3. Expand the Services list in the panel on the left again and you will now see the option for **Xbox Live single sign-on,** click on that

4. Click **New endpoint**

5. Enter the URL host address\* of the service your app will be calling (e.g., `https://contosogameservice.azurewebsites.net`)

6. Select the **Relying Party** from the drop-down that you previously configured above

7. Click **Save**

8. Go back to the **Xbox Live Gameplay Settings** page

9. Select the tab of the Sandbox you want to publish your updated NSAL into

10. Click **Publish** in the right corner  
![Partner Center showing Publish button in Xbox Live configuration](./media/image5.PNG)

### 1.2 Building the sample and debugging locally

Before deploying to Azure, it's easier to run and debug the sample locally on your PC.
This section shows you how to build, configure, and debug the sample on your development machine.
Later in this guide, we'll cover Azure deployment and remote debugging.

#### 1.2.1 Building the sample in Visual Studio

1. [Download and install the latest .NET 9.0 SDK](https://dotnet.microsoft.com/download/dotnet/9.0)

   > **Note:** The sample has been updated to .NET 9.0 as of October 2025. If you're using an older version of the sample, check the documentation version for the appropriate .NET SDK version.

2. Open **GameService.sln** in Visual Studio

3. The NuGet packages for Newtonsoft.Json, Jose.JWT, and others will download automatically shortly after the project loads

4. Open the **XstsConstants.cs** file and change the value of **ServiceName** to be something related to your title or service (this value is used later in logging and calling other services to identify your server).

5. Compile the solution and verify it succeeds.

#### 1.2.2 Enabling Application Settings and Certs when debugging locally

When you run the service locally, you need to configure app settings using the .NET secret-manager tool.
You can install this either through the [Azure Command Line-Interface (CLI)](https://docs.microsoft.com/en-us/cli/azure/install-azure-cli?view=azure-cli-latest) or use Visual Studio's built-in support as shown below.
For more information, see [Safe storage of app secrets in development in ASP.NET Core](https://docs.microsoft.com/en-us/aspnet/core/security/app-secrets?tabs=windows&view=aspnetcore-2.2#SecretManager).

1. Right click on the **GameService** project and then select **Manage User Secrets**

2. In the secrets.json file add the following example to simulate the App Settings your service would read from Azure. Replace the placeholder values with your actual certificate thumbprints.

```json
{
  "RP_CERT_THUMBPRINT": "[thumbprint of your Relying Party Cert]",
  "BP_CERT_THUMBPRINT": "[thumbprint of your Business Partner Cert]"
}
```

   > **Example with actual thumbprint format:**
   >
   > ```json
   > {
   >   "RP_CERT_THUMBPRINT": "A1B2C3D4E5F6G7H8I9J0K1L2M3N4O5P6Q7R8S9T0",
   >   "BP_CERT_THUMBPRINT": "Z9Y8X7W6V5U4T3S2R1Q0P9O8N7M6L5K4J3I2H1G0"
   > }
   > ```

3. For Certificates, import the certs including private certs to your Machine / Current User cert store as that is where Azure will place the certs once configured above.

   > **Security Note:** Never commit the secrets.json file or certificate files to source control. The secrets.json file is automatically excluded by default in .NET projects.

#### 1.2.3 Running and debugging the sample locally

1. Follow the steps above and ensure that the sample builds properly

2. Open **GameService.sln** in Visual Studio

3. Right click on the GameService project and select **Properties**

4. Go to the **Debug** tab

5. Check **Enable SSL** and record the https:// address shown (you will need this when using Fiddler to replay calls to the service for debugging later)

6. Check **Enable Anonymous Authentication**

7. Press **F5** to compile and run the sample locally with the debugger attached

Your service is now running on your local machine and ready for debugging.
Next, let's configure Fiddler to help debug HTTP requests.

#### 1.2.4 Using Fiddler to debug request calls locally

Fiddler is a powerful tool for debugging HTTP traffic between your client and service.
Here's how to set it up:

1. Install an HTTP development tool such as [Fiddler](https://www.telerik.com/fiddler) (Postman can also be used)

2. Make sure you have configured your title's NSAL in Partner Center as outlined previously

3. Enable Fiddler monitoring on the console as outlined in [How to use Fiddler with Xbox One](https://developer.microsoft.com/en-us/games/xbox/docs/xdk/fiddler-setup-networking)

4. Run the SimpleWinHttp sample and make a call to your Azure Service as outlined in [Using the SimpleWinHttp sample to verify your server and NSAL configuration](#14-using-the-simplewinhttp-sample-to-verify-your-service-and-nsal-configuration) below

5. Look for and copy the raw request format for the call to GetClaims (Example below)  
![Fiddler showing raw HTTP request with Authorization header](./media/image6.png)

6. Compile and run the server locally

7. Once the server is running and ready to receive requests go to Fiddler's **Composer** tab and select the **Scratchpad** sub-tab.

8. Paste the raw request text you copied from the window in step 5 into the scratchpad and update the host name to be localhost and the port that your debug server is running on that you got from step 5 of [Running and debugging the sample locally](#123-running-and-debugging-the-sample-locally). Example below:

```http
GET https://localhost:44366/api/getclaims HTTP/1.1
User-Agent: LocalDev
Authorization: XBL3.0 x=[User Hash Goes Here];[Token Goes Here]
Content-Type: application/x-www-form-urlencoded; charset=utf-8
Host: localhost:44366
```

9. Highlight the text in the Composer window and click the **Execute** button to issue the request to your local debug service.

> **NOTE:** Although not needed for Section 1, you will need to update Fiddler's HTTPS settings to skip decryption for **service.auth.xboxlive.com.**
> You need to do this otherwise you will not be able to get a Service token when debugging locally.
> See [Section 2 -- Service Auth X-tokens](#2-service-auth-x-tokens) for more instructions.

### 1.3 Deploying to an Azure App Service and debugging remotely

Now that you've tested the sample locally, you're ready to deploy it to Azure.
Azure provides a quick and easy way to create a fully functioning cloud endpoint that your game can call from the console.
Let's walk through the deployment process.
If you are using the sample as a starting foundation for creating your own service, you can use the Azure Web Service to expand your capacity and deployments later to meet your needs.

You will need to have the Azure SDK installed for Visual Studio to be able to deploy and create a service from Visual Studio. [You can download the needed SDK here.](https://azure.microsoft.com/en-us/downloads/)

1. Right click on the GameService project and select **Publish**

2. Select **New Profile**

3. Select **App Service** and the **Create New** option

4. Click the **Advanced...** link

5. Expand the **File Publish Options** and check **Remove additional files at destination**

6. Click **Save**

7. Click **Publish** and the **Create App Service** wizard will open

8. Enter a name for your app. Note: this name will be part of the URI used to call your service. Example: "ContosoGameService" would have an address of `https://ContosoGameService.azurewebsites.net`

9. It is recommended to define a new Resource Group for your App Service with a descriptive name ending in "\_rg" (e.g., "ContosoGameService_rg")

10. Create a new Hosting Plan or use an existing one

11. Click **Create**

12. Once back on the Publish window

13. Select the **Actions** dropdown and select **Rename**

14. Rename the profile and replace *Web Deploy* with **Release**

Azure now creates all the necessary resources and deploys your service.
When deployment completes, Visual Studio opens a browser to your service endpoint.
You'll see "Access Denied: No auth header" - this is expected!
It confirms your service is running, but you haven't yet added the Relying Party certificate to the deployment.

#### 1.3.1 Adding certificates with private keys to your deployed service

Your deployed service needs access to the Relying Party's private certificate to decrypt and validate X-tokens.
This section shows you how to upload and configure certificates in Azure.
For more details, see [Use an SSL certificate in your application code in Azure App Service](https://docs.microsoft.com/en-us/azure/app-service/app-service-web-ssl-cert-load).

To upload a private certificate and deploy it to your web service:

1. Log into the Azure portal and go to your App Service's page

2. Select **Certificates** under **Settings**

3. Select **Add certificates**

4. Select **Upload certificate(.pfx)** from the drop-down menu

5. Select your .pfx certificate file on your machine and enter the password

6. Click **Add**

The certificate now appears on this page as healthy with its thumbprint value.
However, uploading alone isn't enough - you also need to configure Azure to load the certificate at runtime.
Without setting the certificate's thumbprint in the WEBSITE_LOAD_CERTIFICATES setting, your app won't be able to access the certificate.

Continue with these steps:

1. Copy the thumbprint of the cert

2. Go to the service's **Configuration** page under **Settings**

3. Add a new setting called **WEBSITE_LOAD_CERTIFICATES**

4. Add the thumbprint of the cert as the value of this setting. If the setting already exists, add a semicolon to the end of the existing string and then add your new cert's thumbprint.

**How to verify:**

- In the Azure Portal, navigate to your App Service
- Select **Certificates** under **Settings**
- Verify your uploaded certificate appears in the list with status "Healthy"
- Note the thumbprint value matches what you uploaded
- Select **Configuration** under **Settings**
- Find the **WEBSITE_LOAD_CERTIFICATES** setting in Application settings
- Confirm it contains your certificate's thumbprint

#### 1.3.2 Configuring the App Settings in Azure

Now that your certificate is uploaded, you need to tell your service which certificate to use.
You can configure Azure App Settings either through the Azure portal (steps below) or using Visual Studio's **Edit App Service Settings...** option in the Publish window (see [Deploying to an Azure App Service and debugging remotely](#13-deploying-to-an-azure-app-service-and-debugging-remotely)).

1. Log into the Azure portal and go to your App Service's page

2. Select **Application settings**

3. Scroll down to the **Application settings** section

4. Click **Add new setting**

5. Name the app setting **RP_CERT_THUMBPRINT**

6. Set the value to the thumbprint of your Relying Party certificate you uploaded.

7. Click **Save**

This setting tells your service which certificate to load and cache at startup for decrypting X-tokens.

#### 1.3.3 Creating a debug deployment to Azure for debugging

If you need to debug your service while it's running in Azure, you'll need to create a debug publish profile.
This allows you to attach the Visual Studio debugger to your running service.

1. Right click on the GameService project and select **Publish**

2. Select **New Profile**

3. Select **App Service** and the **Select Existing** option

4. Click the **Advanced...** link

5. Set Configuration to "Debug"

6. Expand the **File Publish Options** and check **Remove additional
    files at destination**

7. Click **Save**

8. Click **Publish**

9. Expand the folder icon that has the name of your Service's Resource
    Group

10. Select your service from the expanded list (identified by the App Service icon - a globe or web app symbol next to it)

11. Click **OK** and Visual Studio will begin a compile and publish of
    the new profile

12. Select the **Actions** dropdown on the Publish window and select
    **Rename**

13. Rename the profile and replace *Web Deploy* with **Debug**

You may run into this common issue during deployment, follow the link for a fix:

[When re-deploying to Azure I get the following error: "Web deployment task failed. (Web Deploy cannot modify the file 'Microsoft.XboxSecureTokens.dll' on the destination because it is locked by an external process.'](#when-re-deploying-to-azure-i-get-the-following-error-web-deployment-task-failed-web-deploy-cannot-modify-the-file-microsoftxboxsecuretokensdll-on-the-destination-because-it-is-locked-by-an-external-process)

#### 1.3.4 Attaching the Visual Studio Debugger to your Azure App Service

1. Right click on the GameService project and select **Publish**

2. Select the Debug profile you created above from the profile drop-down

3. Click **Publish**

4. Once the publish has completed, open the **Cloud Explorer** window
    (View -\> Cloud Explorer)

5. Expand your subscription list and the **App Services** list

6. **Right Click** on your app service and select **Attach Debugger**

> You should now be real-time debugging the instance running in azure.
> If it is unable to attach the debugger, try sending a request call to
> one of the service endpoints to wake it up and then re-attach.

### 1.4 Using the SimpleWinHttp sample to verify your service and NSAL configuration

This will help verify that your NSAL is set up properly and working end to end.

1. Download the SimpleWinHttp sample from the GDK [Samples](https://developer.xboxlive.com/en-us/platform/development/education/Pages/Samples.aspx) on GDN, and extract it to your development PC that has the GDK installed.

2. Open **\\GDKSamples\\Live\\SimpleWinHttp\\SimpleWinHttp.sln**.

3. Open **SimpleWinHttp.cpp**.

4. Update the definition of **\*c_authWebAddress** to be the URI to your deployed web service. For example, `https://contosogameservice.azurewebsites.net/api/getclaims/`.

5. Open **MicrosoftGame.config.**

6. Update the value of **TitleId** to match the title ID (hex) of your title from **Partner Center -\> Xbox Live -\> Xbox Live Setup**

7. Update the **Identity** to also match the Name, Publisher, and Version of your title's MicrosoftGame.config with the values found in **Partner Center - Game Setup -\>Identity Details**)

8. Make sure your dev kit console is in the correct sandbox.

9. Sign in with one of your developer accounts with access to that sandbox on the console

10. Compile and run the sample solution with **F5**

11. Select to make an **Authorized Request** and verify the URI in the output window is the correct URI to your service and check the returned information.

If you get an error while trying to have the client sample talk to your server's default page, see the [FAQs and troubleshooting](#faqs-and-troubleshooting) section later in this document.

### 1.5 Viewing log output from the service

We will cover server logging in more depth during Section 7, but there is a way to see what your service is doing as it runs locally and in Azure.
This is especially helpful for determining startup errors when running in Azure where you are unable to attach to the process before the failure.

### 1.5.1 Viewing log output when running locally through Visual Studio

1. Compile and run the sample locally as outlined under [Running and debugging the sample locally](#123-running-and-debugging-the-sample-locally)

2. Select the Output window

3. Select **Debug** from the **Show output from**: dropdown menu

4. **Right click** on this window and uncheck the following to reduce output other than the logging from the server:

    a.  **Module Load Messages**

    b.  **Module Unload Messages**

    c.  **Thread Exit Messages** to greatly reduce the output from other processes in the window.

### 1.5.2 Enabling service log output through Azure

1. Open the Azure Portal and go to your App Service's page

2. Scroll down the list of options to Monitoring

3. Select **App Service logs**

4. Set **Application Logging (filesystem)** to **On**

5. Set **Level** to **Warning\***

6. Click **Save**

\*This makes it so that only Warning level logs and higher are displayed. Much of the server's output is of type Information to let you know what it is doing, but this also will include a lot more output from other parts of .Net Core.

### 1.5.3 Viewing service log output real-time when running on Azure

1. Open the Azure Portal and go to your App Service's page

2. Scroll down the list of options to Monitoring

3. Select **Log stream**

### 1.5.4 Viewing service log output real-time from Visual Studio

1. Open the Cloud Explorer window

2. Navigate to your service under App Services

3. Right click your service

4. Select View Streaming Logs

## 2. Service Auth X-tokens

In this section we will introduce you to Service Tokens that are obtained by calling the Xbox Server Authentication Service (XSAS) and using a Business Partner (BP) Certificate as authentication.
The Service token can then be exchanged for a Service Auth X-token that allows you to call Xbox Live Services from your service (backend-to-backend or B2B).
When the sample service starts, it will try to cache the Business Partner certificate specified in the BP_CERT_THUMBPRINT app setting and then get and cache a Service Token from that BP Cert in the in-memory cache.

In the next section we will talk about Delegated Auth X-tokens that allow your service to call other Xbox Live services on-behalf-of the user.

For more information on Service tokens see *Obtaining a Service token* in the article [Title service calls to Xbox services](https://learn.microsoft.com/en-us/gaming/gdk/docs/services/fundamentals/s2s-auth-calls/s2s-calls/live-title-service-calls-xbox-live).

### 2.1 Creating a Business Partner certificate

1. Select the **Settings Icon in the upper right and then Developer Settings**
![Partner Center Settings menu with Developer Settings option](./media/image2.jpg)

2. Next select **Xbox Live-\>Web services**  
![Xbox Live menu showing Web services option](./media/image3.png)

3. Select **New Web Service**

4. Select your publisher from the drop down (there is normally only one, but may be more)

5. Enter a name to identify your web service (e.g., "contoso.com" or "Contoso Game Service" - this helps you identify where the certificates will be used and manage them)

6. Click **Save**

7. On the Web Service page, you should now see your new Web Service listed

8. Click **Generate Certificate** for your new Web Service

9. Click the **Show Options** link to see a PowerShell script to create the needed key. You can run this script on any machine and then export the certificate to other machines.

10. Run **Windows PowerShell** as an Administrator

11. Copy in the script from step 9 and press **Enter**

12. Copy the resulting string CSP blob string from PowerShell into the text box under **Download the CSP Certificate** (remove any word wrap line breaks from the console window first)

13. If you want this certificate to only work with one specific sandbox you can select one from the drop-down list. Leaving the drop-down on *Select sandbox* will result in a BP cert that can be used in any of your sandboxes and RETAIL (recommended)

14. Click **Download**

15. Save the .cer file on the PC that ran the script in step 11, rename it with the name you used in step 5 to distinguish it against the others.

16. Follow the instructions under **Binding the Certificate**

17. Make sure once imported that the certificate has the private key bound to it (indicated by the small key icon on the top left)
![Certificate in Windows Certificate Manager showing key icon indicating private key is present](./media/image7.jpg)

18. Record the first section (8x4x4x4x12 GUID) of the Issued To name on the certificate. We will need this value if you are configuring your service to talk to the Collections service b2b in Section 7

19. Right click on the certificate and select All Tasks-\>Export.

20. Follow the export Wizard and make sure to select **Yes, export the private key**

21. Select to secure the exported cert with a password

22. Finish exporting the certificate through the wizard

To add the certificate to your service deployment, follow the instructions previously outlined in Section 1 [Adding certificates with private keys to your deployed service.](#131-adding-certificates-with-private-keys-to-your-deployed-service)

**How to verify:**

- Open **Windows Certificate Manager** (certmgr.msc)
- Navigate to **Personal** > **Certificates**
- Find the certificate with the name you specified in step 5
- Verify it has a small key icon in the top-left corner, confirming the private key is bound
- Double-click to open the certificate and verify the GUID in the "Issued To" field
- Check the expiration date to ensure the certificate is valid
- If uploaded to Azure, verify it appears in your App Service's Certificates page with "Healthy" status

### 2.2 Adding the cert's thumbprint for initialization to the App Settings

Remember to add this setting to your user-secrets if running or debugging locally as covered in [Enabling Azure App Settings when debugging locally](#122-enabling-application-settings-and-certs-when-debugging-locally)

1. Log into the Azure portal and go to your App Service's page

2. Select **Application settings**

3. Scroll down to the **Application settings** section

4. Click **Add new setting**

5. Name the app setting **BP_CERT_THUMBPRINT**

6. Set the value to the thumbprint of your BP cert (visible on the Private Key Certificate (.pfx) screen where you uploaded it.

7. Click **Save**

### 2.3 Verifying your service can obtain a Service Token

Now that the BP cert is generated and set in our App Settings, we can test that our service is obtaining the BP cert and properly getting Service tokens.
To do this you will need to setup log viewing through Azure (covered in [*Viewing log output from the service running in Azure*](#153-viewing-service-log-output-real-time-when-running-on-azure) of Section 1) or by running the service through the debugger on your development PC.

If you are using the Azure log stream, you will want to make sure that the log filter is set to *Information* so that you can see the service tell you when it gets the BP cert and is able to get a Service token.
For running locally, all output is displayed to the Output -\> Show output from: ASP.NET Core Web Server.

Look for the following in the logs to indicate that the Business Partner cert was properly retrieved, and the service obtained a Service token:

```shell
[Information] GameService.Startup:
{"cV":"Y96h0OLnuEua5lfp.2","info":"Initializing BP Cert..."}

[Information] GameService.Startup:
{"cV":"Y96h0OLnuEua5lfp.1.3","info":"BP Cert cached with thumbprint 274C3E9F63D75A3677C7B403FE2F8B45D72B5CAA"}

[Information] GameService.Startup:
{"cV":"Y96h0OLnuEua5lfp.2\","info":"Initializing Service Token Cache..."}

[Information] GameService.Startup:
{"cV":"Y96h0OLnuEua5lfp.3.1.1","info":"Using BP Cert: DC=BUSINESSPARTNERCERT, O=4dea8aa0-72cc-4cab-888c-cfeeea8d2ebe, OU=xcert.xboxlive.com, CN=4d6777a0-72cc-4cab-888c-cfe23a8d978e_GameServiceBP_a9f5e437-d8a2-4646-a914-098272947f5a_BP"}

[Information] GameService.Startup:
{"cV":"Y96h0OLnuEua5lfp.3.2","info":"GameService - Service Token cached eyJhbGciOiJSU0EtT0FF..."}

[Information] GameService.Startup:
{"cV":"Y96h0OLnuEua5lfp.3","info":"Configured and ready for requests..."}
```

## 3. Delegated Auth X-tokens

The code within this section of the sample will show you how to obtain a Delegated Auth X-token that will enable your service to call Xbox Live services on-behalf-of the user.
This is valuable for getting friends lists, checking the user's purchased items directly, and managing consumable products.
We will cover calling those services in the next section.

A Delegated Auth X-token is obtained by calling the XSTS service and providing both the Service Token and the Delegation Token that is one of the claims in a user's client token.
These two tokens notify the XSTS service which partner is asking for the token and which user we are asking to make calls on-behalf-of.
We also need to provide the Relying Party that maps to the endpoint we want to call with the token.
To know which Relying Party to ask for, we get the Xbox Live Endpoints list.
This provides us a mapping of URI hosts to which Relying Party to ask for to call that specific host or endpoint.

For more information on Delegated Auth X-tokens see *Obtaining an XSTS token for S2S calls* in the article [Title service calls to Xbox services](https://learn.microsoft.com/en-us/gaming/gdk/docs/services/fundamentals/s2s-auth-calls/s2s-calls/live-title-service-calls-xbox-live)

The Endpoints are generally static and won't change, so the sample gets the endpoints data on startup and then stores them in an in-memory database cache.
Delegated Auth X-tokens on the other hand are re-usable until their lifetime expires.
Rather than make an external call to XSTS each time we want to call a service backend-to-backend (B2B) we will want to cache the tokens.
Ideally these tokens would be stored on a distributed cache so that multiple instances of our service can use cached tokens from others.
This will be more permanent for our service and produce less traffic to the XSTS service.

No additional configuration or setup is required for this section.

## 4. Calling Xbox Live Services

Now that your service can retrieve and cache delegated auth X-tokens, your service is able to call the Xbox Live services directly without having go through the client console. The Sample has provided the b2bFriends endpoint as an example that calls the Xbox Live People service to retrieve a player's Friends list.
You can use this as a template and make modifications to build out other endpoints or functions that would call into the various Xbox Live Service.

## 5. Collections and Commerce

The Collections service provides the information about what the user owns or has an entitlement to.
This is useful for validating purchased content and managing consumable products directly from your service instead of relying on the information coming from the client.

Although the Collections service can be called through Delegated Auth X-tokens, it is recommended that partners use the Entra ID auth flow with UserStoreIds.
This is also because not all of the Microsoft Store services support X-token auth.
See [Authenticating your service with the Microsoft Store APIs](https://learn.microsoft.com/en-us/gaming/gdk/docs/store/commerce/service-to-service/xstore-authenticating-your-service) for a comparison between the two authentication methods.  

A comprehensive sample of calling the different Microsoft Store services (Collections, Subscriptions, Clawback, etc.) see the [Microsoft Store Services Sample](https://github.com/microsoft/Microsoft-Store-Services-Sample) and the [Microsoft Store Services library](https://github.com/microsoft/Microsoft-Store-Services) on Github.

### 5.1 Configuring your products for B2B Collections in Partner Center

If you are going to use Delegated Auth X-tokens to call Collections, you will need to configure your products in Partner Center as outlined in [Additional configuration required to view and manage products with delegated authentication XSTS tokens](https://learn.microsoft.com/en-us/gaming/gdk/docs/store/commerce/service-to-service/xstore-authenticating-your-service#additional-configuration-required-to-view-and-manage-products-with-delegated-authentication-xsts-tokens)

## 6. Logging & Correlation Vectors

As you may have noticed and was mentioned earlier, the sample service has built-in logging following the guidance of [Logging in ASP.NET Core](https://docs.microsoft.com/en-us/aspnet/core/fundamentals/logging/?view=aspnetcore-2.2).
(See [Viewing log output from the service](#15-viewing-log-output-from-the-service) for instructions of seeing the logs from your deployed or locally running service).
The log messages are formatted to be JSON compatible to be more easily searchable by log retrieval tools.
The sample also utilizes Correlation Vectors (cV) to help keep track of all the logs related to a specific request or flow of a request through the server (more info below).

Following the guidance found in the article [High-performance logging with LoggerMessage in ASP.NET Core](https://docs.microsoft.com/en-us/aspnet/core/fundamentals/logging/loggermessage?view=aspnetcore-2.2), the sample is structured so that all of the log message formatting is handled in a centralized location (XstsLoggerExtensions.cs).
To fire off a pre-formatted log we simply call the corresponding log API and pass in the needed data.
This helps so that when we change the formatting of one log message it is changed for all times the service would log that info.
Although the performance gain of using LoggerMessage is small, when doing something as frequent as logging in a cloud environment it adds up to a large amount of saved compute time and cost.
Adding additional logging and messages can be done using the existing ones as a template.
Note however, that there is a maximum limit of how many variables you can pass to a LoggerMessage before you will get errors.

Correlation Vectors are an open source protocol for tracing a correlation of events through a distributed system based on a lightweight vector clock. By adding this to all your logging messages it is easier to retrieve and search for the logs related to one of the requests made to your service and Microsoft services.
The Xbox services used in this sample accept a cV header in the request (ms-cv) and will use an extended version of that cV in all logging related to that request.
All responses from these services will also include an MS-CV value in the return headers that is the ending cV for that process on the server.
This is especially helpful when debugging or investigating issues that require looking at logs in both yours and Xbox's logging.
If you have an issue, the cV value from the logs on your side will directly correspond to the cV values of the logs needed on the Xbox service side.

For more information on [Correlation Vectors see the Github repository here.](https://github.com/Microsoft/CorrelationVector)

## Appendix A: Setting up an Azure SQL Database

The following information can help you get started on setting up a persistent database to store values and data that you want to retain beyond server instances such as TransactionIDs for consumable validation.
The sample has an example in-memory database that is turned on by default but can be migrated to a persistent service based database to live beyond the instance of the running service.

Details about database configuration and accessing the connection string at runtime through the Azure application settings see [Azure .NET Core Application Settings](https://blogs.msdn.microsoft.com/jpsanders/2017/05/16/azure-net-core-application-settings/).

### A.1 Creating an Azure SQL Database

Follow the instructions within the following article: [Quickstart: Create an Azure SQL database in the Azure portal](https://docs.microsoft.com/en-us/azure/sql-database/sql-database-get-started-portal)

You can also create the database under the same Resource Group as your App Service.
For running the sample and testing a Basic pricing tier SQL database should be enough.
If you are building your own service and plan roll it out as a production service, you will want to adjust the database capabilities and settings in Azure.

### A.2 Initializing the database with Migrations and debugging locally

Now that the database is created, we need to initialize it to have the tables and data that our server will store in the database.
This is done by running the app locally and creating migration files that we can then use to initialize the tables in the database.
As a part of this we need to add the connection string to our user-secrets file (see [Enabling Azure App Settings when debugging locally](#122-enabling-application-settings-and-certs-when-debugging-locally)).
This will also allow us to debug with the database on our development PC.
For more information on migrations and what to do, please see the following:

[Getting Started with EF Core on ASP.NET Core with a New database](https://docs.microsoft.com/en-us/ef/core/get-started/aspnetcore/new-db?view=aspnetcore-2.1&tabs=visual-studio)

1. Open the Sample solution in Visual Studio

2. Go to **Tools \> NuGet Package Manager \> Package Manager Console**

3. Navigate in the Package Manager Console to the local directory where GameService.csproj is found

4. In the Package Manager Console's Default Project dropdown menu select **Microsoft.XboxSecureTokens** (the project the db context we are trying to create lives)

5. Run the following commands to create the needed table structures for the GameServicePersistentDBContext used in the sample.

```powershell
Add-Migration InitialCreate -context "GameServicePersistentDBContext"
```

This will start your sample running locally on your PC to build the migration.

6. After the migration has been completed, run the next command in the Package Manager Console to push the migration to your Azure SQL database.

```powershell
Update-Database -context "GameServicePersistentDBContext"
```

If you get a connection error stating that your client IP is not allowed to access the database, log into the Azure Portal and navigate to the database.
Select **Set Firewall**, add your dev PC's IP to the allow list, save and try again to complete the database update.

7. Re-deploy your web service from Visual Studio

## FAQs and troubleshooting

### When re-deploying to Azure I get the following error: "Web deployment task failed. (Web Deploy cannot modify the file 'Microsoft.XboxSecureTokens.dll' on the destination because it is locked by an external process.'

This usually happens when you are going between a Retail and a Debug deployment as the XboxSecureTokens .dll will be updated.
To get around this error you will need to stop your service by following the instructions below and then re-deploy.

1. open the **Cloud Explorer** window

2. Expand your subscription list and the **App Services** list

3. **Right Click** on your app service and service and select **Stop**

4. Wait a moment and then try the re-publish again

5. In the **Cloud Explorer** right click on **App Services** and select **Refresh**

6. Now right click on your app service and select **Start**

### When the client-side sample tries to call our service, it is getting errors from GetTokenAndSignatureAsync(). How can I resolve these errors?

Most of the NSAL or token-related errors from the console start with 0x87DD\*.
[Error Codes 0x87DD.. when calling GetTokenAndSignatureAsync or Xbox.Services APIs](https://forums.xboxlive.com/questions/2936/error-codes-0x87dd-when-calling-gettokenandsignatu.html), a post in the Xbox Developer Forums, provides a list of the most common errors and how to resolve them.
(Note: if when clicking this link, you go to a general forum page, try pasting the link into a browser address bar to get directly to the page.)

#### HRESULT error 0x800c0019 when trying to call my web service over HTTPS from the console

This error means that the SSL certificate that you used for HTTPS traffic is not trusted by the console, or the certificate has been configured incorrectly on the server.

To correct this, first make sure that the certificate you created and used for SSL on your server uses the full domain name of your server.
To check, double-click the exported .cer file and look at the **Issued to** value (for example, "Issued to: Server.Contoso.com")**.**

Although you can add an SSL cert in Partner Center now on the Endpoint definition page, it is recommended that you always use fully trusted SSL certs.
These can now be obtained free of charge through entities such as <http://letsencrypt.org>.

### I'm getting 403 errors when my console tries to talk to my service

This probably means that the certificates or secrets for your relying party are not configured properly right or you may have entered the wrong certificate or secret key into the Azure Key Vault.
You may want to use Fiddler to check your title's NSAL and debug the service as it attempts to validate the X-token.
To check your title's NSAL do reboot the device to clear the NSAL cache and follow the links below based on the device your title runs on:

[For Xbox Console](https://forums.xboxlive.com/questions/516/is-there-a-way-to-verify-the-nsal-configuration-fr.html)

[For Windows PC](https://forums.xboxlive.com/questions/54883/is-there-a-way-to-verify-the-nsal-configuration-of.html)

#### When trying to obtain a Service token from the XSAS server with my Business Partner certificate I am getting an exception that "The underlying connection was closed\"

This exception is probably due to an expired Business Partner certificate.
Check the expiration date of your certificate to make sure that the date has not passed.
Relying Party certificates will still work with the XSTS system even if they expire, but your Business Partner certificate must be in a good and valid state to be accepted by the Xbox Server Authentication Service (XSAS).
XSAS does not give you a 403 when presented with an invalid or expired certificate; instead, it simply drops the connection, which results in an exception and the messaging about a closed connection during debugging.
To correct this, simply create a new Business Partner certificate.
