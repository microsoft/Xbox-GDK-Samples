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
#ifdef _GAMING_XBOX
            captureFileName << L".xpix";
#else
            captureFileName << L".wpix";
#endif
            m_fileName = captureFileName.str();

            std::wostringstream message;
            message << L"Starting PIX capture ";
            message << m_fileName;
            message << std::endl;
            OutputDebugString(message.str().c_str());
#ifdef _GAMING_XBOX
            auto hr = commandQueue->PIXGpuBeginCapture(D3D12XBOX_PIX_CAPTURE_API | D3D12XBOX_PIX_CAPTURE_ALL_ENGINES, m_fileName.c_str());
            if (!SUCCEEDED(hr))
            {
                throw std::exception("Failure in PIXGpuBeginCapture.\n");
            }
#else
            (void)commandQueue;
            if (m_pixModule != NULL)
            {
                PIXCaptureParameters pixCaptureParams = {};
                pixCaptureParams.GpuCaptureParameters.FileName = m_fileName.c_str();
                PIXBeginCapture(PIX_CAPTURE_GPU, &pixCaptureParams);
            }
#endif
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

#ifdef _GAMING_XBOX
            auto hr = commandQueue->PIXGpuEndCapture();
            if (!SUCCEEDED(hr))
            {
                throw std::exception("Failure in PIXGpuEndCapture.\n");
            }
#else
            (void)commandQueue;
            if (m_pixModule != NULL)
            {
                PIXEndCapture(FALSE);
            }
#endif

            m_fileName = L"";
        }
    }

    static void SetRoot(const std::wstring& root)
    {
        m_root = root;
#ifdef _GAMING_DESKTOP
        if (m_pixModule == NULL)
        {
#ifndef ENABLE_GPU_COUNTERS
            // NOTE: Programmatic PIX capture are disabled on Desktop because its instrumentation
            // prevents Nsight Perf SDK from working correctly.
            // Feel free to uncomment these lines and disable Nsight Perf SDK to create PIX captures
            //
            m_pixModule = PIXLoadLatestWinPixGpuCapturerLibrary();
            PIXSetHUDOptions(PIX_HUD_SHOW_ON_NO_WINDOWS);
#endif // #ifdef ENABLE_GPU_COUNTERS
        }
#endif
    }

    static void SetEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

private:
    std::wstring m_fileName;

#ifdef _GAMING_DESKTOP
    static HMODULE m_pixModule;
#endif

    static std::wstring m_root;
    static bool m_enabled;
};

#ifdef _GAMING_DESKTOP
__declspec(selectany) HMODULE Capture::m_pixModule = NULL;
#endif

__declspec(selectany) std::wstring Capture::m_root;

#if defined(_DEBUG) || defined(PROFILE)
__declspec(selectany) bool Capture::m_enabled = true;
#else
__declspec(selectany) bool Capture::m_enabled = false;
#endif

