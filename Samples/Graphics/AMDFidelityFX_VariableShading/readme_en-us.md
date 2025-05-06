<div style="float: center"><img style="float: left" src="./media/image1.png" /><img style="float: right" src="./media/image2.png" />
<br/><br/><br/><br/><br/></div>

# FidelityFX Variable Shading Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2024 - Update 1)*

# Description

This sample shows how to apply the FidelityFX Variable Shading algorithm to a scene.

![](./media/image3.jpg)

![](./media/image4.png)

# Building the sample

If using Windows Desktop, set the active solution platform to `Gaming.Desktop.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

This sample does not support Xbox One.

*For more information, see* __Running samples__*, in the GDK documentation.*

# Using the sample

| Action                                |  Gamepad                      |
|---------------------------------------|------------------------------|
| Cycle Shading Rate Image Generation Mode |  A button |
| Toggle Shading Rate Image Overlay     |  X button                     |
| Rotate view                           |  Left thumbstick              |
| Reset view                            |  Left thumbstick (click)      |
| Increase/Decrease Variance Cutoff     |  D-Pad Up/Down                |
| Exit                                  |  View Button                  |

# Implementation notes

FidelityFX Variable Shading technique generates a shading rate image
based on the luminance of the scene for use in subsequent frames. The
technique aims to reduce the frequency of costly pixel shader invocation
across the surface of primitives which contain similar color outputs to
reduce bandwidth and computational requirements at high resolutions.

When Variable Rate Shading is enabled, while the shading rate image
produced by the technique is in use, and shading rate combiners are set
appropriately, rendered primitives will utilize shading rates defined by
the shading rate image for the tiles the primitives cover.

Further implementation detail of this algorithm can be found at
<https://gpuopen.com/fidelityfx-variable-shading/> with a deep-dive
presentation at
<https://github.com/GPUOpen-Effects/FidelityFX-VariableShading/blob/master/docs/FFX-VariableShading-Introduction.pdf>
.

## Integration guidelines

### Shading language and API requirements

#### DirectX 12 + HLSL

- `HLSL`
  - `CS_6_2`

### Walkthrough

Include the [`ffx_vrs.h`](./../../../Kits/AMDTK/fidelityfx/include/FidelityFX/host/ffx_vrs.h) header:

```C++
#include <FidelityFX/host/ffx_vrs.h>
```

Query the amount of scratch memory required for the FFX Backend using `ffxGetScratchMemorySizeDX12`:

```C++
const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(FFX_VRS_CONTEXT_COUNT);
```

Allocate the scratch memory for the backend and retrieve the interface using `ffxGetInterfaceDX12`:

```C++
void* scratchBuffer = calloc(scratchBufferSize, 1u);
FfxErrorCode errorCode = ffxGetInterfaceDX12(&m_vrsInitParams.backendInterface, m_deviceResources->GetD3DDevice(), scratchBuffer, scratchBufferSize, FFX_VRS_CONTEXT_COUNT);
FFX_ASSERT(errorCode == FFX_OK);
FFX_ASSERT_MESSAGE(m_vrsInitParams.backendInterface.fpGetSDKVersion(&m_vrsInitParams.backendInterface) == FFX_SDK_MAKE_VERSION(1, 1, 2),
 "FidelityFX VRS sample requires linking with a 1.1.2 version SDK backend");
FFX_ASSERT_MESSAGE(ffxVrsGetEffectVersion() == FFX_SDK_MAKE_VERSION(1, 2, 0),
 "FidelityFX VRS sample requires linking with a 1.2 version FidelityFX VRS library");
```

Create the `FfxVrsContext` by filling out the `FfxVrsContextDescription` structure with the required arguments:

```C++
FfxVrsContext            m_vrsContext;
FfxVrsContextDescription m_vrsInitParams = {};

static unsigned int sImageTileSize            = DeviceDefaultShadingRateImageTileSize();
m_vrsInitParams.flags                       = 0;
m_vrsInitParams.shadingRateImageTileSize    = sImageTileSize;
ffxVrsContextCreate(&m_vrsContext, &m_vrsInitParams);
```

When the time comes for VRS image rate texture rendering, fill out the `FfxVrsDispatchDescription` structure and call `ffxVrsContextDispatch` using it:

```C++
auto const size = m_deviceResources->GetOutputSize();

FfxVrsDispatchDescription dispatchParameters = {};
dispatchParameters.commandList      = ffxGetCommandListDX12(commandList);
dispatchParameters.output           = ffxGetResourceDX12(m_ShadingRateImage.Get(), GetFfxResourceDescription(m_ShadingRateImage.Get()), L"VRSImage", FFX_RESOURCE_STATE_UNORDERED_ACCESS);
dispatchParameters.historyColor     = ffxGetResourceDX12(m_scene->GetResource(), GetFfxResourceDescription(m_scene->GetResource()), L"HistoryColorBuffer", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
dispatchParameters.motionVectors    = ffxGetResourceDX12(m_motionVectors.Get(), GetFfxResourceDescription(m_motionVectors.Get()), L"MotionVectors", FFX_RESOURCE_STATE_RENDER_TARGET);
dispatchParameters.motionFactor     = m_vrsMotionFactor;
dispatchParameters.varianceCutoff   = m_vrsThreshold;
static unsigned int sImageTileSize    = DeviceDefaultShadingRateImageTileSize();
dispatchParameters.tileSize         = sImageTileSize;
dispatchParameters.renderSize       = { uint32_t(size.right), uint32_t(size.bottom) };
dispatchParameters.motionVectorScale.x = -1.f;
dispatchParameters.motionVectorScale.y = -1.f;

// Disabled until remaining things are fixes
FfxErrorCode errorCode = ffxVrsContextDispatch(&m_vrsContext, &dispatchParameters);
FFX_ASSERT(errorCode == FFX_OK);
```

During shutdown, destroy the VRS context:

```C++
ffxVrsContextDestroy(&m_vrsContext);
```

# Update history

Updated with FidelityFX SDK 1.1.1 integration guidelines September 2024.

This sample was written in January 2021.

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

© 2021 Advanced Micro Devices, Inc. All rights reserved.
