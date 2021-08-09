//
// Main.cpp
//

#include <iostream>

#include <DirectxMath.h>

extern "C" void OutputDebugStringW(const wchar_t* lpOutputString);

int main()
{
    std::cout << "Hello, world from GameCore on Xbox" << std::endl;

    OutputDebugStringW(L"INFO: Hello, world from GameCore on Xbox");

    if (!DirectX::XMVerifyCPUSupport())
    {
        std::cerr << "ERROR: This platform does not support our build settings" << std::endl;
    }
}
