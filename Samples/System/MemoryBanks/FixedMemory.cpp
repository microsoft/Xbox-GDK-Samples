//--------------------------------------------------------------------------------------
// FixedMemory.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MemoryDemo.h"

namespace ATG
{
    const char *MemoryDemo::c_fixedTestDataFilename = "t:\\staticMemoryFixedTestData.dat";

    // It's possible to allocate a virtual address range at a specified memory address
    // This allows you to block load data into memory that does not require any pointer fix-ups
    //		The pointers saved to disk are already correct
    // In this case we're using a binary tree that was allocated from the single bank at a fixed address
    bool MemoryDemo::TestFixedAddress()
    {
        bool success = true;
        try
        {
            // Free up any memory from any previous tests
            m_memoryBank.ReleaseBank();

            // Block load the contents of the file into memory at a fixed address
            if (!LoadFixedAddress())
                throw std::logic_error("Unable to load fixed address data");

            // Validate the internal pointers of the binary tree are still correct
            if (!ValidateData(m_rootNode, false, 0))
                throw std::logic_error("Failed to validate fixed address data");
        }
        catch (const std::logic_error what)
        {
            OutputDebugStringA(what.what());
            success = false;
        }

        m_memoryBank.ReleaseBank();
        return success;
    }

    // Load the binary tree from disk, since the tree is allocated from one memory bank a single read from disk is enough
    // Since the memory bank is allocated at the same address as the data was saved there is no need to fix-up the internal addresses
    // They are already correct since data was loaded back into the same base address as it was saved
    bool MemoryDemo::LoadFixedAddress()
    {
        FILE *file = nullptr;
        bool success = true;
        try
        {
            fopen_s(&file, c_fixedTestDataFilename, "rb");
            if (!file)
                throw std::logic_error("unable to open data file\n");

            // Load the saved root address in the file
            // This is the address of the virtual address range to block load the data into
            uintptr_t savedAddress;
            if (fread(&savedAddress, sizeof(savedAddress), 1, file) != 1)
                throw std::logic_error("unable to read fixed data file\n");

            // Create a single virtual address range at the address loaded from the file
            if (!m_memoryBank.CommitBank(c_testDataAllocatorSize * sizeof(TestData), savedAddress))
                throw std::logic_error("unable to commit memory at fixed memory address\n");
            if (m_memoryBank.get() != reinterpret_cast<void *> (savedAddress))
                throw std::logic_error("committed memory at the wrong address in Loading fixed data\n");

            // Block load the contents into memory
            if (fread(m_memoryBank, sizeof(TestData), c_testDataAllocatorSize, file) != c_testDataAllocatorSize)
                throw std::logic_error("unable to read fixed data file\n");
            fclose(file);

            m_rootNode = reinterpret_cast<TestData *> (m_memoryBank.get());
        }
        catch (const std::exception& what)
        {
            OutputDebugStringA(what.what());
            if (file)
                fclose(file);
            m_rootNode = nullptr;
            success = false;
        }
        return success;
    }

    // Helper function to create the initial binary tree for the saved file
    bool MemoryDemo::CreateFixedTestDataFile()
    {
        FILE *file = nullptr;
        TestData *stackAllocator = nullptr;
        try
        {
            // The address space between 4TB and 8TB is reserved at title start by the memory system as a known location for the title to perform fixed address allocations
            constexpr uintptr_t baseAddress = c_LOW_FIXED_RESERVED_ADDRESS + (1ULL * 1024 * 1024 * 1024 * 1024);
            constexpr size_t allocationSize = sizeof(TestData) * c_testDataAllocatorSize;

            stackAllocator = reinterpret_cast<TestData *> (AllocateFixedAddress(reinterpret_cast<void *>(baseAddress), allocationSize));
            // Create the binary tree using a standard stack allocator pattern
            size_t stackAllocatorIndex = 0;
            TestData *rootNode = &(stackAllocator[stackAllocatorIndex++]);
            if (!InternalCreateTestData(rootNode, 1, stackAllocator, stackAllocatorIndex))
                throw std::logic_error("unable to create test tree\n");

            // Save the file to disk
            fopen_s(&file, c_fixedTestDataFilename, "wb");
            if (!file)
                throw std::logic_error("unable to create data file\n");

            // Save the base address for the memory block that holds the binary tree
            if (fwrite(&stackAllocator, sizeof(stackAllocator), 1, file) != 1)
                throw std::logic_error("unable to save fixed data file\n");

            // Block write out the binary tree
            if (fwrite(stackAllocator, sizeof(TestData), c_testDataAllocatorSize, file) != c_testDataAllocatorSize)
                throw std::logic_error("unable to save fixed data file\n");
            fclose(file);
            ReleaseFixedAddress(reinterpret_cast<void *>(stackAllocator), 0);
        }
        catch (const std::exception& what)
        {
            OutputDebugStringA(what.what());
            if (file)
                fclose(file);
            if (stackAllocator)
            {
                ReleaseFixedAddress(reinterpret_cast<void *>(stackAllocator), 0);
            }
            return false;
        }

        return true;
    }
}
