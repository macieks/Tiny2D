#version 130

#include "common/common.vs"

void pos_vs()
{
	gl_Position = GetPosition();
}

###splitter###

#version 130

uniform vec4 Color;

void col_fs()
{
	gl_FragColor = Color;
}

###splitter###

#version 130

out vec2 TEXCOORD0;

#include "common/common.vs"

void tex_vs()
{
	gl_Position = GetPosition();
	TEXCOORD0 = gl_MultiTexCoord0.xy;
}

###splitter###

#version 130

out vec2 TEXCOORD0;
out vec4 COLOR0;

#include "common/common.vs"

void tex_vcol_vs()
{
	gl_Position = GetPosition();
	TEXCOORD0 = gl_MultiTexCoord0.xy;
	COLOR0 = gl_Color;
}

###splitter###

#version 130

in vec2 TEXCOORD0;

uniform sampler2D ColorMap;
uniform vec4 Color;

void tex_col_fs()
{
	gl_FragColor = texture2D(ColorMap, TEXCOORD0) * Color;
}

###splitter###

#version 130

in vec2 TEXCOORD0;
in vec4 COLOR0;

uniform sampler2D ColorMap;

void tex_vcol_fs()
{
	gl_FragColor = texture2D(ColorMap, TEXCOORD0) * COLOR0;
}

###splitter###

#version 130

out vec2 TEXCOORD0;
out vec2 TEXCOORD1;

#include "common/common.vs"

void tex2_vs()
{
	gl_Position = GetPosition();
	TEXCOORD0 = gl_MultiTexCoord0.xy;
	TEXCOORD1 = gl_MultiTexCoord1.xy;
}

###splitter###

#version 130

in vec2 TEXCOORD0;
in vec2 TEXCOORD1;

uniform sampler2D ColorMap0;
uniform sampler2D ColorMap1;
uniform float Scale;
uniform vec4 Color;

vec4 lerp(vec4 from, vec4 to, float scale)
{
	return from + (to - from) * scale;
}

void tex_lerp_col_fs()
{
	vec4 tex0 = texture2D(ColorMap0, TEXCOORD0);
	vec4 tex1 = texture2D(ColorMap1, TEXCOORD1);

	gl_FragColor = lerp(vec4(tex0.rgb * tex0.a, tex0.a), vec4(tex1.rgb * tex1.a, tex1.a), Scale);
}