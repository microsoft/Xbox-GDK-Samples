//--------------------------------------------------------------------------------------
// Lighting.h
//
// Header for sample
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <lamparray.h>

#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// The set of lighting effects a device can run. Each connected device is
// independently assigned exactly one of these.
enum class Effect : uint32_t
{
    ColorCycle,
    ColorWave,
    ColorWheel,
    Blink,
    WASD,
    Purposes,
    Solid,
    Health,
    Ripple,
    Reactive,
    Manual,
    Count
};

// The maximum number of user-adjustable colors any single effect exposes (WASD
// uses two: a keyboard base color and a highlight color for the W/A/S/D keys).
constexpr uint32_t c_MaxEffectColors = 2;

// Everything the sample needs to drive a single connected LampArray device.
//
// Each device owns its own effect assignment and the per-effect animation state
// that effect needs (an elapsed-time accumulator) plus its user-adjustable
// colors. It also caches data that is expensive to recompute every frame: a flat
// lamp-index array, a scratch color buffer, and per-lamp position values used by
// the position-aware effects. The cached values are filled in once, when the
// device attaches.
struct LampArrayDevice
{
    Microsoft::WRL::ComPtr<ILampArray>      lampArray;

    // Scratch buffers sized to the device's lamp count. lampIndices is the
    // identity mapping [0, lampCount) used with SetColorsForIndices; lampColors
    // receives each frame's per-lamp colors before it is submitted.
    std::unique_ptr<uint32_t[]>             lampIndices;
    std::unique_ptr<LampArrayColor[]>       lampColors;

    // Per-lamp values cached on attach to keep the effects cheap:
    //  - normalizedX: lamp X position as a fraction of the device width [0,1]
    //  - wheelAngle:  lamp angle around the device center, in radians
    std::vector<double>                     normalizedX;
    std::vector<double>                     wheelAngle;

    // Lamp indices sorted left-to-right by normalized X position, cached on attach.
    // The Health effect fills its bar in this order so it grows from one edge of
    // the device regardless of the order the hardware reports its lamps in.
    std::vector<uint32_t>                   xOrder;

    // Each lamp's distance from the device center, normalized to [0,1] against the
    // farthest lamp, cached on attach. The Ripple effect uses it to expand a ring
    // of color outward from the center.
    std::vector<double>                     radius;

    // Per-scan-code trail intensity in [0,1] for the Reactive effect, indexed by
    // the keyboard scan code's low byte. A key press sets its entry to 1.0 (in
    // WndProcHandler) and the effect fades it back to 0 over time.
    std::array<float, 256>                  keyIntensity = {};

    // Per-lamp colors for the Manual effect, one entry per lamp (normalized RGB),
    // sized on attach. The Manual effect lets a non-keyboard device's lamps each be
    // set to an individually chosen color, demonstrating addressing lamps by index.
    std::vector<std::array<float, 3>>       manualColors;

    // Per-device animation clock. elapsedSeconds accumulates effect time while the
    // current effect runs (reset when the effect is reassigned); lastCounter is the
    // QueryPerformanceCounter value from the previous run, used to measure the delta.
    double                                  elapsedSeconds = 0.0;
    uint64_t                                lastCounter = 0;

    // Per-effect color-picker values (normalized RGB), kept per effect so adjusting
    // one effect's color does not disturb another's. Slot meaning is per effect:
    // Solid and Blink use [0]; WASD uses [0] for the base and [1] for the W/A/S/D
    // highlight. speedScale multiplies the rate of the time-based effects.
    float                                   colors[static_cast<size_t>(Effect::Count)][c_MaxEffectColors][3] = {};
    float                                   speedScale = 1.0f;

    // Simulated health [0, 1] driven by a slider in the device panel and consumed
    // by the Health effect: it fills a green-to-red bar across the lamps and pulses
    // the lit lamps when health is critically low.
    float                                   health = 1.0f;

    // The effect currently assigned to this device, and a flag that requests a
    // one-time reset of the animation state the next time the effect runs (set
    // when the effect changes or the device first attaches).
    Effect                                  effect = Effect::ColorCycle;
    bool                                    effectChanged = true;

    // Number of lamps on the device (GetLampCount), captured on attach. This is the
    // dimension of the cached buffers above and is read every frame by the effects.
    uint32_t                                lampCount = 0;

    // Connection/availability state from the status callback. Available means this
    // app controls the lighting; a device can be Connected but not Available when
    // another app or the system owns it. Defaults to available so the common case is
    // correct the instant the device attaches; the callback downgrades it if control
    // is lost.
    LampArrayStatus                         status = LampArrayStatus::Connected | LampArrayStatus::Available;

    // Whole-device brightness [0, 1]. Seeded from GetBrightnessLevel on attach and
    // applied via SetBrightnessLevel from the panel slider.
    float                                   brightness = 1.0f;

    // Display label and ImGui widget id, derived on attach from kind and VID/PID.
    std::string                             label;

    // Which lamp the per-lamp inspector currently displays. Pure UI state for the
    // device panel; the inspector reads this lamp's ILampInfo live for display.
    uint32_t                                inspectorLamp = 0;
};

// The sample owns the lifetime of the LampArray callback and the collection of
// connected devices. The collection is touched from two threads -- the worker
// thread that delivers status callbacks and the main thread that runs and draws
// the effects -- so it is guarded by m_devicesMutex.
namespace ImGuiAtg { class DeviceContext; }

class Sample
{
public:
    Sample() = default;
    ~Sample() = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    void Initialize(HWND hWnd);
    void Update();
    void Draw();
    void Shutdown();
    void Activated();
    void Deactivated();
    LRESULT WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#ifdef _GAMING_XBOX
    void Suspend(ImGuiAtg::DeviceContext* dc);
    void Resume(ImGuiAtg::DeviceContext* dc);
#endif

private:
    // The LampArray status callback. Its LampArrayStatus flags report presence
    // (Connected) and control (Available), covering attach, detach, and
    // availability changes. It runs on a LampArray worker thread.
    static void CALLBACK LampArrayStatusCallback(void* context, LampArrayStatus currentStatus, LampArrayStatus previousStatus, ILampArray* lampArray);

    void OnDeviceStatusChanged(ILampArray* lampArray, LampArrayStatus currentStatus);

    void AddDevice(ILampArray* lampArray, LampArrayStatus currentStatus);
    void RemoveDevice(ILampArray* lampArray);

    void DrawDevicePanel(LampArrayDevice& device);
    void DrawLampInspector(LampArrayDevice& device, ILampArray* lampArray);

    LampArrayCallbackToken                          m_callbackToken = LAMPARRAY_INVALID_CALLBACK_TOKEN_VALUE;

    // Guards m_devices: the status callback runs on a worker thread while Update()
    // and Draw() run on the main thread.
    std::mutex                                      m_devicesMutex;
    std::vector<std::unique_ptr<LampArrayDevice>>   m_devices;
};
