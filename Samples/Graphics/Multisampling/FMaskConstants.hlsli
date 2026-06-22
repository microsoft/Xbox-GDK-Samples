//--------------------------------------------------------------------------------------
// FMaskConstants.hlsli
//
// Constants for interpreting fmask
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef FMASKCONSTANTS_INCLUDED
#define FMASKCONSTANTS_INCLUDED

#if !ENABLE_EQAA
#error Must have EQAA_ENABLED defined in order to access FMask
#endif

// These must be kept in sync with the numbers in EQAAConstants.h
#define MAX_NUM_FRAGMENTS 8
#define MAX_LOG_NUM_FRAGMENTS 3
#define MAX_NUM_SAMPLES 16 
#define MAX_LOG_NUM_SAMPLES 4 
#define MAX_QUALITY MAX_LOG_NUM_SAMPLES 

// This code supports either a single ubershader which handles all MSAA settings, 
// or else specializations for each Count and Quality.  
// To create an ubershader, set a constant buffer matching cbFMask below.
// To create specialized shaders, compile with LOG_NUM_FRAGMENTS and QUALITY 
// and COLOR_EXPANDED #defined.  
#if defined(LOG_NUM_FRAGMENTS) && defined(QUALITY) && defined(COLOR_EXPANDED)

// A sample is a sub-pixel location.  
// A fragment is a sub-pixel color. 
// We can have more than one sample assigned to a fragment.
#if QUALITY < LOG_NUM_FRAGMENTS
#define LOG_NUM_SAMPLES LOG_NUM_FRAGMENTS
#else 
#define LOG_NUM_SAMPLES QUALITY
#endif

#define NUM_FRAGMENTS ( 1U << LOG_NUM_FRAGMENTS )
#define NUM_SAMPLES ( 1U << LOG_NUM_SAMPLES )

// An fmask index must be able to encode any fragment.
// Also, if there are more samples than fragments, we require a code for "unknown" color.
// In practice, the hardware uses power-of-two index sizes.
#if LOG_NUM_SAMPLES > LOG_NUM_FRAGMENTS
#define MIN_INDEX_BITS (LOG_NUM_FRAGMENTS + 1)
#else
#define MIN_INDEX_BITS LOG_NUM_FRAGMENTS
#endif

#if MIN_INDEX_BITS <= 2 && !defined( NATIVE_FMASK )
#define INDEX_BITS MIN_INDEX_BITS
#else
#define INDEX_BITS 4
#endif

// Anything which hits outside the VALID_INDEX_MASK is "unknown".  Our choice how
// to handle these.  We simply omit them, and scale the remaining colors.
#define INDEX_MASK ( ( 1U << INDEX_BITS ) - 1 )
#define VALID_INDEX_MASK ( NUM_FRAGMENTS - 1 )
#define INVALID_INDEX_MASK ~VALID_INDEX_MASK

#else

#define UBERSHADER

cbuffer fmask : register(b1)
{
    bool COLOR_EXPANDED;
    uint LOG_NUM_FRAGMENTS;
    uint LOG_NUM_SAMPLES;
    uint NUM_FRAGMENTS;
    uint NUM_SAMPLES;
    uint INDEX_BITS;
    uint INDEX_MASK;
    uint VALID_INDEX_MASK;
    uint INVALID_INDEX_MASK;
    uint QUALITY;
};

#endif
#endif

