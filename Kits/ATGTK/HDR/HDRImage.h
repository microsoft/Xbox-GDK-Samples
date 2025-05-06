//--------------------------------------------------------------------------------------
// HDRImage.h
//
// Loads .HDR file and creates D3D resources for it
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once

#include <atomic>
#include <wrl/client.h>

namespace DX
{

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)

    class HDRImage
    {
    public:
        HDRImage();

        HDRImage(HDRImage&&) = delete;
        HDRImage& operator= (HDRImage&&) = delete;

        HDRImage(HDRImage const&) = delete;
        HDRImage& operator= (HDRImage const&) = delete;

        void Load(const wchar_t* strPath, ID3D12Device* d3dDevice, ID3D12CommandQueue* d3dCommandQueue);
        UINT GetWidth() const { return m_width; }
        UINT GetHeight() const { return m_height; }
        float GetMaxLuminance() const { return m_maxLuminance; }
        bool HasFinishedLoading() const { return m_bFinishedLoading; }
        void SetShaderResourceView(D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle) { m_srvCpuHandle = srvCpuHandle; m_srvGpuHandle = srvGpuHandle; }
        D3D12_GPU_DESCRIPTOR_HANDLE GetShaderResourceView() { return m_srvGpuHandle; }

    private:
        UINT                m_width;
        UINT                m_height;
        float               m_maxLuminance;
        float               m_exposure;
        std::atomic_bool    m_bFinishedLoading;

        Microsoft::WRL::ComPtr<ID3D12Resource>  m_texture;
        D3D12_CPU_DESCRIPTOR_HANDLE             m_srvCpuHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE             m_srvGpuHandle;
    };

#elif defined(__d3d11_h__) || defined(__d3d11_x_h__)

    class HDRImage
    {
    public:
        HDRImage();

        HDRImage(HDRImage&&) = delete;
        HDRImage& operator= (HDRImage&&) = delete;

        HDRImage(HDRImage const&) = delete;
        HDRImage& operator= (HDRImage const&) = delete;

        void Load(const wchar_t* strPath, ID3D11Device* d3dDevice);
        UINT GetWidth() const { return m_width; }
        UINT GetHeight() const { return m_height; }
        float GetMaxLuminance() const { return m_maxLuminance; }
        bool HasFinishedLoading() const { return m_bFinishedLoading; }
        ID3D11ShaderResourceView* GetShaderResourceView() { return m_d3dSRV.Get(); }

    private:
        UINT                m_width;
        UINT                m_height;
        float               m_maxLuminance;
        float               m_exposure;
        std::atomic_bool    m_bFinishedLoading;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_d3dSRV;
    };

#else
#   error Please #include <d3d11.h> or <d3d12.h>
#endif

}