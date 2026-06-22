//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

struct Capture
{
    void Start(ID3D12CommandQueue* commandQueue, const wchar_t* name)
    {
        if (m_enabled)
        {
            std::wostringstream captureFileName;
            captureFileName << m_root;
            captureFileName << name;
            captureFileName << L".xpix";
            m_fileName = captureFileName.str();

            std::wostringstream message;
            message << L"Starting PIX capture ";
            message << m_fileName;
            message << std::endl;
            OutputDebugString(message.str().c_str());

            auto hr = commandQueue->PIXGpuBeginCapture(D3D12XBOX_PIX_CAPTURE_API | D3D12XBOX_PIX_CAPTURE_ALL_ENGINES, m_fileName.c_str());
            if (!SUCCEEDED(hr))
            {
                throw std::exception("Failure in PIXGpuBeginCapture.\n");
            }
        }
    }

    void End(ID3D12CommandQueue* commandQueue) 
    {
        if (m_enabled)
        {
            std::wostringstream message;
            message << L"Ending PIX capture ";
            message << m_fileName;
            message << std::endl;
            OutputDebugString(message.str().c_str());

            auto hr = commandQueue->PIXGpuEndCapture();
            if (!SUCCEEDED(hr))
            {
                throw std::exception("Failure in PIXGpuEndCapture.\n");
            }

            m_fileName = L"";
        }
    }

    static void SetRoot(const std::wstring& root)
    {
        m_root = root;
    }

    static void SetEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

private:
    std::wstring m_fileName;

    static std::wstring m_root;
    static bool m_enabled;
};

__declspec(selectany) std::wstring Capture::m_root;

#if defined(_DEBUG) || defined(PROFILE)
__declspec(selectany) bool Capture::m_enabled = true;
#else
__declspec(selectany) bool Capture::m_enabled = false;
#endif

