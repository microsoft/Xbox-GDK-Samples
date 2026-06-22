// The MIT License
// Copyright © 2013 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


// Simplex Noise (http://en.wikipedia.org/wiki/Simplex_noise), a type of gradient noise
// that uses N+1 vertices for random gradient interpolation instead of 2^N as in regular
// latice based Gradient Noise.


// Value    Noise 2D, Derivatives: https://www.shadertoy.com/view/4dXBRH
// Gradient Noise 2D, Derivatives: https://www.shadertoy.com/view/XdXBRH
// Value    Noise 3D, Derivatives: https://www.shadertoy.com/view/XsXfRH
// Gradient Noise 3D, Derivatives: https://www.shadertoy.com/view/4dffRH
// Value    Noise 2D             : https://www.shadertoy.com/view/lsf3WH
// Value    Noise 3D             : https://www.shadertoy.com/view/4sfGzS
// Gradient Noise 2D             : https://www.shadertoy.com/view/XdXGW8
// Gradient Noise 3D             : https://www.shadertoy.com/view/Xsl3Dl
// Simplex  Noise 2D             : https://www.shadertoy.com/view/Msf3WH
// Wave     Noise 2D             : https://www.shadertoy.com/view/tldSRj


vec2 hash(vec2 p) // replace this by something better
{
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float noise(in vec2 p)
{
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;

    vec2 i = floor(p + (p.x + p.y) * K1);
    vec2 a = p - i + (i.x + i.y) * K2;
    float m = step(a.y, a.x);
    vec2 o = vec2(m, 1.0 - m);
    vec2 b = a - o + K2;
    vec2 c = a - 1.0 + 2.0 * K2;
    vec3 h = max(0.5 - vec3(dot(a, a), dot(b, b), dot(c, c)), 0.0);
    vec3 n = h * h * h * h * vec3(dot(a, hash(i + 0.0)), dot(b, hash(i + o)), dot(c, hash(i + 1.0)));
#if USE_GLSL_FIXES
    return dot(n, vec3(70.0.xxx));
#else
    return dot(n, vec3(70.0));
#endif
}

// -----------------------------------------------

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 p = fragCoord.xy / iResolution.xy;

    vec2 uv = p * vec2(iResolution.x / iResolution.y, 1.0);
	
    float f = 0.0;
	
    // left: value noise	
    if (p.x < 0.6)
    {
        f = noise(16.0 * uv);
    }
    // right: fractal noise (4 octaves)
    else
    {
        uv *= 5.0;
        mat2 m = mat2(1.6, 1.2, -1.2, 1.6);
        f = 0.5000 * noise(uv);
#if USE_GLSL_FIXES
        uv = mul(m, uv);
        f += 0.2500 * noise(uv);
        uv = mul(m, uv);
        f += 0.1250 * noise(uv);
        uv = mul(m, uv);
        f += 0.0625 * noise(uv);
        uv = mul(m, uv);
#else
        uv = m * uv;
        f += 0.2500 * noise(uv);
        uv = m * uv;
        f += 0.1250 * noise(uv);
        uv = m * uv;
        f += 0.0625 * noise(uv);
        uv = m * uv;
#endif
    }

    f = 0.5 + 0.5 * f;
	
    f *= smoothstep(0.0, 0.005, abs(p.x - 0.6));
	
    fragColor = vec4(f, f, f, 1.0);
}
