//--------------------------------------------------------------------------------------
// GamepadHaptics.h
//
// Haptic media item used by the sample.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <string>

// A WAV file available to the haptic playback UI.
struct MediaItem
{
    std::wstring filename;  // Full path to WAV file
    std::string title;      // Display name in the UI
};
