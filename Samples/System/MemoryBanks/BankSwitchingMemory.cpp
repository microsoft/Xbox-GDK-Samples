//--------------------------------------------------------------------------------------
// BankSwitchingMemory.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MemoryDemo.h"

namespace ATG
{
	// This function shows how to create two blocks of memory that can have their contents swapped for a minimal cost
	// This is done in several phases
	//		Create two physical banks
	//		Reserve two virtual address ranges each equal in size to a single physical bank
	//		Assign each virtual address range to a physical bank
	//		Update either virtual address range as desired
	//		Swap the virtual address ranges assigned to the two physical banks
	// At this point the contents of each virtual address range have swapped
	// This is the same way a double buffered system works
	bool MemoryDemo::TestBankSwitching()
	{
        bool success = true;
		try
		{
			// Free up any memory from any previous tests
			m_memoryBank.ReleaseBank();

			// Create two physical banks along with an associated virtual address range
			if (!m_memoryBank.CommitRotateBanks(c_memoryBankSize * sizeof(uint32_t), 2))
				throw std::logic_error("Unable to commit rotated banks");

			// Write out unique contents to each virtual address range
			uint32_t *bank0 = reinterpret_cast<uint32_t *> (m_memoryBank.get(0));
			uint32_t *bank1 = reinterpret_cast<uint32_t *> (m_memoryBank.get(1));
			for (uint32_t i = 0; i < c_memoryBankSize; i++)
			{
				bank0[i] = i;
				bank1[i] = (1 << 24) + i;
			}

			// Swap the virtual address ranges between the two physical banks
			// Physical bank 0 is remapped to virtual address range 1 and vice-versa
			m_memoryBank.SwapBanks(0, 1);

			// Verify the contents of the virtual address ranges were successfully swapped
			for (uint32_t i = 0; i < c_memoryBankSize; i++)
			{
				if (bank1[i] != i)
					throw std::logic_error("Bank switching failed with incorrect data in the original first bank");
				if (bank0[i] != ((1 << 24) + i))
					throw std::logic_error("Bank switching failed with incorrect data in the original second bank");
			}
		}
		catch (const std::logic_error& what)
		{
			OutputDebugStringA(what.what());
			success = false;
		}

		// Clean up allocated memory
		m_memoryBank.ReleaseBank();
		return success;
	}
}
