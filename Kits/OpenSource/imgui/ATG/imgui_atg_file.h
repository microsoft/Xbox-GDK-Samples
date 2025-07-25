//--------------------------------------------------------------------------------------
// imgui_atgfile.h
//
// ATG-specific ImGui file implementation
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <stdio.h>
#include <stdint.h>

typedef FILE* ImFileHandle;

static inline ImFileHandle ImFileOpen(const char* filename, const char* mode)
{
    FILE* f = NULL;
    fopen_s(&f, filename, mode);
    return f;
}

static inline bool ImFileClose(ImFileHandle file)
{
    return fclose(file) == 0;
}

static inline uint64_t ImFileGetSize(ImFileHandle f)
{
    long off = 0, sz = 0;
    return ((off = ftell(f)) != -1 && !fseek(f, 0, SEEK_END) && (sz = ftell(f)) != -1 && !fseek(f, off, SEEK_SET)) ? (uint64_t)sz : (uint64_t)-1;
}

static inline uint64_t ImFileRead(void* data, uint64_t sz, uint64_t count, ImFileHandle f)
{
    return fread(data, (size_t)sz, (size_t)count, f);
}

static inline uint64_t ImFileWrite(const void* data, uint64_t sz, uint64_t count, ImFileHandle f)
{
    return fwrite(data, (size_t)sz, (size_t)count, f);
}
