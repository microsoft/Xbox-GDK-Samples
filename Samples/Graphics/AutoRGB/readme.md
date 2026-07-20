![](./media/image1.png)

# AutoRGB Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2023 QFE1)*

# Description

This sample demonstrates how to extract a single, representative ambient color from a given scene and use it to light up lamps from connected devices (HID). uses the LampArray api to communicate to the device, so this sample **requires** march 2023 QFE 1 GDK or later versions.

![](./media/readme/image0.png)

> **Please note:** As of the March 2023 QFE1 release, the GDK LampArray API only supports the following devices on console. Support for additional devices will be added in future recovery releases.
> - Razer Turret for Xbox One (keyboard and mouse)
> - Razer BlackWidow Tournament Edition Chroma V2

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Controls

| Action                                       |  Gamepad               |
|----------------------------------------------|------------------------|
| Switch between heuristics                    |  A                     |
| Switch scene tint                            |  B                     |
| Switch modes                                 |  Y                     |
| Toggle lights (sample mode)                  |  RB                    |
| Toggle histogram UI                          |  LB                    |
| Modify Color Transition Speed                |  Right/Left DPad       |
| Modify Base Light Intensity (sample mode)    |  Up/Down DPad          |
| Move to the next image (screenshot mode)     |  RT                    |
| Move to the previous image (screenshot mode) |  LT                    |
| Mode the Camera around (sample mode) |  LT-RT for moving up, down Right Stick to rotate view Left Stick to move translate |
| Exit the sample.                             |  View Button           |

# Implementation notes

The sample demonstrates has two modes. 

## Sample Mode

![](./media/readme/image1.png)

The first (default) is a regular sample where the camera can be freely moved around a lit scene. The scene will be rendered with a color band around it, which representes the color currently being extracted. The camera can be freely moved around the scene, and the color will be updated in real time (it is calculated from the previous frame's information, so it has a delay of 1 frame).

The controls allow to modify the scene in the following ways. The lights can be toggled on and off (to test different levels of illumination and how the extracted color looks). Also the **Base Light Intensity** level can be controlled to go up or down. If set to zero, this means the scene will be pitch black when the lights are toggled off. The sample also allows to change the speed at which the representative color changes. This can go as fast as 1 frame, but it can also be made more smooth by decreasing the **Color Transition Speed**.

Finally, by pressing B, the sample will apply a red, green, or blue tint to the whole scene. This is to easily test if the transition speed feels right, or if the Heuristics used are handling basic cases properly (more on heuristics below). By pressing B, the sample will rotate between no tint, red, blue and green (and then back to no tint). This is only for the sample mode.

## Screenshot Mode

![](./media/readme/image2.png)

When pressing Y, the sample toggles between modes. Screenshot mode will show a static image of a game, with the same color band around. The gamepad triggers allow to navigate the screenshots. This is meant to show how well the extracted color can match scenes from production level games. The camera cannot be moved around during this mode, nor can the tint or the base light intensity be modified.

# UI

The sample has pass (Histogram in PIX) in which it will perform some image processing on the current scene (either the sample background or the currently selected screenshot) and obtain 4 histograms, as shown below:

![](./media/readme/image3.png)

The image below has the red tint applied to the scene. The Histogram UI changes accordingly, showing more red in the higher intensity bins.
![](./media/readme/image4.png)

The first three histograms correspond to the amount of Red, Green and Blue present on the scene, and how they distribute in terms of their intensity. Each histogram is divided into 8 bins. Since each color's intensity goes from 0 to 255, each bin contains 32 intensity values, from low intensity (left) to high intensity (right). The height of each bin will correspond to the number of pixels that fall in that intensity range. The fourth histogram will show the luma component, and will be useful to get a measure of the scene's luminosity. 

> Some devices only allow to have their colors set, but others also allow for the intensity to be modified.  

# Heuristics

Once the sample obtains the color distribution information in the form of histograms, it can use this to get a representative ambient color. to do this, it needs to use a **heuristic**. Right now this sample only aims to get a single color for the whole scene (so we get no information regarding color locations). This means the heuristic needs to handle cases where there might be an equal distribution of the three components (red, green, blue) and not return grey, and instead derive which color is contributing more to the scene.

The sample has three heuristics which can be changed with the A button. Here is a brief description of the three:

1. Top X Buckets: This method will start by looking at only the highest bin of the three components. If the amount of pixels covered by that first bin surpasses a set threshold, then only this bin will be utilized to calculate the color. Otherwise, the next bin is added and the process repeats. The idea of this heuristic is to give more weight to the colors which have higher intensity pixels, while also adaptively changing the number of bins to account for pixel count.

2. Average: This simply takes a weighted average of all the histograms, and then normalizes the result. It gives similar results for well lit scenes, but also tends to exagerate colors on more dimmer, darker scenes.

3. Buckets x AVG (**Default**): As the name indicates, this heuristic will interpolate the result of the previous two and offer an in between.

# Notes

# Update history

04/08/2023 -- Ported Sample.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
