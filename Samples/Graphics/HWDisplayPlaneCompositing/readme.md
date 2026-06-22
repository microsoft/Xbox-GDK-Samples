![](./media/image1.png)

# HWDisplayPlaneCompositing

*This sample is compatible with the Microsoft Game Development Kit (March 2024)*

# Description

Xbox Series consoles provide two display planes for titles. One example of using them is to composite UI onto the final scene. This is done in the display hardware without any performance or latency costs to the title.

![](./media/cover.png)

# Building the sample

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Controls

| Action                                 |  Gamepad           |  Keyboard       |
|--------------------------------------- |--------------------|-----------------|
| Exit the sample.                       |  View Button       | Esc             |

# Implementation notes

The hardware display planes can be set up to use different resolutions, refresh rates, a mix of SDR and HDR, using different color spaces and different gamma spaces. For example, UI could be rendered at 4K 30Hz in SDR with the sRGB gamma curve, while the scene could be rendered at 1440p 60Hz in HDR in linear space.

![](./media/image2.png)

Note that HDR compositing is only supported in the March 2024 GDK and onwards. When compositing in HDR, the system level auto tone mapping will be used to generate an SDR image for GameDVR. HDR compositing is done using 200 nits as paper white
for the UI. I.e. a value of 1.0 in UI will be displayed at 200 nits. Values in the display plane with the scene will have to be normalized as usual, therefore a value of 1.0 in the scene display plane will be displayed at 10,000 nits.

Setting up display plane compositing is very simple and is done when setting the present parameters for PresentX(). This setup can be seen in the file DeviceResources.cpp

```
// NOTE: For SDR, present the scene in display plane 0 and UI in display plane 1
//       For HDR, present the scene in display plane 1 and UI in display plane 0
if (g_HDRMode)
{
    planeParameters[0].ppResources = m_uiRenderTargets[m_backBufferIndex].GetAddressOf();
    planeParameters[0].ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;    // UI is SDR in sRGB gamma space using Rec.709
    planeParameters[1].ppResources = m_sceneRenderTargets[m_backBufferIndex].GetAddressOf();
    planeParameters[1].ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_D65P3;   // Scene is linear HDR using P3 color space
    }
else
{
    planeParameters[0].ppResources = m_sceneRenderTargets[m_backBufferIndex].GetAddressOf();
    planeParameters[0].ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;    // Scene is linear SDR using Rec.709
    planeParameters[1].ppResources = m_uiRenderTargets[m_backBufferIndex].GetAddressOf();
    planeParameters[1].ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;    // UI is SDR in sRGB gamma space using Rec.709
}

ThrowIfFailed(
    m_commandQueue->PresentX(2, planeParameters, params)
);

```
# Notes

# Update history

03/8/2024 -- Created Sample.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
