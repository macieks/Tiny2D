#ifndef TINY_2D_OPENGL
#define TINY_2D_OPENGL

#include "Tiny2D.h"
#include "Tiny2D_Common.h"

#ifdef OPENGL_ES
	#ifdef WIN32
		#define CUSTOM_OPENGL_ES
		#pragma comment(lib, "libEGL.lib")
		#pragma comment(lib, "libGLESv2.lib")
		struct SDL_Window;
		bool OpenGLES_CreateContext(SDL_Window* window, App::StartupParams* params);
		void OpenGLES_DestroyContext();
		void OpenGLES_SwapWindow();
	#endif
	void OpenGLES_ConvertFromOpenGL(std::string& code);
	#include "SDL_opengles2.h"

	#define glActiveTextureARB glActiveTexture
	#define glGenFramebuffersEXT glGenFramebuffers
	#define glDeleteFramebuffersEXT glDeleteFramebuffers
	#define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER
	#define glBindFramebufferEXT glBindFramebuffer
	#define GL_DEPTH_ATTACHMENT_EXT GL_DEPTH_ATTACHMENT
	#define GL_STENCIL_ATTACHMENT_EXT GL_STENCIL_ATTACHMENT
	#define GL_COLOR_ATTACHMENT0_EXT GL_COLOR_ATTACHMENT0
	#define glFramebufferTexture2DEXT glFramebufferTexture2D
	#define glFramebufferRenderbufferEXT glFramebufferRenderbuffer
	#define GL_RGBA8 GL_RGBA8_OES
	#define GL_RGB8 GL_RGB8_OES
	#define GL_BGRA GL_BGRA_EXT
	#define GL_VERTEX_SHADER_ARB GL_VERTEX_SHADER
	#define GL_FRAGMENT_SHADER_ARB GL_FRAGMENT_SHADER
	#define glShaderSourceARB glShaderSource
	#define glCompileShaderARB glCompileShader
	#define glCreateShaderObjectARB glCreateShader
	#define GL_OBJECT_COMPILE_STATUS_ARB GL_COMPILE_STATUS
	#define glCreateProgramObjectARB glCreateProgram
	#define glAttachObjectARB glAttachShader
	#define glLinkProgramARB glLinkProgram
	#define glUseProgramObjectARB glUseProgram
	#define glGetObjectParameterivARB glGetShaderiv
	#define GL_OBJECT_LINK_STATUS_ARB GL_LINK_STATUS
	#define GL_OBJECT_COMPILE_STATUS_ARB GL_COMPILE_STATUS
	#define glGetInfoLogARB glGetString
	#define GLcharARB GLchar
	#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB GL_ACTIVE_ATTRIBUTES
	#define GL_OBJECT_ACTIVE_UNIFORMS_ARB GL_ACTIVE_UNIFORMS
	#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB GL_ACTIVE_UNIFORM_MAX_LENGTH
	#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB GL_ACTIVE_ATTRIBUTE_MAX_LENGTH
	#define glGetActiveAttribARB glGetActiveAttrib
	#define glGetAttribLocationARB glGetAttribLocation
	#define glGetActiveUniformARB glGetActiveUniform
	#define glGetUniformLocationARB glGetUniformLocation
	#define glEnableVertexAttribArrayARB glEnableVertexAttribArray
	#define glDisableVertexAttribArrayARB glDisableVertexAttribArray
	#define glVertexAttribPointerARB glVertexAttribPointer
	#define glCheckFramebufferStatusEXT glCheckFramebufferStatus
	#define GL_FRAMEBUFFER_COMPLETE_EXT GL_FRAMEBUFFER_COMPLETE
	#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
	#define GL_FRAMEBUFFER_UNSUPPORTED_EXT GL_FRAMEBUFFER_UNSUPPORTED
	#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
	#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
	#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT GL_FRAMEBUFFER_INCOMPLETE_FORMATS
#else
	#include "SDL_opengl.h"
	#ifdef WIN32
		#pragma comment(lib, "opengl32.lib")
	#endif
#endif

#ifndef OPENGL_ES
	#define GL_PROC(type, func) extern type func;
	#include "Tiny2D_OpenGLProcedures.h"
	#undef GL_PROC
#endif

#ifndef OPENGL_ES
	#define glEnableTexture2D() GL(glEnable(GL_TEXTURE_2D));
	#define glDisableTexture2D() GL(glDisable(GL_TEXTURE_2D));
#else
	#define glEnableTexture2D()
	#define glDisableTexture2D()
#endif

#ifdef DEBUG
	void CheckOpenGLErrors(const char* operation, const char* file, unsigned int line);

	template <typename TYPE>
	TYPE CheckOpenGLCallRet(TYPE result, const char* operation, const char* file, unsigned int line)
	{
		CheckOpenGLErrors(operation, file, line);
		return result;
	}

	#define GL(call) do { call; CheckOpenGLErrors(#call, __FILE__, __LINE__); } while (0)
	#define GLR(call) CheckOpenGLCallRet(call, #call, __FILE__, __LINE__);
#else
	#define GL(call) call
	#define GLR(call) call
#endif

struct TextureObj : Resource
{
	GLuint handle;
	GLenum format;
	GLint internalFormat;
	bool hasAlpha;
	int width;
	int height;
	bool isRenderTarget;
	bool isLoaded;
	float sizeScale;

	TextureObj() :
		Resource("texture"),
		handle(0),
		format(0),
		internalFormat(0),
		width(0),
		height(0),
		isRenderTarget(false),
		isLoaded(false),
		sizeScale(1.0f)
	{}
};

struct Shader : Resource
{
	enum Type
	{
		Type_Vertex = 0,
		Type_Fragment,

		Type_COUNT
	};

	Type type;
	GLuint handle;

	std::string sourceCode;

	Shader() : Resource("shader") {}
};

void Shader_Destroy(Shader* shader);

class ShaderPtr
{
public:
	ShaderPtr(Shader* shader = NULL) { this->shader = shader; }
	~ShaderPtr() { if (shader) Shader_Destroy(shader); }
	inline void operator == (Shader* shader) { if (shader) Shader_Destroy(shader); this->shader = shader; }
	inline Shader* operator -> () const { return shader; }
	inline Shader* operator * () const { return shader; }
private:
	Shader* shader;
};

struct ShaderAttribute
{
	Shape::VertexUsage usage;
	GLint usageIndex;
	GLint location;
};

struct ShaderParameterDescription
{
	enum Type
	{
		Type_Int = 0,
		Type_Float,
		Type_Texture,

		Type_COUNT
	};

	std::string name;
	Type type;
	GLint count;

	inline bool operator != (const ShaderParameterDescription& other) const
	{
		return
			name != other.name ||
			type != other.type ||
			count != other.count;
	}
};

struct ShaderParameter : ShaderParameterDescription
{
	GLint location;
};

struct ShaderProgram : Resource
{
	GLuint handle;

	Shader* vs;
	Shader* fs;

	std::vector<ShaderAttribute> attributes;
	std::vector<ShaderParameter> parameters;

	ShaderProgram() : Resource("shader program") {}
};

struct MaterialParameter
{
	ShaderParameterDescription* shaderParameterDescription;

	union
	{
		int intValue[4];
		float floatValue[4];
		TextureObj* textureValue;
	};
	Sampler sampler;

	MaterialParameter() :
		shaderParameterDescription(NULL)
	{
		textureValue = NULL;
	}
};

struct MaterialBase
{
	std::vector<MaterialParameter> parameters;
};

struct MaterialTechnique
{
	std::string name;
	ShaderProgram* shaderProgram;
	std::vector<int> materialParameterIndices;
	Shape::Blending blending;

	MaterialTechnique() :
		shaderProgram(NULL),
		blending(Shape::Blending_Default)
	{}
};

struct MaterialResource : Resource, MaterialBase
{
	std::vector<MaterialTechnique> techniques;

	MaterialResource() : Resource("material") {}
};

struct MaterialObj : MaterialBase
{
	MaterialTechnique* currentTechnique;
	MaterialResource* resource;
	int screenSizeParamIndex;
	int projectionScaleParamIndex;

	MaterialObj() :
		currentTechnique(NULL),
		resource(NULL),
		screenSizeParamIndex(-1),
		projectionScaleParamIndex(-1)
	{}
};

inline GLint Sampler_WrapMode_ToOpenGL(Sampler::WrapMode mode)
{
	switch (mode)
	{
		case Sampler::WrapMode_Clamp: return GL_CLAMP_TO_EDGE;
		case Sampler::WrapMode_ClampToBorder:
#ifdef OPENGL_ES
			return GL_CLAMP_TO_EDGE;
#else
			return GL_CLAMP_TO_BORDER;
#endif
		case Sampler::WrapMode_Repeat: return GL_REPEAT;
		case Sampler::WrapMode_Mirror: return GL_MIRRORED_REPEAT;
	}
	return GL_CLAMP_TO_EDGE;
}

inline GLenum Shape_Geometry_Type_ToOpenGL(Shape::Geometry::Type type)
{
	switch (type)
	{
		case Shape::Geometry::Type_TriangleFan: return GL_TRIANGLE_FAN;
		case Shape::Geometry::Type_Triangles: return GL_TRIANGLES;
		case Shape::Geometry::Type_Lines: return GL_LINES;
	}

	Assert(!"Unsupported primitive type");
	return GL_TRIANGLE_FAN;
}

#endif // TINY_2D_OPENGL