# Lighting Sample

# Description

This sample demonstrates the **LampArray API** (`lamparray.h`) for controlling
the RGB lighting on devices such as keyboards and mice. It enumerates connected
devices, queries their capabilities and per-lamp geometry, and drives them with a
selection of animated and static effects. Lighting is treated as a **per-device**
property: every connected device is tracked independently and can be assigned its
own effect at runtime.

The sample uses the flat C-style `lamparray.h` interface directly (`ILampArray`,
`RegisterLampArrayStatusCallback`, `SetColor`, `SetColorsForIndices`,
`SetColorsForScanCodes`) -- it does **not** use the WinRT lighting API.

> **Please note:** As of the March 2023 QFE1 release, the GDK LampArray API only supports the following devices on console. Support for additional devices will be added in future recovery releases.
> - Razer Turret for Xbox One (keyboard and mouse)
> - Razer BlackWidow Tournament Edition Chroma V2

# Building the Sample

Build with Visual Studio 2022 (or greater) using the `Lighting.sln`
solution. The sample supports the following platforms:

| Platform | Notes |
|---|---|
| `x64` | Windows desktop (PC). GDK-free. |
| `ARM64` | Windows on ARM desktop (PC). GDK-free. |
| `Gaming.Xbox.Scarlett.x64` | Xbox Series X/S. |
| `Gaming.Xbox.XboxOne.x64` | Xbox One. |

The Windows desktop `x64` and `ARM64` configurations are plain Win32 builds and
do **not** require the GDK.

## SDK compatibility

The Xbox (`Gaming.Xbox.*`) configurations call `XGameRuntimeInitialize` and use
the GDK Direct3D headers, so they require the **October 2025 Microsoft GDK** (or
later) to build and run.

# Using the sample

Connect one or more supported LampArray devices. Each device appears as a collapsible panel showing its
vendor/product IDs, kind, hardware version, lamp count, physical bounding box,
and scan-code support, along with a combo box to choose that device's effect. A
newly connected device immediately starts running the **Color Cycle** effect.

Each panel also has a collapsible **Lamp inspector**. Pick a lamp with the index
slider to see that single lamp's per-lamp details: its declared purposes, its
position within the device (in millimeters), its per-channel RGB and gain level
counts (how finely it can reproduce color), its update latency, its scan code (on
keyboards), and whether it has a fixed, non-settable color.

The available effects are:

| Effect | Behavior | Color pickers |
|---|---|---|
| Color Cycle | Uniformly cycles colors across all lamps. | -- |
| Color Wave | Rainbow that scrolls horizontally across the device. | -- |
| Color Wheel | Rainbow that rotates around the center of the device. | -- |
| Blink | Fades a chosen color off and on across all lamps. | 1 |
| WASD | Highlights the W/A/S/D keys against a colored keyboard (keyboards only). | 2 (base + highlight) |
| Purposes | Colors each lamp by its declared purpose (control, accent, branding, status, illumination, presentation). | -- |
| Solid | A single solid color you choose. | 1 |
| Health | Green-to-red health bar that pulses red when health is low. | -- |
| Ripple | Rings of a chosen color expand outward from the center of the device. | 1 |
| Reactive | Lights each key as you press it, then fades it out (keyboards only). | 1 |
| Manual | Set each lamp to its own color (non-keyboard devices only). | one per lamp |

Some effects are gated by device kind: **WASD** and **Reactive** are offered only
on devices that report scan-code support (keyboards), while **Manual** is offered
only on non-keyboard devices, where the small lamp count makes a per-lamp color
picker practical.

Each panel shows only the controls the selected effect uses. The time-based
effects (Color Cycle, Color Wave, Color Wheel, Blink, Ripple, plus Health's
low-health pulse) add a **Speed** slider (0.25x--4x). **Health** adds a **Health**
slider for the simulated HP the bar reflects: multi-lamp devices fill a bar in
proportion to HP and pulse red below 25%, while single-lamp devices show one
aggregate color. Colors are kept per effect, so adjusting one effect's color does
not disturb another's.

Each panel also exposes a **Brightness** slider (`ILampArray::SetBrightnessLevel`)
and a **Status** line. A device is normally *Available*, meaning this app controls
its lighting; if another application takes over, it becomes *Connected, not
available* and the panel's controls are disabled until control returns.

A log panel to the right reports devices connecting and disconnecting; drag the
bar between the two to resize. On the Windows desktop builds, **F11** or
**Alt+Enter** toggles borderless fullscreen.

# Implementation notes

The LampArray API usage lives in `Lighting.cpp`. The eleven effect routines 
are isolated in `LightingEffects.cpp`.

**Device discovery and availability.** `Sample::Initialize` calls
`RegisterLampArrayStatusCallback`. A single callback reports each device's
`LampArrayStatus`, whose flags encode both presence (`Connected`) and whether this
app currently controls the lighting (`Available`). The sample derives attach/detach
from `Connected` transitions and treats loss of `Available` as another app taking
ownership. Registering with `LampArrayEnumerationKind::Async` returns immediately
and enumerates already-connected devices on a LampArray **worker thread** -- so the
callback does **not** run on the sample's main thread.

**Per-device state.** Each device is a `LampArrayDevice` (`Lighting.h`). On attach
the sample caches the data the effects need so they stay cheap per frame: the
device's metadata, a flat lamp-index array, a scratch color buffer, and -- for the
position-aware effects -- each lamp's normalized X position, its angle around the
device center, its normalized distance from the center (Ripple), and a
left-to-right lamp ordering (Health bar). The assigned effect, its animation state,
and the per-device colors and speed also live here, so each device animates
independently.

**Thread safety.** Because status callbacks arrive on a worker thread, the device
collection (`m_devices`) is guarded by a `std::mutex`: the callback locks to add,
remove, or update a device; `Update()` and `Draw()` lock while iterating. Connect/
disconnect messages are written to the ImGui log directly from the callback.
`Shutdown()` calls `UnregisterLampArrayCallback`, which waits for any in-flight
callback to finish before the devices are released.

**Driving the lights.** `Update()` runs each device's assigned effect via
`LightingEffects::Run`, skipping any device that is not currently `Available` (its
writes would be ignored). Effects set colors with `ILampArray::SetColor` (whole
device), `SetColorsForIndices` (per-lamp -- Color Wave, Color Wheel, Health,
Manual, Ripple), or `SetColorsForScanCodes` (WASD, Reactive). `SetColorsForScanCodes`
is used only after `ILampArray::SupportsScanCodes()` returns true, demonstrating
capability querying before use. When a device regains availability its effect is
forced to reapply, since another app may have driven the lamps in the meantime.

**Addressing lamps by purpose.** The Purposes effect shows an alternative to
index-based addressing: for each `LampPurposes` role it calls
`GetIndicesCountForPurposes` and `GetIndicesForPurposes` to retrieve the serving
lamps, then tints them with `SetColorsForIndices`. This lets an app target, say,
only a keyboard's status indicators without knowing its physical layout.

**Spatial effects.** Three effects color lamps by physical position, demonstrating
the three ways a device's geometry can be read: Color Wave maps the lamp's
**linear** X position to hue, Color Wheel its **angular** position around the
center, and **Ripple** its **radial** distance from the center. Each value is cached
once on attach.

**Per-lamp introspection.** The Lamp inspector reads a single lamp's `ILampInfo`
(`ILampArray::GetLampInfo`) live: `GetPurposes`, `GetPosition`, the per-channel
`GetRedLevelCount` / `GetGreenLevelCount` / `GetBlueLevelCount` / `GetGainLevelCount`
(color resolution), `GetUpdateLatencyInMicroseconds`, `GetScanCode`, and
`GetFixedColor`. The level counts and fixed-color flag matter on real hardware: a
lamp with low level counts shows only a coarse palette, and a lamp that reports a
fixed color ignores color writes entirely.

**Reacting to input.** The **Reactive** effect closes the input-to-lighting loop.
`Sample::WndProcHandler` receives `WM_KEYDOWN`; the scan code in bits 16-23 of
`lParam` is the same space passed to `SetColorsForScanCodes`. A press sets that
key's trail intensity to full, and the effect fades every intensity toward zero
each frame. The message is not consumed, so ImGui and the default handler still see
it.

**Scaling effects to the lamp count.** Devices range from a single-lamp gamepad to
a full keyboard, so the effects branch on lamp count rather than device kind. The
**Health** effect lights `round(health * lampCount)` lamps in left-to-right order,
reading as a coarse bar even on a two-lamp device; a single-lamp device shows one
aggregate color, and below a critical threshold the whole device pulses red. The
**Manual** effect exposes one color picker per lamp and applies them with
`SetColorsForIndices`, addressing each lamp individually by index.

**Brightness.** Each panel's brightness slider calls `ILampArray::SetBrightnessLevel`
to scale the device's output; the initial value is read back with
`GetBrightnessLevel` on attach. `GetMinUpdateIntervalInMicroseconds`, shown in the
info table, is the hardware floor on update frequency.

**Frame timing.** The effects are time-based and each times itself:
`LightingEffects::Run` measures the wall-clock time since that device's effect last
ran (via `QueryPerformanceCounter`) and advances the animation by that delta, so
animations run at the same speed regardless of refresh rate and both `Main.cpp` and
`Update()` stay free of timing logic. The delta is clamped to a small maximum so a
long stall (a breakpoint or window drag) cannot make an animation jump visibly.

# Update history

06/2026 -- Refreshed sample with Windows and Win32 API support.
05/2023 -- Initial Release
