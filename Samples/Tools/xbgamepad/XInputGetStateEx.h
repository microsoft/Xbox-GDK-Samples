//--------------------------------------------------------------------------------------
// XInputGetStateEx.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include "xinput.h"

typedef DWORD(WINAPI* XInputGetStateExFnPtr)
(
    _In_  DWORD         dwUserIndex,  // Index of the gamer associated with the device
    _Out_ XINPUT_STATE* pState        // Receives the current state
    );

extern XInputGetStateExFnPtr XInputGetStateEx;

bool SetupXInputGetStateEx();
