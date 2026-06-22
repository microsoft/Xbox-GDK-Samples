//--------------------------------------------------------------------------------------
// PdbMemoryStream.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// Define for the IPdbMemoryStream interface to make it easier to QueryInterface if needed
class DECLSPEC_UUID("B3BA4E52-B888-4BDD-A317-10B67022BA04") IPdbMemoryStream : public IStream
{
public:
};

// Create an IPdbMemoryStream object an associate the matching PDB file
    // Setting a cache size of 0 disables the cache
    // Setting a cache size of UINT64_MAX block loads the entire PDB into memory, cacheBlockSize is ignored in this case
STDAPI CreatePdbMemoryStream(PCWSTR pdbPath, uint64_t totalCacheSize, uint64_t cacheBlockSize, REFIID riid, __deref_out void **ppvObject);
