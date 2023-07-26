//--------------------------------------------------------------------------------------
// UITKLogManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "UITKLogManager.h"

void UITKLogManager::LogFailedHR(HRESULT hr, const std::string functionName)
{
    char buffer[256] = {};
    sprintf_s(buffer, 256, u8"%s Failed with hr=%08X", functionName.c_str(), hr);
    Log(buffer);
}
void UITKLogManager::ClearLogQueue()
{
    std::lock_guard<std::mutex> scopeLock(m_logMutex);
    m_logQueue.clear();
}
void UITKLogManager::Log(const std::string text)
{
    std::lock_guard<std::mutex> scopeLock(m_logMutex);
    m_logQueue.push_back(text);
    OutputDebugStringA(text.c_str());
    OutputDebugStringA(u8"\n");
}
std::vector<std::string> UITKLogManager::GetLogQueue()
{
    return m_logQueue;
}
size_t UITKLogManager::QueueSize()
{
    return m_logQueue.size();
}
