  ![](./media/image1.png)

#   SimpleCompute Sample

*This sample is compatible with the Microsoft Game Development Kit (June
2020)*

# Description

![Sample Screenshot](./media/image3.png)

SimpleCompute shows how to use DirectCompute™ (i.e. Direct3D Compute
Shader) with DirectX 12. The sample demonstrates submitting compute work
to both the graphics command list and how to use the
D3D12_COMMAND_LIST_TYPE_COMPUTE interface to submit asynchronous compute
shader workloads. It updates a texture by computing the Mandelbrot set
using a compute shader.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using an Xbox One X|S devkit, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

| Action                             |  Gamepad                         |
|------------------------------------|---------------------------------|
| Toggle Async Compute               |  A button                        |
| Reset Viewport to Default          |  Y button                        |
| Pan Viewport                       |  Left stick                      |
| Zoom Viewport                      |  Right stick                     |
| Increase Zoom Speed                |  Right trigger                   |
| Exit                               |  View Button                     |
| Menu                               |  Show/hide help                  |

# Implementation notes

The primary purpose of this sample is to familiarize the reader with
creating and using a simple compute shader.

-   **CreateDeviceDependentResources**: This is where the compiled
    compute shader is loaded and the various Direct3D rendering
    resources are created. The shaders are compiled by Visual Studio.

-   **Render**: If the sample is not using asynchronous compute the
    compute shader is dispatched before the draw call that needs the
    results is dispatched. This updates the texture every frame.

-   **AsyncComputeProc**: If the sample is using asynchronous compute
    the compute shader is dispatched from this thread as soon as it's
    told to start processing. Render will wait until it's told the
    asynchronous task is complete before performing the dependent draw
    call.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
