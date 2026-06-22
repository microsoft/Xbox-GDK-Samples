//--------------------------------------------------------------------------------------
// DataBuffer.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace PlayFabMultiplayerRumble
{

class DataBufferReader
{
public:
    DataBufferReader(const std::vector<uint8_t> &buffer);

    uint8_t ReadByte(void);

    int32_t ReadInt32(void);
    uint32_t ReadUInt32(void);
    uint64_t ReadUInt64(void);

    float ReadSingle(void);
    double ReadDouble(void);

    std::string ReadString(void);

    template<typename T>
    void ReadStruct(T &data)
    {
        ReadData(&data, sizeof(T));
    }

    inline size_t TotalBytes() { return m_buffer.size(); }
    inline size_t RemainingBytes() { return m_buffer.size() - m_pos; }

private:
    void ReadData(void *dest, size_t length);

    size_t m_pos;
    const std::vector<uint8_t> &m_buffer;
};

class DataBufferWriter
{
public:
    DataBufferWriter(size_t initialSize = 512);

    void WriteByte(uint8_t data);

    void WriteInt32(int32_t data);
    void WriteUInt32(uint32_t data);
    void WriteUInt64(uint64_t data);

    void WriteSingle(float data);
    void WriteDouble(double data);

    void WriteString(std::string_view data);

    template<typename T>
    void WriteStruct(const T &data)
    {
        WriteData(&data, sizeof(T));
    }

    std::vector<uint8_t> &&GetBuffer();

    size_t TotalBytes() { return m_pos; }

private:
    void WriteData(const void *src, size_t length);

    size_t m_pos;
    std::vector<uint8_t> m_buffer;
};

}
