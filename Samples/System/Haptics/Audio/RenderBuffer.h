///--------------------------------------------------------------------------------------
// RenderBuffer.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

struct RenderBuffer
{
	UINT32          BufferSize;
	UINT32          BytesFilled;
    BYTE           *Buffer;
    RenderBuffer   *Next;

    RenderBuffer() :
        BufferSize(0),
        BytesFilled(0),
        Buffer( nullptr ),
        Next( nullptr )
    {
    }

    ~RenderBuffer()
    {
        if(Buffer)
        {
            delete[] Buffer;
            Buffer = nullptr;
        }
    }
};
