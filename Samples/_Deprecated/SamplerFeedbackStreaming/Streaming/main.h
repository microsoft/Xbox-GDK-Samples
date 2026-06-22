//--------------------------------------------------------------------------------------
// main.h
//
// Main class for the Streaming devtest.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "d3d12app.hpp"
#include <DirectXMath.h>
#include "JobQueue.h"
#include <random>
#include "menu.hpp"

#include "TextureStreaming.h"
#include "SceneGraph.h"

#include "TextureFiles.h"
#include "ToolDefines.h"

struct SceneRenderMode
{
    ID3D12PipelineState* pPSO;
    const WCHAR* strName;
    bool FreezeStreaming;
};
typedef std::vector<SceneRenderMode> SceneRenderModeVector;
extern SceneRenderModeVector g_SceneModes;

class TimestampSeries
{
private:
    ID3D12QueryHeap * m_pQueryHeap;
    ID3D12Resource* m_pQueryBuffer;
    UINT32 m_MaxTimestampCount;
    UINT64* m_pQueryData;
    DOUBLE* m_pTimeUsec;
    DOUBLE m_InverseTickFrequency;
    DOUBLE m_SmoothingSpeed;

    ID3D12GraphicsCommandList* m_pCurrentCmdList;
    UINT32 m_TimestampIndex;
    UINT32 m_PreviousTimestampCount;

public:
    HRESULT Initialize(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pCmdQueue, UINT32 MaxIntervalCount = 31);
    void Terminate();

    void BeginFrame(ID3D12GraphicsCommandList* pCmdList);
    void MarkTimestamp();
    DOUBLE EndFrame(UINT32* pIntervalCount, DOUBLE** ppIntervalTimesUsec);

    DOUBLE GetPreviousFrame(UINT32* pIntervalCount, DOUBLE** ppIntervalTimesUsec);
};

class DevTestApp :
    public D3D12App
{
private:
    XMFLOAT4X4 m_matProjection;

    FLOAT m_CameraYaw;
    FLOAT m_CameraPitch;
    XMFLOAT3 m_CameraPos;
    XMFLOAT4X4 m_matView;
    static const UINT32 m_SpriteOffset = 10;

    RandomNumberGenerator m_RNG;

    UINT32 m_SceneRenderMode;

    ID3D12PipelineState* m_pScene1PSO;
    ID3D12PipelineState* m_pScene1FilterEmuPSO;
    ID3D12PipelineState* m_pScene2PSO;
    ID3D12PipelineState* m_pScene2FilterEmuPSO;
    ID3D12PipelineState* m_pScene3PSO;
    ID3D12PipelineState* m_pScene3FilterEmuPSO;

    ID3D12PipelineState* m_pScene1NoFeedbackPSO;
    ID3D12PipelineState* m_pScene1ShowMinLODPSO;
    ID3D12PipelineState* m_pScene1ShowMinLODWithFilterPSO;
    ID3D12PipelineState* m_pScene1ShowFeedbackPSO;

    ID3D12CommandQueue* m_pDmaCmdQueue;

    UINT32 m_StreamingTextureCount;
    bool m_SceneCreated;

    SceneObjectInstance* m_pSelectedSceneObjectInstance;
    FLOAT m_DebugOffsetX;
    FLOAT m_DebugOffsetY;

    SettingsMenu m_Menu;
    bool m_SlowLoading;
    bool m_SlowDecompressing;
    bool m_PauseLoading;
    bool m_PauseAgeOut;
    StreamingTextureComponentTextures m_DebugTextureComponent;
    SurfaceTextureType m_DebugTextureLayer;
    UINT32 m_DebugTextureSizeShift;
    bool m_RandomLoads;
    bool m_DoNotLoadUpperLeft;
    bool m_ShowStreamingStats;
    bool m_ShowCpuTimingStats;
    bool m_ShowGpuStats;
    bool m_PresentImmediate;
    bool m_useDMAForStreaming;
    bool m_FullscreenMode;
    bool m_FlushRequested;
    bool m_CycleCameraLocation;
    bool m_MarkTileBoundaries;

    UINT32 m_AutoCameraMovement;
    FLOAT m_AutoCameraTime;

    INT m_FilterSlopeUIndex;
    INT m_FilterSlopeVIndex;
    INT m_FilterOffsetUIndex;
    INT m_FilterOffsetVIndex;

    FLOAT m_StochasticFraction;

    D3D12_SHADER_BYTECODE m_MinLODViewShader;
    D3D12_SHADER_BYTECODE m_FeedbackViewShader;
    D3D12_SHADER_BYTECODE m_FeedbackBufferViewShader;

    D3D12_RESOURCE_DESC m_DebugMinLODMapDesc;
    ID3D12Resource* m_pDebugMinLODMap;
    ID3D12Resource* m_pDebugFakePrimaryMap;
    DescriptorHeapWrapper m_SRVHeap;
    D3D12_GPU_DESCRIPTOR_HANDLE m_hDebugMinLODSRV;
    bool m_UseDebugMinLODMap;

    TimestampSeries m_Timestamps;

    LabeledCpuProbe m_PrevFrameSceneRenderProbes[5];

public:
    DevTestApp(void);
    ~DevTestApp(void);

    virtual VOID PreWindowInit( ThinWinInit* pInit );
    virtual HRESULT PreD3DInitialize( D3DInitParameters* pD3DInitParams );
    virtual HRESULT Initialize();
    virtual HRESULT Terminate();
    virtual HRESULT Update( BOOL Resized );
    virtual HRESULT Render( FRAME_PIPELINE_TOKEN FrameToken );

    virtual LRESULT MouseMsgProc( UINT message, WPARAM wParam, LPARAM lParam );
    virtual LRESULT KeyboardMsgProc( UINT message, WPARAM wParam, LPARAM lParam );

private:
    void CycleCameraLocation();
    void CreateSceneObjects();
    static void AddSceneRenderMode(const WCHAR* strName, ID3D12PipelineState* pPSO, bool FreezeStreaming)
    {
        SceneRenderMode SRM = {};
        SRM.pPSO = pPSO;
        SRM.strName = strName;
        SRM.FreezeStreaming = FreezeStreaming;
        g_SceneModes.push_back(SRM);
    }

    void RenderCpuProbeList(const LabeledCpuProbe* pProbes, UINT32 ProbeCount, INT& Ypos, const WCHAR* strTitle);
};

