#pragma once
//--------------------------------------------------------------------------------------
// File: Logger.h
//
// Authored by: ATG
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

/*
    How do I use this?
    ------------------

    1) Declare the log:
       a) If using inside a class, add "DECLARE_CLASS_LOG();" to your class declaration (i.e. header file)
       b) If using outside a class, use the "DECLARE_LOG();" macro instead

    2) Initialize the log:
       a) If using inside of a class, add "INITIALIZE_CLASS_LOG(classname, level);" to your class implementation (i.e. cpp file), where "level" defines the severity as defined below:
       b) If using outside of a class, use the "INITIALIZE_LOG(logname, level);" macro instead, where "level" defines the severity, as follows:
          ATG::Logger::All
          ATG::Logger::Trace
          ATG::Logger::Debug
          ATG::Logger::Info
          ATG::Logger::Warn
          ATG::Logger::Error
          ATG::Logger::Critical
       c) If you would like to override how the log item is formatted and how it gets output, use the "INITIALIZE(_CLASS)_LOG_EX(classname, level, formatFunc, outputFunc)" as follows:
          INITIALIZE_CLASS_LOG_EXT(Sample, ATG::LoggerLevel::Debug,
            [](LogItem& item) { sprintf(item.finalLine.data(), "-- %s --\n", item.formatted); },
            [](std::string const& str) { OutputDebugStringA(str.c_str()); });
       d) You may also override these methods at runtime using the "LOGGER_SET_FORMATFUNC" and "LOGGER_SET_OUTPUTFUNC" macros.
       e) To use the default formatter or output method, pass Logger::DefaultLogFormat for the formatFunc param, or Logger::DefaultLogOutput for the outputFunc param:
          INITIALIZE_CLASS_LOG_EXT(Sample, ATG::LoggerLevel::Debug, Logger::DefaultLogFormat, Logger::DefaultLogOutput);

    3) Use the "LOGGER_LEVEL(_IF / _SCOPED / _FUNC)" macros to write data to the log, where LEVEL is one of the following: TRACE, DEBUG, INFO, WARN, ERROR, CRIT
       LOGGER_DEBUG("This is a Debug Log message");
       LOGGER_CRIT("This is a Critical Log message");
       LOGGER_INFO_IF(x > 5, "This message displays if x is greater than 5");
       LOGGER_LEVEL_SCOPED("tag", "This message will be tagged with the MyScope identifier");
       LOGGER_LEVEL_FUNC("This message will be tagged with the current method name");
       a) You can also set scopes with the "LOGGER_SCOPE(scope);" macro:
          LOGGER_SCOPE("NewScope");
          LOGGER_INFO("This is a message that will be tagged with the NewScope identifier");
 */

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <string_view>

#define LOGGER_LEVEL_TRACE 0
#define LOGGER_LEVEL_DEBUG 1
#define LOGGER_LEVEL_INFO  2
#define LOGGER_LEVEL_WARN  3
#define LOGGER_LEVEL_ERROR 4
#define LOGGER_LEVEL_CRIT  5
#define LOGGER_LEVEL_DISABLE 1024

#define LOGGER_MIN_LEVEL LOGGER_LEVEL_DEBUG

namespace ATG
{
    namespace LoggerConstants
    {
        constexpr const int     c_maxTagCount           = 8;
        constexpr const size_t  c_maxTagBufferLength    = 128;
        constexpr const size_t  c_baseLogLineSize       = 256;
        constexpr const char    c_tagDelimiter          = '.';
        constexpr const size_t  c_defaultLogLineSize    = c_baseLogLineSize + c_maxTagBufferLength;
    }

    struct LogItem
    {

        std::chrono::time_point<std::chrono::system_clock> timePoint;
        std::string formattedTime;
        std::string level;
        std::string tag;

        size_t size;
        std::string formatted;
        std::string finalLine;
    };

    class Logger
    {
        using string_view   = std::string_view;
        using LogOutputFunc = std::function<void(std::string const&)>;
        using LogFormatFunc = std::function<void(LogItem&)>;

    public:
        enum class LoggerLevel : uint32_t
        {
            All = 0,
            Trace = 0,
            Debug = 1,
            Info = 2,
            Warn = 3,
            Error = 4,
            Critical = 5,
            Disabled = 1024
        };

    public:
        // default formatting and output functions
        static void DefaultLogFormat(LogItem& item)
        {
            sprintf_s(item.finalLine.data(), item.size,
                "!> %s %s %s: %s\n",
                item.formattedTime.c_str(), item.level.c_str(), item.tag.c_str(), item.formatted.c_str());
        }

        static void DefaultLogOutput(std::string const& str)
        {
            OutputDebugStringA(str.c_str());
        }

        static std::string DefaultTimeFormat(std::chrono::time_point<std::chrono::system_clock> timePoint)
        {
            auto time = std::chrono::system_clock::to_time_t(timePoint);
            tm localTime;
            localtime_s(&localTime, &time);

            char timeBuffer[256];
            strftime(timeBuffer, 256, "%Y-%m-%d %H-%M-%S", &localTime);

            int ms = std::chrono::time_point_cast<std::chrono::milliseconds>(timePoint).time_since_epoch().count() % 1000;
            char finalTimeBuffer[256];
            sprintf_s(finalTimeBuffer, "%s.%03d", timeBuffer, ms);
            return std::string(finalTimeBuffer);
        }

    public:
        /// <summary>
        /// Manages a string containing tags, joined by '.': Tag1.Tag2.Tag3
        /// </summary>
        class LogTag
        {
        public:
            LogTag() : m_tagCount(0), m_tagOffsets{}, buf{}
            {}

            LogTag(const char *name) : m_tagCount(0), m_tagOffsets{}, buf{}
            {
                PushTag(name);
            }

            string_view GetTag() const { return string_view(buf, m_tagOffsets[m_tagCount]); }

            void PushTag(string_view tag)
            {
                assert(m_tagCount + 1 < LoggerConstants::c_maxTagCount);
                assert(m_tagOffsets[m_tagCount] + tag.size() + 1 < LoggerConstants::c_maxTagBufferLength);

                uint32_t index = m_tagOffsets[m_tagCount];
                if (m_tagCount > 0) { buf[index++] = LoggerConstants::c_tagDelimiter; }  // Add a delimiter if not the first tag
                memcpy(&buf[index], tag.data(), tag.size());                             // Copy the new tag into the buffer
                m_tagOffsets[++m_tagCount] = uint8_t(index + tag.size());
                buf[m_tagOffsets[m_tagCount]] = '\0';                                    // Terminate the string at the new length
            }

            void PopTag()
            {
                if (m_tagCount > 0)
                {
                    m_tagOffsets[m_tagCount--] = 0;
                }
            }

        public:
            /// <summary>
            /// Helper class to automatically pop tags once scope is left
            /// </summary>
            class ScopedTag
            {
            public:
                ScopedTag(LogTag& logTag, string_view tag) : m_logTag(logTag)
                {
                    m_logTag.PushTag(tag);
                }

                ~ScopedTag()
                {
                    m_logTag.PopTag();
                }

            private:
                LogTag& m_logTag;
            };

            ScopedTag GetScoped(string_view tag)
            {
                return ScopedTag(*this, std::forward<string_view>(tag));
            }

        private:
            int m_tagCount;                                          // The current number of tags in the buffer
            uint32_t m_tagOffsets[LoggerConstants::c_maxTagCount];   // Offsets are to the *next* tag (aka end of the existing tag)
            char buf[LoggerConstants::c_maxTagBufferLength];         // Holds the entire tag
        };

        /// <summary>
        /// This simple struct is used for type safety for log calls that specify an inline-scoped tag
        /// </summary>
        struct Tag
        {
            explicit Tag(const char* tag) : tag(tag) {}
            Tag() = delete;
            const char* tag;
        };

    public:
        Logger(const char *name, LoggerLevel level, LogFormatFunc format = DefaultLogFormat, LogOutputFunc output = DefaultLogOutput) : m_tag(name), m_level(level), m_format(format), m_output(output)
        {
        }

    public:
        template<size_t Size, typename... Args> void Trace(string_view format, const Args&... args) { Log<Size>(LoggerLevel::Trace, format, args...); }
        template<size_t Size, typename... Args> void Trace(Tag tag, string_view format, const Args&... args) { Log<Size>(LoggerLevel::Trace, tag, format, args...); }
        template<size_t Size, typename... Args> void Debug(string_view format, const Args&... args) { Log<Size>(LoggerLevel::Debug, format, args...); }
        template<size_t Size, typename... Args> void Debug(Tag tag, string_view format, const Args&... args) { Log<Size>(LoggerLevel::Debug, tag, format, args...); }
        template<size_t Size, typename... Args> void Info(string_view format, const Args&... args) { Log<Size>(LoggerLevel::Info, format, args...); }
        template<size_t Size, typename... Args> void Info(Tag tag, string_view format, const Args&... args) { Log<Size>(LoggerLevel::Info, tag, format, args...); }
        template<size_t Size, typename... Args> void Warn(string_view format, const Args&... args) { Log<Size>(LoggerLevel::Warn, format, args...); }
        template<size_t Size, typename... Args> void Warn(Tag tag, string_view format, const Args&... args) { Log<Size>(LoggerLevel::Warn, tag, format, args...); }
        template<size_t Size, typename... Args> void Error(string_view format, const Args&... args) { Log<Size>(LoggerLevel::Error, format, args...); }
        template<size_t Size, typename... Args> void Error(Tag tag, string_view format, const Args&... args) { Log<Size>(LoggerLevel::Error, tag, format, args...); }
        template<size_t Size, typename... Args> void Crit(string_view format, const Args&... args) { Log<Size>(LoggerLevel::Critical, format, args...); }
        template<size_t Size, typename... Args> void Crit(Tag tag, string_view format, const Args&... args) { Log<Size>(LoggerLevel::Critical, tag, format, args...); }

        void PushTag(string_view tag) { m_tag.PushTag(std::forward<string_view>(tag)); }
        void PopTag() { m_tag.PopTag(); }
        LogTag::ScopedTag PushScoped(string_view tag) { return m_tag.GetScoped(std::forward<string_view>(tag)); }
        void SetFormatFunc(LogFormatFunc func) { m_format = func; }
        void SetOutputFunc(LogOutputFunc func) { m_output = func; }

    private:
        template<size_t Size, typename... Args>
        void Log(LoggerLevel level, string_view format, const Args&... args)
        {
            if  (level < m_level) { return; }

            LogItem item;
            item.size = Size;
            item.finalLine.resize(Size);

            item.timePoint = std::chrono::system_clock::now();
            item.formattedTime = DefaultTimeFormat(item.timePoint);

            item.formatted.resize(Size);
            sprintf_s(item.formatted.data(), Size, format.data(), args...);

            item.level = std::string(c_levelNames[uint32_t(level)]);
            item.tag = m_tag.GetTag();

            m_format(item);

            m_output(item.finalLine);
        }

        template<size_t Size, typename... Args>
        void Log(LoggerLevel level, Tag tag, string_view format, const Args&... args)
        {
            m_tag.PushTag(tag.tag);
            Log<Size>(level, format, args...);
            m_tag.PopTag();
        }

    private:
        const char* c_levelNames[6] = { "[TRACE]", "[DEBUG]", "[INFO]", "[WARN]", "[ERROR]", "[CRIT]" };
        LogTag m_tag;
        LoggerLevel m_level;
        LogFormatFunc m_format;
        LogOutputFunc m_output;
    };

    using LoggerLevel = Logger::LoggerLevel;
};

// Macros for making logging consistent

#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_TRACE
#define LOGGER_TRACE_EXT(Size, ...) LOG_.Trace<Size>(__VA_ARGS__)
#define LOGGER_TRACE(...) LOGGER_TRACE_EXT(ATG::LoggerConstants::c_defaultLogLineSize, __VA_ARGS__)
#define LOGGER_TRACE_IF(condition, ...) do { if (condition) { LOGGER_TRACE(__VA_ARGS__); } } while (0)
#define LOGGER_TRACE_SCOPED(tag, ...) LOGGER_TRACE(LOGGER_ISCOPE(tag), __VA_ARGS__)
#define LOGGER_TRACE_FUNC(...) LOGGER_TRACE(LOGGER_ISCOPE(__func__), __VA_ARGS__)
#define LOGGER_TRACE_FUNC_EXT(Size, ...) LOGGER_TRACE_EXT(Size, LOGGER_ISCOPE(__func__), __VA_ARGS__)
#else
#define LOGGER_TRACE_EXT(...)
#define LOGGER_TRACE(...)
#define LOGGER_TRACE_IF(...)
#define LOGGER_TRACE_SCOPED(...)
#define LOGGER_TRACE_FUNC(...)
#define LOGGER_TRACE_FUNC_EXT(...)
#endif

#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_DEBUG
#define LOGGER_DEBUG_EXT(Size, ...) LOG_.Debug<Size>(__VA_ARGS__)
#define LOGGER_DEBUG(...) LOGGER_DEBUG_EXT(ATG::LoggerConstants::c_defaultLogLineSize, __VA_ARGS__)
#define LOGGER_DEBUG_IF(condition, ...) do { if (condition) { LOGGER_DEBUG(__VA_ARGS__); } } while (0)
#define LOGGER_DEBUG_SCOPED(tag, ...) LOGGER_DEBUG(LOGGER_ISCOPE(tag), __VA_ARGS__)
#define LOGGER_DEBUG_FUNC(...) LOGGER_DEBUG(LOGGER_ISCOPE(__func__), __VA_ARGS__)
#define LOGGER_DEBUG_FUNC_EXT(Size, ...) LOGGER_DEBUG_EXT(Size, LOGGER_ISCOPE(__func__), __VA_ARGS__)
#else
#define LOGGER_DEBUG_EXT(...)
#define LOGGER_DEBUG(...)
#define LOGGER_DEBUG_IF(...)
#define LOGGER_DEBUG_SCOPED(...)
#define LOGGER_DEBUG_FUNC(...)
#define LOGGER_DEBUG_FUNC_EXT(...)
#endif

#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_INFO
#define LOGGER_INFO_EXT(Size, ...) LOG_.Info<Size>(__VA_ARGS__)
#define LOGGER_INFO(...) LOGGER_INFO_EXT(ATG::LoggerConstants::c_defaultLogLineSize, __VA_ARGS__)
#define LOGGER_INFO_IF(condition, ...) do { if (condition) { LOGGER_INFO(__VA_ARGS__); } } while (0)
#define LOGGER_INFO_SCOPED(tag, ...) LOGGER_INFO(LOGGER_ISCOPE(tag), __VA_ARGS__)
#define LOGGER_INFO_FUNC(...) LOGGER_INFO(LOGGER_ISCOPE(__func__), __VA_ARGS__)
#define LOGGER_INFO_FUNC_EXT(Size, ...) LOGGER_INFO_EXT(Size, LOGGER_ISCOPE(__func__), __VA_ARGS__)
#else
#define LOGGER_INFO_EXT(...)
#define LOGGER_INFO(...)
#define LOGGER_INFO_IF(...)
#define LOGGER_INFO_SCOPED(...)
#define LOGGER_INFO_FUNC(...)
#define LOGGER_INFO_FUNC_EXT(...)
#endif

#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_WARN
#define LOGGER_WARN_EXT(Size, ...) LOG_.Warn<Size>(__VA_ARGS__)
#define LOGGER_WARN(...) LOGGER_WARN_EXT(ATG::LoggerConstants::c_defaultLogLineSize, __VA_ARGS__)
#define LOGGER_WARN_IF(condition, ...) do { if (condition) { LOGGER_WARN(__VA_ARGS__); } } while (0)
#define LOGGER_WARN_SCOPED(tag, ...) LOGGER_WARN(LOGGER_ISCOPE(tag), __VA_ARGS__)
#define LOGGER_WARN_FUNC(...) LOGGER_WARN(LOGGER_ISCOPE(__func__), __VA_ARGS__)
#define LOGGER_WARN_FUNC_EXT(Size, ...) LOGGER_WARN_EXT(Size, LOGGER_ISCOPE(__func__), __VA_ARGS__)
#else
#define LOGGER_WARN_EXT(...)
#define LOGGER_WARN(...)
#define LOGGER_WARN_IF(...)
#define LOGGER_WARN_SCOPED(...)
#define LOGGER_WARN_FUNC(...)
#define LOGGER_WARN_FUNC_EXT(...)
#endif

#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_ERROR
#define LOGGER_ERROR_EXT(Size, ...) LOG_.Error<Size>(__VA_ARGS__)
#define LOGGER_ERROR(...) LOGGER_ERROR_EXT(ATG::LoggerConstants::c_defaultLogLineSize, __VA_ARGS__)
#define LOGGER_ERROR_IF(condition, ...) do { if (condition) { LOGGER_ERROR(__VA_ARGS__); } } while (0)
#define LOGGER_ERROR_SCOPED(tag, ...) LOGGER_ERROR(LOGGER_ISCOPE(tag), __VA_ARGS__)
#define LOGGER_ERROR_FUNC(...) LOGGER_ERROR(LOGGER_ISCOPE(__func__), __VA_ARGS__)
#define LOGGER_ERROR_FUNC_EXT(Size, ...) LOGGER_ERROR_EXT(Size, LOGGER_ISCOPE(__func__), __VA_ARGS__)
#else
#define LOGGER_ERROR_EXT(...)
#define LOGGER_ERROR(...)
#define LOGGER_ERROR_IF(...)
#define LOGGER_ERROR_SCOPED(...)
#define LOGGER_ERROR_FUNC(...)
#define LOGGER_ERROR_FUNC_EXT(...)
#endif

#if LOGGER_MIN_LEVEL <= LOGGER_LEVEL_CRIT
#define LOGGER_CRIT_EXT(Size, ...) LOG_.Crit<Size>(__VA_ARGS__)
#define LOGGER_CRIT(...) LOGGER_CRIT_EXT(ATG::LoggerConstants::c_defaultLogLineSize, __VA_ARGS__)
#define LOGGER_CRIT_IF(condition, ...) do { if (condition) { LOGGER_CRIT(__VA_ARGS__); } } while (0)
#define LOGGER_CRIT_SCOPED(tag, ...) LOGGER_CRIT(LOGGER_ISCOPE(tag), __VA_ARGS__)
#define LOGGER_CRIT_FUNC(...) LOGGER_CRIT(LOGGER_ISCOPE(__func__), __VA_ARGS__)
#define LOGGER_CRIT_FUNC_EXT(Size, ...) LOGGER_CRIT_EXT(Size, LOGGER_ISCOPE(__func__), __VA_ARGS__)
#else
#define LOGGER_CRIT_EXT(...)
#define LOGGER_CRIT(...)
#define LOGGER_CRIT_IF(...)
#define LOGGER_CRIT_SCOPED(...)
#define LOGGER_CRIT_FUNC(...)
#define LOGGER_CRIT_FUNC_EXT(...)
#endif

#if LOGGER_MIN_LEVEL < LOGGER_LEVEL_DISABLE
#define INITIALIZE_LOG_EXT(name, level, outputFunc) namespace { ATG::Logger LOG_ = ATG::Logger(#name, level, outputFunc); }
#define INITIALIZE_LOG(name, level) namespace { ATG::Logger LOG_ = ATG::Logger(#name, level); }
#define INITIALIZE_CLASS_LOG_EXT(classname, level, formatFunc, outputFunc) ATG::Logger classname::LOG_ = ATG::Logger(#classname, level, formatFunc, outputFunc)
#define INITIALIZE_CLASS_LOG(classname, level) ATG::Logger classname::LOG_ = ATG::Logger(#classname, level)
#define DECLARE_CLASS_LOG(...)                          \
private:                                                \
    static ATG::Logger LOG_

#define INITIALIZE_LOG_TRACE(name) INITIALIZE_LOG(name, ATG::LoggerLevel::Trace)
#define INITIALIZE_CLASS_LOG_TRACE(name) INITIALIZE_CLASS_LOG(name, ATG::LoggerLevel::Trace)
#define INITIALIZE_LOG_DEBUG(name) INITIALIZE_LOG(name, ATG::LoggerLevel::Debug)
#define INITIALIZE_CLASS_LOG_DEBUG(name) INITIALIZE_CLASS_LOG(name, ATG::LoggerLevel::Debug)
#define INITIALIZE_LOG_INFO(name) INITIALIZE_LOG(name, ATG::LoggerLevel::Info)
#define INITIALIZE_CLASS_LOG_INFO(name) INITIALIZE_CLASS_LOG(name, ATG::LoggerLevel::Info)
#define INITIALIZE_LOG_WARN(name) INITIALIZE_LOG(name, ATG::LoggerLevel::Warn)
#define INITIALIZE_CLASS_LOG_WARN(name) INITIALIZE_CLASS_LOG(name, ATG::LoggerLevel::Warn)
#define INITIALIZE_LOG_ERROR(name) INITIALIZE_LOG(name, ATG::LoggerLevel::Error)
#define INITIALIZE_CLASS_LOG_ERROR(name) INITIALIZE_CLASS_LOG(name, ATG::LoggerLevel::Error)
#define INITIALIZE_LOG_CRIT(name) INITIALIZE_LOG(name, ATG::LoggerLevel::Critical)
#define INITIALIZE_CLASS_LOG_CRIT(name) INITIALIZE_CLASS_LOG(name, ATG::LoggerLevel::Critical)
#define LOGGER_SCOPE(name) auto TEMP_SCOPED_LOG = LOG_.PushScoped(name);  TEMP_SCOPED_LOG
#define LOGGER_ISCOPE(tag) ATG::Logger::Tag(tag)
#define LOGGER_FUNC() ATG::Logger::Tag(__func__)
#define LOGGER_SET_FORMATFUNC(func) LOG_.SetFormatFunc(func)
#define LOGGER_SET_OUTPUTFUNC(func) LOG_.SetOutputFunc(func)
#else
#define INITIALIZE_LOG(...)
#define INITIALIZE_CLASS_LOG(...)
#define DECLARE_CLASS_LOG(...)
#define INITIALIZE_LOG_TRACE(...)
#define INITIALIZE_CLASS_LOG_TRACE(...)
#define INITIALIZE_LOG_DEBUG(...)
#define INITIALIZE_CLASS_LOG_DEBUG(...)
#define INITIALIZE_LOG_INFO(...)
#define INITIALIZE_CLASS_LOG_INFO(...)
#define INITIALIZE_LOG_WARN(...)
#define INITIALIZE_CLASS_LOG_WARN(...)
#define INITIALIZE_LOG_ERROR(...)
#define INITIALIZE_CLASS_LOG_ERROR(...)
#define INITIALIZE_LOG_CRIT(...)
#define INITIALIZE_CLASS_LOG_CRIT(...)

#define LOGGER_SCOPE(...)
#define LOGGER_ISCOPE(...)
#define LOGGER_FUNC(...)
#define LOGGER_SET_FORMATFUNC(...)
#define LOGGER_SET_OUTPUTFUNC(...)

#endif
