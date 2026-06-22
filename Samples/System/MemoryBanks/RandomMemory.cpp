//--------------------------------------------------------------------------------------
// RandomMemory.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MemoryDemo.h"

// The code related to testing the random address sample
namespace ATG
{
	const char *MemoryDemo::c_randomTestDataFilename = "t:\\staticMemoryRandomTestData.dat";

	// The TestFixedAddress shows how to block read data from a file into a known address such that all the internal pointers are still correct
	// This sample is the traditional way to load data.
	// Allocate a new virtual address range, load the data, then fix-up the addresses
	// The same binary tree format as TestFixedAddress is used, except the pointers are stored as offsets from the base memory address
	bool MemoryDemo::TestRandomAddress()
	{
		try
		{
			// Free up any memory from any previous tests
			m_memoryBank.ReleaseBank();

			// Block load the contents of the file into memory at a random address and fix-up the data
			if (!LoadRandomAddress())
				throw std::logic_error("Unable to load random address data");

			// Validate the internal pointers of the binary tree are still correct
			if (!ValidateData(m_rootNode, false, 0))
				throw std::logic_error("Failed to validate random address data");
		}
		catch (const std::logic_error what)
		{
			OutputDebugStringA(what.what());
			m_memoryBank.ReleaseBank();
			return false;
		}
		return true;
	}

	// Since the address is random all of the internal pointers need to be fixed after loading the data from disk
	// The index within the binary tree node is the offset from the root address of the memory bank
	void MemoryDemo::FixupRandomAddresses(TestData *curNode, TestData* memoryBase)
	{
		if (curNode->m_leftIndex != SIZE_MAX)
			curNode->m_left = &(memoryBase[curNode->m_leftIndex]);
		else
			curNode->m_left = nullptr;

		if (curNode->m_rightIndex != SIZE_MAX)
			curNode->m_right = &(memoryBase[curNode->m_rightIndex]);
		else
			curNode->m_right = nullptr;

		if (curNode->m_left)
			FixupRandomAddresses(curNode->m_left, memoryBase);
		if (curNode->m_right)
			FixupRandomAddresses(curNode->m_right, memoryBase);
	}

	// Load the binary tree from disk, since the tree is allocated from one memory bank a single read from disk is enough
	bool MemoryDemo::LoadRandomAddress()
	{
		FILE *file = nullptr;
		try
		{
			if (!m_memoryBank.CommitBank(c_testDataAllocatorSize * sizeof(TestData)))
				throw std::logic_error("unable to commit memory at random memory address\n");

			fopen_s(&file, c_randomTestDataFilename, "rb");
			if (!file)
				throw std::logic_error("unable to open data file\n");
			if (fread(m_memoryBank, sizeof(TestData), c_testDataAllocatorSize, file) != c_testDataAllocatorSize)
				throw std::logic_error("unable to read data file\n");
			fclose(file);

			// Need to fix up the addresses now
			m_rootNode = reinterpret_cast<TestData *> (m_memoryBank.get());
			FixupRandomAddresses(m_rootNode, m_rootNode);
		}
		catch (const std::exception& what)
		{
			OutputDebugStringA(what.what());
			if (file)
				fclose(file);
			m_memoryBank.ReleaseBank();
			m_rootNode = nullptr;
			return false;
		}
		return true;
	}

	// Helper function to create the initial binary tree for the saved file
	bool MemoryDemo::CreateRandomTestDataFile()
	{
		FILE *file = nullptr;
		try
		{
			// Allocate a block of memory and let the OS pick it's location
			size_t stackAllocatorIndex = 0;
			std::unique_ptr<TestData[]> stackAllocator(new TestData[c_testDataAllocatorSize]);
			if (!stackAllocator)
				throw std::logic_error("unable to allocate memory for random test data");

			TestData *rootNode = &(stackAllocator[stackAllocatorIndex++]);
			rootNode->m_isLeftNode = false;
			rootNode->m_treeLevel = 0;

			// Create the binary tree using a standard stack allocator pattern
			if (!InternalCreateTestData(rootNode, 1, stackAllocator.get(), stackAllocatorIndex))
				throw std::logic_error("unable to create test tree\n");

			// Save the file to disk
			fopen_s(&file, c_randomTestDataFilename, "wb");
			if (!file)
				throw std::logic_error("unable to create data file\n");
			if (fwrite(stackAllocator.get(), sizeof(TestData), c_testDataAllocatorSize, file) != c_testDataAllocatorSize)
				throw std::logic_error("unable to save data file\n");
			fclose(file);
		}
		catch (const std::exception& what)
		{
			OutputDebugStringA(what.what());
			if (file)
				fclose(file);
			return false;
		}

		return true;
	}
}
