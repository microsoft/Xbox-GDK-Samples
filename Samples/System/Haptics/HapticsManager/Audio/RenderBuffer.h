///--------------------------------------------------------------------------------------
// RenderBuffer.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>

namespace ATG
{
    struct RenderBuffer
    {
        uint32_t       BufferSize;
        uint32_t       BytesFilled;
        uint8_t* Buffer;
        RenderBuffer* Next;

        RenderBuffer() :
            BufferSize(0),
            BytesFilled(0),
            Buffer(nullptr),
            Next(nullptr)
        {
        }

        ~RenderBuffer()
        {
            if (Buffer)
            {
                delete[] Buffer;
                Buffer = nullptr;
            }
        }
    };
}
