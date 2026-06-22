# CustomHttp Sample

*This sample is compatible with the Microsoft Game Development Kit (April 2026)*

# Description

This sample demonstrates how to correctly integrate **libcurl** into an Xbox title by
following the GDK networking lifecycle. It covers waiting for network initialization
before initializing libcurl, cleanly tearing down in-flight requests on suspend, and
re-initializing after resume. It also shows how to resolve the Xbox debug proxy address
and detect debug certificates in the system certificate store.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Project Scarlett, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

This sample requires **vcpkg** to provide libcurl. The project includes a `vcpkg.json`
manifest that pulls in `curl` with `default-features: false` (HTTP/HTTPS only).
vcpkg must be integrated with your Visual Studio installation using the `x64-windows`
triplet.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

Press **Make HTTP Request** (A button) to queue an HTTPS GET request to `example.com`.
The response code and body size are printed to the console window. The sample also
resolves and logs the Xbox proxy address on startup, and logs any Fiddler or Xbox
Multiplayer debug certificates found in the system root certificate store.

## Controls

| Action | Button |
|--------|--------|
| Make HTTP Request | **A** |
| Exit | **View** |

# Implementation notes

The core networking logic is in `CustomHttp.cpp`:

- **`WaitForNetworkInitialization()`** — polls `XNetworkingGetConnectivityHint` until
  `networkInitialized` is true before calling `curl_global_init`.
- **`InitializeCurl()` / `CleanupCurl()`** — libcurl global and multi-handle lifecycle
  tied to networking state. `CleanupCurl` removes and frees all in-flight requests before
  destroying the multi handle, making it safe to call on suspend.
- **`ResolveAndCacheProxy()`** — uses `WinHttpGetProxySettingsEx` with
  `WinHttpProxySettingsTypeXBox` to resolve the Xbox debug proxy address.
  `WinHttpCreateProxyResolver` is forward-declared in `SampleWinHttpProxy.h` since it
  may not be declared in older Windows SDK versions of winhttp.h for the GDK but is 
  present in winhttp.lib. The result is cached in `m_proxyAddress` and applied to every 
  request.
- **`LogDebugCertificates()`** — enumerates `CurrentUser\Root` and logs any certificate
  whose subject contains "Fiddler" or "Xbox Multiplayer".
- **`PumpCurlMulti()`** — called each frame from `Update()` to drive
  `curl_multi_perform` and `curl_multi_info_read`, logging results as requests complete.
- **`OnSuspending()` / `OnResuming()`** — calls `CleanupCurl` on suspend and
  re-waits for network initialization before calling `InitializeCurl` on resume.

# Update history

- April 2026 - Initial release

# Privacy Statement

When compiling and running a sample, the file name of the sample executable will be sent
to Microsoft to help track sample usage. To opt-out of this data collection, you can
remove the block of code in Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see the
[Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).
