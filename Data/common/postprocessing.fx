#version 130

out vec4 TEXCOORD0;
out vec4 TEXCOORD1;

uniform vec4 ScreenSize;

void downsample2x2_vs()
{
	gl_Position = gl_Vertex;

	TEXCOORD0.xy = gl_MultiTexCoord0.xy + ScreenSize.zw;
	TEXCOORD0.zw = gl_MultiTexCoord0.xy + vec2(ScreenSize.z, -ScreenSize.w);
	TEXCOORD1.xy = gl_MultiTexCoord0.xy - ScreenSize.zw;
	TEXCOORD1.zw = gl_MultiTexCoord0.xy + vec2(-ScreenSize.z,  ScreenSize.w);
}

###splitter###

#version 130

uniform sampler2D ColorMap;

in vec4 TEXCOORD0;
in vec4 TEXCOORD1;

void downsample2x2_fs()
{
	vec4 tex = vec4(0.0, 0.0, 0.0, 0.0);
	tex += texture2D(ColorMap, TEXCOORD0.xy);
	tex += texture2D(ColorMap, TEXCOORD0.zw);
	tex += texture2D(ColorMap, TEXCOORD1.xy);
	tex += texture2D(ColorMap, TEXCOORD1.zw);
	gl_FragColor = tex * 0.25;
}

###splitter###

#version 130

out vec2 TEXCOORD0;

void tex_vs()
{
	gl_Position = gl_Vertex;
	TEXCOORD0 = gl_MultiTexCoord0.xy;
}

###splitter###

#version 130

out vec2 TEXCOORD0;

uniform vec2 Offset;

void quake_vs()
{
	gl_Position = gl_Vertex + vec4(Offset.xy, 0, 0);
	TEXCOORD0 = gl_MultiTexCoord0.xy;
}

###splitter###

#version 130

uniform sampler2D ColorMap;

in vec2 TEXCOORD0;

void tex_fs()
{
	gl_FragColor = texture2D(ColorMap, TEXCOORD0);
}

###splitter###

#version 130

uniform sampler2D ColorMap0;
uniform sampler2D ColorMap1;
uniform float BlendFactor;

in vec2 TEXCOORD0;

void blend_fs()
{
	vec4 tex0 = texture2D(ColorMap0, TEXCOORD0);
	vec4 tex1 = texture2D(ColorMap1, TEXCOORD0);
	gl_FragColor = tex0 + (tex1 - tex0) * BlendFactor;
}

###splitter###

#include "common/postprocessing_blur.fx"

void vertical_blur_fs()
{
	gl_FragColor = DoBlur(vec2(0, ScreenSize.w));
}

###splitter###

#include "common/postprocessing_blur.fx"

void horizontal_blur_fs()
{
	gl_FragColor = DoBlur(vec2(ScreenSize.z, 0));
}

###splitter###

#version 130

uniform vec4 ScreenSize;
uniform float Time;

uniform float OverExposureAmount;
uniform float DustAmount;
uniform float FrameJitterFrequency;
uniform float MaxFrameJitter;
uniform float GrainThicknes;
uniform float ScratchesAmount;

out vec2 TEXCOORD0;
out vec4 Dust0And1Coords;
out vec4 Dust2And3Coords;
out vec2 TvCoords;
out vec2 NoiseCoords;
out vec4 Line0VertCoords;
out vec4 Line1VertCoords;
out vec4 Line2VertCoords;
out float OverExposure;

void oldtv_vs()
{
	gl_Position = gl_Vertex;

	// some pseudo-random numbers
	float Random0 = mod(Time, 7.000);
	float Random1 = mod(Time, 3.300);
	float Random2 = mod(Time, 0.910);
	float Random3 = mod(Time, 0.110);
	float Random4 = mod(Time, 0.070);
	float Random5 = mod(Time, 0.013);

	// compute vert. frame jitter
	float frameJitterY = 40.0 * MaxFrameJitter * Random2 * Random0 * Random3;
	if (frameJitterY < (6.0 - FrameJitterFrequency) * 10.0)
		frameJitterY = 0.0;
 	frameJitterY *= ScreenSize.w;

	// add jitter to the original coords.
	TEXCOORD0 = gl_MultiTexCoord0.xy + vec2(0.0, frameJitterY);

	// compute overexposure amount
	OverExposure = clamp(OverExposureAmount * Random3, 0.0, 1.0);
	
	// pass original screen coords (border rendering)
	TvCoords = gl_MultiTexCoord0.xy;

	// compute noise coords.
	vec2 NoiseCoordsTmp = (ScreenSize.xy / (GrainThicknes * vec2(128.0, 128.0))) * gl_MultiTexCoord0.xy;
	NoiseCoordsTmp += vec2(100.0 * Random3 * Random1 - Random0, Random4 + Random1 * Random2);
	NoiseCoords = NoiseCoordsTmp.xy;

	// dust section (turns on or off particular dust texture)
	if (DustAmount > 0.0)
		Dust0And1Coords.xy = 2.0 * gl_Vertex.xy + 200.0 * vec2(Random1 * Random4, mod(Time, 0.03));
	else
		Dust0And1Coords.xy = vec2(0.0, 0.0);

	if (DustAmount > 1.0)
		Dust0And1Coords.zw = 2.3 * gl_Vertex.yx - 210.0 * vec2(Random4 * 0.45, Random5 * 2.0);
	else
		Dust0And1Coords.zw = vec2(0.0, 0.0);

	if (DustAmount > 2.0)
		Dust2And3Coords.xy = 1.4 * gl_Vertex.xy + vec2(700.0, 100.0) * vec2(Random2 * Random4, Random2);
	else
		Dust2And3Coords.xy = vec2(0.0, 0.0);

	if (DustAmount > 3.0)
		Dust2And3Coords.zw = 1.7 * gl_Vertex.yx + vec2(-100.0, 130.0) * vec2(Random2 * Random4, Random1 * Random4);
	else
		Dust2And3Coords.zw = vec2(0.0, 0.0);
	
	// vert lines section
	Line0VertCoords = 0.5 * gl_Vertex.xxxx * ScreenSize.xxxx * 0.3;
	Line1VertCoords = Line0VertCoords;
	Line2VertCoords = Line0VertCoords;

	// first line
	if (ScratchesAmount > 0.0)
		Line0VertCoords.x += 0.15 - ((Random1 - Random3 + Random2) * 0.1) * ScreenSize.x;
	else
		Line0VertCoords.x = -1.0;

	// second line
	if (ScratchesAmount > 1.0)
		Line1VertCoords.x += 0.55 + ((Random0 - Random2 + Random4) * 0.1) * ScreenSize.x;
	else
		Line1VertCoords.x = -1.0;

	// third line
	if (ScratchesAmount > 2.0)
		Line2VertCoords.x += 0.31 + ((Random1 - Random2 + Random4) * 0.2) * ScreenSize.x;
	else
		Line2VertCoords.x = -1.0;
}

###splitter###

#version 130

uniform vec4 FilmColor;
uniform float GrainAmount;
uniform float ScratchesLevel;

uniform sampler2D ColorMap;
uniform sampler2D LineMap;
uniform sampler2D DustMap;
uniform sampler2D TvMap;
uniform sampler2D NoiseMap;

in vec2 TEXCOORD0;
in vec4 Dust0And1Coords;
in vec4 Dust2And3Coords;
in vec2 TvCoords;
in vec2 NoiseCoords;
in vec4 Line0VertCoords;
in vec4 Line1VertCoords;
in vec4 Line2VertCoords;
in float OverExposure;

vec4 lerp(vec4 from, vec4 to, float scale)
{
	return from + (to - from) * scale;
}

vec4 lerp(float from, vec4 to, float scale)
{
	vec4 fromVec = vec4(from, from, from, from);
	return fromVec + (to - fromVec) * scale;
}

vec4 texture2DClampedES(sampler2D s, vec2 uv)
{
#ifdef GLES
	return (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) ? vec4(1.0, 1.0, 1.0, 1.0) : texture2D(s, uv);
#else
	return texture2D(s, uv);
#endif
}

vec4 texture2DClampedUOnlyES(sampler2D s, vec2 uv)
{
#ifdef GLES
	return (uv.x < 0.0 || uv.x > 1.0) ? vec4(1.0, 1.0, 1.0, 1.0) : texture2D(s, uv);
#else
	return texture2D(s, uv);
#endif
}

void oldtv_fs()
{
    // sample scene
	vec4 img = texture2D(ColorMap, TEXCOORD0.xy);

	// compute sepia (or other color) image
	vec4 img2 =
		lerp(
			dot(img, vec4(0.30, 0.59, 0.11, 0.3)),
			img * vec4(0.50, 0.59, 0.41, 0.3),
			0.5)
		* FilmColor;

	// sample dust textures
	vec4 dust0 = texture2DClampedES(DustMap, Dust0And1Coords.xy);
	vec4 dust1 = texture2DClampedES(DustMap, Dust0And1Coords.wz);
	vec4 dust2 = texture2DClampedES(DustMap, Dust2And3Coords.xy);
	vec4 dust3 = texture2DClampedES(DustMap, Dust2And3Coords.wz);

	// sample line textures
	vec4 line0 = texture2DClampedUOnlyES(LineMap, Line0VertCoords.xy);
	vec4 line1 = texture2DClampedUOnlyES(LineMap, Line1VertCoords.xy);
	vec4 line2 = texture2DClampedUOnlyES(LineMap, Line2VertCoords.xy);

	// sample border texture
	vec4 tv = texture2D(TvMap, TvCoords.xy);

	// sample noise values
	vec4 noiseVal = texture2D(NoiseMap, NoiseCoords.xy);
	noiseVal = lerp(vec4(1.0, 1.0, 1.0, 1.0), noiseVal, GrainAmount);

	// "accumulate" dust
	float finalDust = dust0.x * dust1.y * dust2.z * dust3.w;

	// "accumulate" lines
	float finalLines = line0.x * line1.x * line2.x;
	//finalLines = lerp(vec4(1.0, 1.0, 1.0, 1.0), finalLines, ScratchesLevel);
	
	// final composition
	gl_FragColor = tv * (OverExposure + finalDust * finalLines * img2) * noiseVal.wwww;
}

###splitter###

uniform sampler2D ColorMap;
in vec2 TEXCOORD0;

void rain_evaporation_fs()
{
	vec4 distortion = texture2D(ColorMap, TEXCOORD0.xy);
	vec4 identity_dir = vec4(0.5, 0.5, 1.0, 0.0);
	
	vec4 distortion_change = identity_dir - distortion;
	distortion_change = clamp(distortion_change, -(1.0 / 255.0), (1.0 / 255.0));

	gl_FragColor = distortion + distortion_change;
}

###splitter###

uniform sampler2D ColorMap;
uniform sampler2D NormalMap;

uniform vec4 DropletColor;

in vec2 TEXCOORD0;

void rain_distortion_fs()
{
	float DistortionScale = 0.02;

	vec4 normal = texture2D(NormalMap, TEXCOORD0.xy);
	normal.xyz = (normal.xyz * 2.0) - 1.0;

	float dif = abs(dot(vec3(0.0, 0.0, 1.0), normal.xyz));
//	dif = max(dif, 0.5);
	vec3 dropletColor = DropletColor.xyz * 3.0 * (1.0 - dif);

	vec4 tex = texture2D(ColorMap, TEXCOORD0.xy + normal.xy * DistortionScale);

	gl_FragColor = vec4(tex.xyz, 1.0) +
		//float4(dropletColor, 1.0);
		vec4(DropletColor.xyz * normal.w, 0.0);
}