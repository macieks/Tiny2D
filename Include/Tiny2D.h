/*
  Tiny2D Library (version 1.0)
  Copyright (C) 2014 Maciej Sawitus <contact@pixelelephant.com>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the final product is required.
	 It would also be nice, but is not required, if you let me know via
	 email that you use my library.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#ifndef TINY_2D
#define TINY_2D

#include "Tiny2D_Base.h"

//! Tiny2D game library namespace
namespace Tiny2D
{
	//! File handle
	class File
	{
	public:
		//! File open modes
		enum OpenMode
		{
			OpenMode_Read = 0,	//!< File is to be opened for reading only
			OpenMode_Write,		//!< File is to be opened for writing only

			OpenMode_COUNT
		};

		//! Constructs empty file handle
		File();
		//! Destructs file handle
		~File();
		//! Opens file at given path in given mode
		bool	Open(const std::string& path, OpenMode openMode);
		//! Closes file
		void	Close();
		//! Gets opened file size (only valid after file has been successfully opened)
		int		GetSize();
		//! Seeks to given offset within a file (relative to file beginning)
		void	Seek(int offset);
		//! Gets current file offset
		int		GetOffset();
		//! Reads 'size' number of bytes from file into 'dst' buffer; returns true on success
		bool	Read(void* dst, int size);
		//! Writes 'size' number of bytes from 'src' buffer to file; returns true on success
		bool	Write(const void* src, int size);
	private:
		File(const File&);
		void operator = (const File&);
		FileObj* file;
	};

	//! XML attribute
	class XMLAttribute
	{
	public:
		//! Gets next attribute
		XMLAttribute*	GetNext();
		//! Gets attribute name
		const char*		GetName() const;
		//! Gets attribute value
		const char*		GetValue() const;
	private:
		XMLAttribute();
		XMLAttribute(const XMLAttribute&);
		void operator = (const XMLAttribute&);
	};

	//! XML node
	class XMLNode
	{
	public:
		//! Gets next node of given name (or just next node if name is NULL)
		XMLNode*		GetNext(const char* name = NULL);
		//! Gets node name
		const char*		GetName() const;
		//! Gets node value
		const char*		GetValue() const;
		//! Gets first node of given name (or just first node if name is NULL) in a given parent node
		XMLNode*		GetFirstNode(const char* name = NULL);
		//! Gets number of nodes with given name (or just all nodes if name is NULL) in a given parent node
		int				CalcNumNodes(const char* name = NULL);
		//! Gets first attribute of given name (or just first attribute if name is NULL) in a given parent node
		XMLAttribute*	GetFirstAttribute(const char* name = NULL);
		//! Gets string value of the first attribute of given name in a given parent node; returns NULL if not found
		const char*		GetAttributeValue(const char* name);
		//! Gets bool value of the first attribute of given name in a given parent node; returns true on success
		bool			GetAttributeValueBool(const char* name, bool& result);
		//! Gets float value of the first attribute of given name in a given parent node; returns true on success
		bool			GetAttributeValueFloat(const char* name, float& result);
		//! Gets an array of floats (space separates) attribute value of the first attribute of given name in a given parent node; returns true on success
		bool			GetAttributeValueFloatArray(const char* name, float* result, int count);
		//! Gets int value of the first attribute of given name in a given parent node; returns true on success
		bool			GetAttributeValueInt(const char* name, int& result);
		//! Gets string value of the first attribute of given name in a given parent node; returns given default value if not found
		const char*		GetAttributeValue(const char* name, const char* defaultValue);
		//! Gets bool value of the first attribute of given name in a given parent node; returns given default value if not found
		void			GetAttributeValueBool(const char* name, bool& result, const bool defaultValue);
		//! Gets float value of the first attribute of given name in a given parent node; returns given default value if not found
		void			GetAttributeValueFloat(const char* name, float& result, const float defaultValue);
		//! Gets int value of the first attribute of given name in a given parent node; returns given default value if not found
		void			GetAttributeValueInt(const char* name, int& result, const int defaultValue);
		//! Adds child node to a given node
		XMLNode*		AddNode(const char* name);
		//! Adds attribute to given node
		XMLAttribute*	AddAttribute(const char* name, const char* value);
		//! Adds bool attribute to given node
		XMLAttribute*	AddAttributeBool(const char* name, bool value);
		//! Adds int attribute to given node
		XMLAttribute*	AddAttributeInt(const char* name, int value);
		//! Adds float attribute to given node
		XMLAttribute*	AddAttributeFloat(const char* name, float value);
	private:
		XMLNode();
		XMLNode(const XMLNode&);
		void operator = (const XMLNode&);
	};

	//! XML document either loaded from path or created at runtime
	class XMLDoc
	{
	public:
		//! Creates empty XML document
		XMLDoc();
		//! Destroys XML document
		~XMLDoc();
		//! Loads XML document from given path
		bool Load(const std::string& path);
		//! Creates XML document with just root node containing version and encoding information
		bool Create(const std::string& version = "1.0", const std::string& encoding = "utf-8");
		//! Saves XML document to file at given path
		bool Save(const std::string& path);
		//! Destroys XML document
		void Destroy();
		//! Gets this XML document as an XML node
		XMLNode* AsNode();
	private:
		XMLDoc(XMLDoc&);
		void operator = (XMLDoc&);
		XMLDocObj* doc;
	};

	//! Renderable shape utilities
	class Shape
	{
	public:
		//! Blending modes
		enum Blending
		{
			Blending_Default = 0,	//!< Defaults to (SrcAlpha, 1-ScrAlpha) for transparent textures; otherwise disabled
			Blending_None,			//!< Blending disabled
			Blending_Additive,		//!< Additive blending (One, One)
			Blending_Alpha,			//!< Standard alpha-blending mode using alpha channel to perform blending (SrcAlpha, 1-ScrAlpha)

			Blending_COUNT
		};

		//! Vertex format
		enum VertexFormat
		{
			VertexFormat_Float32 = 0,	//!< 32-bit float
			VertexFormat_UInt8,			//!< Unsigned 8-bit integer

			VertexFormat_COUNT
		};

		//! Vertex usage
		enum VertexUsage
		{
			VertexUsage_Position = 0,	//!< Position
			VertexUsage_TexCoord,		//!< Texture coordinate
			VertexUsage_Color,			//!< Color

			VertexUsage_COUNT
		};

		//! Vertex stream decription
		struct VertexStream
		{
			const void* data;		//!< Vertex data
			VertexFormat format;	//!< Vertex format
			int count;				//!< Number of vertices
			bool isNormalized;		//!< Is vertex format normalized? (either to -1..1 or 0..1)
			VertexUsage usage;		//!< Vertex usage
			int usageIndex;			//!< Vertex usage index
			int stride;				//!< Vertex stride (difference in bytes between the beginning of 2 vertices)

			//! Constructs empty vertex stream
			VertexStream();
			//! Constructs vertex stream
			VertexStream(const void* data, VertexFormat format, int count, bool isNormalized, VertexUsage usage, int usageIndex, int stride);
		};

		//! Shape vertex (and optionally index) data
		struct Geometry
		{
			//! Geometry types
			enum Type
			{
				Type_TriangleFan = 0,	//!< Triangle fan
				Type_Triangles,			//!< Triangle list
				Type_Lines,				//!< Line list

				Type_COUNT
			};

			Type type;					//!< Geometry primitive type; defaults to Type_TriangleFan
			int numVerts;				//!< Number of vertices; defaults to 0
			int numStreams;				//!< Number of vertex streams; defaults to 0
			VertexStream streams[16];	//!< Vertex streams
			int numIndices;				//!< Number of indices; defaults to 0
			const unsigned short* indices;	//!< Index data; defaults to NULL

			//! Constucts empty geometry
			Geometry();
			//! Sets indices
			inline void SetIndices(int numIndices, const unsigned short* indices) { this->numIndices = numIndices; this->indices = indices; }
			//! Sets number of vertices
			inline void SetNumVerts(int numVerts) { this->numVerts = numVerts; }
			//! Sets position data
			inline void SetPosition(const void* data, int stride = sizeof(float) * 2, VertexFormat format = VertexFormat_Float32, int count = 2) { AddStream(VertexStream(data, format, count, false, VertexUsage_Position, 0, stride)); }
			//! Sets texture coordinate data for a specific stream (usage index)
			inline void SetTexCoord(const void* data, int usageIndex = 0, int stride = sizeof(float) * 2, VertexFormat format = VertexFormat_Float32, int count = 2) { AddStream(VertexStream(data, format, count, false, VertexUsage_TexCoord, usageIndex, stride)); }
			//! Sets color data for a specific stream (usage index)
			inline void SetColor(const void* data, int usageIndex = 0, int stride = sizeof(float) * 4, VertexFormat format = VertexFormat_Float32, int count = 2) { AddStream(VertexStream(data, format, count, false, VertexUsage_Color, usageIndex, stride)); }
			//! Adds vertex data stream
			void AddStream(const VertexStream& stream);
		};

		//! Shape drawing parameters
		struct DrawParams
		{
			Color color;					//!< Color set to 'Color' material parameter just before rendering; defaults to Color::White
			Blending blending;				//!< Blending; defaults to Blending_Default; not used when rendering using Material (in that case blending is taken from technique blending specified in material XML)
			Geometry geometry;				//!< Shape geometry data

			//! Constructs empty (no vertex or index data) draw parameters
			DrawParams();
			//! Sets geometry type
			inline void SetGeometryType(Geometry::Type type) { geometry.type = type; }
			//! Sets indices (optional - only when using indexed geometry)
			inline void SetIndices(int numIndices, const unsigned short* indices) { geometry.SetIndices(numIndices, indices); }
			//! Sets number of vertices
			inline void SetNumVerts(int numVerts) { geometry.numVerts = numVerts; }
			//! Sets position data; stride indicates number of bytes between 2 consecutive vertices
			inline void SetPosition(const void* data, int stride = sizeof(float) * 2, VertexFormat format = VertexFormat_Float32, int count = 2) { geometry.SetPosition(data, stride, format, count); }
			//! Sets texture coordinate data for a particular usage (index); stride indicates number of bytes between 2 consecutive vertices
			inline void SetTexCoord(const void* data, int usageIndex = 0, int stride = sizeof(float) * 2, VertexFormat format = VertexFormat_Float32, int count = 2) { geometry.SetTexCoord(data, usageIndex, stride, format, count); }
			//! Sets color data for a particular usage (index); stride indicates number of bytes between 2 consecutive vertices
			inline void SetColor(const void* data, int usageIndex = 0, int stride = sizeof(float) * 4, VertexFormat format = VertexFormat_Float32, int count = 4) { geometry.SetColor(data, usageIndex, stride, format, count); }
			//! Adds vertex stream data
			inline void AddStream(const VertexStream& stream) { geometry.AddStream(stream); }
		};

		//! Draws untextured shape
		static void			Draw(const DrawParams* params);
		//! Draws rectangle
		static void			DrawRectangle(const Rect& rect, float rotation = 0.0f, const Color& color = Color::White);
		//! Draws circle
		static void			DrawCircle(const Vec2& center, float radius, int numSegments = 12, float rotation = 0.0f, const Color& color = Color::White);
		//! Draws the line
		static void			DrawLine(const Vec2& start, const Vec2& end, const Color& color = Color::White);
		//! Draws the lines; expects 'numLines' * 2 elements in 'points' array
		static void			DrawLines(const Vec2* points, int numLines, const Color& color = Color::White);
	};

	//! Texture sampler description
	struct Sampler
	{
		//! Texture wrap modes
		enum WrapMode
		{
			WrapMode_Clamp = 0,		//!< Clamps to edges
			WrapMode_ClampToBorder,	//!< Clamps to border color (specified via Sampler::borderColor); note: not supported on OpenGL ES platforms (falls back to WrapMode_Clamp)
			WrapMode_Repeat,		//!< Repeats
			WrapMode_Mirror,		//!< Mirrors repeatedly

			WrapMode_COUNT
		};

		static Sampler Default;				//!< Default sampler (for use with 2D sprite style graphics)
		static Sampler DefaultPostprocess;	//!< Default sampler for use with postprocessing (both min and mag linear filtering disabled)

		bool minFilterLinear;	//!< Indicates whether minimization filter shall be linear (nearest otherwise); defaults to true
		bool magFilterLinear;	//!< Indicates whether maximization filter shall be linear (nearest otherwise); defaults to true
		WrapMode uWrapMode;		//!< U wrapping mode; defaults to WrapMode_Clamp
		WrapMode vWrapMode;		//!< V wrapping mode; defaults to WrapMode_Clamp
		Color borderColor;		//!< Border color; defaults to Color::White; unused on mobile platforms (OpenGL ES does not support WrapMode_ClampToBorder)

		//! Constructs default sampler
		Sampler();
		//! Sets minimization and magnification filtering to linear (true) or nearest (false)
		inline void SetFiltering(bool minLinear, bool magLinear) { minFilterLinear = minLinear; magFilterLinear = magLinear; }
		//! Sets wrap mode for U and V texture coordinates; also sets optional borderColor
		inline void SetWrapMode(WrapMode u, WrapMode v, const Color& borderColor = Color::White) { uWrapMode = u; vWrapMode = v; this->borderColor = borderColor; }
	};

	//! Possible state of resource objects
	enum ResourceState
	{
		ResourceState_Uninitialized = 0,	//!< Resource hasn't been initialized yet (via Create() function)
		ResourceState_Created,				//!< Resource has been successfully created and can now be used
		ResourceState_Creating,				//!< Resource is being (asynchronously) created
		ResourceState_AsyncError,			//!< Resource asynchronous initialization failed (only returned on asynchronous failure if Create() function returned true)

		ResourceState_COUNT
	};

	//! Texture handle (optionally render target too)
	class Texture
	{
	public:
		//! Creates an empty texture
		Texture();
		//! Copy constructor; internally only increases texture reference count
		Texture(const Texture& other);
		//! Destroys the texture if not done before
		~Texture();
		//! Gets resource state
		ResourceState GetState() const;
		//! Copy operator; internally only increases texture reference count
		void operator = (const Texture& other);
		//! Equality operator
		bool operator == (const Texture& other) const;
		//! Inequality operator
		bool operator != (const Texture& other) const;
		//! Creates texture of given name; 'immediate' set to true indicates to load it synchronously
		bool Create(const std::string& path, bool immediate = true);
		//! Creates texture that can also be used as a render target; created texture has RGBA components
		bool CreateRenderTarget(int width, int height);
		//! Destroys texture
		void Destroy();
		//! Saves texture under folowing path; supported formats are: PNG and BMP; by default alpha channel is not saved even if present
		bool Save(const std::string& path, bool saveAlpha = false);
		//! Draws texture with given shape parameters
		void Draw(const Shape::DrawParams* params, const Sampler& sampler = Sampler::Default);
		//! Draws texture
		void Draw(const Vec2& position = Vec2(0.0f, 0.0f), float rotation = 0.0f, float scale = 1.0f, const Color& color = Color::White);
		//! Draws result of blending between 2 textures (useful when rendering animated objects)
		static void DrawBlended(Texture& texture0, Texture& texture1, const Shape::DrawParams* params, float scale);
		//! Gets texture width (note: applies fake scaling if the texture was loaded as part of non-default texture version set - see App::DisplayParameters::textureVersionSizeMultiplier)
		int GetWidth();
		//! Gets texture height (note: applies fake scaling if the texture was loaded as part of non-default texture version set - see App::DisplayParameters::textureVersionSizeMultiplier)
		int	GetHeight();
		//! Gets the actual texture width in pixels
		int GetRealWidth();
		//! Gets teh actual texture height in pixels
		int	GetRealHeight();
		//! Gets data of all of the texture pixels; every pixel is described by 3 or 4 values (Red Green Blue and optionally Alpha); returned pixels array has GetRealWidth() * GetRealHeight() * (3 or 4) values
		bool GetPixels(std::vector<unsigned char>& pixels, bool removeAlpha = true);
		//! Begins rendering to texture; only works for render targets; optional clearColor specifies color to which to clear render target
		void BeginDrawing(const Color* clearColor = NULL);
		//! Clears texture to given color; only works for render targets
		void Clear(const Color& color = Color::Black);
		//! Ends rendering to this texture; only works for render targets
		void EndDrawing();
	private:
		TextureObj* obj;
	};

	//! Material instance with support for techniques and shaders
	class Material
	{
	public:
		//! Constructs an empty material
		Material();
		//! Copy constructor; internally creates new material instance object (with custom set of material parameter values) and increases reference count of the material resource
		Material(const Material& other);
		//! Destroys material
		~Material();
		//! Gets resource state
		ResourceState GetState() const;
		//! Copy operator; internally creates new material instance object (with custom set of material parameter values) and increases reference count of the material resource
		void operator = (const Material& other);
		//! Equality operator
		bool operator == (const Material& other) const;
		//! Inequality operator
		bool operator != (const Material& other) const;
		//! Creates material from a material file; expects name.material.xml file to be present; 'immediate' set to true indicates to load it synchronously
		bool Create(const std::string& name, bool immediate = true);
		//! Destroys material
		void Destroy();
		//! Gets index of the technique within material or -1 if not found
		int GetTechniqueIndex(const std::string& name);
		//! Sets current technique by index
		void SetTechnique(int index);
		//! Sets current technique by name
		void SetTechnique(const std::string& name);
		//! Gets material parameter index or -1 if not found
		int GetParameterIndex(const std::string& name);
		//! Sets integer parameter by index
		void SetIntParameter(int index, const int* value, int count = 1);
		//! Sets integer parameter by name
		void SetIntParameter(const std::string& name, const int* value, int count = 1);
		//! Sets float parameter by index
		void SetFloatParameter(int index, const float* value, int count = 1);
		//! Sets float parameter by name
		void SetFloatParameter(const std::string& name, const float* value, int count = 1);
		//! Sets single float parameter by index
		void SetFloatParameter(int index, float value);
		//! Sets single float parameter by value
		void SetFloatParameter(const std::string& name, float value);
		//! Sets texture parameter by index
		void SetTextureParameter(int index, Texture& value, const Sampler& sampler = Sampler::Default);
		//! Sets texture parameter by name
		void SetTextureParameter(const std::string& name, Texture& value, const Sampler& sampler = Sampler::Default);
		//! Draws shape described using given parameters using current technique with currently set parameters
		void Draw(const Shape::DrawParams* params);
		//! Draws fullscreen quad using current technique with currently set parameters
		void DrawFullscreenQuad();
	private:
		MaterialObj* obj;
	};

	//! Animated sprite instance
	class Sprite
	{
	public:
		//! Sprite event callback
		typedef void (*EventCallback)(const std::string& eventName, const std::string& eventValue, void* userData);

		//! Sprite animation mode
		enum AnimationMode
		{
			AnimationMode_Loop = 0,		//!< Infinite looping
			AnimationMode_Once,			//!< Played once; then transitions back to default animation
			AnimationMode_OnceAndFreeze,//!< Play once and then freeze (no animation)
			AnimationMode_LoopWhenDone,	//!< Start infinite looping after current animation is done
			AnimationMode_OnceWhenDone,	//!< Play once after current animation is done

			AnimationMode_COUNT
		};

		//! Sprite draw parameters
		struct DrawParams
		{
			Color color;		//!< Color; defaults to Color::White

			Vec2 position;		//!< Left top coordinate; defaults to (10.0f, 10.0f)

			const Rect* rect;			//!< Optional coordinates specified via rectangle; defaults to NULL; if set 'position' is not used
			const Rect* texCoordRect;	//!< Optional texture coordinates to use; defaults to NULL

			float scale;		//!< Scale; defaults to 1.0f
			float rotation;		//!< Rotation; defaults to 0.0f
			bool flipX;			//!< Flip in X?; defaults to false
			bool flipY;			//!< Flip in Y?; defaults to false

			//! Constructs default draw parameters
			DrawParams();
		};

		//! Constructs an empty sprite
		Sprite();
		//! Copy constructor; internally creates new sprite instance object (with its own state) and increases reference count of the sprite resource
		Sprite(const Sprite& other);
		//! Destructs sprite
		~Sprite();
		//! Gets resource state
		ResourceState GetState() const;
		//! Copy operator; internally creates new sprite instance object (with its own state) and increases reference count of the sprite resource
		void operator = (const Sprite& other);
		//! Creates sprite from a file; name can either be the name of the .sprite.xml file or full path to the image file (including image file extension); 'immediate' set to true indicates to load it synchronously
		bool Create(const std::string& name, bool immediate = true);
		//! Destroys the sprite
		void Destroy();
		//! Sets event callback (events are defined in .sprite.xml file)
		void SetEventCallback(EventCallback callback, void* userData);
		//! Updates sprite state by given delta time
		void Update(float deltaTime);
		//! Plays an animation of given name
		void PlayAnimation(const std::string& name = std::string() /* Default animation */, AnimationMode mode = AnimationMode_Loop, float transitionTime = 0.0f);
		//! Draws the sprite
		void Draw(const DrawParams* params);
		//! Draws the sprite
		void Draw(const Vec2& position = Vec2(0.0f, 0.0f), float rotation = 0);
		//! Draws the sprite centered at given coordinates
		void DrawCentered(const Vec2& center = Vec2(0.0f, 0.0f), float rotation = 0);
		//! Gets sprite width
		int GetWidth();
		//! Gets sprite height
		int GetHeight();
	private:
		SpriteObj* obj;
	};

	//! Text rendering related functionality
	class Text
	{
	public:
		//! Horizontal alignment
		enum HorizontalAlignment
		{
			HorizontalAlignment_Left = 0,	//!< Align text to the left
			HorizontalAlignment_Center,		//!< Center text horizontally (left-right)
			HorizontalAlignment_Right,		//!< Align text to the right

			HorizontalAlignment_COUNT
		};

		//! Vertical alignment
		enum VerticalAlignment
		{
			VerticalAlignment_Top = 0,		//!< Align text to the top
			VerticalAlignment_Center,		//!< Center text vertically (top-bottom)
			VerticalAlignment_Bottom,		//!< Align text to the bottom

			VerticalAlignment_COUNT
		};

		//! Text drawing parameters
		struct DrawParams
		{
			std::string text;						//!< Text
			Color color;							//!< Text color; defaults to Color::White
			Vec2 position;							//!< Left-top text coordinate; defaults to (0,0)
			float width;							//!< Width of the text rectangle (only used when horizontal alignment is non-left); defaults to 0.0f
			float height;							//!< Height of the text rectangle (only used when vertical alignment is non-top); defaults to 0.0f
			float rotation;							//!< Rotation of the text; defaults to 0.0f
			float scale;							//!< Scale of the text; defaults to 1.0f
			HorizontalAlignment horizontalAlignment;//!< Horizontal alignment; defaults to HorizontalAlignment_Left
			VerticalAlignment verticalAlignment;	//!< Vertical alignment; defaults to VerticalAlignment_Top
			bool drawShadow;						//!< Draw the text shadow?; defaults to false
			Color shadowColor;						//!< Shadow color; defaults to Color::Black
			Vec2 shadowOffset;						//!< Shadow offset; defaults to (2,3)

			//! Constructs empty text draw parameters
			DrawParams();
		};
	};

	//! Font resource handle
	class Font
	{
	public:
		//! Font flags
		enum Flags
		{
			Flags_Bold			= 1 << 0,	//!< Bold font
			Flags_Italic		= 1 << 1,	//!< Italic font
			Flags_Underlined	= 1 << 2,	//!< Font with underline
			Flags_StrikeThrough	= 1 << 3	//!< Font with strike-through effect
		};

		//! Creates an empty font
		Font();
		//! Copy constructor; internally only increases reference count of the font resource
		Font(const Font& other);
		//! Destroys the font
		~Font();
		//! Gets resource state
		ResourceState GetState() const;
		//! Copy operator; internally only increases reference count of the font resource
		void operator = (const Font& other);
		//! Creates font from TTF file; 'size' indicates font size in pixels @see enum Flags; 'immediate' set to true indicates to load it synchronously
		bool Create(const std::string& path, int size, unsigned int flags = 0, bool immediate = true);
		//! Destroys the font
		void Destroy();
		//! Caches glyphs from given text, so that consecutive calls to Draw() are faster
		void CacheGlyphs(const std::string& text);
		//! Caches glyphs from given text file, so that consecutive calls to Draw() are faster
		void CacheGlyphsFromFile(const std::string& path);
		//! Draws text
		void Draw(const Text::DrawParams* params);
		//! Draws text
		void Draw(const char* text, const Vec2& position, const Color& color = Color::White);
		//! Calculates text size
		void CalculateSize(const Text::DrawParams* params, float& width, float& height);
	private:
		FontObj* obj;
	};

	//! Sound instance
	class Sound
	{
	public:
		// Creates an empty sound
		Sound();
		//! Copy constructor; internally creates new sound instance object (with its own state) and increases reference count of the sound resource
		Sound(const Sound& other);
		//! Destroys the sound
		~Sound();
		//! Gets resource state
		ResourceState GetState() const;
		//! Copy operator; internally creates new sound instance object (with its own state) and increases reference count of the sound resource
		void operator = (const Sound& other);
		//! Creates sound from a file; isMusic indicates whether this sound shall be played on the special music channel; 'immediate' set to true indicates to load it synchronously
		bool Create(const std::string& path, bool isMusic = false, bool immediate = true);
		//! Destroys the sound (with optional sound fade out)
		void Destroy(float fadeOutTime = 0.0f);
		//! Sets the volume of the sound; expects value within 0..1 range
		void SetVolume(float volume);
		//! Starts playing the sound
		void Play(bool loop = false, float fadeInTime = 0.2f);
		//! Stops playing the sound
		void Stop();
		//! Checks whether sound is playing now
		bool IsPlaying();
	private:
		SoundObj* obj;
	};

	//! Particle effect instance
	class Effect
	{
	public:
		//! Constructs an empty effect
		Effect();
		//! Copy constructor; internally creates new effect instance object (with its own state) and increases reference count of the effect resource
		Effect(const Effect& other);
		//! Destroys an effect immediately
		~Effect();
		//! Gets resource state
		ResourceState GetState() const;
		//! Copy operator; internally creates new effect instance object (with its own state) and increases reference count of the effect resource
		void operator = (const Effect& other);
		//! Create an effect at given position and with given rotation; 'immediate' set to true indicates to load it synchronously
		bool Create(const std::string& path, const Vec2& pos = Vec2(0.0f, 0.0f), float rotation = 0.0f, bool immediate = true);
		//! Starts destruction of an effect; the effect will live until the last particle of it is alive; if an effect is endless it will be destroyed immediately
		void Destroy();
		//! Updates effect by given delta time
		void Update(float deltaTime);
		//! Draws an effect
		void Draw();
		//! Sets effect position
		void SetPosition(const Vec2& pos);
		//! Sets effect rotation
		void SetRotation(float rotation);
		//! Sets effect scale
		void SetScale(float scale);
		//! Sets effect spawn count multiplier (defaults to 1.0f); this is getting applied to all of its emitters
		void SetSpawnCountMultiplier(float multiplier);
	private:
		EffectObj* obj;
	};

	//! Application/base functionality
	class App
	{
	public:
		//! Application display settings
		struct DisplaySettings
		{
			std::string textureVersion;		//!< Optional texture version name suffix; defaults to empty string
			float textureVersionSizeMultiplier; //!< Optional texture version size multiplier; defaults to 2.0f; only used when 'textureVersion' is set
			int width;						//!< Desired display width; 0 indicates to use default/native width; defaults to 0
			int height;						//!< Desired display height; 0 indicates to use default/native height; defaults to 0
			int virtualWidth;				//!< Virtual display width; 0 indicates to make it equal to display width; defaults to 0
			int virtualHeight;				//!< Virtual display height; 0 indicates to make it equal to display height; defaults to 0
			bool fullscreen;				//!< Enable fullscreen mode? Only used on desktop platforms; defaults to true in release and false in debug

			//! Sets up default display settings
			DisplaySettings();

			//! Sets up iPhone non-retina display resolution (480x320)
			void SetiPhoneResolution();
			//! Sets up iPhone retina resolution (960x640)
			void SetiPhoneRetinaResolution();
			//! Sets up iPad non-retina display resolution (1024x768)
			void SetiPadResolution();
			//! Sets up iPad retina display resolution (2048x1536)
			void SetiPadRetinaResolution();
		};

		//! Applicationn startup parameters
		struct StartupParams : DisplaySettings
		{
			std::string name;				//!< Application name
			std::vector<std::string> rootDataDirs;		//!< Root data directories listed in order from the highest to lowest priority
			std::string languageSymbol;		//!< Language symbol (used for text localization); defaults to "EN"
			std::string defaultMaterialName;//!< Name of the default material to be loaded at app startup; defaults to "common/default"
			std::string defaultFontName;	//!< Name of the default font to be loaded at app startup; defaults to "common/courbd.ttf"
			int defaultFontSize;			//!< Default font size; defaults to 16
			std::string defaultCursorName;	//!< Default cursor texture (or sprite) name; defaults to "common/cursor.png"
			bool showMessageBoxOnWarning;	//!< Show message box on every warning?; defaults to false
			bool showMessageBoxOnError;		//!< Show message box on every error?; defaults to false in release and true in debug
			bool exitOnError;				//!< Exit app on error?; defaults to false in release and true in debug
			bool emulateTouchpadWithMouse;	//!< Emulate touchpad with mouse? Only used on desktop platforms; defaults to true on desktop platforms
			bool supportAsynchronousResourceLoading; //!< Support asynchronous resource loading?; defaults to true

			//! Constructs default startup parameters
			StartupParams();
		};

		//! Display mode
		struct DisplayMode
		{
			int width;	//!< Width in pixels
			int height;	//!< Height in pixels
		};

		//! System information
		struct SystemInfo
		{
			int nativeWidth;			//!< Native device display width
			int nativeHeight;			//!< Native device display height
			std::vector<DisplayMode> displayModes;	//!< Available fullscreen display modes
			std::string platformName;	//!< Platform name
		};

		//! Native message box types
		enum MessageBoxType
		{
			MessageBoxType_Info = 0,	//!< Information
			MessageBoxType_Warning,		//!< Warning
			MessageBoxType_Error,		//!< Error

			MessageBoxType_COUNT
		};

		//! App callbacks to be implemented by the user
		class Callbacks
		{
		public:
			virtual ~Callbacks() {}
			//! Invoked before app startup to allow user set up application startup parameters; expects true returned on success; on false application is stopped
			virtual bool OnStartup(int numArguments, const char** arguments, const SystemInfo& systemInfo, StartupParams& outParams) { return true; }
			//! Invoked after successful Tiny2D system startup to allow an app to initialize itself; expects true returned on success; on false application is stopped
			virtual bool OnInit() { return true; }
			//! Invoked just before application shutdown; note: it's totally okay too to destroy all game resources in a destructor
			virtual void OnDeinit() {}
			//! Invoked when app gets activated/restored or deactivated
			virtual void OnActivated(bool activated) {}
			//! Invoked each frame to update an application by 'deltaTime' seconds
			virtual void OnUpdate(float deltaTime) {}
			//! Invoked each frame to draw application; given render target is where the rendering shall be done
			virtual void OnDraw(Texture& renderTarget) {}
			//! Invoked each frame after built-in post-processing rendering is done; this is to allow for rendering some parts of the game (e.g. UI) without any post-processing applied to it
			virtual void OnDrawAfterPostprocessing(Texture& renderTarget) {}
		};

		//! Changes application display settings (resolution, fullscreen mode etc.)
		static bool					ModifyDisplaySettings(const DisplaySettings& settings);
		//! Quits an app
		static void					Quit();
		//! Gets root data directory
		static const std::vector<std::string>&	GetRootDataDirs();
		//! Gets rendering width (possibly virtual resolution width)
		static int					GetWidth();
		//! Gets rendering height (possibly virtual resolution height)
		static int					GetHeight();
		//! Checks if the app is in fullscreen mode
		static bool					IsFullscreen();
		//! Gets main/default render target
		static Texture&				GetMainRenderTarget();
		//! Gets current render target
		static Texture&				GetCurrentRenderTarget();
		//! Gets default material (used to draw textures and sprites)
		static Material&			GetDefaultMaterial();
		//! Gets default font
		static Font&				GetDefaultFont();
		//! Gets app language symbol
		static const std::string&	GetLanguageSymbol();
		//! Shows message box
		static void					ShowMessageBox(const std::string& message, MessageBoxType type = MessageBoxType_Info);
		//! Enables basic visual app stats including FPS
		static void					EnableOnScreenDebugInfo(bool enable);
		//! Puts current thread to sleep
		static void					Sleep(float seconds);
		//! Runs command; for example setting command to "http://www.pixelelephant.com" will open webpage in a browser; note: currently only works on Windows
		static void					RunCommand(const std::string& command);
	};

	//! Text localization functionality
	class Localization
	{
	public:
		//! Parameter value description within localized text
		struct Param
		{
			std::string name;	//!< Parameter name
			//! Localization parameter type
			enum Type
			{
				Type_Int = 0,			//!< Integer type of the localization parameter
				Type_String,			//!< String type of the localization parameter
				Type_Uninitialized,		//!< Uninitialized type of the localization parameter

				Type_COUNT
			};
			Type type;					//!< Parameter type
			int intValue;				//!< Integer value
			std::string stringValue;	//!< String value	

			//! Constructs uninitialized localization parameter
			Param();
			//! Constructs int localization paramter
			Param(const std::string& name, int value);
			//! Constructs string localization paramter
			Param(const std::string& name, const char* value);
			//! Constructs string localization paramter
			Param(const std::string& name, const std::string& value);
		};

		//! Loads text localization set (loads file named 'name'.translations.xml)
		static bool					LoadSet(const std::string& name);
		//! Unloads specific text localization set
		static void					UnloadSet(const std::string& name);
		//! Unloads all text localization sets
		static void					UnloadAllSets();
		//! Localizes given string (with optional parameters)
		static const std::string	Get(const char* stringName, const Param* params = NULL, int numParams = 0);
	};

	//! Defines application callback class
	#define DEFINE_APP(className) \
		class Tiny2DInitializer \
		{ \
		public: \
			Tiny2DInitializer() \
			{ \
				extern void SetCreateTiny2DAppCallbacks(Tiny2D::App::Callbacks* (*func)()); \
				SetCreateTiny2DAppCallbacks(CreateAppCallbacks); \
			} \
			static Tiny2D::App::Callbacks* CreateAppCallbacks() { return new className(); } \
		}; \
		const Tiny2DInitializer g_initializer;

	//! Asynchronous job system
	class Jobs
	{
	public:
		//! Unique job identifier
		typedef int JobID;

		//! Job function (performed on non-main thread)
		typedef void (*JobFunc)(void* userData);
		//! Job done function (performed on main thread); canceled set to true indicates that job was canceled and its job function was not performed
		typedef void (*DoneFunc)(bool canceled, void* userData);

		//! Runs asynchronous job function; when done invokes supplied (optional) done function on main thread; returns unique job id
		static JobID	RunJob(JobFunc jobFunc, DoneFunc doneFunc, void* userData);
		//! Gets number of jobs still in progress
		static int		GetNumJobsInProgress();
		//! Updates jobs that require main thread update
		static void		UpdateDoneJobs(float maxUpdateTime = 1.0f / 120.0f);
		//! Waits for a particular job to complete
		static void		WaitForJob(JobID id);
		//! Cancels particular job
		static void		CancelJob(JobID id);
		//! Waits for all jobs to complete
		static void		WaitForAllJobs();
	};

	//! Time related functionality
	class Time
	{
	public:
		//! Time represented in device/system specific 'ticks'; ticks can be subtracted or added together
		typedef unsigned long long int Ticks;

		//! Gets current time in ticks
		static Ticks	GetTicks();
		//! Converts ticks to seconds
		static float	TicksToSeconds(Ticks ticks);
		//! Converts seconds to ticks
		static Ticks	SecondsToTicks(float seconds);
		//! Gets seconds since given ticks
		static float	SecondsSince(Ticks ticks);
	};

	//! Random numbers
	class Random
	{
	public:
		//! Seeds/resets random number generator with given seed value; seeding with same value will result in same values generated by GetInt() and GetFloat()
		static void		Seed(int seed);
		//! Gets random integer value in range; 'maxValue' must not exceed 32767
		static int		GetInt(int minValue = 0, int maxValue = 32767);
		//! Gets random float value in range
		static float	GetFloat(float minValue = 0.0f, float maxValue = 1.0f);
	};

	//! Logging functionality
	class Log
	{
	public:
	#ifdef ENABLE_LOGGING

		//! Logs debug message
		static void		Debug(const std::string& message);
		//! Logs information message
		static void		Info(const std::string& message);
		//! Logs warning message
		static void		Warn(const std::string& message);
		//! Logs error message
		static void		Error(const std::string& message);

	#else

		//! Logs debug message
		static inline void Debug(const std::string&) {}
		//! Logs information message
		static inline void Info(const std::string&) {}
		//! Logs warning message
		static inline void Warn(const std::string&) {}
		//! Logs error message
		static inline void Error(const std::string&) {}

	#endif

	};

	//! Input devices
	class Input
	{
	public:
		//! Keyboard key identifiers
		enum Key
		{
			Key_MouseLeft,		//!< Left mouse button
			Key_MouseMiddle,	//!< Middle mouse button
			Key_MouseRight,		//!< Right mouse button

			Key_Left,
			Key_Right,
			Key_Up,
			Key_Down,

			Key_Escape,
			Key_Space,
			Key_Enter,

			Key_LeftAlt,
			Key_RightAlt,
			Key_Alt,			//!< Left or right alt
			Key_LeftControl,
			Key_RightControl,
			Key_Control,		//!< Left or right control
			Key_LeftShift,
			Key_RightShift,
			Key_Shift,			//!< Left or right shift

			Key_Android_Back,	//!< Android specific back key
			Key_Android_Home,	//!< Android specific home key
			Key_Menu,
			
			Key_0,
			Key_1,
			Key_2,
			Key_3,
			Key_4,
			Key_5,
			Key_6,
			Key_7,
			Key_8,
			Key_9,

			Key_A,
			Key_B,
			Key_C,
			Key_D,
			Key_E,
			Key_F,
			Key_G,
			Key_H,
			Key_I,
			Key_J,
			Key_K,
			Key_L,
			Key_M,
			Key_N,
			Key_O,
			Key_P,
			Key_Q,
			Key_R,
			Key_S,
			Key_T,
			Key_U,
			Key_V,
			Key_W,
			Key_X,
			Key_Y,
			Key_Z,

			Key_COUNT
		};

				enum ControllerButton
		{
			ControllerButton_A,
			ControllerButton_B,
			ControllerButton_X,
			ControllerButton_Y,
			ControllerButton_BACK,
			ControllerButton_GUIDE,
			ControllerButton_START,
			ControllerButton_LEFTSTICK,
			ControllerButton_RIGHTSTICK,
			ControllerButton_LEFTSHOULDER,
			ControllerButton_RIGHTSHOULDER,
			ControllerButton_DPAD_UP,
			ControllerButton_DPAD_DOWN,
			ControllerButton_DPAD_LEFT,
			ControllerButton_DPAD_RIGHT,

			ControllerButton_LEFTSTICK_Left,
			ControllerButton_LEFTSTICK_Right,
			ControllerButton_LEFTSTICK_Up,
			ControllerButton_LEFTSTICK_Down,
			ControllerButton_RIGHTSTICK_Left,
			ControllerButton_RIGHTSTICK_Right,
			ControllerButton_RIGHTSTICK_Up,
			ControllerButton_RIGHTSTICK_Down,

			ControllerButton_COUNT
		};

		enum ControllerAxis
		{
			ControllerAxis_LEFTX,
			ControllerAxis_LEFTY,
			ControllerAxis_RIGHTX,
			ControllerAxis_RIGHTY,
			ControllerAxis_TRIGGERLEFT,
			ControllerAxis_TRIGGERRIGHT,

			ControllerAxis_COUNT
		};

		//! Touch (on touchpad) description; touch is alive (and has persistent id) from the time of touch until it gets released
		struct Touch
		{
			Vec2 position;		//!< Touch coordinate (with x ranging from 0 to App::GetWidth() and y randing from 0 to App::GetHeight())
			long long int id;	//!< Touch id
		};

		//! Key state (used with keyboard, mouse and other keys)
		enum KeyState
		{
			KeyState_IsDown		= 1 << 0,	//!< Is down/pressed this frame
			KeyState_WasDown	= 1 << 1	//!< Was down/pressed previous frame
		};

		//! Mouse state
		struct MouseState
		{
			Vec2 position;						//!< Mouse coordinates (with x ranging from 0 to App::GetWidth() and y randing from 0 to App::GetHeight())
			Vec2 movement;						//!< Mouse offset
		};

		//! Game controller state
		struct ControllerState
		{
			int deviceId;							//!< Device id
			float axis[ ControllerAxis_COUNT ];		//!< Analog axis values within -1..1 range
			unsigned int buttonState[ ControllerButton_COUNT ]; //!< Button states; prefer using Is/Was-Button-Down/Pressed/Released() functions instead of accessing this array directly

			//! Gets whether given button is currently being pressed
			inline bool			IsButtonDown(ControllerButton button) const { return (buttonState[button] & KeyState_IsDown) != 0; }
			//! Gets whether given button was pressed last frame
			inline bool			WasButtonDown(ControllerButton button) const { return (buttonState[button] & KeyState_WasDown) != 0; }
			//! Gets whether given button has just been pressed
			inline bool			WasButtonPressed(ControllerButton button) const { return buttonState[button] == KeyState_IsDown; }
			//! Gets whether given button has just been released
			inline bool			WasButtonReleased(ControllerButton button) const { return buttonState[button] == KeyState_WasDown; }
		};

		//! Gets key state; return combination of KeyState flags
		static unsigned int			GetKeyState(Key key);
		//! Gets whether given key is currently being pressed
		static inline bool			IsKeyDown(Key key) { return (GetKeyState(key) & KeyState_IsDown) != 0; }
		//! Gets whether given key was pressed last frame
		static inline bool			WasKeyDown(Key key) { return (GetKeyState(key) & KeyState_WasDown) != 0; }
		//! Gets whether given key has just been pressed
		static inline bool			WasKeyPressed(Key key) { return GetKeyState(key) == KeyState_IsDown; }
		//! Gets whether given key has just been released
		static inline bool			WasKeyReleased(Key key) { return GetKeyState(key) == KeyState_WasDown; }
		//! Gets whether mouse is supported
		static bool					HasMouse();
		//! Gets mouse state
		static const MouseState&	GetMouseState();
		//! Gets whether touchpad is supported
		static bool					HasTouchpad();
		//! Gets number of touches currently active
		static int					GetNumTouches();
		//! Gets active touch at given index
		static const Touch&			GetTouch(int index);
		//! Gets whether any touch is active
		static bool					IsTouchpadDown();
		//! Gets whether touchpad was touched previous frame
		static bool					WasTouchpadDown();
		//! Gets whether touchpad has just been touched
		static bool					WasTouchpadPressed();
		//! Gets number of controller devices
		static int					GetNumControllers();
		//! Gets controller device state
		static const ControllerState& GetControllerState(int index);
		static const ControllerState* FindControllerState(int deviceId);
	};

	//! Pool of render textures for convenient reuse between subsystems
	class RenderTexturePool
	{
	public:
		//! Gets (creates if needed) RGBA render texture from the pool
		static Texture	Get(int width, int height);
		//! Releases render texture back to pool; after this operation texture parameter is invalidated
		static void		Release(Texture& texture);
		//! Destroys all render textures in a pool; called automatically when called App::ModifyDisplaySettings()
		static void		DestroyAll();
	};

	//! Built-in postprocesses functionality
	class Postprocessing
	{
	public:
		//! Toggles bloom postprocess
		static void		EnableBloom(bool enable = true, float blendFactor = 0.3f, float blurKernel = 1.0f, int numBlurSteps = 1);
		//! Checks if bloom postprocess is currently enabled
		static bool		IsBloomEnabled();
		//! Toggles "Old TV" postprocess
		static void		EnableOldTV(bool enable = true);
		//! Checks if "Old TV" postprocess is currently enabled
		static bool		IsOldTVEnabled();
		//! Starts "Screen Quake" implemented as a postprocess; it will get disabled by itself after 'length' seconds
		static void		StartQuake(bool start = true, float strength = 1.0f, float length = 1.0f);
		//! Checks if "Screen Quake" postprocess is currently enabled
		static bool		IsQuakeEnabled();
		//! Toggles postprocess of "rainy glass" (rain drops falling down the glass window)
		static void		EnableRainyGlass(bool enable = true, float spawnFrequency = 1.0f, float dropletSpeed = 0.5f, float dropletMinSize = 0.02f, float dropletMaxSize = 0.04f);
		//! Checks if rainy glass postprocess is currently enabled
		static bool		IsRainyGlassEnabled();
	};
};

#endif // TINY_2D