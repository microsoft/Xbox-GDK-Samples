// Created by reinsteam / Dec 2017
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// enables HLSL portability
#define USE_HLSL 1

// defines entry point 'vec4 callMainImage(vec2 fragCoord, vec2 resolution, vec2 mouse);' to call from external shader
#define USE_EXTERNAL_ENTRY 1

#define ATM_PRESET 4
#if ATM_PRESET == 0
#   define ATM_INTEGRATION_TYPE 1
#   define ATM_NUM_SCATTERING_STEPS 16u
#elif ATM_PRESET == 1
#   define ATM_INTEGRATION_TYPE 0
#   define ATM_NUM_SCATTERING_STEPS 8u
#elif ATM_PRESET == 2
#   define ATM_INTEGRATION_TYPE 0
#   define ATM_NUM_SCATTERING_STEPS 16u
#elif ATM_PRESET == 3
#   define ATM_INTEGRATION_TYPE 1
#   define ATM_NUM_SCATTERING_STEPS 32u
#elif ATM_PRESET == 4
#   define ATM_INTEGRATION_TYPE 1
#   define ATM_NUM_SCATTERING_STEPS 64u
#endif

// 1 - Display transmittance table (mie+rayleigh, ozone only, all) / 0 - Display scattering
static uint ATM_SHOW_TRANSMITTANCE; // default: 0

// 1 - Enable / 0 - Disable ozone absorption
#define ATM_OZONE_ABSORPTION 1

// Projection modes:
//     1 - view space
//     2 - equirectangular
//     3 - fisheye
static uint ATM_PROJECTION_MODE; //< default: 2;

// Tonemapping modes:
//     0 - John Hable's Uncharted2 tonemapper
//     1 - John Hable's Filmic ALU tonemapper
static uint ATM_TONEMAPPING_MODE; // default: 0

// Scattering integration type:
//     0 - Standard integration
//     1 - Sebastien Hillaire's integration: https://www.shadertoy.com/view/XlBSRz
#ifndef ATM_INTEGRATION_TYPE
#define ATM_INTEGRATION_TYPE 0
#endif

// Options how view direction below horizon are handled
//     0 - Use simply black color
//     1 - Clamp view direction to use color at horizon
#define ATM_CLAMP_BELOW_HORIZON 1

#ifndef ATM_NUM_SCATTERING_STEPS
#define ATM_NUM_SCATTERING_STEPS 8u
#endif

#define ATM_NUM_TRANSMITTANCE_STEPS 16u

//------------------------------------------------------------------------------
#if USE_HLSL
    #if USE_MANUAL_PIXEL_PACKING
        #if USE_16BIT_TYPES
            typedef half FpTypeBase;
            typedef half2 FpTypeScalar;
            typedef half2 FpType;
            typedef half2x2 FpType2;
            typedef half3x2 FpType3;
            typedef half4x2 FpType4;

        #else
            typedef float FpTypeBase;
            typedef float2 FpTypeScalar;
            typedef float2 FpType;
            typedef float2x2 FpType2;
            typedef float3x2 FpType3;
            typedef float4x2 FpType4;
        #endif
        FpType Predicate(bool2 cond)
        {
            return FpType(cond.x ? 1.0 : 0.0,
                        cond.y ? 1.0 : 0.0);
        }
        #if 0
        FpType Select(bool2 cond, FpType x, FpType y)
        {
            uint mask0 = __XB_MakeUniform(__XB_Ballot64(cond.x).x);
            uint mask1 = __XB_MakeUniform(__XB_Ballot64(cond.y).x);
            FpType r;
            __asm__("v_cndmask_b32 %[rv0], %[v0], %[v1], %[s0] dst_sel:WORD_0 src1_sel:WORD_0 src0_sel:WORD_0\n"
                    : [rv0] "=v" (r.xy)
                    : [v0] "v" (y.xy),
                      [v1] "v" (x.xy),
                      [s0] "s" (mask0)
                    : );
            __asm__("v_cndmask_b32 %[rv0], %[v0], %[v1], %[s0] dst_sel:WORD_1 src1_sel:WORD_1 src0_sel:WORD_1\n"
                    : [rv0] "=v" (r.xy)
                    : [v0] "v" (y.xy),
                      [v1] "v" (x.xy),
                      [s0] "s" (mask1)
                    : );
            return r;
        }
        #else
        FpType Select(bool2 cond, FpType x, FpType y)
        {
            return FpType(cond.x ? x.x : y.x,
                          cond.y ? x.y : y.y);
        }
        #endif
        FpType3 Select(bool2 cond, FpType3 x, FpType3 y)
        {
            return FpType3(Select(cond, x[0], y[0]), Select(cond, x[1], y[1]), Select(cond, x[2], y[2]));
        }
        FpType3 Select(bool3x2 cond, FpType3 x, FpType3 y)
        {
            return FpType3(Select(cond[0], x[0], y[0]), Select(cond[1], x[1], y[1]), Select(cond[2], x[2], y[2]));
        }
    #else
        #if USE_16BIT_TYPES
            typedef half FpTypeBase;
            typedef half FpType;
            typedef half2 FpType2;
            typedef half3 FpType3;
            typedef half4 FpType4;
        #else
            typedef float FpTypeBase;
            typedef float FpType;
            typedef float2 FpType2;
            typedef float3 FpType3;
            typedef float4 FpType4;
        #endif
        FpType Predicate(bool cond)
        {
            return cond ? 1.0 : 0.0;
        }
        FpType Select(bool cond, FpType x, FpType y)
        {
            return cond ? x : y;
        }
        FpType3 Select(bool cond, FpType3 x, FpType3 y)
        {
            return FpType3(Select(cond, x[0], y[0]), Select(cond, x[1], y[1]), Select(cond, x[2], y[2]));
        }
        FpType3 Select(bool3 cond, FpType3 x, FpType3 y)
        {
            return FpType3(Select(cond[0], x[0], y[0]), Select(cond[1], x[1], y[1]), Select(cond[2], x[2], y[2]));
        }
    #endif

    #define fract frac
    #define DEFINE_CONST(type, name, value) static const type name = value

    FpType3 Mul(FpType3 x, FpType y)
    {
        [unroll] for (int i = 0; i < 3; ++i)
        {
            x[i] *= y;
        }
        return x;
    }

    static FpType MakeFpType(FpTypeBase x) { return x; }
    static FpType2 MakeFpType2(FpTypeBase x) { return x; }
    static FpType3 MakeFpType3(FpTypeBase x) { return x; }
    static FpType4 MakeFpType4(FpTypeBase x) { return x; }

    static FpType3 MakeFpType3(FpTypeBase x, FpTypeBase y, FpTypeBase z) { return FpType3(MakeFpType(x), MakeFpType(y), MakeFpType(z)); }
    static FpType4 MakeFpType4(FpTypeBase x, FpTypeBase y, FpTypeBase z, FpTypeBase w) { return FpType4(MakeFpType(x), MakeFpType(y), MakeFpType(z), MakeFpType(w)); }

#if USE_MANUAL_PIXEL_PACKING
    static FpType3 MakeFpType3(FpType x) { return FpType3(x, x, x); }
#endif

#else
    #define DEFINE_CONST(type, name, value) const type name = value
    #define FpType float
    #define FpType2 vec2
    #define FpType3 vec3
    #define FpType4 vec4
#endif

#if USE_MANUAL_PIXEL_PACKING
typedef float2 FpTypeMax;
typedef float2x2 FpTypeMax2;
typedef float3x2 FpTypeMax3;
typedef float4x2 FpTypeMax4;
#else
typedef float FpTypeMax;
typedef float2 FpTypeMax2;
typedef float3 FpTypeMax3;
typedef float4 FpTypeMax4;
#endif

#if USE_EXTERNAL_ENTRY
    static float2 iResolution;
    static float2 iMouse;

    void mainImage(out FpType4 fragColor, in FpTypeMax2 fragCoord);
    FpType4 callMainImage(FpTypeMax2 fragCoord, float2 resolution, float2 mouse)
    {
        FpType4 Color;
        iResolution = resolution;
        iMouse = mouse;
        mainImage(Color, fragCoord);
        return Color;
    }
#endif
//------------------------------------------------------------------------------

DEFINE_CONST(float, kAtmRadiusMin, 6360.0);
DEFINE_CONST(float, kAtmRadiusMax, 6420.0);
DEFINE_CONST(float, kKilometersToMeters, 1000.0);

DEFINE_CONST(FpTypeBase, kAtmRayHeightScale, 1.0 / 8.0);
DEFINE_CONST(FpTypeBase, kAtmMieHeightScale, 1.0 / 1.2);

// http://www.iup.physik.uni-bremen.de/gruppen/molspec/databases/referencespectra/o3spectra2011/index.html
// Version 22.07.2013: Fast Fourier Transform Filter applied to the initial data in the region 213.33 -317 nm
DEFINE_CONST(FpTypeBase, kAtmOznCrossSection_293K_680nm, 1.36820899679147); // * 10^-25 m^2 * molecule^-1
DEFINE_CONST(FpTypeBase, kAtmOznCrossSection_293K_550nm, 3.31405330400124); // * 10^-25 m^2 * molecule^-1
DEFINE_CONST(FpTypeBase, kAtmOznCrossSection_293K_440nm, 0.13601728252538); // * 10^-25 m^2 * molecule^-1

// https://en.wikipedia.org/wiki/Number_density
DEFINE_CONST(FpTypeBase, kAtmAirNumberDensity_293K, 2.504); // * 10^25 molecule * m^-3

// Choose 6 parts per million which is within 15 parts per million reported here:
// https://ozonewatch.gsfc.nasa.gov/facts/ozone.html
DEFINE_CONST(FpTypeBase, kAtmOznConcentration, 6.0e-6);

// Compute ozone absorption coefficients
DEFINE_CONST(FpTypeBase, kAtmOznNumberDensity_293K, kAtmAirNumberDensity_293K * kAtmOznConcentration);
DEFINE_CONST(FpTypeBase, kAtmOznAbsorption_293K_680nm, kAtmOznCrossSection_293K_680nm * kAtmOznNumberDensity_293K);
DEFINE_CONST(FpTypeBase, kAtmOznAbsorption_293K_550nm, kAtmOznCrossSection_293K_550nm * kAtmOznNumberDensity_293K);
DEFINE_CONST(FpTypeBase, kAtmOznAbsorption_293K_440nm, kAtmOznCrossSection_293K_440nm * kAtmOznNumberDensity_293K);

//
// Precomputed Rayleigh scattering coefficients for wavelength L using the following formula :
// ScatteringCoeff(L) = (8.0*pi/3.0) * (n^2.0 - 1.0)^2.0 * ((6.0+3.0*p) / (6.0-7.0*p)) / (L^4.0 * N)
// where
// n - refractive index of the air (1.0003) https://en.wikipedia.org/wiki/Refractive_index
// p - air depolarization factor (0.035)
// N - air number density under NTP : (2.504 * 10^25 molecule * m^-3)
// L - wavelength for which scattering coefficient is computed
//
// See "Rayleigh-scattering calculations for the terrestrial atmosphere" by A.Bucholtz for reference
//
DEFINE_CONST(FpTypeBase, kAtmRayScattering_680nm,  5.8e-6);
DEFINE_CONST(FpTypeBase, kAtmRayScattering_550nm, 13.6e-6);
DEFINE_CONST(FpTypeBase, kAtmRayScattering_440nm, 33.1e-6);

DEFINE_CONST(FpTypeBase, kAtmMieScattering, 2.0e-6);
DEFINE_CONST(FpTypeBase, kAtmMieExtinction, kAtmMieScattering * 1.11);

// cosine of horizon angle at top of the atmosphere
DEFINE_CONST(FpTypeBase, kAtmHorizonSinMin, kAtmRadiusMin / kAtmRadiusMax);
DEFINE_CONST(FpTypeBase, kAtmHorizonCosMin, -sqrt(max(1.0 - kAtmHorizonSinMin * kAtmHorizonSinMin, 0.0)));

// Luminance recovered from "Reference Solar Spectral Irradiance: ASTM G-173"
// (http://rredc.nrel.gov/solar/spectra/am1.5/ASTMG173/ASTMG173.html)
// Scaled by exp2(-16.0);
DEFINE_CONST(FpTypeBase, kAtmRadianceResponseToRGB_Lum, 2.225477);


// Coefficients to convert 3 spectrum samples to linear sRGB
// computed using formula 2 from Eric Bruneton's paper
// "A Qualitative and Quantitative Evaluation of 8 Clear Sky Models"
// Actual values are scaled by exp2(-16.0);
DEFINE_CONST(FpTypeBase, kAtmRadianceResponseToRGB_680nm, 2.795308);
DEFINE_CONST(FpTypeBase, kAtmRadianceResponseToRGB_550nm, 2.345072);
DEFINE_CONST(FpTypeBase, kAtmRadianceResponseToRGB_440nm, 2.079126);

DEFINE_CONST(float, kPi, 3.14159265359);
DEFINE_CONST(FpTypeBase, kLog2Exp, 1.44269504089);
DEFINE_CONST(FpTypeBase, kOneOver4Pi, 1.0 / (4.0 * kPi));

FpTypeMax3 MulF(FpTypeMax3 x, FpTypeMax y)
    {
        [unroll] for (int i = 0; i < 3; ++i)
        {
            x[i] *= y;
        }
        return x;
    }

template<typename FP> FP SqrtSafe(FP x)
{
    return sqrt(max(0.0, x));
}


FpType phaseR(FpType VoL)
{
    return kOneOver4Pi * 0.75 * (1.0 + VoL * VoL);
}

FpType phaseM_HG(FpType VoL, FpTypeBase G)
{
    FpType A = max(0.0, 1.0 + G * (G - 2.0 * VoL));
    FpType D = 1.0 / SqrtSafe(A * A * A);
    return (1.0 - G * G) * kOneOver4Pi * D;
}

FpType phaseM_CS(FpType VoL, FpTypeBase G)
{
    return 1.5 * (1.0 + VoL * VoL) * phaseM_HG(VoL, G) / (2.0 + G * G);
}

#if USE_MANUAL_PIXEL_PACKING
FpTypeBase AtmHorizonCos(FpTypeBase R)
{
    FpTypeBase SinH = FpTypeBase(kAtmRadiusMin) / R;
    FpTypeBase CosH = -SqrtSafe(1.0 - SinH * SinH);
    return CosH;
}
#endif

FpType AtmHorizonCos(FpType R)
{
    FpType SinH = FpTypeBase(kAtmRadiusMin) / R;
    FpType CosH = -SqrtSafe(1.0 - SinH * SinH);
    return CosH;
}

FpTypeMax AtmHorizonCosF(FpTypeMax R)
{
    FpTypeMax SinH = kAtmRadiusMin / R;
    FpTypeMax CosH = -SqrtSafe(1.0 - SinH * SinH);
    return CosH;
}

FpType AtmIntersectTop(FpTypeMax R, FpTypeMax V)
{
    FpType RMaxOverR = FpType(kAtmRadiusMax / R);
    return FpType(-V + sqrt((V * V - 1.0) + FpTypeMax(RMaxOverR * RMaxOverR)));
}


#if USE_MANUAL_PIXEL_PACKING && USE_16BIT_TYPES

static FpTypeMax RiOverR_From_TiOverR(FpTypeMax V, FpType TiOverR)
{
    FpTypeMax r0, r1;
    // op_sel_hi: 0 - means an argument is 32-bit value, 1 means 16-bit value
    // op_sel: 0 - means fetch lower 16-bit from VGPR, 1 - means fetch higher 16-bit from VGPRs
    __asm__(
        "v_fma_mix_f32 %[rv0], %[v0], 2.0, %[v1] op_sel_hi:[0,1,1] \n"
        : [rv0] "=v" (r0.x)
        : [v0] "v" (V.x),
          [v1] "v" (TiOverR.xy)
        : );
    __asm__(
        "v_fma_mix_f32 %[rv0], %[v0], 2.0, %[v1] op_sel:[0,0,1] op_sel_hi:[0,1,1] \n"
        : [rv0] "=v" (r0.y)
        : [v0] "v" (V.y),
          [v1] "v" (TiOverR.xy)
        : );
    __asm__(
        "v_fma_mix_f32 %[rv0], %[v0], %[v1], 1.0 op_sel_hi:[0,1,1] \n"
        : [rv0] "=v" (r1.x)
        : [v0] "v" (r0.x),
          [v1] "v" (TiOverR.xy)
        : );
    __asm__(
        "v_fma_mix_f32 %[rv0], %[v0], %[v1], 1.0 op_sel:[0,1,0] op_sel_hi:[0,1,1] \n"
        : [rv0] "=v" (r1.y)
        : [v0] "v" (r0.y),
          [v1] "v" (TiOverR.xy)
        : );
    FpTypeMax RiOverR = SqrtSafe(r1);
    return RiOverR;
}

#else

static FpTypeMax RiOverR_From_TiOverR(FpTypeMax V, FpType TiOverR)
{
    FpTypeMax RiOverR = SqrtSafe((TiOverR + 2.0 * V) * TiOverR + 1.0);
    return RiOverR;
}
#endif

#if 0 //USE_MANUAL_PIXEL_PACKING && USE_16BIT_TYPES

static FpType Hi_From_RiOverR(FpTypeMax R, FpTypeMax RiOverR)
{
    uint r;
    __asm__(
        "v_fma_mixlo_f16 %[rv0], %[v0], %[v1], -%[v2] op_sel_hi:[0,0,0] \n"
        : [rv0] "=v" (r)
        : [v0] "v" (RiOverR.x),
          [v1] "v" (R.x),
          [v2] "v" (kAtmRadiusMin)
        : );
    __asm__(
        "v_fma_mixhi_f16 %[rv0], %[v0], %[v1], -%[v2] op_sel_hi:[0,0,0] \n"
        : [rv0] "=v" (r)
        : [v0] "v" (RiOverR.y),
          [v1] "v" (R.y),
          [v2] "v" (kAtmRadiusMin)
        : );
    return __XB_AsHalf(r);
}

#else
static FpType Hi_From_RiOverR(FpTypeMax R, FpTypeMax RiOverR)
{
    return FpType(RiOverR * R - kAtmRadiusMin);
}
#endif

FpType3 AtmGetMediumDensity(FpType H)
{
    // Standard Rayleigh / Mie density profiles
    FpType RayDensity = exp2(-H * (kAtmRayHeightScale * kLog2Exp));
    FpType MieDensity = exp2(-H * (kAtmMieHeightScale * kLog2Exp));

    // Piecewise linear approximation of the ozone profile from (Page 10) :
    // ftp://es-ee.tor.ec.gc.ca/pub/ftpcm/!%20for%20Jacob/Introduction%20to%20atmospheric%20chemistry.pdf
    // Density linearly increases from 0 at 15Km to 1.0 at 25Km and decreases back to 0.0 at 40.0Km
    FpType OznDensity = Select(H < 25.0, saturate(+H / 15.0 - 2.0 / 3.0)
                                       , saturate(-H / 15.0 + 8.0 / 3.0));

    return FpType3(RayDensity, MieDensity, OznDensity * RayDensity);
}

// V - cosine of the angle between view direction and zenith
// R - radius at starting point
FpType3 OpticalLengthStep(FpTypeMax R, FpTypeMax V, FpType TiOverR)
{
    // Re-compute radius at distance Ti using cosine theorem
    FpTypeMax RiOverR = RiOverR_From_TiOverR(V, TiOverR);
    return AtmGetMediumDensity(Hi_From_RiOverR(R, RiOverR));
}

FpType3 OpticalLength(FpTypeMax R, FpType V, uint NumSteps)
{
#if 0
    // Early our with infinite to optical length below horizon to make transmittance -> 0.0
    if (V <= AtmHorizonCosF(R))
    {
        return MakeFpType3(1.0e9);
    }
#endif
    FpType MaxDistance = AtmIntersectTop(R, V);
    FpType StpDistance = MaxDistance * (1.0 / FpTypeBase(NumSteps));

    FpType3 OptLen = 0.0;

    OptLen += OpticalLengthStep(R, V, 0.0);
    OptLen += OpticalLengthStep(R, V, MaxDistance);
    OptLen *= 0.5;

    [loop]for (FpTypeBase iStep = 1.0; iStep < FpTypeBase(NumSteps); iStep += 1.0)
    {
        OptLen += OpticalLengthStep(R, V, iStep * StpDistance);
    }
    return Mul(OptLen, FpType(StpDistance * R));
}

FpType3 OznAbsorption()
{
#if ATM_OZONE_ABSORPTION
    FpType3 Absorption = MakeFpType3(
        kAtmOznAbsorption_293K_680nm * kKilometersToMeters,
        kAtmOznAbsorption_293K_550nm * kKilometersToMeters,
        kAtmOznAbsorption_293K_440nm * kKilometersToMeters
    );
    return Absorption;
#else
    return FpType3(0.0);
#endif
}

FpType3 RayScattering()
{
    FpType3 Scattering = MakeFpType3(
        kAtmRayScattering_680nm * kKilometersToMeters,
        kAtmRayScattering_550nm * kKilometersToMeters,
        kAtmRayScattering_440nm * kKilometersToMeters
    );
    return Scattering;
}

FpType3 MieExtinction()
{
    return MakeFpType3(kAtmMieExtinction * kKilometersToMeters);
}

FpType3 MieScattering()
{
    return MakeFpType3(kAtmMieScattering * kKilometersToMeters);
}

FpType3 Transmittance(FpType3 OptLen, FpType UseOzoneAbsorption)
{
    FpType3 OznOptDepth = Mul(OznAbsorption(), OptLen[2] * UseOzoneAbsorption);
    FpType3 RayOptDepth = Mul(RayScattering(), OptLen[0]);
    FpType3 MieOptDepth = Mul(MieExtinction(), OptLen[1]);
    return exp(-(RayOptDepth + MieOptDepth + OznOptDepth));
}

// Parameters for ScatteringStep / Scattering functions
// R        - radius at starting point
// V        - cosine of the angle between view direction and zenith
// L        - cosine of the angle between direction to the light source and zenith
// VoL      - cosine of the angle between view direction and direction to the light source
// Di       - current distance in direction defined by V
// OptLenV  - optical length from a starting to the atmosphere's top
// UseOzoneAbsorption - 1.0 or 0.0 to enable or disbale ozone absorption
void ScatteringStep(FpTypeMax R, FpTypeMax V, FpType L, FpTypeMax VoL, FpType TiOverR, FpType3 OptLenV, FpType UseOzoneAbsorption, out FpType3 RayS, out FpType3 MieS)
{
    FpTypeMax RiOverR = RiOverR_From_TiOverR(V, TiOverR);
    FpType ROverRi = FpType(1.0 / RiOverR);
    FpType Vi = (FpType(V) + TiOverR) * ROverRi;
    FpType Li = (L + TiOverR * L) * ROverRi;
    FpTypeMax Ri = RiOverR * R;
    //if (Li > AtmHorizonCos(Ri))
    FpType LiVisible = Predicate(Li > AtmHorizonCosF(Ri));
    {
        // Optical length from a current point to the atmoshere bound
        // in view direction and in direction to the light source
        FpType3 OptLenVi = OpticalLength(Ri, Vi, ATM_NUM_TRANSMITTANCE_STEPS);
        FpType3 OptLenLi = OpticalLength(Ri, Li, ATM_NUM_TRANSMITTANCE_STEPS);

        // Compute total optical length of the path and compute transmittance from it
        FpType3 Ti = Transmittance(OptLenV - OptLenVi + OptLenLi, UseOzoneAbsorption);

        FpType Hi = FpType(Ri - kAtmRadiusMin);

        // Multiply by corresponding particle density
        RayS = Mul(Ti, LiVisible * exp(-Hi * kAtmRayHeightScale));
        MieS = Mul(Ti, LiVisible * exp(-Hi * kAtmMieHeightScale));
    }
}

FpType3 AtmLimbDarkening(FpTypeMax VoL)
{
    const FpTypeMax kSunAngularRadius32Min33Sec = 0.00473420559;
    const FpTypeMax kSunAngularRadius32Min33SecInv = 1.0 / kSunAngularRadius32Min33Sec;

    // NOTE: This multiply and addition requires 32-bit precision
    FpType ToEdge = FpType(saturate(kSunAngularRadius32Min33SecInv - VoL * kSunAngularRadius32Min33SecInv));

    FpType CosAngle1 = cos(ToEdge * FpTypeBase(kPi * 0.5));
    FpType CosAngle4 = SqrtSafe(1.0 - ToEdge * ToEdge);

    #if 0
        FpType Mask = 1.0 - Select(ToEdge == 1.0, 1.0, 0.0);
    #else
        FpType Mask = 1.0 - Select(ToEdge >= 0.8, smoothstep(0.8, 1.0, ToEdge), MakeFpType(0.0));
    #endif

    // Limb Darkening model from http://www.physics.hmc.edu/faculty/esin/a101/limbdarkening.pdf
    // See Formula 1.
    // Coefficients for wavelengths close to (680 550 440) nm are from Table 2 (PS column)
    return Mul(pow(MakeFpType3(1.0 - 1.0 * (1.0 - CosAngle1)), MakeFpType3(0.420, 0.503, 0.652)), Mask);
}

void Scattering(FpTypeMax R, FpTypeMax V, FpType L, FpTypeMax VoL, FpType UseOzoneAbsorption, out FpType3 Scattering)
{
    FpType3 RayS = MakeFpType3(0.0);
    FpType3 MieS = MakeFpType3(0.0);

    FpTypeMax CosH = AtmHorizonCosF(R);

#if ATM_CLAMP_BELOW_HORIZON
    V = max(V, CosH + 1.0 / 256.0);
#else
    if (any(V >= CosH))
#endif
    {
        uint NumSteps = ATM_NUM_SCATTERING_STEPS;
        FpType MaxDistance = AtmIntersectTop(R, V);
        FpType StpDistance = MaxDistance * (1.0 / FpTypeBase(NumSteps));

        FpType3 OptLenV = OpticalLength(R, FpType(V), ATM_NUM_TRANSMITTANCE_STEPS);

        FpType3 RaySi;
        FpType3 MieSi;

        ScatteringStep(R, V, L, VoL, MakeFpType(0.0), OptLenV, UseOzoneAbsorption, RaySi, MieSi);
        RayS = RaySi;
        MieS = RaySi;

        ScatteringStep(R, V, L, VoL, MaxDistance, OptLenV, UseOzoneAbsorption, RaySi, MieSi);
        RayS = (RayS + RaySi) * 0.5;
        MieS = (MieS + MieSi) * 0.5;

        for (uint iStep = 1u; iStep < NumSteps; ++iStep)
        {
            ScatteringStep(R, V, L, VoL, FpTypeBase(iStep) * StpDistance, OptLenV, UseOzoneAbsorption, RaySi, MieSi);
            RayS += RaySi;
            MieS += MieSi;
        }

        RayS *= Mul(RayScattering(), StpDistance * FpType(R) * phaseR(FpType(VoL)));
        MieS *= Mul(MieScattering(), StpDistance * FpType(R) * phaseM_CS(FpType(VoL), 0.76));

        Scattering = (RayS + MieS) + Transmittance(OptLenV, UseOzoneAbsorption) * AtmLimbDarkening(VoL);
    }
}

void ScatteringNew(FpTypeMax R, FpTypeMax V, FpType L, FpTypeMax VoL, FpType UseOzoneAbsorption, out FpType3 Scattering)
{
    FpType3 RayS = MakeFpType3(0.0);
    FpType3 MieS = MakeFpType3(0.0);

    FpTypeMax CosH = AtmHorizonCosF(R);

#if ATM_CLAMP_BELOW_HORIZON
    V = max(V, CosH + 1.0 / 256.0);
#else
    if (any(V >= CosH))
#endif
    {
        uint NumSteps = ATM_NUM_SCATTERING_STEPS;
        FpType MaxDistance = AtmIntersectTop(R, V);
        FpType StpDistance = MaxDistance * (1.0 / FpTypeBase(NumSteps));

        FpType3 Tr = MakeFpType3(1.0);

        for (FpTypeBase iStep = 0.0; iStep < FpTypeBase(NumSteps); iStep += 1.0)
        {
            FpType TiOverR = iStep * StpDistance;
            FpTypeMax RiOverR = RiOverR_From_TiOverR(V, TiOverR);
            FpType ROverRi = FpType(1.0 / RiOverR);
            FpType Vi = (FpType(V) + TiOverR) * ROverRi;
            FpType Li = (L + TiOverR * L) * ROverRi;
            FpTypeMax Ri = RiOverR * R; 

            FpType3 Density = Mul(AtmGetMediumDensity(Hi_From_RiOverR(R, RiOverR)), StpDistance * FpType(R));
            FpType3 RayStepExtinction = Mul(RayScattering(), Density[0]);
            FpType3 MieStepExtinction = Mul(MieExtinction(), Density[1]);
            FpType3 OznStepExtinction = Mul(OznAbsorption(), Density[2] * UseOzoneAbsorption);

            FpType3 RayStepScattering = RayStepExtinction;
            FpType3 MieStepScattering = Mul(MieScattering(), Density[1]);


            FpType3 RayStepAlbedo = MakeFpType3(1.0);
            FpType3 MieStepAlbedo = MieScattering() / MieExtinction();

            FpType3 StepTransmittance = exp(-(RayStepExtinction + MieStepExtinction + OznStepExtinction));

#if 0
            RMType RayMieStepExtinctionR = float2(RayStepExtinction.x, MieStepExtinction.x);
            RMType RayMieStepExtinctionG = float2(RayStepExtinction.y, MieStepExtinction.y);
            RMType RayMieStepExtinctionB = float2(RayStepExtinction.z, MieStepExtinction.z);

            RMType RayMieStepAlbedoR = float2(RayStepAlbedo.x, MieStepAlbedo.x);
            RMType RayMieStepAlbedoG = float2(RayStepAlbedo.y, MieStepAlbedo.y);
            RMType RayMieStepAlbedoB = float2(RayStepAlbedo.z, MieStepAlbedo.z);
#endif

            //if (Li > AtmHorizonCos(Ri))
            FpType LiVisible = Predicate(Li > AtmHorizonCosF(Ri));
            {
                // Opitcal length from a current point to the atmoshere bound
                // in direction to the light source
                FpType3 OptLenLi = OpticalLength(Ri, Li, ATM_NUM_TRANSMITTANCE_STEPS);
                FpType3 TransmittanceToSun = Transmittance(OptLenLi, UseOzoneAbsorption);

                FpType3 FullPathTransmittance = Tr * TransmittanceToSun;
#if 1
                FpType3 FullPathTransmittanceWithAlbedo = Mul(FullPathTransmittance * RayStepAlbedo, LiVisible);
                RayS += FullPathTransmittanceWithAlbedo * FpType3(1.0 - exp2(FpTypeMax3(-RayStepExtinction * kLog2Exp)));
                MieS += FullPathTransmittanceWithAlbedo * FpType3(1.0 - exp2(FpTypeMax3(-MieStepExtinction * kLog2Exp)));

#elif 0
                vec3 RaySL = TransmittanceToSun * (mkvec3(1.0) - exp(-RayStepExtinction)) * RayStepAlbedo;
                vec3 MieSL = TransmittanceToSun * (mkvec3(1.0) - exp(-MieStepExtinction)) * MieStepAlbedo;
#else
                vec3 RaySL = TransmittanceToSun * (mkvec3(1.0) - StepTransmittance) * RayStepAlbedo;
                vec3 MieSL = TransmittanceToSun * (mkvec3(1.0) - StepTransmittance) * MieStepAlbedo;
#endif
            }

            Tr *= StepTransmittance;
        }

        RayS = Mul(RayS, phaseR(FpType(VoL)));
        MieS = Mul(MieS, phaseM_CS(FpType(VoL), 0.76));

        Scattering = (RayS + MieS) + Tr * AtmLimbDarkening(VoL);
    }
}

FpType3 sRGMGamma(FpType3 color)
{
    FpType3 x = color * 12.92;
    FpType3 y = 1.055 * pow(saturate(color), MakeFpType3(1.0 / 2.4)) - 0.055;

    return Select(color < 0.0031308, x, y);
}

// John Hable's tonemapping function from presentation "Uncharted 2 HDR Lighting", Page 142-143
FpType3 ToneMap_Uncharted2(FpType3 color)
{
    FpTypeBase A = 0.15; // 0.22
    FpTypeBase B = 0.50; // 0.30
    FpTypeBase C = 0.10;
    FpTypeBase D = 0.20;
    FpTypeBase E = 0.02; // 0.01
    FpTypeBase F = 0.30;
    FpTypeBase W = 11.2;

    FpType4 x = FpType4(color, MakeFpType(W));
    x = ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
    return sRGMGamma(FpType3(x[0] / x[3], x[1] / x[3], x[2] / x[3]));
}

// Optimized version of Haarm-Peter Duiker's curve
// by Jim Hejl and Richard Burgess-Dawson
// from John Hable's presentation "Uncharted 2 HDR Lighting", Page 140:
// http://www.gdcvault.com/play/1012459/Uncharted_2__HDR_Lighting
FpType3 ToneMap_FilmicALU(FpType3 color)
{
    color = max(0.0, color - 0.004);
    color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
    return color;
}

FpTypeMax3 UVToViewSpaceCoord(FpTypeMax U, FpTypeMax V, FpTypeMax MinCos)
{
    FpTypeMax HalfFovV = kPi / 6.0;
    FpTypeMax AspRatio = iResolution.x / iResolution.y;

    FpTypeMax yScale = cos(HalfFovV) / sin(HalfFovV);
    FpTypeMax xScale = yScale / AspRatio;

    FpTypeMax3 Dir;
    Dir[2] = 10.0 / kKilometersToMeters;
    Dir[0] = (U * 2.0 - 1.0) / xScale * Dir[2];
    Dir[1] = (V * 2.0 - 1.0) / yScale * Dir[2];
    FpTypeMax LenInv = rsqrt(Dir[0] * Dir[0] + Dir[1] * Dir[1] + Dir[2] * Dir[2]);
    Dir = MulF(Dir, LenInv);
    //Dir = normalize(Dir);
    // clamp cosine of zenith angle
    FpTypeMax SinZenith = SqrtSafe(1.0 - Dir[1] * Dir[1]);
    Dir[0] /= SinZenith;
    Dir[2] /= SinZenith;
    ///Dir.xz /= SqrtSafe(1.0 - Dir.y * Dir.y);
    Dir[1]   = clamp(Dir[1], MinCos, 1.0);
    SinZenith = SqrtSafe(1.0 - Dir[1] * Dir[1]);
    Dir[0] *= SinZenith;
    Dir[2] *= SinZenith;
    return Dir;
}

FpTypeMax3 UVToEquirectCoord(FpTypeMax U, FpTypeMax V, FpTypeMax MinCos)
{
    FpTypeMax Phi = kPi - V * kPi;
    FpTypeMax Theta = U * 2.0 * kPi;
    FpTypeMax3 Dir = FpTypeMax3(cos(Theta), MakeFpType(0.0), sin(Theta));

    FpTypeMax CosClamped = clamp(cos(Phi), MinCos, 1.0);
    FpTypeMax SinClamped = SqrtSafe(1.0 - CosClamped * CosClamped);

    Dir[1] = CosClamped;
    Dir[0] *= SinClamped;
    Dir[2] *= SinClamped;
    return Dir;
}

FpTypeMax3 UVToFisheyeCoord(FpTypeMax U, FpTypeMax V, FpTypeMax MinCos)
{
    FpTypeMax NdcX = U * 2.0 - 1.0;
    FpTypeMax NdcY = V * 2.0 - 1.0;
    NdcX *= iResolution.x / iResolution.y;

    FpTypeMax R = SqrtSafe(NdcX * NdcX + NdcY * NdcY);

    // x - sin(theta), z - cos(theta)
    FpTypeMax3 Dir = FpTypeMax3(NdcX / R, (FpType)0.0, NdcY / R);

    FpTypeMax Phi = saturate(R) * kPi * 0.75;

    FpTypeMax CosClamped = clamp(cos(Phi), MinCos, 1.0);
    FpTypeMax SinClamped = SqrtSafe(1.0 - CosClamped * CosClamped);

    Dir[1] = CosClamped;
    Dir[0] *= SinClamped;
    Dir[2] *= SinClamped;
    return Dir;
}
void mainImage( out FpType4 fragColor, in FpTypeMax2 fragCoord )
{
    FpTypeMax U = fragCoord[0] / iResolution.x;
    FpTypeMax V = 1.0 - fragCoord[1] / iResolution.y;

    if (ATM_SHOW_TRANSMITTANCE == 1)
    {
        FpTypeMax VScaled = V * 3.0;
        V = fract(VScaled);

        #if 0
        // Simple linear mapping
        FpTypeMax Radius = kAtmRadiusMin + V * (kAtmRadiusMax - kAtmRadiusMin);
        FpTypeMax Mu = kAtmHorizonCosMin + U * (1.0 - kAtmHorizonCosMin);
        #else
        // Non-linear mapping
        FpTypeMax Radius = kAtmRadiusMin + FpTypeMax(V * V) * (kAtmRadiusMax - kAtmRadiusMin);
        FpTypeMax Mu = kAtmHorizonCosMin + tan(1.5 * U) / tan(1.5) * (1.0 - kAtmHorizonCosMin);
        #endif

        FpTypeMax CosHorizon = AtmHorizonCosF(Radius);

        Mu = max(Mu, CosHorizon);

        FpType3 OptLen = OpticalLength(Radius, FpType(Mu), 64u);

        FpType3 OznOptDepth = Mul(OznAbsorption(), OptLen[2]);
        FpType3 RayOptDepth = Mul(RayScattering(), OptLen[0]);
        FpType3 MieOptDepth = Mul(MieScattering(), OptLen[1]);

        if (any(Mu >= CosHorizon))
        {
            FpType3 SumOptDepth;

            SumOptDepth = Select(VScaled > 2.0, RayOptDepth + MieOptDepth, OznOptDepth);
            SumOptDepth = Select(VScaled > 1.0, SumOptDepth, RayOptDepth + MieOptDepth + OznOptDepth);
            fragColor = FpType4(exp(-SumOptDepth), MakeFpType(0.0));
        }
        else
        {
            fragColor = MakeFpType4(0.0, 1.0, 1.0, 0.0);
        }
    }
    else
    {
        float U1 = iMouse.x / iResolution.x;
        float V1 = iMouse.y / iResolution.y;

        FpTypeMax R = kAtmRadiusMin + 0.1;
        FpTypeMax CosH = AtmHorizonCosF(R) - 0.01;

        FpTypeMax3 VDir = 0.0;
        FpTypeMax3 LDir = 0.0;
        if (ATM_PROJECTION_MODE == 1)
        {
            VDir = UVToViewSpaceCoord(U, V, -1.0);
            LDir = UVToViewSpaceCoord(U1, V1, CosH);
        }
        else if (ATM_PROJECTION_MODE == 2)
        {
            VDir = UVToEquirectCoord(U, V, -1.0);
            LDir = UVToEquirectCoord(U1, V1, CosH);
        }
        else
        {
            VDir = UVToFisheyeCoord(U, V, -1.0);
            LDir = UVToFisheyeCoord(U1, V1, CosH);
        }

        FpTypeMax VoL = LDir[0] * VDir[0] + LDir[1] * VDir[1] + LDir[2] * VDir[2];

        FpTypeMax V = VDir[1];
        FpType L = FpType(LDir[1]);

        FpType3 Result = MakeFpType3(0.0);
#if ATM_INTEGRATION_TYPE == 0
        Scattering(R, V, L, VoL, Predicate(VDir[2] < 0.0), Result);
#elif ATM_INTEGRATION_TYPE == 1
        ScatteringNew(R, V, L, VoL, Predicate(VDir[2] < 0.0), Result);
#endif

        FpType Exposure = exp2(4.0);

        Result = Mul(Result, Exposure);

        Result *= Select(VDir[0] * VDir[2] < 0.0, MakeFpType3(kAtmRadianceResponseToRGB_680nm, kAtmRadianceResponseToRGB_550nm, kAtmRadianceResponseToRGB_440nm)
                                                , MakeFpType3(kAtmRadianceResponseToRGB_440nm));

        if (ATM_TONEMAPPING_MODE == 0)
            Result = ToneMap_Uncharted2(Result);
        else
            Result = ToneMap_FilmicALU(Result);

        fragColor = FpType4(Result, MakeFpType(0.0));
    }
}
