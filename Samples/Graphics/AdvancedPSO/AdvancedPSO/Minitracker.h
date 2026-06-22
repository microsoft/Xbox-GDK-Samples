//-----------------------------------------------------------------------------
// Minitracker.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <mutex>


// hook XMemAlloc allocations
void* __stdcall MyAlloc(_In_ SIZE_T dwSize, _In_ ULONGLONG dwAttributes);

// hook XMemAlloc frees
void __stdcall MyFree(_In_ void* lpAddress, _In_ ULONGLONG dwAttributes);


namespace DX
{
    // Used to track allocations from XMemAlloc.
    struct Minitracker
    {
        struct Entry
        {
            size_t              totalBytes;
            XALLOC_ATTRIBUTES   xAttributes;
            DWORD               threadId;
        };

        static bool                                s_enabled;
        static std::unordered_map< void*, Entry >  s_allocs;
        static std::mutex                          s_critSection;

        DWORD                                      threadId;

        Minitracker()
        {
            threadId = GetCurrentThreadId();
        }

        void Start()
        {
            s_enabled = true;
        }

        void Stop()
        {
            s_enabled = false;
        }

        void Clear()
        {
            std::lock_guard<std::mutex> lock(s_critSection);
            s_allocs.clear();
        }

        void GetStats(const std::unordered_set<ULONGLONG>& ignoreObjectTypes,
                      size_t& numHeapAllocs, size_t& heapSize, size_t& numGpuHeapAllocs, size_t& gpuHeapSize)
        {
            numHeapAllocs = 0;
            heapSize = 0;
            numGpuHeapAllocs = 0;
            gpuHeapSize = 0;

            {
                std::lock_guard<std::mutex> lock(s_critSection);

                for (const auto& it : s_allocs)
                {
                    const auto& alloc = it.second;

                    // only count allocations from this thread
                    if(alloc.threadId != threadId)
                        continue;

                    // should we ignore this type of allocation?
                    if (ignoreObjectTypes.find(alloc.xAttributes.s.ObjectType) != ignoreObjectTypes.end())
                        continue;

                    // Account for it.
                    if (alloc.xAttributes.s.MemoryType == XALLOC_MEMTYPE_HEAP_CACHEABLE)
                    {
                        numHeapAllocs++;
                        heapSize += alloc.totalBytes;
                    }
                    else
                    {
                        numGpuHeapAllocs++;
                        gpuHeapSize += alloc.totalBytes;
                    }
                }
            }
        }

        void GetStatsString(const std::unordered_set<uint64_t>& ignoreObjectTypes, std::wstring& output)
        {
            size_t numHeapAllocs, heapSize, numGpuHeapAllocs, gpuHeapSize;
            GetStats(ignoreObjectTypes, numHeapAllocs, heapSize, numGpuHeapAllocs,gpuHeapSize);

            wchar_t buffer[256] = {};
            swprintf_s(buffer, L"%lld allocs, %lld bytes (heap),\n%lld allocs, %lld bytes (graphics)", numHeapAllocs, heapSize, numGpuHeapAllocs, gpuHeapSize);
            output = buffer;
        }

        // Performs Stop(), GetStatsString(), Clear()
        void ReportAndClear(const std::unordered_set<uint64_t>& ignoreObjectTypes, std::wstring& output)
        {
            Stop();
            GetStatsString(ignoreObjectTypes, output);
            Clear();
        }

    }; // MiniTracker
}
