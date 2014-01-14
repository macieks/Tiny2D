#version 130

uniform sampler2D ColorMap;
uniform float BlurKernel;
uniform vec4 ScreenSize;

in vec2 TEXCOORD0;

#if 0 // GLSL extremely slow when using int indexed array of float weights

#define NUM_BLUR_SAMPLES 13

float BlurWeights[NUM_BLUR_SAMPLES] = float[]
(
	0.002216,
	0.008764,
	0.026995,
	0.064759,
	0.120985,
	0.176033,
	0.199471,
	0.176033,
	0.120985,
	0.064759,
	0.026995,
	0.008764,
	0.002216
);

vec4 DoBlur(vec2 pixelOffset)
{
	float pixelKernel = -6.0 * BlurKernel;
	vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < NUM_BLUR_SAMPLES; i++, pixelKernel += BlurKernel)
        color += texture2D(ColorMap, TEXCOORD0 + pixelOffset * pixelKernel) * BlurWeights[i];
	return color;
}

#else

vec4 DoBlur(vec2 pixelOffset)
{
	pixelOffset *= BlurKernel;

	return
		texture2D(ColorMap, TEXCOORD0 - 6.0 * pixelOffset) * 0.002216 +
		texture2D(ColorMap, TEXCOORD0 - 5.0 * pixelOffset) * 0.008764 +
		texture2D(ColorMap, TEXCOORD0 - 4.0 * pixelOffset) * 0.026995 +
		texture2D(ColorMap, TEXCOORD0 - 3.0 * pixelOffset) * 0.064759 +
		texture2D(ColorMap, TEXCOORD0 - 4.0 * pixelOffset) * 0.120985 +
		texture2D(ColorMap, TEXCOORD0 - 1.0 * pixelOffset) * 0.176033 +
		texture2D(ColorMap, TEXCOORD0					  ) * 0.199471 +
		texture2D(ColorMap, TEXCOORD0 + 1.0 * pixelOffset) * 0.176033 +
		texture2D(ColorMap, TEXCOORD0 + 2.0 * pixelOffset) * 0.120985 +
		texture2D(ColorMap, TEXCOORD0 + 3.0 * pixelOffset) * 0.064759 +
		texture2D(ColorMap, TEXCOORD0 + 4.0 * pixelOffset) * 0.026995 +
		texture2D(ColorMap, TEXCOORD0 + 5.0 * pixelOffset) * 0.008764 +
		texture2D(ColorMap, TEXCOORD0 + 6.0 * pixelOffset) * 0.002216;
}

#endif