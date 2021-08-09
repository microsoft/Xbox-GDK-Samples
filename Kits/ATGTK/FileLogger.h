//--------------------------------------------------------------------------------------
// FileLogger.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <fstream>

namespace ATG
{
    class FileLogger
    {
    public:
        struct Flags
        {
            std::wstring m_location;
            std::wstring m_directoryOverride;
            bool m_appendToFile;
            bool m_includeCompilerVersion;
            bool m_includeTimeStamp;
            bool m_appendTxtToName;
            bool m_delayFlush;
            bool m_includeGaming;
            Flags(const std::wstring& loc = L"", bool append = false, bool includeTimeStamp = false, bool includeCompilerVersion = false, bool appendTxt = false, bool delayFlush = true, const std::wstring& directoryOverride = L"", bool includeGaming = true) :
                m_location(loc), m_directoryOverride(directoryOverride), m_appendToFile(append), m_includeCompilerVersion(includeCompilerVersion), m_includeTimeStamp(includeTimeStamp), m_appendTxtToName(appendTxt), m_delayFlush(delayFlush), m_includeGaming(includeGaming)
            {}
        };
    protected:
        std::thread* m_outputThread;
        std::mutex m_queueCrit;
        std::vector<std::wstring> m_outputQueue[2];
        uint32_t m_currentOutputQueue;
        std::atomic<uint32_t> m_killFlag;
        std::wstring m_prefix;

        std::basic_fstream<wchar_t>* m_streamFile;
        std::wstring m_baseFileName;
        std::wstring m_fullFileName;
        std::wstring m_directoryOverride;
        bool m_append;
        bool m_appendTxt;
        bool m_delayFlush;
        bool m_includeTimeStamp;
        bool m_includeGaming;
        bool m_includeCompilerVersion;

        virtual void ShutdownLogger();
        virtual void StartupLogger();

        void SaveLogThread(void);
        void CreateSaveLogThread(void);

        virtual void DumpQueue(uint32_t queueIndex);
        virtual bool CanDumpQueue(uint32_t /*queueIndex*/) { return !m_delayFlush; }
        void OpenLogFile();

    public:
        FileLogger(const FileLogger& rhs) = delete;
        FileLogger& operator= (const FileLogger& rhs) = delete;
        FileLogger& operator= (const FileLogger&& rhs) = delete;
        FileLogger() noexcept(false);
        FileLogger(const std::wstring& location, bool append, bool includeTimeStamp = false, bool includeCompilerVersion = false, bool appendTxt = false, bool delayFlush = true, const std::wstring& directoryOverride = L"", bool includeGaming = true) noexcept;
        FileLogger(const Flags& flags) noexcept;
        virtual ~FileLogger();

        void SetPrefix(const std::wstring& newPrefix) { m_prefix = newPrefix; }

        bool ResetLogFile(bool append, const std::wstring& location = L"");
        void Log(const std::wstring& logLine);
        void FormattedLog(const wchar_t* logLine, ...);
    };
}
