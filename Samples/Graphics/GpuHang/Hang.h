#pragma once

#include <algorithm>
#include <vector>

namespace DX
{
    class DeviceResources;
}

// The hang interface
class IHang
{
public:
    enum QueueType : uint32_t
    {
        Graphics,
        Async,
        Dma,

        QueueTypeCount
    };
    static const wchar_t* QueueNames[];
    virtual QueueType GetQueue() const = 0;
    virtual bool SetQueue(QueueType queue) = 0;

    enum HangAction : uint32_t
    {
        Dump,
#ifdef LIVE_DEBUGGING_SUPPORT
        LiveDebug,
        LiveDebugThenDump,
#endif

        HangActionCount
    };
    static const wchar_t* HangActionNames[];
    virtual HangAction GetHangAction() const = 0;
    virtual bool SetHangAction(HangAction hangAction) = 0;

    virtual const wchar_t* GetName() const = 0;
    virtual const wchar_t* GetFileName() const = 0;
    virtual const std::vector<const wchar_t*> GetDescription() const = 0;

    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue) = 0;
    virtual void Render(ID3D12Device* device, ID3D12CommandQueue* commandQueue, bool hang) = 0;
    virtual void Check(ID3D12Device* device, ID3D12CommandQueue* commandQueue) = 0;
    virtual void Uninitialize(ID3D12Device* device) = 0;
    virtual void GpuFullStop(ID3D12Device* device, ID3D12CommandQueue* commandQueue) = 0;
};

__declspec(selectany) const wchar_t* IHang::QueueNames[] =
{
    L"Graphics",
    L"Async",
    L"Dma",
};
static_assert(_countof(IHang::QueueNames) == IHang::QueueType::QueueTypeCount, "Array size mismatch");

__declspec(selectany) const wchar_t* IHang::HangActionNames[] =
{
    L"Dump",
#ifdef LIVE_DEBUGGING_SUPPORT
    L"LiveDebug",
    L"LiveDebugThenDump",
#endif
};
static_assert(_countof(IHang::HangActionNames) == IHang::HangAction::HangActionCount, "Array size mismatch");

class Hang : public IHang
{
protected:
    const uint32_t                  m_supportedQueues;
    QueueType                       m_queue;
    const uint32_t                  m_supportedHangActions;
    HangAction                      m_hangAction;

public:
    Hang(uint32_t supportedQueues = (1U << QueueType::Graphics)
        , uint32_t supportedHangActions = (1U << HangAction::Dump))
        : m_supportedQueues(supportedQueues)
        , m_queue(QueueType::QueueTypeCount)
        , m_supportedHangActions(supportedHangActions)
        , m_hangAction(HangAction::HangActionCount)
    {
        HangList().push_back(this);
    }

    // We don't expect this to ever be called
    virtual ~Hang()
    {
        HangList().erase(std::remove(HangList().begin(), HangList().end(), this), HangList().end());
    }

    virtual QueueType GetQueue() const override
    {
        return m_queue;
    }

    virtual bool SetQueue(QueueType queue) override
    {
        bool supported = (m_supportedQueues & (1U << queue));
        if (supported)
        {
            m_queue = queue;
        }
        return supported;
    }

    virtual HangAction GetHangAction() const override
    {
        return m_hangAction;
    }

    virtual bool SetHangAction(HangAction hangAction) override
    {
        bool supported = (m_supportedHangActions & (1U << hangAction));
        if (supported)
        {
            m_hangAction = hangAction;
        }
        return supported;
    }

    // Use "Construct On First Use Idiom" to avoid order-of-static-initialization dependencies
    static std::vector<IHang*>& HangList()
    {
        static std::vector<IHang*> hangList;

        return hangList;
    }

    void GpuFullStop(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
#ifdef LIVE_DEBUGGING_SUPPORT
        ID3D12Device11* device11 = nullptr;
        DX::ThrowIfFailed(device->QueryInterface(IID_GRAPHICS_PPV_ARGS(&device11)));
#endif

        Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
        DX::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(fence.ReleaseAndGetAddressOf())));

        commandQueue->Signal(fence.Get(), 1);

        auto originalTime = GetTickCount64();
        while (0 == fence->GetCompletedValue())
        {
#ifdef LIVE_DEBUGGING_SUPPORT
            if (device11->IsLiveDebugServerActiveX())
            {
                originalTime = GetTickCount64();  // Reset timer
            }
#endif

            auto currentTime = GetTickCount64();
            auto waitTime = 5000U; // 5 seconds
            if (currentTime > originalTime + waitTime)
            {
#ifdef LIVE_DEBUGGING_SUPPORT
                D3D12XBOX_REPORT_GPU_HALT_FLAGS flags = D3D12XBOX_REPORT_GPU_HALT_FLAG_ALL;
                switch (m_hangAction)
                {
                case Dump:
                    flags = D3D12XBOX_REPORT_GPU_HALT_FLAG_NO_LIVE_DEBUG;
                    break;
                case LiveDebug:
                    flags = D3D12XBOX_REPORT_GPU_HALT_FLAG_NO_DUMP;
                    break;
                case LiveDebugThenDump:
                    flags = D3D12XBOX_REPORT_GPU_HALT_FLAG_ALL;
                    break;
                case HangActionCount:
                default:
                    assert(false);  // Unsupported option
                    break;
                };
                device11->ReportGpuHaltX(flags);
#else
                device->ReportGpuHangX(D3D12XBOX_REPORT_GPU_HANG_FLAG_NONE);
#endif
            }
            else
            {
                auto sleepTime = 10U; // 10 ms
                Sleep(sleepTime);
            }
        }
    }
};

