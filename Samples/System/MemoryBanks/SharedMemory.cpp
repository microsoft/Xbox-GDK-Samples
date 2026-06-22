//--------------------------------------------------------------------------------------
// SharedMemory.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MemoryDemo.h"

// The code related to testing the shared address sample
namespace ATG
{
	// This function shows how to share a single physical block between multiple virtual address ranges
	// This is useful when different protection flags are needed for the same block of memory
	// It can also be used to create circular buffers that don't require special case handling when wrapping
	bool MemoryDemo::TestSharedAddress()
	{
		__try   // There could be random pointer dereferencing in here on bad data
		{       // we are building with /EHsc which means C++ try/catch will not catch OS exceptions like null pointer dereferences.
				// these types of exceptions require SEH to catch

			// Free up any memory from any previous tests
			m_memoryBank.ReleaseBank();

			// Create two random virtual address ranges that share one physical block of memory
			m_memoryBank.CommitSharedBanks(c_memoryBankSize, 2, false);

			// Set the contents of the first bank to a known non-zero value
			memset(m_memoryBank.get(0), 1, c_memoryBankSize);

			// Bank 0 and bank 1 should have the same contents since their backing physical pages are the same
			if (memcmp(m_memoryBank.get(0), m_memoryBank.get(1), c_memoryBankSize) != 0)
			{
				OutputDebugStringA("shared memory banks are not equal\n");
				return false;
			}

			// Free up any memory from the previous test
			m_memoryBank.ReleaseBank();

			// Create two virtual address ranges that are adjacent that share one physical block of memory
			m_memoryBank.CommitSharedBanks(c_memoryBankSize * sizeof(uint32_t), 2, true);

			uint32_t *baseAddress = reinterpret_cast<uint32_t *> (m_memoryBank.get(0));

			// Write to memory so that it writes off the end of the buffer and should rotate around to the start automatically
			for (uint32_t i = 0; i < c_memoryBankSize; i++)
			{
				baseAddress[i + (c_memoryBankSize / 2)] = i;
			}

			// Validate the memory correctly mapped without needing any special case handling when wrapping in the circular buffer
			for (uint32_t i = 0; i < c_memoryBankSize; i++)
			{
				uint32_t savedValue = baseAddress[i];
				if (savedValue != ((i + (c_memoryBankSize / 2)) % c_memoryBankSize))
				{
					OutputDebugStringA("Testing adjacent shared memory banks did not wrap correctly\n");
					return false;
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			OutputDebugStringA("Testing shared memory banks threw an exception\n");
			return false;
		}

		return true;
	}
}
