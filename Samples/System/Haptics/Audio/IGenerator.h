//--------------------------------------------------------------------------------------
// IGenerator.h
//
// Common interface for different generator types
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

class IGenerator
{
    public:
        virtual ~IGenerator() {};
        virtual bool IsEOF() const = 0;
        virtual UINT32 GetBufferLength() const = 0;
        virtual void Flush() = 0;
        virtual HRESULT FillSampleBuffer(UINT32 bytesToRead, BYTE* pData) = 0;
};
