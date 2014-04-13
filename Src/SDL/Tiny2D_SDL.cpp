#include "../OpenGL/Tiny2D_OpenGL.h"

#include <list>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <SDL_rwops.h>

#include <OGGVorbis\stb_vorbis.h>

#include <cstdlib>
#include <csignal>

#ifndef OPENGL_ES
	#define GL_PROC(type, func) type func = NULL;
	#include "../OpenGL/Tiny2D_OpenGLProcedures.h"
	#undef GL_PROC
#endif

void App_InitGLProcedures()
{
#ifndef OPENGL_ES
	#define GL_PROC(type, func) func = (type) SDL_GL_GetProcAddress(#func);
	#include "../OpenGL/Tiny2D_OpenGLProcedures.h"
	#undef GL_PROC
#endif
}
namespace Tiny2D
{

SDL_Window* g_window = NULL;
bool g_windowHasFocus = false;
bool g_mouseHasFocus = false;

SDL_mutex* g_logMutex = NULL;

GLuint g_fbo = 0;

Uint64 g_timer;
Uint64 g_timerFrequency;

std::map<int, Input::Key> g_sdlToTinyKey;

#define MAX_AUDIO_CHANNELS 16
SoundObj* g_channelChunks[MAX_AUDIO_CHANNELS] = {0};
int g_numAudioChannels = 0;
SoundObj* g_music = NULL;

void Sound_OnChannelFinished(int channel);
void Sound_OnMusicFinished();

void Jobs_Init();
void Jobs_Deinit();

bool operator == (const App::DisplayMode& a, const App::DisplayMode& b)
{
	return a.width == b.width && a.height == b.height;
}

bool App_PreStartup()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		SDL_Log(string_format("SDL_Init failed, reason: %s", SDL_GetError()).c_str());
        return false;
    }

	// Figure out native width

	g_systemInfo.nativeWidth = g_systemInfo.nativeHeight = 0;

	for (int i = 0; i < SDL_GetNumVideoDisplays(); i++)
	{
		SDL_Rect rect;
		if (SDL_GetDisplayBounds(i, &rect))
		{
			Log::Error(std::string("SDL_GetDisplayBounds failed, reason: ") + SDL_GetError());
			return false;
		}

		g_systemInfo.nativeWidth = max(g_systemInfo.nativeWidth, rect.w);
		g_systemInfo.nativeHeight = max(g_systemInfo.nativeHeight, rect.h);

		for (int j = 0; j < SDL_GetNumDisplayModes(i); j++)
		{
			SDL_DisplayMode mode;
			if (!SDL_GetDisplayMode(i, j, &mode))
			{
				App::DisplayMode dstMode;
				dstMode.width = mode.w;
				dstMode.height = mode.h;
				vector_add_unique(g_systemInfo.displayModes, dstMode);
			}
		}
	}

	// Figure out device and platform names

#if defined(__ANDROID__)
	g_systemInfo.platformName = "android";
#elif defined(__IPHONEOS__)
	g_systemInfo.platformName = "ios";
#elif defined(__WIN32__) || defined(__WIN64__)
	g_systemInfo.platformName = "windows";
#elif defined(__MACOSX__)
	g_systemInfo.platformName = "macos";
#elif defined(__LINUX__)
	g_systemInfo.platformName = "linux";
#else
	g_systemInfo.platformName = "unknown";
#endif

	return true;
}

#define App_MapKey(key, sdlKey) g_sdlToTinyKey[(int) sdlKey] = Input::key;

bool App::ModifyDisplaySettings(const App::DisplaySettings& settings)
{
	RenderTexturePool::DestroyAll();

	g_textureVersion = settings.textureVersion;
	g_textureVersionSizeMultiplier = settings.textureVersionSizeMultiplier;
	g_isFullscreen = settings.fullscreen;

	// Determine width and height of the window to create

	int width = settings.width;
	int height = settings.height;
	if (!width || !height)
	{
		width = g_systemInfo.nativeWidth;
		height = g_systemInfo.nativeHeight;
	}

	g_width = width;
	g_height = height;

	// Determine viewport position and size

	g_virtualWidth = settings.virtualWidth ? settings.virtualWidth : g_width;
	g_virtualHeight = settings.virtualHeight ? settings.virtualHeight : g_height;

	if (g_width == g_virtualWidth && g_height == g_virtualHeight)
	{
		g_viewportLeft = 0;
		g_viewportTop = 0;
		g_viewportWidth = g_width;
		g_viewportHeight = g_height;
	}
	else // Letterbox
	{
		const float widthRatio = (float) g_virtualWidth / (float) g_width;
		const float heightRatio = (float) g_virtualHeight / (float) g_height;

		if (widthRatio < heightRatio)
		{
			g_viewportHeight = g_height;
			g_viewportWidth = (int) ((float) g_virtualWidth / heightRatio);
		}
		else
		{
			g_viewportWidth = g_width;
			g_viewportHeight = (int) ((float) g_virtualHeight / widthRatio);
		}

		g_viewportLeft = (g_width - g_viewportWidth) / 2;
		g_viewportTop = (g_height - g_viewportHeight) / 2;
	}

	// Resize window

	Log::Info(string_format("Resizing window (size %dx%d viewport %dx%d fullscreen %s)", g_width, g_height, g_viewportWidth, g_viewportHeight, g_isFullscreen ? "yes" : "no"));
	SDL_SetWindowSize(g_window, g_width, g_height);

	// Change fullscreen mode

	SDL_SetWindowFullscreen(g_window, settings.fullscreen ? 1 : 0);

	// Update main render target dimensions

	TextureObj* mainRenderTarget = Texture_Get(g_mainRenderTarget);
	mainRenderTarget->width = g_width;
	mainRenderTarget->height = g_height;

	return true;
}

bool App_Startup(App::StartupParams* params)
{
	g_rootDataDirs = params->rootDataDirs;
	g_languageSymbol = params->languageSymbol;

	g_showMessageBoxOnError = params->showMessageBoxOnError;
	g_showMessageBoxOnWarning = params->showMessageBoxOnWarning;
	g_exitOnError = params->exitOnError;

	g_emulateTouchpadWithMouse = params->emulateTouchpadWithMouse;
	g_supportAsynchronousResourceLoading = params->supportAsynchronousResourceLoading;

	g_textureVersion = params->textureVersion;
	g_textureVersionSizeMultiplier = params->textureVersionSizeMultiplier;

	g_isFullscreen = params->fullscreen;

	// Initialize logging system

	g_logMutex = SDL_CreateMutex();
	Assert(g_logMutex);

	// Determine width and height of the window to create

	int left = 50;
	int top = 50;

	if (!params->width || !params->height)
	{
		params->width = g_systemInfo.nativeWidth;
		params->height = g_systemInfo.nativeHeight;
	}

	g_width = params->width;
	g_height = params->height;

	// Determine viewport position and size

	g_virtualWidth = params->virtualWidth ? params->virtualWidth : g_width;
	g_virtualHeight = params->virtualHeight ? params->virtualHeight : g_height;

	if (g_width == g_virtualWidth && g_height == g_virtualHeight)
	{
		g_viewportLeft = 0;
		g_viewportTop = 0;
		g_viewportWidth = g_width;
		g_viewportHeight = g_height;
	}
	else // Letterbox
	{
		const float widthRatio = (float) g_virtualWidth / (float) g_width;
		const float heightRatio = (float) g_virtualHeight / (float) g_height;

		if (widthRatio < heightRatio)
		{
			g_viewportHeight = g_height;
			g_viewportWidth = (int) ((float) g_virtualWidth / heightRatio);
		}
		else
		{
			g_viewportWidth = g_width;
			g_viewportHeight = (int) ((float) g_virtualHeight / widthRatio);
		}

		g_viewportLeft = (g_width - g_viewportWidth) / 2;
		g_viewportTop = (g_height - g_viewportHeight) / 2;
	}

	// Create window

	Log::Info(string_format("Creating window (size %dx%d viewport %dx%d)", g_width, g_height, g_viewportWidth, g_viewportHeight));

	g_windowHasFocus = false;
	if (!(g_window = SDL_CreateWindow(params->name.c_str(), left, top, g_width, g_height, (params->fullscreen ? SDL_WINDOW_FULLSCREEN : 0) | SDL_WINDOW_OPENGL)))
	{
		Log::Error(std::string("SDL_CreateWindow failed, reason: ") + SDL_GetError());
		return false;
	}

	// Create OpenGL context

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,        8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,      8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,       8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,      8);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,     32);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,      24);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,	1);

#if defined(OPENGL_ES) && !defined(CUSTOM_OPENGL_ES)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

#ifdef CUSTOM_OPENGL_ES
	if (!OpenGLES_CreateContext(g_window, params))
#else
	if (!SDL_GL_CreateContext(g_window))
#endif
	{
		Log::Error(std::string("Failed to create OpenGL context, reason: ") + SDL_GetError());
		return false;
	}

	Log::Info(string_format("OpenGL Version: %s", glGetString(GL_VERSION)));
	Log::Info(string_format("OpenGL Vendor: %s", glGetString(GL_VENDOR)));
	Log::Info(string_format("OpenGL Renderer: %s", glGetString(GL_RENDERER)));
	Log::Info(string_format("OpenGL Shading Language Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION)));

	const char* extensions = (const char*) glGetString(GL_EXTENSIONS);
	(void) extensions;

	App_InitGLProcedures();

#ifndef OPENGL_ES
	GL(glDisable(GL_LIGHTING));
	GL(glShadeModel(GL_SMOOTH));
#endif
	GL(glDisable(GL_DEPTH_TEST));
	GL(glDepthMask(GL_FALSE));
	GL(glDisable(GL_CULL_FACE));

	Log::Info("Creating default FBO");

	GL(glGenFramebuffersEXT(1, &g_fbo));
	if (!g_fbo)
	{
		Log::Error("Failed to create OpenGL FBO");
		return false;
	}

	TextureObj* mainRenderTarget = new TextureObj();
	mainRenderTarget->handle = 0;
	mainRenderTarget->width = g_width;
	mainRenderTarget->height = g_height;
	mainRenderTarget->refCount = 1;
	mainRenderTarget->isRenderTarget = true;

	Texture_SetHandle(mainRenderTarget, g_mainRenderTarget);

	// Set up default samplers

	Sampler::DefaultPostprocess.minFilterLinear = false;
	Sampler::DefaultPostprocess.magFilterLinear = false;

	// Initialize timer

	Log::Info("Initializing timer");

	g_timer = SDL_GetPerformanceCounter();
	g_timerFrequency = SDL_GetPerformanceFrequency();

	// Map keys to SDL keys

	Log::Info("Initializing keys");

	App_MapKey(Key_Left, SDLK_LEFT);
	App_MapKey(Key_Right, SDLK_RIGHT);
	App_MapKey(Key_Up, SDLK_UP);
	App_MapKey(Key_Down, SDLK_DOWN);
	App_MapKey(Key_Escape, SDLK_ESCAPE);
	App_MapKey(Key_Space, SDLK_SPACE);
	App_MapKey(Key_Enter, SDLK_RETURN);
	App_MapKey(Key_LeftShift, SDLK_LSHIFT);
	App_MapKey(Key_RightShift, SDLK_RSHIFT);
	App_MapKey(Key_LeftAlt, SDLK_LALT);
	App_MapKey(Key_RightAlt, SDLK_RALT);
	App_MapKey(Key_LeftControl, SDLK_LCTRL);
	App_MapKey(Key_RightControl, SDLK_RCTRL);
	App_MapKey(Key_Android_Back, SDLK_AC_BACK);
	App_MapKey(Key_Android_Home, SDLK_AC_HOME);
	App_MapKey(Key_Menu, SDLK_MENU);
	App_MapKey(Key_0, SDLK_0);
	App_MapKey(Key_1, SDLK_1);
	App_MapKey(Key_2, SDLK_2);
	App_MapKey(Key_3, SDLK_3);
	App_MapKey(Key_4, SDLK_4);
	App_MapKey(Key_5, SDLK_5);
	App_MapKey(Key_6, SDLK_6);
	App_MapKey(Key_7, SDLK_7);
	App_MapKey(Key_8, SDLK_8);
	App_MapKey(Key_9, SDLK_9);
	App_MapKey(Key_A, SDLK_a);
	App_MapKey(Key_B, SDLK_b);
	App_MapKey(Key_C, SDLK_c);
	App_MapKey(Key_D, SDLK_d);
	App_MapKey(Key_E, SDLK_e);
	App_MapKey(Key_F, SDLK_f);
	App_MapKey(Key_G, SDLK_g);
	App_MapKey(Key_H, SDLK_h);
	App_MapKey(Key_I, SDLK_i);
	App_MapKey(Key_J, SDLK_j);
	App_MapKey(Key_K, SDLK_k);
	App_MapKey(Key_L, SDLK_l);
	App_MapKey(Key_M, SDLK_m);
	App_MapKey(Key_N, SDLK_n);
	App_MapKey(Key_O, SDLK_o);
	App_MapKey(Key_P, SDLK_p);
	App_MapKey(Key_Q, SDLK_q);
	App_MapKey(Key_R, SDLK_r);
	App_MapKey(Key_S, SDLK_s);
	App_MapKey(Key_T, SDLK_t);
	App_MapKey(Key_U, SDLK_u);
	App_MapKey(Key_V, SDLK_v);
	App_MapKey(Key_W, SDLK_w);
	App_MapKey(Key_X, SDLK_x);
	App_MapKey(Key_Y, SDLK_y);
	App_MapKey(Key_Z, SDLK_z);
	memset(g_keyStates, 0, sizeof(g_keyStates));

	g_wasTouchpadDown = false;
	g_wasTouchPressed = false;

	// Initialize cursor

	Log::Info("Initializing mouse");

	SDL_ShowCursor(0);
	SDL_WarpMouseInWindow(g_window, g_width / 2, g_height / 2);
	g_mouse.position = Vec2((float) g_width / 2.0f, g_height / 2.0f);
	g_mouse.movement = Vec2(0.0f, 0.0f);

	// Initialize audio

	Log::Info("Initializing audio");

	for (g_numAudioChannels = MAX_AUDIO_CHANNELS; g_numAudioChannels >= 1; g_numAudioChannels--)
		if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, g_numAudioChannels, 4096) >= 0)
			break;

	if (g_numAudioChannels == 0)
	{
		Log::Error(std::string("Mix_OpenAudio failed, reason: ") + SDL_GetError());
		return false;
	}
	Mix_ChannelFinished(Sound_OnChannelFinished);
	Mix_HookMusicFinished(Sound_OnMusicFinished);

	// Initialize fonts

	Log::Info("Initializing fonts");

	if (TTF_Init() < 0)
	{
		Log::Error(std::string("TTF_Init failed, reason: ") + SDL_GetError());
		return false;
	}

	// Initialize job system

	Log::Info("Initializing job system");

	Jobs_Init();

	// Load default resources

	Log::Info("Loading default resources");

	g_defaultMaterial.Create(params->defaultMaterialName);
	if (g_defaultMaterial.GetState() != ResourceState_Created)
	{
		Log::Error(string_format("Failed to initialize app, reason: failed to load default material %s", params->defaultMaterialName.c_str()));
		return false;
	}

	g_defaultFont.Create(params->defaultFontName, params->defaultFontSize);
	if (g_defaultFont.GetState() != ResourceState_Created)
	{
		Log::Error(string_format("Failed to initialize app, reason: failed to load default font %s", params->defaultFontName.c_str()));
		return false;
	}
	//Font_CacheGlyphsFromFile(g_defaultFont, "common/common_characters.txt");

	if (g_emulateTouchpadWithMouse)
	{
		g_cursorSprite.Create(params->defaultCursorName);
		if (g_cursorSprite.GetState() != ResourceState_Created)
		{
			Log::Error(string_format("Failed to initialize app, reason: failed to load cursor %s", params->defaultCursorName.c_str()));
			return false;
		}
	}

	Log::Info("Initialization completed successfully");

	return true;
}

void App_Shutdown()
{
	Log::Info("Shutting down subsystems");

	Localization::UnloadAllSets();
	RenderTexturePool::DestroyAll();
	g_cursorSprite.Destroy();
	g_defaultFont.Destroy();
	g_defaultMaterial.Destroy();
	g_mainRenderTarget.Destroy();
	GL(glDeleteFramebuffersEXT(1, &g_fbo));
	Resource_ListUnfreed();
	Jobs_Deinit();
	TTF_Quit();
	Log::Info("Closing audio");
	Mix_CloseAudio();
	Log::Info("Closing window");
#ifdef CUSTOM_OPENGL_ES
	OpenGLES_DestroyContext();
#endif
	SDL_ShowCursor(1);
	SDL_DestroyWindow(g_window);
	Log::Info("Shutting down SDL");
	SDL_DestroyMutex(g_logMutex);
	SDL_Quit();
}

void App::EnableOnScreenDebugInfo(bool enable)
{
	g_enableOnScreenDebugInfo = enable;
}

#if defined(__WIN32__) || defined(__WIN64__)
	#include <Shellapi.h>
#endif

void App::RunCommand(const std::string& command)
{
#if defined(__WIN32__) || defined(__WIN64__)
	ShellExecuteA(NULL, "open", command.c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(__MACOSX__) || defined(__LINUX__)
	int ret = system(command.c_str());
	if (WIFSIGNALED(ret) && (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT))
	{
		/* SIGINT or SIGQUIT triggered */
	}
#else
	Log::Warn("App::RunCommand() not implemented on current platform");
#endif
}

void App::Quit()
{
	g_quit = true; // Exit cleanly in debug builds
}

void App_Restart()
{
	g_restartApp = true;
	g_quit = true;
}

void App_ProcessEvents()
{
	// Save old input state

	for (int i = 0; i < Input::Key_COUNT; i++)
		if (g_keyStates[i] & Input::KeyState_IsDown)
			g_keyStates[i] |= Input::KeyState_WasDown;
		else
			g_keyStates[i] &= ~Input::KeyState_WasDown;

	// Handle events

	bool justRegainedFocus = false;
	g_wasTouchPressed = false;
	g_wasTouchpadDown = Input::IsTouchpadDown();

	SDL_PumpEvents();
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
			case SDL_WINDOWEVENT:
				switch (e.window.event)
				{
//				case SDL_WINDOWEVENT_HIDDEN:
//				case SDL_WINDOWEVENT_MINIMIZED:
				case SDL_WINDOWEVENT_FOCUS_LOST:
					g_app->OnActivated(false);
					g_windowHasFocus = false;
					SDL_ShowCursor(1);
					break;
//				case SDL_WINDOWEVENT_SHOWN:
//				case SDL_WINDOWEVENT_MAXIMIZED:
//				case SDL_WINDOWEVENT_RESTORED:
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					g_app->OnActivated(true);
					g_windowHasFocus = true;
					justRegainedFocus = true;
					SDL_ShowCursor(0);
					break;
				case SDL_WINDOWEVENT_LEAVE:
					g_mouseHasFocus = false;
					break;
				case SDL_WINDOWEVENT_ENTER:
					g_mouseHasFocus = true;
					g_keyStates[Input::Key_MouseLeft] &= ~Input::KeyState_IsDown;
					g_keyStates[Input::Key_MouseMiddle] &= ~Input::KeyState_IsDown;
					g_keyStates[Input::Key_MouseRight] &= ~Input::KeyState_IsDown;
					break;
				}
				break;
			case SDL_QUIT:
				App::Quit();
				break;
			case SDL_KEYDOWN:
			{
				const int key = g_sdlToTinyKey[e.key.keysym.sym];
				g_keyStates[key] |= Input::KeyState_IsDown;
				break;
			}
			case SDL_KEYUP:
			{
				const int key = g_sdlToTinyKey[e.key.keysym.sym];
				g_keyStates[key] &= ~Input::KeyState_IsDown;
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
				switch (e.button.button)
				{
					case SDL_BUTTON_LEFT:
						g_keyStates[Input::Key_MouseLeft] |= Input::KeyState_IsDown;
						if (g_emulateTouchpadWithMouse)
							g_wasTouchPressed = true;
						break;
					case SDL_BUTTON_MIDDLE: g_keyStates[Input::Key_MouseMiddle] |= Input::KeyState_IsDown; break;
					case SDL_BUTTON_RIGHT: g_keyStates[Input::Key_MouseRight] |= Input::KeyState_IsDown; break;
                }
				break;
	        case SDL_MOUSEBUTTONUP:
				switch (e.button.button)
				{
					case SDL_BUTTON_LEFT: g_keyStates[Input::Key_MouseLeft] &= ~Input::KeyState_IsDown; break;
					case SDL_BUTTON_MIDDLE: g_keyStates[Input::Key_MouseMiddle] &= ~Input::KeyState_IsDown; break;
					case SDL_BUTTON_RIGHT: g_keyStates[Input::Key_MouseRight] &= ~Input::KeyState_IsDown; break;
                }
				break;
			case SDL_FINGERDOWN:
				Input_AddTouch(e.tfinger.x, e.tfinger.y, e.tfinger.fingerId);
				g_wasTouchPressed = true;
				break;
			case SDL_FINGERUP:
				Input_RemoveTouch(e.tfinger.x, e.tfinger.y, e.tfinger.fingerId);
				break;
        }
        break;
	}

	g_keyStates[Input::Key_Alt] = g_keyStates[Input::Key_LeftAlt] | g_keyStates[Input::Key_RightAlt];
	g_keyStates[Input::Key_Control] = g_keyStates[Input::Key_LeftControl] | g_keyStates[Input::Key_RightControl];
	g_keyStates[Input::Key_Shift] = g_keyStates[Input::Key_LeftShift] | g_keyStates[Input::Key_RightShift];

	// Update mouse

	if (g_windowHasFocus)
	{
		if (!justRegainedFocus)
		{
			int mouseX, mouseY;
			SDL_GetMouseState(&mouseX, &mouseY);

			g_mouse.movement = Vec2(
				(float) ((mouseX - g_width / 2) * g_virtualWidth / g_viewportWidth),
				(float) ((mouseY - g_height / 2) * g_virtualHeight / g_viewportHeight));

			const int cursorMinVisibility = 16;
			g_mouse.position = Vec2(
				(float) clamp(g_mouse.position.x + g_mouse.movement.x, 0.0f, (float) (g_virtualWidth - 1 - cursorMinVisibility)),
				(float) clamp(g_mouse.position.y + g_mouse.movement.y, 0.0f, (float) (g_virtualHeight - 1 - cursorMinVisibility)));
		}
		SDL_WarpMouseInWindow(g_window, g_width / 2, g_height / 2);
	}
}

void App_UpdateTimer()
{
	const float maxFrameTime = 1.0f / 10.0f;

	const Uint64 prevTimer = g_timer;
	g_timer = SDL_GetPerformanceCounter();
	g_deltaTime = min((float) ((double) (g_timer - prevTimer) / (double) g_timerFrequency), maxFrameTime);
}

void App_EndDrawFrame()
{
#ifdef CUSTOM_OPENGL_ES
	OpenGLES_SwapWindow();
#else
	SDL_GL_SwapWindow(g_window);
#endif
}

void App::ShowMessageBox(const std::string& message, MessageBoxType type)
{
	SDL_MessageBoxData data;
	switch (type)
	{
		case MessageBoxType_Warning: data.flags = SDL_MESSAGEBOX_WARNING; data.title = "Warning"; break;
		case MessageBoxType_Error: data.flags = SDL_MESSAGEBOX_ERROR; data.title = "Error"; break;
		default:
		//case MessageBoxType_Info:
		    data.flags = SDL_MESSAGEBOX_INFORMATION; data.title = "Information";
		    break;
	}
	data.message = message.c_str();
	data.window = g_window;
	data.numbuttons = 1;
	SDL_MessageBoxButtonData button;
	button.flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
	button.text = "OK";
	data.buttons = &button;

	int buttonId;
	SDL_ShowMessageBox(&data, &buttonId);
}

void App::Sleep(float seconds)
{
	SDL_Delay((int) (seconds * 1000.0f));
}

Time::Ticks Time::GetTicks()
{
	return SDL_GetPerformanceCounter();
}

float Time::TicksToSeconds(Time::Ticks ticks)
{
	return (float) ((double) ticks / (double) g_timerFrequency);
}

Time::Ticks Time::SecondsToTicks(float seconds)
{
	return (Time::Ticks) ((double) seconds * (double) g_timerFrequency);
}

bool Input::HasMouse()
{
#if defined(__WIN32__) || defined(__WIN64__) || defined(__LINUX__) || defined(__MACOSX__)
	return !g_emulateTouchpadWithMouse;
#else
	return false;
#endif
}

bool Input::HasTouchpad()
{
#if defined(__WIN32__) || defined(__WIN64__) || defined(__LINUX__) || defined(__MACOSX__)
	return g_emulateTouchpadWithMouse;
#else
	return true;
#endif
}

// Logging

#ifdef ENABLE_LOGGING

void Log::Debug(const std::string& message)
{
	SDL_LockMutex(g_logMutex);
	SDL_LogDebug(0, message.c_str());
	SDL_UnlockMutex(g_logMutex);
}

void Log::Info(const std::string& message)
{
	SDL_LockMutex(g_logMutex);
	SDL_LogInfo(0, message.c_str());
	SDL_UnlockMutex(g_logMutex);
}

void Log::Warn(const std::string& message)
{
	SDL_LockMutex(g_logMutex);
	SDL_LogWarn(0, message.c_str());
	SDL_UnlockMutex(g_logMutex);
	if (g_showMessageBoxOnWarning)
		App::ShowMessageBox(message, App::MessageBoxType_Warning);
}

void Log::Error(const std::string& message)
{
	if (g_showMessageBoxOnError)
		App::ShowMessageBox(message, App::MessageBoxType_Error);
	SDL_LockMutex(g_logMutex);
	SDL_LogError(0, message.c_str());
	SDL_UnlockMutex(g_logMutex);
	if (g_exitOnError)
		exit(4);
}

#endif

// Texture

int Texture_GetBytesPerPixel(TextureObj* texture)
{
	switch (texture->format)
	{
		case GL_RGBA: return 4;
		case GL_RGB: return 3;
		default: return 0;
	}
}

bool Texture_Save(TextureObj* texture, const std::string& path, bool saveAlpha)
{
	std::vector<unsigned char> pixels;
	if (!Texture_GetPixels(texture, pixels, !saveAlpha))
	{
		Log::Warn(string_format("Failed to save texture to '%s', reason: failed to grab pixels from OpenGL texture", path.c_str()));
		return false;
	}

	const int bytesPerPixel = (texture->hasAlpha && saveAlpha) ? 4 : 3;

	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(&pixels[0], texture->width, texture->height, bytesPerPixel * 8, texture->width * bytesPerPixel, 0x000000FF, 0x0000FF00, 0x00FF0000, saveAlpha ? 0xFF000000 : 0);
	if (!surface)
	{
		Log::Warn(string_format("Failed to save texture to '%s', reason: failed to create SDL surface (%dx%d)", path.c_str(), texture->width, texture->height));
		return false;
	}

	int result;
	if (strstr(path.c_str(), ".png"))
		result = IMG_SavePNG(surface, path.c_str());
	else if (strstr(path.c_str(), ".bmp"))
		result = SDL_SaveBMP(surface, path.c_str());
	else
	{
		Log::Warn(string_format("Failed to save texture to '%s', reason: unsupported output format", path.c_str()));
		SDL_FreeSurface(surface);
		return false;
	}

	if (result != 0)
		Log::Warn(string_format("Failed to save texture to '%s', reason: SDL save image function failed, ret code = %d", path.c_str(), result));

	SDL_FreeSurface(surface);
	return result == 0;
}

TextureObj* Texture_CreateFromSurface(TextureObj* resource, SDL_Surface* surface)
{
	GLuint handle;
	GL(glGenTextures(1, &handle));
	GL(glBindTexture(GL_TEXTURE_2D, handle));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	GLint internalFormat;
	GLenum format;
	if (surface->format->BitsPerPixel == 32)
	{
		format = GL_RGBA;
#ifdef OPENGL_ES
		internalFormat = GL_RGBA;
#else
		internalFormat = GL_RGBA8;
#endif
	}
	else if (surface->format->BitsPerPixel == 24)
	{
		format = GL_RGB;
#ifdef OPENGL_ES
		internalFormat = GL_RGB;
#else
		internalFormat = GL_RGB8;
#endif
	}
	else
	{
		GL(glDeleteTextures(1, &handle));
		Log::Error(string_format("Unsupported texture format (SDL surface format: %d bpp: %d)", surface->format->format, surface->format->BitsPerPixel));
		SDL_FreeSurface(surface);
		return NULL;
	}

	GL(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels));

	if (!resource)
		resource = new TextureObj();
	resource->handle = handle;
	resource->format = format;
	resource->internalFormat = internalFormat;
	resource->width = surface->w;
	resource->height = surface->h;
	resource->hasAlpha =
		resource->format == GL_BGRA ||
		resource->format == GL_RGBA;

	SDL_FreeSurface(surface);

	return resource;
}

struct TextureJobData
{
	TextureObj* resource;
	SDL_Surface* surface;
};

std::string Texture_TranslateName(const std::string& name)
{
	std::string translatedName = name;
	string_replace_all(translatedName, ".", g_textureVersion + ".");
	return translatedName;
}

void Texture_JobFunc(void* userData)
{
	TextureJobData* jobData = (TextureJobData*) userData;

	std::string path = Texture_TranslateName(jobData->resource->name);
	SDL_RWops* rw = File_OpenSDLFileRW(path, File::OpenMode_Read);
	if (rw)
	{
		jobData->surface = IMG_Load_RW(rw, 1);
		if (jobData->surface)
			jobData->resource->sizeScale = g_textureVersionSizeMultiplier;
	}
	else if (g_textureVersion.length())
	{
		path = jobData->resource->name;
		rw = File_OpenSDLFileRW(path, File::OpenMode_Read);
		if (!rw)
		{
			Log::Error(string_format("Failed to load texture from %s, reason: failed to open file", path.c_str(), SDL_GetError()));
			return;
		}
		jobData->surface = IMG_Load_RW(rw, 1);
	}

	if (!jobData->surface)
		Log::Error(string_format("IMG_Load failed for %s, reason: %s", path.c_str(), SDL_GetError()));
}

void Texture_DoneFunc(bool canceled, void* userData)
{
	TextureJobData* jobData = (TextureJobData*) userData;

	if (!jobData->surface || !Texture_CreateFromSurface(jobData->resource, jobData->surface))
	{
		jobData->resource->state = ResourceState_AsyncError;
		if (canceled)
			Log::Info(string_format("Texture %s async loading was canceled", jobData->resource->name.c_str()));
		else
			Log::Error(string_format("Texture %s async loading failed", jobData->resource->name.c_str()));
	}
	else
	{
		jobData->resource->state = ResourceState_Created;
		Log::Info(string_format("Texture %s finished async loading", jobData->resource->name.c_str()));
	}

	delete jobData;
}

TextureObj* Texture_Create(const std::string& name, bool immediate)
{
	immediate = immediate || !g_supportAsynchronousResourceLoading;

	TextureObj* resource = static_cast<TextureObj*>(Resource_Find("texture", name));
	if (resource)
	{
		Resource_IncRefCount(resource);
		return resource;
	}

	if (immediate)
	{
		float sizeScale = 1.0f;

		SDL_Surface* surface = NULL;

		std::string path = Texture_TranslateName(name);
		SDL_RWops* rw = File_OpenSDLFileRW(path, File::OpenMode_Read);
		if (rw)
		{
			surface = IMG_Load_RW(rw, 1);
			if (!surface)
			{
				Log::Error(string_format("IMG_Load_RW failed for %s, reason: %s", path.c_str(), SDL_GetError()));
				return NULL;
			}
			sizeScale = g_textureVersionSizeMultiplier;
		}
		else
		{
			if (g_textureVersion.length())
			{
				path = name;
				rw = File_OpenSDLFileRW(path, File::OpenMode_Read);
				if (rw)
					surface = IMG_Load_RW(rw, 1);
			}
			if (!surface)
			{
				Log::Error(string_format("IMG_Load_RW failed for %s, reason: %s", path.c_str(), SDL_GetError()));
				return NULL;
			}
		}

		resource = Texture_CreateFromSurface(NULL, surface);
		if (!resource)
			return NULL;

		resource->sizeScale = sizeScale;
		resource->isLoaded = true;
		resource->name = name;
		resource->state = ResourceState_Created;
		Resource_IncRefCount(resource);
	}
	else
	{
		resource = new TextureObj();
		resource->isLoaded = true;
		resource->name = name;
		resource->state = ResourceState_Creating;
		Resource_IncRefCount(resource);

		TextureJobData* jobData = new TextureJobData();
		jobData->resource = resource;
		resource->jobID = JobSystem::RunJob(Texture_JobFunc, Texture_DoneFunc, jobData);
	}

	return resource;
}

#if 0

TextureObj* Texture_CreateCameraCapture(Camera camera = Camera_Back, CameraCaptureSize size = CameraCaptureSize_Medium, bool captureSingleFrame = true);

TextureObj* Texture_CreateCameraCapture(Camera camera, CameraCaptureSize size, bool captureSingleFrame)
{
	SDL_Camera

	GLuint handle;
	GL(glGenTextures(1, &handle));
	GL(glBindTexture(GL_TEXTURE_2D, handle));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	GLint internalFormat;
	GLenum format;
#ifdef OPENGL_ES
		internalFormat = GL_RGB;
		format = GL_RGB;
#else
		internalFormat = GL_RGB8;
		format = GL_RGB;
#endif

	GL(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels));
}

void Texture_PauseCameraCapture(TextureObj* texture, bool pause)
{
}

#endif

// SoundObj

struct SoundResource : Resource
{
	Mix_Chunk* chunk;
	Mix_Music* music;

	SoundResource() :
		Resource("sound"),
		chunk(NULL),
		music(NULL)
	{}
};

struct SoundObj
{
	SoundResource* resource;
	int channel;
	float volume;
	bool toDestroy;

	SoundObj() :
		resource(NULL),
		channel(-1),
		volume(1.0f),
		toDestroy(false)
	{}
};

Mix_Chunk* Sound_LoadOGG(const std::string& path)
{
	// Load OGG file

	int size = 0;
	void* data = NULL;
	if (!File_Load(path, data, size))
	{
		Log::Error(string_format("Failed to load OGG file %s", path.c_str()));
		return NULL;
	}

	// Decode OGG file

	int oggChannels = 0;
	short* oggData = NULL;
	const int result = stb_vorbis_decode_memory((unsigned char*) data, size, &oggChannels, (short**) &oggData);
	if (result <= 0)
	{
		free(data);
		Log::Error(string_format("Failed to decode OGG file %s", path.c_str()));
		return NULL;
	}

	// Create SDL audio chunk out of audio data

	const int oggDataSize = result * oggChannels * sizeof(short);
	Mix_Chunk* chunk = Mix_QuickLoad_RAW((Uint8*) oggData, oggDataSize);
	if (!chunk)
	{
		stb_vorbis_free(oggData);
		free(data);
		Log::Error(std::string("Mix_QuickLoad_RAW failed for ") + path + ", reason: " + Mix_GetError());
		return NULL;
	}

	// Clean up

	free(data);

	return chunk;
}

bool SoundResource_LoadData(const std::string& name, bool isMusic, Mix_Chunk*& chunk, Mix_Music*& music)
{
	SDL_RWops* rw = File_OpenSDLFileRW(name, File::OpenMode_Read);
	if (!rw)
	{
		Log::Error(string_format("Failed to load create sound from %s, reason: failed to open file for reading", name.c_str()));
		return false;
	}

	if (isMusic)
	{
		music = Mix_LoadMUS_RW(rw, 1);
		if (!music)
		{
			Log::Error(string_format("Failed to load music from %s via Mix_LoadMUS", name.c_str()));
			return false;
		}
	}
	else
	{
		if (strstr(name.c_str(), ".ogg"))
		{
			SDL_RWclose(rw);
			chunk = Sound_LoadOGG(name);
		}
		else
			chunk = Mix_LoadWAV_RW(rw, 1);

		if (!chunk)
		{
			Log::Error(string_format("Failed to load WAV/OGG sample from %s", name.c_str()));
			return false;
		}
	}
	return true;
}

struct SoundResourceJobData
{
	SoundResource* resource;
	bool isMusic;
};

void SoundResource_JobFunc(void* userData)
{
	SoundResourceJobData* jobData = (SoundResourceJobData*) userData;
	if (!SoundResource_LoadData(jobData->resource->name, jobData->isMusic, jobData->resource->chunk, jobData->resource->music))
		Log::Error(string_format("Sound %s async loading failed", jobData->resource->name.c_str()));
}

void SoundResource_DoneFunc(bool canceled, void* userData)
{
	SoundResourceJobData* jobData = (SoundResourceJobData*) userData;
	if (jobData->resource->chunk || jobData->resource->music)
		jobData->resource->state = ResourceState_Created;
	else
	{
		jobData->resource->state = ResourceState_AsyncError;
		if (canceled)
			Log::Info(string_format("Sound %s async loading was canceled", jobData->resource->name.c_str()));
		else
			Log::Error(string_format("Sound %s async loading failed", jobData->resource->name.c_str()));
	}
	delete jobData;
}

SoundObj* Sound_Clone(SoundObj* other)
{
	SoundObj* sound = new SoundObj();
	sound->resource = other->resource;
	Resource_IncRefCount(sound->resource);
	return sound;
}

SoundObj* Sound_Create(const std::string& name, bool isMusic, bool immediate)
{
	immediate = immediate || !g_supportAsynchronousResourceLoading;

	SoundResource* resource = static_cast<SoundResource*>(Resource_Find("sound", name));
	if (!resource)
	{
		Mix_Chunk* chunk = NULL;
		Mix_Music* music = NULL;
		if (immediate && !SoundResource_LoadData(name, isMusic, chunk, music))
			return NULL;

		resource = new SoundResource();
		resource->name = name;
		resource->state = immediate ? ResourceState_Created : ResourceState_Creating;
		resource->chunk = chunk;
		resource->music = music;

		if (!immediate)
		{
			SoundResourceJobData* jobData = new SoundResourceJobData();
			jobData->resource = resource;
			jobData->isMusic = isMusic;

			resource->jobID = JobSystem::RunJob(SoundResource_JobFunc, SoundResource_DoneFunc, jobData);
		}
	}

	SoundObj* sound = new SoundObj();
	sound->resource = resource;
	Resource_IncRefCount(resource);
	return sound;
}

void Sound_Destroy(SoundObj* sound, float fadeOutTime)
{
	if (sound->channel != -1)
	{
		if (fadeOutTime == 0.0f)
			Sound_Stop(sound);
		else
		{
			Assert(fadeOutTime > 0.0f);
			sound->toDestroy = true;
			if (sound->resource->chunk)
				Mix_FadeOutChannel(sound->channel, (int) (fadeOutTime * 1000.0f));
			else
				Mix_FadeOutMusic((int) (fadeOutTime * 1000.0f));
			return;
		}
	}

	if (!Resource_DecRefCount(sound->resource))
	{
		if (sound->resource->state == ResourceState_Creating)
			JobSystem::CancelJob(sound->resource->jobID);

		if (sound->resource->chunk)
		{
			if (strstr(sound->resource->name.c_str(), ".ogg"))
				stb_vorbis_free(sound->resource->chunk->abuf);
			Mix_FreeChunk(sound->resource->chunk);
		}
		else if (sound->resource->music)
			Mix_FreeMusic(sound->resource->music);
	}
	delete sound;
}

inline int Sound_ToSDLVolume(float volume)
{
	return clamp((int) (volume * 128.0f), 0, 128);
}

void Sound_SetVolume(SoundObj* sound, float volume)
{
	sound->volume = volume;

	if (sound->channel == -1)
		return;

	if (sound->resource->chunk)
		Mix_Volume(sound->channel, Sound_ToSDLVolume(volume));
	else
		Mix_VolumeMusic(Sound_ToSDLVolume(volume));
}

void Sound_SetPitch(SoundObj* sound, float pitch)
{
#if 0 // SDL doesn't support pitch
	sound->pitch = pitch;

	if (sound->channel == -1)
		return;

	if (sound->resource->chunk)
		Mix_Pitch(sound->channel, Sound_ToSDLPitch(pitch));
	else
		Mix_PitchMusic(Sound_ToSDLPitch(pitch));
#endif
}

ResourceState Sound_GetState(SoundObj* sound)
{
	return sound->resource->state;
}

void Sound_Play(SoundObj* sound, bool loop, float fadeInTime)
{
	Sound_Stop(sound);

	if (sound->resource->chunk)
	{
		const int channel = Mix_GroupAvailable(-1);
		if (channel == -1)
			return;
		g_channelChunks[channel] = sound;
		Mix_Volume(channel, Sound_ToSDLVolume(sound->volume));

		sound->channel =
			fadeInTime == 0.0f ?
			Mix_PlayChannel(channel, sound->resource->chunk, loop ? -1 : 0) :
			Mix_FadeInChannel(channel, sound->resource->chunk, loop ? -1 : 0, (int) (fadeInTime * 1000.0f));

		Assert(channel == sound->channel);
	}
	else
	{
		Mix_VolumeMusic(Sound_ToSDLVolume(sound->volume));

		sound->channel =
			fadeInTime == 0.0f ?
			Mix_PlayMusic(sound->resource->music, loop ? -1 : 0) :
			Mix_FadeInMusic(sound->resource->music, loop ? -1 : 0, (int) (fadeInTime * 1000.0f));

		g_music = sound;
	}
}

void Sound_Stop(SoundObj* sound)
{
	if (sound->channel != -1)
	{
		if (sound->resource->chunk)
		{
			g_channelChunks[sound->channel] = NULL;
			sound->channel = -1;
			Mix_HaltChannel(sound->channel);
		}
		else
		{
			if (sound == g_music)
				g_music = NULL;
			sound->channel = -1;
			Mix_HaltMusic();
		}
	}
}

bool Sound_IsPlaying(SoundObj* sound)
{
	return sound->channel != -1;
}

void Sound_OnChannelFinished(int channel)
{
	if (SoundObj* sound = g_channelChunks[channel])
	{
		g_channelChunks[channel] = NULL;
		sound->channel = -1;
		if (sound->toDestroy)
			Sound_Destroy(sound, 0.0f);
	}
}

void Sound_OnMusicFinished()
{
	if (g_music)
	{
		g_music->channel = -1;
		if (g_music->toDestroy)
			Sound_Destroy(g_music, 0.0f);
		g_music = NULL;
	}
}

// FontObj

FontObj* Font_Create(const std::string& faceName, int size, unsigned int flags, bool immediate)
{
	immediate = immediate || !g_supportAsynchronousResourceLoading;

	const std::string name = string_format("%s:%d:%u", faceName.c_str(), size, flags);

	FontObj* resource = static_cast<FontObj*>(Resource_Find("font", name));
	if (!resource)
	{
		SDL_RWops* rw = File_OpenSDLFileRW(faceName, File::OpenMode_Read);
		if (!rw)
		{
			Log::Error(string_format("Failed to create font from %s, reason: failed to open file for reading", faceName.c_str()));
			return NULL;
		}

		TTF_Font* font = TTF_OpenFontRW(rw, 1, size);
		if (!font)
		{
			Log::Error(string_format("Failed to load font from %s, reason: %s", faceName.c_str(), SDL_GetError()));
			return NULL;
		}

		TTF_SetFontStyle(font,
			((flags & Font::Flags_Bold) ? TTF_STYLE_BOLD : 0) |
			((flags & Font::Flags_Italic) ? TTF_STYLE_ITALIC : 0) |
			((flags & Font::Flags_StrikeThrough) ? TTF_STYLE_STRIKETHROUGH : 0) |
			((flags & Font::Flags_Underlined) ? TTF_STYLE_UNDERLINE : 0));
		TTF_SetFontKerning(font, 1);

		resource = new FontObj();
		resource->state = ResourceState_Created;
		resource->font = font;
		resource->name = name;
		resource->size = size;
	}

	Resource_IncRefCount(resource);

	return resource;
}

void Font_Destroy(FontObj* font)
{
	if (!Resource_DecRefCount(font))
	{
		if (font->texture)
			Texture_Destroy(font->texture);
		TTF_CloseFont(font->font);
		delete font;
	}
}

void Font_CacheGlyphs(FontObj* font, unsigned int* buffer, int bufferSize)
{
	// Check what's missing

	std::map<unsigned int, Glyph> glyphsToBuild;

	for (int i = 0; i < bufferSize; i++)
		if (!map_find(font->glyphs, buffer[i]) && !map_find(glyphsToBuild, buffer[i]))
		{
			int minx, maxx, miny, maxy, advance;
			if (TTF_GlyphMetrics(font->font, buffer[i], &minx, &maxx, &miny, &maxy, &advance))
				continue;

			Glyph& glyph = map_add(glyphsToBuild, buffer[i]);
			glyph.code = buffer[i];
			glyph.pos.left = (float) minx;
			glyph.pos.top = (float) maxx;
			glyph.pos.width = (float) (maxx - minx);
			glyph.pos.height = (float) (maxy - miny);
			glyph.advancePos = (float) advance;
		}

	// Anything to add to cache?

	if (!glyphsToBuild.size())
		return;

	// Add all existing glyphs too

	if (font->texture)
	{
		for (std::map<unsigned int, Glyph>::iterator it = font->glyphs.begin(); it != font->glyphs.end(); ++it)
			glyphsToBuild[it->first] = it->second;
		font->glyphs.clear();

		Texture_Destroy(font->texture);
		font->texture = NULL;
	}

	// Determine optimal layout

	std::vector<RectPacker::Rect> packerRects;
	for (std::map<unsigned int, Glyph>::iterator it = glyphsToBuild.begin(); it != glyphsToBuild.end(); ++it)
	{
		Glyph& glyph = it->second;

		RectPacker::Rect& packerRect = vector_add(packerRects);
		packerRect.w = (int) glyph.pos.width;
		packerRect.h = (int) glyph.pos.height;
		packerRect.userData = &glyph;
	}

	unsigned int textureWidth, textureHeight;
	RectPacker::Solve(2, 1, true, packerRects, textureWidth, textureHeight);

	// Create surface to copy all glyphs into

	SDL_Surface* surface = SDL_CreateRGBSurface(0, textureWidth, textureHeight, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	if (!surface)
	{
		Log::Error(string_format("SDL_CreateRGBSurface failed while generating font, reason: %s", SDL_GetError()));
		return;
	}

	// Construct font texture

	SDL_Color white;
	white.r = 255;
	white.g = 255;
	white.b = 255;
	white.a = 255;

	for (std::vector<RectPacker::Rect>::iterator it = packerRects.begin(); it != packerRects.end(); ++it)
	{
		Glyph* glyph = (Glyph*) it->userData;

		glyph->uv.left = (float) it->x / (float) textureWidth;
		glyph->uv.top = (float) it->y / (float) textureHeight;
		glyph->uv.width = (float) it->w / (float) textureWidth;
		glyph->uv.height = (float) it->h / (float) textureHeight;

		const unsigned short codes[2] = {(unsigned short) glyph->code, (unsigned short) '\0' };
		SDL_Surface* glyphSurface = TTF_RenderUNICODE_Blended(font->font, codes, white);
		if (!glyphSurface)
			continue;

		int minX = INT_MAX, maxX = INT_MIN, minY = INT_MAX, maxY = INT_MIN;
		for (int y = 0; y < glyphSurface->h; y++)
		{
			for (int x = 0; x < glyphSurface->w; x++)
			{
				const unsigned char alpha = ((unsigned char*) glyphSurface->pixels)[(x + y * glyphSurface->w) * 4 + 3];

				if (alpha)
				{
					minX = min(minX, x);
					maxX = max(maxX, x);
					minY = min(minY, y);
					maxY = max(maxY, y);
				}
			}
		}

		if (it->w && it->h)
		{
			SDL_Rect glyphSrcRect;
			glyphSrcRect.x = minX;
			glyphSrcRect.y = minY;
			glyphSrcRect.w = maxX - minX + 1;
			glyphSrcRect.h = maxY - minY + 1;

			glyph->pos.left = (float) minX;
			glyph->pos.top = (float) minY;

			SDL_Rect glyphDstRect;
			glyphDstRect.x = it->x;
			glyphDstRect.y = it->y;
			glyphDstRect.w = it->w;
			glyphDstRect.h = it->h;

			SDL_BlitSurface(glyphSurface, &glyphSrcRect, surface, &glyphDstRect);
		}

		SDL_FreeSurface(glyphSurface);

		font->glyphs[glyph->code] = *glyph;
	}

	// Create texture from surface

	font->texture = Texture_CreateFromSurface(NULL, surface);
	if (!font->texture)
	{
		Log::Error(string_format("Failed to create texture from surface while (re)generating font %s", font->name.c_str()));
		return;
	}
	font->texture->refCount = 1; // Increase refcount manually to prevent texture from being added to managed resources
}

void Font_CalculateSize(FontObj* font, const Text::DrawParams* params, float& width, float& height)
{
	int widthInt, heightInt;
	const int result = TTF_SizeUTF8(font->font, params->text.c_str(), &widthInt, &heightInt);
	(void) result;
	width = (float) widthInt;
	height = (float) heightInt;
}

// Asynchronous job system

struct Job
{
	JobSystem::JobID id;
	JobSystem::JobFunc jobFunc;
	JobSystem::DoneFunc doneFunc;
	void* userData;
};

volatile bool quitJobSystem = false;
SDL_mutex* jobMutex = NULL;
SDL_Thread* thread = NULL;
std::list<Job> jobs;
std::list<Job> doneJobs;
volatile int numJobs = 0;
volatile JobSystem::JobID currentJobID = 0;

int Jobs_MainFunc(void*)
{
	while (!quitJobSystem)
	{
		// Get next job off the queue

		SDL_LockMutex(jobMutex);
		if (jobs.empty())
		{
			SDL_UnlockMutex(jobMutex);
			SDL_Delay(10);
			continue;
		}
		Job job = jobs.front();
		jobs.pop_front();
		currentJobID = job.id;
		SDL_UnlockMutex(jobMutex);

		// Do the job

		job.jobFunc(job.userData);

		// Finalize the job

		SDL_LockMutex(jobMutex);
		currentJobID = 0;
		if (job.doneFunc)
			doneJobs.push_back(job); // Transfer to done jobs queue for main thread processing
		else
			--numJobs;
		SDL_UnlockMutex(jobMutex);
	}
	return 0;
}

void Jobs_Init()
{
	quitJobSystem = false;
	jobMutex = SDL_CreateMutex();
	SDL_CreateThread(Jobs_MainFunc, "jobsys", NULL);
}

void Jobs_Deinit()
{
	quitJobSystem = true;
	int status;
	SDL_WaitThread(thread, &status);
	SDL_DestroyMutex(jobMutex);
	jobMutex = NULL;
}

void JobSystem::UpdateDoneJobs(float maxTime)
{
	const Time::Ticks maxTicks = Time::SecondsToTicks(maxTime);
	const Time::Ticks startTicks = Time::GetTicks();

	while (1)
	{
		SDL_LockMutex(jobMutex);
		if (doneJobs.empty())
		{
			SDL_UnlockMutex(jobMutex);
			break;
		}
		Job doneJob = doneJobs.front();
		doneJobs.pop_front();
		SDL_UnlockMutex(jobMutex);

		doneJob.doneFunc(false, doneJob.userData);
		--numJobs;

		const Time::Ticks currentTicks = Time::GetTicks();
		if (currentTicks - startTicks >= maxTicks)
			break;
	}
}

bool Jobs_IsJobWaiting(JobSystem::JobID id)
{
	SDL_LockMutex(jobMutex);
	if (currentJobID == id)
	{
		SDL_UnlockMutex(jobMutex);
		return true;
	}
	for (std::list<Job>::iterator it = jobs.begin(); it != jobs.end(); ++it)
		if (it->id == id)
		{
			SDL_UnlockMutex(jobMutex);
			return true;
		}
	SDL_UnlockMutex(jobMutex);
	return false;
}

void JobSystem::WaitForJob(JobID id)
{
	while (Jobs_IsJobWaiting(id))
		App::Sleep(0.1f);

	SDL_LockMutex(jobMutex);
	for (std::list<Job>::iterator it = doneJobs.begin(); it != doneJobs.end(); ++it)
		if (it->id == id)
		{
			Job job = *it;
			doneJobs.erase(it);
			SDL_UnlockMutex(jobMutex);
			job.doneFunc(false, job.userData);
			--numJobs;
			return;
		}
	SDL_UnlockMutex(jobMutex);
}

void JobSystem::CancelJob(JobID id)
{
	SDL_LockMutex(jobMutex);
	if (currentJobID == id)
	{
		SDL_UnlockMutex(jobMutex);
		JobSystem::WaitForJob(id);
		return;
	}
	for (std::list<Job>::iterator it = jobs.begin(); it != jobs.end(); ++it)
		if (it->id == id)
		{
			Job job = *it;
			jobs.erase(it);
			SDL_UnlockMutex(jobMutex);
			if (job.doneFunc)
				job.doneFunc(true, job.userData);
			--numJobs;
			return;
		}
	for (std::list<Job>::iterator it = doneJobs.begin(); it != doneJobs.end(); ++it)
		if (it->id == id)
		{
			Job job = *it;
			doneJobs.erase(it);
			SDL_UnlockMutex(jobMutex);
			job.doneFunc(false, job.userData);
			--numJobs;
			return;
		}
	SDL_UnlockMutex(jobMutex);
}

int JobSystem::GetNumJobsInProgress()
{
	return numJobs;
}

JobSystem::JobID Jobs_GenerateNewJobID()
{
	static JobSystem::JobID id = 1;
	return ++id;
}

JobSystem::JobID JobSystem::RunJob(JobSystem::JobFunc jobFunc, JobSystem::DoneFunc doneFunc, void* userData)
{
	Job job;
	job.jobFunc = jobFunc;
	job.doneFunc = doneFunc;
	job.userData = userData;
	job.id = Jobs_GenerateNewJobID();

	SDL_LockMutex(jobMutex);
	jobs.push_back(job);
	++numJobs;
	SDL_UnlockMutex(jobMutex);

	return job.id;
}

// File

FileObj* File_Open(const std::string& name, File::OpenMode openMode)
{
	return (FileObj*) File_OpenSDLFileRW(name, openMode);
}

void File_Close(FileObj* file)
{
	SDL_RWclose((SDL_RWops*) file);
}

int File_GetSize(FileObj* file)
{
	return (int) SDL_RWsize((SDL_RWops*) file);
}

void File_Seek(FileObj* file, int offset)
{
	SDL_RWseek((SDL_RWops*) file, offset, RW_SEEK_SET);
}

int File_GetOffset(FileObj* file)
{
	return (int) SDL_RWtell((SDL_RWops*) file);
}

bool File_Read(FileObj* file, void* dst, int size)
{
	return (int) SDL_RWread((SDL_RWops*) file, dst, size, 1) == 1;
}

bool File_Write(FileObj* file, const void* src, int size)
{
	return (int) SDL_RWwrite((SDL_RWops*) file, src, size, 1) == 1;
}

SDL_RWops* File_OpenSDLFileRW(const std::string& name, File::OpenMode openMode)
{
	const std::vector<std::string>& rootDirs = App::GetRootDataDirs();

	for (std::vector<std::string>::const_iterator it = rootDirs.begin(); it != rootDirs.end(); ++it)
	{
		const std::string fullPath = *it + name;
		SDL_RWops* f = SDL_RWFromFile(fullPath.c_str(), openMode == File::OpenMode_Read ? "rb" : "wb");
		if (f)
			return f;
	}
	return NULL;
}

};

// Main

int main(int argc, char** argv)
{
	if (!Tiny2D::App_PreStartup())
		return -1;

	return Tiny2D::App_Main(argc, argv);
}
