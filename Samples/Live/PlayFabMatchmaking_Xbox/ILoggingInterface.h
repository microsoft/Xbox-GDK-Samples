//--------------------------------------------------------------------------------------
// ILoggingInterface.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <string>

class ILoggingInterface
{
public:
    virtual void AppendLineOfText(const std::string& lineOfText) const = 0;
};

class Loggable
{
public:
    Loggable(const ILoggingInterface& logger) :
        m_logger(logger)
    {
    }

    virtual ~Loggable() = default;

protected:
    const ILoggingInterface& m_logger;

protected:
    template<typename ... Args>
    void Log(const std::string& format, Args ... args) const
    {
        auto expectedSize = snprintf(nullptr, size_t(0), format.c_str(), args ...) + 1; // Extra space for '\0'
        if (expectedSize <= 0) { throw std::runtime_error("Error during formatting."); }

        auto size = static_cast<size_t>(expectedSize);
        std::unique_ptr<char[]> buf(new char[size]);

        snprintf(buf.get(), static_cast<size_t>(size), format.c_str(), args ...);
        auto logLine = std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside

        m_logger.AppendLineOfText(logLine);
    }
};
