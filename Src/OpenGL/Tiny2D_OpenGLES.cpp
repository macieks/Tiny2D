#include "Tiny2D_OpenGL.h"

namespace Tiny2D
{

#ifdef CUSTOM_OPENGL_ES

#include "SDL.h"
#include "SDL_syswm.h"

#include "EGL/egl.h"

EGLBoolean CreateEGLContext(EGLNativeWindowType hWnd, EGLDisplay* eglDisplay,
							EGLContext* eglContext, EGLSurface* eglSurface,
							EGLint* configAttribList, EGLint* surfaceAttribList)
{
	EGLint numConfigs;
	EGLint majorVersion;
	EGLint minorVersion;
	EGLDisplay display;
	EGLContext context;
	EGLSurface surface;
	EGLConfig config;
	EGLint contextAttribs[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE, EGL_NONE
	};

	// Get display
	display = eglGetDisplay(GetDC(hWnd)); // EGL_DEFAULT_DISPLAY
	if (display == EGL_NO_DISPLAY)
		return EGL_FALSE;

	// Initialize EGL
	if (!eglInitialize(display, &majorVersion, &minorVersion))
		return EGL_FALSE;

	// Get configs
	if (!eglGetConfigs(display, NULL, 0, &numConfigs))
		return EGL_FALSE;

	// Choose config
	if (!eglChooseConfig(display, configAttribList, &config, 1, &numConfigs))
		return EGL_FALSE;

	// Create a surface
	surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)hWnd, surfaceAttribList);
	if (surface == EGL_NO_SURFACE)
		return EGL_FALSE;

	// Bind the API
	eglBindAPI(EGL_OPENGL_ES_API);

	// Create a GL context
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
	if (context == EGL_NO_CONTEXT)
		return EGL_FALSE;

	// Make the context current
	if (!eglMakeCurrent(display, surface, surface, context))
		return EGL_FALSE;

	*eglDisplay = display;
	*eglSurface = surface;
	*eglContext = context;
	return EGL_TRUE;
}

struct ESContext
{
   GLint       width;
   GLint       height;
   EGLNativeWindowType  hWnd;
   EGLDisplay  eglDisplay;
   EGLContext  eglContext;
   EGLSurface  eglSurface;
};

static ESContext es_context;

bool OpenGLES_CreateContext(SDL_Window* window, App::StartupParams* params)
{
   SDL_SysWMinfo info;
   SDL_VERSION(&info.version); // initialize info structure with SDL version info
   SDL_bool get_win_info = SDL_GetWindowWMInfo(window, &info);
   Assert(get_win_info);
   EGLNativeWindowType hWnd = info.info.win.window;

   es_context.width = params->width;
   es_context.height = params->height;
   es_context.hWnd = hWnd;

   EGLint configAttribList[] =
   {
      EGL_RED_SIZE,       8,
      EGL_GREEN_SIZE,     8,
      EGL_BLUE_SIZE,      8,
      EGL_ALPHA_SIZE,     8 /*: EGL_DONT_CARE*/,
      EGL_DEPTH_SIZE,     24,
      EGL_STENCIL_SIZE,   8,
      EGL_SAMPLE_BUFFERS, 0,
      EGL_NONE
   };
   EGLint surfaceAttribList[] =
   {
      //EGL_POST_SUB_BUFFER_SUPPORTED_NV, flags & (ES_WINDOW_POST_SUB_BUFFER_SUPPORTED) ? EGL_TRUE : EGL_FALSE,
      //EGL_POST_SUB_BUFFER_SUPPORTED_NV, EGL_FALSE,
      EGL_NONE, EGL_NONE
   };

   if (!CreateEGLContext(es_context.hWnd,
         &es_context.eglDisplay,
         &es_context.eglContext,
         &es_context.eglSurface,
         configAttribList,
         surfaceAttribList))
   {
      Log::Error("Failed to create OpenGL ES 2.0 context");
	  return false;
   }

   const char* extensions = (const char*) glGetString(GL_EXTENSIONS);

   return true;
}

void OpenGLES_DestroyContext()
{
  if (EGL_NO_CONTEXT != es_context.eglContext)
  {
     eglDestroyContext(es_context.eglDisplay, es_context.eglContext);
     es_context.eglContext = EGL_NO_CONTEXT;
  }

  if (EGL_NO_SURFACE != es_context.eglSurface)
  {
     eglDestroySurface(es_context.eglDisplay, es_context.eglSurface);
     es_context.eglSurface = EGL_NO_SURFACE;
  }

  if (EGL_NO_DISPLAY != es_context.eglDisplay)
  {
     eglTerminate(es_context.eglDisplay);
     es_context.eglDisplay = EGL_NO_DISPLAY;
  }
}

void OpenGLES_SwapWindow()
{
   eglSwapBuffers( es_context.eglDisplay, es_context.eglSurface );
}

#endif

const char* get_eof_after(std::string& code, const char* after)
{
	const char* lastHash = NULL;
	const char* ptr = code.c_str() - 1;
	while ((ptr = strstr(ptr + 1, after)))
		lastHash = ptr;
	const char* eofAfterLastHash = lastHash ? strstr(lastHash, "\n") : NULL;
	eofAfterLastHash = eofAfterLastHash ? (eofAfterLastHash + 1) : code.c_str();
	return eofAfterLastHash;
}

void insert_after_pragmas(std::string& code, const std::string& s)
{
	code.insert((int) (get_eof_after(code, "#") - code.c_str()), s);
}

void OpenGLES_ConvertFromOpenGL(std::string& code)
{
	// OpenGL (non-ES) compatibility

	const char* attributes[] =
	{
		"gl_Vertex",
		"gl_MultiTexCoord0",
		"gl_MultiTexCoord1",
		"gl_TexCoord",
		"gl_Color"
	};

	if (const char* eofAfterVersion = get_eof_after(code, "#version"))
		code = eofAfterVersion;

	for (int i = 0; i < (int) ARRAYSIZE(attributes); i++)
		if (strstr(code.c_str(), attributes[i]))
		{
			insert_after_pragmas(code, string_format("attribute vec4 %s;\r\n", attributes[i]));
			string_replace_all(code, attributes[i], attributes[i] + 3);
		}
	string_replace_all(code, "out ", "varying ");
	string_replace_all(code, "in ", "varying ");

	code = "#define GLES 1\r\n" + code;
	if (!strstr(code.c_str(), "precision mediump float") && !strstr(code.c_str(), "precision highp float"))
		code = "precision mediump float;\r\n" + code;

	// HLSL compatibility

	string_replace_all(code, "tex2D", "texture2D");
	string_replace_all(code, "float2", "vec2");
	string_replace_all(code, "float3", "vec3");
	string_replace_all(code, "float4", "vec4");
}

};
