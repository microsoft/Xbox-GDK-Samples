//--------------------------------------------------------------------------------------
// WaveSampleGenerator.cpp
//
// Demonstrates how to play an in-memory WAV file via WASAPI.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "WaveSampleGenerator.h"

//--------------------------------------------------------------------------------------
//  Name:   WaveSampleReader
//  Desc:   Class that can read from WAVE data and convert it to floating point
//--------------------------------------------------------------------------------------

namespace ATG
{
    class WaveSampleReader
    {
    public:
        WaveSampleReader(const uint8_t* pWaveData, uint32_t waveSize, WAVEFORMATEXTENSIBLE* pWfx);

        ~WaveSampleReader()
        {
        }

        void ReadBlock(float* pOutput, uint32_t nMaxChannels);

        bool IsEOF(void) const
        {
            return ((m_pCurrent - m_pBase) >= m_nSize);
        }

    private:

        const uint8_t* m_pBase;
        const uint8_t* m_pCurrent;
        uint32_t       m_nSize;
        uint32_t       m_nBitsPerSample;
        uint32_t       m_nChannels;
        uint32_t       m_nBlockAlign;
        bool           m_bIsFloat;
    };

    WaveSampleReader::WaveSampleReader(const uint8_t* pWaveData, uint32_t waveSize, WAVEFORMATEXTENSIBLE* pWfx) :
        m_pBase(pWaveData),
        m_pCurrent(pWaveData),
        m_nSize(waveSize),
        m_nBitsPerSample(0),
        m_nChannels(0),
        m_nBlockAlign(0),
        m_bIsFloat(false)
    {
        if ((pWfx->Format.wFormatTag == WAVE_FORMAT_PCM) ||
            (pWfx->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE && pWfx->SubFormat == KSDATAFORMAT_SUBTYPE_PCM))
        {
            m_bIsFloat = false;
            m_nBitsPerSample = pWfx->Format.wBitsPerSample;

            m_nChannels = pWfx->Format.nChannels;
            m_nBlockAlign = pWfx->Format.nBlockAlign;
        }
        else if ((pWfx->Format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) ||
            (pWfx->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE && pWfx->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
        {
            m_bIsFloat = true;
            m_nBitsPerSample = pWfx->Format.wBitsPerSample;

            m_nChannels = pWfx->Format.nChannels;
            m_nBlockAlign = pWfx->Format.nBlockAlign;
        }
    }


    //--------------------------------------------------------------------------------------
    //  Name:   ReadBlock
    //  Desc:   Read one block's worth of sample data, convert it to floating point and
    //          store as many channels as is possible in the array provided.
    //--------------------------------------------------------------------------------------

    void WaveSampleReader::ReadBlock(float* pOutput, uint32_t nMaxChannels)
    {
        if (!IsEOF())
        {
            const uint32_t  nCopyChannels = min(nMaxChannels, m_nChannels);
            const uint32_t  nSampleIncrement = m_nBitsPerSample / 8;

            const uint8_t* pSample = m_pCurrent;
            uint32_t  nChannel = 0;

            //  Copy as many channels as we can
            for (; nChannel < nCopyChannels; ++nChannel)
            {
                float   fValue = 0;

                if (m_bIsFloat)
                {
                    fValue = *(float*)pSample;
                }
                else
                {
                    switch (m_nBitsPerSample)
                    {
                    case 16:
                        fValue = *(INT16*)(pSample) / ((float)INT16_MAX + 1.0f);
                        break;

                    case 24:
                    {
                        //  Convert from 24 bit to 32 bit, replicating top byte into
                        //  low byte.
                        const uint32_t unpacked = (((uint32_t)pSample[2] & 0xFFU) << 24) |
                            (((uint32_t)pSample[1] & 0xFFU) << 16) |
                            (((uint32_t)pSample[0] & 0xFFU) << 8) |
                            (((uint32_t)pSample[2] & 0xFFU));
                        fValue = ((INT32)unpacked) / ((float)INT32_MAX + 1.0f);
                    }
                    break;

                    case 32:
                        fValue = *(INT32*)(pSample) / ((float)INT32_MAX + 1.0f);
                        break;
                    }
                }
                pOutput[nChannel] = fValue;
                pSample += nSampleIncrement;
            }

            //  Fill remaining channels with zero
            for (; nChannel < nMaxChannels; ++nChannel)
            {
                pOutput[nChannel] = 0.0f;
            }

            //  Advance current pointer by one block
            m_pCurrent += m_nBlockAlign;
        }
        else
        {
            //  If we're at EOF then just clear the output buffer
            ZeroMemory(pOutput, nMaxChannels * sizeof(float));
        }
    }


    //--------------------------------------------------------------------------------------
    //  Name:   WaveSampleWriter
    //  Desc:   Class that accepts floating point sample data, and can store it in an
    //          alternative representation.
    //--------------------------------------------------------------------------------------

    class WaveSampleWriter
    {
    public:
        WaveSampleWriter(uint8_t* pBuffer, uint32_t bufferSize, WAVEFORMATEX* pWfx);

        ~WaveSampleWriter()
        {
        }

        void WriteBlock(const float* pInput, uint32_t nMaxChannels);

        bool IsEOF(void) const
        {
            return ((m_pCurrent - m_pBase) >= m_nSize);
        }

    private:

        uint8_t* m_pBase;
        uint8_t* m_pCurrent;
        uint32_t      m_nSize;
        uint32_t      m_nBitsPerSample;
        uint32_t      m_nChannels;
        uint32_t      m_nBlockAlign;
        bool        m_bIsFloat;
    };


    //--------------------------------------------------------------------------------------
    //  Name: WaveSampleWriter
    //--------------------------------------------------------------------------------------

    WaveSampleWriter::WaveSampleWriter(uint8_t* pBuffer, uint32_t bufferSize, WAVEFORMATEX* pWfx) :
        m_pBase(pBuffer),
        m_pCurrent(pBuffer),
        m_nSize(bufferSize),
        m_nBitsPerSample(0),
        m_nChannels(0),
        m_nBlockAlign(0),
        m_bIsFloat(false)
    {
        if ((pWfx->wFormatTag == WAVE_FORMAT_PCM) ||
            (pWfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && ((WAVEFORMATEXTENSIBLE*)pWfx)->SubFormat == KSDATAFORMAT_SUBTYPE_PCM))
        {
            m_bIsFloat = false;
            m_nBitsPerSample = pWfx->wBitsPerSample;

            m_nChannels = pWfx->nChannels;
            m_nBlockAlign = pWfx->nBlockAlign;
        }
        else if ((pWfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) ||
            (pWfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && ((WAVEFORMATEXTENSIBLE*)pWfx)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
        {
            m_bIsFloat = true;
            m_nBitsPerSample = pWfx->wBitsPerSample;

            m_nChannels = pWfx->nChannels;
            m_nBlockAlign = pWfx->nBlockAlign;
        }
    }


    //--------------------------------------------------------------------------------------
    //  Name: WriteBlock
    //  Desc: Given a number of floating point samples, write as many as we can
    //        into our output buffer.
    //--------------------------------------------------------------------------------------

    void WaveSampleWriter::WriteBlock(const float* pInput, uint32_t nMaxChannels)
    {
        if (!IsEOF())
        {
            const uint32_t  nCopyChannels = min(nMaxChannels, m_nChannels);
            const uint32_t  nSampleIncrement = m_nBitsPerSample / 8;

            uint8_t* pSample = m_pCurrent;
            uint32_t  nChannel = 0;

            //  Copy as many channels as we can
            for (; nChannel < nCopyChannels; ++nChannel)
            {
                float   fValue = pInput[nChannel];

                if (m_bIsFloat)
                {
                    *(float*)(pSample) = fValue;
                }
                else
                {
                    switch (m_nBitsPerSample)
                    {
                    case 16:
                        *(INT16*)(pSample) = (INT16)(fValue * ((float)INT16_MAX));
                        break;

                    case 32:
                        *(INT32*)(pSample) = (INT32)(fValue * ((float)INT32_MAX));
                        break;
                    }
                }

                pSample += nSampleIncrement;
            }

            //  Fill remaining channels with zero
            ZeroMemory(pSample, (m_nChannels - nChannel) * nSampleIncrement);

            //  Advance current pointer by one block
            m_pCurrent += m_nBlockAlign;
        }
    }

    WaveSampleGenerator::WaveSampleGenerator()
    {
        m_SampleQueue = nullptr;
        m_SampleQueueTail = &m_SampleQueue;
    }

    WaveSampleGenerator::~WaveSampleGenerator()
    {
        // Flush unused samples
        Flush();
    }


    //--------------------------------------------------------------------------------------
    //  Name: GenerateSampleBuffer()
    //  Desc: Create a linked list of sample buffers
    //--------------------------------------------------------------------------------------

    HRESULT WaveSampleGenerator::GenerateSampleBuffer(uint8_t* pWaveData, uint32_t waveSize, WAVEFORMATEXTENSIBLE* pSourceWfx, uint32_t framesPerPeriod, WAVEFORMATEX* pWfx)
    {
        HRESULT hr = S_OK;

        float sampleRatio = (float)pWfx->nSamplesPerSec / (float)pSourceWfx->Format.nSamplesPerSec;

        uint32_t renderBufferSizeInBytes = framesPerPeriod * pWfx->nBlockAlign;
        uint32_t renderDataLength = (waveSize / pSourceWfx->Format.nBlockAlign * pWfx->nBlockAlign) + (renderBufferSizeInBytes - 1);
        uint32_t renderBufferCount = (uint32_t)(renderDataLength * sampleRatio / renderBufferSizeInBytes);

        WaveSampleReader reader(pWaveData, waveSize, pSourceWfx);

        for (uint32_t i = 0; i < renderBufferCount; i++)
        {
            RenderBuffer* sampleBuffer = new RenderBuffer();

            sampleBuffer->BufferSize = renderBufferSizeInBytes;
            sampleBuffer->BytesFilled = renderBufferSizeInBytes;
            sampleBuffer->Buffer = new uint8_t[renderBufferSizeInBytes];

            WaveSampleWriter writer(sampleBuffer->Buffer, sampleBuffer->BufferSize, pWfx);

            while (!writer.IsEOF())
            {
                constexpr uint32_t nMaxChannels = 8;
                float channelData[nMaxChannels];

                //  Get a block of channel data (as much as we're interested in).
                //  If the reader is at end of file then this returns a block
                //  of zeroed samples.
                reader.ReadBlock(channelData, nMaxChannels);

                for (unsigned int j = 0; j < sampleRatio; j++)
                {
                    writer.WriteBlock(channelData, nMaxChannels);
                }
            }

            *m_SampleQueueTail = sampleBuffer;
            m_SampleQueueTail = &sampleBuffer->Next;
        }
        return hr;
    }


    //--------------------------------------------------------------------------------------
    //  Name: FillSampleBuffer()
    //  Desc: File the Data buffer of size BytesToRead with the first item in the queue.  
    //  Caller is responsible for allocating and freeing buffer
    //--------------------------------------------------------------------------------------

    HRESULT WaveSampleGenerator::FillSampleBuffer(uint32_t bytesToRead, uint8_t* pData)
    {
        if (nullptr == pData)
        {
            return E_POINTER;
        }

        RenderBuffer* sampleBuffer = m_SampleQueue;

        if (bytesToRead > sampleBuffer->BufferSize)
        {
            return E_INVALIDARG;
        }

        CopyMemory(pData, sampleBuffer->Buffer, bytesToRead);

        m_SampleQueue = m_SampleQueue->Next;

        return S_OK;
    }


    //--------------------------------------------------------------------------------------
    //  Name: Flush()
    //  Desc: Remove and free unused samples from the queue
    //--------------------------------------------------------------------------------------

    void WaveSampleGenerator::Flush()
    {
        while (m_SampleQueue != nullptr)
        {
            RenderBuffer* sampleBuffer = m_SampleQueue;
            m_SampleQueue = sampleBuffer->Next;
        }
    }
}
