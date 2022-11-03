//--------------------------------------------------------------------------------------
// PointSprites.h
//
// Demonstrates how to render point sprites in 10 different ways.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:
    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();
    void OnConstrained() {}
    void OnUnConstrained() {}

    // Properties
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void CreateTestPSOs();

private:
    struct Test;
    using TestList = std::vector<Test>;
    using TestSequence = std::vector<Test*>;

    using TestMemFn = void (Sample::*)(struct Test&, ID3D12GraphicsCommandList*);

private:
    void UpdateVertexData(ID3D12GraphicsCommandList* commandList);

    void RegisterTest(bool enabledByDefault, const wchar_t* name, TestMemFn testFn);
    void RegisterTests();
    void RunTests(const TestSequence& testSequence, ID3D12GraphicsCommandList* commandList);
    void ClearTestResults();
    Test* ComputeAverageTestTimeAndWinner(int& enabledTestCount);
    void ToggleTest(unsigned testIdx);

    void CreateForwardTestSequence(TestSequence& testSequence);
    void CreateReverseTestSequence(TestSequence& testSequence);
    void CreateRandomTestSequence(TestSequence& testSequence);

    // VS Tests
    void TestVSNativeQuads(Test& test, ID3D12GraphicsCommandList* commandList);
    void TestVSNativeInstancedQuads(Test& test, ID3D12GraphicsCommandList* commandList);
    void TestVSTriangleStripInstancedQuads(Test& test, ID3D12GraphicsCommandList* commandList);
    void TestVSRectListQuads(Test& test, ID3D12GraphicsCommandList* commandList);
    void TestVSTriangles(Test& test, ID3D12GraphicsCommandList* commandList);
    void TestVSQuads(Test& test, ID3D12GraphicsCommandList* commandList);
    void TestVSInstancedTriangles(Test& test, ID3D12GraphicsCommandList* commandList);
    void TestVSInstancedQuads(Test& test, ID3D12GraphicsCommandList* commandList);

    // GS Tests
    template<bool OnChip> void TestGSTriangles(Test& test, ID3D12GraphicsCommandList* commandList);
    template<bool OnChip> void TestGSQuads(Test& test, ID3D12GraphicsCommandList* commandList);
    template<bool OnChip> void TestGSOnlyTriangles(Test& test, ID3D12GraphicsCommandList* commandList);
    template<bool OnChip> void TestGSOnlyQuads(Test& test, ID3D12GraphicsCommandList* commandList);

    // DS Tests
    void TestDSTriangles(Test& test, ID3D12GraphicsCommandList* commandList);
    void TestDSQuads(Test& test, ID3D12GraphicsCommandList* commandList);

#ifdef _GAMING_XBOX_SCARLETT
    // MS Tests
    void TestMSTriangles(Test& test, ID3D12GraphicsCommandList* commandList);
#endif

private:
    enum DescriptorHeapIndex
    {
        SRV_Font,
        SRV_CtrlFont,
        SRV_ParticleTex,
        SRV_Count
    };

    enum RootSignatureIndex
    {
        RS_ConstantBuffer,
        RS_VertexBuffer,
        RS_ParticleTex,
        RS_Count
    };

    struct Test
    {
        Test(bool state, unsigned id, const wchar_t* testName, TestMemFn testFn)
            : Enabled(state)
            , Id(id)
            , Name(testName)
            , TotalTimeResult(0.0f)
            , AverageTimeResult(0.0f)
            , RunCount(0)
            , TestMemFn(testFn)
        { }

        bool Enabled; // Possible expansion to enable or disable tests
        unsigned Id;
        const wchar_t* Name;
        float TotalTimeResult;
        float AverageTimeResult;
        int RunCount;
        TestMemFn TestMemFn;
    };

private:
    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;
    int                                         m_displayWidth;
    int                                         m_displayHeight;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;
    std::unique_ptr<DX::GPUTimer>               m_gpuTimer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::CommonStates>      m_commonStates;
    std::unique_ptr<DirectX::DescriptorPile>    m_srvPile;

    std::unique_ptr<DirectX::SpriteBatch>       m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;

    // Test root signature and PSOs
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_commonRS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_nativeQuadPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_nativeQuadInstPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_triStripInstQuadPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_rectListQuadPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_triPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_quadPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_triInstPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_quadInstPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_triGeoOffChipPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_quadGeoOffChipPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_onlyTriGeoOffChipPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_onlyQuadGeoOffChipPSO;

    // On-chip Shaders - only applicable on Xbox One
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_triGeoOnChipPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_quadGeoOnChipPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_onlyTriGeoOnChipPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_onlyQuadGeoOnChipPSO;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_triDomainPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_quadDomainPSO;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_msPSO;

    // Vertex buffer and particle texture
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_particleTex;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_vertices;
    D3D12_VERTEX_BUFFER_VIEW                    m_vbView;

    // Sample variables
    float                                       m_particleSize;
    bool                                        m_zeroViewport;
    bool                                        m_refreshParticles;
    int                                         m_selectorIdx;

    // Graphics test list
    TestList                                    m_testList;
    TestSequence                                m_forwardSequence;
    TestSequence                                m_reverseSequence;
    TestSequence                                m_randomSequence;
};
