//--------------------------------------------------------------------------------------
// File: TestFramework.h
//
// Stubbed test framework code
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#pragma once

#include "GamePad.h"

#ifdef ATG_AUTOMATION
#include "TestFrameworkImpl.h"
#else
using ATGGamePad = DirectX::GamePad;

namespace ATG
{
    class TestFramework
    {
        public:
            void TestInitialize() {}
            void TestTick() {}
    };
};
#endif
