  ![](./media/image1.png)

#   AmbientOcclusion Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

This sample demonstrates two ambient occlusion techniques: MiniEngine's Screen Space Ambient Occlusion (SSAO) and Intel's Ground Truth Ambient Occlusion (GTAO). The sample also shows FP16 versions of some of the shaders which can improve performance with slight tradeoffs in quality.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The left and right thumbsticks can be used to move the camera around the scene. 
The up and down D-Pad buttons select the current option. 
The left and right D-Pad buttons modify option values. 
The A button changes the display between the AO texture and the scene. 
The B button switches between SSAO and GTAO. 
The Y button turns AO on or off in the scene rendering. 
The X button toggles between FP16 and FP32 shaders (Scarlett only).

![](./media/image2.bmp)

# Controls

| Action                        |  Gamepad                |
|-------------------------------|-------------------------|
| Toggle AO only rendering      | A button                |
| Toggle SSAO/GTAO              |  B button               |
| Turn AO on/off                | Y button                |
| Toggle FP16/FP32 shaders      | X button                |
| Select an option              |  D-Pad up/down          |
| Modify selected option        |  D-Pad right/left       |
| Move camera                   |  Left Thumbstick        |
| Rotate camera                 |  Right Thumbstick       |

# Implementation notes

The sample demonstrates 2 ways of rendering ambient occlusion.

1.  Screen Space Ambient Occlusion (SSAO): The SSAO implementation is from [MiniEngine](https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/MiniEngine). It reads from the compressed depth buffer directly using HTile and creates downsampled versions. It then samples the depth textures and creates multiple resolutions of AO textures. The final pass reads from the AO textures and upsamples and blurs them.

2.  Ground Truth Ambient Occlusion (GTAO): The GTAO implementation is from [Intel](https://github.com/GameTechDev/XeGTAO/tree/master). It first downsamples the depth buffer to create a mip chain. If the engine does not already export normals it runs a pass to generate normals from the depth buffer. It then runs the main pass to sample from the depth buffer to generate an initial AO texture and an edge texture. The final pass denoises the AO texture.

Both methods have support for FP16 shaders on Scarlett for passes where it provides a performace improvement compared to the FP32 version. Performance improvements with FP16 typically come from a combination of increased occupancy (due to packing two values into one register) or fewer VALU operations (from use of packed math instructions).

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
