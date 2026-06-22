//--------------------------------------------------------------------------------------
// LauncherCache.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "LauncherCache.h"
#include "Json.h"
#include "StringUtil.h"

using namespace ATG;

LauncherCache::LauncherCache(const char* cacheFilePath)
    : m_cacheFilePath(cacheFilePath)
{
}

static void ReadFromJson(const json& jsonData, std::map<std::string, LauncherCache::PathCacheData>& cache)
{
    for (json::const_iterator fileIter = jsonData.begin(); fileIter != jsonData.end(); ++fileIter)
    {
        std::string path = fileIter.key();
        for (json::const_iterator valueIter = fileIter.value().begin(); valueIter != fileIter.value().end(); ++valueIter)
        {
            std::string id = valueIter.key();
            std::string value = valueIter.value();

            cache[DX::ToLower(path)].m_values[DX::ToLower(id)] = value;
        }
    }
}

bool LauncherCache::Load()
{
    json jsonData;
    std::ifstream fileStream;
    fileStream.open(m_cacheFilePath);
    if (fileStream)
    {
        fileStream >> jsonData;

        Reset(false);
        ReadFromJson(jsonData, m_cache);

        fileStream.close();
        return true;
    }

    return false;
}

bool LauncherCache::Save()
{
    std::ofstream fileStream;
    fileStream.open(m_cacheFilePath);
    if (fileStream)
    {
        json jsonData;
        for (auto pathIter = m_cache.begin(); pathIter != m_cache.end(); ++pathIter)
        {
            std::string path = pathIter->first;
            for (auto idIter = pathIter->second.m_values.begin(); idIter != pathIter->second.m_values.end(); ++idIter)
            {
                std::string id = idIter->first;
                std::string value = idIter->second;
                jsonData[path][id] = value;
            }
        }

        fileStream << std::setw(4) << jsonData << std::endl;

        fileStream.close();
        return true;
    }

    return false;
}

bool LauncherCache::Reset(bool save, bool forceOnSaveFailure)
{
    if (save && !Save() && !forceOnSaveFailure)
    {
        return false;
    }

    m_cache.clear();
    return true;
}

void LauncherCache::SetValue(const std::string& path, const std::string& id, const std::string& value)
{
    m_cache[DX::ToLower(path)].m_values[DX::ToLower(id)] = value;
}

std::string LauncherCache::GetValue(const std::string& path, const std::string& id)
{
    std::string pathLower = DX::ToLower(path);
    std::string idLower = DX::ToLower(id);

    auto pathIter = m_cache.find(pathLower);
    if (pathIter != m_cache.end())
    {
        auto idIter = pathIter->second.m_values.find(idLower);
        if (idIter != pathIter->second.m_values.end())
        {
            return idIter->second;
        }
    }

    return "";
}
