//--------------------------------------------------------------------------------------
// TextEntry.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace ATG
{
    typedef void CALLBACK TextEntryCallback(
        void* userContext,             // User context pointer passed to ShowTextEntry
        bool confirmed,                // True = User confirmed, False = User canceled
        const char* resultTextBuffer,  // Text entered by the user
        uint32_t resultTextBufferUsed  // Length of text entered by user, including NULL terminator
    );

    bool ShowTextEntry(
        const char* titleText,       // Text which appears in the titlebar
        const char* descriptionText, // Text which appears at the top of the dialog, max 2 lines
        const char* defaultText,     // Default/hint text prepopulated in the edit box
        uint32_t maxTextLength,      // Maximum length of text the user can enter
        TextEntryCallback callback,  // Callback function when the user confirms or cancels
        void* context                // User context pointer passed to the callback
    );
}

// Externs provided by the VirtualKeyboard*.cpp snippet
extern bool ShowVirtualKeyboard();
extern bool HideVirtualKeyboard();
