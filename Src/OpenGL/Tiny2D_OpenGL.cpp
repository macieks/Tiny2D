#include "Tiny2D_OpenGL.h"

namespace Tiny2D
{

extern GLuint g_fbo;

float g_screenSizeMaterialParam[4];
float g_projectionScaleMaterialParam[4];

ResourceState Texture_GetState(TextureObj* texture)
{
	return texture->state;
}

void Texture_Destroy(TextureObj* texture)
{
	if (!Resource_DecRefCount(texture))
	{
		if (texture->state == ResourceState_Creating)
			Jobs::CancelJob(texture->jobID);

		if (texture == Texture_Get(App::GetMainRenderTarget()))
		{
			//Log::Error("Attempted to destroy main render target");
		}
		else if (texture->handle)
			glDeleteTextures(1, &texture->handle);
		delete texture;
	}
}

void Shape_SetBlending(bool primitiveIsTranslucent, Shape::Blending blending)
{
	switch (blending)
	{
	case Shape::Blending_Default:
		if (!primitiveIsTranslucent)
			GL(glDisable(GL_BLEND));
		else
		{
			GL(glEnable(GL_BLEND));
			GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
		}
		break;
	case Shape::Blending_Alpha:
		GL(glEnable(GL_BLEND));
		GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
		break;
	case Shape::Blending_Additive:
		GL(glEnable(GL_BLEND));
		GL(glBlendFunc(GL_ONE, GL_ONE));
		break;
    default:
		GL(glDisable(GL_BLEND));
		break;
	}
}

void Texture_Draw(TextureObj* texture, const Vec2& position, float rotation, float scale, const Color& color)
{
	float uv[8] =
	{
		0, 0,
		1, 0,
		1, 1,
		0, 1
	};

	const float scaledWidth = (float) texture->width * scale;
	const float scaledHeight = (float) texture->height * scale;

	float left = position.x;
	float top = position.y;

	left -= (scaledWidth - (float) texture->width) * 0.5f;
	top -= (scaledHeight - (float) texture->height) * 0.5f;

	float xy[8] =
	{
		left, top,
		left + scaledWidth, top,
		left + scaledWidth, top + scaledHeight,
		left, top + scaledHeight
	};

	if (rotation != 0)
	{
		const float centerX = left + scaledWidth * 0.5f;
		const float centerY = top + scaledHeight * 0.5f;

		const float rotationSin = sinf(rotation);
		const float rotationCos = cosf(rotation);

		float* xyPtr = xy;
		for (int i = 0; i < 4; i++, xyPtr += 2)
			Vertex_Rotate(xyPtr[0], xyPtr[1], centerX, centerY, rotationSin, rotationCos);
	}

	Shape::DrawParams params;
	params.SetNumVerts(4);
	params.SetTexCoord(uv);
	params.SetPosition(xy);
	params.color = color;

	Texture_Draw(texture, &params);
}

void Texture_Draw(TextureObj* texture, const Shape::DrawParams* params, const Sampler& sampler)
{
	MaterialObj* material = Material_Get(App::GetDefaultMaterial());

	Material_SetTechnique(material, "tex_col");
	Material_SetTextureParameter(material, "ColorMap", texture, sampler);
	Material_SetFloatParameter(material, "Color", (const float*) &params->color, 4);
	Material_Draw(material, params);
}

int Texture_GetWidth(TextureObj* texture)
{
	if (texture->sizeScale != 1.0f)
		return (int) ((float) texture->width * texture->sizeScale);
	return texture->width;
}

int Texture_GetHeight(TextureObj* texture)
{
	if (texture->sizeScale != 1.0f)
		return (int) ((float) texture->height * texture->sizeScale);
	return texture->height;
}

int Texture_GetRealWidth(TextureObj* texture)
{
	return texture->width;
}

int Texture_GetRealHeight(TextureObj* texture)
{
	return texture->height;
}

TextureObj* Texture_CreateRenderTarget(int width, int height)
{
	GLenum format = GL_RGBA;
#ifdef OPENGL_ES
	GLint internalFormat = GL_RGBA;
#else
	GLint internalFormat = GL_RGBA8;
#endif

	TextureObj* texture = new TextureObj();
	texture->state = ResourceState_Created;
	texture->width = width;
	texture->height = height;
	texture->format = format;
	texture->internalFormat = internalFormat;
	texture->hasAlpha = true;
	texture->refCount = 1;
	texture->isRenderTarget = true;

	GL(glGenTextures(1, &texture->handle));
	GL(glBindTexture(GL_TEXTURE_2D, texture->handle));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	GL(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, NULL));

	return texture;
}

void Texture_BeginDrawing(TextureObj* texture, const Color* clearColor)
{
	if (App::GetCurrentRenderTarget().GetState() == ResourceState_Created)
	{
		Log::Error("Failed to begin drawing to texture, reason: previous rendering wasn't finalized via Texture_EndDrawing");
		return;
	}
	if (!texture->isRenderTarget)
	{
		Log::Error("Failed to begin drawing to texture, reason: texture is not render target");
		return;
	}

	App_SetCurrentRenderTarget(texture);

	// Determine rendering viewport and scaling

	const bool isMainRenderTarget = App::GetMainRenderTarget() == texture;

	int viewportLeft = 0;
	int viewportTop = 0;
	int viewportWidth = texture->width;
	int viewportHeight = texture->height;

	if (isMainRenderTarget)
	{
		extern int g_viewportLeft;
		extern int g_viewportTop;
		extern int g_viewportWidth;
		extern int g_viewportHeight;

		viewportLeft = g_viewportLeft;
		viewportTop = g_viewportTop;
		viewportWidth = g_viewportWidth;
		viewportHeight = g_viewportHeight;
	}

	// Update screen size and scale material parameters

	g_screenSizeMaterialParam[0] = (float) texture->width;
	g_screenSizeMaterialParam[1] = (float) texture->height;
	g_screenSizeMaterialParam[2] = 1.0f / g_screenSizeMaterialParam[0];
	g_screenSizeMaterialParam[3] = 1.0f / g_screenSizeMaterialParam[1];

	if (App_GetSceneRenderTarget() == texture)
	{
		g_projectionScaleMaterialParam[0] = 2.0f / (float) App::GetWidth();
		g_projectionScaleMaterialParam[1] = (isMainRenderTarget ? -2.0f : 2.0f) / (float) App::GetHeight();
	}
	else
	{
		g_projectionScaleMaterialParam[0] = 2.0f / (float) texture->width;
		g_projectionScaleMaterialParam[1] = (isMainRenderTarget ? -2.0f : 2.0f) / (float) texture->height;
	}
	g_projectionScaleMaterialParam[2] = -1.0f;
	g_projectionScaleMaterialParam[3] = isMainRenderTarget ? 1.0f : -1.0f;

	// Bind fbo

	if (App::GetMainRenderTarget() == texture)
	{
		GL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
#ifndef OPENGL_ES
		GL(glDrawBuffer(GL_BACK));
		GL(glReadBuffer(GL_BACK));
#endif
	}
	else
	{
		GL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_fbo));
		GL(glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, texture->handle, 0));
#ifndef OPENGL_ES
		const GLenum drawBuffer = GL_COLOR_ATTACHMENT0_EXT;
		GL(glDrawBuffers(1, &drawBuffer));
		GL(glReadBuffer(GL_COLOR_ATTACHMENT0_EXT));
#endif

		GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
		{
			const char* statusString = NULL;
			switch (status)
			{
#define FBO_STATUS_CASE(value) case value: statusString = #value; break;
				FBO_STATUS_CASE(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT)
				FBO_STATUS_CASE(GL_FRAMEBUFFER_UNSUPPORTED_EXT)
				FBO_STATUS_CASE(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT)
				FBO_STATUS_CASE(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT)
#ifndef OPENGL_ES
				FBO_STATUS_CASE(GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT)
				FBO_STATUS_CASE(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT)
				FBO_STATUS_CASE(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT)
#endif
#undef FBO_STATUS_CASE
				default: statusString = "UNKNOWN"; break;
			}
			Log::Error(string_format("Invalid FBO status: %s", statusString));
		}
	}

	// Clear main buffer to black if letterboxing

	if (isMainRenderTarget)
	{
		const bool viewportMatchesTextureAspect = (float) App::GetWidth() / (float) App::GetHeight() == (float) texture->width / (float) texture->height;
		if (!viewportMatchesTextureAspect)
		{
			GL(glViewport(0, 0, texture->width, texture->height));
			Texture_Clear(texture, Color::Black);
		}
	}

	// Set up viewport

	GL(glViewport(viewportLeft, viewportTop, viewportWidth, viewportHeight));

	// Clear

	if (clearColor)
		Texture_Clear(texture, *clearColor);
}

void Texture_Clear(TextureObj* texture, const Color& color)
{
	if (App::GetCurrentRenderTarget() != texture)
	{
		Log::Error("Cleared texture must be currently set render target (via Texture_BeginDrawing)");
		return;
	}

	GL(glClearColor(color.r, color.g, color.b, color.a));
	GL(glClear(GL_COLOR_BUFFER_BIT));
}

bool Texture_GetPixels(TextureObj* texture, std::vector<unsigned char>& pixels, bool removeAlpha)
{
	if (texture == Texture_Get(App::GetMainRenderTarget()))
		return false;
	if (!texture->handle)
	{
		Log::Warn("Failed to get texture pixels, reason: texture handle not valid");
		return false;
	}

	const int numBytesPerPixel = Texture_GetBytesPerPixel(texture);
	if (!numBytesPerPixel)
	{
		Log::Warn("Failed to get texture pixels, reason: failed to get BPP for texture format");
		return false;
	}

	pixels.resize( texture->width * texture->height * numBytesPerPixel );
#ifdef OPENGL_ES
	if (texture != Texture_Get(App::GetCurrentRenderTarget()))
	{
		Log::Warn("Failed to get texture pixels, reason: this operation only supported for texture currently bound as frame buffer");
		// TODO: Try binding to FBO and use glReadPixels then
		return false;
	}
	GL(glReadPixels(0, 0, texture->width, texture->height, texture->format, GL_UNSIGNED_BYTE, &pixels[0]));
#else
	glEnableTexture2D();
	glBindTexture(GL_TEXTURE_2D, texture->handle);
	GL(glGetTexImage(GL_TEXTURE_2D, 0, texture->format, GL_UNSIGNED_BYTE, &pixels[0]));
#endif

	if (texture->hasAlpha && removeAlpha)
		Pixels_RGBA_To_RGB(pixels);

	return true;
}

const float* App_GetScreenSizeMaterialParam()
{
	return g_screenSizeMaterialParam;
}

const float* App_GetProjectionScaleMaterialParam()
{
	return g_projectionScaleMaterialParam;
}

void Texture_EndDrawing(TextureObj* texture)
{
	if (App::GetCurrentRenderTarget() != texture)
	{
		Log::Error("Failed to end drawing to texture, reason: texture to end drawing to must be currently set as a render target via Texture_BeginDrawing");
		return;
	}

	App_SetCurrentRenderTarget(NULL);
}

void CheckOpenGLErrors(const char* operation, const char* file, unsigned int line)
{
	GLenum error = glGetError();
	if (error == GL_NO_ERROR)
		return;

	const char* errorAsString = "UNKNOWN";
	switch (error)
	{
#define CASE(value) case value: errorAsString = #value;
		CASE(GL_INVALID_ENUM)
		CASE(GL_INVALID_VALUE)
		CASE(GL_INVALID_OPERATION)
		CASE(GL_OUT_OF_MEMORY)
		CASE(GL_INVALID_FRAMEBUFFER_OPERATION)
#ifndef OPENGL_ES
		CASE(GL_STACK_OVERFLOW)
		CASE(GL_STACK_UNDERFLOW)
#endif
	}

	Log::Error(string_format("OpenGL error: %s (%u)\nOPERATION: %s\nFILE: %s\nLINE: %u\n", errorAsString, (unsigned int) error, operation, file, line));
}

};
