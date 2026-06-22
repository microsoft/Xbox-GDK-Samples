//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Hang.h"
#include "Util.h"

class HangCommandBufferContention final : public Hang
{
public:
    HangCommandBufferContention() :
        Hang(),
        m_hThread{nullptr},
        m_hEventBegin{nullptr},
        m_hEventEnd{nullptr},
        m_useCritSec(false),
        m_critSec{}
    {
    }

    virtual ~HangCommandBufferContention() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"Command buffer contention";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"CommandBufferContention";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs if a command buffer contains data which is not parseable by the GPU's command processor. ");
        description.push_back(L"A typical cause is writing to the same command list from multiple threads, without synchronization. ");
        description.push_back(L"This test case does not hang 100% of the time! Try pressing [A] repeatedly! ");

        return description;
    }

    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* /* commandQueue */) override
    {
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocator);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandList);

        InitializeCriticalSection(&m_critSec);

        struct ThreadParameters
        {
            ID3D12GraphicsCommandList*  commandList;
            uint32_t                    index;
            HANDLE                      hEventBegin;
            HANDLE                      hEventEnd;
            volatile bool*              useCritSec;
            CRITICAL_SECTION*           critSec;
        };
        static ThreadParameters threadParameters[_countof(m_hThread)] = {};
        auto ThreadProc = [] (LPVOID lpParameter)->DWORD
        { 
            auto parameters = *reinterpret_cast<ThreadParameters*>(lpParameter);

            WaitForSingleObject(parameters.hEventBegin, INFINITE);

            wchar_t eventName[256] = L"";
            for(auto eventNum = 0U; eventNum < 1000; ++eventNum)
            {
                _snwprintf_s(eventName, _countof(eventName), _TRUNCATE, L"Thread %d iteration %d", parameters.index, eventNum);

                if (*parameters.useCritSec)
                {
                    EnterCriticalSection(parameters.critSec);
                }

                PIXBeginRetailEvent(parameters.commandList, PIX_COLOR_DEFAULT, eventName);
                PIXEndRetailEvent(parameters.commandList);

                if (*parameters.useCritSec)
                {
                    LeaveCriticalSection(parameters.critSec);
                }
            }

            SetEvent(parameters.hEventEnd);

            return S_OK;
        };
        wchar_t threadName[256] = L"";
        for (auto i = 0U; i < _countof(m_hThread); ++i)
        {
            m_hEventBegin[i] = CreateEvent(nullptr, FALSE, FALSE, L"");
            m_hEventEnd[i] = CreateEvent(nullptr, FALSE, FALSE, L"");

            ThreadParameters parameters =
            {
                m_commandList.Get(),
                i, 
                m_hEventBegin[i], 
                m_hEventEnd[i], 
                &m_useCritSec,
                &m_critSec, 
            };
            threadParameters[i] = parameters;
            m_hThread[i] = CreateThread(nullptr, 0, ThreadProc, &threadParameters[i], 0, nullptr);
            if (!m_hThread[i])
            {
                throw std::exception("Could not create thread");
            }
            _snwprintf_s(threadName, _countof(threadName), _TRUNCATE, L"Thread %d", i);
            if (0 == SetThreadDescription(m_hThread[i], threadName)) 
            {
                throw std::exception("Could not set thread name");
            }
            if (0 == SetThreadAffinityMask(m_hThread[i], (1ULL << (i+1))))   // Run on its own core
            {
                throw std::exception("Could not set thread affinity");
            }
            if (0 == SetThreadPriority(m_hThread[i], THREAD_PRIORITY_HIGHEST))   // Run whenever possible
            {
                throw std::exception("Could not set thread priority");
            }
        }
    }

    virtual void Uninitialize(ID3D12Device* /* device */) override
    {
        m_commandList = nullptr;
        m_commandAllocator = nullptr;

        for (auto i = 0U; i < _countof(m_hEventBegin); ++i)
        {
            CloseHandle(m_hEventBegin[i]);
            CloseHandle(m_hEventEnd[i]);
            CloseHandle(m_hThread[i]);
        }
    }

    virtual void Check(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        GpuFullStop(device, commandQueue);
    }

    virtual void Render(ID3D12Device* device, ID3D12CommandQueue* commandQueue, bool hang) override
    {
        m_useCritSec = !hang; 

        SCOPED_ERROR_FILTER(device, 0x5A7F44FC); // PIX event stack underflowed in command list (0x4078dc00900 "m_commandList"), which means that more End events were issued than Begin events.

        PIXBeginRetailEvent(commandQueue, PIX_COLOR_DEFAULT, __FUNCTIONW__);

        for (auto i = 0U; i < _countof(m_hEventBegin); ++i)
        {
            SetEvent(m_hEventBegin[i]);
        }

        WaitForMultipleObjects(_countof(m_hEventEnd), m_hEventEnd, TRUE, INFINITE);

        DX::ThrowIfFailed(m_commandList->Close());
        commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));

        PIXEndRetailEvent(commandQueue);

        // Force validation now, while we have the errors disabled
        commandQueue->KickoffX();
    }

private:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>          m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>       m_commandList;

    HANDLE                                                  m_hThread[2];
    HANDLE                                                  m_hEventBegin[2];
    HANDLE                                                  m_hEventEnd[2];

    volatile bool                                           m_useCritSec;
    CRITICAL_SECTION                                        m_critSec;
};

static HangCommandBufferContention hang;
