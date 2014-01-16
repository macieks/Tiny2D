#include "Tiny2D_OpenGL.h"

int g_maxTextureUnitSet = -1;

// Shader

bool Shader_LoadSourceCodeFromString(const std::string& path, std::string& sourceCodeOut);

bool Shader_LoadSourceCode(const std::string& path, std::string& sourceCodeOut)
{
	char* sourceCode = NULL;
	int sourceCodeSize = 0;
	if (!File_Load(path, (void*&) sourceCode, sourceCodeSize))
	{
		Log::Error(string_format("Failed to load shader %s", path.c_str()));
		return false;
	}

	sourceCodeOut = sourceCode;
	return Shader_LoadSourceCodeFromString(path, sourceCodeOut);
}

bool Shader_LoadSourceCodeFromString(const std::string& path, std::string& sourceCodeOut)
{
	// Evaluate #includes recursively

	static const char* includePrefix = "#include \"";
	static const int includePrefixLength = strlen(includePrefix);

	while (const char* includeStart = strstr(sourceCodeOut.c_str(), includePrefix))
	{
		const char* includeEnd = strstr(includeStart + includePrefixLength, "\"");
		if (!includeEnd)
		{
			Log::Error(string_format("Failed to parse #include for shader file %s", path.c_str()));
			return false;
		}

		const std::string includeText = sourceCodeOut.substr((int) (includeStart - sourceCodeOut.c_str()), (int) (includeEnd - includeStart));
		const std::string includePath = includeText.substr(includePrefixLength, includeText.length() - includePrefixLength);

		std::string includeFileContent;
		if (!Shader_LoadSourceCode(includePath, includeFileContent))
		{
			Log::Error(string_format("Failed to load shader file %s included from %s", includePath.c_str(), path.c_str()));
			return false;
		}

		sourceCodeOut.replace((int) (includeStart - sourceCodeOut.c_str()), (int) (includeEnd - includeStart + 1), includeFileContent);
	}

	return true;
}

bool Shader_LoadShaderCode(const std::string& path, const std::string& entry, std::string& sourceCodeOut)
{
	// Load file content

	char* sourceCode = NULL;
	int sourceCodeSize = 0;
	if (!File_Load(path, (void*&) sourceCode, sourceCodeSize))
	{
		Log::Error(string_format("Failed to load shader from %s", path.c_str()));
		return false;
	}

	// Find source code part containing desired entry function

	static const char* splitterString = "###splitter###";
	static const int splitterStringLength = strlen(splitterString);

	char* start = sourceCode;
	char* end = sourceCode;
	bool done = false;
	while (!done)
	{
		end = strstr(start, splitterString);
		if (!end)
		{
			end = sourceCode + sourceCodeSize;
			done = true;
		}
		*end = '\0';
		if (strstr(start, entry.c_str()))
		{
			sourceCodeOut = start;
			if (entry != "main")
				string_replace_all(sourceCodeOut, entry, "main");
			return Shader_LoadSourceCodeFromString(path, sourceCodeOut);
		}

		start = end + splitterStringLength;
	}

	Log::Error(string_format("Failed to find entry %s in shader %s", entry.c_str(), path.c_str()));
	return false;
}

struct line_replace_callback
{
	int index;

	line_replace_callback(int start) :
		index(start)
	{}

	std::string Generate()
	{
		return string_format("%d: ", index++);
	}
};

Shader* Shader_Create(const std::string& path, Shader::Type type, const std::string& entry)
{
	const std::string name = path + ":" + entry;

	Shader* shader = static_cast<Shader*>(Resource_Find("shader", name));
	if (!shader)
	{
		std::string sourceCode;
		if (!Shader_LoadShaderCode(path, entry, sourceCode))
			return NULL;
#ifdef OPENGL_ES
		OpenGLES_ConvertFromOpenGL(sourceCode);
#endif

		GLuint handle = GLR(glCreateShaderObjectARB(type == Shader::Type_Vertex ? GL_VERTEX_SHADER_ARB : GL_FRAGMENT_SHADER_ARB));
		if (!handle)
		{
			Log::Error("glCreateShaderObject returned invalid handle");
			return NULL;
		}

		const char* sourceCodeChars = sourceCode.c_str();

		GL(glShaderSourceARB(handle, 1, (const GLcharARB**) &sourceCodeChars, NULL));
		GL(glCompileShaderARB(handle));

		GLint compileResult = 0;
		GL(glGetShaderiv(handle, GL_OBJECT_COMPILE_STATUS_ARB, &compileResult));
		if (!compileResult)
		{
			GLsizei logSize;
			GLcharARB log[1 << 16];
#ifdef OPENGL_ES
			GL(glGetShaderInfoLog(handle, ARRAYSIZE(log), &logSize, log));
#else
			GL(glGetInfoLogARB(handle, ARRAYSIZE(log), &logSize, log));
#endif
			std::string sourceWithLineNumbers = std::string("1: ") + sourceCode;
			line_replace_callback replaceCallback(2);
			string_replace_all_pred<line_replace_callback>(sourceWithLineNumbers, (const std::string&) std::string("\n"), replaceCallback);
			Log::Error(string_format("Failed to compile shader program %s:%s, reason:\n%s\nGLSL source code:\n%s", path.c_str(), entry.c_str(), log, sourceWithLineNumbers.c_str()));

			GL(glDeleteShader(handle));
			return NULL;
		}

		shader = new Shader();
		shader->name = name;
		shader->type = type;
		shader->handle = handle;
#ifdef DEBUG
		shader->sourceCode = sourceCode;
#endif
	}

	Resource_IncRefCount(shader);
	return shader;
}

void Shader_Destroy(Shader* shader)
{
	if (!Resource_DecRefCount(shader))
	{
		GL(glDeleteShader(shader->handle));
		delete shader;
	}
}

// Shader program

ShaderProgram* ShaderProgram_Create(const std::string& vertexShader, const std::string& vertexShaderEntry, const std::string& fragmentShader, const std::string& fragmentShaderEntry)
{
	const std::string name = "VS:" + vertexShader + ":" + vertexShaderEntry + " FS:" + fragmentShader + ":" + fragmentShaderEntry;

	ShaderProgram* program = static_cast<ShaderProgram*>(Resource_Find("shader program", name));
	if (!program)
	{
		ShaderPtr vs = Shader_Create(vertexShader, Shader::Type_Vertex, vertexShaderEntry);
		ShaderPtr fs = Shader_Create(fragmentShader, Shader::Type_Fragment, fragmentShaderEntry);
		if (!*vs || !*fs)
			return NULL;

		// Build program

		GLuint handle = GLR(glCreateProgramObjectARB());
		if (!handle)
		{
			Log::Error("glCreateProgramObject returned invalid handle");
			return NULL;
		}

		GL(glAttachObjectARB(handle, vs->handle));
		GL(glAttachObjectARB(handle, fs->handle));

		// Link program

		GL(glLinkProgramARB(handle));

		GLint linkResult;
		GL(glGetProgramiv(handle, GL_OBJECT_LINK_STATUS_ARB, &linkResult));
		if (!linkResult)
		{
			GLsizei logSize;
			GLcharARB log[1 << 16];
#ifdef OPENGL_ES
			GL(glGetShaderInfoLog(handle, ARRAYSIZE(log), &logSize, log));
#else
			GL(glGetInfoLogARB(handle, ARRAYSIZE(log), &logSize, log));
#endif
			Log::Error(string_format("Failed to link shader program %s, reason: %s", name.c_str(), log));

			GL(glDeleteProgram(handle));
			return NULL;
		}

		// Create the actual resource

		program = new ShaderProgram();
		program->name = name;
		program->vs = *vs;
		program->fs = *fs;
		program->handle = handle;

		Resource_IncRefCount(program->vs);
		Resource_IncRefCount(program->fs);

		// Collect attributes

		GL(glUseProgram(handle));

		GLint attrCount;
		GL(glGetProgramiv(handle, GL_OBJECT_ACTIVE_ATTRIBUTES_ARB, &attrCount));

		GLenum attrType;
		GLint attrNameLength;
		GLint attrSize;
		GLchar attrName[128];

		for (GLint i = 0; i < attrCount; i++)
		{
			GL(glGetActiveAttribARB(handle, i, ARRAYSIZE(attrName), &attrNameLength, &attrSize, &attrType, attrName));

#ifdef OPENGL_ES
	#define ATTRIBUTE(name) name
#else
	#define ATTRIBUTE(name) "gl_"name
#endif

			Shape::VertexUsage usage;
			GLint usageIndex;
			if (!strcmp(attrName, ATTRIBUTE("Vertex"))) { usage = Shape::VertexUsage_Position; usageIndex = 0; }
			else if (!strcmp(attrName, ATTRIBUTE("MultiTexCoord0")) || !strcmp(attrName, ATTRIBUTE("TexCoord"))) { usage = Shape::VertexUsage_TexCoord; usageIndex = 0; }
			else if (!strcmp(attrName, ATTRIBUTE("MultiTexCoord1"))) { usage = Shape::VertexUsage_TexCoord; usageIndex = 1; }
			else if (!strcmp(attrName, ATTRIBUTE("Color"))) { usage = Shape::VertexUsage_Color; usageIndex = 0; }
			else
			{
				GL(glDeleteProgram(handle));

				Log::Error(string_format("Unsupported input shader semantic (name = '%s') in program %s", attrName, name.c_str()));
				return NULL;
			}

			ShaderAttribute& attribute = vector_add(program->attributes);
			attribute.location = GLR(glGetAttribLocation(handle, attrName));
			attribute.usage = usage;
			attribute.usageIndex = usageIndex;
		}

		// Collect uniforms

		GLint uniformCount;
		GL(glGetProgramiv(handle, GL_OBJECT_ACTIVE_UNIFORMS_ARB, &uniformCount));

		GLenum uniformType;
		GLint uniformNameLength;
		GLint uniformSize;
		GLchar uniformName[128];

		GLint samplerIndex = 0;
		for (GLint i = 0; i < uniformCount; i++)
		{
			GL(glGetActiveUniformARB(handle, i, ARRAYSIZE(uniformName), &uniformNameLength, &uniformSize, &uniformType, uniformName));

			if (uniformSize != 1)
			{
				GL(glDeleteProgram(handle));
				Log::Error(string_format("Uniform variable %s in %s shader program is an array (size = %d) but arrays are not supported", uniformName, name.c_str(), uniformSize));
				return NULL;
			}

			ShaderParameter::Type type;
			GLint count;
			switch (uniformType)
			{
			case GL_INT: type = ShaderParameter::Type_Int; count = 1; break;
			case GL_FLOAT: type = ShaderParameter::Type_Float; count = 1; break;
			case 0x8b50: type = ShaderParameter::Type_Float; count = 2; break;
			case 0x8b51: type = ShaderParameter::Type_Float; count = 3; break;
			case 0x8b52: type = ShaderParameter::Type_Float; count = 4; break;
			case GL_SAMPLER_2D: type = ShaderParameter::Type_Texture; count = 1; break;
			default:
				GL(glDeleteProgram(handle));
				Log::Error(string_format("Uniform variable %s of unsupported type detected in %s shader program", uniformName, name.c_str()));
				return NULL;
			}

			ShaderParameter& parameter = vector_add(program->parameters);
			parameter.name = uniformName;
			parameter.count = count;
			parameter.type = type;

			// Assign consecutive sampler index to sampler uniform

			const GLint location = GLR(glGetUniformLocationARB(handle, uniformName));

			if (parameter.type == ShaderParameter::Type_Texture)
			{
				parameter.location = samplerIndex++;
				GL(glUniform1i(location, parameter.location));
			}
			else
				parameter.location = location;
		}

		GL(glUseProgram(0));
	}

	Resource_IncRefCount(program);
	return program;
}

void ShaderProgram_Destroy(ShaderProgram* program)
{
	if (!Resource_DecRefCount(program))
	{
		Shader_Destroy(program->vs);
		Shader_Destroy(program->fs);
		GL(glDeleteProgram(program->handle));
		delete program;
	}
}

ShaderParameter* ShaderProgram_GetParameter(ShaderProgram* program, const std::string& name)
{
	for (std::vector<ShaderParameter>::iterator it = program->parameters.begin(); it != program->parameters.end(); ++it)
		if (it->name == name)
			return &(*it);
	return NULL;
}

// MaterialObj

int Material_AddParameter(MaterialResource* resource, ShaderParameter* shaderParameter)
{
	MaterialParameter& materialParameter = vector_add(resource->parameters);
	materialParameter.shaderParameterDescription = shaderParameter;
	switch (shaderParameter->type)
	{
	case ShaderParameterDescription::Type_Int:
		memset(materialParameter.intValue, 0, sizeof(materialParameter.intValue));
		break;
	case ShaderParameterDescription::Type_Float:
		memset(materialParameter.floatValue, 0, sizeof(materialParameter.floatValue));
		break;
	case ShaderParameterDescription::Type_Texture:
		materialParameter.textureValue = NULL;
		break;
	}
	return resource->parameters.size() - 1;
}

bool Material_LoadTechnique(MaterialResource* resource, XMLNode* techniqueNode, MaterialTechnique& technique)
{
	technique.name = XMLNode_GetAttributeValue(techniqueNode, "name");

	if (const char* blending = XMLNode_GetAttributeValue(techniqueNode, "blending"))
	{
		if (!strcmp(blending, "none")) technique.blending = Shape::Blending_None;
		else if (!strcmp(blending, "additive")) technique.blending = Shape::Blending_Additive;
		else if (!strcmp(blending, "alpha")) technique.blending = Shape::Blending_Alpha;
		else if (!strcmp(blending, "default")) technique.blending = Shape::Blending_Default;
	}

	// Load shader program

	const char* vsPath = NULL;
	const char* fsPath = NULL;

	const char* vsEntry = "main";
	const char* fsEntry = "main";

	for (XMLNode* shaderNode = XMLNode_GetFirstNode(techniqueNode, "shader"); shaderNode; shaderNode = XMLNode_GetNext(shaderNode, "shader"))
	{
		const char* type = XMLNode_GetAttributeValue(shaderNode, "type");
		const char* shaderPath = XMLNode_GetAttributeValue(shaderNode, "path");
		const char* entry = XMLNode_GetAttributeValue(shaderNode, "entry");

		if (!type || !shaderPath)
		{
			Log::Error(string_format("Failed to load material %s technique %s, reason: 'shader' node doesn't specify shader 'type' and 'path' attributes", resource->name.c_str(), technique.name.c_str()));
			return false;
		}

		if (!strcmp(type, "vertex"))
		{
			vsPath = shaderPath;
			if (entry)
				vsEntry = entry;
		}
		else if (!strcmp(type, "fragment"))
		{
			fsPath = shaderPath;
			if (entry)
				fsEntry = entry;
		}
		else
		{
			Log::Error(string_format("Failed to load material %s technique %s, reason: 'shader' node specifies unsupported shader 'type' %s", resource->name.c_str(), technique.name.c_str(), type));
			return false;
		}
	}

	if (!vsPath || !fsPath)
	{
		Log::Error(string_format("Failed to load material %s technique %s, reason: material doesn't specify both vertex and fragment shaders", resource->name.c_str(), technique.name.c_str()));
		return false;
	}

	technique.shaderProgram = ShaderProgram_Create(vsPath, vsEntry, fsPath, fsEntry);
	if (!technique.shaderProgram)
	{
		Log::Error(string_format("Failed to load material %s technique %s, reason: failed to build shader program from vertex shader %s and fragment shader %s", resource->name.c_str(), technique.name.c_str(), vsPath, fsPath));
		return false;
	}

	// Update material parameters from shader parameters

	for (std::vector<ShaderParameter>::iterator it = technique.shaderProgram->parameters.begin(); it != technique.shaderProgram->parameters.end(); ++it)
	{
		int materialParameterIndex = -1;
		for (unsigned int i = 0; i < resource->parameters.size(); i++)
			if (resource->parameters[i].shaderParameterDescription->name == it->name)
			{
				MaterialParameter& materialParameter = resource->parameters[i];
				if (*materialParameter.shaderParameterDescription != *static_cast<ShaderParameterDescription*>(&(*it)))
				{
					Log::Error(string_format("Shader parameter %s in material %s exists in at least 2 different setups - once as type %d count %d and now (in shader program %s) as type %d count %d",
						it->name.c_str(), resource->name.c_str(),
						materialParameter.shaderParameterDescription->type, materialParameter.shaderParameterDescription->count,
						technique.name.c_str(),
						it->type, it->count));
					return false;
				}

				materialParameterIndex = i;
				break;
			}

		if (materialParameterIndex == -1)
			materialParameterIndex = Material_AddParameter(resource, &(*it));

		technique.materialParameterIndices.push_back(materialParameterIndex);
	}

	return true;
}

MaterialObj* Material_Clone(MaterialObj* other)
{
	MaterialObj* material = new MaterialObj();
	material->resource = other->resource;
	Resource_IncRefCount(material->resource);
	Material_SetTechnique(material, 0);
	material->screenSizeParamIndex = other->screenSizeParamIndex;
	material->projectionScaleParamIndex = other->projectionScaleParamIndex;
	return material;
}


MaterialObj* Material_Create(const std::string& name, bool immediate)
{
	immediate = immediate || !g_supportAsynchronousResourceLoading;

	MaterialResource* resource = static_cast<MaterialResource*>(Resource_Find("material", name));
	if (!resource)
	{
		const std::string path = name + ".material.xml";

		XMLDoc doc;
		if (!doc.Load(path))
		{
			Log::Error(string_format("Failed to load material from %s", path.c_str()));
			return NULL;
		}

		XMLNode* rootNode = doc.AsNode()->GetFirstNode("material");
		if (!rootNode)
		{
			Log::Error(string_format("Failed to load material %s, reason: root 'material' node not found", name.c_str()));
			return NULL;
		}

		// Create resource

		resource = new MaterialResource();
		resource->name = name;
		resource->state = ResourceState_Created;

		// Load techniques

		for (XMLNode* techniqueNode = XMLNode_GetFirstNode(rootNode, "technique"); techniqueNode; techniqueNode = XMLNode_GetNext(techniqueNode, "technique"))
			if (!Material_LoadTechnique(resource, techniqueNode, vector_add(resource->techniques)))
			{
				for (std::vector<MaterialTechnique>::iterator it = resource->techniques.begin(); it != resource->techniques.end(); ++it)
					if (it->shaderProgram)
						ShaderProgram_Destroy(it->shaderProgram);

				return NULL;
			}

		// Load default material parameter values

		// TODO
	}
	Resource_IncRefCount(resource);

	// Create material

	MaterialObj* material = new MaterialObj();
	material->resource = resource;
	Material_SetTechnique(material, 0);

	// Get screen size parameter index

	material->screenSizeParamIndex = Material_GetParameterIndex(material, "ScreenSize");
	material->projectionScaleParamIndex = Material_GetParameterIndex(material, "ProjectionScale");

	return material;
}

void Material_ReleaseTextures(MaterialBase* material)
{
	for (std::vector<MaterialParameter>::iterator it = material->parameters.begin(); it != material->parameters.end(); ++it)
		if (it->shaderParameterDescription->type == ShaderParameter::Type_Texture && it->textureValue)
		{
			Texture_Destroy(it->textureValue);
			it->textureValue = NULL;
		}
}

void Material_Destroy(MaterialObj* material)
{
	Material_ReleaseTextures(material);
	if (!Resource_DecRefCount(material->resource))
	{
		if (material->resource->state == ResourceState_CreationInProgress)
		{
			Log::Warn(string_format("Attempted to destroy the material %s that is still being loaded - waiting for all asynchronous jobs to complete", material->resource->name.c_str()));
			Jobs::WaitForAllJobs();
		}

		Material_ReleaseTextures(material->resource);
		for (std::vector<MaterialTechnique>::iterator it = material->resource->techniques.begin(); it != material->resource->techniques.end(); ++it)
			ShaderProgram_Destroy(it->shaderProgram);
		delete material->resource;
	}
	delete material;
}

MaterialParameter* Material_GetParameter(MaterialBase* material, const std::string& name)
{
	for (std::vector<MaterialParameter>::iterator it = material->parameters.begin(); it != material->parameters.end(); ++it)
		if (it->shaderParameterDescription->name == name)
			return &(*it);
	return NULL;
}

int Material_GetParameterIndex(MaterialObj* material, const std::string& name)
{
	if (material->parameters.size() == 0)
	{
		material->parameters = material->resource->parameters;
		for (unsigned int i = 0; i < material->parameters.size(); i++)
			if (material->parameters[i].shaderParameterDescription->type == ShaderParameterDescription::Type_Texture &&
				material->parameters[i].textureValue)
			{
				Resource_IncRefCount(material->parameters[i].textureValue);
			}
	}

	for (unsigned int i = 0; i < material->parameters.size(); i++)
		if (material->parameters[i].shaderParameterDescription->name == name)
			return i;

	return -1;
}

void Material_SetIntParameter(MaterialObj* material, const std::string& name, const int* value, int count)
{
	const int index = Material_GetParameterIndex(material, name);
	if (index != -1)
		Material_SetIntParameter(material, index, value, count);
}

void Material_SetIntParameter(MaterialObj* material, int index, const int* value, int count)
{
	if (index >= (int) material->parameters.size())
	{
		Log::Warn(string_format("Failed to set int parameter for material %s, reason: invalid parameter index", material->resource->name.c_str()));
		return;
	}

	MaterialParameter& parameter = material->parameters[index];
	if (parameter.shaderParameterDescription->type != ShaderParameter::Type_Int)
	{
		Log::Warn(string_format("Failed to set int parameter %s for material %s, reason: parameter type mismatch", parameter.shaderParameterDescription->name.c_str(), material->resource->name.c_str()));
		return;
	}
	if (parameter.shaderParameterDescription->count != count)
	{
		Log::Warn(string_format("Failed to set int parameter %s for material %s, reason: parameter count mismatch", parameter.shaderParameterDescription->name.c_str(), material->resource->name.c_str()));
		return;
	}

	memcpy(parameter.intValue, value, sizeof(int) * count);
}

void Material_SetFloatParameter(MaterialObj* material, const std::string& name, const float* value, int count)
{
	const int index = Material_GetParameterIndex(material, name);
	if (index != -1)
		Material_SetFloatParameter(material, index, value, count);
}

void Material_SetFloatParameter(MaterialObj* material, int index, const float* value, int count)
{
	if (index >= (int) material->parameters.size())
	{
		Log::Warn(string_format("Failed to set float parameter for material %s, reason: invalid parameter index", material->resource->name.c_str()));
		return;
	}

	MaterialParameter& parameter = material->parameters[index];
	if (parameter.shaderParameterDescription->type != ShaderParameter::Type_Float)
	{
		Log::Warn(string_format("Failed to set float parameter %s for material %s, reason: parameter type mismatch", parameter.shaderParameterDescription->name.c_str(), material->resource->name.c_str()));
		return;
	}
	if (parameter.shaderParameterDescription->count != count)
	{
		Log::Warn(string_format("Failed to set float parameter %s for material %s, reason: parameter count mismatch", parameter.shaderParameterDescription->name.c_str(), material->resource->name.c_str()));
		return;
	}

	memcpy(parameter.floatValue, value, sizeof(float) * count);
}

void Material_SetTextureParameter(MaterialObj* material, const std::string& name, TextureObj* value, const Sampler& sampler)
{
	const int index = Material_GetParameterIndex(material, name);
	if (index != -1)
		Material_SetTextureParameter(material, index, value, sampler);
}

void Material_SetTextureParameter(MaterialObj* material, int index, TextureObj* value, const Sampler& sampler)
{
	if (index >= (int) material->parameters.size())
	{
		Log::Warn(string_format("Failed to set texture parameter for material %s, reason: invalid parameter index", material->resource->name.c_str()));
		return;
	}

	MaterialParameter& parameter = material->parameters[index];
	if (parameter.shaderParameterDescription->type != ShaderParameter::Type_Texture)
	{
		Log::Warn(string_format("Failed to set texture parameter %s for material %s, reason: parameter type mismatch", parameter.shaderParameterDescription->name.c_str(), material->resource->name.c_str()));
		return;
	}

	if (parameter.textureValue != value)
	{
		if (parameter.textureValue)
			Texture_Destroy(parameter.textureValue);
		parameter.textureValue = value;
		if (parameter.textureValue)
			Resource_IncRefCount(parameter.textureValue);
	}

	parameter.sampler = sampler;
}

void Material_CommitParameter(MaterialParameter* materialParameter, ShaderParameter* shaderParameter)
{
	switch (shaderParameter->type)
	{
	case ShaderParameter::Type_Int:
		switch (shaderParameter->count)
		{
			case 1: GL(glUniform1iv(shaderParameter->location, 1, (const GLint*) materialParameter->intValue)); break;
			case 2: GL(glUniform2iv(shaderParameter->location, 1, (const GLint*) materialParameter->intValue)); break;
			case 3: GL(glUniform3iv(shaderParameter->location, 1, (const GLint*) materialParameter->intValue)); break;
			case 4: GL(glUniform4iv(shaderParameter->location, 1, (const GLint*) materialParameter->intValue)); break;
		}
		break;
	case ShaderParameter::Type_Float:
		switch (shaderParameter->count)
		{
			case 1: GL(glUniform1fv(shaderParameter->location, 1, (const GLfloat*) materialParameter->floatValue)); break;
			case 2: GL(glUniform2fv(shaderParameter->location, 1, (const GLfloat*) materialParameter->floatValue)); break;
			case 3: GL(glUniform3fv(shaderParameter->location, 1, (const GLfloat*) materialParameter->floatValue)); break;
			case 4: GL(glUniform4fv(shaderParameter->location, 1, (const GLfloat*) materialParameter->floatValue)); break;
		}
		break;
	case ShaderParameter::Type_Texture:
		g_maxTextureUnitSet = max(g_maxTextureUnitSet, shaderParameter->location);
		GL(glActiveTexture(GL_TEXTURE0 + shaderParameter->location));
		glEnableTexture2D();
		GL(glBindTexture(GL_TEXTURE_2D, materialParameter->textureValue ? materialParameter->textureValue->handle : 0));

		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, Sampler_WrapMode_ToOpenGL(materialParameter->sampler.uWrapMode)));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, Sampler_WrapMode_ToOpenGL(materialParameter->sampler.vWrapMode)));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, materialParameter->sampler.minFilterLinear ? GL_NEAREST : GL_NEAREST));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, materialParameter->sampler.magFilterLinear ? GL_LINEAR : GL_NEAREST));
#ifndef OPENGL_ES
		GL(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (const float*) &materialParameter->sampler.borderColor));
#endif

		break;
	}
}

void Material_Draw(MaterialObj* material, const Shape::DrawParams* params)
{
	if (params->geometry.numVerts == 0)
	{
		Log::Error(string_format("Failed to draw using material %s, reason: zero number of verts to draw", material->resource->name.c_str()));
		return;
	}

	// Set built-in parameters

	if (material->screenSizeParamIndex != -1)
		Material_SetFloatParameter(material, material->screenSizeParamIndex, App_GetScreenSizeMaterialParam(), 4);
	if (material->projectionScaleParamIndex != -1)
		Material_SetFloatParameter(material, material->projectionScaleParamIndex, App_GetProjectionScaleMaterialParam(), 4);

	// Get technique and program

	MaterialTechnique* technique = material->currentTechnique;
	ShaderProgram* program = technique->shaderProgram;

	// Bind vertex data

	std::vector<ShaderAttribute>& attrs = program->attributes;
	for (std::vector<ShaderAttribute>::iterator it = attrs.begin(); it != attrs.end(); ++it)
	{
		const Shape::VertexStream* stream = NULL;
		for (int i = 0; i < params->geometry.numStreams; i++)
			if (it->usage == params->geometry.streams[i].usage && it->usageIndex == params->geometry.streams[i].usageIndex)
			{
				stream = &params->geometry.streams[i];
				break;
			}
		if (!stream)
		{
			const char* vertexUsageName = NULL;
			switch (it->usage)
			{
				case Shape::VertexUsage_Position: vertexUsageName = "position"; break;
				case Shape::VertexUsage_TexCoord: vertexUsageName = "texture coordinate"; break;
				case Shape::VertexUsage_Color: vertexUsageName = "color"; break;
				default: return;
			}

			Log::Warn(string_format("Missing %s vertex stream required for %s shader program", vertexUsageName, program->name.c_str()));
			return;
		}

#ifdef OPENGL_ES
		GL(glEnableVertexAttribArrayARB(it->location));
		GL(glVertexAttribPointerARB(it->location, stream->count, GL_FLOAT, GL_FALSE, stream->stride, stream->data));
#else
		switch (it->usage)
		{
		case Shape::VertexUsage_Position:
			GL(glEnableClientState(GL_VERTEX_ARRAY));
			GL(glVertexPointer(stream->count, GL_FLOAT, stream->stride, stream->data));
			break;
		case Shape::VertexUsage_TexCoord:
			GL(glClientActiveTexture(GL_TEXTURE0 + it->usageIndex));
			GL(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
			GL(glTexCoordPointer(stream->count, GL_FLOAT, stream->stride, stream->data));
			break;
		case Shape::VertexUsage_Color:
			GL(glEnableClientState(GL_COLOR_ARRAY));
			GL(glColorPointer(stream->count, GL_FLOAT, stream->stride, stream->data));
			break;
		}
#endif
	}

	// Set shader program

	GL(glUseProgram(program->handle));

	// Commit parameters

	g_maxTextureUnitSet = -1;
	for (unsigned int i = 0; i < technique->materialParameterIndices.size(); i++)
	{
		const int index = technique->materialParameterIndices[i];
		MaterialParameter* parameter = &(material->parameters.size() ? material->parameters[index] : material->resource->parameters[index]);
		Material_CommitParameter(parameter, &technique->shaderProgram->parameters[i]);
	}

	// Set blending

	Shape_SetBlending(true, technique->blending);

	// Draw

	if (params->geometry.numIndices > 0)
		GL(glDrawElements(Shape_Geometry_Type_ToOpenGL(params->geometry.type), params->geometry.numIndices, GL_UNSIGNED_SHORT, params->geometry.indices));
	else
		GL(glDrawArrays(Shape_Geometry_Type_ToOpenGL(params->geometry.type), 0, params->geometry.numVerts));

	// Revert back to defaults

	GL(glUseProgram(0));

	for (int i = g_maxTextureUnitSet; i >= 0; i--)
	{
		GL(glActiveTexture(GL_TEXTURE0 + i));
		glDisableTexture2D();
	}

	for (std::vector<ShaderAttribute>::iterator it = attrs.begin(); it != attrs.end(); ++it)
	{
#ifdef OPENGL_ES
		GL(glDisableVertexAttribArrayARB(it->location));
#else
		switch (it->usage)
		{
		case Shape::VertexUsage_Position:
			GL(glDisableClientState(GL_VERTEX_ARRAY));
			break;
		case Shape::VertexUsage_TexCoord:
			GL(glClientActiveTexture(GL_TEXTURE0 + it->usageIndex));
			GL(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
			break;
		case Shape::VertexUsage_Color:
			GL(glDisableClientState(GL_COLOR_ARRAY));
			break;
		}
#endif
	}
}

int Material_GetTechniqueIndex(MaterialObj* material, const std::string& name)
{
	for (unsigned int i = 0; i < material->resource->techniques.size(); i++)
		if (material->resource->techniques[i].name == name)
			return i;
	return -1;
}

void Material_SetTechnique(MaterialObj* material, const std::string& name)
{
	const int index = Material_GetTechniqueIndex(material, name);
	if (index == -1)
	{
		Log::Warn(string_format("Technique %s not present in material %s", name.c_str(), material->resource->name.c_str()));
		return;
	}
	Material_SetTechnique(material, index);
}

void Material_SetTechnique(MaterialObj* material, int index)
{
	if ((int) material->resource->techniques.size() <= index)
	{
		Log::Warn(string_format("Attempted to set technique for material %s at invalid index %d", material->resource->name.c_str(), index));
		return;
	}
	material->currentTechnique = &material->resource->techniques[index];
}
