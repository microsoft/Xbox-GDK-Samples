//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "SparseLightingCommon.hlsli"

#ifndef SPARSE_LIGHTING_USE_21BIT_UNORM_PACKED_DEPTH
# error This header needs to know whether to pack depth or not
#endif

// Rotate per frame which pixel is to be lit, could be a good opton for a title, but makes the deblocker temporarily unstable
#ifndef SPARSE_LIGHTING_PER_FRAME_ROTATE_LIT_PIXEL
# define SPARSE_LIGHTING_PER_FRAME_ROTATE_LIT_PIXEL         1
#endif

#define CLEAR_CODE_SHIFT_00                                 0
#define CLEAR_CODE_SHIFT_10                                 1
#define CLEAR_CODE_SHIFT_01                                 2
#define CLEAR_CODE_SHIFT_11                                 3

#define CLEAR_CODE_00                                       (1 << CLEAR_CODE_SHIFT_00)
#define CLEAR_CODE_10                                       (1 << CLEAR_CODE_SHIFT_10)
#define CLEAR_CODE_01                                       (1 << CLEAR_CODE_SHIFT_01)
#define CLEAR_CODE_11                                       (1 << CLEAR_CODE_SHIFT_11)
#define CLEAR_CODE_ALL                                      0xf

/*
    Pack data for whether to light this pixel and optionally copy results in each 2x2 block

    4 pixels packed into 4 nibbles
    Saves VGPRs, but importantly, allows the state of multiple pixels to be set in one assignment
*/
#define BUILD_WRITE_CODE_COPY_H                             1u
#define BUILD_WRITE_CODE_COPY_V                             2u
#define BUILD_WRITE_CODE_COPY_D                             4u
#define BUILD_WRITE_CODE_COPY_BITS_MASK                     7u

#define BUILD_WRITE_CODE_WRITE_PIXEL                        8u

// build nibble for a pixel
#define BUILD_WRITE_CODE_NIBBLE(copyH, copyV, copyD)        (BUILD_WRITE_CODE_WRITE_PIXEL | ((copyH) ? BUILD_WRITE_CODE_COPY_H: 0) | ((copyV) ? BUILD_WRITE_CODE_COPY_V: 0) | ((copyD) ? BUILD_WRITE_CODE_COPY_D: 0))
#define BUILD_WRITE_CODE_NIBBLE_NO_COPY                     BUILD_WRITE_CODE_WRITE_PIXEL

// build nibbles for 2x2 pixels
#define BUILD_WRITE_CODE_VERT_SHIFT                         4u
#define BUILD_WRITE_CODE_HORZ_SHIFT                         8u
#define BUILD_WRITE_CODE_DIAG_SHIFT                         (BUILD_WRITE_CODE_VERT_SHIFT | BUILD_WRITE_CODE_HORZ_SHIFT) // == 12

#define BUILD_WRITE_CODE_NIBBLE_00(copyH, copyV, copyD)     (BUILD_WRITE_CODE_NIBBLE(copyH, copyV, copyD))
#define BUILD_WRITE_CODE_NIBBLE_01(copyH, copyV, copyD)     (BUILD_WRITE_CODE_NIBBLE(copyH, copyV, copyD) << BUILD_WRITE_CODE_VERT_SHIFT)
#define BUILD_WRITE_CODE_NIBBLE_10(copyH, copyV, copyD)     (BUILD_WRITE_CODE_NIBBLE(copyH, copyV, copyD) << BUILD_WRITE_CODE_HORZ_SHIFT)
#define BUILD_WRITE_CODE_NIBBLE_11(copyH, copyV, copyD)     (BUILD_WRITE_CODE_NIBBLE(copyH, copyV, copyD) << BUILD_WRITE_CODE_DIAG_SHIFT)      

#define BUILD_WRITE_CODE_NIBBLE_00_NO_COPY                  (BUILD_WRITE_CODE_NIBBLE_NO_COPY)
#define BUILD_WRITE_CODE_NIBBLE_01_NO_COPY                  (BUILD_WRITE_CODE_NIBBLE_NO_COPY << BUILD_WRITE_CODE_VERT_SHIFT)
#define BUILD_WRITE_CODE_NIBBLE_10_NO_COPY                  (BUILD_WRITE_CODE_NIBBLE_NO_COPY << BUILD_WRITE_CODE_HORZ_SHIFT)
#define BUILD_WRITE_CODE_NIBBLE_11_NO_COPY                  (BUILD_WRITE_CODE_NIBBLE_NO_COPY << BUILD_WRITE_CODE_DIAG_SHIFT)      

#define GET_WRITE_CODE_NIBBLE_00(writeCode)                 ((writeCode) & 0xf)
#define GET_WRITE_CODE_NIBBLE_01(writeCode)                 (((writeCode) >> BUILD_WRITE_CODE_VERT_SHIFT) & 0xf)
#define GET_WRITE_CODE_NIBBLE_10(writeCode)                 (((writeCode) >> BUILD_WRITE_CODE_HORZ_SHIFT) & 0xf)
#define GET_WRITE_CODE_NIBBLE_11(writeCode)                 (((writeCode) >> BUILD_WRITE_CODE_DIAG_SHIFT) & 0xf)

// set all pixels in 2x2 to be individually lit
#define BUILD_WRITE_CODE_LIGHT_ALL4_PIXELS                  (BUILD_WRITE_CODE_NIBBLE_00_NO_COPY | BUILD_WRITE_CODE_NIBBLE_01_NO_COPY | BUILD_WRITE_CODE_NIBBLE_10_NO_COPY | BUILD_WRITE_CODE_NIBBLE_11_NO_COPY)

// are we rotating which pixels are lit per frame?
#if SPARSE_LIGHTING_PER_FRAME_ROTATE_LIT_PIXEL == 1
# define SPARSE_LIGHTING_FRAME_INDEX                        sparseLightingFrameIndex                    // from constant buffer
# define SPARSE_LIGHTING_L_COPY_CODE_TL                     sparseLightingLCopyCodeTL                   // top left
# define SPARSE_LIGHTING_L_COPY_CODE_TR                     sparseLightingLCopyCodeTR                   // top right
# define SPARSE_LIGHTING_L_COPY_CODE_BL                     sparseLightingLCopyCodeBL
# define SPARSE_LIGHTING_L_COPY_CODE_BR                     sparseLightingLCopyCodeBR
#else
# define SPARSE_LIGHTING_FRAME_INDEX                        0L
# define SPARSE_LIGHTING_L_COPY_CODE_TL                     BUILD_WRITE_CODE_NIBBLE_00(true, true, false)
# define SPARSE_LIGHTING_L_COPY_CODE_TR                     BUILD_WRITE_CODE_NIBBLE_01(true, true, false)
# define SPARSE_LIGHTING_L_COPY_CODE_BL                     BUILD_WRITE_CODE_NIBBLE_10(true, true, false)
# define SPARSE_LIGHTING_L_COPY_CODE_BR                     BUILD_WRITE_CODE_NIBBLE_11(true, true, false)
#endif

#define BUILD_WRITE_CODE_COPY_ALL_SHIFT                     (sparseLightingFrameIndex * 4)                                          // legal values for upload are 0..3
#define BUILD_WRITE_CODE_COPY_HORIZ_SHIFT0                  ((sparseLightingFrameIndex & 1) * BUILD_WRITE_CODE_HORZ_SHIFT)          // 0 or 8, which are the offsets for the xaxis, i.e. 00 to 10, or 01 to 11
#define BUILD_WRITE_CODE_COPY_HORIZ_SHIFT1                  (((sparseLightingFrameIndex ^ 1) & 1) * BUILD_WRITE_CODE_HORZ_SHIFT)    // inverse of above

#define BUILD_WRITE_CODE_COPY_VERT_SHIFT0                   ((sparseLightingFrameIndex & 1) * BUILD_WRITE_CODE_VERT_SHIFT)          // 0 or 4, which are the offsets for the yaxis, i.e. 00 to 01, or 10 to 11
#define BUILD_WRITE_CODE_COPY_VERT_SHIFT1                   (((sparseLightingFrameIndex ^ 1) & 1) * BUILD_WRITE_CODE_VERT_SHIFT)    // inverse of above

#define BUILD_WRITE_CODE_NIBBLE_COPY_ALL                    (BUILD_WRITE_CODE_NIBBLE(true, true, true) << BUILD_WRITE_CODE_COPY_ALL_SHIFT)

#define BUILD_WRITE_CODE_NIBBLE_COPY_HORIZ0                 (BUILD_WRITE_CODE_NIBBLE_00(true, false, false) << BUILD_WRITE_CODE_COPY_HORIZ_SHIFT0)
#define BUILD_WRITE_CODE_NIBBLE_COPY_HORIZ1                 (BUILD_WRITE_CODE_NIBBLE_01(true, false, false) << BUILD_WRITE_CODE_COPY_HORIZ_SHIFT1)
#define BUILD_WRITE_CODE_NIBBLE_COPY_VERT0                  (BUILD_WRITE_CODE_NIBBLE_00(false, true, false) << BUILD_WRITE_CODE_COPY_VERT_SHIFT0)
#define BUILD_WRITE_CODE_NIBBLE_COPY_VERT1                  (BUILD_WRITE_CODE_NIBBLE_10(false, true, false) << BUILD_WRITE_CODE_COPY_VERT_SHIFT1)



uint AddWriteIfNotClear(uint writeCode, uint clearCode, uint clearCodeShift)
{
    clearCode = (clearCode >> clearCodeShift) & 1;

    return writeCode * (clearCode ^ 1);
}


uint RemoveClearPixels(uint clearCode, uint writeCode)
{
    [branch]
    if (clearCode)  // should be good coherence on this branch
    {
        writeCode = (clearCode & CLEAR_CODE_00) ? writeCode & ~BUILD_WRITE_CODE_NIBBLE_00_NO_COPY : writeCode;
        writeCode = (clearCode & CLEAR_CODE_01) ? writeCode & ~BUILD_WRITE_CODE_NIBBLE_01_NO_COPY : writeCode;
        writeCode = (clearCode & CLEAR_CODE_10) ? writeCode & ~BUILD_WRITE_CODE_NIBBLE_10_NO_COPY : writeCode;
        writeCode = (clearCode & CLEAR_CODE_11) ? writeCode & ~BUILD_WRITE_CODE_NIBBLE_11_NO_COPY : writeCode;
    }
    return writeCode;
}


uint RemoveOffRTPixels(uint writeCode, uint2 pixelCoord)
{
    // remove pixels which are off the edges of the render target            
    writeCode = any(pixelCoord + uint2(0, 0) >= renderTargetDims.xy) ? writeCode & ~BUILD_WRITE_CODE_NIBBLE_00_NO_COPY : writeCode;
    writeCode = any(pixelCoord + uint2(0, 1) >= renderTargetDims.xy) ? writeCode & ~BUILD_WRITE_CODE_NIBBLE_01_NO_COPY : writeCode;
    writeCode = any(pixelCoord + uint2(1, 0) >= renderTargetDims.xy) ? writeCode & ~BUILD_WRITE_CODE_NIBBLE_10_NO_COPY : writeCode;
    writeCode = any(pixelCoord + uint2(1, 1) >= renderTargetDims.xy) ? writeCode & ~BUILD_WRITE_CODE_NIBBLE_11_NO_COPY : writeCode;

    return writeCode;
}


void WriteSparseLightingCoord(bool condition, uint writeCode, uint depth21bits, uint baseOffset, uint2 localCoord)
{
    uint index = WavePrefixCountBits(condition);

    [branch]
    if (condition)
    {
        SparseLightingCoordsUAV[baseOffset + index] = BuildSparseData(localCoord, writeCode & BUILD_WRITE_CODE_COPY_BITS_MASK, depth21bits);
    }
}


uint WriteSparseCoordinates(uint writeCode, uint2 Gid, uint2 tileLocalCoord, uint depth21bits00, uint depth21bits01, uint depth21bits10, uint depth21bits11)
{
    uint pixelsToLightCount;

    uint writeCode00 = GET_WRITE_CODE_NIBBLE_00(writeCode);
    uint writeCode01 = GET_WRITE_CODE_NIBBLE_01(writeCode);
    uint writeCode10 = GET_WRITE_CODE_NIBBLE_10(writeCode);
    uint writeCode11 = GET_WRITE_CODE_NIBBLE_11(writeCode);

    bool write00 = (writeCode00 & BUILD_WRITE_CODE_WRITE_PIXEL) ? true : false;
    bool write01 = (writeCode01 & BUILD_WRITE_CODE_WRITE_PIXEL) ? true : false;
    bool write10 = (writeCode10 & BUILD_WRITE_CODE_WRITE_PIXEL) ? true : false;
    bool write11 = (writeCode11 & BUILD_WRITE_CODE_WRITE_PIXEL) ? true : false;

    uint pixelsWritten00 = WaveActiveCountBits(write00);
    uint pixelsWritten01 = WaveActiveCountBits(write01);
    uint pixelsWritten10 = WaveActiveCountBits(write10);
    pixelsToLightCount   = WaveActiveCountBits(write11);
    pixelsToLightCount  += pixelsWritten00 + pixelsWritten01 + pixelsWritten10;
    
    [branch]        // scalar
    if (pixelsToLightCount > (256 - 32))
    {
        // we're not saving any processing as all wave32's are needed (none can early out)
        // no need to write coordinates, light every pixel
        pixelsToLightCount = 255;       // uses as R8, so write 255 instead of 256
    }
    else
    {
        uint baseOffset = TileIDToLinearIndexScalar(Gid.xy);
    
        WriteSparseLightingCoord(write00, writeCode00, depth21bits00, baseOffset, tileLocalCoord + uint2(0, 0)); baseOffset += pixelsWritten00;
        WriteSparseLightingCoord(write01, writeCode01, depth21bits01, baseOffset, tileLocalCoord + uint2(0, 1)); baseOffset += pixelsWritten01;
        WriteSparseLightingCoord(write10, writeCode10, depth21bits10, baseOffset, tileLocalCoord + uint2(1, 0)); baseOffset += pixelsWritten10;
        WriteSparseLightingCoord(write11, writeCode11, depth21bits11, baseOffset, tileLocalCoord + uint2(1, 1));
    }
    return pixelsToLightCount;
}


// input equals is coverage values
uint BuildWriteCode(inout bool removeClearPixels, uint clearCode, uint shadingRateX, uint shadingRateY, bool equalHoriz0, bool equalHoriz1, bool equalVert0, bool equalVert1)
{
    removeClearPixels = false;

    equalHoriz0 = equalHoriz0 && shadingRateX;
    equalHoriz1 = equalHoriz1 && shadingRateX;
    equalVert0  = equalVert0  && shadingRateY;
    equalVert1  = equalVert1  && shadingRateY;

    [branch]
    if (clearCode)
    {
        // This could be avoided if title guarentees that equality from coverage bits will not pass for clear pixels
        // in my case, I'm using 4 bits for coverage, so there's no spare bit pattern here for clear / not clear, so this test is needed
        // Also could be avoided for any tiles where we know there are no clear pixels, which would be epecially nice since this test would be scalar
        equalHoriz0 = equalHoriz0 && (0 == (clearCode & (CLEAR_CODE_00 | CLEAR_CODE_10)));
        equalHoriz1 = equalHoriz1 && (0 == (clearCode & (CLEAR_CODE_01 | CLEAR_CODE_11)));
        equalVert0  = equalVert0  && (0 == (clearCode & (CLEAR_CODE_00 | CLEAR_CODE_01)));
        equalVert1  = equalVert1  && (0 == (clearCode & (CLEAR_CODE_10 | CLEAR_CODE_11)));
    }                    
    if (equalHoriz0 && equalHoriz1 && equalVert0)
    {
        // Hooray! 2x2 rate and all four pixels have same coverage, and are not clear
        return BUILD_WRITE_CODE_NIBBLE_COPY_ALL;
    }
    if (equalHoriz0 && equalHoriz1)
    {
        // 2x1 rate, with both rows having same coverage, and are not clear
        return BUILD_WRITE_CODE_NIBBLE_COPY_HORIZ0 | BUILD_WRITE_CODE_NIBBLE_COPY_HORIZ1;
    }
    if (equalVert0 && equalVert1)
    {
        // 1x2 rate, with both colums having same coverage, and are not clear
        return BUILD_WRITE_CODE_NIBBLE_COPY_VERT0 | BUILD_WRITE_CODE_NIBBLE_COPY_VERT1;
    }
    if (!(equalHoriz0 || equalHoriz1 || equalVert0 || equalVert1))
    {
        // have to light all non-clear pixels
        removeClearPixels = true;       // using a boolean rather than calling RemoveClearPixels here ensures there's only one copy of RemoveClearPixels in the compiled shader

        return BUILD_WRITE_CODE_LIGHT_ALL4_PIXELS;
    }
    // now we're into the complex coverage cases, 2x2, 1x2, 2x1, 1x1 fully covered have all been handled
    // but at least two pixels are equal, so there is a saving to be had yet
    // 
    // Key:
    //  l is the lit pixel performing copies
    //  = is a pixel copy
    //  X is a pixel that must be uniquely lit (boo!), or is clear
    //
    // The below code does not rotate which pixels are lit in the case where the period is 3 frames, which is much harder to do
    // Instead it is arranged not to copy on the diagonal (though this would be possible), because the distance between pixels centers and therefore the error is less with horizontally or vertically adjacent pixels
    if (equalHoriz0)
    {
        if (equalVert0)
        {
            // l=                                                                                                                                              
            // =X                                                                                                                                              
            return SPARSE_LIGHTING_L_COPY_CODE_TL | AddWriteIfNotClear(BUILD_WRITE_CODE_NIBBLE_11_NO_COPY, clearCode, CLEAR_CODE_SHIFT_11);
        }                                                                                                                                                      
        if (equalVert1)                                                                                                                                        
        {    
            // =l
            // X=
            return SPARSE_LIGHTING_L_COPY_CODE_TR | AddWriteIfNotClear(BUILD_WRITE_CODE_NIBBLE_01_NO_COPY, clearCode, CLEAR_CODE_SHIFT_01);
        }
        // l=
        // XX
        return BUILD_WRITE_CODE_NIBBLE_COPY_HORIZ0 |
            AddWriteIfNotClear(BUILD_WRITE_CODE_NIBBLE_01_NO_COPY, clearCode, CLEAR_CODE_SHIFT_01) |
            AddWriteIfNotClear(BUILD_WRITE_CODE_NIBBLE_11_NO_COPY, clearCode, CLEAR_CODE_SHIFT_11);
    }
    if (equalHoriz1)
    {
        if (equalVert0)
        {
            // =X                                                                                                                                              
            // l=                                                                                                                                              
            return SPARSE_LIGHTING_L_COPY_CODE_BL | AddWriteIfNotClear(BUILD_WRITE_CODE_NIBBLE_10_NO_COPY, clearCode, CLEAR_CODE_SHIFT_10);
        }                                                                                                                                                      
        if (equalVert1)                                                                                                                                        
        {   
            // X=
            // =l
            return SPARSE_LIGHTING_L_COPY_CODE_BR | AddWriteIfNotClear(BUILD_WRITE_CODE_NIBBLE_00_NO_COPY, clearCode, CLEAR_CODE_SHIFT_00);
        }
        // XX
        // l=
        return BUILD_WRITE_CODE_NIBBLE_COPY_HORIZ1 |
            AddWriteIfNotClear(BUILD_WRITE_CODE_NIBBLE_00_NO_COPY, clearCode, CLEAR_CODE_SHIFT_00) |
            AddWriteIfNotClear(BUILD_WRITE_CODE_NIBBLE_10_NO_COPY, clearCode, CLEAR_CODE_SHIFT_10);
    }    
    if (equalVert0)
    {
        // lX
        // =X
        return BUILD_WRITE_CODE_NIBBLE_COPY_VERT0 |
            AddWriteIfNotClear(BUILD_WRITE_CODE_NIBBLE_10_NO_COPY, clearCode, CLEAR_CODE_SHIFT_10) |
            AddWriteIfNotClear(BUILD_WRITE_CODE_NIBBLE_11_NO_COPY, clearCode, CLEAR_CODE_SHIFT_11);
    }
    else // equalVert1 must be true!
    {
        // Xl
        // X=
        return BUILD_WRITE_CODE_NIBBLE_COPY_VERT1 |
            AddWriteIfNotClear(BUILD_WRITE_CODE_NIBBLE_00_NO_COPY, clearCode, CLEAR_CODE_SHIFT_00) |
            AddWriteIfNotClear(BUILD_WRITE_CODE_NIBBLE_01_NO_COPY, clearCode, CLEAR_CODE_SHIFT_01);
    }
}


uint Build21BitDepth(float linearZ)
{
#if SPARSE_LIGHTING_USE_21BIT_UNORM_PACKED_DEPTH == 1
    // 21-bit linearZ, +0.5 makes this a round nearest which has better average error
    const uint maxZ = float((1U << 21) - 1);
    return min(uint(linearZ * maxZ + 0.5), maxZ);
#else
    return 0;
#endif
}
