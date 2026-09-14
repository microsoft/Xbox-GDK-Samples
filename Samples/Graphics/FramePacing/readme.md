<!-----
---
page_type: sample
languages:
- cpp
products:
- gdk
urlFragment: "framepacing"
extendedZipContent:
- path: LICENSE
  target: LICENSE
- path: Kits
  target: Kits
- path: Media
  target: Media
description: "This sample demonstrates how CPU work, GPU work, swap chain buffers, and Xbox frame scheduling interact to affect frame rate, latency, and presentation margin."
---
----->

# Frame Pacing Sample

*This sample requires the Microsoft Game Development Kit with Xbox Extensions
(October 2024 or later).*

# Description

FramePacing demonstrates how CPU work, GPU work, swap chain buffers, and Xbox frame
scheduling interact to affect frame rate, latency, and presentation margin.
The sample can apply configurable artificial workloads to the update,
rendering, graphics, and compute stages, then visualize the resulting frame
pipeline with line and interval graphs.

The controls support both manual experimentation and automatic modes. Automatic
frame loads replay sawtooth, jitter, glitch, and spike sequences. Automatic
pacing uses recent frame statistics to select a frame period, buffer count, and
frame-event offset for the selected target rate.

# Building the sample

The sample requires Visual Studio 2022, the `v143` toolset, and a current GDK
with Xbox Extensions.

For Xbox One, set the active solution platform to
`Gaming.Xbox.XboxOne.x64`.

For Xbox Series X|S, set the active solution platform to
`Gaming.Xbox.Scarlett.x64`.

*For more information, see* **Running samples** *in the GDK documentation.*

# Controls

| Action | Gamepad |
|---|---|
| Select a setting | D-pad or left stick Up/Down |
| Change the selected setting | D-pad or left stick Left/Right |
| Toggle automatic frame pacing | A |
| Toggle automatic frame loads | X |
| Pause graph updates | B |
| Zoom graph history | Left/Right trigger |
| Pan graph history | Right stick Left/Right |
| Exit | View |


# Implementation notes

`ID3D12Device::SetFrameIntervalX` sets the target frame interval and period.
`ID3D12Device::ScheduleFrameEventX` and `WaitFrameEventX` schedule and wait for
the frame origin. Each frame carries a `D3D12XBOX_FRAME_PIPELINE_TOKEN` through
update, graphics, compute, and `PresentX`.

The sample submits timestamp queries on both graphics and compute queues and
correlates them with `GetFrameStatisticsX`. The resulting data drives the
frame-time, latency, margin, CPU, GPU, and flip graphs. Artificial workloads
exist only to make pacing behavior and missed budgets easy to reproduce.

The automatic pacing mode is illustrative rather than a production policy.
Games should choose thresholds and safety margins using their own workload,
latency goals, supported displays, and quality strategy.

The **Frame Threshold (%)** menu option is passed to `PresentX` as
`ImmediateThresholdPercent`. When a frame finishes late, the runtime displays
it immediately only if scanout has not passed that percentage of the screen.
A threshold of 0 disables limited tearing, while 100 permits a late flip
anywhere during scanout.

The tear indicator shows `PercentScanned` from the display statistics for the
latest completed frame. A value of 0% is a VBlank-aligned flip with no visible
tear. To demonstrate limited tearing, increase **Frame Threshold (%)** above
zero and use a frame-load sequence that occasionally exceeds the frame budget.
The indicator then moves to the scan position where the late frame was
displayed.

# Privacy Statement

When compiling and running a sample, the file name of the sample executable
will be sent to Microsoft to help track sample usage. To opt out of this data
collection, remove the block of code in `Main.cpp` labeled "Sample Usage
Telemetry".

For more information, see the
[Microsoft Privacy Statement](https://privacy.microsoft.com/privacystatement/).
