#ifndef TINY_2D_TYPES
#define TINY_2D_TYPES

#include <math.h>
#include <string>
#include <vector>

namespace Tiny2D
{
	#ifndef DEBUG
		#ifdef _DEBUG
			#define DEBUG
		#endif
	#endif

	#ifdef DEBUG
		//! Assertion macro
		#define Assert(condition) \
			do \
			{ \
				if (!(condition)) \
				{ \
					HandleAssertion(#condition, __FILE__, __LINE__); \
				} \
			} while (0)
		void HandleAssertion(const char* what, const char* fileName, int fileLine);
	#else
		//! Assertion macro
		#define Assert(condition)
	#endif

	#if defined(WIN32) || defined(WIN64) || defined(LINUX) || defined(MACOS)
		//! Indicates if we're on desktop platform (or otherwise on mobile)
		#define DESKTOP
	#endif

	#ifdef DESKTOP
		#ifdef DEBUG
			#define ENABLE_LOGGING
		#endif
	#else
		#define ENABLE_LOGGING
	#endif

	//! String formatting helper function
	std::string string_format(const char* format, ...);

	// Forward declarations of internal object types
	struct TextureObj;
	struct EffectObj;
	struct FontObj;
	struct SoundObj;
	struct MaterialObj;
	struct SpriteObj;
	struct FileObj;
	struct XMLDocObj;

	//! Color with RGBA components
	struct Color
	{
		static Color White;		//!< White color i.e. (1,1,1,1)
		static Color Black;		//!< Black color i.e. (0,0,0,1)
		static Color Red;		//!< Red color i.e. (1,0,0,1)
		static Color Green;		//!< Green color i.e. (0,1,0,1)
		static Color Blue;		//!< Blue color i.e. (0,0,1,1)
		static Color Yellow;	//!< Yellow color i.e. (1,1,0,1)

		float r;	//!< Red color component
		float g;	//!< Green color component
		float b;	//!< Blue color component
		float a;	//!< Alpha/transparency color component

		//! Constructs color
		inline Color(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f)
		{
			this->r = r;
			this->g = g;
			this->b = b;
			this->a = a;
		}

		//! Copy constructor
		inline Color(const Color& other) :
			r(other.r),
			g(other.g),
			b(other.b),
			a(other.a)
		{}

		//! Multiplies color by float factor
		inline Color operator * (float multiplier) const
		{
			Color color;
			color.r = r * multiplier;
			color.g = g * multiplier;
			color.b = b * multiplier;
			color.a = a * multiplier;
			return color;
		}

		//! Adds colors
		inline Color operator + (const Color& other) const
		{
			return Color(r + other.r, g + other.g, b + other.b, a + other.a);
		}

		//! Subtracts colors
		inline Color operator - (const Color& other) const
		{
			return Color(r - other.r, g - other.g, b - other.b, a - other.a);
		}

		//! Multiplies two colors together
		inline Color operator * (const Color& other) const
		{
			return Color(r * other.r, g * other.g, b * other.b, a * other.a);
		}
	};

	//! Rectangle representation
	struct Rect
	{
		float left;		//!< Left coordinate
		float top;		//!< Top coordinate
		float width;	//!< Width
		float height;	//!< Height

		//! Constructs rectangle
		inline Rect(float left = 0.0f, float top = 0.0f, float width = 100.0f, float height = 100.0f)
		{
			this->left = left;
			this->top = top;
			this->width = width;
			this->height = height;
		}

		//! Sets up rectangle
		inline void Set(float left, float top, float width, float height)
		{
			this->left = left;
			this->top = top;
			this->width = width;
			this->height = height;
		}

		//! Gets right rectangle coordinate
		inline float Right() const { return left + width; }
		//! Gets bottom rectangle coordinate
		inline float Bottom() const { return top + height; }

		//! Tests whether rectangle intersects with point at given coordinates
		inline bool Intersect(float x, float y) const
		{
			return
				left <= x && x <= left + width &&
				top <= y && y <= top + height;
		}

		//! Moves rectangle by given x,y amount
		inline void Translate(float x, float y)
		{
			left += x;
			top += y;
		}

		//! Scales rectangle while preserving its center coordinates
		inline void ScaleCentered(float scale)
		{
			const float newWidth = width * scale;
			const float newHeight = height * scale;

			left -= (newWidth - width) * 0.5f;
			top -= (newHeight - height) * 0.5f;
			width = newWidth;
			height = newHeight;
		}

		//! Intersects two rectangles
		inline bool Intersect( const Rect& other ) const
		{
			return !(
				left > other.Right() ||
				Right() < other.left ||
				top > other.Bottom() ||
				Bottom() < other.top );
		}
	};

	//! 2 component vector of floats
	struct Vec2
	{
		float x;	//!< X coordinate
		float y;	//!< Y coordinate

		//! Constructs vector
		inline Vec2(float x = 0.0f, float y = 0.0f)
		{
			this->x = x;
			this->y = y;
		}

		//! Copy consructor
		inline Vec2(const Vec2& other) :
			x(other.x),
			y(other.y)
		{}

		//! Copy operator
		inline void operator = (const Vec2& other)
		{
			x = other.x;
			y = other.y;
		}

		//! Sets x and y components of the vector
		inline void Set(float x, float y)
		{
			this->x = x;
			this->y = y;
		}

		//! Assigns single value to both x and y components
		inline void operator = (float value)
		{
			x = y = value;
		}

		//! Gets component at given index
		inline const float operator [] (int index) const
		{
			Assert(0 <= index && index <= 1);
			return ((float*) &x)[index];
		}

		//! Gets reference to component at given index
		inline float& operator [] (int index)
		{
			Assert(0 <= index && index <= 1);
			return ((float*) &x)[index];
		}

		//! Calculates length
		inline float Length() const
		{
			return sqrtf(x * x + y * y);
		}

		//! Calculates squared length
		inline float LengthSqr() const
		{
			return x * x + y * y;
		}

		//! Adds 2 vectors
		inline Vec2 operator + (const Vec2& other) const
		{
			return Vec2(x + other.x, y + other.y);
		}

		//! Adds another vector to itself
		inline void operator += (const Vec2& other)
		{
			x += other.x;
			y += other.y;
		}

		//! Negates vector
		inline Vec2 operator - () const
		{
			return Vec2(-x, -y);
		}

		//! Subtracts one vector from another
		inline Vec2 operator - (const Vec2& other) const
		{
			return Vec2(x - other.x, y - other.y);
		}

		//! Subtracts vector from itself
		inline void operator -= (const Vec2& other)
		{
			x -= other.x;
			y -= other.y;
		}

		//! Returns vector whole components are scaled by given scalar
		inline Vec2 operator * (float value) const
		{
			return Vec2(x * value, y * value);
		}

		//! Multiplies all vector components by scalar
		inline void operator *= (float value)
		{
			x *= value;
			y *= value;
		}

		//! Returns result of multiplication of this vector by other vector component-wise
		inline Vec2 operator * (const Vec2& other) const
		{
			return Vec2(x * other.x, y * other.y);
		}

		//! Multiplies this vector by other vector component-wise
		inline void operator *= (const Vec2& other)
		{
			x *= other.x;
			y *= other.y;
		}

		//! Returns this vector divided component-wise by given scalar value
		inline Vec2 operator / (float value) const
		{
			return Vec2(x / value, y / value);
		}

		//! Divides all components of this vector by given scalar value
		inline void operator /= (float value)
		{
			x /= value;
			y /= value;
		}

		//! Returns this vector divided component-wise by other vector
		inline Vec2 operator / (const Vec2& other) const
		{
			return Vec2(x / other.x, y / other.y);
		}

		//! Divides all components of this vector by other vector
		inline void operator /= (const Vec2& other)
		{
			x /= other.x;
			y /= other.y;
		}

		//! Calculates dot product of 2 vectors
		inline static float Dot(const Vec2& a, const Vec2& b)
		{
			return a.x * b.x + a.y * b.y;
		}

		//! Lerps between 2 vectors
		inline static Vec2 Lerp(const Vec2& a, const Vec2& b, float scale)
		{
			return Vec2(a.x + (b.x - a.x) * scale, a.y + (b.y - a.y) * scale);
		}

		//! Normalizes vector to given length (defaults to 1); returns previous length
		float Normalize(float length = 1.0f)
		{
			if (x == 0.0f && y == 0.0f)
			{
				x = 0.0f;
				y = length;
				return 0.0f;
			}
			else
			{
				const float oldLength = Length();
				const float oldLengthInv = 1.0f / oldLength;
				const float lengthScale = oldLengthInv * length;
				x *= lengthScale;
				y *= lengthScale;
				return oldLength;
			}
		}

		//! Rotates vector by given angle around given origin
		void Rotate(float angle, const Vec2& origin = Vec2(0, 0))
		{
			const float rotationCos = cosf(angle);
			const float rotationSin = sinf(angle);

			*this -= origin;
			Set(rotationCos * x + rotationSin * y, rotationCos * y - rotationSin * x);
			*this += origin;
		}
	};
};

#endif // TINY_2D_TYPES