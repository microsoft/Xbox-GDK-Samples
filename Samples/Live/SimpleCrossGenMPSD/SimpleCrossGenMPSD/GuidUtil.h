//-----------------------------------------------------------------------------
// GuidUtil.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace DX
{
    class GuidUtil
    {
    public:
        static std::string NewGuid()
        {
            GUID id = {};
            char buf[64] = {};

            CoCreateGuid(&id);

            sprintf_s(buf, "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
                id.Data1, id.Data2, id.Data3,
                id.Data4[0], id.Data4[1], id.Data4[2], id.Data4[3],
                id.Data4[4], id.Data4[5], id.Data4[6], id.Data4[7]);

            return std::string(buf);
        }
    };
}
