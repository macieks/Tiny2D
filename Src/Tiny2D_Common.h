#ifndef TINY_2D_COMMON
#define TINY_2D_COMMON

#include <vector>
#include <map>
#include <math.h>
#if defined(WIN32)
	#include <shlwapi.h>
#endif

#ifndef _MSC_VER
    #include <stdlib.h>
    #include <stdio.h>
	#ifndef INT_MAX
		#define INT_MAX         2147483647
		#define INT_MIN         (-2147483647 - 1)
	#endif
#endif

struct SDL_RWops;

extern "C"
{
	typedef struct _TTF_Font TTF_Font;
};

namespace Tiny2D
{
	#define PI 3.141592654f
	#ifndef ARRAYSIZE
		#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
	#endif

	#ifndef min
		template <typename TYPE>
		inline TYPE min(TYPE a, TYPE b) { return a < b ? a : b; }
		template <typename TYPE>
		inline TYPE max(TYPE a, TYPE b) { return a > b ? a : b; }
	#endif

	// Utils

	class Timer
	{
	private:
		Time::Ticks start;
		Time::Ticks end;
	public:
		Timer();
		void End();
		float ToSeconds() const;
	};

	template <typename TYPE>
	inline TYPE clamp(TYPE value, TYPE minValue, TYPE maxValue)
	{
		return value < minValue ? minValue : (value > maxValue ? maxValue : value);
	}

	std::string string_format(const char* format, ...);

	inline std::string string_from_bool(bool value)
	{
		return value ? "true" : "false";
	}

	inline bool string_to_bool(const std::string& s, bool& result)
	{
		if (s == "true") { result = true; return true; }
		if (s == "false") { result = false; return true; }
		return false;
	}

	inline std::string string_from_int(int value)
	{
		char buffer[32];
		sprintf(buffer, "%d", value);
		return std::string(buffer);
	}

	inline bool string_to_int(const std::string& s, int& result)
	{
		return sscanf(s.c_str(), "%d", &result) == 1;
	}

	inline std::string string_from_float(float value)
	{
		char buffer[32];
		sprintf(buffer, "%f", value);
		return std::string(buffer);
	}

	inline bool string_to_float(const std::string& s, float& result)
	{
		return sscanf(s.c_str(), "%f", &result) == 1;
	}

	inline int string_replace_all(std::string& s, const std::string& src, const std::string& dst)
	{
		int numReplacements = 0;
		size_t lookHere = 0;
		size_t foundHere;
		while ((foundHere = s.find(src, lookHere)) != std::string::npos)
		{
			s.replace(foundHere, src.size(), dst);
			lookHere = foundHere + dst.size();
			numReplacements++;
		}
		return numReplacements;
	}

	inline void string_split(std::vector<std::string>& tokens, const std::string& str, const std::string& delimiters = " ")
	{
		tokens.clear();

		// Skip delimiters at beginning.
		std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
		// Find first "non-delimiter".
		std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

		while (std::string::npos != pos || std::string::npos != lastPos)
		{
			// Found a token, add it to the vector.
			tokens.push_back(str.substr(lastPos, pos - lastPos));
			// Skip delimiters.  Note the "not_of"
			lastPos = str.find_first_not_of(delimiters, pos);
			// Find next "non-delimiter"
			pos = str.find_first_of(delimiters, lastPos);
		}
	}

	template <class REPLACEMENT_PRED>
	inline int string_replace_all_pred(std::string& s, const std::string& src, REPLACEMENT_PRED& repl)
	{
		int numReplacements = 0;
		size_t lookHere = 0;
		size_t foundHere;
		while ((foundHere = s.find(src, lookHere)) != std::string::npos)
		{
			const std::string dst = repl.Generate();
			s.replace(foundHere, src.size(), dst);
			lookHere = foundHere + dst.size();
			numReplacements++;
		}
		return numReplacements;
	}

	template <typename TYPE>
	inline TYPE& vector_add(std::vector<TYPE>& container)
	{
		TYPE dummy;
		container.push_back(dummy);
		return container.back();
	}

	template <typename TYPE>
	inline void vector_add_unique(std::vector<TYPE>& container, TYPE& value)
	{
		for (typename std::vector<TYPE>::iterator it = container.begin(); it != container.end(); ++it)
			if (*it == value)
				return;
		container.push_back(value);
	}

	template <typename TYPE>
	void vector_remove_at(std::vector<TYPE>& container, unsigned int index)
	{
		container[index] = container.back();
		container.pop_back();
	}

	template <typename KEY_TYPE, typename VALUE_TYPE>
	inline VALUE_TYPE& map_add(std::map<KEY_TYPE, VALUE_TYPE>& container, const KEY_TYPE& key)
	{
		return container[key];
	}

	template <typename KEY_TYPE, typename VALUE_TYPE>
	inline VALUE_TYPE* map_find(std::map<KEY_TYPE, VALUE_TYPE*>& container, const KEY_TYPE& key)
	{
	#if 0
		std::map<KEY_TYPE, VALUE_TYPE*>::iterator it = container.find(key);
		return it == container.end() ? NULL : it->second;
	#else
		return container.find(key) == container.end() ? NULL : container.find(key)->second;
	#endif
	}

	template <typename KEY_TYPE, typename VALUE_TYPE>
	inline VALUE_TYPE* map_find(std::map<KEY_TYPE, VALUE_TYPE>& container, const KEY_TYPE& key)
	{
	#if 0
		std::map<KEY_TYPE, VALUE_TYPE>::iterator it = container.find(key);
		return it == container.end() ? NULL : &it->second;
	#else
		return container.find(key) == container.end() ? NULL : &container.find(key)->second;
	#endif
	}

	inline void Vertex_Rotate(float& x, float& y, float centerX, float centerY, float rotationSin, float rotationCos)
	{
		x -= centerX;
		y -= centerY;

		const float xRotated = rotationCos * x - rotationSin * y;
		const float yRotated = rotationCos * y + rotationSin * x;

		x = xRotated;
		y = yRotated;

		x += centerX;
		y += centerY;
	}

	// UTF

	bool UTF8ToUTF32(const unsigned char* src, unsigned int srcSize, unsigned int* dst, unsigned int& dstSize);

	// Rect

	struct RectI
	{
		int left, top, width, height;
	};

	//! General purpose multi-channel texture rectangle packer (can pack different rectangles into different texture channels)
	class RectPacker
	{
	public:
		struct Rect
		{
			unsigned int x;
			unsigned int y;
			unsigned int w;
			unsigned int h;

			unsigned int layer;

			void* userData;

			struct Cmp
			{
				inline bool operator () (const Rect& a, const Rect& b) const
				{
					return b.w < a.w || (b.w == a.w && b.h < a.h);
				}
			};
		};

		//! Packs set of rectangles into possibly smallest rectangle
		static void Solve(unsigned int spaceBetweenRects, unsigned int numLayers, bool pow2Dim, std::vector<Rect>& rects, unsigned int& width, unsigned int& height);
		//! Packs set of rectangles into fixed size rectangle
		static bool SolveFixedRect(unsigned int spaceBetweenRects, unsigned int numLayers, std::vector<Rect>& rects, unsigned int width, unsigned int height);
	};

	// App

	extern bool g_supportAsynchronousResourceLoading;

	extern App::Callbacks* g_app;

	extern std::vector<std::string> g_rootDataDirs;
	extern std::string g_languageSymbol;
	extern bool g_quit;
	extern bool g_restartApp;

	extern bool g_showMessageBoxOnWarning;
	extern bool g_showMessageBoxOnError;
	extern bool g_exitOnError;

	extern int g_width;
	extern int g_height;
	extern bool g_isFullscreen;
	extern int g_virtualWidth;
	extern int g_virtualHeight;
	extern int g_viewportLeft;
	extern int g_viewportTop;
	extern int g_viewportWidth;
	extern int g_viewportHeight;

	extern bool g_enableOnScreenDebugInfo;
	extern int g_frameIndex;

	extern float g_fps;
	#define NUM_FRAMES 16
	extern int g_frameTimeIndex;
	extern float g_frameTimes[NUM_FRAMES];
	extern float g_updateTime;
	extern float g_renderTime;

	extern Texture g_currentRenderTarget;
	extern Texture g_sceneRenderTarget;
	extern Texture g_mainRenderTarget;
	extern float g_deltaTime;

	extern unsigned int g_keyStates[Input::Key_COUNT];
	extern Input::MouseState g_mouse;
	extern bool g_emulateTouchpadWithMouse;
	extern Sprite g_cursorSprite;
	extern bool g_wasTouchPressed;
	extern bool g_wasTouchpadDown;
	extern int g_numTouches;
	#define MAX_TOUCHES 10
	extern Input::Touch g_touches[MAX_TOUCHES];

	extern Material g_defaultMaterial;
	extern Font g_defaultFont;

	extern std::string g_textureVersion;
	extern float g_textureVersionSizeMultiplier;
	extern App::SystemInfo g_systemInfo;

	extern bool g_supportAsynchronousResourceLoading;

	int App_Main(int argc, char** argv);
	bool App_Startup(App::StartupParams* params);
	void App_Shutdown();
	void App_Restart();
	void App_Run();
	void App_ProcessEvents();
	void App_UpdateTimer();
	void App_UpdateFPS();
	void App_Update();
	void App_Draw();
	void App_EndDrawFrame();
	const float* App_GetScreenSizeMaterialParam();
	const float* App_GetProjectionScaleMaterialParam();
	Texture& App_GetSceneRenderTarget();

	// Texture

	bool			operator == (Texture& texture, const TextureObj* obj);
	bool			operator != (Texture& texture, const TextureObj* obj);
	ResourceState	Texture_GetState(TextureObj* texture);
	void			Texture_SetHandle(TextureObj* obj, Texture& handle);
	TextureObj*		Texture_Get(Texture& handle);
	TextureObj*		Texture_Create(const std::string& name, bool immediate = true);
	TextureObj*		Texture_CreateRenderTarget(int width, int height);
	void			Texture_Destroy(TextureObj* texture);
	bool			Texture_Save(TextureObj* texture, const std::string& path, bool saveAlpha = false);
	int				Texture_GetBytesPerPixel(TextureObj* texture);
	void			Texture_Draw(TextureObj* texture, const Shape::DrawParams* params, const Sampler& sampler = Sampler::Default);
	void			Texture_Draw(TextureObj* texture, const Vec2& position, float rotation = 0.0f, float scale = 1.0f, const Color& color = Color::White);
	int				Texture_GetWidth(TextureObj* texture);
	int				Texture_GetHeight(TextureObj* texture);
	int				Texture_GetRealWidth(TextureObj* texture);
	int				Texture_GetRealHeight(TextureObj* texture);
	bool			Texture_GetPixels(TextureObj* texture, std::vector<unsigned char>& pixels, bool removeAlpha = true);
	void			Texture_BeginDrawing(TextureObj* texture, const Color* clearColor = NULL);
	void			Texture_Clear(TextureObj* texture, const Color& color = Color::Black);
	void			Texture_EndDrawing(TextureObj* texture);

	// Material

	void			Material_SetHandle(MaterialObj* material, Material& handle);
	ResourceState	Material_GetState(MaterialObj* material);
	MaterialObj*	Material_Get(Material& handle);
	MaterialObj*	Material_Create(const std::string& name, bool immediate = true);
	MaterialObj*	Material_Clone(MaterialObj* material);
	void			Material_Destroy(MaterialObj* material);
	int				Material_GetTechniqueIndex(MaterialObj* material, const std::string& name);
	void			Material_SetTechnique(MaterialObj* material, int index);
	void			Material_SetTechnique(MaterialObj* material, const std::string& name);
	int				Material_GetParameterIndex(MaterialObj* material, const std::string& name);
	void			Material_SetIntParameter(MaterialObj* material, int index, const int* value, int count = 1);
	void			Material_SetIntParameter(MaterialObj* material, const std::string& name, const int* value, int count = 1);
	void			Material_SetFloatParameter(MaterialObj* material, int index, const float* value, int count = 1);
	void			Material_SetFloatParameter(MaterialObj* material, const std::string& name, const float* value, int count = 1);
	void			Material_SetTextureParameter(MaterialObj* material, int index, TextureObj* value, const Sampler& sampler = Sampler::Default);
	void			Material_SetTextureParameter(MaterialObj* material, const std::string& name, TextureObj* value, const Sampler& sampler = Sampler::Default);
	void			Material_Draw(MaterialObj* material, const Shape::DrawParams* params);
	void			Material_DrawFullscreenQuad(MaterialObj* material);

	FontObj*		Font_Create(const std::string& path, int size, unsigned int flags = 0, bool immediate = true);
	void			Font_Destroy(FontObj* font);
	void			Font_CacheGlyphs(FontObj* font, const std::string& text);
	void			Font_CacheGlyphsFromFile(FontObj* font, const std::string& path);
	void			Font_Draw(FontObj* font, const Text::DrawParams* params);
	inline void		Font_Draw(FontObj* font, const char* text, const Vec2& position, const Color& color = Color::White) { Text::DrawParams params; params.text = text; params.position = position; params.color = color; Font_Draw(font, &params); }
	void			Font_CalculateSize(FontObj* font, const Text::DrawParams* params, float& width, float& height);

	SpriteObj*		Sprite_Create(const std::string& name, bool immediate = true);
	SpriteObj*		Sprite_Clone(SpriteObj* sprite);
	void			Sprite_Destroy(SpriteObj* sprite);
	void			Sprite_SetEventCallback(SpriteObj* sprite, Sprite::EventCallback callback, void* userData);
	void			Sprite_Update(SpriteObj* sprite, float deltaTime);
	void			Sprite_PlayAnimation(SpriteObj* sprite, const std::string& name = std::string() /* default animation */, Sprite::AnimationMode mode = Sprite::AnimationMode_Loop, float transitionTime = 0);
	void			Sprite_Draw(SpriteObj* sprite, const Sprite::DrawParams* params);
	inline void		Sprite_Draw(SpriteObj* sprite, const Vec2& position, float rotation = 0) { Sprite::DrawParams params; params.position = position; params.rotation = rotation; Sprite_Draw(sprite, &params); }
	int				Sprite_GetWidth(SpriteObj* sprite);
	int				Sprite_GetHeight(SpriteObj* sprite);

	FileObj*		File_Open(const std::string& path, File::OpenMode openMode);
	void			File_Close(FileObj* file);
	int				File_GetSize(FileObj* file);
	void			File_Seek(FileObj* file, int offset);
	int				File_GetOffset(FileObj* file);
	bool			File_Read(FileObj* file, void* dst, int size);
	bool			File_Write(FileObj* file, const void* src, int size);
	bool			File_Load(const std::string& path, void*& dst, int& size);
	SDL_RWops*		File_OpenSDLFileRW(const std::string& path, File::OpenMode openMode);

	XMLDocumentObj*		XMLDocument_Load(const std::string& path);
	XMLDocumentObj*		XMLDocument_Create(const std::string& version = "1.0", const std::string& encoding = "utf-8");
	bool			XMLDocument_Save(XMLDocumentObj* doc, const std::string& path);
	void			XMLDocument_Destroy(XMLDocumentObj* doc);
	XMLNode*		XMLDocument_AsNode(XMLDocumentObj* doc);
	XMLNode*		XMLNode_GetNext(XMLNode* node, const char* name = NULL);
	const char*		XMLNode_GetName(const XMLNode* node);
	const char*		XMLNode_GetValue(const XMLNode* node);
	XMLNode*		XMLNode_GetFirstNode(XMLNode* node, const char* name = NULL);
	int				XMLNode_CalcNumNodes(XMLNode* node, const char* name = NULL);
	XMLAttribute*	XMLNode_GetFirstAttribute(XMLNode* node, const char* name = NULL);
	const char*		XMLNode_GetAttributeValue(XMLNode* node, const char* name);
	bool			XMLNode_GetAttributeValueBool(XMLNode* node, const char* name, bool& result);
	bool			XMLNode_GetAttributeValueFloat(XMLNode* node, const char* name, float& result);
	bool			XMLNode_GetAttributeValueFloatArray(XMLNode* node, const char* name, float* result, int count);
	bool			XMLNode_GetAttributeValueInt(XMLNode* node, const char* name, int& result);
	const char*		XMLNode_GetAttributeValue(XMLNode* node, const char* name, const char* defaultValue);
	void			XMLNode_GetAttributeValueBool(XMLNode* node, const char* name, bool& result, const bool defaultValue);
	void			XMLNode_GetAttributeValueFloat(XMLNode* node, const char* name, float& result, const float defaultValue);
	void			XMLNode_GetAttributeValueInt(XMLNode* node, const char* name, int& result, const int defaultValue);
	XMLNode*		XMLNode_AddNode(XMLNode* node, const char* name);
	XMLAttribute*	XMLNode_AddAttribute(XMLNode* node, const char* name, const char* value);
	XMLAttribute*	XMLNode_AddAttributeBool(XMLNode* node, const char* name, bool value);
	XMLAttribute*	XMLNode_AddAttributeInt(XMLNode* node, const char* name, int value);
	XMLAttribute*	XMLNode_AddAttributeFloat(XMLNode* node, const char* name, float value);
	XMLAttribute*	XMLAttribute_GetNext(XMLAttribute* attr);
	const char*		XMLAttribute_GetName(const XMLAttribute* attr);
	const char*		XMLAttribute_GetValue(const XMLAttribute* attr);

	SoundObj*		Sound_Create(const std::string& path, bool isMusic = false, bool immediate = true);
	ResourceState	Sound_GetState(SoundObj* sound);
	SoundObj*		Sound_Clone(SoundObj* sound);
	void			Sound_Destroy(SoundObj* sound, float fadeOutTime = 0.0f);
	void			Sound_SetVolume(SoundObj* sound, float volume);
	void			Sound_SetPitch(SoundObj* sound, float pitch);
	void			Sound_Play(SoundObj* sound, bool loop = false, float fadeInTime = 0.2f);
	void			Sound_Stop(SoundObj* sound);
	bool			Sound_IsPlaying(SoundObj* sound);

	EffectObj*		Effect_Create(const std::string& path, const Vec2& pos, float rotation = 0.0f, float scale = 1.0f, bool immediate = true);
	EffectObj*		Effect_Clone(EffectObj* effect);
	ResourceState	Effect_GetState(EffectObj* effect);
	void			Effect_Destroy(EffectObj* effect);
	void			Effect_SetPosition(EffectObj* effect, const Vec2& pos);
	void			Effect_SetRotation(EffectObj* effect, float rotation);
	void			Effect_SetScale(EffectObj* effect, float scale);
	void			Effect_SetSpawnCountMultiplier(EffectObj* effect, float multiplier);
	bool			Effect_Update(EffectObj* effect, float deltaTime); // Returns false if destroyed
	void			Effect_Draw(EffectObj* effect);

	// Input

	void Input_AddTouch(float x, float y, long long int id);
	void Input_RemoveTouch(float x, float y, long long int id);

	void Pixels_RGBA_To_RGB(std::vector<unsigned char>& pixels);

	// Base resource

	struct Resource
	{
		const char* type;
		std::string name;
		int refCount;
		ResourceState state;
		JobSystem::JobID jobID;

		Resource(const char* _type) :
			type(_type),
			refCount(0),
			state(ResourceState_Uninitialized),
			jobID(0)
		{}
	};

	void Resource_ListUnfreed();
	Resource* Resource_Find(const char* type, const std::string& name);
	void Resource_IncRefCount(Resource* resource);
	int Resource_DecRefCount(Resource* resource);

	// Font

	struct Glyph
	{
		unsigned int code;
		Rect pos;
		float advancePos;
		Rect uv;
	};

	struct FontObj : Resource
	{
		TTF_Font* font;
		int size;
		TextureObj* texture;
		std::map<unsigned int, Glyph> glyphs;

		FontObj() :
			Resource("font"),
			font(NULL),
			texture(NULL)
		{}
	};

	void Font_CacheGlyphs(FontObj* font, unsigned int* buffer, int bufferSize);

	// Sprite

	struct SpriteResource : Resource
	{
		struct Frame
		{
			Texture texture;
			Rect rectangle; // Coordinates within texture atlas
		};
		struct Event
		{
			float time;
			std::string name;
			std::string value;
		};
		struct Animation
		{
			std::string name;
			float frameTime;
			float totalTime;
			std::vector<Frame> frames;
			std::vector<Event> events;
		};
		bool hasAtlas;
		Texture atlas;
		std::map<std::string, Animation> animations;
		Animation* defaultAnimation;
		int width;
		int height;
		Material material;

		SpriteResource() :
			Resource("sprite"),
			hasAtlas(false),
			width(0),
			height(0)
		{}
	};

	struct SpriteObj
	{
		struct AnimationInstance
		{
			SpriteResource::Animation* animation;
			float time;
			float weight;
			float weightChangeSpeed;
			Sprite::AnimationMode mode;
		};

		SpriteResource* resource;
		std::vector<AnimationInstance> animationInstances;
		Sprite::EventCallback callback;
		void* userData;

		SpriteObj() :
			resource(NULL),
			callback(NULL),
			userData(NULL)
		{}

	};

	void App_SetCurrentRenderTarget(TextureObj* texture);

	void Shape_SetBlending(bool primitiveIsTranslucent, Shape::Blending blending);

	void Postprocessing_DrawBloom(Texture& output, Texture& scene);

	void Postprocessing_DrawOldTV(Texture& output, Texture& scene);
	void Postprocessing_UpdateOldTV(float deltaTime);

	void Postprocessing_ShutdownQuake();
	void Postprocessing_DrawQuake(Texture& output, Texture& scene);
	void Postprocessing_UpdateQuake(float deltaTime);

	void Postprocessing_DrawRainyGlass(Texture& renderTarget, Texture& scene);
	void Postprocessing_UpdateRainyGlass(float deltaTime);
};

#endif // TINY_2D_COMMON
