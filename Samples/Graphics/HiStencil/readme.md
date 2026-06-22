  ![](./media/image1.png)

#   Hi-Stencil Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

Hi-stencil is turned off by default on D3D12.X. But the good news is
that titles can configure hi-stencil. There are 2 hi-stencil comparison
states that can be set by the title. Stencil operations can be run at
tile rate instead of running it at sample rate, resulting in a perf
gain. Any title which uses the stencil test is likely to see a
performance improvement by setting hi-stencil manually. The sample shows
hi-stencil API usage.

The sample also shows how to use a compute shader to read hi-stencil
results from the HTile buffer. The stencil pass can then be run using
Async Compute and can result in overall perf improvement.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

This sample doesn't support Xbox Series X|S due to changes in htile. It
will run on the devkit via emulation.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

Sample running Pixel Shader:

![](./media/image3.png)

Sample running Compute Shader:

![](./media/image4.png)

| Action                                          |  Gamepad            |
|-------------------------------------------------|--------------------|
| Toggle Hi-stencil On/Off                        |  A button           |
| Toggle Compute shader and Pixel shader implementation |  X button |
| Toggle Resummarize On/Off                       |  Y button           |
| Toggle HiStencilControlX API                    |  B button           |
| Set resource to override HiStencilControlX toggle |  Left Trigger button             |
| Collect HTile values before drawing exterior mesh |  Right Trigger button             |
| Toggle resources in ESRAM  |  Left Shoulder button             |
| Show counts from Dispatch call  |  Right Shoulder button             |
| Help                                            |  Menu button        |
| Exit                                            |  View Button        |

# Implementation Notes

Hi-stencil is turned off by default on D3D12.X. There are 2 hi-stencil
comparison states that can be set by the title (State0 and State1).

```cpp
typedef struct D3D12XBOX_HISTENCIL_COMPARE_STATE
{
    D3D12XBOX_HISTENCIL_COMPARE_FUNCTION CompareFunction : 4;
    UINT CompareValue : 8;
    UINT CompareMask : 12;
    BOOL Enabled : 8;
} D3D12XBOX_HISTENCIL_COMPARE_STATE;

typedef struct D3D12XBOX_HISTENCIL_CONTROL
{
    D3D12XBOX_HISTENCIL_COMPARE_STATE State0;
    D3D12XBOX_HISTENCIL_COMPARE_STATE State1;
} D3D12XBOX_HISTENCIL_CONTROL;
```

SetHiStencilStateX and SetHiStencilControlX APIs have been added to
D3D12.X to help set these hi-stencil comparison states.

-   **SetHiStencilStateX** is used to set the hi-stencil compare test on
    a depth stencil resource and will persist on a given resource when
    it is bound.

-   **SetHiStencilControlX** is used to temporarily set the state
    globally and will be used for any stencil resource until a depth
    stencil with an associated hi-stencil state is bound to the
    pipeline.

```cpp
void D3DAPI SetHiStencilStateX(
    _In_ ID3D12Resource* pResource,
    _In_opt_ const D3D12XBOX_HISTENCIL_CONTROL* pControl);

void D3DAPI SetHiStencilControlX(
    _In_opt_ const D3D12XBOX_HISTENCIL_CONTROL* pControl);
```

The results of the comparison are stored in the HTile buffer. These
tests when correlated to a stencil test later in the frame, can be an
effective way to reject/accept tiles instead of reading individual
sample stencil values.

## HTile Buffer:

The HTile buffer stores 32-bits of metadata for every 8x8 block of
pixels of the depth stencil buffer.

Interpretation of HTile bits, when

-   Stencil is not present (Not covered in this sample)

![](./media/image5.png)

-   Stencil is present

![](./media/image6.png)

-   **SMem**: Stencil Memory Format is a 2-bit value which indicates how
    the stencil values in the tile are stored

| SMem  |  Description                                                  |
|-------|--------------------------------------------------------------|
| 0     |  **Clear** -- The entire tile has the Clear value.            |
| 1  |  **Single Value** **--** Entire tile has a single stencil value. 1^st^ 8-bits of data in the stencil buffer tile is the value for entire tile                                    |
| 2  |  **Expanded and Clear** -- Unused currently on XBox. All the samples for this tile in the stencil buffer have the clear value.                                                       |
| 3  |  **Expanded** -- The tile has been expanded, so the individual samples in the stencil buffer have the correct stencil value                                                |

-   **SR\***: Hierarchical Stencil Pretest Results. There are 2
    independent compare results (SR0 and SR1) that can be stored in the
    HTile. The APIs SetHiStencilStateX and SetHiStencilControlX can be
    used to set the comparison function, mask and compare values. SR0
    and SR1 have 2 bits each:

    -   **Bit 0**: May Fail

    -   **Bit 1**: May Pass

> Interpretation of values set by these 2 bits:

| SR\*      |  Description                                              |
|-----------|----------------------------------------------------------|
| 0         |  **Cleared or not compared**                              |
| 1  |  **May Fail** -- At least one sample has failed hi-stencil test                                          |
| 2  |  **May Pass** -- At least one sample has passed hi-stencil test                                          |
| 3  |  **May Pass or May Fail** - At least one sample passed hi-stencil test and at least one sample failed hi-stencil. The hardware can also set both these bits if the hi-stencil state test is turned off or if the Stencil pass is ALWAYS or NEVER or if the compare mask is 0 or there are stencil writes in flight. So, in those cases, even if the tile says has a value of 3 for SR\*, all samples could fail.                                  |

-   **Hi-Stencil states in the sample**

In the sample, SetHiStencilStateX is used to pass the test for 2 of
the discs which are drawn with stencil values COMPARE_VALUE and
(COMPARE_VALUE+1).

```cpp
hiStencilControl.State0.Enabled = TRUE;
hiStencilControl.State0.CompareFunction = D3D12XBOX_HISTENCIL_COMPARE_FUNCTION_LEQUAL;
hiStencilControl.State0.CompareValue = COMPARE_VALUE;
hiStencilControl.State0.CompareMask = 0xFF;
```

When using SetHiStencilControlX, it sets the 2nd comparison state to
make sure another disc drawn with stencil value of
(COMPARE_VALUE_2 + 1) passes the HiStencil test:

```cpp
hiStencilControl.State1.Enabled = TRUE;
hiStencilControl.State1.CompareFunction = D3D12XBOX_HISTENCIL_COMPARE_FUNCTION_LEQUAL;
hiStencilControl.State1.CompareValue = COMPARE_VALUE_2 + 1;
hiStencilControl.State1.CompareMask = 0xFF;
```
## Compute Shader:

The HTile buffer can be used to read the hi-stencil pre-test results and
these can be used to implement stencil tests in a compute shader. This
tends to be slower than the usual pixel shader implementation but has
the advantage that it can be run in Async Compute and thus shave off
some time from the graphics pipe. There are 2 compute shaders used in
the sample:

-   **Interpret HTile values:** This shader uses an Append buffer to
    store the tile coordinates which have the "MayPass" bit of SR\* set.
    For debugging purposes, the shader can also count number of tiles
    with "MayFail" bit set, as well as number of may pass tiles which
    have a single value (SMem =1).

-   **Determine expanded stencil results:** The second compute shader
    uses the values from the above append buffer to determine samples
    that have passed stencil and blends green or blue color with the
    target. It uses SGPRs for tiles which have a single stencil value
    (SMem = 1) (blend output with Green color) and uses VGPRs if the
    tile is expanded as each thread needs to read a separate value from
    the stencil buffer (blend output with Blue color).

## Useful PIX counters:

| Counter                    |  Description                             |
|----------------------------|-----------------------------------------|
| DB_P ERF_SEL_DB_SC_S\_TILE_RATE |  Stencil operations at tile rate. Counter available only on Scorpio.      |
| DB_PER F_SEL_DB_SC_TILE_TILE_RATE  |  Depth and stencil operations at tile rate. Can be used on Durango as the above counter (DB_PERF_SEL_DB_SC_S\_TILE_RATE) is unavailable. If depth comparisons are off, this counter just shows stencil operations.                             |
| DB _PERF_SEL_DB_SC_TILE_TILES |  Total number of tiles (129,600 for a 4K target, 32,400 for 1080p)               |
| PreZSamplesFailingS  |  Number of failed stencil operations at sample rate                             |
| PreZSamplesPassing  |  Number of passed stencil operations at sample rate if depth operations are turned off. It includes depth+stencil if depth comparisons are on             |

# Results:

**Results for writing pixels when stencil passes using Pixel Shader:**

| Counter  |  Scorpio -- 4K  |   |  Durango -- 1080p |  |
|-----------------------------------|--------|--------|--------|-------|
|  |  Hi-S tencil Off |  Hi-S tencil On |  Hi-S tencil Off |  Hi-St encil On    |
| EOP to EOP duration  |  0.089 ms |  0.062 ms |  0.072 ms |  0.055 ms    |
| \% Hi-Z Tiles Rejected            |  0      |  40%    |  0      |  39.8% |
| PreZSamplesPassing (Stencil test running at sample rate) |  1,2 35,187  |  23,187  |  3 08,857  |  1 2,025 |
| DB_PERF_SEL_DB_SC_S\_TILE_RATE (Scorpio) or DB_PERF_SEL_DB_SC_TILE_TILE_RATE (Durango) (Stencil test running at tile rate) |  0  |  18,939  |  0  |  4,638 |
| \% of passing samples running at tile-rate |  0  |  98.12%  |  0  |  9 6.16% |

When using Hi-Stencil, Scorpio shows a gain of **30%**, and Durango
shows a gain of **23%**.

**Writing to pixels when stencil passes:**

| Counter  |  Scorpio -- 4K  |   |  Durango -- 1080p |  |
|---------------------------------|--------|--------|--------|-------|
|                                 |  PS     |  CS     |  PS     |  CS    |
| Sample writes passing stencil  |  1,2 35,187 |   |  3 08,857 |  |
| Time taken (DSV+RTV in DRAM)  |  0.062 ms |  0.088 ms |  0.055 ms |  0.103 ms    |
| Time taken (DSV+RTV in ESRAM)  |  \-  |  \-  |  0.027 ms |  0.044 ms    |

When targets are in ESRAM, the time taken by Compute shader could be
closer to the Pixel shader timing as the draw call may be bound by pixel
rate. When in DRAM, the calls are bound by the DRAM bandwidth. As
mentioned above, although the Compute Shader takes more time to execute,
running it on Async can reduce overall frame time.

# Update history

Initial release February 2019

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
