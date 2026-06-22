//-----------------------------------------------------------------------------
// PSOGen.cpp
//
// Building & running this tool copies the output data into AdvancedPSO assets
// directory.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "pch.h"
#include "..\Shared\PsoSet.h"
#include "..\Shared\PsoDesc.h"

namespace
{
#ifdef _SCARLETT
#ifdef _DEBUG
    const wchar_t* psoPath = L"..\\AdvancedPSO\\Assets\\psoset\\Scarlett-Debug";
#else
    const wchar_t* psoPath = L"..\\AdvancedPSO\\Assets\\psoset\\Scarlett-Release";
#endif
#else
#ifdef _DEBUG
    const wchar_t* psoPath = L"..\\AdvancedPSO\\Assets\\psoset\\XboxOne-Debug";
#else
    const wchar_t* psoPath = L"..\\AdvancedPSO\\Assets\\psoset\\XboxOne-Release";
#endif
#endif
}

using namespace Microsoft::WRL;

int main()
{
    // Create UMD12 Device - this will fail if the Dependant DLLs are not available
    // in the working directory or DLL search path. The project copies these during
    // a custom build step.
    ComPtr<ID3D12Device> device;
    DX::ThrowIfFailed(
        D3D12CreateDevice(nullptr,
            D3D_FEATURE_LEVEL_11_0,
            IID_GRAPHICS_PPV_ARGS(device.ReleaseAndGetAddressOf())
            ));

    // Use a shared description (this is used by AdvancedPSO solution). Normally this
    // would be driven by data shared between title and tool.
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ATG::GetSharedPSODescription(psoDesc);

    // Save a set of PSO components, for every combination of given shaders
    ATG::PsoSet psoSet(psoPath);
    psoSet.SaveCombination( device.Get(),
                            psoDesc, 
                            L"test_set",
                            L"",
                            L"root_signature.cso",
                            { L"vs_static.cso", L"vs_morph.cso", L"vs_tint.cso" },
                            { L"ps_light.cso", L"ps_texture.cso", L"ps_light_texture.cso" });

    return EXIT_SUCCESS;
}

