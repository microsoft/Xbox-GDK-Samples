//--------------------------------------------------------------------------------------
// GameLibrary.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GameLibrary.h"
#include <string>

// A simple function that programs can call
void GetDllInfo(const char* exeName, const unsigned long long destSize, char* destBuffer)
{
    std::string result = "ComboDLL.dll: GetDllInfo(), ";
    result.append(exeName);
    strcpy_s(destBuffer, destSize, result.c_str());
}
