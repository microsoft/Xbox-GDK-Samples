<img style="float: left" src="./media/image1.png" /><img style="float: right" src="./media/image3.png" />
<br/><br/><br/><br/><br/>

# FidelityFX Contrast Adaptive Sharpening (CAS) Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2024 - Update 1)*

# Description

This sample shows different methods of utilizing AMD's FidelityFX Contrast Adaptive Sharpening (CAS) to a scene.

![](./media/image4.jpeg)

# Building the sample

If using Windows Desktop, set the active solution platform to `Gaming.Desktop.x64`.

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

| Action                                |  Gamepad                      |
|---------------------------------------|------------------------------|
| Cycle CAS Mode                        |  A button / B button          |
| Cycle renderscale for upscaling modes |  X button / Y button          |
| Rotate view                           |  Left thumbstick              |
| Reset view                            |  Left thumbstick (click)      |
| Increase Sharpness amount             |  Right Trigger                |
| Decrease Sharpness amount             |  Left Trigger                 |
| Exit                                  |  View Button                  |

# Implementation notes

Contrast Adaptive Sharpening (CAS) provides a mixed ability to sharpen
and optionally scale an image. The algorithm adjusts the amount of
sharpening per pixel to target an even level of sharpness across the
image. Areas of the input image that are already sharp are sharpened
less, while areas that lack detail are sharpened more. This allows for
higher overall natural visual sharpness with fewer artifacts.

CAS was designed to help increase the quality of existing Temporal
Anti-Aliasing (TAA) solutions. TAA often introduces a variable amount of
blur due to temporal feedback. The adaptive sharpening provided by CAS
is ideal to restore detail in images produced after TAA .

CAS' optional scaling capability is designed to support Dynamic
Resolution Scaling (DRS). DRS changes render resolution every frame,
which requires scaling prior to compositing the fixed-resolution User
Interface (UI). CAS supports both up-sampling and down-sampling in the
same single pass that applies sharpening.

Further implementation detail on the sharpening and upscaling algorithms
can be found at <https://gpuopen.com/fidelityfx-cas/> with a deep-dive
presentation at
<https://gpuopen.com/wp-content/uploads/2019/07/FidelityFX-CAS.pptx>

## Integration guidelines

### Shading language and API requirements

#### DirectX 12 + HLSL

- `HLSL`
  - `CS_6_2`

### Walkthrough

Include the [`ffx_cas.h`](./../../../Kits/AMDTK/fidelityfx/include/FidelityFX/host/ffx_cas.h) header:

```C++
#include <FidelityFX/host/ffx_cas.h>
```

Query the amount of scratch memory required for the FFX Backend using `ffxGetScratchMemorySizeDX12`:

```C++
const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(FFX_CAS_CONTEXT_COUNT);
```

Allocate the scratch memory for the backend and retrieve the interface using `ffxGetInterfaceDX12`:

```C++
void* scratchBuffer = calloc(scratchBufferSize, 1);
FfxErrorCode errorCode = ffxGetInterfaceDX12(&m_InitializationParameters.backendInterface, m_deviceResources->GetD3DDevice(), 
scratchBuffer, scratchBufferSize, FFX_CAS_CONTEXT_COUNT);
FFX_ASSERT_MESSAGE(errorCode == FFX_OK, "Could not initialize the FidelityFX SDK backend");
FFX_ASSERT_MESSAGE(m_InitializationParameters.backendInterface.fpGetSDKVersion(&m_InitializationParameters.backendInterface) ==
 FFX_SDK_MAKE_VERSION(1, 1, 2), "FidelityFX CAS sample requires linking with a 1.1.2 version SDK backend");
```

Create the `FfxCasContext` by filling out the `FfxCasContextDescription` structure with the required arguments:

```C++
FfxCasContext            m_CasContext;
FfxCasContextDescription m_InitializationParameters = {};

if (GetUpscaleMode() == UpscaleMode::NativeRender) {
    m_InitializationParameters.flags |= FFX_CAS_SHARPEN_ONLY;
}
m_InitializationParameters.colorSpaceConversion = FFX_CAS_COLOR_SPACE_LINEAR;

RECT size = m_deviceResources->GetOutputSize();

float outWidth = static_cast<float>(size.right);
float outHeight = static_cast<float>(size.bottom);

float inWidth = static_cast<float>(size.right * m_renderScales[GetRenderScale()]);
float inHeight = static_cast<float>(size.bottom * m_renderScales[GetRenderScale()]);

m_InitializationParameters.maxRenderSize.width = static_cast<uint32_t>(inWidth);
m_InitializationParameters.maxRenderSize.height = static_cast<uint32_t>(inHeight);
m_InitializationParameters.displaySize.width = static_cast<uint32_t>(outWidth);
m_InitializationParameters.displaySize.height = static_cast<uint32_t>(outHeight);

// Create the cas context
ffxCasContextCreate(&m_CasContext, &m_InitializationParameters);
```

When the time comes for Cas rendering, fill out the `FfxCasDispatchDescription` structure and call `ffxCasContextDispatch` using it:

```C++
RECT size = m_deviceResources->GetOutputSize();
uint32_t inWidth = static_cast<uint32_t>(size.right * m_renderScales[GetRenderScale()]);
uint32_t inHeight = static_cast<uint32_t>(size.bottom * m_renderScales[GetRenderScale()]);

FfxCasDispatchDescription dispatchParameters = {};
dispatchParameters.commandList = ffxGetCommandListDX12(commandList);
dispatchParameters.renderSize = { inWidth, inHeight };
dispatchParameters.sharpness = m_sharpness;

// All cauldron resources come into a render module in a generic read state (ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource)
dispatchParameters.color = ffxGetResourceDX12(m_scene->GetResource(), GetFfxResourceDescription(m_scene->GetResource()), nullptr, FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
dispatchParameters.output = ffxGetResourceDX12(m_casOutput.Get(), GetFfxResourceDescription(m_casOutput.Get()), nullptr, FFX_RESOURCE_STATE_UNORDERED_ACCESS);

FfxErrorCode errorCode = ffxCasContextDispatch(&m_CasContext, &dispatchParameters);
FFX_ASSERT(errorCode == FFX_OK);
```

During shutdown, destroy the CAS context:

```C++
ffxCasContextDestroy(&m_CasContext);
```

# Update history

Updated with FidelityFX SDK 1.1.1 integration guidelines September 2024.

This sample was written in October 2020.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Disclaimer

The information contained herein is for informational purposes only, and
is subject to change without notice. While every precaution has been
taken in the preparation of this document, it may contain technical
inaccuracies, omissions and typographical errors, and AMD is under no
obligation to update or otherwise correct this information. Advanced
Micro Devices, Inc. makes no representations or warranties with respect
to the accuracy or completeness of the contents of this document, and
assumes no liability of any kind, including the implied warranties of
noninfringement, merchantability or fitness for particular purposes,
with respect to the operation or use of AMD hardware, software or other
products described herein. No license, including implied or arising by
estoppel, to any intellectual property rights is granted by this
document. Terms and limitations applicable to the purchase or use of
AMD's products are as set forth in a signed agreement between the
parties or in AMD\'s Standard Terms and Conditions of Sale.

AMD, the AMD Arrow logo, Radeon, RDNA, Ryzen, and combinations thereof
are trademarks of Advanced Micro Devices, Inc. Other product names used
in this publication are for identification purposes only and may be
trademarks of their respective companies.

Windows is a registered trademark of Microsoft Corporation in the US
and/or other countries.

Xbox is a registered trademark of Microsoft Corporation in the US and/or
Other countries.

© 2020 Advanced Micro Devices, Inc. All rights reserved.
