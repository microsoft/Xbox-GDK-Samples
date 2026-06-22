//--------------------------------------------------------------------------------------
// PlayFabUtils.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------- 
#pragma once

#include <vector>
#include <string>

class PropertyHelper
{
public:
    PropertyHelper() = default;
    ~PropertyHelper() = default;

    void AddProperty(const std::string& propertyKey, const std::string& propertyValue)
    {
        if (propertyKey.empty() == false)
        {
            m_propertyKeys.Add(propertyKey);
            m_propertyValues.Add(propertyValue);
        }
    }

    uint32_t GetCount() const
    {
        return m_propertyKeys.GetCount();
    }

    const char** GetKeyData()
    {
        return m_propertyKeys.GetData();
    }

    const char** GetValueData()
    {
        return m_propertyValues.GetData();
    }

private:
    class AnsiStringList
    {
    public:
        AnsiStringList() = default;
        ~AnsiStringList() = default;

        void Add(const std::string& inString)
        {
            //store the data
            m_ansiStrings.push_back(inString);

            //increment the count
            m_count += 1;
        }

        const char** GetData()
        {
            m_ansiStringPtrs.clear();
            m_ansiStringPtrs.reserve(m_ansiStrings.size());

            for (const std::string& str : m_ansiStrings)
            {
                //store the pointer
                m_ansiStringPtrs.push_back(str.c_str());
            }

            return m_ansiStringPtrs.data();
        }

        uint32_t GetCount() const
        {
            return m_count;
        }

    private:
        std::vector<std::string> m_ansiStrings;
        std::vector<const char*> m_ansiStringPtrs;
        uint32_t m_count = 0;
    };

    AnsiStringList m_propertyKeys;
    AnsiStringList m_propertyValues;
};
