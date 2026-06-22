//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

#include "xg_xs.h"

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr uint32_t kDefault1dSizeX = 1U << 14u;

    constexpr uint32_t kDefault2dSizeX = 3840U;
    constexpr uint32_t kDefault2dSizeY = 2160U;

    constexpr uint32_t kDefault3dSizeX = 480U;
    constexpr uint32_t kDefault3dSizeY = 270U;
    constexpr uint32_t kDefault3dSizeZ = 64U;

    /** These constant must match threadgroup sizes defined in the shader */
    constexpr uint32_t kDefault1dThreadgroupX = 64U;
    constexpr uint32_t kDefault2dThreadgroupX = 8U;
    constexpr uint32_t kDefault2dThreadgroupY = 8U;
    constexpr uint32_t kDefault3dThreadgroupX = 4U;
    constexpr uint32_t kDefault3dThreadgroupY = 4U;
    constexpr uint32_t kDefault3dThreadgroupZ = 4U;

    /** These constants define how many image ops threadgroup executes horizontally/vertically.
     *  Must match the actual number of image ops in the shader */
    constexpr uint32_t kNumImageOpsPerThreadgroupPerX = 4U;
    constexpr uint32_t kNumImageOpsPerThreadgroupPerY = 2U;

    constexpr uint32_t kDefaultBlockSizeIn64KPages = 2U * 1024U;

    constexpr uint32_t D3D12_UAV_DIMENSION_TEXTURE2DMS = D3D12_UAV_DIMENSION_TEXTURE2D;
}

/** @brief Defines opcodes used by the shader */
typedef enum OpcodeType
{
    kOpcodeTypeLoad     = 0u,
    kOpcodeTypeSample   = 1u,
    kOpcodeTypeStore    = 2u,
    kOpcodeTypeAtomic   = 3u,
    kOpcodeTypeCount    = 4u,
    kOpcodeTypeMaxUint  = 0xffffffffu
} OpcodeType;

/** @brief  Defines a list of texture dimensions to test */
#define DIM_LIST                \
    X(1D, Tex1d, Tex1d)         \
    X(2D, Tex2d, Tex2dArray)    \
    X(2DMS, Tex2dMsaa, Tex2dArrayMsaa)\
    X(3D, Tex3d, Tex3d)

static const struct {
    uint32_t srvDim;
    uint32_t uavDim;
    char const *shaderName;
    char const *resourceName;
} s_allDimensions [] = {
#define X(dim, shaderName, resourceName) { D3D12_SRV_DIMENSION_TEXTURE##dim, D3D12_UAV_DIMENSION_TEXTURE##dim, #shaderName, #resourceName },
    DIM_LIST
#undef X
};

/** @brief  Defines a list of shader opcodes to test */
#define OPCODE_LIST     \
    X(Load)             \
    X(Sample)           \
    X(Store)            \
    X(Atomic)           \

static const struct {
    OpcodeType  opcode;
    char const *opcodeName;
} s_allOpcodes[] =
{
#define X(opcode) { kOpcodeType##opcode, #opcode },
    OPCODE_LIST
#undef X
};

/** @brief  Defines a list of formats to test */
#define FORMAT_LIST                             \
    X(R8_UNORM          , 8_UNORM           ,  8, R8_UNorm)     \
    X(R16_FLOAT         , 16_FLOAT          , 16, R16_Float)    \
    X(R32_FLOAT         , 32_FLOAT          , 32, R32_Float)    \
    X(R32_UINT          , 32_UINT           , 32, R32_UInt)     \
    X(R16G16B16A16_FLOAT, 16_16_16_16_FLOAT , 64, RGBA16_Float) \
    X(R32G32B32A32_FLOAT, 32_32_32_32_FLOAT ,128, RGBA32_Float) \

static const struct {
    DXGI_FORMAT             dxgiFmt;
    D3D12XBOX_IMAGE_FORMAT  xboxFmt;
    uint32_t                elemBits;
    const char             *userName;
} s_allFormats[] = {
#define X(dxgiFmt, xboxFmt, elemBits, userName) \
    { DXGI_FORMAT_##dxgiFmt, D3D12XBOX_IMAGE_FORMAT_##xboxFmt, elemBits, #userName },
    FORMAT_LIST
#undef X
};

/** @brief  Defines a list of all swizzle modes for reference */
#define SWIZZLE_MODE_LIST \
    X(LINEAR)             \
    X(256B_S)             \
    X(256B_D)             \
    X(256B_R)             \
    X(4KB_Z)              \
    X(4KB_S)              \
    X(4KB_D)              \
    X(4KB_R)              \
    X(64KB_Z)             \
    X(64KB_S)             \
    X(64KB_D)             \
    X(64KB_R)             \
    X(VAR_Z)              \
    X(VAR_S)              \
    X(VAR_D)              \
    X(VAR_R)              \
    X(64KB_Z_T)           \
    X(64KB_S_T)           \
    X(64KB_D_T)           \
    X(64KB_R_T)           \
    X(4KB_Z_X)            \
    X(4KB_S_X)            \
    X(4KB_D_X)            \
    X(4KB_R_X)            \
    X(64KB_Z_X)           \
    X(64KB_S_X)           \
    X(64KB_D_X)           \
    X(64KB_R_X)           \
    X(VAR_Z_X)            \
    X(VAR_S_X)            \
    X(VAR_D_X)            \
    X(VAR_R_X)            \

/** @brief  Defines a list of swizzle modes supported by 3D textures */
#define SWIZZLE_MODE_3DTEX_LIST \
    X(LINEAR)                   \
    X(4KB_S)                    \
    X(64KB_S)                   \
    X(64KB_S_T)                 \
    X(4KB_S_X)                  \
    X(64KB_Z_X)                     /**< The only swizzling mode that could be bound as Depth Buffer and 3D texture for read/write */\
    X(64KB_S_X)                 \
    X(64KB_D_X)                 \
    X(64KB_R_X)                     /**< The only swizzling mode that could be bound as Color Buffer and 3D texture for read/write */\

/** @brief  Defines a list of swizzle modes supported by 2D textures */
#define SWIZZLE_MODE_2DTEX_LIST \
    X(LINEAR)                   \
    X(256B_S)                   \
    X(256B_D)                   \
    X(4KB_S)                    \
    X(4KB_D)                    \
    X(64KB_S)                   \
    X(64KB_D)                   \
    X(64KB_S_T)                 \
    X(64KB_D_T)                 \
    X(4KB_S_X)                  \
    X(4KB_D_X)                  \
    X(64KB_Z_X)                 \
    X(64KB_S_X)                 \
    X(64KB_D_X)                 \
    X(64KB_R_X)                 \

/** @brief  Defines a list of swizzle modes supported by 2D textures with MSAA*/
#define SWIZZLE_MODE_2DTEXMSAA_LIST \
    X(64KB_Z_X)                     \
    X(64KB_R_X)                     \

/** @brief  Defines a list of swizzle modes supported by 1D textures */
#define SWIZZLE_MODE_1DTEX_LIST \
    X(LINEAR)                   \
    X(64KB_Z_X)                 \
    X(64KB_R_X)                 \

typedef struct SwizzleMode
{
    XG_SWIZZLE_MODE mode;
    char const     *name;
} SwizzleMode;

#define X(name) { XG_SWIZZLE_MODE_##name, #name },

static const SwizzleMode s_swizzleMode3d [] =
{
    SWIZZLE_MODE_3DTEX_LIST
};

static const SwizzleMode s_swizzleMode2d [] =
{
    SWIZZLE_MODE_2DTEX_LIST
};

static const SwizzleMode s_swizzleMode2dMsaa [] =
{
    SWIZZLE_MODE_2DTEXMSAA_LIST
};

static const SwizzleMode s_swizzleMode1d [] =
{
    SWIZZLE_MODE_1DTEX_LIST
};

#undef X

static inline D3D12_GPU_VIRTUAL_ADDRESS allocateGpuMemory64KPages(uint32_t numPages)
{
    DWORD allocationFlags = MEM_64K_PAGES | MEM_RESERVE | MEM_COMMIT;
    DWORD protectionFlags = DWORD(PAGE_READWRITE | PAGE_GRAPHICS_READWRITE | PAGE_WRITECOMBINE);
    return (D3D12_GPU_VIRTUAL_ADDRESS)XMemVirtualAlloc(0, numPages << 16, allocationFlags, XMEM_GRAPHICS, protectionFlags);
}

static inline void freeGpuMemory64KPages(D3D12_GPU_VIRTUAL_ADDRESS address)
{
    auto success = VirtualFree(reinterpret_cast<void*>(address), 0, MEM_RELEASE);
    if (!success)
    {
        DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
}

class SwizzleModeBenchmark final : public Benchmark
{
public:
    SwizzleModeBenchmark(uint32_t resDimIdx)
        : Benchmark()
        , m_resDimIdx(resDimIdx)
    {}

    ~SwizzleModeBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        if (m_resDimIdx == 1u)
            return L"SwizzleMode(2D)";
        else if (m_resDimIdx == 2u)
            return L"SwizzleMode(2DMsaa)";
        if (m_resDimIdx == 3u)
            return L"SwizzleMode(3D)";
        else
            return L"SwizzleMode";
    }

    void Initialize(ID3D12Device* device) override
    {
        /** some default memory chunk that should satisfy all the request, it's going to be updated if the size of actual resource doesn't fit */
        SampleTest::m_resourceMemory = allocateGpuMemory64KPages(kDefaultBlockSizeIn64KPages);
        SampleTest::m_resourceMemorySizeIn64KbPages = kDefaultBlockSizeIn64KPages;

        uint32_t numModes = 0u;
        SwizzleMode const *modes = 0;

        switch (m_resDimIdx)
        {
            case 0:
                modes = s_swizzleMode1d;
                numModes = _countof(s_swizzleMode1d);
            break;

            case 1:
                modes = s_swizzleMode2d;
                numModes = _countof(s_swizzleMode2d);
            break;

            case 2:
                modes = s_swizzleMode2dMsaa;
                numModes = _countof(s_swizzleMode2dMsaa);
            break;

            case 3:
                modes = s_swizzleMode3d;
                numModes = _countof(s_swizzleMode3d);
            break;
        }

        for (uint32_t opcodeIdx = 0; opcodeIdx < _countof(s_allOpcodes); ++opcodeIdx)
        {
            for (uint32_t formatIdx = 0u; formatIdx < _countof(s_allFormats); ++formatIdx)
            {
                for (uint32_t swmodeIdx = 0u; swmodeIdx < numModes; ++swmodeIdx)
                {
                    /** if operation is atomic, skip any format except R32_UINT.
                     *  if operation is other than atomic, skip R32_UINT format */
                    if ((s_allFormats[formatIdx].dxgiFmt == DXGI_FORMAT_R32_UINT && s_allOpcodes[opcodeIdx].opcode != kOpcodeTypeAtomic) ||
                        (s_allFormats[formatIdx].dxgiFmt != DXGI_FORMAT_R32_UINT && s_allOpcodes[opcodeIdx].opcode == kOpcodeTypeAtomic))
                    {
                        continue;
                    }
                    /** Don't add atomic and sample opcodes for Msaa textures, only loads and stores */
                    if (m_resDimIdx == 2u && (s_allOpcodes[opcodeIdx].opcode == kOpcodeTypeAtomic || s_allOpcodes[opcodeIdx].opcode == kOpcodeTypeSample))
                    {
                        continue;
                    }
                    AddTest(new SampleTest(opcodeIdx, m_resDimIdx, formatIdx, modes[swmodeIdx].mode, modes[swmodeIdx].name));
                }
            }
        }

        AddCounter(GPUPerfCounters::GRBM_PERF_SEL_TCP_BUSY);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_image_wavefronts);
        AddCounter(GPUPerfCounters::TCP_PERF_SEL_TA_REQ);
        AddCounter(GPUPerfCounters::TCP_PERF_SEL_REQ);
        AddCounter(GPUPerfCounters::TCP_PERF_SEL_TCP_LATENCY);
        AddCounter(GPUPerfCounters::TCP_PERF_SEL_TA_REQ_STATE_READ);
        AddCounter(GPUPerfCounters::TCP_PERF_SEL_GL1_REQ_READ);
        AddCounter(GPUPerfCounters::TCP_PERF_SEL_GL1_REQ_WRITE);
        AddCounter(GPUPerfCounters::TCP_PERF_SEL_GL1_REQ_ATOMIC_WITH_RET);
        AddCounter(GPUPerfCounters::TCP_PERF_SEL_GL1_REQ_ATOMIC_WITHOUT_RET);
        AddCounter(GPUPerfCounters::GL2C_PERF_SEL_EA_RDREQ_DRAM_32B);
        AddCounter(GPUPerfCounters::GL2C_PERF_SEL_EA_WRREQ_DRAM_32B);

        auto csBlob = DX::ReadData(L"SwizzleModeTex1dAtomicCs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0,
            csBlob.data(),
            csBlob.size(),
            IID_GRAPHICS_PPV_ARGS(SampleTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(SampleTest::m_rootSignature);

        SampleTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        SampleTest::m_rootSignature.Reset();
        freeGpuMemory64KPages(SampleTest::m_resourceMemory);
        SampleTest::m_resourceMemory = 0;
        SampleTest::m_resourceMemorySizeIn64KbPages = 0;
    }

private:
    uint32_t m_resDimIdx;

    class SampleTest final : public Test
    {
    public:

        SampleTest(uint32_t opcodeIdx, uint32_t resdimIdx, uint32_t formatIdx, XG_SWIZZLE_MODE mode, const char *modeName)
            : m_opcodeIdx(opcodeIdx)
            , m_resdimIdx(resdimIdx)
            , m_formatIdx(formatIdx)
            , m_mode(mode)
            , m_modeName(modeName)
        { }

        void Initialize(ID3D12Device* device) override
        {
            std::wostringstream csName;
            csName << L"SwizzleMode";
            csName << s_allDimensions[m_resdimIdx].shaderName;
            csName << s_allOpcodes[m_opcodeIdx].opcodeName;
            csName << L"Cs.cso";
            auto csBlob = DX::ReadData(csName.str().c_str());

            D3D12_COMPUTE_PIPELINE_STATE_DESC csPipelineDesc =
            {
                /*.pRootSignature                   */  m_rootSignature.Get(),
                /*.CS                               */  {
                /*.CS.pShaderBytecode               */      csBlob.data(),
                /*.CS.BytecodeLength                */      csBlob.size(),
                                                        },
                /*.NodeMask =                       */  0U,
                /*.CachedPSO =                      */  {
                /*.CachedPso.pCachedBlob            */      0,
                /*.CachedPso.CachedBlobSizeInBytes  */      0
                                                        },
                /*.Flags                            */  D3D12_PIPELINE_STATE_FLAG_NONE
            };

            DX::ThrowIfFailed(device->CreateComputePipelineState(&csPipelineDesc, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_pipelineState);

            D3D12_RESOURCE_DESC resDesc = { };
            D3D12XBOX_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            D3D12XBOX_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

            resDesc.Alignment           = (1U << 16u) - 1U;
            resDesc.MipLevels           = 1U;
            resDesc.SampleDesc.Count    = 1U;
            resDesc.SampleDesc.Quality  = 0U;
            resDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

            uavDesc.CounterResourceLocation     = 0ULL;
            uavDesc.MemoryType                  = 0U;

            srvDesc.Shader4ComponentMapping     = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.MemoryType                  = 0U;
            srvDesc.TextureWarnLevelOfDetail    = 0.0;
            srvDesc.TexturePerfModulation       = 0;

            uavDesc.ViewDimension = static_cast<D3D12_UAV_DIMENSION>(s_allDimensions[m_resdimIdx].uavDim);
            srvDesc.ViewDimension = static_cast<D3D12_SRV_DIMENSION>(s_allDimensions[m_resdimIdx].srvDim);

            const uint32_t resourceShrinkFactor = IsLockhartClass() ? 2u : 1u;

            // Create the appropriate type of texture for the test
            switch (srvDesc.ViewDimension)
            {
                case D3D12_SRV_DIMENSION_TEXTURE1D:
                {
                    srvDesc.Texture1D.MostDetailedMip       = 0;
                    srvDesc.Texture1D.MipLevels             = resDesc.MipLevels;
                    srvDesc.Texture1D.ResourceMinLODClamp   = 0.0;

                    uavDesc.Texture1D.MipSlice              = 0;

                    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;

                    m_resSizeX = kDefault1dSizeX / resourceShrinkFactor;
                    m_resSizeY = 1U;
                    m_resSizeZ = 1U;

                    m_dispatchSizeX = m_resSizeX / (kDefault1dThreadgroupX * kNumImageOpsPerThreadgroupPerX);
                    m_dispatchSizeY = 1U;
                    m_dispatchSizeZ = 1U;
                }
                break;

                case D3D12_SRV_DIMENSION_TEXTURE2D:
                case D3D12_SRV_DIMENSION_TEXTURE2DMS:
                {
                    if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2D)
                    {
                        srvDesc.Texture2D.MostDetailedMip       = 0;
                        srvDesc.Texture2D.MipLevels             = resDesc.MipLevels;
                        srvDesc.Texture2D.PlaneSlice            = 0;
                        srvDesc.Texture2D.ResourceMinLODClamp   = 0.0;
                    }
                    else
                    {
                        resDesc.SampleDesc.Count = 2U;
                        srvDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
                    }

                    uavDesc.Texture2D.MipSlice              = 0;
                    uavDesc.Texture2D.PlaneSlice            = 0;

                    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

                    m_resSizeX = kDefault2dSizeX;
                    m_resSizeY = kDefault2dSizeY / resourceShrinkFactor;
                    m_resSizeZ = 1U;

                    m_dispatchSizeX = m_resSizeX / (kDefault2dThreadgroupX * kNumImageOpsPerThreadgroupPerX);
                    m_dispatchSizeY = m_resSizeY / (kDefault2dThreadgroupY * kNumImageOpsPerThreadgroupPerY);
                    m_dispatchSizeZ = 1U;
                }
                break;

                case D3D12_SRV_DIMENSION_TEXTURE3D:
                {
                    srvDesc.Texture3D.MostDetailedMip       = 0;
                    srvDesc.Texture3D.MipLevels             = resDesc.MipLevels;
                    srvDesc.Texture3D.ResourceMinLODClamp   = 0.0;

                    uavDesc.Texture3D.MipSlice      = 0;
                    uavDesc.Texture3D.FirstWSlice   = 0;
                    uavDesc.Texture3D.WSize         = kDefault3dSizeZ;

                    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;

                    m_resSizeX = kDefault3dSizeX;
                    m_resSizeY = kDefault3dSizeY;
                    m_resSizeZ = kDefault3dSizeZ / resourceShrinkFactor;

                    m_dispatchSizeX = m_resSizeX / (kDefault3dThreadgroupX * kNumImageOpsPerThreadgroupPerX);
                    m_dispatchSizeY = m_resSizeY / (kDefault3dThreadgroupY * kNumImageOpsPerThreadgroupPerY);
                    m_dispatchSizeZ = m_resSizeZ / (kDefault3dThreadgroupZ);
                }
                break;

            default:
                throw(std::exception("Add support for unrecognized texture dimension."));
                break;
            }
            resDesc.Width            = m_resSizeX;
            resDesc.Height           = m_resSizeY;
            resDesc.DepthOrArraySize = static_cast<UINT16>(m_resSizeZ);

            resDesc.Layout = static_cast<D3D12_TEXTURE_LAYOUT>(0x100 | m_mode);

            XG_RESOURCE_DESC const *xgResDesc = reinterpret_cast<XG_RESOURCE_DESC *>(&resDesc);
            XGTextureAddressComputer *addrComputer;
            XGCreateTextureComputer(xgResDesc, &addrComputer);
            UINT64 sizeInBytes = addrComputer->GetResourceSizeBytes();
            addrComputer->Release();
            if (sizeInBytes > (m_resourceMemorySizeIn64KbPages << 16u))
            {
                freeGpuMemory64KPages(m_resourceMemory);
                m_resourceMemorySizeIn64KbPages = static_cast<uint32_t>((sizeInBytes + ((1u << 16u) - 1u)) >> 16u);
                m_resourceMemory = allocateGpuMemory64KPages(m_resourceMemorySizeIn64KbPages);
            }

            uavDesc.ResourceLocation = m_resourceMemory;
            srvDesc.ResourceLocation = m_resourceMemory;

            auto dxgiFmt = s_allFormats[m_formatIdx].dxgiFmt;
            auto xboxFmt = s_allFormats[m_formatIdx].xboxFmt;
            resDesc.Format = dxgiFmt;
            uavDesc.Format = dxgiFmt;
            srvDesc.Format = dxgiFmt;
            srvDesc.ImageFormat = xboxFmt;

            D3D12_DESCRIPTOR_HEAP_DESC descHeapSrv =
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,             // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                2U,                                                 // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,          // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                    // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapSrv, IID_GRAPHICS_PPV_ARGS(m_descriptorHeap.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_descriptorHeap);

            auto descriptorSrvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                0U,
                device->GetDescriptorHandleIncrementSize(descHeapSrv.Type));

            auto descriptorUavCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                1U,
                device->GetDescriptorHandleIncrementSize(descHeapSrv.Type));

            m_descriptorSrvGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(),
                0U,
                device->GetDescriptorHandleIncrementSize(descHeapSrv.Type));
            m_descriptorUavGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(),
                1U,
                device->GetDescriptorHandleIncrementSize(descHeapSrv.Type));

            device->CreatePlacedRawUnorderedAccessViewX(&resDesc, &uavDesc, descriptorUavCpu);
            device->CreatePlacedRawShaderResourceViewX(&resDesc, &srvDesc, descriptorSrvCpu);
        }

        void Uninitialize() override
        {
            m_descriptorHeap.Reset();
            m_pipelineState.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << s_allOpcodes[m_opcodeIdx].opcodeName;
            name << L", " << s_allDimensions[m_resdimIdx].resourceName;
            name << L", " << m_modeName;
            name << L", " << s_allFormats[m_formatIdx].userName;

            return name.str();
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Op", L"", 6, 2);
            report->AddColumn(L"SwMode", L"", 9, 0);
            report->AddColumn(L"Format", L"", 13, 0);
            report->AddColumn(L"BPE", L"", 3, 0);
            report->AddColumn(L"Time", L" us", 9, 2);
            report->AddColumn(L"DRAM Bandwidth", L" GB/s", 9, 2);
            report->AddColumn(L"TA Req/Inst", L"", 10, 2);
            report->AddColumn(L"TCP Req/Inst", L"", 10, 2);
            report->AddColumn(L"TCP Req/Clock", L"", 10, 2);
            report->AddColumn(L"TCP Latency", L"", 10, 2);
            report->AddColumn(L"L1$ Req/Inst", L"", 10, 2);
            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
            const auto& gpuProperties = GpuProperties::Get();

            std::wostringstream name;

            name << s_allOpcodes[m_opcodeIdx].opcodeName;

            auto timeMs = m_elapsedTime * 1000.0;

            auto tcpBusy = GetCounterValue(GPUPerfCounters::GRBM_PERF_SEL_TCP_BUSY);
            auto inst = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_image_wavefronts);
            auto taReq = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_TA_REQ);
            auto tcpReq = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_REQ);
            auto tcpLatency = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_TCP_LATENCY);
            auto tcpStateReads = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_TA_REQ_STATE_READ);
            auto gl1Read = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_GL1_REQ_READ);
            auto gl1Write = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_GL1_REQ_WRITE);
            auto gl1AtomicRet = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_GL1_REQ_ATOMIC_WITH_RET);
            auto gl1AtomicNoRet = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_GL1_REQ_ATOMIC_WITHOUT_RET);
            auto rdReq32B = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_EA_RDREQ_DRAM_32B);
            auto wrReq32B = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_EA_WRREQ_DRAM_32B);


            auto numSe = gpuProperties.m_numSe;
            auto numCuPerSe = gpuProperties.m_numCuPerSe;
            auto perInst = 1.0f / float(inst);
            report->AddRowData(name.str());
            report->AddRowData(m_modeName);
            report->AddRowData(s_allFormats[m_formatIdx].userName);
            report->AddRowData(s_allFormats[m_formatIdx].elemBits);
            report->AddRowData(timeMs);
            report->AddRowData(static_cast<double>((rdReq32B + wrReq32B) * 32u) / (1000.0 * timeMs));
            report->AddRowData(taReq * perInst);
            report->AddRowData(tcpReq * perInst);
            report->AddRowData(tcpReq / float(numSe * numCuPerSe * tcpBusy));
            report->AddRowData(tcpLatency / float(tcpStateReads));
            report->AddRowData((gl1Read + gl1Write + gl1AtomicRet + gl1AtomicNoRet) * perInst);

            /** Transparent color has special meaning: when passed into EndRow, it signifies that even/odd report rows will be colored differently for readability */
            auto color = DirectX::Colors::Transparent;
            if (m_mode == XG_SWIZZLE_MODE_LINEAR)
            {
                color = DirectX::Colors::Tomato;
            }
            report->EndRow(color);

        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            ID3D12DescriptorHeap* descriptorHeaps[] =
            {
                m_descriptorHeap.Get(),
            };
            commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

            commandList->SetComputeRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());

            commandList->SetComputeRootDescriptorTable(0, m_descriptorSrvGpu);
            commandList->SetComputeRootDescriptorTable(1, m_descriptorUavGpu);

            struct CbData
            {
                UINT x, y, z, w;
                float InvX, InvY, InvZ, InvW;
            } cbData = { m_dispatchSizeX, m_dispatchSizeY, m_dispatchSizeZ, 1u,
                         1.0f / (float)m_resSizeX, 1.0f / (float)m_resSizeY, 1.0f / (float)m_resSizeZ, 1.0f
                        };

            commandList->SetComputeRoot32BitConstants(2, sizeof(cbData) / sizeof(UINT), (void*)&cbData, 0);
            commandList->Dispatch(m_dispatchSizeX, m_dispatchSizeY, m_dispatchSizeZ);
        }

        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>      m_rootSignature;
        static D3D12_GPU_VIRTUAL_ADDRESS        m_resourceMemory;
        static uint32_t                         m_resourceMemorySizeIn64KbPages;

    private:
        // Resources which are unique per-test
        ComPtr<ID3D12PipelineState>             m_pipelineState;

        ComPtr<ID3D12DescriptorHeap>            m_descriptorHeap;
        D3D12_GPU_DESCRIPTOR_HANDLE             m_descriptorSrvGpu;
        D3D12_GPU_DESCRIPTOR_HANDLE             m_descriptorUavGpu;

        uint32_t                                m_opcodeIdx;
        uint32_t                                m_resdimIdx;
        uint32_t                                m_formatIdx;
        XG_SWIZZLE_MODE                         m_mode;
        const char *                            m_modeName;
        uint32_t                                m_resSizeX;
        uint32_t                                m_resSizeY;
        uint32_t                                m_resSizeZ;
        uint32_t                                m_dispatchSizeX;
        uint32_t                                m_dispatchSizeY;
        uint32_t                                m_dispatchSizeZ;
    };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature>     SwizzleModeBenchmark::SampleTest::m_rootSignature;
D3D12_GPU_VIRTUAL_ADDRESS       SwizzleModeBenchmark::SampleTest::m_resourceMemory;
uint32_t                        SwizzleModeBenchmark::SampleTest::m_resourceMemorySizeIn64KbPages;

SwizzleModeBenchmark benchmark2d(1u);
SwizzleModeBenchmark benchmark2dmsaa(2u);
SwizzleModeBenchmark benchmark3d(3u);
