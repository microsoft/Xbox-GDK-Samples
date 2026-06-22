cbuffer cb : register (b0)
{
    uint4 dispatchSize;
    float4 ElementSizeInverted;
};

SamplerState DefaultSampler : s0;

#define kOpcodeTypeLoad    1
#define kOpcodeTypeSample  2
#define kOpcodeTypeStore   3
#define kOpcodeTypeAtomic  4

/** For Atomic opcodes only `uint` data type is supported */
#if OpcodeType == kOpcodeTypeAtomic
#   define DataType uint
#else
#   define DataType float4
#endif

/** Validate ResourceDim against Msaa setting and define resource definition macro */
#if MsaaEnabled && ResourceDim != 2
#   error Unsupported resource dimension for Msaa
#endif

/** Validate resource dimensions and initialise dispatch dimensions assuming */
#if ResourceDim == 1 
#   define ThreadCoordTypeU32 uint
#   define ThreadCoordTypeF32 float
#   define ThreadCoordSwizzle x
#   define DefaultTgSizeX 64
#   define DefaultTgSizeY 1
#   define DefaultTgSizeZ 1
      Texture1D<DataType> RoTex : register(t0);
    RWTexture1D<DataType> RwTex : register(u0);
#elif ResourceDim == 2
#   define ThreadCoordTypeU32 uint2
#   define ThreadCoordTypeF32 float2
#   define ThreadCoordSwizzle xy
#   define DefaultTgSizeX 8
#   define DefaultTgSizeY 8
#   define DefaultTgSizeZ 1
#   if MsaaEnabled
      Texture2DMS<DataType, 2> RoTex : register(t0);
    RWTexture2DMS<DataType, 2> RwTex : register(u0);
#   else
      Texture2D<DataType> RoTex : register(t0);
    RWTexture2D<DataType> RwTex : register(u0);
#   endif
#elif ResourceDim == 3
#   define ThreadCoordTypeU32 uint3
#   define ThreadCoordTypeF32 float3
#   define ThreadCoordSwizzle xyz
#   define DefaultTgSizeX 4
#   define DefaultTgSizeY 4
#   define DefaultTgSizeZ 4
      Texture3D<DataType> RoTex : register(t0);
    RWTexture3D<DataType> RwTex : register(u0);
#else
#   error Unsupported ResourceDim
#endif

/** Initialiase threadgroup dimensions if they were not defined by the user */
#ifndef TgSizeX
#   define TgSizeX DefaultTgSizeX
#endif

#ifndef TgSizeY
#   define TgSizeY DefaultTgSizeY
#endif

#ifndef TgSizeZ
#   define TgSizeZ DefaultTgSizeZ
#endif

#define TgSize (TgSizeX * TgSizeY * TgSizeZ)

/** Validate threadgroup sizes */
#ifdef __XBOX_SCARLETT
#   if TgSize == 32
#      define __XBOX_ENABLE_WAVE32 1
#   elif TgSize == 64
#      define __XBOX_ENABLE_WAVE32 0
#   else
#      error Threadgroup size should be equal to the size of one wave, either 32 or 64
#   endif
#else
#   if TgSize != 64
#       error TgSize should contain 64 threads on Xbox One
#   endif
#endif

/** Validate OpcodeType against Msaa */
#if MsaaEnabled
#   if OpcodeType == kOpcodeTypeLoad
        static void Opcode(ThreadCoordTypeU32 ThreadId, uint LocalId)
        {
            DataType elem = RoTex.Load(ThreadId, 0);
            if (all(ThreadId == 0xffffffffu) /* the condition is always 'FALSE' but the compiler doesn't know and keeps the `image_load` instruction */)
            {
                RwTex.XB_Store(ThreadId, 0, elem);
            }
        }
#   elif OpcodeType == kOpcodeTypeStore
        static void Opcode(ThreadCoordTypeU32 ThreadId, uint LocalId)
        {
            RwTex.XB_Store(ThreadId, 0, LocalId);
        }
#   elif OpcodeType == kOpcodeTypeAtomic || OpcodeType == kOpcodeTypeSample
#       error Unsupported OpcodeType for Msaa
#   else
#       error Unknown OpcodeType
#   endif
#else
#   if OpcodeType == kOpcodeTypeLoad
        static void Opcode(ThreadCoordTypeU32 ThreadId, uint LocalId)
        {
            DataType elem = RoTex[ThreadId];
            if (all(ThreadId == 0xffffffffu) /* the condition is always 'FALSE' but the compiler doesn't know and keeps the `image_load` instruction */)
            {
                RwTex[ThreadId] = elem;
            }
        }
#   elif OpcodeType == kOpcodeTypeStore
        static void Opcode(ThreadCoordTypeU32 ThreadId, uint LocalId)
        {
            RwTex[ThreadId] = LocalId;
        }
#   elif OpcodeType == kOpcodeTypeAtomic
        static void Opcode(ThreadCoordTypeU32 ThreadId, uint LocalId)
        {
            InterlockedAdd(RwTex[ThreadId], LocalId);
        }
#   elif OpcodeType == kOpcodeTypeSample
        static void Opcode(ThreadCoordTypeU32 ThreadId, uint LocalId)
        {
            DataType elem = RoTex.SampleLevel(DefaultSampler, (ThreadCoordTypeF32(ThreadId) + 0.5) * ElementSizeInverted.ThreadCoordSwizzle, 0);
            if (all(ThreadId == 0xffffffffu) /* the condition is always 'FALSE' but the compiler doesn't know and keeps the `image_sample` instruction */)
            {
                RwTex[ThreadId] = elem;
            }
        }
#   else
#       error Unknown OpcodeType
#   endif
#endif


[RootSignature("DescriptorTable(SRV(t0, numDescriptors=1),visibility=SHADER_VISIBILITY_ALL), DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL), RootConstants(b0, num32bitconstants=8), StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT, ComparisonFunc = COMPARISON_NEVER)")]
[numthreads(TgSizeX, TgSizeY, TgSizeZ)]
void main(ThreadCoordTypeU32 GroupId : SV_GroupId, ThreadCoordTypeU32 GroupThreadId : SV_GroupThreadId, uint LocalId : SV_GroupIndex)
{
    ThreadCoordTypeU32 ThreadId;
    ThreadId.x = (GroupId.x * TgSizeX) << 2u;
#if ResourceDim > 1
    ThreadId.y = (GroupId.y * TgSizeY) << 1u;
#endif
#if ResourceDim > 2
    ThreadId.z = (GroupId.z * TgSizeZ);
#endif
    ThreadId += GroupThreadId;

    ThreadCoordTypeU32 Start = ThreadId;
    Opcode(ThreadId, LocalId);
    ThreadId.x += TgSizeX;
    Opcode(ThreadId, LocalId);
    ThreadId.x += TgSizeX;
    Opcode(ThreadId, LocalId);
    ThreadId.x += TgSizeX;
    Opcode(ThreadId, LocalId);
#if ResourceDim > 1
    ThreadId = Start;
    ThreadId.y += TgSizeY;
    Opcode(ThreadId, LocalId);
    ThreadId.x += TgSizeX;
    Opcode(ThreadId, LocalId);
    ThreadId.x += TgSizeX;
    Opcode(ThreadId, LocalId);
    ThreadId.x += TgSizeX;
    Opcode(ThreadId, LocalId);
#endif
}
