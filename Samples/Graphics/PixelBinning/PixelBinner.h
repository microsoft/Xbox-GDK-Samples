//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>

/**
 * @brief   Initializes internal root signatures and pipeline states
 *
 * @param[in]   device  A pointer to D3D12 device
 */
void pixelBinnerInit(struct ID3D12Device *device);

/**
 * @brief   Releases internal root signatures and pipeline states
 */
void pixelBinnerTerm(void);

/**
 * @brief   Retrieves the maximal number of supported bins to assign pixels to
 *          and the number of 4-byte counters that can be used to complete this assignment
 *
 * @param[out]  outMaxBinsSupported     A pointer to a uint16_t receiving the number of bins supported
 * @param[out]  outMaxCountersSupported A pointer to a uint32_t receiving the number of 4-byte counters supported
 *
 * @note    It is safe to cache returned values as they are determined at compile-time
 */
void pixelBinnerGetLimits(uint16_t *outMaxBinsSupported, uint32_t *outMaxCountersSupported);

/**
 * @brief   Computes the number of bins and the number of 4-byte counters that will be used after applying internal limits
 *
 * @param[in,out]   inoutNumBinsUsed        A pointer to a uint16_t storing the number of bins to apply limits to
 * @param[in,out]   inoutNumCountersUsed    A pointer to a uint32_t storing the number of counters to apply limits to
 *
 * @note    It is fine to pass any values as this function applies lower and upper limits
 *          Returned values could be higher than passed into the function
 *          It is recommended to pass both non-null pointers
 *
 * @see     pixelBinnerGetLimits
 */
void pixelBinnerApplyLimits(uint16_t *inoutNumBinsUsed, uint32_t *inoutNumCountersUsed);

/**
 * @brief   Controls how pixel coordinates are packed within the buffer.
 *
 *          Global packing collects all the pixels with the same id across the entire image and stores them adjacently in memory.
 *          [                      All the pixels within image                            ]
 *          [pixels_with_id 0]..[pixels_with_id 5]..[ pixels_with_id M]..[pixels_with_id N]
 *          This way of packing allows to process pixels with the same id by specialized shader.
 *          A buffer with arguments per each bin id for ExecuteIndirect is created. It allows to change a shader
 *          and dispatch the number of threadgroups processing only pixels with that id.
 *          Potential applications: deferred material classification/any tile classification techniques.
 *
 *          Local packing collects all the pixels with same id across a single macro tile and stores them adjacently in memory.
 *          [     Pixels within macro tile 0     ]....[     Pixels within macro tile M      ]
 *          [pixels_with_id 0]..[pixels_with_id N]....[ pixels_with_id 0]..[pixels_with_id N]
 *          This way of packing doesn't allow to process pixels with a particular id by a specialized shader,
 *          but at the same time it improves coherency by making sure that threadgroups will process packed
 *          pixels with the same id rather than with different ids
 *          Potential applications: raytracing. Sort pixels based on ray id/key created based on direction/position to make sure
 *          threadgroups doing raytracing process rays which are more spatially coherent.
 */
typedef enum PixelBinnerPackingType
{
    kPixelBinnerPackingType_Global   = 0x0, /**< Collect pixels across the image, store pixels with the same id adjacently  */
    kPixelBinnerPackingType_Local    = 0x1  /**< Collect pixels across a macro tile, store pixels with with the same id adjacently */
} PixelBinnerPackingType;

/**
 * @brief   Holds all necessary parameters to run binning and compute memory requirements for the intermediate resources.
 *
 * @note    Should be recomputed if either resolution of a texture for which binning is done -- changes or
 *          the number of bins used to classify pixels to is modified
 *
 * @see     pixelBinnerInitConfig
 */
typedef struct PixelBinnerConfig
{
    uint16_t    m_resX;                     /**< The width  of a texture in pixels this configuration computed for */
    uint16_t    m_resY;                     /**< The height of a texture in pixels this configuration computed for */

    uint8_t     m_log2MicroTileSizeX;       /**< log2 of the width  of the micro tile chosen based on 'm_numBinsRequired' */
    uint8_t     m_log2MicroTileSizeY;       /**< log2 of the height of the micro tile chosen based on 'm_numBinsRequired' */

    uint8_t     m_log2MacroTileSizeX;       /**< log2 of the width  of the macro tile chosen based on dimensions of the micro tile and applied constraints */
    uint8_t     m_log2MacroTileSizeY;       /**< log2 of the height of the macro tile chosen based on dimensions of the micro tile and applied constraints */

    uint16_t    m_numMicroTilesX;           /**< The number of micro tiles in x dimension covering the entire texture of m_resX x m_resY pixels */
    uint16_t    m_numMicroTilesY;           /**< The number of micro tiles in y dimension covering the entire texture of m_resX x m_resY pixels */

    uint16_t    m_numMacroTilesX;           /**< The number of macro tiles in x dimension covering the entire texture of m_resX x m_resY pixels */
    uint16_t    m_numMacroTilesY;           /**< The number of macro tiles in y dimension covering the entire texture of m_resX x m_resY pixels */

    uint16_t    m_numBinsRequired;          /**< The actual number of bins that can be used to classify pixels */
    uint8_t     m_packingType;
    uint8_t     m_padding0;

    uint32_t    m_numU32CountersReserved;   /**< The number of 4-byte counters used to constrain the number of macro tiles */
    uint32_t    m_numU32CountersRequired;   /**< The number of 4-byte counters required to complete binning for a given configuration */

    uint8_t     m_log2PrefixSizeL1;         /**< log2 of the size of the kernel computing level1 prefix sum */
    uint8_t     m_log2PrefixSizeL2;         /**< log2 of the size of the kernel computing level2 prefix sum */

    uint16_t    m_dispatchSizeL1;           /**< The dispatch dimension of level1 prefix sum */

    uint32_t m_overridePSOIndex : 1;
    uint32_t m_countingPSOIndex : 2;
} PixelBinnerConfig;

/**
 * @brief   Initializes a PixelBinnerConfig structure.
 *
 * @param[out]  outConfig       A pointer to a structure which receives all necessary parameters
 * @param[in]   resX            Width of a texture in pixels
 * @param[in]   resY            Height of a texture in pixels
 * @param[in]   numBinsRequired The number of bins to classify pixels to. Actual number will rounded to the next power of 2
 *
 * @see     PixelBinnerConfig
 */
void pixelBinnerInitConfig(PixelBinnerConfig *outConfig, uint16_t resX, uint16_t resY, uint16_t numBinsRequired, PixelBinnerPackingType packing);

/**
 * @brief   Limits the number of counters used for binning
 *
 * @param[in, out]  inoutConfig             A pointer to a structure which receives updated internal parameters
 * @param[in]       numU32CountersReserved  The maximal number of counters that can be used for binning. The valid range could be determined by pixelBinnerApplyLimits
 *
 * @note    Updates dimensions of the macro tile originally determined by pixelBinnerInitConfig
 *
 */
void pixelBinnerConstrainNumCounters(PixelBinnerConfig *inoutConfig, uint32_t numU32CountersReserved);

/**
 * @brief   Allows to specify the size of the macro tile used for binning
 *
 * @param[in, out]  inoutConfig                     A pointer to a structure which receives updated internal parameters
 * @param[in]       numMicroTilesPerMacroTileXLog2  A log2 of the width  of the macro tile in micro tiles
 * @param[in]       numMicroTilesPerMacroTileYLog2  A log2 of the height of the macro tile in micro tiles
 *
 * @note    Constraining the size of the macro tile can be helpful to define exactly thos areas of the texture within every
 *          of each pixels are shuffled. After the binning/classification is completed, pixels from the same macro tile
 *          are stored adjacently in memory ordered only by bin ids.
 *          Moreover, the size of the macro tile also controls the total number of macro tiles covering the image
 *          and therefore the size of intermediate memory required to complete binning. The number of allocated
 *          4-byte elements for every macro tile is equal to the number of bins used for a given binning/classification config.
 *          See PixelBinnerConfig::m_numBinsRequired
 */
void pixelBinnerConstrainMacroTileSize(PixelBinnerConfig *inoutConfig, uint32_t numMicroTilesPerMacroTileXLog2, uint32_t numMicroTilesPerMacroTileYLog2);

/**
 * @brief   Specifies information about a memory block
 */
typedef struct PixelBinnerMemSizeAlign
{
    uint32_t m_size;
    uint32_t m_align;
} PixelBinnerMemSizeAlign;

/**
 * @brief   Defines ids for resources used by the pixel binner
 */
typedef enum PixelBinnerResId
{
    kPixelBinnerResId_PrefixLevel1  = 0,    /**< For internal use only. References a buffer which stores prefix sums of 4-byte elements within blocks of some fixed size */
    kPixelBinnerResId_PrefixLevel2  = 1,    /**< For internal use only. References a buffer which stores prefix sums of all blocks of 4-byte elements */
    kPixelBinnerResId_BinnedPixels  = 2,    /**< For internal and external use. References a buffer which stores pixel coordinates from the same bin within macro adjacently. */
    kPixelBinnerResId_PixelOffsets  = 3,    /**< For internal and external use. References a buffer with indirect arguments specifying per bin id dispatch arguments, offset into kPixelBinnerResId_BinnedPixels ans size of pixel sequence */
    kPixelBinnerResId_BinIdTex2d    = 4,    /**< For internal and external use. References a texture which store bin ids for every pixel */
    kPixelBinnerResId_CheckTex2d    = 5,    /**< For internal use only. References a texture used to verify correctness of the binning pass */
    kPixelBinnerResId_OutputTex2d   = 6,    /**< For internal and external use. References a texture where any debug visualizaiton is drawn */
    kPixelBinnerResId_Count,

    kPixelBinnerResId_MaskBinIdTex2d= 1u << kPixelBinnerResId_BinIdTex2d,
    kPixelBinnerResId_MaskCheckTex2d= 1u << kPixelBinnerResId_CheckTex2d,

    kPixelBinnerResId_MaskBufOnly   = (1u << kPixelBinnerResId_PrefixLevel1)
                                    | (1u << kPixelBinnerResId_PrefixLevel2)
                                    | (1u << kPixelBinnerResId_BinnedPixels)
                                    | (1u << kPixelBinnerResId_PixelOffsets),

    kPixelBinnerResId_MaskTexOnly    = kPixelBinnerResId_MaskBinIdTex2d | kPixelBinnerResId_MaskCheckTex2d,
    kPixelBinnerResId_MaskMainPass   = kPixelBinnerResId_MaskBufOnly | kPixelBinnerResId_MaskBinIdTex2d,
    kPixelBinnerResId_MaskAll        = kPixelBinnerResId_MaskBufOnly | kPixelBinnerResId_MaskTexOnly
} PixelBinnerResId;

typedef struct PixelBinnerResources
{
    ID3D12Resource         *m_d3dPtrs[kPixelBinnerResId_Count];
    uint64_t                m_gpuAddr[kPixelBinnerResId_Count];

    uint64_t                m_gpuSrvDescs[kPixelBinnerResId_Count];
    uint64_t                m_gpuUavDescs[kPixelBinnerResId_Count];

    uint64_t                m_cpuSrvDescs[kPixelBinnerResId_Count];
    uint64_t                m_cpuUavDescs[kPixelBinnerResId_Count];

    PixelBinnerMemSizeAlign m_memoryInfos[kPixelBinnerResId_Count];
} PixelBinnerResources;

/**
 * @brief   Computes memory requirements for resources specified by a mask. Computed results stores in PixelBinnerResources::m_memoryInfos
 *
 * @param[out]  outRes  Receives information about required memory for every resource
 * @param[in]   mask    A bitmask which specifies for which resources to query memory requirements
 * @param[in]   device  A D3D12 device
 * @param[in]   config  A configuration structure used to estimate required memory for every resource
 */
void pixelBinnerQueryMemoryRequirements(PixelBinnerResources *outRes, uint32_t mask, struct ID3D12Device *device, PixelBinnerConfig const *config);

/**
 * @brief   Creates committed resources for every resource id specified by the `mask` parameter. The following members of `outRes` are updated
 *          `PixelBinnerResources::m_d3dPtrs` receives pointers to ID3D12Resource interface for every created resource
 *          `PixelBinnerResources::m_gpuAddr` receives pointers to Gpu visible memory blocks allocated for every created resources
 *          `PixelBinnerResources::m_memoryInfos` receives sizes and alignments of memory blocks allocated for every resource
 *
 * @param[out]  outRes  Receives pointers to ID3D12Resource resources, pointers to Gpu visible memory blocks and sizes of those memory blocks
 * @param[in]   mask    A bitmask which specifies what resources to create
 * @param[in]   device  A D3D12 device
 * @param[in]   config  A configuration structure used to estimate required memory for every resource
 *
 * @note    This method helps to quickly create committed resources and use its underlying memory without a need to allocate/manage memory
 */
void pixelBinnerCreateCommittedResources(PixelBinnerResources *outRes, uint32_t mask, struct ID3D12Device *device, PixelBinnerConfig const *config);

/**
 * @brief   Creates Srv descriptors at designated locations for resources specified by the `mask` parameter
 *
 * @param[in,out]   outRes  Specifies where to place created Srv descriptors through `PixelBinnerResources::m_cpuSrvDescs` which must contain valid Cpu handles from ID3D12Heap
 * @param[in]       mask    A bitmask which specifies for what resources Srvs should be created
 * @param[in]       device  A D3D12 device
 * @param[in]       config  A configuration structure used to estimate required memory for every resource
 */
void pixelBinnerCreateSrvDescriptors(PixelBinnerResources const *outRes, uint32_t mask, struct ID3D12Device *device, PixelBinnerConfig const *config);

/**
 * @brief   Creates Uav descriptors at designated locations for resources specified by the `mask` parameter
 *
 * @param[in,out]   outRes  Specifies where to place created Srv descriptors through `PixelBinnerResources::m_cpuUavDescs` which must contain valid Cpu handles from ID3D12Heap
 * @param[in]       mask    A bitmask which specifies for what resources Uavs should be created
 * @param[in]       device  A D3D12 device
 * @param[in]       config  A configuration structure used to estimate required memory for every resource
 */
void pixelBinnerCreateUavDescriptors(PixelBinnerResources const *outRes, uint32_t mask, struct ID3D12Device *device, PixelBinnerConfig const *config);

/**
 * @brief   Submits commands of the main binning pass to a command list.
 *          The main pass does all the work required to sort pixels in a way that those with the same bin ids are stored adjacently in memory.
 *          Only pixels from the same macro tile are stored adjacently. Pixels with the same bin id, but from different macro tiles are still
 *          separated.
 *
 * @param[in]   cmdList     A pointer to a command list where commands are recorded
 * @param[in]   config      A pointer to a structure that holds configuration of an instance of the pixel binning pass
 * @param[in]   resources   A pointer to a structure that holds input/output Gpu resources for the pass. The following resources must be valid:
 *                          `PixelBinnerResources::m_gpuAddr` must contain valid memory addresses through the range [PixelBinnerResId_CountsLevel1 .. PixelBinnerResId_BinnedPixels]
 *                          `PixelBinnerResources::m_gpuSrvDesc` must contain valid Gpu Srv handle at PixelBinnerResId_BinIdTex2d index
 */
void pixelBinnerSubmit(ID3D12GraphicsCommandList *cmdList, PixelBinnerConfig const *config, PixelBinnerResources const *resources);

/**
 * @brief   Specifies identifiers for debug visualization passes.
 *          Devergency pass shows the rate of how many unique ids are used in every micro tile relative to the total number of pixels in this micro tile
 *          PixelOrder pass shows ordering of groups of pixels with the same id relative to to the entire sequence of pixels
 */
typedef enum PixelBinnerDbgVisPassId
{
    kPixelBinnerDbgVisPassId_Divergency = 0x1,  /**< The identifier for divergency pass */
    kPixelBinnerDbgVisPassId_PixelOrder = 0x2,  /**< The identifier for PixelOrder pass */
    kPixelBinnerDbgVisPassId_Count              /**< The number of debug visualization passes */
} PixelBinnerDbgVisPassId;

/**
 * @brief   Submits commands of the debug visualization pass to a command list.
 *          Debug visualization pass shows divergency rate of bin ids within each micro tile.
 *          The shape of macro tile for a given configuration (as binning is done per macro tile) is shown with slighly attenuated colors in a checkerboard pattern
 *
 * @param[in]   cmdList     A pointer to a command list where commands are recorded
 * @param[in]   config      A pointer to a structure that holds configuration of an instance of the pixel binning pass
 * @param[in]   resources   A pointer to a structure that holds input/output Gpu resources for the pass. The following resources must be valid:
 *                          `PixelBinnerResources::m_gpuSrvDesc` must contain valid Gpu Srv handle at PixelBinnerResId_BinIdTex2d index
 *                          `PixelBinnerResources::m_gpuUavDesc` must contain valid Gpu Uav handle at PixelBinnerResId_OutputTex2d index
 * @param[in]   passId      An identifier of debug visualization pass to submit
 */
void pixelBinnerSubmitDbgVisPass(ID3D12GraphicsCommandList *cmdList, PixelBinnerConfig const *config, PixelBinnerResources const *resources, PixelBinnerDbgVisPassId passId);

/**
 * @brief   Submits commands of the checking pass to a command list.
 *          The checking pass highlights in pink those macro tiles that fails verification.
 *
 * @param[in]   cmdList     A pointer to a command list where commands are recorded
 * @param[in]   config      A pointer to a structure that holds configuration of an instance of the pixel binning pass
 * @param[in]   resources   A pointer to a structure that holds input/output Gpu resources for the pass. The following resources must be valid:
 *                         `PixelBinnerResources::m_gpuAddr` must contain valid memory addresses at PixelBinnerResId_BinnedPixels/{_CheckTex2d} indices
 *                         `PixelBinnerResources::m_gpuUavDesc` must contain valid Gpu Uav handles at PixelBinnerResId_CheckTex2d/{_OutputTex2d} indices
 */
void pixelBinnerSubmitCheckPass(ID3D12GraphicsCommandList *cmdList, PixelBinnerConfig const *config, PixelBinnerResources const *resources);
