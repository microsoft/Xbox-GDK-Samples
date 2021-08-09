//--------------------------------------------------------------------------------------
// FrontPanelRenderTarget.h
//
// Microsoft Game Core on Xbox for DirectX 12
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <XFrontPanelDisplay.h>

#include "BufferDescriptor.h"
#include "PostProcess.h"
#include "RenderTargetState.h"

namespace ATG
{
    class FrontPanelRenderTarget
    {
    public:
        FrontPanelRenderTarget();
        virtual ~FrontPanelRenderTarget();

        FrontPanelRenderTarget(FrontPanelRenderTarget&&) = default;
        FrontPanelRenderTarget& operator=(FrontPanelRenderTarget&&) = default;

        FrontPanelRenderTarget(const FrontPanelRenderTarget&) = delete;
        FrontPanelRenderTarget& operator=(const FrontPanelRenderTarget&) = delete;
        
        void CreateDeviceDependentResources(_In_ ID3D12Device *device, unsigned int numRenderTargets);
        void CreateWindowSizeDependentResources(
            _In_ ID3D12Device *device, 
            _In_ unsigned int numRenderTargets,
            _In_reads_(numRenderTargets) ID3D12Resource*const* pRenderTargets);

        void Clear(_In_ ID3D12GraphicsCommandList *commandList, const float ColorRGBA[4], unsigned int rtIndex);
        void SetAsRenderTarget(_In_ ID3D12GraphicsCommandList *commandList, unsigned int rtIndex);

        ID3D12Resource *GetRenderTarget(_In_ unsigned int rtIndex) const { return m_blitRenderTargets[rtIndex].Get(); }
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const
        {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, m_rtvDescriptorSize);
        }
        DXGI_FORMAT GetRenderTargetFormat() const { return m_blitRenderTargetFormat; }

        // Render a grayscale image using passed-in renderTarget resource.
        // Resource must be one of the render targets that what used to initialize this class.
        void GPUBlit(
            _In_ ID3D12GraphicsCommandList* commandList, 
            _In_ ID3D12Resource* renderTargetResource, 
            _In_ unsigned int renderTargetIndex);

        // Copy the render target from the previous frame to a staging texture and then copy it back to the CPU.
        // Causes a GPU synchronization to ensure work from previous frame completes before reading on the CPU.
        void CopyToBuffer(
            _In_ ID3D12Device* device,
            _In_ ID3D12CommandQueue* commandQueue,
            _In_ unsigned int renderTargetIndex, 
            _Inout_ BufferDesc &desc);

        // Copy the render target to a staging texture, copy the result back to the CPU
        // and then present it to the front panel display
        // Causes a GPU synchronization to ensure work from previous frame completes before reading on the CPU.
        void PresentToFrontPanel(
            _In_ ID3D12Device* device, 
            _In_ ID3D12CommandQueue* commandQueue, 
            _In_ unsigned int renderTargetIndex);

    protected:

        // Front Panel object
        uint32_t                                        m_displayWidth;
        uint32_t                                        m_displayHeight;

        // Direct3D 12 Resources
        Microsoft::WRL::ComPtr<ID3D12Resource>          m_blitRenderTargets[3];
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    m_rtvDescriptorHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    m_srvDescriptorHeap;
        UINT                                            m_rtvDescriptorSize;
        UINT                                            m_srvDescriptorSize;
        UINT                                            m_numBlitRenderTargets;
        DXGI_FORMAT                                     m_blitRenderTargetFormat;

        // Resources for GPU Blit / Present
        std::unique_ptr<DirectX::BasicPostProcess>      m_panelBlt;
        std::unique_ptr<uint8_t[]>                      m_buffer;
        Microsoft::WRL::ComPtr<ID3D12Resource>          m_staging;
    };
}
