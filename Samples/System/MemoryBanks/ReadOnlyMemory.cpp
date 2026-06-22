//--------------------------------------------------------------------------------------
// ReadOnlyMemory.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MemoryDemo.h"

// The code related to testing the read only address sample
namespace ATG
{
	bool MemoryDemo::TestReadOnlyBank()
	{
		// Free up any memory from any previous tests
		m_memoryBank.ReleaseBank();

        // Create two random virtual address ranges that share one physical block of memory
		if (!m_memoryBank.CommitSharedBanks(c_memoryBankSize, 2))
			return false;

        // Bank 0 will be left writeable
        // Bank 1 will be set to read-only
		m_memoryBank.LockBank(1);

		__try       // This block should not throw an exception since the data should be writeable.
		{           // We are building with /EHsc which means C++ try/catch will not catch OS exceptions like null pointer dereferences.
					// These types of exceptions require SEH to catch
			memset(m_memoryBank.get(0), 1, c_memoryBankSize);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			m_memoryBank.ReleaseBank();
			OutputDebugStringA("Trying to write to read-write block of shared bank failed, it should not\n");
			return false;
		}

		bool successfullTest = false;
		__try       // This should throw an exception since the data block is now read-only
		{           // We are building with /EHsc which means C++ try/catch will not catch OS exceptions
					// like this write access violation, so we use SEH instead to catch it
			char *bankBaseAddress = static_cast<char *> (m_memoryBank.get(1));
			for (uint32_t i = 0; i < c_memoryBankSize; i++)
			{
				bankBaseAddress[i] = 1;     // Should throw access violation exception
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			successfullTest = true;
		}
		if (!successfullTest)
			throw std::logic_error("Trying to write to read-only block of shared bank succeeded which is should not");
		return true;
	}
}
