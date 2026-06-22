//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

uint PackU32ToU16(uint2 v)
{
    return __XB_PackU32ToU16(v.x, v.y);
}

uint2 UnpackU16FromU32(uint x)
{
    return uint2(x & 0xffff, x >> 16);
}

uint CountBits64(uint2 v)
{
    return countbits(v.x) + countbits(v.y);
}
