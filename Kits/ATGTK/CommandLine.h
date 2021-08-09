//--------------------------------------------------------------------------------------
// CommandLine.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <cwchar>
#include <string>
#include <vector>

namespace ATG
{
    inline std::vector<std::wstring> BreakCommandLine_QuotedParameters(const wchar_t* commandLineParams)
    {
        std::vector<std::wstring> toret;

        bool inToken = false;
        bool inQuotedToken = false;
        const wchar_t* start = commandLineParams;
        const wchar_t* cur = start;
        while (*cur != 0)
        {
            if (inToken)
            {
                if (isspace(*cur))
                {
                    toret.push_back(std::wstring(start, static_cast<uint64_t> (cur - start)));

                    inToken = false;
                }
                else if ((*cur) == '\"')
                {
                    toret.push_back(std::wstring(start, static_cast<uint64_t> (cur - start)));

                    inToken = false;

                    inQuotedToken = true;
                    start = cur + 1;
                }
            }
            else if (inQuotedToken)
            {
                if ((*cur) == '\"')
                {
                    toret.push_back(std::wstring(start, static_cast<uint64_t> (cur - start)));

                    inQuotedToken = false;
                }
            }
            else
            {
                if (!isspace(*cur))
                {
                    if ((*cur) == '\"')
                    {
                        inQuotedToken = true;
                        start = cur + 1;
                    }
                    else
                    {
                        inToken = true;
                        start = cur;
                    }
                }
            }

            ++cur;
        }
        if (inToken || inQuotedToken)
        {
            toret.push_back(std::wstring(start, static_cast<uint64_t> (cur - start)));
        }
        return toret;
    }

	inline std::vector<std::wstring> BreakCommandLine(const wchar_t *commandLineParams)
	{
		std::vector<std::wstring> toret;

        const wchar_t* start = commandLineParams;
        const wchar_t* cur = start;
		while (*cur != 0)
		{
			while (!isspace(*cur) && (*cur != 0))
				++cur;
			toret.push_back(std::wstring(start, static_cast<uint64_t> (cur - start)));
			while (isspace(*cur) && (*cur != 0))
				++cur;
			start = cur;
		}
		if (start != cur)
		{
			toret.push_back(std::wstring(start, static_cast<uint64_t> (cur - start)));
		}
		return toret;
	}

	inline std::vector<std::wstring> BreakCommandLine(int32_t argc, wchar_t *argv[])
	{
		std::vector<std::wstring> toret;

		for (int32_t i = 1; i < argc; i++)			// first entry is the name of the executable, so skip it
		{
			toret.push_back(argv[i]);
		}
		return toret;
	}
}
