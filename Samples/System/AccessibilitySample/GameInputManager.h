//--------------------------------------------------------------------------------------
// GameInputManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <GameInput.h>
#if GAMEINPUT_API_VERSION == 1
using namespace GameInput::v1;
#elif GAMEINPUT_API_VERSION == 2
using namespace GameInput::v2;
#elif GAMEINPUT_API_VERSION == 3
using namespace GameInput::v3;
#endif

class GameInputManager
{
public:
    static bool Init();
    static void Shutdown();
    static GameInputKind GetActiveGameInputKind();

private:
    static void CALLBACK DeviceCallback(
        GameInputCallbackToken token,
        void*,
        IGameInputDevice* device,
        uint64_t ts,
        GameInputDeviceStatus current,
        GameInputDeviceStatus previous);

    inline static std::mutex                                               m_mutex{};
    inline static Microsoft::WRL::ComPtr<IGameInput>                       m_gameInput{};
    inline static GameInputCallbackToken                                   m_deviceToken{};
    inline static std::vector<Microsoft::WRL::ComPtr<IGameInputDevice>>    m_devices{};
    inline static GameInputKind                                            m_activeKind{};
};
