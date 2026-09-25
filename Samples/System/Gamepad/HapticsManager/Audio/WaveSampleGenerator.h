//--------------------------------------------------------------------------------------
// WaveSampleGenerator.h
//
// Demonstrates how to play an in-memory WAV file via WASAPI.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <wrl.h>
#include <mmreg.h>

#include "IGenerator.h"
#include "RenderBuffer.h"

namespace ATG
{
    class WaveSampleGenerator : public IGenerator
    {
        public:
            WaveSampleGenerator();
            virtual ~WaveSampleGenerator();

            virtual bool     IsEOF()           const { return (m_SampleQueue == nullptr); }
            virtual uint32_t GetBufferLength() const { return (m_SampleQueue != nullptr ? m_SampleQueue->BufferSize : 0); }
            virtual void     Flush();
            virtual HRESULT  FillSampleBuffer(uint32_t bytesToRead, uint8_t* pData);
            HRESULT          GenerateSampleBuffer(uint8_t* pWaveData, uint32_t waveSize, WAVEFORMATEXTENSIBLE* pSourceWfx, uint32_t framesPerPeriod, WAVEFORMATEX* pWfx);

        private:
            RenderBuffer* m_SampleQueue;
            RenderBuffer** m_SampleQueueTail;
        };
}
