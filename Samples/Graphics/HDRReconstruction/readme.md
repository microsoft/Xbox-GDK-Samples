# HDR Reconstruction Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

This sample shows how HDR can be reconstructed from an already
tonemapped SDR image as a simple postprocessing technique. The technique
is useful for adding HDR to a game without disrupting the render
pipeline, keeping the same artistic intent as the tonemapped image, and
can also be applied to SDR videos and UI splash screens. The sample has
a toggle to easily compare the difference between SDR and reconstructed
HDR.

![](./media/image1.jpeg)

One challenge with HDR is that videos and UI splash screens are mostly
saved as SDR and will look dull compared to the actual rendered scene.
This technique can HDR\'ify the SDR content automatically. Another
challenge is that when you simply remove the game\'s tonemap operator to
retain HDR scene values, the artistic intent of the image might get
lost, especially if the tonemap operator is combined with operations
like color grading, brightness and contrast. A good short-term solution
is to reconstruct HDR scene values from the already tonemapped and color
graded final image of the game, using an inverse tonemapper, thus
keeping the artistic intent of the SDR image. We refer to this as SDR
mastered, and cannot utilize HDR as an HDR mastered image, but it\'s
still a good short-term solution with which games have already shipped.

Pros

-   SDR mastered, i.e. the SDR artistic intent stays the same for HDR

-   Postprocessing stays the same, e.g. color grading, tone mapping, AA

-   Cut scene videos stay the same

-   Simple to implement as a single postprocessing technique

Cons

-   Colors close to white look emissive, e.g. fog, particles

-   Noise like film grain gets exaggerated

-   Loss of precision, FP11:11:10 -> 10:10:10:2

-   Not as good as HDR mastered

NOTE: The sample uses several 8-bit compressed SDR images to show the
result, so compression artifacts and banding might be visible, which is
not caused by the HDR reconstruction technique.

Refer to the Xfest 2017 presentation \"[HDR Tips and Tricks from the
Trenches](http://aka.ms/XF17022)\"

The images used in this sample were obtained from
<https://www.halowaypoint.com/en-us> and <https://gearsofwar.com/en-US/>
for use in this sample demonstration

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Project Scarlett, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The sample uses the following controls.

| Action                                    |  Gamepad                  |
|-------------------------------------------|--------------------------|
| Toggle HDR/SDR                            |  A                        |
| Toggle gamut expansion                    |  B                        |
| Adjust display gamma                      |  D-Pad Up/Down            |
| Adjust reconstructed color saturation     |  D-Pad Left / Right       |
| Next image                                |  Right shoulder           |
| Previous image                            |  Left shoulder            |

# Implementation notes

In this sample, the final tonemapped SDR backbuffer of a game is
simulated by rendering an SDR tonemapped texture into the backbuffer.
Note that these images are 8-bit compressed, so you might see
compression artifacts and banding in the sample, which is not caused by
the HDR reconstruction technique.

A simple inverse tonemapper based on Reinhard is used to reconstruct HDR
values, with and extra offset and scale to bend the curve so that the
value of 0.5 returns paper white nits, and the value of 1.0f returns a
specified max nits value, e.g. 1000 nits. Refer to slides 12-15 of Xfest
2017 presentation [HDR Tips and Tricks from the
Trenches](http://aka.ms/XF17022). The HDR values can be reconstructed
by applying the inverse tonemapper per luminance or per color channel.
Similar to (forward) tonemappers, using per luminance will give correct
results, but can be a little desaturated. Using per color channel will
be more saturated, but could case color shifting. Interpolating between
the two results is an easy way to control the color saturation of the
reconstructed values.

In this sample the UI is rendered on top of the final tonemapped SDR
image, i.e. white text will be the value of (1.0f, 1.0f, 1.0f), which as
a tonemapped value, is just as bright as a tonemapped sun. Because the
value of (1.0f, 1.0f, 1.0f) is reconstructed to the max nits, e.g. 1000
nits, the white UI text will be much too bright and fatiguing to the
consumer. We therefore calculate a linear scale to dim down the UI
rendering so that when reconstructed, white text will be as bright as
the paper white nits, e.g. 300 nits.

A gamut expansion is also implemented by expanding colors from the
Rec.709 color space into the P3 color space, resulting in a more
colorful image. We separate gamut expansions for the SDR and HDR range
and then interpolate between the two. For SDR, you cannot expand to the
full P3 color space, because it will change skin tone too much,
therefore we use a custom color space that is wider than Rec.709, but
smaller than P3. For HDR we expand to the full P3 color space. This
helps keep color saturation in brights, e.g. a bright blue sky will
still be blue and not lose color the brighter it gets.

The pixel shader can easily be optimized using a 3D lookup table (LUT).

The extra detail and brightness from HDR is indicated in some of the
sample images below.

![Sample Screenshot](./media/image2.jpeg)

![Sample Screenshot](./media/image3.jpeg)

![Sample Screenshot](./media/image4.jpeg)

![Sample Screenshot](./media/image5.jpeg)

# Update history

Initial release June 2019

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
