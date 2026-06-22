//--------------------------------------------------------------------------------------
// LauncherCache.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace ATG
{

    // A simple cache class that contains arbitrary cached strings for a specified path (case-insensitive) and
    // provides operations to save, load, and reset the cache.
    class LauncherCache
    {
    public:

        struct PathCacheData
        {
            std::map<std::string, std::string>  m_values;
        };

    public:

        LauncherCache(const char* cacheFilePath);

        bool Load();
        bool Save();
        bool Reset(bool save = false, bool forceOnSaveFailure = true);

        void SetValue(const std::string& path, const std::string& id, const std::string& value);
        std::string GetValue(const std::string& path, const std::string& id);

    protected:

        std::map<std::string, PathCacheData>    m_cache;
        std::string                             m_cacheFilePath;
    };

}
