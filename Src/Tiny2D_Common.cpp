#include "Tiny2D.h"
#include "Tiny2D_Common.h"

FILE _iob[] = {*stdin, *stdout, *stderr};

extern "C" FILE * __cdecl __iob_func(void)
{
    return _iob;
}

Tiny2D::App::Callbacks* (*g_createAppCallbacksFunc)() = NULL;

void SetCreateTiny2DAppCallbacks(Tiny2D::App::Callbacks* (*func)())
{
	g_createAppCallbacksFunc = func;
}

namespace Tiny2D
{

App::Callbacks* g_app = NULL;
App::SystemInfo g_systemInfo;

std::vector<std::string> g_rootDataDirs;
std::string g_languageSymbol;
bool g_quit = false;
bool g_restartApp = false;

bool g_showMessageBoxOnWarning = false;
bool g_showMessageBoxOnError = false;
bool g_exitOnError = false;

int g_width = 0;
int g_height = 0;
int g_virtualWidth = 0;
int g_virtualHeight = 0;
bool g_isFullscreen = false;
int g_viewportLeft = 0;
int g_viewportTop = 0;
int g_viewportWidth = 0;
int g_viewportHeight = 0;

bool g_enableOnScreenDebugInfo = false;
int g_frameIndex = 0;

float g_fps = 0.0f;
int g_frameTimeIndex = 0;
float g_frameTimes[NUM_FRAMES] = {0.0f};
float g_updateTime = 0.0f;
float g_renderTime = 0.0f;

Texture g_currentRenderTarget;
Texture g_sceneRenderTarget;
Texture g_mainRenderTarget;
float g_deltaTime;

unsigned int g_keyStates[Input::Key_COUNT];
Input::MouseState g_mouse;
bool g_emulateTouchpadWithMouse = false;
Sprite g_cursorSprite;
bool g_wasTouchPressed = false;
bool g_wasTouchpadDown = false;
int g_numTouches = 0;
Input::Touch g_touches[MAX_TOUCHES];

Material g_defaultMaterial;
Font g_defaultFont;

std::string g_textureVersion;
float g_textureVersionSizeMultiplier;

bool g_supportAsynchronousResourceLoading;

// STL

std::string string_format(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	char buffer[1 << 16];
#if defined(_MSC_VER)
	_vsnprintf_s(buffer, ARRAYSIZE(buffer), ARRAYSIZE(buffer), format, args);
#else
    vsnprintf(buffer, ARRAYSIZE(buffer), format, args);
#endif

	va_end(args);

	return std::string(buffer);
}

// Color

Color Color::White(1, 1, 1, 1);
Color Color::Black(0, 0, 0, 1);
Color Color::Red(1, 0, 0, 1);
Color Color::Green(0, 1, 0, 1);
Color Color::Blue(0, 0, 1, 1);
Color Color::Yellow(1, 1, 0, 1);

// Sampler

Sampler Sampler::Default;
Sampler Sampler::DefaultPostprocess;

// Random

void Random::Seed(int seed)
{
	srand((unsigned int) seed);
}

int Random::GetInt(int minValue, int maxValue)
{
#ifdef DEBUG
	if (maxValue - minValue > RAND_MAX)
		Log::Warn(string_format("Random number range larger than max allowed range of %d", RAND_MAX));
#endif
	return minValue + (rand() % (maxValue - minValue + 1));
}

float Random::GetFloat(float minValue, float maxValue)
{
	const float normalized = (float) rand() / (float) RAND_MAX;
	return minValue + normalized * (maxValue - minValue);
}

// App

App::DisplaySettings::DisplaySettings() :
	textureVersion(""),
	textureVersionSizeMultiplier(1.0f),
	width(0),
	height(0),
	virtualWidth(0),
	virtualHeight(0),
	fullscreen(true)
{
#ifdef DESKTOP
	width = 800;
	height = 600;
#endif

#ifdef DEBUG
	fullscreen = false;
#endif
}

void App::DisplaySettings::SetiPhoneResolution()
{
	width = 480;
	height = 320;
}

void App::DisplaySettings::SetiPhoneRetinaResolution()
{
	width = 960;
	height = 640;
}

void App::DisplaySettings::SetiPadResolution()
{
	width = 1024;
	height = 768;
}

void App::DisplaySettings::SetiPadRetinaResolution()
{
	width = 2048;
	height = 1536;
}

App::StartupParams::StartupParams() :
	name("Tiny2D App"),
	languageSymbol("EN"),
	defaultMaterialName("common/default"),
	defaultFontName("common/courbd.ttf"),
	defaultFontSize(16),
	defaultCursorName("common/cursor.png"),
	showMessageBoxOnWarning(false),
	showMessageBoxOnError(false),
	exitOnError(false),
	emulateTouchpadWithMouse(false),
	supportAsynchronousResourceLoading(true)
{
#ifdef DESKTOP
	emulateTouchpadWithMouse = true;
	rootDataDirs.push_back("../../Data/");		// App data
	rootDataDirs.push_back("../../../Data/");	// Common data
#else
	rootDataDirs.push_back("");		// All data
#endif

#ifdef DEBUG
	showMessageBoxOnError = true;
	exitOnError = true;
#endif
}

const std::vector<std::string>& App::GetRootDataDirs()
{
	return g_rootDataDirs;
}

void App_UpdateFPS()
{
	g_frameTimes[(g_frameTimeIndex++) & (NUM_FRAMES - 1)] = g_deltaTime;
	g_fps = 0.0f;
	for (int i = 0; i < NUM_FRAMES; i++)
		g_fps += g_frameTimes[i];
	g_fps = 1.0f / (g_fps / (float) NUM_FRAMES);
}

void App_Update()
{
	const float jobUpdateTime = min(1.0f / 120.0f, 1.0f / 60.0f - g_deltaTime);
	Jobs::UpdateDoneJobs(jobUpdateTime);

	g_app->OnUpdate(g_deltaTime);

#ifdef DEBUG
	if (Input::WasKeyPressed(Input::Key_R) && Input::IsKeyDown(Input::Key_Alt) && Input::IsKeyDown(Input::Key_Control))
		App_Restart();
#endif
}

void App_DrawCursor()
{
	if (g_emulateTouchpadWithMouse)
	{
		const Input::MouseState& mouseState = Input::GetMouseState();

		Rect rect;
		rect.Set(
			(mouseState.position.x - (float) g_cursorSprite.GetWidth() / 2),
			(mouseState.position.y - (float) g_cursorSprite.GetHeight() / 2),
			(float) g_cursorSprite.GetHeight(),
			(float) g_cursorSprite.GetHeight());
		if (Input::IsKeyDown(Input::Key_MouseLeft))
			rect.ScaleCentered(2.0f);

		Sprite::DrawParams params;
		params.rect = &rect;
		g_cursorSprite.Draw(&params);
	}
}

void App_Draw()
{
	// Update postprocesses

	if (Postprocessing::IsOldTVEnabled())
		Postprocessing_UpdateOldTV(g_deltaTime);
	if (Postprocessing::IsQuakeEnabled())
		Postprocessing_UpdateQuake(g_deltaTime);
	if (Postprocessing::IsRainyGlassEnabled())
		Postprocessing_UpdateRainyGlass(g_deltaTime);

	// Figure out buffer to draw to

	int numPostprocessesEnabled =
		Postprocessing::IsBloomEnabled() +
		Postprocessing::IsOldTVEnabled() +
		Postprocessing::IsRainyGlassEnabled() +
		Postprocessing::IsQuakeEnabled();

	Texture scene;
	if (numPostprocessesEnabled)
		scene = RenderTexturePool::Get(g_viewportWidth, g_viewportHeight);

	// Draw the scene

	g_sceneRenderTarget = scene.GetState() == ResourceState_Created ? scene : g_mainRenderTarget;
	g_app->OnDraw(g_sceneRenderTarget);
	g_sceneRenderTarget.Destroy();

	// Draw postprocesses

#define DRAW_POSTPROCESS(name) \
	if (Postprocessing::Is##name##Enabled()) \
	{ \
		Texture output = --numPostprocessesEnabled ? RenderTexturePool::Get(g_viewportWidth, g_viewportHeight) : g_mainRenderTarget; \
		g_sceneRenderTarget = output; \
		Postprocessing_Draw##name(output, scene); \
		RenderTexturePool::Release(scene); \
		scene = output; \
	}

	DRAW_POSTPROCESS(Bloom);
	DRAW_POSTPROCESS(RainyGlass);
	DRAW_POSTPROCESS(Quake);
	DRAW_POSTPROCESS(OldTV);

	{
		Texture sceneTex(scene);
		g_app->OnDrawAfterPostprocessing(sceneTex);
	}
	App_DrawCursor();
	g_sceneRenderTarget.Destroy();
}

void App_DrawDebugInfo()
{
	const std::string statsString = string_format(
		"FPS: %.2f\n"
		"Update: %.2f ms Render: %.2f ms",
		g_fps,
		g_updateTime * 1000.0f, g_renderTime * 1000.0f);

	Shape::DrawRectangle(Rect(5, 5, 350, 45), 0, Color(0, 0, 0, 0.3f));
	g_defaultFont.Draw(statsString.c_str(), Vec2(10.0f, 10.0f));
}

void App_Run()
{
	while (!g_quit)
	{
		App_ProcessEvents();
		App_UpdateTimer();
		App_UpdateFPS();

		Timer updateTimer;
		App_Update();
		updateTimer.End();

		Timer renderTimer;
		App_Draw();
		renderTimer.End();

		if (!(g_frameIndex & 15))
		{
			g_updateTime = updateTimer.ToSeconds();
			g_renderTime = renderTimer.ToSeconds();
		}

		if (g_enableOnScreenDebugInfo)
			App_DrawDebugInfo();

		App_EndDrawFrame();

		g_frameIndex++;
	}

	Postprocessing::EnableBloom(false);
	Postprocessing::EnableOldTV(false);
	Postprocessing::EnableRainyGlass(false);
	Postprocessing_ShutdownQuake();
}

Texture& App::GetMainRenderTarget()
{
	return g_mainRenderTarget;
}

Texture& App_GetSceneRenderTarget()
{
	return g_sceneRenderTarget;
}

Texture& App::GetCurrentRenderTarget()
{
	return g_currentRenderTarget;
}

void App_SetCurrentRenderTarget(TextureObj* texture)
{
	Texture_SetHandle(texture, g_currentRenderTarget);
}

Material& App::GetDefaultMaterial()
{
	return g_defaultMaterial;
}

Font& App::GetDefaultFont()
{
	return g_defaultFont;
}

int App_MainRun(int argc, char** argv)
{
	// Create app object

	g_app = g_createAppCallbacksFunc();
	if (!g_app)
	{
		fprintf(stderr, "Failed to create app callbacks. Did you set up DEFINE_APP(yourClassName) in your project?");
		return 1;
	}

	// Startup system

	App::StartupParams params;
	if (!g_app->OnStartup(argc - 1, (const char**) (argv + 1), g_systemInfo, params))
		return 2;
	if (!App_Startup(&params))
		return 3;

	// Init app

	if (!g_app->OnInit())
		return 4;

	// Run app

	g_quit = false;
	App_Run();

	// Deinit app

	g_app->OnDeinit();
	delete g_app;

	// Shutdown system

	App_Shutdown();

	return 0;
}

int App_Main(int argc, char** argv)
{
	while (1)
	{
		g_restartApp = false;
		const int returnCode = App_MainRun(argc, argv);
		if (!g_restartApp)
			return returnCode;
	}

	return 0;
}

int App::GetWidth()
{
	return g_virtualWidth;
}

int App::GetHeight()
{
	return g_virtualHeight;
}

bool App::IsFullscreen()
{
	return g_isFullscreen;
}

const std::string& App::GetLanguageSymbol()
{
	return g_languageSymbol;
}

void Jobs::WaitForAllJobs()
{
	while (GetNumJobsInProgress())
	{
		Jobs::UpdateDoneJobs(1.0f);
		App::Sleep(0.1f);
	}
}

void Input_AddTouch(float x, float y, long long int id)
{
	Input::Touch* touch = NULL;
	int i;
	for (i = 0; i < g_numTouches; i++)
		if (g_touches[i].id == id)
		{
			touch = &g_touches[i];
			break;
		}
	if (i == MAX_TOUCHES)
		return;
	else if (!touch)
	{
		touch = &g_touches[g_numTouches++];
		touch->id = id;
	}

	touch->position.x = (((x * (float) g_width) - (float) g_viewportLeft) * g_virtualWidth / g_viewportWidth);
	touch->position.y = (((y * (float) g_height) - (float) g_viewportTop) * g_virtualHeight / g_viewportHeight);
}

void Input_RemoveTouch(float x, float y, long long int id)
{
	for (int i = 0; i < g_numTouches; i++)
		if (g_touches[i].id == id)
		{
			g_touches[i] = g_touches[--g_numTouches];
			break;
		}
}

unsigned int Input::GetKeyState(Key key)
{
	return g_keyStates[key];
}

const Input::MouseState& Input::GetMouseState()
{
	return g_mouse;
}

int Input::GetNumTouches()
{
	if (g_emulateTouchpadWithMouse)
		return Input::IsKeyDown(Input::Key_MouseLeft);

	return g_numTouches;
}

const Input::Touch& Input::GetTouch(int index)
{
	Assert(index < Input::GetNumTouches());

	if (g_emulateTouchpadWithMouse)
	{
		Input::Touch& touch = g_touches[0];
		touch.position = g_mouse.position;
		touch.id = 0;
	}

	return g_touches[index];
}

bool Input::IsTouchpadDown()
{
	return Input::GetNumTouches() > 0;
}

bool Input::WasTouchpadDown()
{
	return g_wasTouchpadDown;
}

bool Input::WasTouchpadPressed()
{
	return g_wasTouchPressed;
}

// Timer

Timer::Timer()
{
	start = Time::GetTicks();
	end = 0;
}

void Timer::End()
{
	end = Time::GetTicks();
}

float Timer::ToSeconds() const
{
	return Time::TicksToSeconds(end - start);
}

float Time::SecondsSince(Time::Ticks ticks)
{
	return Time::TicksToSeconds(Time::GetTicks() - ticks);
}

// Resource

typedef std::map<std::string, Resource*> ResourceMap;
typedef std::map<std::string, ResourceMap*> ResourceTypeMap;
ResourceTypeMap g_resources;

void Resource_ListUnfreed()
{
	for (ResourceTypeMap::iterator it = g_resources.begin(); it != g_resources.end(); ++it)
		for (ResourceMap::iterator it2 = it->second->begin(); it2 != it->second->end(); ++it2)
			Log::Warn(string_format("Unfreed %s resource %s on exit", it2->second->type, it2->second->name.c_str()));
}

ResourceMap& Resource_GetTypeMap(const char* type)
{
	ResourceMap*& resourceMap = g_resources[type];
	if (!resourceMap)
		resourceMap = new ResourceMap();
	return *resourceMap;
}

Resource* Resource_Find(const char* type, const std::string& name)
{
	return map_find(Resource_GetTypeMap(type), name);
}

void Resource_IncRefCount(Resource* resource)
{
	if (++resource->refCount == 1)
	{
		Assert(resource->name.length() > 0);
		ResourceMap& resourceMap = Resource_GetTypeMap(resource->type);
		Assert(resourceMap.find(resource->name) == resourceMap.end());
		resourceMap[resource->name] = resource;
		Log::Info(string_format("Created %s resource '%s'", resource->type, resource->name.c_str()));
	}
}

int Resource_DecRefCount(Resource* resource)
{
	Assert(resource->refCount >= 1);
	if (--resource->refCount == 0)
		Resource_GetTypeMap(resource->type).erase(resource->name);
	return resource->refCount;
}

Sampler::Sampler() :
	minFilterLinear(true),
	magFilterLinear(true),
	uWrapMode(Sampler::WrapMode_Clamp),
	vWrapMode(Sampler::WrapMode_Clamp),
	borderColor(Color::White)
{}


// Texture pool

std::vector<Texture> g_texturePool;

Texture RenderTexturePool::Get(int width, int height)
{
	for (std::vector<Texture>::iterator it = g_texturePool.begin(); it != g_texturePool.end(); ++it)
		if (it->GetWidth() == width && it->GetHeight() == height)
		{
			Texture texture = *it;
			g_texturePool.erase(it);
			return texture;
		}

	Texture handle;
	Texture_SetHandle(Texture_CreateRenderTarget(width, height), handle);
	return handle;
}

void RenderTexturePool::Release(Texture& texture)
{
	g_texturePool.push_back(texture);
	texture.Destroy();
}

void RenderTexturePool::DestroyAll()
{
	for (std::vector<Texture>::iterator it = g_texturePool.begin(); it != g_texturePool.end(); ++it)
		it->Destroy();
	g_texturePool.clear();
}

// FileObj

bool File_Load(const std::string& path, void*& data, int& size)
{
	File file;
	if (!file.Open(path, File::OpenMode_Read))
	{
		Log::Error(string_format("Failed to open file %s", path.c_str()));
		return false;
	}
	size = file.GetSize();
	data = malloc(size + 1);
	if (!data)
	{
		Log::Error(string_format("Failed to allocate %d bytes while loading file %s", size, path.c_str()));
		return false;
	}
	if (!file.Read(data, size))
	{
		free(data);
		Log::Error(string_format("Failed to load %d bytes from file %s", size, path.c_str()));
		return false;
	}
	((char*) data)[size] = '\0';

	return true;
}

// Text and Font

Text::DrawParams::DrawParams() :
	color(Color::White),
	position(0, 0),
	width(0), height(0),
	rotation(0),
	scale(1),
	horizontalAlignment(Text::HorizontalAlignment_Left),
	verticalAlignment(Text::VerticalAlignment_Top),
	drawShadow(false),
	shadowColor(Color::Black),
	shadowOffset(2, 3)
{}

void Font_CacheGlyphsFromFile(FontObj* font, const std::string& path)
{
	char* buffer = NULL;
	int bufferSize = 0;
	if (File_Load(path, (void*&) buffer, bufferSize))
	{
		Font_CacheGlyphs(font, buffer);
		free(buffer);
	}
}

void Font_CacheGlyphs(FontObj* font, const std::string& text)
{
	// Convert to UTF32

	unsigned int buffer[1024];
	unsigned int bufferSize = ARRAYSIZE(buffer);
	if (!UTF8ToUTF32((const unsigned char*) text.c_str(), text.length(), buffer, bufferSize))
		return;

	// Make sure all glyphs are present

	Font_CacheGlyphs(font, buffer, bufferSize);
}

void Font_Draw(FontObj* font, const Text::DrawParams* params)
{
	// Convert to UTF32

	unsigned int buffer[1024];
	unsigned int bufferSize = ARRAYSIZE(buffer);
	if (!UTF8ToUTF32((const unsigned char*) params->text.c_str(), params->text.length(), buffer, bufferSize))
		return;

	// Make sure all glyphs are present

	Font_CacheGlyphs(font, buffer, bufferSize);

	// Generate positions and uvs for the text

	if (!font->texture)
		return;

	const float scale = params->scale;

	std::vector<float> xy;
	std::vector<float> uv;

	float x = params->position.x;
	float y = params->position.y;
	for (unsigned int i = 0; i < bufferSize; i++)
	{
		if (buffer[i] == '\r')
			continue;
		if (buffer[i] == '\n')
		{
			y += (float) font->size * scale;
			x = params->position.x;
			continue;
		}

		Glyph* glyph = map_find(font->glyphs, buffer[i]);
		if (!glyph)
			continue;

		uv.push_back(glyph->uv.left); uv.push_back(glyph->uv.top);
		uv.push_back(glyph->uv.left + glyph->uv.width); uv.push_back(glyph->uv.top);
		uv.push_back(glyph->uv.left + glyph->uv.width); uv.push_back(glyph->uv.top + glyph->uv.height);
		uv.push_back(glyph->uv.left); uv.push_back(glyph->uv.top + glyph->uv.height);
		uv.push_back(*(uv.end() - 8)); uv.push_back(*(uv.end() - 8));
		uv.push_back(*(uv.end() - 6)); uv.push_back(*(uv.end() - 6));

		xy.push_back(x + glyph->pos.left * scale); xy.push_back(y + glyph->pos.top * scale);
		xy.push_back(x + (glyph->pos.left + glyph->pos.width) * scale); xy.push_back(y + glyph->pos.top * scale);
		xy.push_back(x + (glyph->pos.left + glyph->pos.width) * scale); xy.push_back(y + (glyph->pos.top + glyph->pos.height) * scale);
		xy.push_back(x + glyph->pos.left * scale); xy.push_back(y + (glyph->pos.top + glyph->pos.height) * scale);
		xy.push_back(*(xy.end() - 8)); xy.push_back(*(xy.end() - 8));
		xy.push_back(*(xy.end() - 6)); xy.push_back(*(xy.end() - 6));

		x += glyph->advancePos * scale;
	}

	// Determine mins and maxes of the text

	float minX = (float) INT_MAX, maxX = (float) INT_MIN, minY = (float) INT_MAX, maxY = (float) INT_MIN;
	for (unsigned int i = 0; i < xy.size(); i += 2)
	{
		minX = min(minX, xy[i]);
		maxX = max(maxX, xy[i]);
		minY = min(minY, xy[i + 1]);
		maxY = max(maxY, xy[i + 1]);
	}

	// Determine the center of the text

	float centerX = 0.0f, centerY = 0.0f;
	if (params->width == 0.0f || params->height == 0.0f)
	{
		centerX = (minX + maxX) * 0.5f;
		centerY = (minY + maxY) * 0.5f;
	}
	else
	{
		centerX = params->position.x + params->width * 0.5f;
		centerY = params->position.y + params->height * 0.5f;
	}

	// Align the text

	switch (params->horizontalAlignment)
	{
		case Text::HorizontalAlignment_Center:
		{
			const float offset = params->position.x + params->width * 0.5f - (minX + maxX) * 0.5f;
			for (unsigned int i = 0; i < xy.size(); i += 2)
				xy[i] += offset;
			break;
		}
		case Text::HorizontalAlignment_Right:
		{
			const float offset = params->position.x + params->width - maxX;
			for (unsigned int i = 0; i < xy.size(); i += 2)
				xy[i] += offset;
			break;
		}
		default:
		//case Text::HorizontalAlignment_Left:
            break;
	}

	switch (params->verticalAlignment)
	{
		case Text::VerticalAlignment_Center:
		{
			const float offset = params->position.y + params->height * 0.5f - (minY + maxY) * 0.5f;
			for (unsigned int i = 1; i < xy.size(); i += 2)
				xy[i] += offset;
			break;
		}
		case Text::VerticalAlignment_Bottom:
		{
			const float offset = params->position.y + params->height - maxY;
			for (unsigned int i = 1; i < xy.size(); i += 2)
				xy[i] += offset;
			break;
		}
		default:
		//case Text::VerticalAlignment_Top:
            break;
	}

	// Set up draw params

	Shape::DrawParams texParams;
	texParams.SetGeometryType(Shape::Geometry::Type_Triangles);
	texParams.SetNumVerts(xy.size() / 2);
	texParams.SetTexCoord(&uv[0]);

	// Draw shadow

	if (params->drawShadow)
	{
		// Offset verts for the shadow rendering

		std::vector<float> xyShadow = xy;

		for (unsigned int i = 0; i < xyShadow.size(); i += 2)
		{
			xyShadow[i] += params->shadowOffset.x;
			xyShadow[i + 1] += params->shadowOffset.y;
		}

		// Rotate the shadow text

		if (params->rotation != 0)
		{
			const float rotationSin = sinf(params->rotation);
			const float rotationCos = cosf(params->rotation);

			for (unsigned int i = 0; i < xyShadow.size(); i += 2)
				Vertex_Rotate(xyShadow[i], xyShadow[i + 1], centerX, centerY, rotationSin, rotationCos);
		}

		// Draw the shadow

		texParams.color = params->shadowColor;
		texParams.SetPosition(&xyShadow[0]);

		Texture_Draw(font->texture, &texParams);
	}

	// Rotate the text

	if (params->rotation != 0)
	{
		const float rotationSin = sinf(params->rotation);
		const float rotationCos = cosf(params->rotation);

		for (unsigned int i = 0; i < xy.size(); i += 2)
			Vertex_Rotate(xy[i], xy[i + 1], centerX, centerY, rotationSin, rotationCos);
	}

	// Draw

	texParams.color = params->color;
	texParams.SetPosition(&xy[0]);

	Texture_Draw(font->texture, &texParams);
}

void Material_DrawFullscreenQuad(MaterialObj* material)
{
	const float uv[8] =
	{
		0, 0,
		1, 0,
		1, 1,
		0, 1
	};

	float xy[8] =
	{
		-1, 1,
		1, 1,
		1, -1,
		-1, -1
	};

	if (App::GetCurrentRenderTarget() != App::GetMainRenderTarget())
		for (int i = 1; i < 8; i += 2)
			xy[i] = -xy[i];

	Shape::DrawParams params;
	params.SetNumVerts(4);
	params.SetPosition(xy);
	params.SetTexCoord(uv);

	Material_Draw(material, &params);
}

void Shape::Geometry::AddStream(const Shape::VertexStream& stream)
{
	for (int i = 0; i < numStreams; i++)
		if (streams[i].usage == stream.usage && streams[i].usageIndex == stream.usageIndex)
		{
			streams[i] = stream;
			return;
		}

	Assert(numStreams < ARRAYSIZE(streams));
	streams[numStreams++] = stream;
}

bool operator == (Texture& texture, const TextureObj* obj)
{
	return Texture_Get(texture) == obj;
}

bool operator != (Texture& texture, const TextureObj* obj)
{
	return Texture_Get(texture) != obj;
}

void Texture_SetHandle(TextureObj* obj, Texture& handle)
{
	if (obj)
		Resource_IncRefCount((Resource*) obj);
	memcpy(&handle, &obj, sizeof(TextureObj*));
}

TextureObj* Texture_Get(Texture& handle)
{
	return *(TextureObj**) (void**) &handle;
}

void Material_SetHandle(MaterialObj* obj, Material& handle)
{
	memcpy(&handle, &obj, sizeof(MaterialObj*));
}

MaterialObj* Material_Get(Material& handle)
{
	return *(MaterialObj**) (void**) &handle;
}

void HandleAssertion(const char* what, const char* fileName, int fileLine)
{
	Log::Error(string_format("Condition %s failed in %s:%d", what, fileName, fileLine));
	exit(-1);
}

void Pixels_RGBA_To_RGB(std::vector<unsigned char>& pixels)
{
	for (unsigned int src = 0, dst = 0; src < pixels.size(); src += 4, dst += 3)
		for (unsigned int i = 0; i < 3; i++)
			pixels[dst + i] = pixels[src + i];

	pixels.resize( pixels.size() / 4 * 3 );
}

};
