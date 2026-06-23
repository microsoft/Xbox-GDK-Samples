//--------------------------------------------------------------------------------------
// File: GamePad.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GamePad.h"

using namespace InputController;
using Microsoft::WRL::ComPtr;


namespace
{
    constexpr float c_XboxOneThumbDeadZone = .24f;  // Recommended Xbox One controller deadzone

    float ApplyLinearDeadZone(float value, float maxValue, float deadZoneSize) noexcept
    {
        if (value < -deadZoneSize)
        {
            // Increase negative values to remove the deadzone discontinuity.
            value += deadZoneSize;
        }
        else if (value > deadZoneSize)
        {
            // Decrease positive values to remove the deadzone discontinuity.
            value -= deadZoneSize;
        }
        else
        {
            // Values inside the deadzone come out zero.
            return 0;
        }

        // Scale into 0-1 range.
        const float scaledValue = value / (maxValue - deadZoneSize);
        return std::max(-1.f, std::min(scaledValue, 1.f));
    }

    void ApplyStickDeadZone(
        float x,
        float y,
        GamePad::DeadZone deadZoneMode,
        float maxValue,
        float deadZoneSize,
        _Out_ float& resultX,
        _Out_ float& resultY) noexcept
    {
        switch (deadZoneMode)
        {
        case GamePad::DEAD_ZONE_INDEPENDENT_AXES:
            resultX = ApplyLinearDeadZone(x, maxValue, deadZoneSize);
            resultY = ApplyLinearDeadZone(y, maxValue, deadZoneSize);
            break;

        case GamePad::DEAD_ZONE_CIRCULAR:
            {
                const float dist = sqrtf(x*x + y * y);
                const float wanted = ApplyLinearDeadZone(dist, maxValue, deadZoneSize);

                const float scale = (wanted > 0.f) ? (wanted / dist) : 0.f;

                resultX = std::max(-1.f, std::min(x * scale, 1.f));
                resultY = std::max(-1.f, std::min(y * scale, 1.f));
            }
            break;

        default: // GamePad::DEAD_ZONE_NONE
            resultX = ApplyLinearDeadZone(x, maxValue, 0);
            resultY = ApplyLinearDeadZone(y, maxValue, 0);
            break;
        }
    }
}


#pragma region Implementations

//======================================================================================
// GameInput
//======================================================================================

class GamePad::Impl
{
public:
    Impl(GamePad* owner) :
        mOwner(owner),
        mCtrlChanged(INVALID_HANDLE_VALUE),
        mDeviceToken(0),
        mMostRecentGamepad(0)
    {
        if (s_gamePad)
        {
            throw std::logic_error("GamePad is a singleton");
        }

        s_gamePad = this;

        HRESULT hr = GameInputCreate(mGameInput.GetAddressOf());
        //if (SUCCEEDED(hr))
        //{
        //    ThrowIfFailed(mGameInput->RegisterDeviceCallback(
        //        nullptr,
        //        GameInputKindGamepad,
        //        GameInputDeviceConnected,
        //        GameInputBlockingEnumeration,
        //        this,
        //        OnGameInputDevice,
        //        &mDeviceToken));
        //}
        //else
        //{
        //    DebugTrace("ERROR: GameInputCreate [gamepad] failed with %08X\n", static_cast<unsigned int>(hr));
        //#ifdef _GAMING_XBOX
        //    ThrowIfFailed(hr);
        //#elif defined(_DEBUG)
        //    DebugTrace(
        //        "\t**** Check that the 'GameInput Service' is running on this system.    ****\n"
        //        "\t**** NOTE: All calls to GetState will be reported as 'not connected'. ****\n");
        //#endif
        //}
    }

    Impl(Impl&&) = default;
    Impl& operator= (Impl&&) = default;

    Impl(Impl const&) = delete;
    Impl& operator= (Impl const&) = delete;

    ~Impl()
    {
        if (mDeviceToken)
        {
            if (mGameInput)
            {
                if (!mGameInput->UnregisterCallback(mDeviceToken, UINT64_MAX))
                {
                    //DebugTrace("ERROR: GameInput::UnregisterCallback [gamepad] failed");
                }
            }

            mDeviceToken = 0;
        }

        s_gamePad = nullptr;
    }

    void GetState(int player, _Out_ State& state, DeadZone deadZoneMode)
    {
        memset(&state, 0, sizeof(State));

        IGameInputDevice* device = nullptr;

        if (player >= 0 && player < MAX_PLAYER_COUNT)
        {
            device = mInputDevices[player].Get();
            if (!device)
                return;

            assert(mGameInput != nullptr);
        }
        else if (player == c_MostRecent)
        {
            player = mMostRecentGamepad;
            assert(player >= 0 && player < MAX_PLAYER_COUNT);
            device = mInputDevices[player].Get();
            if (!device)
                return;

            assert(mGameInput != nullptr);
        }
        else if (player != c_MergedInput || !mGameInput)
        {
            return;
        }

        ComPtr<IGameInputReading> reading;
        if (SUCCEEDED(mGameInput->GetCurrentReading(GameInputKindGamepad, device, reading.GetAddressOf())))
        {
            GameInputGamepadState pad;
            if (reading->GetGamepadState(&pad))
            {
                state.connected = true;
                state.packet = reading->GetSequenceNumber(GameInputKindGamepad);

                state.buttons.a = (pad.buttons & GameInputGamepadA) != 0;
                state.buttons.b = (pad.buttons & GameInputGamepadB) != 0;
                state.buttons.x = (pad.buttons & GameInputGamepadX) != 0;
                state.buttons.y = (pad.buttons & GameInputGamepadY) != 0;
                state.buttons.leftStick = (pad.buttons & GameInputGamepadLeftThumbstick) != 0;
                state.buttons.rightStick = (pad.buttons & GameInputGamepadRightThumbstick) != 0;
                state.buttons.leftShoulder = (pad.buttons & GameInputGamepadLeftShoulder) != 0;
                state.buttons.rightShoulder = (pad.buttons & GameInputGamepadRightShoulder) != 0;
                state.buttons.view = (pad.buttons & GameInputGamepadView) != 0;
                state.buttons.menu = (pad.buttons & GameInputGamepadMenu) != 0;

                state.dpad.up = (pad.buttons & GameInputGamepadDPadUp) != 0;
                state.dpad.down = (pad.buttons & GameInputGamepadDPadDown) != 0;
                state.dpad.right = (pad.buttons & GameInputGamepadDPadRight) != 0;
                state.dpad.left = (pad.buttons & GameInputGamepadDPadLeft) != 0;

                ApplyStickDeadZone(pad.leftThumbstickX, pad.leftThumbstickY,
                    deadZoneMode, 1.f, c_XboxOneThumbDeadZone,
                    state.thumbSticks.leftX, state.thumbSticks.leftY);

                ApplyStickDeadZone(pad.rightThumbstickX, pad.rightThumbstickY,
                    deadZoneMode, 1.f, c_XboxOneThumbDeadZone,
                    state.thumbSticks.rightX, state.thumbSticks.rightY);

                state.triggers.left = pad.leftTrigger;
                state.triggers.right = pad.rightTrigger;
            }
        }
    }

    void GetCapabilities(int player, _Out_ Capabilities& caps)
    {
        if (player == c_MostRecent)
            player = mMostRecentGamepad;

        if (player >= 0 && player < MAX_PLAYER_COUNT)
        {
            IGameInputDevice* device = mInputDevices[player].Get();
            if (device)
            {
                if (device->GetDeviceStatus() & GameInputDeviceConnected)
                {
                    auto deviceInfo = device->GetDeviceInfo();
                    caps.connected = true;
                    caps.gamepadType = Capabilities::GAMEPAD;
                    caps.id = deviceInfo->deviceId;
                    caps.vid = deviceInfo->vendorId;
                    caps.pid = deviceInfo->productId;
                    return;
                }
                else
                {
                    mInputDevices[player].Reset();
                }
            }
        }

        memset(&caps, 0, sizeof(Capabilities));
    }

    bool SetVibration(int player, float leftMotor, float rightMotor, float leftTrigger, float rightTrigger) noexcept
    {
        if (player == c_MostRecent)
            player = mMostRecentGamepad;

        if (player >= 0 && player < MAX_PLAYER_COUNT)
        {
            IGameInputDevice* device = mInputDevices[player].Get();
            if (device)
            {
                GameInputRumbleParams const params =
                {
                    leftMotor,
                    rightMotor,
                    leftTrigger,
                    rightTrigger
                };

                device->SetRumbleState(&params);
                return true;
            }
        }

        return false;
    }

    void Suspend() noexcept
    {
        for (int player = 0; player < MAX_PLAYER_COUNT; ++player)
        {
            IGameInputDevice* device = mInputDevices[player].Get();
            if (device)
            {
                device->SetRumbleState(nullptr);
            }
        }
    }

    void Resume() noexcept
    {
        for (int player = 0; player < MAX_PLAYER_COUNT; ++player)
        {
            IGameInputDevice* device = mInputDevices[player].Get();
            if (device)
            {
                if (!(device->GetDeviceStatus() & GameInputDeviceConnected))
                {
                    mInputDevices[player].Reset();
                }
            }
        }
    }

    _Success_(return)
        bool GetDevice(int player, _Outptr_ IGameInputDevice** device) noexcept
    {
        if (!device)
            return false;

        if (player == c_MostRecent)
            player = mMostRecentGamepad;

        *device = nullptr;

        if (player >= 0 && player < MAX_PLAYER_COUNT)
        {
            IGameInputDevice* dev = mInputDevices[player].Get();
            if (dev)
            {
                dev->AddRef();
                *device = dev;
                return true;
            }
        }

        return false;
    }

    GamePad*    mOwner;

    static GamePad::Impl* s_gamePad;

    HANDLE mCtrlChanged;

private:
    ComPtr<IGameInput>          mGameInput;
    ComPtr<IGameInputDevice>    mInputDevices[MAX_PLAYER_COUNT];

    GameInputCallbackToken      mDeviceToken;

    int mMostRecentGamepad;

    static void CALLBACK OnGameInputDevice(
        _In_ GameInputCallbackToken,
        _In_ void * context,
        _In_ IGameInputDevice * device,
        _In_ uint64_t,
        _In_ GameInputDeviceStatus currentStatus,
        _In_ GameInputDeviceStatus) noexcept
    {
        auto impl = reinterpret_cast<GamePad::Impl*>(context);

        if (currentStatus & GameInputDeviceConnected)
        {
            size_t empty = MAX_PLAYER_COUNT;
            size_t k = 0;
            for (; k < MAX_PLAYER_COUNT; ++k)
            {
                if (impl->mInputDevices[k].Get() == device)
                {
                    impl->mMostRecentGamepad = static_cast<int>(k);
                    break;
                }
                else if (!impl->mInputDevices[k])
                {
                    if (empty >= MAX_PLAYER_COUNT)
                        empty = k;
                }
            }

            if (k >= MAX_PLAYER_COUNT)
            {
                // Silently ignore "extra" gamepads as there's no hard limit
                if (empty < MAX_PLAYER_COUNT)
                {
                    impl->mInputDevices[empty] = device;
                    impl->mMostRecentGamepad = static_cast<int>(empty);
                }
            }
        }
        else
        {
            for (size_t k = 0; k < MAX_PLAYER_COUNT; ++k)
            {
                if (impl->mInputDevices[k].Get() == device)
                {
                    impl->mInputDevices[k].Reset();
                    break;
                }
            }
        }

        if (impl->mCtrlChanged != INVALID_HANDLE_VALUE)
        {
            SetEvent(impl->mCtrlChanged);
        }
    }
};

GamePad::Impl* GamePad::Impl::s_gamePad = nullptr;

void GamePad::RegisterEvents(HANDLE ctrlChanged) noexcept
{
    pImpl->mCtrlChanged = (!ctrlChanged) ? INVALID_HANDLE_VALUE : ctrlChanged;
}

_Success_(return)
bool GamePad::GetDevice(int player, _Outptr_ IGameInputDevice * *device) noexcept
{
    return pImpl->GetDevice(player, device);
}

#pragma endregion

#ifdef _MSC_VER
#pragma warning( disable : 4355 )
#endif

// Public constructor.
GamePad::GamePad() noexcept(false)
    : pImpl(std::make_unique<Impl>(this))
{
}


// Move constructor.
GamePad::GamePad(GamePad&& moveFrom) noexcept
    : pImpl(std::move(moveFrom.pImpl))
{
    pImpl->mOwner = this;
}


// Move assignment.
GamePad& GamePad::operator= (GamePad&& moveFrom) noexcept
{
    pImpl = std::move(moveFrom.pImpl);
    pImpl->mOwner = this;
    return *this;
}


// Public destructor.
GamePad::~GamePad() = default;


GamePad::State GamePad::GetState(int player, DeadZone deadZoneMode)
{
    State state;
    pImpl->GetState(player, state, deadZoneMode);
    return state;
}


GamePad::Capabilities GamePad::GetCapabilities(int player)
{
    Capabilities caps;
    pImpl->GetCapabilities(player, caps);
    return caps;
}


bool GamePad::SetVibration(int player, float leftMotor, float rightMotor, float leftTrigger, float rightTrigger) noexcept
{
    return pImpl->SetVibration(player, leftMotor, rightMotor, leftTrigger, rightTrigger);
}


void GamePad::Suspend() noexcept
{
    pImpl->Suspend();
}


void GamePad::Resume() noexcept
{
    pImpl->Resume();
}


GamePad& GamePad::Get()
{
    if (!Impl::s_gamePad || !Impl::s_gamePad->mOwner)
        throw std::logic_error("GamePad singleton not created");

    return *Impl::s_gamePad->mOwner;
}



//======================================================================================
// ButtonStateTracker
//======================================================================================

#define UPDATE_BUTTON_STATE(field) field = static_cast<ButtonState>( ( !!state.buttons.field ) | ( ( !!state.buttons.field ^ !!lastState.buttons.field ) << 1 ) )

void GamePad::ButtonStateTracker::Update(const GamePad::State& state) noexcept
{
    UPDATE_BUTTON_STATE(a);

    assert((!state.buttons.a && !lastState.buttons.a) == (a == UP));
    assert((state.buttons.a && lastState.buttons.a) == (a == HELD));
    assert((!state.buttons.a && lastState.buttons.a) == (a == RELEASED));
    assert((state.buttons.a && !lastState.buttons.a) == (a == PRESSED));

    UPDATE_BUTTON_STATE(b);
    UPDATE_BUTTON_STATE(x);
    UPDATE_BUTTON_STATE(y);

    UPDATE_BUTTON_STATE(leftStick);
    UPDATE_BUTTON_STATE(rightStick);

    UPDATE_BUTTON_STATE(leftShoulder);
    UPDATE_BUTTON_STATE(rightShoulder);

    UPDATE_BUTTON_STATE(back);
    UPDATE_BUTTON_STATE(start);

    dpadUp = static_cast<ButtonState>((!!state.dpad.up) | ((!!state.dpad.up ^ !!lastState.dpad.up) << 1));
    dpadDown = static_cast<ButtonState>((!!state.dpad.down) | ((!!state.dpad.down ^ !!lastState.dpad.down) << 1));
    dpadLeft = static_cast<ButtonState>((!!state.dpad.left) | ((!!state.dpad.left ^ !!lastState.dpad.left) << 1));
    dpadRight = static_cast<ButtonState>((!!state.dpad.right) | ((!!state.dpad.right ^ !!lastState.dpad.right) << 1));

    assert((!state.dpad.up && !lastState.dpad.up) == (dpadUp == UP));
    assert((state.dpad.up && lastState.dpad.up) == (dpadUp == HELD));
    assert((!state.dpad.up && lastState.dpad.up) == (dpadUp == RELEASED));
    assert((state.dpad.up && !lastState.dpad.up) == (dpadUp == PRESSED));

    // Handle 'threshold' tests which emulate buttons

    bool threshold = state.IsLeftThumbStickUp();
    leftStickUp = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsLeftThumbStickUp()) << 1));

    threshold = state.IsLeftThumbStickDown();
    leftStickDown = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsLeftThumbStickDown()) << 1));

    threshold = state.IsLeftThumbStickLeft();
    leftStickLeft = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsLeftThumbStickLeft()) << 1));

    threshold = state.IsLeftThumbStickRight();
    leftStickRight = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsLeftThumbStickRight()) << 1));

    threshold = state.IsRightThumbStickUp();
    rightStickUp = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsRightThumbStickUp()) << 1));

    threshold = state.IsRightThumbStickDown();
    rightStickDown = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsRightThumbStickDown()) << 1));

    threshold = state.IsRightThumbStickLeft();
    rightStickLeft = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsRightThumbStickLeft()) << 1));

    threshold = state.IsRightThumbStickRight();
    rightStickRight = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsRightThumbStickRight()) << 1));

    threshold = state.IsLeftTriggerPressed();
    leftTrigger = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsLeftTriggerPressed()) << 1));

    threshold = state.IsRightTriggerPressed();
    rightTrigger = static_cast<ButtonState>((!!threshold) | ((!!threshold ^ !!lastState.IsRightTriggerPressed()) << 1));

    lastState = state;
}

#undef UPDATE_BUTTON_STATE


void GamePad::ButtonStateTracker::Reset() noexcept
{
    memset(this, 0, sizeof(ButtonStateTracker));
}
