//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "Hang.h"
#include "Util.h"

class HangFalsePositive final : public Hang
{
public:
    HangFalsePositive() :
        Hang()
    {
    }

    virtual ~HangFalsePositive() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"False positive";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"FalsePositive";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This ""hang"" occurs if the title calls ReportGpuHangX when the GPU is not actually hung. ");

        return description;
    }

    virtual void Initialize(ID3D12Device* /* device */, ID3D12CommandQueue* /* commandQueue */) override 
    {
    }

    virtual void Uninitialize(ID3D12Device* /* device */) override
    {
    }

    virtual void Check(ID3D12Device* /* device */, ID3D12CommandQueue* /* commandQueue */) override 
    {
    }

    virtual void Render(ID3D12Device* device, ID3D12CommandQueue* /* commandQueue */, bool hang) override 
    {
        if (hang)
        {
#ifdef HIX_EXCEPTION_SUPPORT
            ID3D12Device11* device11 = nullptr;
            DX::ThrowIfFailed(device->QueryInterface(IID_GRAPHICS_PPV_ARGS(&device11)));

            // May as well not reboot, since the GPU is fine
            device11->ReportGpuHaltX(D3D12XBOX_REPORT_GPU_HALT_FLAG_ALL);
#else
            // May as well not reboot, since the GPU is fine
            device->ReportGpuHangX(D3D12XBOX_REPORT_GPU_HANG_FLAG_DO_NOT_REBOOT);
#endif
        }
    }
};

static HangFalsePositive hang;
