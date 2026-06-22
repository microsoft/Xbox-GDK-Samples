//--------------------------------------------------------------------------------------
// InMemoryQueueHandler.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DSQueueHandler.h"

#pragma once
class InMemoryQueueHandler :
	public DSQueueHandler
{

public:
    InMemoryQueueHandler(uint64_t uniqueDataSize, uint64_t totalStreamSize, float initialRandPercent, uint16_t initialAlignment, uint32_t m_initialChunkSize);

protected:
    void WorkerStartupFunction();


private:
    float        m_initialRandPercent;   
    uint16_t     m_initialAlignment;       
    uint32_t     m_initialChunkSize;        
};

