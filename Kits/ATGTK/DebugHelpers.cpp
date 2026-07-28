//--------------------------------------------------------------------------------------
// File: DebugHelpers.cpp
//
// A set of helpers for debugging
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "pch.h"
#include "DebugHelpers.h"

//////////////////////////////////////////////////////////////////////////////////////
//
// For this to work with Xbox samples, you MUST deploy the sample's PDB to the same
// directory as the executable, otherwise its symbols will not be found.  The easiest
// way to do this is to remove "*.pdb" from Xbox One -> Layout -> Exclusion Filter in
// the project settings.
//
//////////////////////////////////////////////////////////////////////////////////////
std::string DX::PrintStackTrace()
{
    const DWORD MaxFrames = 16;
    const DWORD MaxNameLen = 512;

    std::stringstream ss;

    // cache the current process handle
    HANDLE process = GetCurrentProcess();

    // set symbol resolution to undecorated names, defer loading, get line info
    DWORD options = SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES;
    if(SymSetOptions(options) != options)
    {
        ss << "Unable to set symbol options\n";
        return ss.str();
    }

    // initialize symbol resolution
    if(!SymInitialize(process, nullptr, TRUE))
    {
        ss << "Unable to initialize symbols: " << std::hex << HRESULT_FROM_WIN32(GetLastError());
        return ss.str();
    }

    DWORD hash = 0;
    LPVOID backTrace[MaxFrames] = {};
    WORD captured = CaptureStackBackTrace(0, MaxFrames, backTrace, &hash);
    auto *sym = static_cast<IMAGEHLP_SYMBOL64*>(calloc(sizeof(IMAGEHLP_SYMBOL64) + ((MaxNameLen-1) * sizeof(wchar_t)), 1));
    if(sym)
    {
        sym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
        sym->MaxNameLength = MaxNameLen;

        for(unsigned int i = 0; i < captured; i++)
        {
            // get the module name
            IMAGEHLP_MODULE64 mod{};
            mod.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
            if(SymGetModuleInfo64(process, reinterpret_cast<DWORD64>(backTrace[i]), &mod))
            {
                // get the symbol name
                if(SymGetSymFromAddr64(process, reinterpret_cast<DWORD64>(backTrace[i]), 0, sym))
                {
                    // get the file/line number
                    DWORD displacement = 0;
                    IMAGEHLP_LINE64 line = {};
                    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
                    if(SymGetLineFromAddr64(process, reinterpret_cast<DWORD64>(backTrace[i]), &displacement, &line))
                    {
                        ss << "\t" << mod.ModuleName << "!" << sym->Name << " - " << line.FileName << " (" << line.LineNumber << ")\n";
                    }
                    else
                    {
                        ss << "\t" << mod.ModuleName << "!" << sym->Name << "\n";
                    }
                }
                else
                {
                    ss << "\t" << mod.ModuleName << "!" << std::hex << reinterpret_cast<DWORD64>(backTrace[i]) << "\n";
                }
            }
            else
            {
                ss << "\t" << std::hex << reinterpret_cast<DWORD64>(backTrace[i]) << "\n";
            }
        }
        OutputDebugStringA(ss.str().c_str());
        free(sym);
    }
    else
    {
        ss << "\tUnable to allocate IMAGEHLP_SYMBOL64: " << std::hex << HRESULT_FROM_WIN32(GetLastError()) << "\n";
    }
    return ss.str();
}
