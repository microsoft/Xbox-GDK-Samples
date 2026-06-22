  ![](./media/image1.png)

# Bank Rotation Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# Description

*Bank Rotation* is a technique which helps in speeding up writes to
multiple render targets in the same draw call. This is useful during
deferred passes when writing to multiple GBuffers. It only helps if the
target is in DRAM as bank rotation doesn't affect targets in ESRAM.

Xbox One and Xbox One S have 8 banks, Xbox One X has 16 banks and Xbox
Series X|S has 2 banks. This feature is particularly useful on Xbox One
X as all the GBuffers will be located in DRAM.

**Results observed:**

With 4 GBuffers the sample shows around **10%-14%** gain on Xbox One
with each GBuffer assigned to a different bank and around **20%** with
Xbox One X. The gain will increase if you have more GBuffers in the
pass.

# ![](./media/image3.png) Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

| Action                                 |  Gamepad                     |
|----------------------------------------|-----------------------------|
| Change resource allocation technique   |  A button                    |
| Exit                                   |  View Button                 |

# Implementation notes

Bank rotation support is only available when using the DirectX 12 API.
Here is a code example for Xbox One. The code for Xbox Series X|S is
very similar, refer to the sample source code.

*Committed Resources:*

```cpp
ResDesc.Layout = D3D12XBOX_BANK_ROTATED_TILE_MODE(D3D12_TEXTURE_LAYOUT, bankRotationIndex);
```
This new layout can then be used with CreateCommittedResource while
creating the resource.

*Placed or Component Placed Resources:*

```cpp
D3D12_GPU_VIRTUAL_ADDRESS rotatedAddress;

assert(XGComputeBankRotationAddress((XG_GPU_VIRTUAL_ADDRESS)gpuAddress, 
                                    &xgResLayout[gbufferIndex], 
                                    0, 
                                    0, 
                                    bankRotationIndex, 
                                    &rotatedAddress));
```

This new rotatedAddress can be used to place the resource.

# Known issues

None

# Update history

Initial release May 2017

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
