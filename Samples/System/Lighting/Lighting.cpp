//--------------------------------------------------------------------------------------
// Lighting.cpp
//
// Demonstrates the Win32 LampArray API (lamparray.h) for controlling RGB
// lighting devices such as keyboards and mice. Each connected device is tracked
// independently and can be assigned its own lighting effect.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Lighting.h"
#include "LightingEffects.h"

#ifdef _GAMING_XBOX
#include "imgui/imgui_atg_device_context.h"
#endif

#include <algorithm>
#include <cmath>
#include <numeric>

// The LampArray entry points live in lamparray.lib. Linking it here keeps the
// dependency next to the code that uses it.
#pragma comment(lib, "lamparray.lib")

using Microsoft::WRL::ComPtr;

namespace
{
    void SetColorSlot(float slot[3], float r, float g, float b)
    {
        slot[0] = r;
        slot[1] = g;
        slot[2] = b;
    }

    const char* KindName(LampArrayKind kind)
    {
        switch (kind)
        {
        case LampArrayKind::Keyboard:       return "Keyboard";
        case LampArrayKind::Mouse:          return "Mouse";
        case LampArrayKind::GameController: return "Game Controller";
        case LampArrayKind::Peripheral:     return "Peripheral";
        case LampArrayKind::Scene:          return "Scene";
        case LampArrayKind::Notification:   return "Notification";
        case LampArrayKind::Chassis:        return "Chassis";
        case LampArrayKind::Wearable:       return "Wearable";
        case LampArrayKind::Furniture:      return "Furniture";
        case LampArrayKind::Art:            return "Art";
        case LampArrayKind::Headset:        return "Headset";
        case LampArrayKind::Microphone:     return "Microphone";
        case LampArrayKind::Speaker:        return "Speaker";
        case LampArrayKind::Undefined:      return "Undefined";
        default:                            return "Undefined";
        }
    }

    // A lamp can declare one or more LampPurposes (it is a flag set), so build a
    // comma-separated list of the roles it serves.
    std::string PurposesText(LampPurposes purposes)
    {
        if (purposes == LampPurposes::Undefined)
        {
            return "Undefined";
        }

        std::string result;
        const auto append = [&](LampPurposes flag, const char* name)
        {
            if ((purposes & flag) == flag)
            {
                if (!result.empty())
                {
                    result += ", ";
                }
                result += name;
            }
        };

        append(LampPurposes::Control, "Control");
        append(LampPurposes::Accent, "Accent");
        append(LampPurposes::Branding, "Branding");
        append(LampPurposes::Status, "Status");
        append(LampPurposes::Illumination, "Illumination");
        append(LampPurposes::Presentation, "Presentation");
        return result;
    }

    // A device's LampArrayStatus is a flag set: Connected means it is present,
    // Available means this app currently controls its lighting. A device can be
    // Connected without being Available when another app or the system owns it; in
    // that state our color writes are silently ignored.
    bool IsConnected(LampArrayStatus status)
    {
        return (status & LampArrayStatus::Connected) == LampArrayStatus::Connected;
    }

    bool IsAvailable(LampArrayStatus status)
    {
        return (status & LampArrayStatus::Available) == LampArrayStatus::Available;
    }

    const char* StatusText(LampArrayStatus status)
    {
        if (IsAvailable(status))
        {
            return "Available (this app controls the lighting)";
        }
        if (IsConnected(status))
        {
            return "Connected, not available (another app owns the lighting)";
        }
        return "Disconnected";
    }

    ImVec4 StatusColor(LampArrayStatus status)
    {
        if (IsAvailable(status))
        {
            return ImVec4(0.4f, 0.85f, 0.4f, 1.0f);
        }
        if (IsConnected(status))
        {
            return ImVec4(1.0f, 0.6f, 0.2f, 1.0f);
        }
        return ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
    }
}

void Sample::Initialize(HWND /*hWnd*/)
{
    // RegisterLampArrayStatusCallback is the entry point for LampArray: a single
    // callback reports presence and availability for every device, now and as
    // devices come and go. LampArrayEnumerationKind::Async enumerates the
    // already-connected devices on a LampArray worker thread rather than blocking
    // here, so the callback can fire on that thread at any time after this call.
    HRESULT hr = RegisterLampArrayStatusCallback(Sample::LampArrayStatusCallback, LampArrayEnumerationKind::Async, this, &m_callbackToken);
    if (FAILED(hr))
    {
        LOG("RegisterLampArrayStatusCallback failed (0x%08X)\n", static_cast<unsigned int>(hr));
    }
    else
    {
        LOG("Waiting for LampArray devices...\n");
    }
}

void CALLBACK Sample::LampArrayStatusCallback(void* context, LampArrayStatus currentStatus, LampArrayStatus /*previousStatus*/, ILampArray* lampArray)
{
    static_cast<Sample*>(context)->OnDeviceStatusChanged(lampArray, currentStatus);
}

// Runs on a LampArray worker thread. Translates a status change into an add,
// remove, or availability update. A device is identified by its ILampArray
// pointer, which is stable for the life of the connection.
void Sample::OnDeviceStatusChanged(ILampArray* lampArray, LampArrayStatus currentStatus)
{
    std::lock_guard<std::mutex> lock(m_devicesMutex);

    LampArrayDevice* existing = nullptr;
    for (auto& device : m_devices)
    {
        if (device->lampArray.Get() == lampArray)
        {
            existing = device.get();
            break;
        }
    }

    if (IsConnected(currentStatus) && existing == nullptr)
    {
        AddDevice(lampArray, currentStatus);
    }
    else if (!IsConnected(currentStatus) && existing != nullptr)
    {
        RemoveDevice(lampArray);
    }
    else if (existing != nullptr)
    {
        // The device regained availability: another app may have driven its lamps
        // while we were not in control, so flag the assigned effect to reapply.
        if (IsAvailable(currentStatus) && !IsAvailable(existing->status))
        {
            existing->effectChanged = true;
        }
        existing->status = currentStatus;
    }
}

// Builds a LampArrayDevice for a newly connected device: caches its lamp count
// and per-lamp geometry so the effects stay cheap, then adds it to the collection.
void Sample::AddDevice(ILampArray* lampArray, LampArrayStatus currentStatus)
{
    auto device = std::make_unique<LampArrayDevice>();

    device->lampArray = lampArray;
    device->status = currentStatus;
    device->lampCount = lampArray->GetLampCount();
    device->brightness = static_cast<float>(lampArray->GetBrightnessLevel());

    // Starting colors for the effects that expose color pickers.
    SetColorSlot(device->colors[static_cast<size_t>(Effect::Solid)][0], 0.0f, 0.6f, 1.0f);   // blue
    SetColorSlot(device->colors[static_cast<size_t>(Effect::Blink)][0], 1.0f, 0.0f, 1.0f);   // magenta
    SetColorSlot(device->colors[static_cast<size_t>(Effect::WASD)][0],  0.0f, 0.0f, 1.0f);   // keyboard base: blue
    SetColorSlot(device->colors[static_cast<size_t>(Effect::WASD)][1],  1.0f, 1.0f, 0.0f);   // W/A/S/D highlight: yellow
    SetColorSlot(device->colors[static_cast<size_t>(Effect::Ripple)][0], 0.0f, 1.0f, 1.0f);  // cyan
    SetColorSlot(device->colors[static_cast<size_t>(Effect::Reactive)][0], 1.0f, 1.0f, 1.0f); // white trail

    // lampIndices is the identity map [0, lampCount) passed to SetColorsForIndices;
    // lampColors is the scratch buffer the effects fill each frame.
    device->lampIndices.reset(new uint32_t[device->lampCount]);
    device->lampColors.reset(new LampArrayColor[device->lampCount]);
    device->normalizedX.resize(device->lampCount);
    device->wheelAngle.resize(device->lampCount);
    device->radius.resize(device->lampCount);

    device->manualColors.assign(device->lampCount, { 1.0f, 1.0f, 1.0f });

    // GetBoundingBox returns the device's physical extent (in meters) enclosing
    // every lamp; the spatial effects normalize each lamp's position against it.
    LampArrayPosition boundingBox = {};
    lampArray->GetBoundingBox(&boundingBox);
    const float centerX = boundingBox.xInMeters / 2.0f;
    const float centerY = boundingBox.yInMeters / 2.0f;

    // GetLampInfo exposes each lamp's physical position. Precompute the values the
    // spatial effects need: normalized X (Color Wave), angle about the center
    // (Color Wheel), and distance from the center (Ripple).
    for (uint32_t i = 0; i < device->lampCount; ++i)
    {
        device->lampIndices[i] = i;
        device->lampColors[i] = LampArrayColor{};

        LampArrayPosition position = {};
        ComPtr<ILampInfo> lampInfo;
        if (SUCCEEDED(lampArray->GetLampInfo(i, &lampInfo)))
        {
            lampInfo->GetPosition(&position);
        }

        device->normalizedX[i] = (boundingBox.xInMeters > 0.0f)
            ? position.xInMeters / boundingBox.xInMeters : 0.0;
        device->wheelAngle[i] = std::atan2(position.yInMeters - centerY, position.xInMeters - centerX);

        const double dx = position.xInMeters - centerX;
        const double dy = position.yInMeters - centerY;
        device->radius[i] = std::sqrt(dx * dx + dy * dy);
    }

    // Normalize distance against the farthest lamp so the Ripple front sweeps from
    // 0 (center) to 1 (edge).
    const double maxRadius = device->lampCount > 0
        ? *std::max_element(device->radius.begin(), device->radius.end()) : 0.0;
    if (maxRadius > 0.0)
    {
        for (double& r : device->radius)
        {
            r /= maxRadius;
        }
    }

    // Order lamps left-to-right so the Health bar fills from one edge regardless of
    // the order the hardware reports its lamps.
    device->xOrder.resize(device->lampCount);
    std::iota(device->xOrder.begin(), device->xOrder.end(), 0u);
    std::sort(device->xOrder.begin(), device->xOrder.end(),
        [&](uint32_t a, uint32_t b) { return device->normalizedX[a] < device->normalizedX[b]; });

    char label[128] = {};
    sprintf_s(label, "%s (VID %04X / PID %04X)", KindName(lampArray->GetLampArrayKind()), lampArray->GetVendorId(), lampArray->GetProductId());
    device->label = label;

    LOG("%s connected with %u lamps\n", label, device->lampCount);
    m_devices.push_back(std::move(device));
}

void Sample::RemoveDevice(ILampArray* lampArray)
{
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it)
    {
        if ((*it)->lampArray.Get() == lampArray)
        {
            LOG("%s disconnected\n", (*it)->label.c_str());
            m_devices.erase(it);
            break;
        }
    }
}

void Sample::Update()
{
    // Run each device's assigned effect. A device that is not Available is skipped:
    // we do not control its lighting, so its color writes would be ignored.
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    for (auto& device : m_devices)
    {
        if (!IsAvailable(device->status))
        {
            continue;
        }
        LightingEffects::Run(*device);
    }
}

void Sample::Draw()
{
    ImGuiAtg::BeginFullscreenLayout();

    ImGui::BeginChild("##SplitArea", ImVec2(0, ImGui::GetContentRegionAvail().y - ImGuiAtg::GetFooterHeight()));

    ImGuiAtg::BeginSplitV("##LogSplit", 760.0f);

    {
        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetWindowFocus();
        }

        std::lock_guard<std::mutex> lock(m_devicesMutex);

        if (m_devices.empty())
        {
            ImGui::TextDisabled("No LampArray devices connected. Connect a supported keyboard or mouse.");
        }
        else
        {
            for (auto& device : m_devices)
            {
                DrawDevicePanel(*device);
            }
        }
    }

    ImGuiAtg::SplitNext();

    ImGuiAtg::DrawLogPanel();

    ImGuiAtg::EndSplit();
    ImGui::EndChild();

    ImGuiAtg::DrawFooter();

    ImGuiAtg::EndFullscreenLayout();
}

// Draws one device: its capabilities and an effect selector.
void Sample::DrawDevicePanel(LampArrayDevice& device)
{
    ImGui::PushID(&device);

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader(device.label.c_str()))
    {
        ILampArray* lampArray = device.lampArray.Get();

        // SupportsScanCodes reports whether the device can be addressed by keyboard
        // scan code; it gates the keyboard-only effects below.
        const bool supportsScanCodes = lampArray->SupportsScanCodes();

        LampArrayPosition boundingBox = {};
        lampArray->GetBoundingBox(&boundingBox);

        // Device-level capabilities, read live from ILampArray getters.
        if (ImGui::BeginTable("##info", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGuiAtg::DrawNameValueTable("Kind", "%s", KindName(lampArray->GetLampArrayKind()));
            ImGuiAtg::DrawNameValueTable("VID / PID", "0x%04X / 0x%04X", lampArray->GetVendorId(), lampArray->GetProductId());
            ImGuiAtg::DrawNameValueTable("Hardware version", "0x%04X", lampArray->GetHardwareVersion());
            ImGuiAtg::DrawNameValueTable("Lamp count", "%u", device.lampCount);
            ImGuiAtg::DrawNameValueTable("Bounding box (mm)", "%.1f x %.1f x %.1f",
                boundingBox.xInMeters * 1000.0f, boundingBox.yInMeters * 1000.0f, boundingBox.zInMeters * 1000.0f);
            ImGuiAtg::DrawNameValueTable("Min update interval", "%llu us", lampArray->GetMinUpdateIntervalInMicroseconds());
            ImGuiAtg::DrawNameBoolValueTable("Enabled", lampArray->GetIsEnabled());
            ImGuiAtg::DrawNameValueTableColored("Status", StatusColor(device.status), "%s", StatusText(device.status));
            ImGuiAtg::DrawNameBoolValueTable("Supports scan codes", supportsScanCodes);
            ImGui::EndTable();
        }

        DrawLampInspector(device, lampArray);

        ImGui::Spacing();

        // Disable the controls when we do not control the lighting; our writes
        // would be ignored (see the Status row).
        const bool available = IsAvailable(device.status);
        ImGui::BeginDisabled(!available);

        ImGui::PushItemWidth(300.0f);

        // Effect selector. The scan-code- and lamp-count-dependent effects are
        // offered only on devices that support them, queried above.
        const auto currentIndex = static_cast<size_t>(device.effect);
        if (ImGui::BeginCombo("##Effect", LightingEffects::Name(device.effect), ImGuiComboFlags_HeightLarge))
        {
            for (size_t i = 0; i < static_cast<size_t>(Effect::Count); ++i)
            {
                // WASD and Reactive light physical keys, so they require scan codes.
                if ((static_cast<Effect>(i) == Effect::WASD || static_cast<Effect>(i) == Effect::Reactive) && !supportsScanCodes)
                {
                    continue;
                }

                // Manual exposes one picker per lamp, so it is offered only on the
                // low-lamp-count non-keyboard devices.
                if (static_cast<Effect>(i) == Effect::Manual && supportsScanCodes)
                {
                    continue;
                }

                const bool selected = (i == currentIndex);
                if (ImGui::Selectable(LightingEffects::Name(static_cast<Effect>(i)), selected) && !selected)
                {
                    device.effect = static_cast<Effect>(i);
                    device.effectChanged = true;        // Reset the effect's animation state before it next runs.
                }
                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 0)); ImGui::SameLine();
        ImGui::TextWrapped("%s", LightingEffects::Description(device.effect));

        const LightingEffects::EffectControls& controls = LightingEffects::Controls(device.effect);

        // Whole-device brightness, scaling whatever the effect outputs.
        if (ImGui::SliderFloat("##Brightness", &device.brightness, 0.0f, 1.0f, "Brightness %.2f"))
        {
            device.lampArray->SetBrightnessLevel(device.brightness);
        }

        if (controls.usesSpeed)
        {
            ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 0)); ImGui::SameLine();
            ImGui::SliderFloat("##Speed", &device.speedScale, 0.25f, 4.0f, "Speed %.2fx");
        }

        for (uint32_t i = 0; i < controls.colorCount; ++i)
        {
            ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 0)); ImGui::SameLine();

            // Solid and WASD are sent only when (re)applied, so a color change must
            // request a reapply; the animated effects re-read their color each frame.
            if (ImGui::ColorEdit3(controls.colorLabels[i], device.colors[currentIndex][i], ImGuiColorEditFlags_NoInputs))
            {
                if (device.effect == Effect::Solid || device.effect == Effect::WASD)
                {
                    device.effectChanged = true;
                }
            }
        }

        if (device.effect == Effect::Health)
        {
            ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 0)); ImGui::SameLine();
            ImGui::SliderFloat("##Health", &device.health, 0.0f, 1.0f, "Health %.2f");
        }

        // Manual: one color picker per lamp, under a single group label since a
        // per-swatch label will not fit the packed grid.
        if (device.effect == Effect::Manual)
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Colors");

            for (uint32_t i = 0; i < device.lampCount; ++i)
            {
                // Keep the first swatch on the label's line; wrap every 8.
                if (i == 0 || i % 8 != 0)
                {
                    ImGui::SameLine();
                }

                ImGui::PushID(static_cast<int>(i));
                ImGui::ColorEdit3("##lamp", device.manualColors[i].data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                ImGui::PopID();
            }
        }

        ImGui::PopItemWidth();
        ImGui::EndDisabled();
    }

    ImGui::Spacing();
    ImGui::PopID();
}

// Per-lamp inspector. A device can expose well over a hundred lamps, so the panel
// inspects one at a time. The details come from that lamp's ILampInfo: its
// position, the per-channel level counts that define how finely it can reproduce
// color, its purposes and scan code, and whether it has a fixed (non-settable)
// color.
void Sample::DrawLampInspector(LampArrayDevice& device, ILampArray* lampArray)
{
    if (!ImGui::TreeNode("Lamp inspector"))
    {
        return;
    }

    if (device.lampCount == 0)
    {
        ImGui::TextUnformatted("Device reports no lamps.");
        ImGui::TreePop();
        return;
    }

    int lamp = static_cast<int>(device.inspectorLamp);
    ImGui::PushItemWidth(300.0f);
    ImGui::SliderInt("##LampIndex", &lamp, 0, static_cast<int>(device.lampCount) - 1, "Lamp %d");
    ImGui::PopItemWidth();
    device.inspectorLamp = static_cast<uint32_t>(std::clamp(lamp, 0, static_cast<int>(device.lampCount) - 1));

    // GetLampInfo returns an ILampInfo describing a single lamp.
    ComPtr<ILampInfo> lampInfo;
    if (FAILED(lampArray->GetLampInfo(device.inspectorLamp, &lampInfo)) || !lampInfo)
    {
        ImGui::TextUnformatted("Lamp info unavailable.");
        ImGui::TreePop();
        return;
    }

    LampArrayPosition position = {};
    lampInfo->GetPosition(&position);

    // GetFixedColor returns true when the lamp's color is fixed in hardware (for
    // example an indicator LED); such a lamp ignores color writes. The per-channel
    // level counts (below) give how many discrete steps each channel supports -- a
    // low count means the lamp can only show a coarse palette.
    LampArrayColor fixedColor = {};
    const bool hasFixedColor = lampInfo->GetFixedColor(&fixedColor);

    if (ImGui::BeginTable("##lampinfo", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGuiAtg::DrawNameValueTable("Purposes", "%s", PurposesText(lampInfo->GetPurposes()).c_str());
        ImGuiAtg::DrawNameValueTable("Position (mm)", "%.1f, %.1f, %.1f",
            position.xInMeters * 1000.0f, position.yInMeters * 1000.0f, position.zInMeters * 1000.0f);
        ImGuiAtg::DrawNameValueTable("RGB level counts", "%u / %u / %u",
            lampInfo->GetRedLevelCount(), lampInfo->GetGreenLevelCount(), lampInfo->GetBlueLevelCount());
        ImGuiAtg::DrawNameValueTable("Gain level count", "%u", lampInfo->GetGainLevelCount());
        ImGuiAtg::DrawNameValueTable("Update latency", "%llu us", lampInfo->GetUpdateLatencyInMicroseconds());
        if (lampArray->SupportsScanCodes())
        {
            ImGuiAtg::DrawNameValueTable("Scan code", "0x%02X", lampInfo->GetScanCode());
        }
        if (hasFixedColor)
        {
            ImGuiAtg::DrawNameValueTable("Fixed color", "R%u G%u B%u", fixedColor.r, fixedColor.g, fixedColor.b);
        }
        else
        {
            ImGuiAtg::DrawNameValueTable("Fixed color", "%s", "None (color is settable)");
        }
        ImGui::EndTable();
    }

    ImGui::TreePop();
}

void Sample::Shutdown()
{
    // UnregisterLampArrayCallback waits (up to the timeout, in microseconds) for any
    // in-flight callback to finish, after which no callback can touch m_devices. It
    // returns false on timeout, so retry until it confirms before releasing.
    if (m_callbackToken != LAMPARRAY_INVALID_CALLBACK_TOKEN_VALUE)
    {
        while (!UnregisterLampArrayCallback(m_callbackToken, 5000000))
        {
        }
        m_callbackToken = LAMPARRAY_INVALID_CALLBACK_TOKEN_VALUE;
    }

    std::lock_guard<std::mutex> lock(m_devicesMutex);
    m_devices.clear();
}

void Sample::Activated()
{
}

void Sample::Deactivated()
{
}

LRESULT Sample::WndProcHandler(HWND /*hWnd*/, UINT msg, WPARAM /*wParam*/, LPARAM lParam)
{
    // Drive the Reactive effect from key input. A WM_KEYDOWN carries the keyboard
    // scan code in bits 16-23 of lParam -- the same scan-code space passed to
    // ILampArray::SetColorsForScanCodes -- so the pressed key can be lit directly
    // by scan code. We light it on every keyboard running Reactive and let the
    // effect fade it out. The message is not consumed (return 0).
    if (msg == WM_KEYDOWN)
    {
        const uint32_t scanCode = (static_cast<uint32_t>(lParam) >> 16) & 0xFF;

        std::lock_guard<std::mutex> lock(m_devicesMutex);
        for (auto& device : m_devices)
        {
            if (device->effect == Effect::Reactive)
            {
                device->keyIntensity[scanCode] = 1.0f;
            }
        }
    }

    return 0;
}

#ifdef _GAMING_XBOX
void Sample::Suspend(ImGuiAtg::DeviceContext* dc)
{
    dc->Suspend();
}

// On resume, force every device to reapply its effect: while the title was
// suspended the system or another app may have driven the lamps.
void Sample::Resume(ImGuiAtg::DeviceContext* dc)
{
    dc->Resume();

    std::lock_guard<std::mutex> lock(m_devicesMutex);
    for (auto& device : m_devices)
    {
        device->effectChanged = true;
    }
}
#endif
