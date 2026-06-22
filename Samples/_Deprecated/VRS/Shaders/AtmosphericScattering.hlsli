//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define G                       -0.975          // G is ordinarily between -0.75 and -0.999 Negative values of G scatter more light in a forward direction, positive values scatter light back towards the light source
#define G2                      (G * G)

#define SKY_COLOUR_L            float3(0.07, 0.12, 0.25)
#define SKY_COLOUR_H            float3(0.09, 0.17, 0.4)

#define DISC_COLOUR_H           float3(0.04, 0.02, 0.02)
#define DISC_COLOUR_L           float3(0.08, 0.04, 0.01)


float CalcInvZenithAngle(float dirY)
{
    float offset = 0.125;

    return 1.0 / max(0.0, (dirY + offset) / (1.0 + offset));
}


float3 AtmosphericScattering(float attenuation, float mieAttenuation, float cosAngle, float lightDirY, out float3 extinction)
{
    // poor mans substitute for doing proper integration through the atmosphere
    float3 skyColour = lerp(SKY_COLOUR_L, SKY_COLOUR_H, saturate(lightDirY));
    float3 discColour = lerp(DISC_COLOUR_L, DISC_COLOUR_H, saturate(lightDirY));

    extinction = exp(-(attenuation * (skyColour + discColour)));

    // Henyey-Greenstein function
    float phaseFunctionNumerator = (1.0 + cosAngle * cosAngle);
    float invPhaseFunctionDenominator = pow(1.0 + G2 + (2.0 * G * cosAngle), -1.5);     // use negative power here to remove a divide
    float phase = phaseFunctionNumerator * invPhaseFunctionDenominator;
    phase *= (0.2 * mieAttenuation * (1.0 - G2)) / (2.0 + G2);                          // equations give 3x(1-g^2) / 2x(2 + g^2), removed the multiples by 3/2 and replaced with other constants for artist effect

    float3 phaseColour = phase * (discColour / skyColour);

    // boost into HDR range, especially when looking at the sun
    float hdrExtra = 0.75 * phaseFunctionNumerator;
    return hdrExtra + phaseColour;
}
