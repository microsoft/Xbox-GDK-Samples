#pragma once
//--------------------------------------------------------------------------------------
// File: UILog.h
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

#include "UICore.h"
#include "UIDebugConfig.h"

#include <string_view>

NAMESPACE_ATG_UITK_BEGIN

namespace UILogConstants
{
    constexpr const int     c_maxTagCount           = 8;
    constexpr const size_t  c_maxTagBufferLength    = 128;
    constexpr const size_t  c_baseLogLineSize       = 256;
    constexpr const char    c_tagDelimiter          = '.';
    constexpr const size_t  c_defaultLogLineSize    = c_baseLogLineSize + c_maxTagBufferLength;
}

class UILog
{
    using string_view   = std::string_view;
    using LogOutputFunc = std::function<void(const char*)>;

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
            assert(m_tagCount + 1 < UILogConstants::c_maxTagCount);
            assert(m_tagOffsets[m_tagCount] + tag.size() + 1 < UILogConstants::c_maxTagBufferLength);

            uint32_t index = m_tagOffsets[m_tagCount];
            if (m_tagCount > 0) { buf[index++] = UILogConstants::c_tagDelimiter; }  // Add a delimiter if not the first tag
            memcpy(&buf[index], tag.data(), tag.size());                            // Copy the new tag into the buffer
            m_tagOffsets[++m_tagCount] = uint8_t(index + tag.size());            
            buf[m_tagOffsets[m_tagCount]] = '\0';                                   // Terminate the string at the new length
        }

        void PopTag()
        {
            if (m_tagCount > 0)
            {
                m_tagOffsets[m_tagCount--] = 0;
                buf[m_tagOffsets[m_tagCount]] = '\0';                               // Terminate the string at the new length
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
        int m_tagCount;                                         // The current number of tags in the buffer
        uint32_t m_tagOffsets[UILogConstants::c_maxTagCount];   // Offsets are to the *next* tag (aka end of the existing tag)
        char buf[UILogConstants::c_maxTagBufferLength];         // Holds the entire tag
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
    enum class UILogLevel : uint32_t
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
    UILog(const char *name, UILogLevel level, LogOutputFunc output = OutputDebugStringA) : m_tag(name), m_level(level), m_output(output)
    {
    }

public:
    template<size_t Size, typename... Args> void Trace(string_view format, const Args&... args) { Log<Size>(UILogLevel::Trace, format, args...); }
    template<size_t Size, typename... Args> void Trace(Tag tag, string_view format, const Args&... args) { Log<Size>(UILogLevel::Trace, tag, format, args...); }
    template<size_t Size, typename... Args> void Debug(string_view format, const Args&... args) { Log<Size>(UILogLevel::Debug, format, args...); }
    template<size_t Size, typename... Args> void Debug(Tag tag, string_view format, const Args&... args) { Log<Size>(UILogLevel::Debug, tag, format, args...); }
    template<size_t Size, typename... Args> void Info(string_view format, const Args&... args) { Log<Size>(UILogLevel::Info, format, args...); }
    template<size_t Size, typename... Args> void Info(Tag tag, string_view format, const Args&... args) { Log<Size>(UILogLevel::Info, tag, format, args...); }
    template<size_t Size, typename... Args> void Warn(string_view format, const Args&... args) { Log<Size>(UILogLevel::Warn, format, args...); }
    template<size_t Size, typename... Args> void Warn(Tag tag, string_view format, const Args&... args) { Log<Size>(UILogLevel::Warn, tag, format, args...); }
    template<size_t Size, typename... Args> void Error(string_view format, const Args&... args) { Log<Size>(UILogLevel::Error, format, args...); }
    template<size_t Size, typename... Args> void Error(Tag tag, string_view format, const Args&... args) { Log<Size>(UILogLevel::Error, tag, format, args...); }
    template<size_t Size, typename... Args> void Crit(string_view format, const Args&... args) { Log<Size>(UILogLevel::Critical, format, args...); }
    template<size_t Size, typename... Args> void Crit(Tag tag, string_view format, const Args&... args) { Log<Size>(UILogLevel::Critical, tag, format, args...); }

    void PushTag(string_view tag) { m_tag.PushTag(std::forward<string_view>(tag)); }
    void PopTag() { m_tag.PopTag(); }
    LogTag::ScopedTag PushScoped(string_view tag) { return m_tag.GetScoped(std::forward<string_view>(tag)); }

private:
    template<size_t Size, typename... Args>
    void Log(UILogLevel level, string_view format, const Args&... args)
    {
        if  (level < m_level) { return; }
        auto counter = GetLineCounter();
        auto time = GetAppLifetimeInMS();

        char line[Size];

        int written = sprintf_s(line,
            "!> %5d %7d %s %s: ",                                                           // Format the log line context
            counter, time, c_levelNames[uint32_t(level)], m_tag.GetTag().data());
        written += sprintf_s(line + written, Size - written, format.data(), args...);       // Append the log message
        line[written] = '\n';                                                               // Append a newline
        ++written;
        line[written] = '\0';                                                               // Terminate the string

        m_output(line);
    }

    template<size_t Size, typename... Args>
    void Log(UILogLevel level, Tag tag, string_view format, const Args&... args)
    {
        m_tag.PushTag(tag.tag);
        Log<Size>(level, format, args...);
        m_tag.PopTag();
    }

    // Gets a timestamp in milliseconds since initialization
    static int32_t GetAppLifetimeInMS();
    static int32_t GetLineCounter();

private:
    const char* c_levelNames[6] = { "[TRACE]", "[DEBUG]", "[INFO]", "[WARN]", "[ERROR]", "[CRIT]" };
    LogTag m_tag;
    UILogLevel m_level;
    LogOutputFunc m_output;
};

using UILogLevel = UILog::UILogLevel;

#if UILOG_MIN_LEVEL < UILOG_LEVEL_DISABLE
#define DECLARE_CLASS_LOG(...) static UILog LOG_
#else
#define DECLARE_CLASS_LOG(...)
#endif

class UIAssert
{
public:
    DECLARE_CLASS_LOG();
};

NAMESPACE_ATG_UITK_END

// Macros for making logging consistent

#if UILOG_MIN_LEVEL <= UILOG_LEVEL_TRACE
#define UILOG_TRACE_EXT(Size, ...) LOG_.Trace<Size>(__VA_ARGS__)
#define UILOG_TRACE(...) UILOG_TRACE_EXT(UILogConstants::c_defaultLogLineSize, __VA_ARGS__)
#define UILOG_TRACE_IF(condition, ...) do { if (condition) { UILOG_TRACE(__VA_ARGS__); } } while (0)
#define UILOG_TRACE_SCOPED(tag, ...) UILOG_TRACE(UILOG_ISCOPE(tag), __VA_ARGS__)
#define UILOG_TRACE_FUNC(...) UILOG_TRACE(UILOG_ISCOPE(__func__), __VA_ARGS__)
#define UILOG_TRACE_FUNC_EXT(Size, ...) UILOG_TRACE_EXT(Size, UILOG_ISCOPE(__func__), __VA_ARGS__)
#else
#define UILOG_TRACE_EXT(...)
#define UILOG_TRACE(...)
#define UILOG_TRACE_IF(...)
#define UILOG_TRACE_SCOPED(...)
#define UILOG_TRACE_FUNC(...)
#define UILOG_TRACE_FUNC_EXT(...)
#endif

#if UILOG_MIN_LEVEL <= UILOG_LEVEL_DEBUG
#define UILOG_DEBUG_EXT(Size, ...) LOG_.Debug<Size>(__VA_ARGS__)
#define UILOG_DEBUG(...) UILOG_DEBUG_EXT(UILogConstants::c_defaultLogLineSize, __VA_ARGS__)
#define UILOG_DEBUG_IF(condition, ...) do { if (condition) { UILOG_DEBUG(__VA_ARGS__); } } while (0)
#define UILOG_DEBUG_SCOPED(tag, ...) UILOG_DEBUG(UILOG_ISCOPE(tag), __VA_ARGS__)
#define UILOG_DEBUG_FUNC(...) UILOG_DEBUG(UILOG_ISCOPE(__func__), __VA_ARGS__)
#define UILOG_DEBUG_FUNC_EXT(Size, ...) UILOG_DEBUG_EXT(Size, UILOG_ISCOPE(__func__), __VA_ARGS__)
#else
#define UILOG_DEBUG_EXT(...)
#define UILOG_DEBUG(...)
#define UILOG_DEBUG_IF(...)
#define UILOG_DEBUG_SCOPED(...)
#define UILOG_DEBUG_FUNC(...)
#define UILOG_DEBUG_FUNC_EXT(...)
#endif

#if UILOG_MIN_LEVEL <= UILOG_LEVEL_INFO
#define UILOG_INFO_EXT(Size, ...) LOG_.Info<Size>(__VA_ARGS__)
#define UILOG_INFO(...) UILOG_INFO_EXT(UILogConstants::c_defaultLogLineSize, __VA_ARGS__)
#define UILOG_INFO_IF(condition, ...) do { if (condition) { UILOG_INFO(__VA_ARGS__); } } while (0)
#define UILOG_INFO_SCOPED(tag, ...) UILOG_INFO(UILOG_ISCOPE(tag), __VA_ARGS__)
#define UILOG_INFO_FUNC(...) UILOG_INFO(UILOG_ISCOPE(__func__), __VA_ARGS__)
#define UILOG_INFO_FUNC_EXT(Size, ...) UILOG_INFO_EXT(Size, UILOG_ISCOPE(__func__), __VA_ARGS__)
#else
#define UILOG_INFO_EXT(...)
#define UILOG_INFO(...)
#define UILOG_INFO_IF(...)
#define UILOG_INFO_SCOPED(...)
#define UILOG_INFO_FUNC(...)
#define UILOG_INFO_FUNC_EXT(...)
#endif

#if UILOG_MIN_LEVEL <= UILOG_LEVEL_WARN
#define UILOG_WARN_EXT(Size, ...) LOG_.Warn<Size>(__VA_ARGS__)
#define UILOG_WARN(...) UILOG_WARN_EXT(UILogConstants::c_defaultLogLineSize, __VA_ARGS__)
#define UILOG_WARN_IF(condition, ...) do { if (condition) { UILOG_WARN(__VA_ARGS__); } } while (0)
#define UILOG_WARN_SCOPED(tag, ...) UILOG_WARN(UILOG_ISCOPE(tag), __VA_ARGS__)
#define UILOG_WARN_FUNC(...) UILOG_WARN(UILOG_ISCOPE(__func__), __VA_ARGS__)
#define UILOG_WARN_FUNC_EXT(Size, ...) UILOG_WARN_EXT(Size, UILOG_ISCOPE(__func__), __VA_ARGS__)
#else
#define UILOG_WARN_EXT(...)
#define UILOG_WARN(...)
#define UILOG_WARN_IF(...)
#define UILOG_WARN_SCOPED(...)
#define UILOG_WARN_FUNC(...)
#define UILOG_WARN_FUNC_EXT(...)
#endif

#if UILOG_MIN_LEVEL <= UILOG_LEVEL_ERROR
#define UILOG_ERROR_EXT(Size, ...) LOG_.Error<Size>(__VA_ARGS__)
#define UILOG_ERROR(...) UILOG_ERROR_EXT(UILogConstants::c_defaultLogLineSize, __VA_ARGS__)
#define UILOG_ERROR_IF(condition, ...) do { if (condition) { UILOG_ERROR(__VA_ARGS__); } } while (0)
#define UILOG_ERROR_SCOPED(tag, ...) UILOG_ERROR(UILOG_ISCOPE(tag), __VA_ARGS__)
#define UILOG_ERROR_FUNC(...) UILOG_ERROR(UILOG_ISCOPE(__func__), __VA_ARGS__)
#define UILOG_ERROR_FUNC_EXT(Size, ...) UILOG_ERROR_EXT(Size, UILOG_ISCOPE(__func__), __VA_ARGS__)
#else
#define UILOG_ERROR_EXT(...)
#define UILOG_ERROR(...)
#define UILOG_ERROR_IF(...)
#define UILOG_ERROR_SCOPED(...)
#define UILOG_ERROR_FUNC(...)
#define UILOG_ERROR_FUNC_EXT(...)
#endif

#if UILOG_MIN_LEVEL <= UILOG_LEVEL_CRIT
#define UILOG_CRIT_EXT(Size, ...) LOG_.Crit<Size>(__VA_ARGS__)
#define UILOG_CRIT(...) UILOG_CRIT_EXT(UILogConstants::c_defaultLogLineSize, __VA_ARGS__)
#define UILOG_CRIT_IF(condition, ...) do { if (condition) { UILOG_CRIT(__VA_ARGS__); } } while (0)
#define UILOG_CRIT_SCOPED(tag, ...) UILOG_CRIT(UILOG_ISCOPE(tag), __VA_ARGS__)
#define UILOG_CRIT_FUNC(...) UILOG_CRIT(UILOG_ISCOPE(__func__), __VA_ARGS__)
#define UILOG_CRIT_FUNC_EXT(Size, ...) UILOG_CRIT_EXT(Size, UILOG_ISCOPE(__func__), __VA_ARGS__)
#else
#define UILOG_CRIT_EXT(...)
#define UILOG_CRIT(...)
#define UILOG_CRIT_IF(...)
#define UILOG_CRIT_SCOPED(...)
#define UILOG_CRIT_FUNC(...)
#define UILOG_CRIT_FUNC_EXT(...)
#endif

using assert_string = std::wstring;

#ifdef _DEBUG
#define UI_ERROR L"ERROR" && false
#define UI_INVALID_OPERATION L"INVALID_OPERATION" && false
#define UI_ASSERT(condition, message) (void)(                                                       \
            (!!(condition)) ||                                                              \
            (_wassert(( DX::Utf8ToWide(message).append(" " _CRT_WIDE(#condition)) ).c_str(), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \
        )
#else
#define UI_ERROR
#define UI_INVALID_OPERATION
#define UI_ASSERT(...)
#endif

#if UILOG_MIN_LEVEL < UILOG_LEVEL_DISABLE
#define INITIALIZE_LOG_EXT(name, level, outputFunc) namespace { UILog LOG_ = UILog(#name, level, outputFunc); }
#define INITIALIZE_LOG(name, level) namespace { UILog LOG_ = UILog(#name, level); }
#define INITIALIZE_CLASS_LOG_EXT(classname, level, outputFunc) UILog classname::LOG_ = UILog(#classname, level, outputFunc)
#define INITIALIZE_CLASS_LOG(classname, level) UILog classname::LOG_ = UILog(#classname, level)
#define INITIALIZE_LOG_TRACE(name) INITIALIZE_LOG(name, UILogLevel::Trace)
#define INITIALIZE_CLASS_LOG_TRACE(name) INITIALIZE_CLASS_LOG(name, UILogLevel::Trace)
#define INITIALIZE_LOG_DEBUG(name) INITIALIZE_LOG(name, UILogLevel::Debug)
#define INITIALIZE_CLASS_LOG_DEBUG(name) INITIALIZE_CLASS_LOG(name, UILogLevel::Debug)
#define INITIALIZE_LOG_INFO(name) INITIALIZE_LOG(name, UILogLevel::Info)
#define INITIALIZE_CLASS_LOG_INFO(name) INITIALIZE_CLASS_LOG(name, UILogLevel::Info)
#define INITIALIZE_LOG_WARN(name) INITIALIZE_LOG(name, UILogLevel::Warn)
#define INITIALIZE_CLASS_LOG_WARN(name) INITIALIZE_CLASS_LOG(name, UILogLevel::Warn)
#define INITIALIZE_LOG_ERROR(name) INITIALIZE_LOG(name, UILogLevel::Error)
#define INITIALIZE_CLASS_LOG_ERROR(name) INITIALIZE_CLASS_LOG(name, UILogLevel::Error)
#define INITIALIZE_LOG_CRIT(name) INITIALIZE_LOG(name, UILogLevel::Critical)
#define INITIALIZE_CLASS_LOG_CRIT(name) INITIALIZE_CLASS_LOG(name, UILogLevel::Critical)
#define XUILOG_SCOPE_DEFER1(left, right) left##right
#define XUILOG_SCOPE_DEFER2(left, right) XUILOG_SCOPE_DEFER1(left, right)
#define XUILOG_SCOPE_DEFER3(name) XUILOG_SCOPE_DEFER2(name, __LINE__)
#define UILOG_SCOPE(name) auto XUILOG_SCOPE_DEFER3(TEMP_SCOPED_LOG_) = LOG_.PushScoped(name);  XUILOG_SCOPE_DEFER3(TEMP_SCOPED_LOG_)
#define UILOG_ISCOPE(tag) UILog::Tag(tag)
#define UILOG_FUNC() UILog::Tag(__func__)
#else
#define INITIALIZE_LOG(...)
#define INITIALIZE_CLASS_LOG(...)
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


#define UILOG_SCOPE(...)
#define UILOG_ISCOPE(...)
#endif

