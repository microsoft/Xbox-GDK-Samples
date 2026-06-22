//-----------------------------------------------------------------------------
// PsoSet.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>

namespace ATG
{
    // A PsoSet represents a set of PSOs based on a given combination of vertex and pixel shaders, and a base PSO description
    // with a root signature. 
    //
    // The individual shader components are de-duplicated across PSOs, but a unique packet and meta data component is generated
    // and stored per PSO.
    //
    // This class can be used both offline to generate the PSO components to disk, and in title to load the PSO components as requested.
    class PsoSet
    {
        struct PSOComponent;

    public:

        PsoSet(const wchar_t* basePath) : m_basePath(basePath)
        {
            BOOL ok = CreateDirectoryW(basePath, nullptr);
            if (!ok && GetLastError() == ERROR_PATH_NOT_FOUND)
            {
                throw std::runtime_error("Couldn't create output directory");
            }
        }

        ~PsoSet()
        {
#ifdef _GAMING_XBOX
            for (auto& c : m_psoComponents)
            {
                if (c.second.pGpu != nullptr)
                {
                    XMemFree(c.second.pGpu, c.second.xAttr.Attributes);
                }
                if (c.second.pMetaData != nullptr)
                {
                    XMemFree(c.second.pMetaData, c.second.xAttr.Attributes);
                }
            }
#endif
        }

#ifdef _GAMING_XBOX
        void ReleaseTransient()
        {
            for (auto& c : m_psoComponents)
            {
                if (c.second.pGpu != nullptr && c.second.IsGpuTransient)
                {
                    XMemFree(c.second.pGpu, c.second.xAttr.Attributes);
                    c.second.pGpu = nullptr;
                }
                if (c.second.pMetaData != nullptr && c.second.IsMetaDataTransient)
                {
                    XMemFree(c.second.pMetaData, c.second.xAttr.Attributes);
                    c.second.pMetaData = nullptr;
                }
            }
            m_psoComponents.clear();
        }
#endif

#ifndef _GAMING_XBOX
        // Save a set of de-duplicated PSOs given a set of compiled input shaders and a root signature file.
        // This method will write the following to the output directory:
        //
        //      Root signature (copied from input)
        //      All vertex shader GPU and meta blobs
        //      All pixel shader GPU and meta blobs
        //      For each PSO combination - > a packet and meta data blob
        //
        void SaveCombination(
            ID3D12Device*                           device,
            D3D12_GRAPHICS_PIPELINE_STATE_DESC      psoDesc,
            const wchar_t*                          psoName,
            const wchar_t*                          inputPath,
            const wchar_t*                          rootSignaturePath,
            const std::unordered_set<std::wstring>& vertexShaderFilenames,
            const std::unordered_set<std::wstring>& pixelShaderFilenames)
        {
            using namespace Microsoft::WRL;

         
            // Create base path
            std::wstring psoBasePath = m_basePath + std::wstring(L"\\") + std::wstring(psoName);
            std::wstring shaderBasePath = std::wstring(inputPath) + std::wstring(L"\\");

            // Load all the shaders
            std::unordered_map<std::wstring, std::vector<uint8_t>> shaderObjectBlobs;
            for (const auto& vsf : vertexShaderFilenames)
            {
                auto blob = DX::ReadData(std::wstring(shaderBasePath + vsf).c_str());
                shaderObjectBlobs.insert(std::make_pair(vsf, blob));
            }
            for (const auto& psf : pixelShaderFilenames)
            {
                auto blob = DX::ReadData(std::wstring(shaderBasePath + psf).c_str());
                shaderObjectBlobs.insert(std::make_pair(psf, blob));
            }

            // Write root signature blob out
            auto rsBlob = DX::ReadData(std::wstring(shaderBasePath + rootSignaturePath).c_str());
            DX::WriteData(GetRootSignatureFilename(psoBasePath).c_str(), rsBlob);
            OutputDebugStringW(std::wstring(L"RS: " + GetRootSignatureFilename(psoBasePath) + L"\n").c_str());

            // Generate blobs for each combination of shaders, with some optimizations so we don't re-write the same one.
            //
            // 1. Write a root signature only once (above).
            // 2. Write a main packet for every combination of shader.
            // 3. Write each pixel shader output only once.
            // 4. Write each vertex shader output only once.
            // 5. Write the fetch shader output only once. (Scarlett only)
            bool writePS = true;
            bool writeVS = true;
#ifdef _SCARLETT
            bool writeFS = true;
#endif

            for (const auto& vsf : vertexShaderFilenames)
            {
                for (const auto& psf : pixelShaderFilenames)
                {
                    // Only write VS for first iteration of all the pixel shaders
                    writeVS = (psf == *(pixelShaderFilenames.begin()));

                    // Patch shaders into desc
                    auto vsBlob = shaderObjectBlobs[vsf];
                    auto psBlob = shaderObjectBlobs[psf];

                    psoDesc.VS = { vsBlob.data(), vsBlob.size() };
                    psoDesc.PS = { psBlob.data(), psBlob.size() };

                    // Create PSO
                    ComPtr<ID3D12PipelineState> state;
                    DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(state.ReleaseAndGetAddressOf())));

                    // Deserialize 
                    D3D12XBOX_SERIALIZE_GRAPHICS_PIPELINE_STATE serialized;
                    DX::ThrowIfFailed(device->SerializeGraphicsPipelineStateX(state.Get(), D3D12XBOX_SERIALIZE_FLAGS_NONE, &serialized));

                    // Name suffix for output blobs
                    std::wstring vsName = GetShaderNameFromPath(vsf);
                    std::wstring psName = GetShaderNameFromPath(psf);

                    // Per-PSO packet
                    DX::WriteData(GetPsoPacketFilename(psoBasePath, vsName, psName).c_str(), serialized.pPacket);
                    DX::WriteData(GetPsoPacketMetaFilename(psoBasePath, vsName, psName).c_str(), serialized.pMetaData);
                    OutputDebugStringW(std::wstring(GetPsoPacketFilename(psoBasePath, vsName, psName) + L"\n").c_str());

                    // Per-shader packets
                    if (writeVS)
                    {
                        DX::WriteData(GetPsoShaderGpuFilename(psoBasePath, vsName).c_str(), serialized.VS.pGpuInstructions);
                        DX::WriteData(GetPsoShaderMetaFilename(psoBasePath, vsName).c_str(), serialized.VS.pMetaData);
                        OutputDebugStringW(std::wstring(L"VS: " + GetPsoShaderGpuFilename(psoBasePath, vsName) + L"\n").c_str());
                    }

                    if (writePS)
                    {
                        DX::WriteData(GetPsoShaderGpuFilename(psoBasePath, psName).c_str(), serialized.PS.pGpuInstructions);
                        DX::WriteData(GetPsoShaderMetaFilename(psoBasePath, psName).c_str(), serialized.PS.pMetaData);
                        OutputDebugStringW(std::wstring(L"PS: " + GetPsoShaderGpuFilename(psoBasePath, psName) + L"\n").c_str());
                    }

#ifdef _SCARLETT
                    // Fetch shaders only on Scarlett
                    if (writeFS)
                    {
                        // Fetch shader doesn't correlate to a user-provided shader, so need to provide a name programmatically
                        //
                        // NOTE: This is a hack since we're relying on our foreknowledge that this set of shaders only uses a single
                        // vertex layout, which is what determines the fetch shader. In a real world application the fetch shader
                        // would need to be de-duplicated same as everything else.
                        std::wstring fsName = L"fs_nmtx"; 

                        DX::WriteData(GetPsoShaderGpuFilename(psoBasePath, fsName).c_str(), serialized.FS.pGpuInstructions);

                        // Fetch shader doesn't necessarily have metadata
                        if (serialized.FS.pMetaData)
                        {
                            DX::WriteData(GetPsoShaderMetaFilename(psoBasePath, fsName).c_str(), serialized.FS.pMetaData);
                        }

                        OutputDebugStringW(std::wstring(L"FS: " + GetPsoShaderGpuFilename(psoBasePath, fsName) + L"\n").c_str());

                        writeFS = false; // All VS in this sample use the same vertex format; only need to write once
                    }
#endif

                }

                // don't need to write PS after first row of VS
                writePS = false;
            }
        }
#else

        // Shader combinations to load into PSOs.
        struct ShaderCombination
        {
#ifdef _GAMING_XBOX_SCARLETT
            const wchar_t* fsName;
#endif
            const wchar_t* vsName;
            const wchar_t* psName;
        };

        // Create a single deserialized PSO based on a combination of vertex shader and pixel shader (serialized by Save() method above)
        // Each PSO component is cached on demand in heap or graphics memory, so subsequent loads are faster.
        // Titles may choose to preload components from sets at level start time.
        Microsoft::WRL::ComPtr<ID3D12PipelineState> LoadCombination(ID3D12Device* device, const ShaderCombination& s)
        {
            using namespace Microsoft::WRL;

            D3D12XBOX_DESERIALIZE_GRAPHICS_PIPELINE_STATE deserializedDesc = {};
           
            // Cache root signature
            LoadRootSignature(device);
            deserializedDesc.pRootSignature = m_rootSignature.Get();

            // Load packet & set into desc
            // The PSO packet is unique per shader combination. 
            PSOComponent* psoPacket = LoadPsoComponent(PsoSet::GetPsoPacketFilename(m_basePath, s.vsName, s.psName).c_str(),
                                                        PsoSet::GetPsoPacketMetaFilename(m_basePath, s.vsName, s.psName).c_str(), false);
            deserializedDesc.pPacket = psoPacket->pGpu;
            deserializedDesc.PacketSize = psoPacket->GpuSize;
            psoPacket->IsGpuTransient = true; // This is a transient allocation
            deserializedDesc.pMetaData = psoPacket->pMetaData;
            deserializedDesc.MetaDataSize = psoPacket->MetaDataSize;
            psoPacket->IsMetaDataTransient = true; // This is a transient allocation

            // Load pixel shader
            PSOComponent* ps = LoadPsoComponent(PsoSet::GetPsoShaderGpuFilename(m_basePath, s.psName).c_str(),
                                                PsoSet::GetPsoShaderMetaFilename(m_basePath, s.psName).c_str(), true);
            deserializedDesc.PS.pGpuInstructions = ps->pGpu;
            deserializedDesc.PS.GpuInstructionsSize = ps->GpuSize;
            ps->IsGpuTransient = false; // This is not a transient allocation
            deserializedDesc.PS.pMetaData = ps->pMetaData;
            deserializedDesc.PS.MetaDataSize = ps->MetaDataSize;
            ps->IsMetaDataTransient = true; // This is a transient allocation

            // Load vertex shader
            PSOComponent* vs = LoadPsoComponent(PsoSet::GetPsoShaderGpuFilename(m_basePath, s.vsName).c_str(),
                                                PsoSet::GetPsoShaderMetaFilename(m_basePath, s.vsName).c_str(), true);
            deserializedDesc.VS.pGpuInstructions = vs->pGpu;
            deserializedDesc.VS.GpuInstructionsSize = vs->GpuSize;
            vs->IsGpuTransient = false; // This is not a transient allocation
            deserializedDesc.VS.pMetaData = vs->pMetaData;
            deserializedDesc.VS.MetaDataSize = vs->MetaDataSize;
            vs->IsMetaDataTransient = true; // This is a transient allocation

#ifdef _GAMING_XBOX_SCARLETT
            // Load fetch shader
            PSOComponent* fs = LoadPsoComponent(PsoSet::GetPsoShaderGpuFilename(m_basePath, s.fsName).c_str(),
                                                PsoSet::GetPsoShaderMetaFilename(m_basePath, s.fsName).c_str(), true);
            deserializedDesc.FS.pGpuInstructions = fs->pGpu;
            deserializedDesc.FS.GpuInstructionsSize = fs->GpuSize;
            fs->IsGpuTransient = false; // This is not a transient allocation
            deserializedDesc.FS.pMetaData = fs->pMetaData;
            deserializedDesc.FS.MetaDataSize = fs->MetaDataSize;
            fs->IsMetaDataTransient = true; // This is a transient allocation
#endif

            // Create PSO (this is not cached, but could be)
            ComPtr<ID3D12PipelineState> pso;
            HRESULT hr = device->DeserializeGraphicsPipelineStateX(&deserializedDesc, IID_GRAPHICS_PPV_ARGS(pso.ReleaseAndGetAddressOf()));
            if (FAILED(hr))
            {
                OutputDebugStringW(L"DeserializeGraphicsPipelineStateX returned failure. A full rebuild of the sample solution might help.");
                throw DX::com_exception(hr);
            }

            return pso;
        }

        // Alls PSOs in the set must have the same root signature, so we store this along side the components. This will be used
        // when the PSO is deserialized, and the tile may use this for SetRootSignature() call during draw.
        Microsoft::WRL::ComPtr<ID3D12RootSignature> LoadRootSignature(ID3D12Device* device)
        {
            // Load or return cached root signature - there is only one per PSO set.
            if (m_rootSignature == nullptr)
            {
                // Load if required
                std::vector<uint8_t> rsBlob = DX::ReadData(PsoSet::GetRootSignatureFilename(m_basePath).c_str());
                DX::ThrowIfFailed(device->CreateRootSignature(0, rsBlob.data(), rsBlob.size(), IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));
            }

            return m_rootSignature;
        }

#endif

    private:
        // A structure to hold an individual de-serialized component of a PSO
        struct PSOComponent
        {
            // packet or shader code
            void*   pGpu;
            SIZE_T  GpuSize;
            bool    IsGpuTransient;    

            // metadata
            void*   pMetaData;
            SIZE_T  MetaDataSize;
            bool    IsMetaDataTransient;

#if _GAMING_XBOX
            // cache attributes for XMemFree
            XALLOC_ATTRIBUTES xAttr;
#endif
        };

        // Standard file names for each component. All the components could be archived into a single binary, but for the sample we keep
        // separate files.

        inline std::wstring GetRootSignatureFilename(const std::wstring& basePath)
        {
            return std::wstring(basePath) + std::wstring(L".rootsig.bin");
        }

        inline std::wstring GetPsoPacketFilename(const std::wstring& basePath, const std::wstring& vertexShaderName, const std::wstring& pixelShaderName)
        {
            return basePath + std::wstring(L".") + vertexShaderName + std::wstring(L".") + pixelShaderName + std::wstring(L".pso.packet.bin");
        }

        inline std::wstring GetPsoPacketMetaFilename(const std::wstring& basePath, const std::wstring& vertexShaderName, const std::wstring& pixelShaderName)
        {
            return basePath + std::wstring(L".") + vertexShaderName + std::wstring(L".") + pixelShaderName + std::wstring(L".pso.meta.bin");
        }

        inline std::wstring GetPsoShaderGpuFilename(const std::wstring& basePath, const std::wstring& shaderName)
        {
            return basePath + std::wstring(L".") + shaderName + std::wstring(L".shader.gpu.bin");
        }

        inline std::wstring GetPsoShaderMetaFilename(const std::wstring& basePath, const std::wstring& shaderName)
        {
            return basePath + std::wstring(L".") + shaderName + std::wstring(L".shader.meta.bin");
        }

        inline std::wstring GetShaderNameFromPath(const std::wstring& shaderFilename)
        {
            return shaderFilename.substr(0, shaderFilename.find_last_of(L"."));
        }

#if _GAMING_XBOX
        // Used in title to load a component into heap or graphics memory and store entry in cache.
        PSOComponent* LoadPsoComponent(const wchar_t* gpuFilename, const wchar_t* metaFilename, bool useGraphicsMemoryForGpu)
        {
            auto name = gpuFilename;

            // Find it
            auto entry = m_psoComponents.find(name);
            if (entry == m_psoComponents.end())
            {
                // Create a new entry
                PSOComponent psoComponent{};
              
                if (DX::FileExists(metaFilename))
                {
                    DX::ThrowIfFailed(InitializePSOComponentFromFile(metaFilename, psoComponent.xAttr, &psoComponent.pMetaData, &psoComponent.MetaDataSize));
                }

                if (useGraphicsMemoryForGpu)
                {
                    psoComponent.xAttr.s.PageSize = XALLOC_PAGESIZE_2MB;
                    psoComponent.xAttr.s.MemoryType = XALLOC_MEMTYPE_GRAPHICS_SHADER_CACHEABLE_NONCOHERENT;
                }

                // Load packet into heap or graphics memory
                DX::ThrowIfFailed(InitializePSOComponentFromFile(gpuFilename, psoComponent.xAttr, &psoComponent.pGpu, &psoComponent.GpuSize));

                // add into cache
                auto insertResult = m_psoComponents.insert(std::make_pair(name, psoComponent));
                entry = insertResult.first;
            }

            // return
            return &(entry->second);
        }

        // Load an individual blob from a file into video or CPU memory. Note this code contains an extra memcpy from temporary blob
        // to heap.
        inline HRESULT InitializePSOComponentFromFile(const wchar_t* path, XALLOC_ATTRIBUTES xAttr, _Out_ void** destMemory, _Out_ SIZE_T* destSize)
        {
            using namespace Microsoft::WRL;

            // Load blob
            auto blob = DX::ReadData(path);

            *destSize = blob.size();

            // Allocate memory
            *destMemory = XMemAlloc(*destSize, xAttr.Attributes);
                
            if (*destMemory == nullptr)
            {
                return E_OUTOFMEMORY;
            }

            // Finally, copy from temporary blob to heap
            memcpy(*destMemory, blob.data(), blob.size());

            return S_OK;
        }
#endif

        std::wstring                                   m_basePath;      // base file path for set (includes set name)
        Microsoft::WRL::ComPtr<ID3D12RootSignature>    m_rootSignature; // root signature for set
        std::unordered_map<std::wstring, PSOComponent> m_psoComponents; // collection of components for PSOs.
    };
}
