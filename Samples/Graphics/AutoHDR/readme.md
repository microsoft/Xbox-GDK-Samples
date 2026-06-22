# Auto HDR Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

Auto HDR is an Xbox Series X|S feature that can visually enhance an SDR
title by automatically adding HDR to it at a system level. The feature
uses Xbox Series X|S hardware, so there is no performance cost to the
title or system. I.e. no extra CPU or GPU, no extra memory or bandwidth,
and no extra latency is added. The feature is applied to the majority of
back compat ERA titles, but it can also be used by native Xbox Series
X|S GDK titles. This sample shows how GDK titles can use Auto HDR as its
HDR implementation by using the D3D device creation flag
`D3D12XBOX_CREATE_DEVICE_FLAG_ENABLE_AUTO_HDR`.

![A screenshot of a video game Description automatically generated](./media/image1.png)

The images used in this sample were obtained from
<https://www.halowaypoint.com/en-us> and <https://gearsofwar.com/en-US/>
for use in this sample demonstration

# Building the sample

This sample is only supported for Xbox Series X|S using `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The sample uses the following controls.

| Action                                    |  Gamepad                  |
|-------------------------------------------|--------------------------|
| Toggle adjusting UI brightness            |  A                        |
| Adjust reconstructed color saturation     |  D-Pad Left / Right       |
| Next image                                |  Right shoulder           |
| Previous image                            |  Left shoulder            |

# Implementation notes

**Render as SDR**

Since Auto HDR is applied at a system level, the title should just
render as SDR. The sample renders and presents as SDR.

**Swap buffer format**

It is highly recommended to use a high precision swap buffer format to
avoid precision artifacts like banding.

```cpp
m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
                                                          DXGI_FORMAT_UNKNOWN,
                                                          2,
                                                          DX::DeviceResources::c_Enable4K_UHD);
```

**Switch TV to HDR mode and create D3D device with Auto HDR flag**

Even though the title renders everything in SDR, the title still needs
to switch the TV into HDR mode.

```cpp
if (SwitchDisplayToHDR())
{
    params.CreateDeviceFlags = D3D12XBOX_CREATE_DEVICE_FLAG_ENABLE_AUTO_HDR;
}
```
**Switch TV to HDR mode after Resume/Unconstrained**

While a title is suspended/constrained, it's possible that the console's
display settings might have changed, so the title has to switch the TV
to HDR mode again when resuming.

```cpp
void Sample::OnResuming()
{
    // Display modes could have changed while title was suspended, so we need to make
    // sure that the TV is still in HDR mode. This is required for native HDR and
    // Auto HDR.

    m_deviceResources->SwitchDisplayToHDR();
```

**Adjust UI brightness**

```cpp
    if (isDisplayInHDRMode)
    {
        // Auto HDR will show pure white pixels as 1000 nits, so text/UI/HUD will become
        // much too bright. We linearly scale down the brightness of the UI
        m_UIBrightnessScale = m_bAdjustUIBrighness ? 0.8f : 1.0f;
    }
    else
    {
        // If the TV is in SDR mode, we don\'t do any brightness scaling
        m_UIBrightnessScale = 1.0f;
    }
}
```
# Update history

Initial release June 2021

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
