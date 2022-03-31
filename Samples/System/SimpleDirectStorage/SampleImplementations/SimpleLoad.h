//--------------------------------------------------------------------------------------
// SimpleLoad.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// Shows how to setup DirectStorage, read a file, shutdown DirectStorage
// Explicitly does not derive from ImplementationBase to enforce all the main DirectStorage pieces are in this one class
class SimpleLoad
{
private:
    static constexpr uint32_t c_dataReadSize = 65536;

public:
    SimpleLoad() {}
    ~SimpleLoad() {}

    bool RunSample(const std::wstring& fileName);
};
