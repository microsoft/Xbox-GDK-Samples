//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Hang.h"
#include "Util.h"

class HangCommandBufferCorruption final : public Hang
{
public:
    HangCommandBufferCorruption() :
        Hang()
    {
    }

    virtual ~HangCommandBufferCorruption() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"Command buffer corruption";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"CommandBufferCorruption";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs if a command buffer contains data which is not parseable by the GPU's command processor. ");
        description.push_back(L"A typical cause of corruption is freeing the command buffer memory, and reusing it, before the GPU has finishing executing the commands. ");

        return description;
    }

    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* /* commandQueue */) override
    {
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocator);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandList);
    }

    virtual void Uninitialize(ID3D12Device* /* device */) override
    {
        m_commandList = nullptr;
        m_commandAllocator = nullptr;
    }

    virtual void Check(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        GpuFullStop(device, commandQueue);
    }

    virtual void Render(ID3D12Device* /* device */, ID3D12CommandQueue* commandQueue, bool hang) override
    {
        PIXBeginRetailEvent(commandQueue, PIX_COLOR_DEFAULT, __FUNCTIONW__);

        if (hang)
        {
            PIXBeginRetailEvent(m_commandList.Get(), PIX_COLOR_DEFAULT, L"Look for corruption after this");
            PIXEndRetailEvent(m_commandList.Get());
        }

        // Implementation-specific: This is where the command buffer starts
        auto commandBufferPointer = m_commandList->m_Putter.m_pCurrent;

        DX::ThrowIfFailed(m_commandList->Close());
        commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));

        PIXEndRetailEvent(commandQueue);

        if (hang)
        {
            // Simulate a typical case of command buffer corruption.
            // Pretend the underlying memory was reallocated and re-used while the command list was still in flight.
            static_cast<uint32_t*>(commandBufferPointer)[0] = 0x01234567;
            static_cast<uint32_t*>(commandBufferPointer)[1] = 0x89abcdef;
        }
    }

private:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>          m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>       m_commandList;
};

static HangCommandBufferCorruption hang;
