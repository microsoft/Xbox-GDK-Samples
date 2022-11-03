//--------------------------------------------------------------------------------------
// SimpleLoad.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// Shows how to setup DirectStorage, read a file, and shutdown DirectStorage
// Explicitly does not derive from ImplementationBase to enforce all the main DirectStorage pieces are in this one class
class SimpleLoad
{
private:
    static const uint32_t c_dataReadSize = 65536;

public:
    SimpleLoad() = default;
    ~SimpleLoad() = default;

    bool RunSample(const std::wstring& fileName);
};
