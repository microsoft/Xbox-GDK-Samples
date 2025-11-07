  ![](./media/image1.png)![](./media/image2.png)

#   FidelityFX Ambient Occlusion (CACAO)

*This sample is compatible with the Microsoft Game Development Kit
(March 2024 - Update 1)*

# Description

Combined Adaptive Compute Ambient Occlusion (CACAO) is a highly
optimized adaptive sampling ambient occlusion implementation. There are
various quality presets included in this sample, which can be tuned to
the needs of the environment.

More information is available on GPUOpen at
<https://gpuopen.com/fidelityfx-cacao/>

![](./media/image3.jpeg)

# Building the sample

If using Windows Desktop, set the active solution platform to `x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If using Xbox One, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

When running Windows Desktop, ensure the latest graphics drivers are installed.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

| Action                                |  Gamepad                      |
|---------------------------------------|------------------------------|
| Rotate view                           |  Left thumbstick              |
| Exit                                  |  View Button                  |
| CACAO On/Off                          |  X Button                     |
| CACAO Quality Preset                  |  A / B Buttons                |
| View CACAO Buffer                     |  Y Button                     |
| AO Buffer Composite Blend Factor      |  Right / Left Triggers        |

# Implementation notes

CACAO is a highly optimized adaptation of the Intel ASSAO (Adaptive
Screen Space Ambient Occlusion) implementation. It provides 5 quality
levels - LOWEST, LOW, MEDIUM, HIGH, ADAPTIVE.

![](./media/image4.png)
![](./media/image5.png)

## Integration guidelines

### Shading language and API requirements

#### DirectX 12 + HLSL

- `HLSL`
  - `CS_6_2`

### Walkthrough

Include the [`ffx_cacao.h`](./../../../Kits/AMDTK/fidelityfx/include/FidelityFX/host/ffx_cacao.h) header:

```C++
#include <FidelityFX/host/ffx_cacao.h>
```

Query the amount of scratch memory required for the FFX Backend using `ffxGetScratchMemorySizeDX12`:

```C++
const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(FFX_CACAO_CONTEXT_COUNT);
```

Allocate the scratch memory for the backend and retrieve the interface using `ffxGetInterfaceDX12`:

```C++
void* scratchBuffer = calloc(scratchBufferSize, 1u);
  FfxErrorCode errorCode = ffxGetInterfaceDX12(&m_ffxInterface, m_deviceResources->GetD3DDevice(), scratchBuffer,
      scratchBufferSize, FFX_CACAO_CONTEXT_COUNT * 2);
  FFX_ASSERT_MESSAGE(errorCode == FFX_OK, "Could not initialize FidelityFX SDK backend context.");
  FFX_ASSERT_MESSAGE(m_ffxInterface.fpGetSDKVersion(&m_ffxInterface) == FFX_SDK_MAKE_VERSION(1, 1, 2),
      "FidelityFX CACAO sample requires linking with a 1.1.2 version SDK backend");
```

Create the `FfxCacaoContext` by filling out the `FfxCacaoContextDescription` structure with the required arguments:

```C++
FFX_ASSERT_MESSAGE(ffxCacaoGetEffectVersion() == FFX_SDK_MAKE_VERSION(1, 4, 0),
      "FidelityFX Cacao sample requires linking with a 1.4 version FidelityFX Cacao library");
FfxCacaoContext m_cacaoContext;
FfxCacaoContextDescription description = {};

auto const size = m_deviceResources->GetOutputSize();
description.backendInterface = m_ffxInterface;
description.width = static_cast<uint32_t>(size.right);
description.height = static_cast<uint32_t>(size.bottom);
description.useDownsampledSsao = false;
errorCode = ffxCacaoContextCreate(&m_cacaoContext, &description);
FFX_ASSERT_MESSAGE(errorCode == FFX_OK, "Could not initialize FidelityFX SDK backend context.");
```

When the time comes for cacao to be rendered, fill out the `FfxCacaoDispatchDescription` structure and call `ffxCacaoContextDispatch` using it:

```C++
FfxCacaoDispatchDescription dispatchDescription = {};
dispatchDescription.commandList = ffxGetCommandListDX12(commandList);            
dispatchDescription.depthBuffer = ffxGetResourceDX12(m_motionVectorDepth.Get(), GetFfxResourceDescription(m_motionVectorDepth.Get()), L"CacaoInputDepth", FFX_RESOURCE_STATE_COMPUTE_READ);
dispatchDescription.normalBuffer = ffxGetResourceDX12(m_normals.Get(), GetFfxResourceDescription(m_normals.Get()), L"CacaoInputNormal", FFX_RESOURCE_STATE_COMPUTE_READ);
dispatchDescription.outputBuffer = ffxGetResourceDX12((m_cacaoOutput.Get()), GetFfxResourceDescription(m_cacaoOutput.Get()), L"CacaoInputOutput", FFX_RESOURCE_STATE_UNORDERED_ACCESS);
dispatchDescription.proj = &proj;
dispatchDescription.normalsToView = &normalsWorldToView;
dispatchDescription.normalUnpackMul = 2;
dispatchDescription.normalUnpackAdd = -1;
errorCode = ffxCacaoContextDispatch(&m_cacaoContext, &dispatchDescription);
FFX_ASSERT_MESSAGE(errorCode == FFX_OK, "Error returned from ffxCacaoContextDispatch");
```

During shutdown, destroy the CACAO context:

```C++
ffxCacaoContextDestroy(&m_cacaoContext);
```

## Quality modes

This sample exposes quality presets as well as operating in a
downsampled resolution. They are provided in a structure
FFX_CACAO_PRESETS defined within CACAO.h.

| Default Values  |  radius = 1.2f<br/>shadowMultiplier = 1.0f<br/>shadowPower = 1.50f<br/>shadowClamp = 0.98f<br/>horizonAngleThreshold = 0.06f<br/>fadeOutFrom = 20.0f<br/>fadeOutTo = 40.0f<br/>adaptiveQualityLimit = 0.75f<br/>sharpness = 0.98f<br/>detailShadowStrength = 0.5f<br/>generateNormals = FFX_CACAO_FALSE<br/>bilateralSigmaSquared = 5.0f<br/>bilateralSimilarityDistanceSigma = 0.1f          |
|-------------------|--------------------------------------------------|
| Adaptive Quality Native Resolution |  qualityLevel = FFX_CACAO_QUALITY_HIGHEST blurPassCount = 2                                |
| High Quality Native Resolution |  qualityLevel = FFX_CACAO_QUALITY_HIGH blurPassCount = 2                                |
| Medium Quality Native Resolution |  qualityLevel = FFX_CACAO_QUALITY_MEDIUM blurPassCount = 2                                |
| Low Quality Native Resolution |  qualityLevel = FFX_CACAO_QUALITY_LOW blurPassCount = 6                                |
| Lowest Quality Native Resolution |  qualityLevel = FFX_CACAO_QUALITY_LOWEST blurPassCount = 6                                |
| Adaptive Quality Downsampled Resolution |  qualityLevel = FFX_CACAO_QUALITY_HIGHEST blurPassCount = 2 |
| High Quality Downsampled Resolution |  qualityLevel = FFX_CACAO_QUALITY_HIGH blurPassCount = 2 |
| Medium Quality Downsampled Resolution  |  qualityLevel = FFX_CACAO_QUALITY_MEDIUM blurPassCount = 3 bilateralSigmaSquared = 5.0f bilateralSimilarityDistanceSigma = 0.2f          |
| Low Quality Downsampled Resolution  |  qualityLevel = FFX_CACAO_QUALITY_LOW blurPassCount = 6 bilateralSigmaSquared = 8.0f bilateralSimilarityDistanceSigma = 0.8f          |
| Lowest Quality Downsampled Resolution  |  qualityLevel = FFX_CACAO_QUALITY_LOWEST blurPassCount = 6 bilateralSigmaSquared = 8.0f bilateralSimilarityDistanceSigma = 0.8f          |

# Update history

Updated with FidelityFX SDK 1.1.1 integration guidelines September 2024.

This sample was written in September 2021.

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
