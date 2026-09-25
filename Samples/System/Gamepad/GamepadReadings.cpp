//--------------------------------------------------------------------------------------
// GamepadReadings.cpp
//
// Polling and callback-based GameInput reading.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Gamepad.h"

using Microsoft::WRL::ComPtr;

namespace
{
    void DrawCenteredText(ImDrawList* drawList, float x, float y, float elementWidth, const char* text, ImU32 color);
    void DrawButtonGlyph(const char* fallbackLabel, GameInputLabel label, bool pressed, float glyphSize);
    void DrawVerticalTrigger(const char* label, float value, float barWidth, float barHeight);
    void DrawThumbstick(const char* label, float x, float y, float stickSize);
}

//--------------------------------------------------------------------------------------
// PollGamepadReading - default polling mode when callback mode is not enabled
//
// Passing a device filters GetCurrentReading to that gamepad; nullptr would return the
// latest matching reading from any device.
//--------------------------------------------------------------------------------------
void Sample::PollGamepadReading(GamepadDevice& gamepad)
{
    ComPtr<IGameInputReading> reading;

    HRESULT hr = m_gameInput->GetCurrentReading(
        GameInputKindGamepad | GameInputKindSensors,
        gamepad.device.Get(),
        &reading);

    if (SUCCEEDED(hr) && reading)
    {
        gamepad.hasGamepadState = reading->GetGamepadState(&gamepad.gamepadState);
        gamepad.hasSensorsState = reading->GetSensorsState(&gamepad.sensorsState);
        gamepad.readingTimestamp = reading->GetTimestamp();
    }
    else
    {
        gamepad.hasGamepadState = false;
        gamepad.hasSensorsState = false;
    }
}

//--------------------------------------------------------------------------------------
// Reading Callback Mode - only used when callback mode is enabled
//
// Reading callbacks run on a GameInput worker thread when new input arrives.
//--------------------------------------------------------------------------------------
void CALLBACK Sample::OnReadingChanged(
    GameInputCallbackToken /*token*/,
    void* context,
    IGameInputReading* reading) noexcept
{
    auto* sample = static_cast<Sample*>(context);

    ComPtr<IGameInputDevice> device;
    reading->GetDevice(&device);

    std::lock_guard<std::mutex> lock(sample->m_gamepadsMutex);

    for (auto& gamepad : sample->m_gamepads)
    {
        if (gamepad.device.Get() == device.Get())
        {
            gamepad.hasGamepadState = reading->GetGamepadState(&gamepad.gamepadState);
            gamepad.hasSensorsState = reading->GetSensorsState(&gamepad.sensorsState);
            gamepad.readingTimestamp = reading->GetTimestamp();
            break;
        }
    }
}

//--------------------------------------------------------------------------------------
// RegisterReadingCallbacks - switch to callback-driven input
//
// A nullptr device registers one callback for all matching devices. The returned token
// unregisters that single callback.
//--------------------------------------------------------------------------------------
bool Sample::RegisterReadingCallbacks()
{
    if (!m_gameInput)
        return false;

    if (m_readingCallbackToken == 0)
    {
        HRESULT hr = m_gameInput->RegisterReadingCallback(
            nullptr,
            GameInputKindGamepad | GameInputKindSensors,
            this,
            OnReadingChanged,
            &m_readingCallbackToken);

        if (FAILED(hr))
        {
            ImGuiAtg::Log("Failed to register reading callback: %08X\n", static_cast<unsigned int>(hr));
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------
// UnregisterReadingCallbacks - switch back to polling mode
//
// UnregisterCallback waits for in-flight callbacks before returning.
//--------------------------------------------------------------------------------------
void Sample::UnregisterReadingCallbacks()
{
    if (!m_gameInput)
        return;

    if (m_readingCallbackToken != 0)
    {
        m_gameInput->UnregisterCallback(m_readingCallbackToken);
        m_readingCallbackToken = 0;
    }
}

//--------------------------------------------------------------------------------------
// Standard gamepad buttons, paddles, and separately reported system buttons.
//--------------------------------------------------------------------------------------
void Sample::DrawButtonsSection(const GamepadDevice& gamepad)
{
    const auto& state = gamepad.gamepadState;
    const auto* gamepadInfo = gamepad.deviceInfo->gamepadInfo;

    if (gamepadInfo && ImGui::CollapsingHeader("Buttons", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
    {
        float glyphSize = ImGui::GetFontSize() * 1.5f;
        float spacing = ImGuiAtg::Scaled(8);

        DrawButtonGlyph("A",         gamepadInfo->aButtonLabel,    state.buttons & GameInputGamepadA,         glyphSize); ImGui::SameLine(0.0f, spacing);
        DrawButtonGlyph("B",         gamepadInfo->bButtonLabel,    state.buttons & GameInputGamepadB,         glyphSize); ImGui::SameLine(0.0f, spacing);
        DrawButtonGlyph("X",         gamepadInfo->xButtonLabel,    state.buttons & GameInputGamepadX,         glyphSize); ImGui::SameLine(0.0f, spacing);
        DrawButtonGlyph("Y",         gamepadInfo->yButtonLabel,    state.buttons & GameInputGamepadY,         glyphSize); ImGui::SameLine(0.0f, spacing);
        DrawButtonGlyph("DPad Up",   gamepadInfo->dpadUpLabel,     state.buttons & GameInputGamepadDPadUp,    glyphSize); ImGui::SameLine(0.0f, spacing);
        DrawButtonGlyph("DPad Down", gamepadInfo->dpadDownLabel,   state.buttons & GameInputGamepadDPadDown,  glyphSize); ImGui::SameLine(0.0f, spacing);
        DrawButtonGlyph("DPad Left", gamepadInfo->dpadLeftLabel,   state.buttons & GameInputGamepadDPadLeft,  glyphSize); ImGui::SameLine(0.0f, spacing);
        DrawButtonGlyph("DPad Right",gamepadInfo->dpadRightLabel,  state.buttons & GameInputGamepadDPadRight, glyphSize);

        DrawButtonGlyph("LB",     gamepadInfo->leftShoulderButtonLabel,    state.buttons & GameInputGamepadLeftShoulder,    glyphSize); ImGui::SameLine(0.0f, spacing);
        DrawButtonGlyph("RB",     gamepadInfo->rightShoulderButtonLabel,   state.buttons & GameInputGamepadRightShoulder,   glyphSize); ImGui::SameLine(0.0f, spacing);
        DrawButtonGlyph("LStick", gamepadInfo->leftThumbstickButtonLabel,  state.buttons & GameInputGamepadLeftThumbstick,  glyphSize); ImGui::SameLine(0.0f, spacing);
        DrawButtonGlyph("RStick", gamepadInfo->rightThumbstickButtonLabel, state.buttons & GameInputGamepadRightThumbstick, glyphSize); ImGui::SameLine(0.0f, spacing);
        DrawButtonGlyph("View",   gamepadInfo->viewButtonLabel,            state.buttons & GameInputGamepadView,            glyphSize); ImGui::SameLine(0.0f, spacing);
        DrawButtonGlyph("Menu",   gamepadInfo->menuButtonLabel,            state.buttons & GameInputGamepadMenu,            glyphSize); ImGui::SameLine(0.0f, spacing);

        if (gamepad.deviceInfo->supportedSystemButtons & GameInputSystemButtonGuide)
        {
            DrawButtonGlyph("Guide", GameInputLabelGuide, (gamepad.systemButtons & GameInputSystemButtonGuide) != 0, glyphSize); ImGui::SameLine(0.0f, spacing);
        }
        if (gamepad.deviceInfo->supportedSystemButtons & GameInputSystemButtonShare)
        {
            DrawButtonGlyph("Share", GameInputLabelShare, (gamepad.systemButtons & GameInputSystemButtonShare) != 0, glyphSize); ImGui::SameLine(0.0f, spacing);
        }

        DrawButtonGlyph("PL1", GameInputLabelPaddleLeft1,  state.buttons & GameInputGamepadPaddleLeft1,  glyphSize); ImGui::SameLine(0.0f, spacing);
        DrawButtonGlyph("PL2", GameInputLabelPaddleLeft2,  state.buttons & GameInputGamepadPaddleLeft2,  glyphSize); ImGui::SameLine(0.0f, spacing);
        DrawButtonGlyph("PR1", GameInputLabelPaddleRight1, state.buttons & GameInputGamepadPaddleRight1, glyphSize); ImGui::SameLine(0.0f, spacing);
        DrawButtonGlyph("PR2", GameInputLabelPaddleRight2, state.buttons & GameInputGamepadPaddleRight2, glyphSize);
    }
}

//--------------------------------------------------------------------------------------
// Triggers use [0, 1]; thumbstick axes use [-1, 1].
//--------------------------------------------------------------------------------------
void Sample::DrawAnalogSection(const GameInputGamepadState& state)
{
    if (ImGui::CollapsingHeader("Analog Inputs", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
    {
        float stickSize = ImGuiAtg::Scaled(80);
        float barWidth = ImGuiAtg::Scaled(24);
        float gap = ImGuiAtg::Scaled(42);

        ImGui::Indent(ImGuiAtg::Scaled(12));
        DrawVerticalTrigger("LT", state.leftTrigger, barWidth, stickSize);
        ImGui::SameLine(0.0f, gap);
        DrawThumbstick("Left Stick", state.leftThumbstickX, state.leftThumbstickY, stickSize);
        ImGui::SameLine(0.0f, gap);
        DrawThumbstick("Right Stick", state.rightThumbstickX, state.rightThumbstickY, stickSize);
        ImGui::SameLine(0.0f, gap);
        DrawVerticalTrigger("RT", state.rightTrigger, barWidth, stickSize);
        ImGui::Unindent(ImGuiAtg::Scaled(12));
    }
}

//--------------------------------------------------------------------------------------
// sensorsInfo is present when supportedInput includes GameInputKindSensors.
//--------------------------------------------------------------------------------------
void Sample::DrawSensorsSection(const GamepadDevice& gamepad)
{
    if (ImGui::CollapsingHeader("Sensors", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
    {
        if (!(gamepad.deviceInfo->supportedInput & GameInputKindSensors))
        {
            ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Sensors not supported by this device.");
            return;
        }

        if (ImGui::BeginTable("Sensors", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Sensor Kind", ImGuiTableColumnFlags_WidthFixed, ImGuiAtg::Scaled(180));
            const float valueColumnWidth = ImGui::CalcTextSize("-23.567").x + ImGuiAtg::Scaled(4);
            ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed, valueColumnWidth);
            ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthFixed, valueColumnWidth);
            ImGui::TableSetupColumn("Z", ImGuiTableColumnFlags_WidthFixed, valueColumnWidth);
            ImGui::TableSetupColumn("W", ImGuiTableColumnFlags_WidthFixed, valueColumnWidth);
            ImGui::TableHeadersRow();

            const auto& state = gamepad.sensorsState;
            const auto supportedSensors = gamepad.deviceInfo->sensorsInfo->supportedSensors;

            if (supportedSensors & GameInputSensorsAccelerometer)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Acceleration (G)");
                ImGui::TableNextColumn();
                ImGui::Text("% 7.3f", state.accelerationInGX);
                ImGui::TableNextColumn();
                ImGui::Text("% 7.3f", state.accelerationInGY);
                ImGui::TableNextColumn();
                ImGui::Text("% 7.3f", state.accelerationInGZ);
            }

            if (supportedSensors & GameInputSensorsGyrometer)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Gyroscope (rad/s)");
                ImGui::TableNextColumn();
                ImGui::Text("% 7.3f", state.angularVelocityInRadPerSecX);
                ImGui::TableNextColumn();
                ImGui::Text("% 7.3f", state.angularVelocityInRadPerSecY);
                ImGui::TableNextColumn();
                ImGui::Text("% 7.3f", state.angularVelocityInRadPerSecZ);
            }

            if (supportedSensors & GameInputSensorsCompass)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Heading (degrees)");
                ImGui::TableNextColumn();
                ImGui::Text("% 7.3f", state.headingInDegreesFromMagneticNorth);
            }

            if (supportedSensors & GameInputSensorsOrientation)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Orientation");
                ImGui::TableNextColumn();
                ImGui::Text("% 7.3f", state.orientationX);
                ImGui::TableNextColumn();
                ImGui::Text("% 7.3f", state.orientationY);
                ImGui::TableNextColumn();
                ImGui::Text("% 7.3f", state.orientationZ);
                ImGui::TableNextColumn();
                ImGui::Text("% 7.3f", state.orientationW);
            }

            ImGui::EndTable();
        }
    }
}

namespace
{
    void DrawCenteredText(ImDrawList* drawList, float x, float y, float elementWidth, const char* text, ImU32 color)
    {
        ImVec2 textSize = ImGui::CalcTextSize(text);
        drawList->AddText(ImVec2(x + (elementWidth - textSize.x) * 0.5f, y), color, text);
    }

    void DrawButtonGlyph(const char* fallbackLabel, GameInputLabel label, bool pressed, float glyphSize)
    {
        if (pressed)
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
        ImGuiAtg::Glyph(label, glyphSize, fallbackLabel);
        if (pressed)
            ImGui::PopStyleColor();
    }

    void DrawVerticalTrigger(const char* label, float value, float barWidth, float barHeight)
    {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
        ImU32 backgroundColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
        ImU32 borderColor = ImGui::GetColorU32(ImGuiCol_Border);
        ImU32 fillColor = ImGui::GetColorU32(ImGuiCol_SliderGrab);
        float lineHeight = ImGui::GetTextLineHeight();
        float padding = ImGuiAtg::Scaled(1);
        float gap = ImGuiAtg::Scaled(2);

        drawList->AddRectFilled(pos, ImVec2(pos.x + barWidth, pos.y + barHeight), backgroundColor);
        drawList->AddRect(pos, ImVec2(pos.x + barWidth, pos.y + barHeight), borderColor);

        float fillHeight = barHeight * value;
        drawList->AddRectFilled(
            ImVec2(pos.x + padding, pos.y + barHeight - fillHeight),
            ImVec2(pos.x + barWidth - padding, pos.y + barHeight),
            fillColor);

        float textY = pos.y + barHeight + gap;
        DrawCenteredText(drawList, pos.x, textY, barWidth, label, textColor);
        textY += lineHeight;
        char valueText[16];
        snprintf(valueText, sizeof(valueText), "%.2f", value);
        DrawCenteredText(drawList, pos.x, textY, barWidth, valueText, textColor);

        ImGui::Dummy(ImVec2(barWidth, barHeight + lineHeight * 2 + gap * 2));
    }

    void DrawThumbstick(const char* label, float x, float y, float stickSize)
    {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 center(pos.x + stickSize * 0.5f, pos.y + stickSize * 0.5f);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
        ImU32 backgroundColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
        ImU32 borderColor = ImGui::GetColorU32(ImGuiCol_Border);
        ImU32 guideColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        ImU32 dotColor = ImGui::GetColorU32(ImGuiCol_SliderGrab);
        float lineHeight = ImGui::GetTextLineHeight();
        float gap = ImGuiAtg::Scaled(2);

        drawList->AddCircleFilled(center, stickSize * 0.5f, backgroundColor);
        drawList->AddCircle(center, stickSize * 0.5f, borderColor);

        drawList->AddLine(ImVec2(center.x - stickSize * 0.4f, center.y),
            ImVec2(center.x + stickSize * 0.4f, center.y), guideColor);
        drawList->AddLine(ImVec2(center.x, center.y - stickSize * 0.4f),
            ImVec2(center.x, center.y + stickSize * 0.4f), guideColor);

        ImVec2 dot(center.x + x * stickSize * 0.4f, center.y - y * stickSize * 0.4f);
        drawList->AddCircleFilled(dot, ImGuiAtg::Scaled(4), dotColor);

        float textY = pos.y + stickSize + gap;
        DrawCenteredText(drawList, pos.x, textY, stickSize, label, textColor);
        textY += lineHeight;
        char xText[24];
        char yText[24];
        snprintf(xText, sizeof(xText), "X: %5.2f", x);
        snprintf(yText, sizeof(yText), "Y: %5.2f", y);
        DrawCenteredText(drawList, pos.x, textY, stickSize, xText, textColor);
        textY += lineHeight;
        DrawCenteredText(drawList, pos.x, textY, stickSize, yText, textColor);

        ImGui::Dummy(ImVec2(stickSize, stickSize + lineHeight * 3 + gap * 2));
    }
}
