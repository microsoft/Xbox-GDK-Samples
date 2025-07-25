#include <wrl.h>
#include <GameInput.h>
#if GAMEINPUT_API_VERSION == 1
using namespace GameInput::v1;
#endif

using Microsoft::WRL::ComPtr;

namespace
{
    enum class ActiveInputType
    {
        Unknown,
        Keyboard,
        Mouse,
        Gamepad
    };

    static GameInputGamepadState lastGamepadState{};
    static uint64_t lastGamepadTimestamp = 0;
};

bool AxisChanged(float val, float lastVal, float threshold)
{
    if(val > lastVal + threshold ||
       val < lastVal - threshold)
    {
        return true;
    }
    return false;
}

ActiveInputType GetActiveInputType(IGameInput* gameInput)
{
    ComPtr<IGameInputReading> gpReading, kbReading, msReading;
    uint64_t gpStamp = 0, kbStamp = 0, msStamp = 0;
    HRESULT hr;

    hr = gameInput->GetCurrentReading(GameInputKindGamepad, nullptr, &gpReading);
    if(SUCCEEDED(hr))
    {
        GameInputGamepadState state{};
        gpReading->GetGamepadState(&state);

        // only use timestamp if the analogs passed some thresholds, helps eliminate false positives due to analog jitter
        if(AxisChanged(state.leftThumbstickX,  lastGamepadState.leftThumbstickX,  0.05) ||
           AxisChanged(state.leftThumbstickY,  lastGamepadState.leftThumbstickY,  0.05) ||
           AxisChanged(state.rightThumbstickX, lastGamepadState.rightThumbstickX, 0.05) ||
           AxisChanged(state.rightThumbstickY, lastGamepadState.rightThumbstickY, 0.05) ||
           AxisChanged(state.leftTrigger,      lastGamepadState.leftTrigger,      0.1)  ||
           AxisChanged(state.rightTrigger,     lastGamepadState.rightTrigger,     0.1)  ||
           state.buttons != lastGamepadState.buttons)
        {
            gpStamp = gpReading->GetTimestamp();
            lastGamepadTimestamp = gpStamp;
        }
        else
        {
            gpStamp = lastGamepadTimestamp;
        }

        lastGamepadState = state;
    }

    hr = gameInput->GetCurrentReading(GameInputKindKeyboard, nullptr, &kbReading);
    kbStamp = SUCCEEDED(hr) && kbReading ? kbReading->GetTimestamp() : 0;

    hr = gameInput->GetCurrentReading(GameInputKindMouse, nullptr, &msReading);
    msStamp = SUCCEEDED(hr) && msReading ? msReading->GetTimestamp() : 0;

    if(gpStamp > msStamp && gpStamp > kbStamp)
    {
        return ActiveInputType::Gamepad;
    }
    else if(msStamp > kbStamp && msStamp > gpStamp)
    {
        return ActiveInputType::Mouse;
    }
    else if(kbStamp > msStamp && kbStamp > gpStamp)
    {
        return ActiveInputType::Keyboard;
    }

    return ActiveInputType::Unknown;
}

