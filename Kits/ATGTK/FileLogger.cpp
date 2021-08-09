//--------------------------------------------------------------------------------------
// FileLogger.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FileLogger.h"

#include <ctime>
#include <cstdarg>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

#if __cplusplus >= 201703L
#define FILESYSTEM_NAMESPACE(X) std::filesystem::X
#else
#define FILESYSTEM_NAMESPACE(X) std::experimental::filesystem::X
#endif

#if defined(_XBOX_ONE)
#include <xdk.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <direct.h>
#endif

using namespace ATG;

FileLogger::FileLogger() noexcept(false)
{
    m_outputQueue[0].reserve(1000);
    m_outputQueue[1].reserve(1000);
    m_currentOutputQueue = 0;
    m_outputThread = nullptr;

    m_baseFileName = L"";
    m_append = false;
    m_delayFlush = false;
    m_appendTxt = true;
    m_streamFile = nullptr;
    m_directoryOverride = L"";
    m_includeCompilerVersion = false;
    m_includeTimeStamp = false;
}

FileLogger::FileLogger(const std::wstring& location, bool append, bool includeTimeStamp, bool includeCompilerVersion, bool appendTxt, bool delayFlush, const std::wstring& directoryOverride, bool includeGaming) noexcept : FileLogger()
{
    m_baseFileName = location;
    m_append = append;
    m_appendTxt = appendTxt;
    m_delayFlush = delayFlush;
    m_directoryOverride = directoryOverride;
    m_includeGaming = includeGaming;
    m_includeTimeStamp = includeTimeStamp;
    m_includeCompilerVersion = includeCompilerVersion;
    StartupLogger();
}

FileLogger::FileLogger(const Flags& flags) noexcept
{
    FileLogger();
    m_baseFileName = flags.m_location;
    m_append = flags.m_appendToFile;
    m_appendTxt = flags.m_appendTxtToName;
    m_delayFlush = flags.m_delayFlush;
    m_directoryOverride = flags.m_directoryOverride;
    m_includeGaming = flags.m_includeGaming;
    m_includeCompilerVersion = flags.m_includeCompilerVersion;
    StartupLogger();
}

FileLogger::~FileLogger()
{
    ShutdownLogger();
}

#ifdef _GAMING_DESKTOP
// By default policy the location of the logs on desktop should be in the 'logs' directory located next to the solution file.
// This is the default current directory for Win32, however not for GDK Desktop so we have to search for it
FILESYSTEM_NAMESPACE(path) FindSolutionFileLocation()
{
    FILESYSTEM_NAMESPACE(path) toret(FILESYSTEM_NAMESPACE(current_path)());

    // early skip if we happen to be in the windows directory which is never right
    if (strncmp(toret.string().c_str(), "C:\\WINDOWS", 10) != 0)
    {
        for (auto& p : FILESYSTEM_NAMESPACE(directory_iterator)(toret))
        {
            if (FILESYSTEM_NAMESPACE(is_regular_file)(p.path()))
            {
                if (p.path().extension().string() == ".sln")
                    return toret;
            }
        }
    }

    wchar_t processPath[256] = {};
    GetModuleFileNameW(nullptr, processPath, 256);
    toret = processPath;
    toret = toret.parent_path();

    FILESYSTEM_NAMESPACE(path) curDir(toret);
    while ((curDir != curDir.root_path()) && (!curDir.empty()))
    {
        for (auto& p : FILESYSTEM_NAMESPACE(directory_iterator)(curDir))
        {
            if (FILESYSTEM_NAMESPACE(is_regular_file)(p.path()))
            {
                if (p.path().extension().string() == ".sln")
                    return curDir;
            }
        }
        curDir = curDir.parent_path();
    }

    return toret;
}
#endif

void FileLogger::OpenLogFile()
{
    FILESYSTEM_NAMESPACE(path) baseFilePath(m_baseFileName);
    FILESYSTEM_NAMESPACE(path) baseFilePathExtension(baseFilePath.extension());
    if (m_appendTxt)
        baseFilePathExtension = L".txt";
    baseFilePath.replace_extension();
    FILESYSTEM_NAMESPACE(path) fullFileName;

#if defined(_GAMING_XBOX)
    if (m_directoryOverride.size() != 0)
    {
        fullFileName = L"d:\\";
        fullFileName += m_directoryOverride;
        fullFileName += L"\\";
    }
    else
    {
        fullFileName = L"d:\\";
    }
#else
    if (m_directoryOverride.size() != 0)
    {
        _wmkdir(m_directoryOverride.c_str());
        fullFileName = m_directoryOverride;
        fullFileName += L"\\";
    }
    else
    {
#ifdef _GAMING_DESKTOP
        fullFileName = FindSolutionFileLocation();
        fullFileName /= L"Logs";
#else
        fullFileName = L"Logs";
#endif
        _mkdir(fullFileName.string().c_str());
        fullFileName += L"\\";
    }
#endif

    fullFileName += baseFilePath;
    {
        std::wstring pathAsString = fullFileName;
        auto findLoc = pathAsString.find_last_not_of(L' ');
        if (findLoc != std::wstring::npos)
            pathAsString = pathAsString.substr(0, findLoc + 1);
        fullFileName = pathAsString;
    }
    if (m_includeCompilerVersion)
    {
        wchar_t temp[32];
        _itow_s<32>(_MSC_FULL_VER, temp, 10);
        fullFileName += L"_";
        fullFileName += temp;
    }

    fullFileName += baseFilePathExtension;

    m_fullFileName = fullFileName;
    std::ios_base::openmode fred = std::ios_base::out;
    if (m_append)
        fred += std::ios_base::app;
    m_streamFile = new std::basic_fstream<wchar_t>(m_fullFileName, fred);
}

void FileLogger::ShutdownLogger()
{
    m_delayFlush = false;

    if (m_outputThread)
    {
        assert(m_outputThread->get_id() != std::this_thread::get_id());
        {
            m_killFlag.store(1);		// if it's the current thread the next we'll either reset to 0 or die next time through the loop
            m_outputThread->join();
        }
        DumpQueue(m_currentOutputQueue);
        DumpQueue(!m_currentOutputQueue);
        {
            m_outputThread = nullptr;
        }
    }

    delete m_streamFile;
    m_streamFile = nullptr;
}

void FileLogger::StartupLogger()
{
    OpenLogFile();
    m_killFlag.store(0);
    assert(!m_outputThread);
    CreateSaveLogThread();
}

bool FileLogger::ResetLogFile(bool append, const std::wstring& location)
{
    ShutdownLogger();
    if (location.size() != 0)
        m_baseFileName = location;
    m_append = append;
    StartupLogger();
    return true;
}

void FileLogger::DumpQueue(uint32_t queueIndex)
{
    if (m_delayFlush)
        return;

    for (auto& iter : m_outputQueue[queueIndex])
    {
        *m_streamFile << iter.c_str();
        *m_streamFile << std::endl;
    }
    m_outputQueue[queueIndex].clear();
}

void FileLogger::CreateSaveLogThread(void)
{
    assert(!m_outputThread);
    m_outputThread = new std::thread(&FileLogger::SaveLogThread, this);
}

void FileLogger::FormattedLog(const wchar_t* logLine, ...)
{
    size_t destSize = wcslen(logLine) * 2;
    std::wstring formattedLogLine;
    va_list vaList;
    va_start(vaList, logLine);
    {
        std::unique_ptr<wchar_t> formatted;

        formatted.reset(new wchar_t[destSize]);
        int32_t errorCode = vswprintf(formatted.get(), destSize, logLine, vaList);
        if (errorCode < 0 || errorCode >= static_cast<int32_t> (destSize))
            formattedLogLine = logLine;
        else
            formattedLogLine = std::wstring(formatted.get());
    }
    va_end(vaList);

    Log(formattedLogLine);
}

void FileLogger::Log(const std::wstring& logLine)
{
    if (m_prefix.empty() && !m_includeTimeStamp)
    {
        std::lock_guard<std::mutex> lg(m_queueCrit);
        m_outputQueue[m_currentOutputQueue].emplace(m_outputQueue[m_currentOutputQueue].end(), logLine);
    }
    else
    {
        std::wstringstream fullLogString;
        if (!m_prefix.empty())
        {
            fullLogString << m_prefix;
        }

        if (m_includeTimeStamp)
        {
            fullLogString << L"[";

            std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::tm calender;
            localtime_s(&calender, &now_c);
            fullLogString << std::put_time<wchar_t>(&calender, L"%F %T");

            fullLogString << L"] ";
        }
        fullLogString << logLine.c_str();

        {
            std::lock_guard<std::mutex> lg(m_queueCrit);
            m_outputQueue[m_currentOutputQueue].emplace(m_outputQueue[m_currentOutputQueue].end(), fullLogString.str());
        }
    }
}

void FileLogger::SaveLogThread(void)
{
    while (m_killFlag.load() == 0)
    {
        if (!m_delayFlush)
        {
            uint32_t outputQueue = m_currentOutputQueue;
            {
                std::lock_guard<std::mutex> lg(m_queueCrit);
                m_currentOutputQueue = !m_currentOutputQueue;
            }
            DumpQueue(outputQueue);
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}
