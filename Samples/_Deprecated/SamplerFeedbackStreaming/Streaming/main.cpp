//--------------------------------------------------------------------------------------
// main.cpp
//
// Main app class for the Streaming devtest.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "precomp.hpp"
#include "main.h"
#include <stdio.h>
#include "texfill.hpp"

// Compiled scene rendering shaders
#include "VSDefault.vfxhpp"
#include "PSDefault1Layer.pfxhpp"
#include "PSDefault2Layers.pfxhpp"
#include "PSDefault3Layers.pfxhpp"
#include "PSDefault1LayerFilterEmu.pfxhpp"
#include "PSDefault2LayersFilterEmu.pfxhpp"
#include "PSDefault3LayersFilterEmu.pfxhpp"

// Compiled scene rendering shaders (debug component views)
#include "PSNoFeedback1Layer.pfxhpp"
#include "PSShowMinLOD.pfxhpp"
#include "PSShowMinLODWithFilter.pfxhpp"
#include "PSShowFeedback.pfxhpp"

// Compiled UI display shaders
#include "PSMinLODView.pfxhpp"
#include "PSFeedbackView.pfxhpp"
#include "PSFeedbackBufferView.pfxhpp"

static const FLOAT g_DefaultStochasticFraction = 0.007f;

UINT32 g_Seed = 0;
SceneRenderModeVector g_SceneModes;
void SceneRenderModePrintCallback(INT Value, WCHAR* strBuffer, UINT32 BufferSizeChars)
{
    if (Value < (UINT32)g_SceneModes.size())
    {
        wcscpy_s(strBuffer, BufferSizeChars, g_SceneModes[Value].strName);
    }
    else
    {
        strBuffer[0] = L'\0';
    }
}

static void* AllocateThreadData(UINT32 ThreadIndex)
{
    JobQueueThreadData* pThreadData = new JobQueueThreadData();
    pThreadData->ThreadIndex = ThreadIndex;
    pThreadData->hThreadEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    pThreadData->RNG.SetSeed(ThreadIndex * g_Seed);
    return pThreadData;
}

static void FreeThreadData(UINT32 ThreadIndex, void* pThreadData)
{
    JobQueueThreadData* pJQTD = (JobQueueThreadData*)pThreadData;
    CloseHandle(pJQTD->hThreadEvent);
    delete pJQTD;
}

void PercentageFloatPrintCallback(FLOAT Value, WCHAR* strBuffer, UINT32 BufferSizeChars)
{
    swprintf_s(strBuffer, BufferSizeChars, L"%0.1f%%", Value * 100.0f);
}

void DebugDisplaySizePrintCallback(INT Value, WCHAR* strBuffer, UINT32 BufferSizeChars)
{
    Value = std::max(Value, 0);
    UINT32 PrintValue = 128U << Value;
    swprintf_s(strBuffer, BufferSizeChars, L"%u x %u", PrintValue, PrintValue);
}

HRESULT TimestampSeries::Initialize(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pCmdQueue, UINT32 MaxIntervalCount)
{
    m_MaxTimestampCount = MaxIntervalCount + 1;
    m_PreviousTimestampCount = 0;

    m_pTimeUsec = new DOUBLE[m_MaxTimestampCount];
    if (m_pTimeUsec == nullptr)
    {
        return E_OUTOFMEMORY;
    }
    ZeroMemory(m_pTimeUsec, MaxIntervalCount * sizeof(DOUBLE));
    m_SmoothingSpeed = 1.0;

    UINT64 TickFrequency = 0;
    pCmdQueue->GetTimestampFrequency(&TickFrequency);
    m_InverseTickFrequency = 1.0 / (DOUBLE)TickFrequency;

    HRESULT hr;

    D3D12_QUERY_HEAP_DESC QueryHeapDesc = {};
    QueryHeapDesc.Count = m_MaxTimestampCount * 2;
    QueryHeapDesc.NodeMask = D3D12XBOX_NODE_MASK;
    QueryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    hr = pd3dDevice->CreateQueryHeap(&QueryHeapDesc, __uuidof(ID3D12QueryHeap), (void**)&m_pQueryHeap);
    if (FAILED(hr)) return hr;

    CD3DX12_RESOURCE_DESC QueryBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(QueryHeapDesc.Count * sizeof(UINT64));
    CD3DX12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE_READBACK);
    hr = pd3dDevice->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &QueryBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), (void**)&m_pQueryBuffer);
    if (FAILED(hr)) return hr;
    m_pQueryBuffer->SetName(L"Query Buffer");

    hr = m_pQueryBuffer->Map(0, nullptr, (void**)&m_pQueryData);
    if (FAILED(hr)) return hr;

#if defined(_XBOX_ONE) && defined(_TITLE)
    ZeroMemory(m_pQueryData, QueryBufferDesc.Width);
#endif

    return S_OK;
}

void TimestampSeries::Terminate()
{
    delete[] m_pTimeUsec;
    m_pQueryData = nullptr;

    if (m_pQueryBuffer != nullptr)
    {
        m_pQueryBuffer->Unmap(0, nullptr);
        m_pQueryBuffer->Release();
        m_pQueryBuffer = nullptr;
    }
    if (m_pQueryHeap != nullptr)
    {
        m_pQueryHeap->Release();
        m_pQueryHeap = nullptr;
    }
}

void TimestampSeries::BeginFrame(ID3D12GraphicsCommandList* pCmdList)
{
    m_pCurrentCmdList = pCmdList;
    m_TimestampIndex = 0;
    MarkTimestamp();
}

void TimestampSeries::MarkTimestamp()
{
    assert(m_pCurrentCmdList != nullptr);
    assert(m_TimestampIndex < m_MaxTimestampCount);

    m_pCurrentCmdList->EndQuery(m_pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, m_TimestampIndex);
    ++m_TimestampIndex;
}

DOUBLE TimestampSeries::EndFrame(UINT32* pIntervalCount, DOUBLE** ppIntervalTimesUsec)
{
    MarkTimestamp();
    m_pCurrentCmdList->ResolveQueryData(m_pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, m_TimestampIndex, m_pQueryBuffer, 0);
    m_pCurrentCmdList = nullptr;

    if (m_TimestampIndex < 2)
    {
        m_PreviousTimestampCount = 0;
        if (pIntervalCount != nullptr)
        {
            *pIntervalCount = 0;
        }
        if (ppIntervalTimesUsec != nullptr)
        {
            *ppIntervalTimesUsec = nullptr;
        }
        return 0;
    }

    const UINT32 IntervalCount = m_TimestampIndex - 1;
    for (UINT32 i = 0; i < IntervalCount; ++i)
    {
        const UINT64 IntervalTicks = m_pQueryData[i + 1] - m_pQueryData[i];
        const DOUBLE IntervalTimeUsec = (DOUBLE)IntervalTicks * m_InverseTickFrequency * 1e6;
        m_pTimeUsec[i] = m_pTimeUsec[i] * (1.0 - m_SmoothingSpeed) + IntervalTimeUsec * m_SmoothingSpeed;
    }

    const UINT64 WholeFrameTicks = m_pQueryData[IntervalCount] - m_pQueryData[0];
    const DOUBLE WholeTimeSec = (DOUBLE)WholeFrameTicks * m_InverseTickFrequency;
    const DOUBLE WholeTimeUsec = WholeTimeSec * 1e6;
    m_pTimeUsec[IntervalCount] = m_pTimeUsec[IntervalCount] * (1.0 - m_SmoothingSpeed) + WholeTimeUsec * m_SmoothingSpeed;

    m_SmoothingSpeed = 50.0 * WholeTimeSec;
    if (m_SmoothingSpeed > 1.0)
    {
        m_SmoothingSpeed = 1.0;
    }

    m_PreviousTimestampCount = m_TimestampIndex;
    if (pIntervalCount != nullptr)
    {
        *pIntervalCount = IntervalCount;
    }
    if (ppIntervalTimesUsec != nullptr)
    {
        *ppIntervalTimesUsec = m_pTimeUsec;
    }
    return m_pTimeUsec[IntervalCount];
}

DOUBLE TimestampSeries::GetPreviousFrame(UINT32* pIntervalCount, DOUBLE** ppIntervalTimesUsec)
{
    if (m_PreviousTimestampCount < 2)
    {
        *pIntervalCount = 0;
        *ppIntervalTimesUsec = nullptr;
        return 0;
    }

    const UINT32 IntervalCount = m_PreviousTimestampCount - 1;
    *pIntervalCount = IntervalCount;
    *ppIntervalTimesUsec = m_pTimeUsec;
    return m_pTimeUsec[IntervalCount];
}

DevTestApp::DevTestApp(void)
    : m_pDmaCmdQueue(nullptr)
{
    g_Seed = 12345;
    JobQueueDesc QueueDesc = {};
    QueueDesc.ThreadCount = 5;
    //QueueDesc.ThreadCount = 1;
    QueueDesc.pJobThreadDataInitFunction = AllocateThreadData;
    QueueDesc.pJobThreadDataFreeFunction = FreeThreadData;
    g_JobQueue.Initialize(&QueueDesc);
    m_EmulateLeftStickFromKeyboard = true;
}

DevTestApp::~DevTestApp(void)
{
}

void DevTestApp::PreWindowInit( ThinWinInit* pInit )
{
    wcscpy_s( pInit->strWindowTitle, L"D3D12 Streaming" );

#if defined(_GAMING_XBOX) || defined(_GAMING_XBOX_SCARLETT)
    pInit->Width = 3840;
    pInit->Height = 2160;
#endif
}

HRESULT DevTestApp::PreD3DInitialize( D3DInitParameters* pD3DInitParams )
{
    pD3DInitParams->BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    pD3DInitParams->DepthBufferFormat = DXGI_FORMAT_D32_FLOAT;
    pD3DInitParams->CreateUISprite = TRUE;

    // When changing this root signature, make sure to update the HLSL version to match in RootSignature.hlsli
    pD3DInitParams->RTSlot[0].InitAsConstantBufferView(0);
    pD3DInitParams->RTSlot[1].InitAsConstantBufferView(1);
    pD3DInitParams->DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 9, 0, 0); // t0-t8 space 0 (tiled resource surface maps)
    pD3DInitParams->DescRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 9, 0, 1); // t0-t8 space 1 (MinLOD SRVs)
    pD3DInitParams->DescRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 9, 0, 2); // u0-u8 space 2 (Feedback UAVs)
    pD3DInitParams->DescRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 16, 0, 0); // s0-s15 (Samplers)
    pD3DInitParams->RTSlot[2].InitAsDescriptorTable(1, &pD3DInitParams->DescRange[0], D3D12_SHADER_VISIBILITY_PIXEL); // t0-t8 space 0
    pD3DInitParams->RTSlot[3].InitAsDescriptorTable(1, &pD3DInitParams->DescRange[1], D3D12_SHADER_VISIBILITY_PIXEL); // t0-t8 space 1
    pD3DInitParams->RTSlot[4].InitAsDescriptorTable(1, &pD3DInitParams->DescRange[2], D3D12_SHADER_VISIBILITY_PIXEL); // u0-u8 space 2
    pD3DInitParams->RTSlot[5].InitAsDescriptorTable(1, &pD3DInitParams->DescRange[3], D3D12_SHADER_VISIBILITY_PIXEL); // s0-s15
//     pD3DInitParams->StaticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // s0
//     pD3DInitParams->StaticSamplers[1].Init(14, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // s14
//     pD3DInitParams->StaticSamplers[2].Init(15, D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // s15
    pD3DInitParams->RootSignatureSlotCount = 6;
//     pD3DInitParams->StaticSamplerCount = 3;

#if defined(_XBOX_ONE) && defined(_TITLE) && !DBG
    pD3DInitParams->CreateDeviceParameters.ProcessDebugFlags = D3D12XBOX_PROCESS_DEBUG_FLAG_INSTRUMENTED;
#endif

    return S_OK;
}

HRESULT DevTestApp::Initialize()
{
    HRESULT hr;

#if !defined(_XBOX_ONE)
    ID3D12InfoQueue* pInfoQueue = nullptr;
    hr = m_pd3dDevice->QueryInterface(__uuidof(pInfoQueue), (void**)&pInfoQueue);
    if (SUCCEEDED(hr) && pInfoQueue != nullptr)
    {
        D3D12_MESSAGE_ID IgnoreIDs[] = 
        { 
//             D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE, 
//             D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_GPU_WRITTEN_READBACK_RESOURCE_MAPPED,
        };

        D3D12_INFO_QUEUE_FILTER Filter = {};
        Filter.DenyList.NumIDs = ARRAYSIZE(IgnoreIDs);
        Filter.DenyList.pIDList = IgnoreIDs;
        hr = pInfoQueue->AddStorageFilterEntries(&Filter);

        pInfoQueue->Release();
    }
#endif

    m_SlowLoading = false;
    m_SlowDecompressing = false;
    m_PauseLoading = false;
    m_PauseAgeOut = false;
    m_SceneRenderMode = 0;
    m_DebugTextureComponent = STCT_Count;
    m_DebugTextureLayer = ST_Diffuse;
    m_pSelectedSceneObjectInstance = nullptr;
    m_DebugTextureSizeShift = 1;
    m_DebugOffsetX = 0;
    m_DebugOffsetY = 0;
    m_RandomLoads = false;
    m_DoNotLoadUpperLeft = false;
    m_ShowStreamingStats = true;
    m_ShowCpuTimingStats = false;
    m_ShowGpuStats = true;
    m_AutoCameraMovement = 0;
    m_AutoCameraTime = 0;
    m_UseDebugMinLODMap = false;
    m_PresentImmediate = true;
    m_useDMAForStreaming = false;
    m_FlushRequested = false;
    m_CycleCameraLocation = false;
    m_MarkTileBoundaries = false;
    m_FullscreenMode = IsFullscreenMode();

    m_FilterSlopeUIndex = 3;
    m_FilterSlopeVIndex = 3;
    m_FilterOffsetUIndex = 3;
    m_FilterOffsetVIndex = 3;

    m_StochasticFraction = g_DefaultStochasticFraction;

    m_RNG.SetSeed(g_Seed);
    LocateTextures();
    m_StreamingTextureCount = GetSceneTextureCount();
    m_SceneCreated = false;

#if defined(_XBOX_ONE) && defined(_TITLE)
    D3D12XBOX_COMMAND_QUEUE_DESC DmaQueueDesc = {};
    DmaQueueDesc.Type = D3D12XBOX_COMMAND_LIST_TYPE_DMA;
    DmaQueueDesc.EngineOrPipeIndex = 1;
    hr = m_pd3dDevice->CreateCommandQueueX(&DmaQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&m_pDmaCmdQueue);
#else
    D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
    QueueDesc.NodeMask = D3D12XBOX_NODE_MASK;
    QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    hr = m_pd3dDevice->CreateCommandQueue(&QueueDesc, __uuidof(*m_pDmaCmdQueue), (void**)&m_pDmaCmdQueue);
#endif
    if (FAILED(hr)) { return hr; }

    ZeroMemory(m_PrevFrameSceneRenderProbes, sizeof(m_PrevFrameSceneRenderProbes));

    m_Sprite.SetOffset(XMVectorSet(m_SpriteOffset, m_SpriteOffset, 0, 0));

    ID3D12GraphicsCommandList* pInitCmdList = CreateCommandList();

    g_StreamingTextureManager.Initialize(m_pd3dDevice, pInitCmdList, 8192, 1000);

    hr = InitializeSceneObjects(m_pd3dDevice, pInitCmdList, &m_UploadHeap);
    if (FAILED(hr))
    {
        return hr;
    }

    {
        m_SRVHeap.Initialize(m_pd3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100, true);

        m_DebugMinLODMapDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UNORM, 8, 8, 1, 1, 1);
        hr = CreateDefaultResource(m_pd3dDevice, &m_DebugMinLODMapDesc, D3D12_RESOURCE_STATE_COPY_DEST, (void**)&m_pDebugMinLODMap);
        assert(SUCCEEDED(hr));

        D3D12_RESOURCE_DESC FakePrimaryMapDesc = m_DebugMinLODMapDesc;
        FakePrimaryMapDesc.Width *= 128;
        FakePrimaryMapDesc.Height *= 128;
        hr = CreateDefaultResource(m_pd3dDevice, &FakePrimaryMapDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, (void**)&m_pDebugFakePrimaryMap);
        assert(SUCCEEDED(hr));

        m_pd3dDevice->CreateShaderResourceView(m_pDebugFakePrimaryMap, nullptr, m_SRVHeap.hCPU(0));
        m_pd3dDevice->CreateShaderResourceView(m_pDebugMinLODMap, nullptr, m_SRVHeap.hCPU(1));
        m_hDebugMinLODSRV = m_SRVHeap.hGPU(0);

        const BYTE InitData[64] = 
        {   
            16, 16, 16, 16, 32, 32, 32, 32,
            16, 16, 16, 16, 32, 32, 32, 32,
            16, 16,  8,  8, 32, 32, 32, 32,
            16, 16,  8,  0, 32, 32, 32, 32,
            32, 32, 32, 32, 32, 32, 32, 32,
            32, 32, 32, 32, 32, 32, 32, 32,
            32, 32, 32, 32, 32, 32, 32, 32,
            32, 32, 32, 32, 32, 32, 32, 32,
        };
        UINT32 RowPitchBytes = 8;

        D3D12_SUBRESOURCE_FOOTPRINT PitchDesc = {};
        PitchDesc.Width = (UINT32)m_DebugMinLODMapDesc.Width;
        PitchDesc.Height = m_DebugMinLODMapDesc.Height;
        PitchDesc.Depth = 1;
        PitchDesc.Format = m_DebugMinLODMapDesc.Format;
        PitchDesc.RowPitch = NextMultiple(RowPitchBytes, (UINT32)D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT);

        m_UploadHeap.CopyTextureSubresourceToDefaultTexture(InitData, RowPitchBytes, PitchDesc, pInitCmdList, m_pDebugMinLODMap, 0);
        ResourceBarrier(pInitCmdList, m_pDebugMinLODMap, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    pInitCmdList->Close();
    m_pd3dCmdQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&pInitCmdList);
    SAFE_RELEASE(pInitCmdList);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
    PSODesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
    PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    PSODesc.DepthStencilState.DepthEnable = TRUE;
    PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    PSODesc.InputLayout.NumElements = ARRAYSIZE(SceneVertexElements);
    PSODesc.InputLayout.pInputElementDescs = SceneVertexElements;
    PSODesc.NumRenderTargets = 1;
    PSODesc.RTVFormats[0] = m_InitParams.BackBufferFormat;
    PSODesc.DSVFormat = m_InitParams.DepthBufferFormat;
    PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    PSODesc.RasterizerState.DepthClipEnable = TRUE;
    PSODesc.SampleDesc.Count = 1;
    PSODesc.SampleMask = -1;
    PSODesc.pRootSignature = m_pDefaultRootSignature;

    PSODesc.VS.pShaderBytecode = VSDefault_vfxhpp;
    PSODesc.VS.BytecodeLength = sizeof(VSDefault_vfxhpp);

    PSODesc.PS.pShaderBytecode = PSDefault1Layer_pfxhpp;
    PSODesc.PS.BytecodeLength = sizeof(PSDefault1Layer_pfxhpp);

    hr = m_pd3dDevice->CreateGraphicsPipelineState(&PSODesc, __uuidof(*m_pScene1PSO), (void**)&m_pScene1PSO);
    if (FAILED(hr))
    {
        return hr;
    }

    PSODesc.PS.pShaderBytecode = PSDefault2Layers_pfxhpp;
    PSODesc.PS.BytecodeLength = sizeof(PSDefault2Layers_pfxhpp);

    hr = m_pd3dDevice->CreateGraphicsPipelineState(&PSODesc, __uuidof(*m_pScene2PSO), (void**)&m_pScene2PSO);
    if (FAILED(hr))
    {
        return hr;
    }

    PSODesc.PS.pShaderBytecode = PSDefault3Layers_pfxhpp;
    PSODesc.PS.BytecodeLength = sizeof(PSDefault3Layers_pfxhpp);

    hr = m_pd3dDevice->CreateGraphicsPipelineState(&PSODesc, __uuidof(*m_pScene3PSO), (void**)&m_pScene3PSO);
    if (FAILED(hr))
    {
        return hr;
    }

    PSODesc.PS.pShaderBytecode = PSDefault1LayerFilterEmu_pfxhpp;
    PSODesc.PS.BytecodeLength = sizeof(PSDefault1LayerFilterEmu_pfxhpp);

    hr = m_pd3dDevice->CreateGraphicsPipelineState(&PSODesc, __uuidof(*m_pScene1FilterEmuPSO), (void**)&m_pScene1FilterEmuPSO);
    if (FAILED(hr))
    {
        return hr;
    }

    PSODesc.PS.pShaderBytecode = PSDefault2LayersFilterEmu_pfxhpp;
    PSODesc.PS.BytecodeLength = sizeof(PSDefault2LayersFilterEmu_pfxhpp);

    hr = m_pd3dDevice->CreateGraphicsPipelineState(&PSODesc, __uuidof(*m_pScene2FilterEmuPSO), (void**)&m_pScene2FilterEmuPSO);
    if (FAILED(hr))
    {
        return hr;
    }

    PSODesc.PS.pShaderBytecode = PSDefault3LayersFilterEmu_pfxhpp;
    PSODesc.PS.BytecodeLength = sizeof(PSDefault3LayersFilterEmu_pfxhpp);

    hr = m_pd3dDevice->CreateGraphicsPipelineState(&PSODesc, __uuidof(*m_pScene3FilterEmuPSO), (void**)&m_pScene3FilterEmuPSO);
    if (FAILED(hr))
    {
        return hr;
    }

    PSODesc.PS.pShaderBytecode = PSNoFeedback1Layer_pfxhpp;
    PSODesc.PS.BytecodeLength = sizeof(PSNoFeedback1Layer_pfxhpp);

    hr = m_pd3dDevice->CreateGraphicsPipelineState(&PSODesc, __uuidof(*m_pScene1NoFeedbackPSO), (void**)&m_pScene1NoFeedbackPSO);
    if (FAILED(hr))
    {
        return hr;
    }

    PSODesc.PS.pShaderBytecode = PSShowMinLOD_pfxhpp;
    PSODesc.PS.BytecodeLength = sizeof(PSShowMinLOD_pfxhpp);

    hr = m_pd3dDevice->CreateGraphicsPipelineState(&PSODesc, __uuidof(*m_pScene1ShowMinLODPSO), (void**)&m_pScene1ShowMinLODPSO);
    if (FAILED(hr))
    {
        return hr;
    }

    PSODesc.PS.pShaderBytecode = PSShowMinLODWithFilter_pfxhpp;
    PSODesc.PS.BytecodeLength = sizeof(PSShowMinLODWithFilter_pfxhpp);

    hr = m_pd3dDevice->CreateGraphicsPipelineState(&PSODesc, __uuidof(*m_pScene1ShowMinLODWithFilterPSO), (void**)&m_pScene1ShowMinLODWithFilterPSO);
    if (FAILED(hr))
    {
        return hr;
    }

    PSODesc.PS.pShaderBytecode = PSShowFeedback_pfxhpp;
    PSODesc.PS.BytecodeLength = sizeof(PSShowFeedback_pfxhpp);

    hr = m_pd3dDevice->CreateGraphicsPipelineState(&PSODesc, __uuidof(*m_pScene1ShowFeedbackPSO), (void**)&m_pScene1ShowFeedbackPSO);
    if (FAILED(hr))
    {
        return hr;
    }

#if !XBOX_SAMPLER_FEEDBACK
    AddSceneRenderMode(L"Default 1 Layer", m_pScene1PSO, false);
#endif
    AddSceneRenderMode(L"Default 1 Layer (MinLOD Filter)", m_pScene1FilterEmuPSO, false);
#if !XBOX_SAMPLER_FEEDBACK
    AddSceneRenderMode(L"Default 2 Layers", m_pScene2PSO, false);
#endif
    AddSceneRenderMode(L"Default 2 Layers (MinLOD Filter)", m_pScene2FilterEmuPSO, false);
#if !XBOX_SAMPLER_FEEDBACK
    AddSceneRenderMode(L"Default 3 Layers", m_pScene3PSO, false);
#endif
    AddSceneRenderMode(L"Default 3 Layers (MinLOD Filter)", m_pScene3FilterEmuPSO, false);
    AddSceneRenderMode(L"No Feedback 1 Layer", m_pScene1NoFeedbackPSO, true);
#if !XBOX_SAMPLER_FEEDBACK
    AddSceneRenderMode(L"Diffuse MinLOD Map", m_pScene1ShowMinLODPSO, false);
#endif
    AddSceneRenderMode(L"Diffuse MinLOD Map (MinLOD Filter)", m_pScene1ShowMinLODWithFilterPSO, false);
    AddSceneRenderMode(L"Diffuse MinLOD Feedback Map", m_pScene1ShowFeedbackPSO, false);

    m_CameraPos = XMFLOAT3(0, 0, 0);
    CycleCameraLocation();

    g_SceneGraph.Initialize(m_pd3dDevice, m_pGpuFence, &m_CpuFence);
    
    m_Menu.AddPrintCallbackUint(L"Render Mode", &m_SceneRenderMode, 0, (UINT32)g_SceneModes.size() - 1, SceneRenderModePrintCallback);
    m_Menu.AddFloat(L"Feedback Fraction (X)", &m_StochasticFraction, 0, 1, 0.05f, PercentageFloatPrintCallback);

    m_Menu.AddChildMenu(L"MinLOD Filtering Options");
    m_Menu.AddInt(L"MinLOD Map Filter U Offset", &m_FilterOffsetUIndex, 0, ARRAYSIZE(g_FilterOffsetValues) - 1, g_strFilterOffsetValues);
    m_Menu.AddInt(L"MinLOD Map Filter V Offset", &m_FilterOffsetVIndex, 0, ARRAYSIZE(g_FilterOffsetValues) - 1, g_strFilterOffsetValues);
    m_Menu.AddInt(L"MinLOD Map Filter U Slope", &m_FilterSlopeUIndex, 0, ARRAYSIZE(g_FilterSlopeValues) - 1, g_strFilterSlopeValues);
    m_Menu.AddInt(L"MinLOD Map Filter U Slope", &m_FilterSlopeVIndex, 0, ARRAYSIZE(g_FilterSlopeValues) - 1, g_strFilterSlopeValues);
    m_Menu.NavigateToRoot();

    m_Menu.AddChildMenu(L"Streaming Debug Options");
    m_Menu.AddBool(L"Do Not Load Upper Left", &m_DoNotLoadUpperLeft);
    m_Menu.AddBool(L"Slow Loading", &m_SlowLoading);
    m_Menu.AddBool(L"Slow Decompressing", &m_SlowDecompressing);
    m_Menu.AddBool(L"Pause Loading", &m_PauseLoading);
    m_Menu.AddBool(L"Pause Age-Out", &m_PauseAgeOut);
    m_Menu.AddBool(L"Random Loads", &m_RandomLoads);
    m_Menu.AddBool(L"Mark Tile Boundaries", &m_MarkTileBoundaries);
    m_Menu.AddUint(L"MinLOD Map Edit Delay", g_StreamingTextureManager.GetMinLODMapEditDelayCount(), 0, 20);
    m_Menu.AddBool(L"Flush Loaded Tiles (Menu)", &m_FlushRequested);
    m_Menu.NavigateToRoot();

    m_Menu.AddChildMenu(L"Streaming Debug Views");
    m_Menu.AddBool(L"Show Streaming Statistics", &m_ShowStreamingStats);
    m_Menu.AddBool(L"Show CPU Timings", &m_ShowCpuTimingStats);
    static const WCHAR* strDebugComponentEnums[] = { L"Primary", L"MinLOD", L"Feedback", L"Disabled" };
    m_Menu.AddInt(L"Debug Texture Mode", (INT*)&m_DebugTextureComponent, 0, STCT_Count, strDebugComponentEnums);
    static const WCHAR* strDebugLayerEnums[] = { L"Diffuse", L"Normal", L"Specular" };
    m_Menu.AddInt(L"Debug Texture Layer", (INT*)&m_DebugTextureLayer, ST_Diffuse, ST_Specular, strDebugLayerEnums);
    m_Menu.AddPrintCallbackUint(L"Debug Texture Display Size", &m_DebugTextureSizeShift, 0, 6, DebugDisplaySizePrintCallback);
    m_Menu.AddFloat(L"Debug Texture X Offset", &m_DebugOffsetX, 0, 2000, 250);
    m_Menu.AddFloat(L"Debug Texture Y Offset", &m_DebugOffsetY, 0, 2000, 250);
    m_Menu.AddBool(L"Show Debug MinLOD Map", &m_UseDebugMinLODMap);
    m_Menu.NavigateToRoot();

    m_Menu.AddChildMenu(L"Scene and Rendering");
    m_Menu.AddBool(L"Present Immediate", &m_PresentImmediate);
    m_Menu.AddBool(L"Use DMA For Streaming", &m_useDMAForStreaming);
#if! defined(_XBOX_ONE)
    m_Menu.AddBool(L"Full Screen", &m_FullscreenMode);
#endif
    m_Menu.AddBool(L"Show GPU Stats", &m_ShowGpuStats);
    m_Menu.AddUint(L"Auto Camera Movement (B)", &m_AutoCameraMovement, 0, 10);
    m_Menu.AddBool(L"Cycle Camera Location (Y)", &m_CycleCameraLocation);
    m_Menu.NavigateToRoot();

    m_MinLODViewShader.pShaderBytecode = PSMinLODView_pfxhpp;
    m_MinLODViewShader.BytecodeLength = sizeof(PSMinLODView_pfxhpp);
    m_FeedbackViewShader.pShaderBytecode = PSFeedbackView_pfxhpp;
    m_FeedbackViewShader.BytecodeLength = sizeof(PSFeedbackView_pfxhpp);
    m_FeedbackBufferViewShader.pShaderBytecode = PSFeedbackBufferView_pfxhpp;
    m_FeedbackBufferViewShader.BytecodeLength = sizeof(PSFeedbackBufferView_pfxhpp);

    hr = m_Timestamps.Initialize(m_pd3dDevice, m_pd3dCmdQueue);
    if (FAILED(hr)) return hr;

    return S_OK;
}

void DevTestApp::CycleCameraLocation()
{
    struct CameraLocation
    {
        FLOAT Position[3];
        FLOAT Pitch;
        FLOAT Yaw;
    };

    static const CameraLocation s_FixedCameraLocations[] =
    {
        { { 50.0f      , 50.0f      , -50.0f       }, 0.65f        , -0.7853f      },
        { { 30.8299122f, 13.1248751f, -35.8400459f }, 0.0580393523f, -0.761360049f },
        { { 31.1142139f, 10.9941101f, -18.8752632f }, 0.115914091f , -2.34915209f  },
        { { 40.8973923f, 9.63434792f,  31.9422932f }, 0.0973547027f, -8.63038254f  },
    };

    INT32 FoundIndex = -1;
    for (UINT32 i = 0; i < ARRAYSIZE(s_FixedCameraLocations); ++i)
    {
        if (m_CameraPos.x == s_FixedCameraLocations[i].Position[0] &&
            m_CameraPos.y == s_FixedCameraLocations[i].Position[1] &&
            m_CameraPos.z == s_FixedCameraLocations[i].Position[2])
        {
            FoundIndex = (INT32)i;
            break;
        }
    }

    INT32 NextIndex = (FoundIndex + 1) % ARRAYSIZE(s_FixedCameraLocations);
    const CameraLocation& CL = s_FixedCameraLocations[NextIndex];
    m_CameraPos = XMFLOAT3(CL.Position[0], CL.Position[1], CL.Position[2]);
    m_CameraPitch = CL.Pitch;
    m_CameraYaw = CL.Yaw;
}

void DevTestApp::CreateSceneObjects()
{
    RandomNumberGenerator TexRNG;
    TexRNG.SetSeed(g_Seed + 1);

    WCHAR strObjectName[64];
    UINT32 TextureIndex = 0;
    const INT32 CitySize = 10;
    const FLOAT Spacing = 5.0f;
    for (INT32 y = -CitySize; y <= CitySize; ++y)
    {
        for (INT32 x = -CitySize; x <= CitySize; ++x)
        {
            XMVECTOR vPos = XMVectorSet(x, 0, y, 0) * Spacing;
            FLOAT HalfHeight = m_RNG.NextFloat(0.5f, 3.0f);
            vPos = XMVectorSetY(vPos, HalfHeight);
            XMVECTOR Scale = XMVectorSet(m_RNG.NextFloat(0.9f, 1.1f), HalfHeight, m_RNG.NextFloat(0.9f, 1.1f), 1);

            UINT32 BuildingHeight = m_RNG.NextInt(1, 5);
            for (UINT32 z = 0; z < BuildingHeight; ++z)
            {
                swprintf_s(strObjectName, L"Cube x%d y%d z%u", x, y, z);

                const UINT32 TextureIndex2 = TexRNG.NextInt(0, m_StreamingTextureCount - 1);
                const UINT32 TextureIndex3 = TexRNG.NextInt(0, m_StreamingTextureCount - 1);

                g_SceneGraph.AddInstance(g_pCubeObject, vPos, g_XMIdentityR3, Scale, TextureIndex, TextureIndex2, TextureIndex3, strObjectName);
                TextureIndex = (TextureIndex + 1) % m_StreamingTextureCount;

                const FLOAT ScaleFactor = 0.9f;
                FLOAT NewHalfHeight = HalfHeight * ScaleFactor;
                Scale *= ScaleFactor;
                vPos += XMVectorSet(0, HalfHeight + NewHalfHeight, 0, 0);
                HalfHeight = NewHalfHeight;
            }
        }
    }
}

HRESULT DevTestApp::Terminate()
{
    return S_OK;
}

LRESULT DevTestApp::MouseMsgProc( UINT message, WPARAM wParam, LPARAM lParam )
{
    return 0;
}

LRESULT DevTestApp::KeyboardMsgProc( UINT message, WPARAM wParam, LPARAM lParam )
{
    return 0;
}

HRESULT DevTestApp::Update( BOOL Resized )
{
    if (Resized)
    {
        XMMATRIX matProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, m_AspectRatio, 0.01f, 1000.0f);
        XMStoreFloat4x4(&m_matProjection, matProj);
    }

    if (!m_SceneCreated)
    {
        if (m_StreamingTextureCount > 0)
        {
            CreateSceneObjects();
            m_SceneCreated = true;
        }
        else
        {
            m_StreamingTextureCount = GetSceneTextureCount();
        }
    }

    const FLOAT FracTime = (FLOAT)(m_Timer.GetAbsoluteTime() - floor(m_Timer.GetAbsoluteTime()));
    if (FracTime < 0.5f)
    {
        g_SceneGraph.SetNonResidentColor(XMVectorSet(1, 0, 0, 1));
    }
    else
    {
        g_SceneGraph.SetNonResidentColor(XMVectorSet(1, 0, 1, 1));
    }

#if !defined(_XBOX_ONE)
    // Disable stochastic discard on NVIDIA GPUs
    if (m_SelectedAdapterDesc.VendorId == 0x10de)
    {
        m_StochasticFraction = 1.0f;
    }
#endif

    g_SceneGraph.SetStochasticConstants(FracTime, m_StochasticFraction);

    const FLOAT DeltaTime = (FLOAT)m_Timer.GetDeltaTime();

    {
        if (IsButtonDown(GamepadButtons::LeftShoulder) && m_DebugTextureComponent < STCT_Count)
        {
            const FLOAT fSpeed = 1000.0f;
            m_DebugOffsetX += DeltaTime * m_ThumbLeftX * fSpeed;
            m_DebugOffsetY += DeltaTime * m_ThumbLeftY * fSpeed;
            m_DebugOffsetX = std::max(0.0f, m_DebugOffsetX);
            m_DebugOffsetY = std::max(0.0f, m_DebugOffsetY);
        }
        else if (m_AutoCameraMovement > 0)
        {
            const FLOAT MovementSpeed = (FLOAT)m_AutoCameraMovement * 0.05f;
            m_AutoCameraTime += MovementSpeed * DeltaTime;
            m_AutoCameraTime = fmodf(m_AutoCameraTime, XM_2PI);

            const FLOAT CamRadius = 30.0f;
            const FLOAT CamAltitude = 6.0f;
            const FLOAT CamAltitudeAdd = 2.0f;
            XMVECTOR Translation = XMVectorSet(CamRadius * sinf(m_AutoCameraTime), CamAltitude + CamAltitudeAdd * sinf(m_AutoCameraTime * 2.0f), CamRadius * cosf(m_AutoCameraTime), 1);
            XMStoreFloat3(&m_CameraPos, Translation);

            m_CameraYaw = m_AutoCameraTime + XM_PIDIV2;
            m_CameraPitch = 0;
            XMMATRIX matCameraWorld = XMMatrixRotationRollPitchYaw(0, m_CameraYaw, 0);
            matCameraWorld.r[3] = Translation;

            XMVECTOR vDet;
            XMStoreFloat4x4(&m_matView, XMMatrixInverse(&vDet, matCameraWorld));
        }
        else
        {
            const FLOAT ScaledRotationSpeed = 1.0f * DeltaTime;
            m_CameraYaw += ScaledRotationSpeed * m_ThumbRightX;
            m_CameraPitch += ScaledRotationSpeed * -m_ThumbRightY;
            XMMATRIX matCameraWorld = XMMatrixRotationRollPitchYaw(m_CameraPitch, m_CameraYaw, 0);

            const FLOAT ScaledTranslationSpeed = DeltaTime * (IsButtonDown(GamepadButtons::RightShoulder) ? 10.0f : 1.0f);
            XMVECTOR vTranslation = XMVectorZero();
            vTranslation += XMVectorScale(matCameraWorld.r[0], ScaledTranslationSpeed * m_ThumbLeftX);
            if (IsButtonDown(GamepadButtons::LeftThumbstick))
            {
                vTranslation += XMVectorScale(matCameraWorld.r[1], ScaledTranslationSpeed * m_ThumbLeftY);
            }
            else
            {
                vTranslation += XMVectorScale(matCameraWorld.r[2], ScaledTranslationSpeed * m_ThumbLeftY);
            }
            XMVECTOR CurrentTranslation = XMLoadFloat3(&m_CameraPos);
            CurrentTranslation += vTranslation;
            XMStoreFloat3(&m_CameraPos, CurrentTranslation);
            matCameraWorld.r[3] = XMVectorSelect(g_XMOne, CurrentTranslation, g_XMSelect1110);

            XMVECTOR vDet;
            XMStoreFloat4x4(&m_matView, XMMatrixInverse(&vDet, matCameraWorld));

            if (m_DebugTextureComponent < STCT_Count)
            {
                m_pSelectedSceneObjectInstance = g_SceneGraph.FindSceneObjectInstance(matCameraWorld.r[3], matCameraWorld.r[2]);
            }
            else
            {
                m_pSelectedSceneObjectInstance = nullptr;
            }
        }
    }

    const BOOL MenuButtonPressed = m_Menu.Update((UINT32)m_LastReading.Buttons, m_PressedButtons, DeltaTime);
    if (MenuButtonPressed)
    {
        const SceneRenderMode& SRM = g_SceneModes[m_SceneRenderMode];
        g_StreamingTextureManager.SetLoadSleepMsec(m_SlowLoading ? 1000 : 0);
        g_StreamingTextureManager.SetDecompressSleepMsec(m_SlowDecompressing ? 1000 : 0);
        g_StreamingTextureManager.FreezeLoading(m_PauseLoading || SRM.FreezeStreaming);
        g_StreamingTextureManager.FreezeAgeOut(m_PauseAgeOut || SRM.FreezeStreaming);
        g_StreamingTextureManager.SetRandomLoadsPerFrame(m_RandomLoads ? 10 : 0);
        g_StreamingTextureManager.SetDoNotLoadUpperLeft(m_DoNotLoadUpperLeft);
        g_StreamingTextureManager.SetMarkTileBoundaries(m_MarkTileBoundaries);
        SetFullscreenMode(m_FullscreenMode);
    }

    if (IsButtonPressed(GamepadButtons::Menu) || m_FlushRequested)
    {
        g_StreamingTextureManager.FlushLoadedTiles();
        m_FlushRequested = false;
    }
    if (IsButtonPressed(GamepadButtons::Y) || m_CycleCameraLocation)
    {
        CycleCameraLocation();
        m_CycleCameraLocation = false;
    }
    if (IsButtonPressed(GamepadButtons::X))
    {
        if (m_StochasticFraction == g_DefaultStochasticFraction)
        {
            m_StochasticFraction = 1.0f;
        }
        else
        {
            m_StochasticFraction = g_DefaultStochasticFraction;
        }
    }
    if (!MenuButtonPressed && IsButtonPressed(GamepadButtons::B))
    {
        m_AutoCameraMovement = (m_AutoCameraMovement > 0) ? 0 : 3;
    }

    ID3D12CommandQueue* pUpdateQueue = m_useDMAForStreaming ? m_pDmaCmdQueue : m_pd3dCmdQueue;
    ID3D12CommandQueue* pRenderQueue = m_useDMAForStreaming ? m_pd3dCmdQueue : nullptr;
    g_StreamingTextureManager.UpdateStreaming(pUpdateQueue, pRenderQueue);

    g_SceneGraph.SetFilterSlopeEnums(m_FilterSlopeUIndex, m_FilterSlopeVIndex, m_FilterOffsetUIndex, m_FilterOffsetVIndex);
    m_Sprite.SetAlternateUVector(XMFLOAT4(g_FilterSlopeValues[m_FilterSlopeUIndex], g_FilterSlopeValues[m_FilterSlopeVIndex], 0, 0));

    return S_OK;
}

HRESULT DevTestApp::Render(FRAME_PIPELINE_TOKEN FrameToken)
{
    HRESULT hr = S_OK;
    WCHAR strText[512];
    const FLOAT ColorWhite[4] = { 1, 1, 1, 1 };
    const FLOAT ColorYellow[4] = { 1, 1, 0.75f, 1 };
    const FLOAT ColorBlue[4] = { 0, 0, 0.5f, 1 };
    const FLOAT ColorBlack[4] = { 0, 0, 0, 1 };
    const FLOAT ColorGray[4] = { 0.5f, 0.5f, 0.5f, 1 };

    const SceneRenderMode& SRM = g_SceneModes[m_SceneRenderMode];

    const XMMATRIX matProjection = XMLoadFloat4x4(&m_matProjection);
    const XMMATRIX matView = XMLoadFloat4x4(&m_matView);

    ID3D12GraphicsCommandList* pCmdList = CreateCommandList(true);

    m_Timestamps.BeginFrame(pCmdList);

    if (!SRM.FreezeStreaming)
    {
        g_StreamingTextureManager.Render(pCmdList);
    }

    m_Timestamps.MarkTimestamp();

    ID3D12Resource* pBackBufferTexture = GetCurrentBackBufferTexture();
    D3D12_CPU_DESCRIPTOR_HANDLE BackBufferRTV = GetCurrentBackBufferRenderTargetView();

    PrepareBackBufferForRendering(pCmdList, pBackBufferTexture);

    pCmdList->ClearRenderTargetView(BackBufferRTV, ColorBlue, 0, nullptr);
    pCmdList->ClearDepthStencilView(m_DepthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    m_Timestamps.MarkTimestamp();

    pCmdList->OMSetRenderTargets(1, &BackBufferRTV, TRUE, &m_DepthStencilView);
    pCmdList->RSSetViewports(1, &m_Viewport);
    pCmdList->RSSetScissorRects(1, &m_ScissorRect);

    pCmdList->SetGraphicsRootSignature(m_pDefaultRootSignature);

    pCmdList->SetPipelineState(SRM.pPSO);

    LabeledCpuProbe SceneRenderProbeArray[ARRAYSIZE(m_PrevFrameSceneRenderProbes)] = {};
    CpuProbeWriter SceneRenderProbes(SceneRenderProbeArray, ARRAYSIZE(SceneRenderProbeArray));

    g_SceneGraph.Render(matView, matProjection, pCmdList, m_pd3dCmdQueue);

    SceneRenderProbes.WriteProbe("Scenegraph Traversal & Submit", g_SceneGraph.GetInstanceCount());
    m_Timestamps.MarkTimestamp();

    UINT32 IntervalCount = 0;
    DOUBLE* pIntervalTimeUsec = nullptr;
    DOUBLE FrameGpuTimeUsec = m_Timestamps.GetPreviousFrame(&IntervalCount, &pIntervalTimeUsec);

    m_Sprite.BeginScene(pCmdList, m_pd3dCmdQueue, m_InitParams.BackBufferFormat, BackBufferRTV, m_Viewport);

    INT Ypos = 0;
    if (m_Timer.GetAverageDeltaTime() > 0)
    {
    #if XBOX_SAMPLER_FEEDBACK
        const WCHAR* strTitle = L"Streaming (SF Hardware)";
    #else
        const WCHAR* strTitle = L"Streaming (SF Emulation)";
    #endif

    #if DBG || _DEBUG
        const WCHAR* strDebug = L" [DEBUG]";
    #else
        const WCHAR* strDebug = L"";
    #endif

        swprintf_s(strText, L"%s%s %0.2lf fps %0.2lf ms frame %u", strTitle, strDebug, 1.0 / m_Timer.GetAverageDeltaTime(), 1000.0 * m_Timer.GetAverageDeltaTime(), m_FrameIndex);
        m_Sprite.DrawText( 0, Ypos, 20, strText, ColorWhite );
        Ypos += 20;

        if (m_ShowGpuStats)
        {
        #if SUPPORT_TIER_1
            const WCHAR* strTierMode = L" (Tier 1 Mode)";
        #else
            const WCHAR* strTierMode = L"";
        #endif
            swprintf_s(strText, L"%s %I64u MB VRAM Tier %u%s %u x %u", 
                m_SelectedAdapterDesc.Description, 
                m_SelectedAdapterDesc.DedicatedVideoMemory >> 20, 
                m_FeatureOptions.TiledResourcesTier, 
                strTierMode, 
                m_ScissorRect.right, 
                m_ScissorRect.bottom);

            m_Sprite.DrawText(0, Ypos, 20, strText, ColorWhite);
            Ypos += 20;

            if (IntervalCount > 0)
            {
                assert(IntervalCount >= 4);
                swprintf_s(strText, L"GPU time: %0.3f us (stream %0.3f clear %0.3f scene %0.3f UI %0.3f)", FrameGpuTimeUsec, pIntervalTimeUsec[0], pIntervalTimeUsec[1], pIntervalTimeUsec[2], pIntervalTimeUsec[3]);
                m_Sprite.DrawText(0, Ypos, 20, strText, ColorWhite);
                Ypos += 20;
            }
        }
    }

    if (m_StreamingTextureCount == 0)
    {
        Ypos += 20 * 8;
    #if defined(_XBOX_ONE)
        m_Sprite.DrawText(0, Ypos, 20, L"Could not locate texture content. Place the trimg and trdata files in the following layout relative path:", ColorWhite);
    #else
        m_Sprite.DrawText(0, Ypos, 20, L"Could not locate texture content. Place the trimg and trdata files in the following absolute path:", ColorWhite);
    #endif
        Ypos += 20;
        m_Sprite.DrawText(0, Ypos, 20, GetTextureBasePath(), ColorWhite);
        Ypos += 20;
    #if !defined(_XBOX_ONE)
        m_Sprite.DrawText(0, Ypos, 20, L"A Windows Explorer window has been opened to the parent directory path.", ColorWhite);
        Ypos += 20;
    #endif
    }

    StreamingTextureStatistics Stats = {};

    if (m_ShowStreamingStats)
    {
        Stats = g_StreamingTextureManager.GetStatistics();
        FLOAT CommittedPercentage = 0.0f;
        if (Stats.VirtualTotalTileCount > 0)
        {
            CommittedPercentage = 100.0f * ((FLOAT)Stats.VirtualMappedTileCount / (FLOAT)Stats.VirtualTotalTileCount);
        }
        swprintf_s(strText, L"Memory reserved: %I64u MB (%u textures) committed: %I64u KB (%0.1f%%) (tile pool %u/%u)", (UINT64)Stats.VirtualTotalTileCount >> 4, Stats.TextureCount, (UINT64)Stats.VirtualMappedTileCount << 6, CommittedPercentage, Stats.TilePoolAllocatedTiles, Stats.TilePoolSizeTiles);
        m_Sprite.DrawText(0, Ypos, 20, strText, ColorYellow);
        Ypos += 20;

        swprintf_s(strText, L"Tiles all: %4u seen: %2u load: %2u comp: %2u dcomp: %2u infl: %2u ageoutM: %2u ageoutS: %2u rmap: %2u", Stats.AllTileCount, Stats.SeenTileCount, Stats.LoadingTileCount, Stats.CompressedLoadTileCount, Stats.DecompressingTileCount, Stats.InFlightTileCount, Stats.AgeOutMappedCount, Stats.AgeOutSeenCount, Stats.MinLODMapEditCount);
        m_Sprite.DrawText(0, Ypos, 18, strText, ColorYellow);
        Ypos += 20;

        swprintf_s(strText, L"KB/sec: %7u (peak %7u) IOPS: %5u (peak %5u)", Stats.KBytesLoadedPerSecond, Stats.PeakKBytesLoadedPerSecond, Stats.TileLoadsPerSecond, Stats.PeakTileLoadsPerSecond);
        m_Sprite.DrawText(0, Ypos, 20, strText, ColorYellow);
        Ypos += 20;

        const DOUBLE UsecPerTick = 1000000.0 / (DOUBLE)m_Timer.GetPerfFreq();

        FLOAT MinLatency = (FLOAT)((DOUBLE)Stats.StorageReadLatency.MinValue * UsecPerTick);
        FLOAT MaxLatency = (FLOAT)((DOUBLE)Stats.StorageReadLatency.MaxValue * UsecPerTick);
        FLOAT AvgLatency = (FLOAT)((DOUBLE)Stats.StorageReadLatency.AverageValue * UsecPerTick);
        swprintf_s(strText, L"Storage read latency         : Avg %0.3f us Min %0.3f us Max %0.3f us", AvgLatency, MinLatency, MaxLatency);
        m_Sprite.DrawText(0, Ypos, 20, strText, ColorYellow);
        Ypos += 20;

        MinLatency = (FLOAT)((DOUBLE)Stats.TileSeenToLoadLatency.MinValue * UsecPerTick);
        MaxLatency = (FLOAT)((DOUBLE)Stats.TileSeenToLoadLatency.MaxValue * UsecPerTick);
        AvgLatency = (FLOAT)((DOUBLE)Stats.TileSeenToLoadLatency.AverageValue * UsecPerTick);
        swprintf_s(strText, L"Tile seen to load latency    : Avg %0.3f us Min %0.3f us Max %0.3f us", AvgLatency, MinLatency, MaxLatency);
        m_Sprite.DrawText(0, Ypos, 20, strText, ColorYellow);
        Ypos += 20;

        MinLatency = (FLOAT)((DOUBLE)Stats.TileLoadToCompleteLatency.MinValue * UsecPerTick);
        MaxLatency = (FLOAT)((DOUBLE)Stats.TileLoadToCompleteLatency.MaxValue * UsecPerTick);
        AvgLatency = (FLOAT)((DOUBLE)Stats.TileLoadToCompleteLatency.AverageValue * UsecPerTick);
        swprintf_s(strText, L"Tile load to complete latency: Avg %0.3f us Min %0.3f us Max %0.3f us", AvgLatency, MinLatency, MaxLatency);
        m_Sprite.DrawText(0, Ypos, 20, strText, ColorYellow);
        Ypos += 20;

        WCHAR strBottlenecks[128] = L"";
        if (Stats.Bottlenecks.DwordValue == 0)
        {
            wcscpy_s(strBottlenecks, L"None");
        }
        else
        {
            const WCHAR* strComma = L"";
            if (Stats.Bottlenecks.AsyncLoadSlots)
            {
                wcscat_s(strBottlenecks, strComma);
                wcscat_s(strBottlenecks, L"Async Load Slots");
                strComma = L", ";
            }
            if (Stats.Bottlenecks.TilePoolTiles)
            {
                wcscat_s(strBottlenecks, strComma);
                wcscat_s(strBottlenecks, L"Tile Pool Tiles");
                strComma = L", ";
            }
            if (Stats.Bottlenecks.CompressedBuffers)
            {
                wcscat_s(strBottlenecks, strComma);
                wcscat_s(strBottlenecks, L"Compressed Tile Buffers");
                strComma = L", ";
            }
            if (Stats.Bottlenecks.DecompressedBuffers)
            {
                wcscat_s(strBottlenecks, strComma);
                wcscat_s(strBottlenecks, L"Decompressed Tile Buffers");
                strComma = L", ";
            }
        }
        swprintf_s(strText, L"Bottlenecks: %s", strBottlenecks);
        m_Sprite.DrawText(0, Ypos, 20, strText, ColorYellow);
        Ypos += 20;

        swprintf_s(strText, L"Mip Residency:");
        const UINT32 NumColumns = 3;
        UINT32 LineCount = 1;
        const UINT32 LastMipArrayIndex = ARRAYSIZE(Stats.VirtualTotalTileCountPerMip) - 1;
        for (UINT32 i = 0; i <= LastMipArrayIndex; ++i)
        {
            WCHAR strMipText[32];

            const UINT32 MappedCount = Stats.VirtualMappedTileCountPerMip[i];
            const UINT32 TotalCount = Stats.VirtualTotalTileCountPerMip[i];
            FLOAT MappedPercent = 0.0f;
            if (TotalCount > 0)
            {
                MappedPercent = 100.0f * ((FLOAT)MappedCount / (FLOAT)TotalCount);
            }

            const WCHAR* strPlus = (i == LastMipArrayIndex) ? L"+" : L" ";

            swprintf_s(strMipText, L" %u%s: (%5u/%6u %5.1f%%)",
                i,
                strPlus,
                MappedCount,
                TotalCount,
                MappedPercent);

            wcscat_s(strText, strMipText);
            if ((i % NumColumns) == (NumColumns - 1))
            {
                wcscat_s(strText, L"\n              ");
                ++LineCount;
            }
        }
        m_Sprite.DrawText(0, Ypos, 20, strText, ColorYellow);
        Ypos += 20 * LineCount;

        swprintf_s(strText, L"Mip Accessed :");
        LineCount = 1;
        for (UINT32 i = 0; i <= LastMipArrayIndex; ++i)
        {
            WCHAR strMipText[32];

            const UINT32 AccessedCount = Stats.VirtualAccessedTileCountPerMip[i];
            const UINT32 TotalCount = Stats.VirtualTotalTileCountPerMip[i];
            FLOAT AccessedPercent = 0.0f;
            if (TotalCount > 0)
            {
                AccessedPercent = 100.0f * ((FLOAT)AccessedCount / (FLOAT)TotalCount);
            }

            const WCHAR* strPlus = (i == LastMipArrayIndex) ? L"+" : L" ";

            swprintf_s(strMipText, L" %u%s: (%5u/%6u %5.1f%%)",
                i,
                strPlus,
                AccessedCount,
                TotalCount,
                AccessedPercent);

            wcscat_s(strText, strMipText);
            if ((i % NumColumns) == (NumColumns - 1))
            {
                wcscat_s(strText, L"\n              ");
                ++LineCount;
            }
        }
        m_Sprite.DrawText(0, Ypos, 20, strText, ColorYellow);
        Ypos += 20 * LineCount;
    }

    if (m_ShowCpuTimingStats)
    {
        if (!m_ShowStreamingStats)
        {
            Stats = g_StreamingTextureManager.GetStatistics();
        }

        RenderCpuProbeList(Stats.UpdateProbes, ARRAYSIZE(Stats.UpdateProbes), Ypos, L"Streaming Update");
        RenderCpuProbeList(Stats.RenderProbes, ARRAYSIZE(Stats.RenderProbes), Ypos, L"Streaming Render");

        // Copy prev frame scene render probes to this frame's buffer, as we have not yet completed capturing this frame's probes:
        const UINT32 CurrentProbeIndex = ARRAYSIZE(SceneRenderProbeArray) - SceneRenderProbes.GetProbesRemaining();
        assert(CurrentProbeIndex > 0);
        for (UINT32 i = CurrentProbeIndex; i < ARRAYSIZE(SceneRenderProbeArray); ++i)
        {
            SceneRenderProbeArray[i].strLabel = m_PrevFrameSceneRenderProbes[i].strLabel;
            SceneRenderProbeArray[i].TickCount = (m_PrevFrameSceneRenderProbes[i].TickCount - m_PrevFrameSceneRenderProbes[i - 1].TickCount) + SceneRenderProbeArray[i - 1].TickCount;
        }
        RenderCpuProbeList(SceneRenderProbeArray, ARRAYSIZE(SceneRenderProbeArray), Ypos, L"Scene Render");
    }

    if (m_DebugTextureComponent < STCT_Count)
    {
        const INT HalfViewportWidth = ((INT)m_Viewport.Width - (m_SpriteOffset * 2)) / 2;
        const INT HalfViewportHeight = ((INT)m_Viewport.Height - (m_SpriteOffset * 2)) / 2;
        const INT CrosshairHalfWidth = 8;
        const INT CrosshairHalfThickness = 1;

        // crosshair
        m_Sprite.DrawRect(HalfViewportWidth - CrosshairHalfThickness, HalfViewportHeight - CrosshairHalfWidth, CrosshairHalfThickness * 2, CrosshairHalfWidth * 2, ColorWhite);
        m_Sprite.DrawRect(HalfViewportWidth - CrosshairHalfWidth, HalfViewportHeight - CrosshairHalfThickness, CrosshairHalfWidth * 2, CrosshairHalfThickness * 2, ColorWhite);
    }

    StreamingTexture* pDebugTexture = nullptr;
    FeedbackTexture* pDebugFeedbackTexture = nullptr;
    D3D12_GPU_DESCRIPTOR_HANDLE hDebugGPU = {};
    ID3D12DescriptorHeap* pDebugDescriptorHeap = g_SceneGraph.GetDescriptorHeap();

    if (m_pSelectedSceneObjectInstance != nullptr)
    {
        swprintf_s(strText, L"Scene object: %s", m_pSelectedSceneObjectInstance->strName);
        m_Sprite.DrawText(0, Ypos, 20, strText, ColorGray);
        Ypos += 20;

        pDebugTexture = m_pSelectedSceneObjectInstance->pSurfaceTextures[m_DebugTextureLayer];
        pDebugFeedbackTexture = m_pSelectedSceneObjectInstance->pFeedbackTextures[m_DebugTextureLayer];
        hDebugGPU = g_SceneGraph.GetInstanceGpuDescriptor(m_pSelectedSceneObjectInstance, m_DebugTextureLayer, m_DebugTextureComponent);

        // The debug shader for the MinLOD map requires t0 to be the primary texture and t1 to be the MinLOD texture, provided as
        // a single descriptor range.
        // The GPU visible descriptors for these textures are not contiguous in the scene graph descriptor heap, so here we just-in-time copy
        // them to the sample's descriptor heap in a contiguous destination range.
        if (m_DebugTextureComponent == STCT_MinLODTexture)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE hPrimary = g_SceneGraph.GetInstanceCpuDescriptor(m_pSelectedSceneObjectInstance, m_DebugTextureLayer, STCT_PrimaryTexture);
            D3D12_CPU_DESCRIPTOR_HANDLE hMinLOD = g_SceneGraph.GetInstanceCpuDescriptor(m_pSelectedSceneObjectInstance, m_DebugTextureLayer, STCT_MinLODTexture);
            m_pd3dDevice->CopyDescriptorsSimple(1, m_SRVHeap.hCPU(2), hPrimary, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            m_pd3dDevice->CopyDescriptorsSimple(1, m_SRVHeap.hCPU(3), hMinLOD, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            hDebugGPU = m_SRVHeap.hGPU(2);
            pDebugDescriptorHeap = m_SRVHeap;
        }
    #if XBOX_SAMPLER_FEEDBACK
        else if (m_DebugTextureComponent == STCT_FeedbackTexture)
        {
            // The feedback UAV does not have the dimensions of the feedback map as it is a HW-specific encoding for SF; compose a new descriptor now:
            D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
            UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
            UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            UAVDesc.Texture2DArray.ArraySize = pDebugTexture->FileHeader.SliceCount;
            UAVDesc.Texture2DArray.FirstArraySlice = pDebugFeedbackTexture->FirstSliceIndexWithinResource;

            D3D12_CPU_DESCRIPTOR_HANDLE hTempUAV = m_SRVHeap.hCPU(4);
            m_pd3dDevice->CreateUnorderedAccessView(pDebugFeedbackTexture->pTextureResource, nullptr, &UAVDesc, hTempUAV);
            hDebugGPU = m_SRVHeap.hGPU(4);
            pDebugDescriptorHeap = m_SRVHeap;
        }
    #endif
    }

    {
        UINT32 DebugBaseWidth = 0;
        UINT32 DebugBaseHeight = 0;
        UINT32 MipLevelCount = 1;
        UINT32 SliceCount = 1;
        ID3D12Resource* pUAVResource = nullptr;
        D3D12_SRV_DIMENSION SRVDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        bool PointSample = true;
        const D3D12_SHADER_BYTECODE* pAltShader = nullptr;
        FLOAT ModulateColor[4] = { 1, 1, 1, 1 };

        if (pDebugTexture != nullptr)
        {
            const TiledResourceImage::Header& FH = pDebugTexture->FileHeader;
            MipLevelCount = FH.MipLevelCount;
            SliceCount = FH.SliceCount;

            const bool IsPrimaryTexture = (m_DebugTextureComponent == STCT_PrimaryTexture);

            const WCHAR* strTextureDisplayName = pDebugTexture->strFileName;
            const WCHAR* pLastSlash = wcsrchr(strTextureDisplayName, L'\\');
            if (pLastSlash != nullptr)
            {
                strTextureDisplayName = pLastSlash + 1;
            }

            if (IsPrimaryTexture)
            {
                UINT32 TileWidthTexels = 0;
                UINT32 TileHeightTexels = 0;
                TiledResourceImage::GetTileShape(&FH, nullptr, nullptr, &TileWidthTexels, &TileHeightTexels);

                DebugBaseWidth = (FH.MipLevels[0].WidthTilesM1 + 1) * TileWidthTexels;
                DebugBaseHeight = (FH.MipLevels[0].HeightTilesM1 + 1) * TileHeightTexels;

                swprintf_s(strText, L"Texture %u: %s (%u x %u %um %us %s)",
                    pDebugTexture->TextureUniqueIndex,
                    strTextureDisplayName,
                    DebugBaseWidth,
                    DebugBaseHeight,
                    FH.MipLevelCount,
                    FH.SliceCount,
                    g_strFormatNames[FH.DXGIFormat]);
            }
            else
            {
                MipLevelCount = 1;
                ModulateColor[0] = 256.0f / (8.0f * 5);

                ID3D12Resource* pResource = nullptr;

                const WCHAR* strLabel = nullptr;
                if (m_DebugTextureComponent == STCT_MinLODTexture)
                {
                    pResource = pDebugTexture->MinLODTexture.pTextureResource;
                    strLabel = L"MinLOD Map";
                    pAltShader = &m_MinLODViewShader;
                }
                else
                {
                    pResource = pDebugFeedbackTexture->pTextureResource;
                    strLabel = L"Feedback Map";
                    pAltShader = &m_FeedbackViewShader;
                }

                if (pResource != nullptr)
                {
                    D3D12_RESOURCE_DESC TexDesc = pResource->GetDesc();

                    swprintf_s(strText, L"Texture %u: %s %s (%I64u x %u %us %s)",
                        pDebugTexture->TextureUniqueIndex,
                        strTextureDisplayName,
                        strLabel,
                        TexDesc.Width,
                        TexDesc.Height,
                        TexDesc.DepthOrArraySize,
                        g_strFormatNames[TexDesc.Format]);

                    DebugBaseWidth = TexDesc.Width;
                    DebugBaseHeight = TexDesc.Height;

                    if (m_DebugTextureComponent == STCT_FeedbackTexture)
                    {
                        pUAVResource = pResource;
                        ResourceBarrier(pCmdList, pUAVResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                    }
                }
                else if (m_DebugTextureComponent == STCT_FeedbackTexture)
                {
                    // Feedback texture is a buffer UAV (subset of a larger buffer), so no resource pointer

                    const D3D12_SUBRESOURCE_FOOTPRINT& FP = pDebugFeedbackTexture->Staging.CopyLocation.PlacedFootprint.Footprint;
                    DebugBaseWidth = FP.Width;
                    DebugBaseHeight = FP.Height;

                    pAltShader = &m_FeedbackBufferViewShader;
                    m_Sprite.SetAlternateUVector(XMFLOAT4(g_FilterSlopeValues[m_FilterSlopeUIndex], g_FilterSlopeValues[m_FilterSlopeVIndex], (FLOAT)FP.Width, (FLOAT)FP.Height));
                    SRVDimension = D3D12_SRV_DIMENSION_BUFFER;

                    swprintf_s(strText, L"Texture %u: %s %s (%u x %u %us %s)",
                        pDebugTexture->TextureUniqueIndex,
                        strTextureDisplayName,
                        strLabel,
                        FP.Width,
                        FP.Height,
                        FP.Depth,
                        g_strFormatNames[FP.Format]);
                }
            }
        }

        if (m_UseDebugMinLODMap)
        {
            pAltShader = &m_MinLODViewShader;
            hDebugGPU = m_hDebugMinLODSRV;
            DebugBaseWidth = m_DebugMinLODMapDesc.Width;
            DebugBaseHeight = m_DebugMinLODMapDesc.Height;
            swprintf_s(strText, L"Debug MinLOD Map (%u x %u 1s R8_UNORM)", (UINT32)m_DebugMinLODMapDesc.Width, m_DebugMinLODMapDesc.Height);
        }

        if (hDebugGPU.ptr != 0)
        {
            const UINT32 DebugMaxDimension = 128U << m_DebugTextureSizeShift;
            if (DebugBaseWidth > DebugBaseHeight)
            {
                DebugBaseHeight = (DebugBaseHeight * DebugMaxDimension) / DebugBaseWidth;
                DebugBaseWidth = DebugMaxDimension;
            }
            else
            {
                DebugBaseWidth = (DebugBaseWidth * DebugMaxDimension) / DebugBaseHeight;
                DebugBaseHeight = DebugMaxDimension;
            }

            const UINT32 SpacingTexels = 2;
            const UINT32 DebugWidthTexels = DebugBaseWidth + SpacingTexels * 2;

            m_Sprite.DrawText(0, Ypos, 20, strText, ColorWhite);
            Ypos += 20;

            for (UINT32 SliceIndex = 0; SliceIndex < SliceCount; ++SliceIndex)
            {
                INT CursorX = DebugWidthTexels * SliceIndex - (INT)m_DebugOffsetX;
                INT CursorY = Ypos - (INT)m_DebugOffsetY;
                UINT32 MipWidth = DebugBaseWidth;
                UINT32 MipHeight = DebugBaseHeight;
                for (UINT32 MipIndex = 0; MipIndex < MipLevelCount; ++MipIndex)
                {
                    const FLOAT UVWhole[4] = { 0, 0, 1, 1 };
                    m_Sprite.SetDescriptorHeap(pDebugDescriptorHeap);
                    m_Sprite.DrawTexturedQuad(CursorX, CursorY, MipWidth, MipHeight, hDebugGPU, SRVDimension, ModulateColor, false, 0, UVWhole, PointSample, (FLOAT)MipIndex, pAltShader);
                    switch (MipIndex % 4)
                    {
                    case 0:
                        CursorX += (MipWidth / 2);
                        CursorY += MipHeight + SpacingTexels;
                        break;
                    case 1:
                        CursorX -= (MipWidth / 2) + SpacingTexels;
                        CursorY += (MipHeight / 2);
                        break;
                    case 2:
                        CursorY -= (MipHeight / 2) + SpacingTexels;
                        break;
                    case 3:
                        CursorX += MipWidth + SpacingTexels;
                        break;
                    }
                    MipWidth = std::max(MipWidth >> 1, 1U);
                    MipHeight = std::max(MipHeight >> 1, 1U);
                }
            }

            if (pUAVResource != nullptr)
            {
                ResourceBarrier(pCmdList, pUAVResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            }
        }
    }


    m_Menu.Render(&m_Sprite, 1150, 0, 360, 20);

    m_Sprite.EndScene();

    SceneRenderProbes.WriteProbe("UI Text Submit");

    PrepareBackBufferForPresent(pCmdList, pBackBufferTexture);

    m_Timestamps.EndFrame(nullptr, nullptr);

    pCmdList->Close();
    m_pd3dCmdQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&pCmdList);

    SceneRenderProbes.WriteProbe("ExecuteCommandLists");

#if defined(_XBOX_ONE) && defined(_TITLE)
    D3D12XBOX_PRESENT_PLANE_PARAMETERS planeParameters = { 0 };
    planeParameters.Token = FrameToken;
    planeParameters.ResourceCount = 1;
    planeParameters.ppResources = &pBackBufferTexture;

    D3D12XBOX_PRESENT_PARAMETERS presentParameters = { 0 };
    presentParameters.Flags = m_PresentImmediate ? D3D12XBOX_PRESENT_FLAG_IMMEDIATE : D3D12XBOX_PRESENT_FLAG_NONE;

    hr = m_pd3dCmdQueue->PresentX(1, &planeParameters, &presentParameters);
#else
    m_pSwapChain->Present(m_PresentImmediate ? 0 : 1, m_PresentImmediate ? DXGI_PRESENT_ALLOW_TEARING : 0);
#endif
    SAFE_RELEASE(pCmdList);

    SceneRenderProbes.WriteProbe("Present");

    memcpy(m_PrevFrameSceneRenderProbes, SceneRenderProbeArray, sizeof(m_PrevFrameSceneRenderProbes));

    return hr;
}

void DevTestApp::RenderCpuProbeList(const LabeledCpuProbe* pProbes, UINT32 ProbeCount, INT& Ypos, const WCHAR* strTitle)
{
    WCHAR strText[512];
    const FLOAT ColorWhite[4] = { 1, 1, 1, 1 };
    const FLOAT ColorGreen[4] = { 0.75f, 1, 0.75f, 1 };

    const DOUBLE UsecPerTick = 1000000.0 / (DOUBLE)m_Timer.GetPerfFreq();

    swprintf_s(strText, L"--- [%s] ---", strTitle);
    m_Sprite.DrawText(0, Ypos, 20, strText, ColorGreen);
    Ypos += 20;
    UINT32 LastProbeIndex = ProbeCount - 1;

    for (UINT32 i = 1; i < ProbeCount; ++i)
    {
        const LabeledCpuProbe& CurrentProbe = pProbes[i];
        const LabeledCpuProbe& PrevProbe = pProbes[i - 1];

        if (CurrentProbe.strLabel == nullptr)
        {
            LastProbeIndex = i - 1;
            break;
        }

        UINT64 Delta = CurrentProbe.TickCount - PrevProbe.TickCount;
        const DOUBLE TimeUsec = (DOUBLE)Delta * UsecPerTick;
        if (CurrentProbe.Divisor > 0)
        {
            const DOUBLE ItemUsec = TimeUsec / (DOUBLE)CurrentProbe.Divisor;
            swprintf_s(strText, L"  %8.3f usec: %S (%0.3f usec/item)", TimeUsec, CurrentProbe.strLabel, ItemUsec);
        }
        else
        {
            swprintf_s(strText, L"  %8.3f usec: %S", TimeUsec, CurrentProbe.strLabel);
        }
        m_Sprite.DrawText(0, Ypos, 20, strText, ColorWhite);
        Ypos += 20;
    }

    const UINT64 SectionDelta = pProbes[LastProbeIndex].TickCount - pProbes[0].TickCount;
    const DOUBLE TimeUsec = (DOUBLE)SectionDelta * UsecPerTick;
    swprintf_s(strText, L"  %8.3f usec: Cumulative for %s", TimeUsec, strTitle);
    m_Sprite.DrawText(0, Ypos, 20, strText, ColorGreen);
    Ypos += 20;
}

MODERN_MAIN(DevTestApp);
