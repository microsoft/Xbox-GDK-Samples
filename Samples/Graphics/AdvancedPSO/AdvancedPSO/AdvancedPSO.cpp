//--------------------------------------------------------------------------------------
// AdvancedPSO.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AdvancedPSO.h"

#include "Minitracker.h"
#include "..\Shared\PsoSet.h"
#include "..\Shared\PsoDesc.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    // Track memory allocations in driver
    DX::Minitracker s_Minitracker;

#ifdef _GAMING_XBOX_SCARLETT
    // fs_nmtx - fetch shader for all the PSOs
    const std::array <const wchar_t*, 1> fetchShaders =
    {
        L"fs_nmtx"
    };
#endif

    // vs_static - vertex shader for diffuse and directional lighting
    // vs_morph - vertex shader with animated outputs (wave effect)
    // vs_tint - vertex shader with diffuse tint animation
    const std::array <const wchar_t*, 3> vertexShaders =
    {
        L"vs_static",
        L"vs_morph",
        L"vs_tint",
    };

    // ps_light - pixel shader with lighting and diffuse color only
    // ps_texture - pixel shader with texture only (no lighting)
    // ps_light_texture - pixel shader with lighting and texture
    const std::array <const wchar_t*, 3> pixelShaders =
    {
        L"ps_light",
        L"ps_texture",
        L"ps_light_texture",
    };

    const std::array<ATG::PsoSet::ShaderCombination, 9> shaderCombos =
#ifdef _GAMING_XBOX_SCARLETT
    { {
        { fetchShaders[0], vertexShaders[0], pixelShaders[0], },
        { fetchShaders[0], vertexShaders[0], pixelShaders[1], },
        { fetchShaders[0], vertexShaders[0], pixelShaders[2], },
        { fetchShaders[0], vertexShaders[1], pixelShaders[0], },
        { fetchShaders[0], vertexShaders[1], pixelShaders[1], },
        { fetchShaders[0], vertexShaders[1], pixelShaders[2], },
        { fetchShaders[0], vertexShaders[2], pixelShaders[0], },
        { fetchShaders[0], vertexShaders[2], pixelShaders[1], },
        { fetchShaders[0], vertexShaders[2], pixelShaders[2], },
    } };
#else
    {{
        { vertexShaders[0], pixelShaders[0], },
        { vertexShaders[0], pixelShaders[1], },
        { vertexShaders[0], pixelShaders[2], },
        { vertexShaders[1], pixelShaders[0], },
        { vertexShaders[1], pixelShaders[1], },
        { vertexShaders[1], pixelShaders[2], },
        { vertexShaders[2], pixelShaders[0], },
        { vertexShaders[2], pixelShaders[1], },
        { vertexShaders[2], pixelShaders[2], },
        }};
#endif

#ifdef _GAMING_XBOX_SCARLETT
#ifdef _DEBUG
    const wchar_t* psoPath = L"Assets\\psoset\\Scarlett-Debug\\test_set";
#else
    const wchar_t* psoPath = L"Assets\\psoset\\Scarlett-Release\\test_set";
#endif
#else
#ifdef _DEBUG
    const wchar_t* psoPath = L"Assets\\psoset\\XboxOne-Debug\\test_set";
#else
    const wchar_t* psoPath = L"Assets\\psoset\\XboxOne-Release\\test_set";
#endif
#endif

    // Don't count one-off house keeping objects within driver, these are amortized across many objects
    // These will be exposed via XDK in a future release.
    const uint64_t c_Internal_UniquenessDB = 138;
    const uint64_t c_Internal_PSOSecondaryBlockDB = 149;
    const uint64_t c_Internal_Chunk = 150;

    const std::unordered_set<uint64_t> s_ignoreObjectTypes({ c_Internal_UniquenessDB,
                                                             c_Internal_PSOSecondaryBlockDB,
                                                             c_Internal_Chunk });

    // Create deserialized, derived and regular CreateGraphicsPipelineState PSOs, measuring heap memory and graphics memory
    // for each type of creation. 
    void CreateSamplePSOs(ID3D12Device*                             device,
                          std::vector<ComPtr<ID3D12PipelineState>>& deserialized, 
                          std::vector<ComPtr<ID3D12PipelineState>>& derived,
                          std::vector<ComPtr<ID3D12PipelineState>>& createOnline,
                          ComPtr<ID3D12RootSignature>&              rootSig,
                          std::unique_ptr<ATG::PsoSet>&             newPsoSet,
                          std::wstring&                             deserializedStats,
                          std::wstring&                             derivedStats,
                          std::wstring&                             createGPSOStats)
    {
        // Create PSO set and load common root signature. The PsoSet owns the shader GPU code, and must exist
        // for entire sample. Titles may choose to cache these and unload if necessary.
        newPsoSet = std::make_unique<ATG::PsoSet>(psoPath);
        rootSig = newPsoSet->LoadRootSignature(device);

        // Deserialized effects - these are de-duplicated from individual components that have been created offline,
        // and stored directly in graphics memory. The components are owned by the PsoSet class, not the driver.
        {
            // For the sample we'll hook xmemalloc and record allocations and their types from here
            s_Minitracker.Start();

            for (const auto& s : shaderCombos)
            {
                // LoadCombination will return a PSO in set created from a combination of vertex and pixel shaders.
                // The base pipeline state and root signature are already encoded into the set.
                // If the loaded shader microcode version is incompatible (because it was built with an incompatible XDK)
                // then LoadCombination will throw an exception.
                deserialized.push_back(newPsoSet->LoadCombination(device, s));
            }

            // We have now loaded all the PSO combinations and created the PSOs. We need to retain all the
            // persistent allocations referenced by PSOs. But we can release the transient allocations now.
            // These have been copied by the driver into internal structures.
            //
            // Later, when this object goes out of scope, the sample will release the persistent memory associated with the PSOs.
            newPsoSet->ReleaseTransient();

            // Report allocation totals that occurred, ignoring a few bookkeeping objects that skew the numbers for a small
            // number of PSOs.
            s_Minitracker.ReportAndClear(s_ignoreObjectTypes, deserializedStats);
        }

        // Derived effects - these are created with very little overhead from already existing pipeline state objects
        {
            // For sample, change fill mode to wireframe. 
            D3D12XBOX_DERIVED_GRAPHICS_STATE_DESC desc = {};
            desc.Type = D3D12XBOX_DERIVED_GRAPHICS_STATE_TYPE_RASTERIZER;
            desc.RasterizerDesc = CommonStates::Wireframe;
            desc.RasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

            s_Minitracker.Start();
            
            for (size_t i = 0; i < ATG::Scene::c_numShapes; i++)
            {
                // Derive from deserialized effects. This creates a new PSO that references the internal state of source PSO,
                // storing only a small amount of instance specific data (a new packet).
                Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
                device->CreateDerivedGraphicsPipelineStateX(deserialized[i].Get(), 1, &desc, IID_GRAPHICS_PPV_ARGS(pso.ReleaseAndGetAddressOf()));

                derived.push_back(pso);
            }

            s_Minitracker.ReportAndClear(s_ignoreObjectTypes, derivedStats);
        }

        // CreateGraphicsPipelineState - these are created via vanilla D3D12 API, and have the most overhead because each PSO
        // will store a copy of each shader's GPU code.
        {
            // For the sample, load a PSO desc that is shared with the PSO generation tool. Normally this would be specified
            // in a shared data file and generated by both the tool and the title.
            D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
            ATG::GetSharedPSODescription(desc);
            desc.pRootSignature = rootSig.Get();
      
            // Load all the shader object code. This isn't counted in the memory stats, but could be additional overhead if it
            // was cached by a title.
            typedef std::vector<uint8_t> Blob;
            std::unordered_map<std::wstring, Blob> shaderObjectBlobs;
            for (const auto& s : shaderCombos)
            {
                wchar_t fname[_MAX_PATH] = {};

                // Vertex shader
                swprintf_s(fname, L"%ls.cso", s.vsName);
                shaderObjectBlobs.insert(std::make_pair(s.vsName, DX::ReadData(fname)));
          
                // Pixel shader
                swprintf_s(fname, L"%ls.cso", s.psName);
                shaderObjectBlobs.insert(std::make_pair(s.psName, DX::ReadData(fname)));
            }

            s_Minitracker.Start();

            for (const auto& s : shaderCombos)
            {
                // Vertex shader
                Blob& vsBlob = shaderObjectBlobs[s.vsName];
                desc.VS = { vsBlob.data(), vsBlob.size() };

                // Pixel shader
                Blob& psBlob = shaderObjectBlobs[s.psName];
                desc.PS = { psBlob.data(), psBlob.size() };

                // Create new pipeline state - will duplicate shader GPU code for each PSO and each shader.
                Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
                device->CreateGraphicsPipelineState(&desc, IID_GRAPHICS_PPV_ARGS(pso.ReleaseAndGetAddressOf()));
                createOnline.push_back(pso);
            }

            s_Minitracker.ReportAndClear(s_ignoreObjectTypes, createGPSOStats);
        }

        // Format stats string for UI
        deserializedStats = std::wstring(L"Deserialized:\n") + deserializedStats;
        derivedStats = std::wstring(L"Derived:\n") + derivedStats;
        createGPSOStats = std::wstring(L"CreatePSO:\n") + createGPSOStats;
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        ATG::GetRenderTargetFormat(), ATG::GetDepthFormat(),
        2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();  
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_deviceResources->WaitForOrigin();

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedSeconds = float(timer.GetElapsedSeconds());
    m_deserialized.Update(elapsedSeconds, XMVectorSet(-90.f, -20.f, 0.f, 1.f));
    m_derived.Update(elapsedSeconds, XMVectorSet(-20.f, -20.f, 0.f, 1.f));
    m_base.Update(elapsedSeconds, XMVectorSet(50.f, -20.f, 0.f, 1.f));

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    PIXEndEvent();
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);
 
    PIXBeginEvent(commandList, 1, L"Title & UI Text");
    {
        m_fontBatch->SetViewport(m_deviceResources->GetScreenViewport());
        m_fontBatch->Begin(commandList);

        auto size = m_deviceResources->GetOutputSize();
        float uiScale = (size.bottom > 1440) ? 2.f : ((size.bottom > 1080) ? 1.3f : 1.f);

        // Title
        m_titleFont->DrawString(m_fontBatch.get(), L"AdvancedPSO Sample", XMVectorSet(50 * uiScale, 50 * uiScale, 0, 1), ATG::ColorsLinear::OffWhite);

        wchar_t buffer[256] = {};
#ifdef _GAMING_XBOX_SCARLETT
        swprintf_s(buffer, L"Num PSOs: %lld\nNum unique shaders: %lld", shaderCombos.size(), fetchShaders.size() + vertexShaders.size() + pixelShaders.size());
#else
        swprintf_s(buffer, L"Num PSOs: %lld\nNum unique shaders: %lld", shaderCombos.size(), vertexShaders.size() + pixelShaders.size());
#endif
        m_UiFont->DrawString(m_fontBatch.get(), buffer, XMVectorSet(50 * uiScale, 120 * uiScale, 0, 1), ATG::ColorsLinear::OffWhite);

        // UI
        m_UiFont->DrawString(m_fontBatch.get(), m_deserializedPsoStatString.c_str(), XMVectorSet(200 * uiScale, 750 * uiScale, 0, 1), ATG::ColorsLinear::OffWhite);
        m_UiFont->DrawString(m_fontBatch.get(), m_derivedPsoStatString.c_str(), XMVectorSet(750 * uiScale, 750 * uiScale, 0, 1), ATG::ColorsLinear::OffWhite);
        m_UiFont->DrawString(m_fontBatch.get(), m_createPsoStatString.c_str(), XMVectorSet(1325 * uiScale, 750 * uiScale, 0, 1), ATG::ColorsLinear::OffWhite);

        m_fontBatch->End();
    }
    PIXEndEvent(commandList);

    PIXBeginEvent(commandList, 1, L"Deserialized effects");
    {
        m_deserialized.Draw(commandList);
    }
    PIXEndEvent(commandList);

    PIXBeginEvent(commandList, 1, L"Derived effects");
    {
        m_derived.Draw(commandList);
    }
    PIXEndEvent(commandList);

    // Render all the shapes
    PIXBeginEvent(commandList, 1, L"Base effects");
    {
        m_base.Draw(commandList);
    }
    PIXEndEvent(commandList);

    PIXEndEvent(commandList); // Render

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // Descriptors and resource uploading
    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Font
    {
        const RenderTargetState rtState(ATG::GetRenderTargetFormat(), ATG::GetDepthFormat());

        SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        auto viewPort = m_deviceResources->GetScreenViewport();
        m_fontBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd, &viewPort);

        m_titleFont = std::make_unique<SpriteFont>(
            device,
            resourceUpload,
#ifdef _GAMING_XBOX_SCARLETT
            L"SegoeUI_48.spritefont",
#else
            L"SegoeUI_36.spritefont",
#endif
            m_resourceDescriptors->GetCpuHandle(Descriptors::TitleFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::TitleFont));

        m_UiFont = std::make_unique<SpriteFont>(
            device,
            resourceUpload,
#ifdef _GAMING_XBOX_SCARLETT
            L"SegoeUI_36.spritefont",
#else
            L"SegoeUI_24.spritefont",
#endif
            m_resourceDescriptors->GetCpuHandle(Descriptors::UiFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::UiFont));
    }

    // Shape & effect
    {
        // Load texture
        DX::ThrowIfFailed(DirectX::CreateDDSTextureFromFileEx(device, resourceUpload, L"MossRock.dds",
            0, D3D12_RESOURCE_FLAG_NONE, DDS_LOADER_FORCE_SRGB,
            m_texture.ReleaseAndGetAddressOf()));

        DirectX::CreateShaderResourceView(device, m_texture.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::DefaultTexture), false);

        // Set up the geometric primitive 
        m_base.m_mesh = GeometricPrimitive::CreateDodecahedron(5.f);
        m_derived.m_mesh = GeometricPrimitive::CreateDodecahedron(5.f);
        m_deserialized.m_mesh = GeometricPrimitive::CreateDodecahedron(5.f);
    }

    // Create deserialized, derived and regular CreateGraphicsPipelineState PSOs, measuring their memory usage.
    {
        std::vector<ComPtr<ID3D12PipelineState>> deserialized;
        std::vector<ComPtr<ID3D12PipelineState>> derived;
        std::vector<ComPtr<ID3D12PipelineState>> createOnline;
        ComPtr<ID3D12RootSignature>              rootSig;

        CreateSamplePSOs(device, deserialized, derived, createOnline, rootSig,
                            m_psoSet, m_deserializedPsoStatString, m_derivedPsoStatString, m_createPsoStatString);

        // Deserialized effects
        for (size_t i = 0; i < ATG::Scene::c_numShapes; i++)
        {
            m_deserialized.m_effects[i] = std::make_unique<ATG::CustomEffect>(rootSig, deserialized[i]);
            m_deserialized.m_effects[i]->m_constants.SetLightingConstants();
            m_deserialized.m_effects[i]->m_texture = m_resourceDescriptors->GetGpuHandle(Descriptors::DefaultTexture);
      
            m_derived.m_effects[i] = std::make_unique<ATG::CustomEffect>(rootSig, derived[i]);
            m_derived.m_effects[i]->m_constants.SetLightingConstants();
            m_derived.m_effects[i]->m_texture = m_resourceDescriptors->GetGpuHandle(Descriptors::DefaultTexture);
      
            m_base.m_effects[i] = std::make_unique<ATG::CustomEffect>(rootSig, createOnline[i]);
            m_base.m_effects[i]->m_constants.SetLightingConstants();
            m_base.m_effects[i]->m_texture = m_resourceDescriptors->GetGpuHandle(Descriptors::DefaultTexture);
        }
    }

    // Kick off the resource upload
    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());

    finished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // View parameters
    const auto outputSize = m_deviceResources->GetOutputSize();
    const float aspectRatio = float(outputSize.right) / float(outputSize.bottom);
    constexpr float fovAngleY = 70.0f * XM_PI / 180.0f;

    // This sample makes use of a right-handed coordinate system using row-major matrices.
    XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
        fovAngleY,
        aspectRatio,
        0.01f,
        1000.0f
        );

    const XMVECTORF32 eye = { 0.0f, 20.f, 100.f, 0.0f };
    const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
    const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };
    const XMMATRIX lookAt = XMMatrixLookAtRH(eye, at, up);

    m_base.InitializeViewProj(lookAt, perspectiveMatrix);
    m_deserialized.InitializeViewProj(lookAt, perspectiveMatrix);
    m_derived.InitializeViewProj(lookAt, perspectiveMatrix);
}
#pragma endregion
