//--------------------------------------------------------------------------------------
// WaveSampleGenerator.h
//
// Demonstrates how to play an in-memory WAV file via WASAPI.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "IGenerator.h"
#include "RenderBuffer.h"

class WaveSampleGenerator : public IGenerator
{
    public:
        WaveSampleGenerator();
        virtual ~WaveSampleGenerator();

        virtual bool   IsEOF() const { return (m_SampleQueue == nullptr); }
        virtual UINT32 GetBufferLength() const { return (m_SampleQueue != nullptr ? m_SampleQueue->BufferSize : 0); }
        virtual void   Flush();
        virtual HRESULT FillSampleBuffer(UINT32 bytesToRead, BYTE* pData);

        HRESULT GenerateSampleBuffer( BYTE* pWaveData, DWORD waveSize, WAVEFORMATEXTENSIBLE* pSourceWfx, UINT32 framesPerPeriod, WAVEFORMATEX *pWfx );

    private:
        RenderBuffer*  m_SampleQueue;
        RenderBuffer** m_SampleQueueTail;
};
