//--------------------------------------------------------------------------------------
// Fluid.h
//
// Demonstrates basic Navier-Stokes flow simulation using compute shader 6.0 and 3D textures.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define CPP_SHARED
#include "FluidShared.h"
#include "Arcball.h"

#pragma warning(push)
#pragma warning(disable : 4324)  // turn off alignment warning, we are treating warnings as errors

//--------------------------------------------------------------------------------------
// The class holding necessary resources for basic Navier-Stokes flow simulation
//--------------------------------------------------------------------------------------
class Fluid
{
public:
    Fluid();

    // Initialzie the simulation's D3D12 resources, views, and pipeline objects.
    void Initialize(ID3D12Device* device, DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat);

    // Reset the simulation
    void Reset() { m_resetSim = true; }

    // Update the simulation control variables from the input device.
    void Update(float elapsedTime, const DirectX::GamePad::State& pad);

    // Advance the simulation one step Forward
    void Step(ID3D12GraphicsCommandList* commandList, float elapsedTime);

    // Render a visualization of the current state of the particle simulation.
    void Render(ID3D12GraphicsCommandList* commandList, DirectX::FXMMATRIX viewProj, DirectX::HXMVECTOR eyePosition);

private:
    // Resets the simulation and starts from the beginning.
    void ResetSim(ID3D12GraphicsCommandList* commandList);

    // Creates and initializes a D3D12 resource and its SRV & UAV.
    void Create3DTextures(ID3D12Device* device);

    // Creates a 3D texture, its shader resource view and unordered access view.
    void CreateResourceAndViews(
        ID3D12Device* device, 
        const D3D12_RESOURCE_DESC* pDesc, 
        D3D12_RESOURCE_STATES initState, 
        int srvHeapIndex, 
        int uavHeapIndex, 
        ID3D12Resource** resource,
        const wchar_t *name);

private:
    // indices of root parameters
    enum ComputeRootParameters
    {
        ComputeRootParamCB0 = 0,
        ComputeRootParamSRV0,
        ComputeRootParamSRV1,
        ComputeRootParamUAV0,
        ComputeRootParamUAV1,
        ComputeRootParamCount
    };

    enum RenderRootParameters
    {
        RenderRootParamCB0 = 0,
        RenderRootParamSRV0,
        RenderRootParamSRV1,
        RenderRootParamCount
    };

    // resource view indices into the descriptor heap
    enum DescriptorHeapIndex
    {
        SRV_Color0 = 0,
        UAV_Color0,
        SRV_Color1,
        UAV_Color1,
        SRV_Velocity0,
        UAV_Velocity0,
        SRV_Velocity1,
        UAV_Velocity1,
        SRV_Pressure,
        UAV_Pressure,
        SRV_TempVector,
        UAV_TempVector,
        SRV_TempScalar,
        UAV_TempScalar,
        SRV_ColorOneEigth,
        UAV_ColorOneEigth,
        HeapCount
    };

private:
    DX::Arcball                                 m_emitterRot;

    float                                       m_emitterCenter;
    bool                                        m_resetSim;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_color[2];          // The trace particles carried around by the fluid velocity field - this is what we see when rendering
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_velocity[2];       // The velocity field
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_pressure;          // The pressure field for projecting the divergent velocity field back to a divergence free field
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_tempVector;        // Temp vector and scalar field used in Jacobi iteration when solving for pressure field
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_tempScalar;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_colorOneEighth;    // 1/8 sized m_color used to accelerate rendering

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_advectColorPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_advectVelocityPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_vorticityPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_confinementPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_gaussian1PSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_gaussian4PSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_divergencePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_jacobiPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_projectPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_renderPSO;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_computeRS;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

    std::unique_ptr<DirectX::DescriptorPile>    m_srvPile;
};

#pragma warning(pop)