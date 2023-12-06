//--------------------------------------------------------------------------------------
// SimpleLogManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleLogManager.h"
#include <iomanip>

SimpleLogManager::SimpleLogManager() noexcept(false)
{
}

SimpleLogManager::~SimpleLogManager()
{
}

void SimpleLogManager::LogFailedHR(HRESULT hr, const std::string& functionName)
{
    char buffer[256] = {};
    sprintf_s(buffer, 256, u8"%s Failed with hr=%08X", functionName.c_str(), hr);
    Log(buffer);
}

void SimpleLogManager::ClearLogQueue()
{
    std::lock_guard<std::mutex> scopeLock(m_logMutex);
    m_logQueue.clear();
}

void SimpleLogManager::Log(const std::string& text)
{
    std::lock_guard<std::mutex> scopeLock(m_logMutex);
    std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm;
    std::stringstream ss;
    localtime_s(&tm, &currentTime);
    ss << std::put_time(&tm, "%X");
    std::string toPrint = "[" + ss.str() + "] " + text;

    m_logQueue.push_back(toPrint);
    OutputDebugStringA(toPrint.c_str());
    OutputDebugStringA(u8"\n");
}

std::vector<std::string> SimpleLogManager::GetLogQueue()
{
    std::lock_guard<std::mutex> scopeLock(m_logMutex);
    return m_logQueue;
}

std::vector<std::string> SimpleLogManager::FlushLogQueue()
{
    std::lock_guard<std::mutex> scopeLock(m_logMutex);
    std::vector<std::string> toReturn = std::move(m_logQueue);
    m_logQueue.clear();
    return toReturn;
}

size_t SimpleLogManager::QueueSize()
{
    return m_logQueue.size();
}

std::mutex& SimpleLogManager::GetMutex()
{
    return m_logMutex;
}
