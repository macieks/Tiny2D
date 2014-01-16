#include "Tiny2D.h"
#include "Tiny2D_Common.h"

struct SpawnArea
{
	enum Type
	{
		Type_Point = 0,
		Type_Rectangle,
		Type_Circle,

		Type_COUNT
	};

	Type type;
	union
	{
		struct
		{
			float width;
			float height;
		} rectangle;
		struct
		{
			float radius;
		} circle;
	};
};

struct BaseRange
{
	bool isLoaded;
	BaseRange() : isLoaded(false) {}
};

template <typename TYPE> struct Range {};

template <> struct Range<float> : BaseRange
{
	float minValue;
	float maxValue;

	bool Load(XMLNode* node)
	{
		if (!node)
			return false;

		float _minValue, _maxValue;
		if (node->GetAttributeValueFloat("min", _minValue) && node->GetAttributeValueFloat("max", _maxValue))
		{
			minValue = _minValue;
			maxValue = _maxValue;
			isLoaded = true;
			return true;
		}

		if (node->GetAttributeValueFloat("value", minValue))
		{
			maxValue = minValue;
			isLoaded = true;
			return true;
		}

		Log::Error("Missing 'min' and 'max' attributes");
		return false;
	}

	float Generate() const
	{
		return Random::GetFloat(minValue, maxValue);
	}

	void Set(float minValue, float maxValue)
	{
		this->minValue = minValue;
		this->maxValue = maxValue;
	}

	void operator = (float value)
	{
		minValue = maxValue = value;
	}

	inline bool operator == (float value) const
	{
		return minValue == maxValue && minValue == value;
	}
};

template <> struct Range<Vec2> : BaseRange
{
	Vec2 minValue;
	Vec2 maxValue;

	bool Load(XMLNode* node)
	{
		if (!node)
			return false;

		Vec2 _minValue, _maxValue;
		if (node->GetAttributeValueFloatArray("min", (float*) &_minValue, 2) && node->GetAttributeValueFloatArray("max", (float*) &_maxValue, 2))
		{
			minValue = _minValue;
			maxValue = _maxValue;
			isLoaded = true;
			return true;
		}

		if (node->GetAttributeValueFloatArray("value", (float*) &minValue, 2))
		{
			maxValue = minValue;
			isLoaded = true;
			return true;
		}

		Log::Error("Missing float2 'min' and 'max' attributes");
		return false;
	}

	Vec2 Generate() const
	{
		return Vec2(Random::GetFloat(minValue[0], maxValue[0]), Random::GetFloat(minValue[1], maxValue[1]));
	}

	void Set(const Vec2& minValue, const Vec2& maxValue)
	{
		this->minValue = minValue;
		this->maxValue = maxValue;
	}

	inline void operator = (const Vec2& value)
	{
		minValue = value;
		maxValue = value;
	}
};

inline float Lerp(float a, float b, float scale)
{
	return a + (b - a) * scale;
}

inline bool FromString(const char* s, float& result)
{
	return s && sscanf(s, "%f", &result) == 1;
}

inline Vec2 Lerp(const Vec2& a, const Vec2& b, const float scale)
{
	return a + (b - a) * scale;
}

inline Vec2 Lerp(const Vec2& a, const Vec2& b, const Vec2& scale)
{
	return a + (b - a) * scale;
}

inline bool FromString(const char* s, Vec2& result)
{
	if (!s)
		return false;
	const int numScanned = sscanf(s, "%f %f", &result.x, &result.y);
	switch (numScanned)
	{
		case 0: return false;
		case 1: result.y = result.x; break;
	}
	return true;
}

inline Color Lerp(const Color& a, const Color& b, const float scale)
{
	return a + (b - a) * scale;
}

inline bool FromString(const char* s, Color& result)
{
	if (!s)
		return false;
	const int numScanned = sscanf(s, "%f %f %f %f", &result.r, &result.g, &result.b, &result.a);
	switch (numScanned)
	{
		case 0: return false;
		case 1:
			result.g = result.b = result.r;
			result.a = 1.0f;
			break;
		case 2:
			result.b = 1.0f;
			result.a = 1.0f;
			break;
		case 3:
			result.a = 1.0f;
			break;
	}
	return true;
}

enum Seed
{
	Seed_Color = 0,
	Seed_Size,
	Seed_SizeScale,
	Seed_Velocity,
	Seed_SpawnCount,
	Seed_Acceleration,
	Seed_RotationScale,
	Seed_RotationSpeed,

	Seed_COUNT
};

const float g_seedValues[Seed_COUNT * 2] =
{
	113.538753097f,	892.53485734953f,
	845.4387530f,	520.12121244f,
	947.5437892f,	994.678134654f,
	212.4324354f,	354.37334893f,
	193.543875346f,	323.954534534f,
	295.87443543f,	402.435353453f,
	392.434323532f,	632.43223423432f,
	119.423354345f,	290.9353453454f,
};

inline float frac(float value)
{
	return value - floor(value);
}

inline float SeedToFloat(float srcSeed, Seed seedName)
{
	return frac(g_seedValues[seedName << 1] * srcSeed + g_seedValues[(seedName << 1) + 1]);
}

inline Vec2 SeedToVec2(float srcSeed, Seed seedName)
{
	return Vec2(SeedToFloat(srcSeed, seedName), SeedToFloat(srcSeed, (Seed) (seedName + 1)));
}

template <typename TYPE, typename SEED_TYPE = float> struct Track
{
	struct ControlPoint
	{
		float time;
		TYPE minValue;
		TYPE maxValue;

		inline TYPE GetValue(SEED_TYPE seed) const { return Lerp(minValue, maxValue, seed); }
	};

	bool isLoaded;
	std::vector<ControlPoint> points;

	Track() : isLoaded(false) {}

	TYPE SampleAt(float time, SEED_TYPE seed, bool loop) const
	{
		Assert(0.0f <= time && time <= 1.0f);

		const unsigned int numPoints = points.size();

		if (numPoints == 1)
			return points[0].GetValue(seed);

		unsigned int end = 0;
		while (end < numPoints && points[end].time < time)
			end++;

		unsigned int start;
		float startTime;
		if (!loop)
		{
			if (end == 0)
				return points[0].GetValue(seed);
			if (end == numPoints)
				return points.back().GetValue(seed);
			start = end - 1;
			startTime = points[start].time;
		}
		else
		{
			start = !end ? (numPoints - 1) : (end - 1);
			startTime = points[start].time + (!end ? 1.0f : 0.0f);
		}

		const float scale = (time - startTime) / (points[end].time - startTime);
		return Lerp(points[start].GetValue(seed), points[end].GetValue(seed), scale);
	}

	bool Load(XMLNode* node)
	{
		if (!node)
			return false;

		std::vector<ControlPoint> points;

		const int numControlPoints = node->CalcNumNodes("key");
		if (numControlPoints)
		{
			for (XMLNode* cpNode = node->GetFirstNode("key"); cpNode; cpNode = cpNode->GetNext("key"))
			{
				ControlPoint& cp = vector_add(points);
				if (!cpNode->GetAttributeValueFloat("time", cp.time))
					return false;
				if (FromString(cpNode->GetAttributeValue("value"), cp.minValue))
				{
					cp.maxValue = cp.minValue;
					continue;
				}
				if (FromString(cpNode->GetAttributeValue("min"), cp.minValue) && FromString(cpNode->GetAttributeValue("max"), cp.maxValue))
					continue;
				return false;
			}
		}
		else // Fall back to just scalar value
		{
			ControlPoint& cp = vector_add(points);
			cp.time = 0.0f;
			if (FromString(node->GetAttributeValue("value"), cp.minValue))
				cp.maxValue = cp.minValue;
			else if (!FromString(node->GetAttributeValue("min"), cp.minValue) || !FromString(node->GetAttributeValue("max"), cp.maxValue))
				return false;
		}
		this->points = points;
		isLoaded = true;
		return true;
	}

	void operator = (const TYPE& value)
	{
		points.resize(1);
		points[0].time = 0.0f;
		points[0].minValue = value;
		points[0].maxValue = value;
	}

	inline bool operator == (const TYPE& value) const
	{
		return points.size() == 1 && points[0].minValue == value && points[0].maxValue == value;
	}
};

struct EmitterResource
{
	int maxParticles;
	Shape::Blending blending;
	bool localSimulation;
	struct
	{
		SpawnArea spawnArea;
		Range<float> spawnCount;
		Range<Vec2> velocity;
		Range<float> rotation;
	} initial;
	struct
	{
		struct
		{
			Track<Vec2, Vec2> velocity; // Optional: either velocity or initial.velocity+acceleration is used
			Track<Vec2, Vec2> acceleration;
			Track<Color> color;
			Track<Vec2, Vec2> size;
			Track<float> rotationSpeed;
		} overParticleLife;
		struct
		{
			Track<float> spawnCount;
			Track<float> cyclesTotal;
			Track<float> lifeTotal;
			Track<Color> color;
			Track<float> sizeScale;
			Track<float> rotationScale;
		} overEmitterLife;
	} particles;
	struct
	{
		Range<float> cyclesTotal;
		Range<float> lifeTotal;
	} emitter;
	Material material;
	std::string technique;
	Texture colorMap;

	EmitterResource() :
		maxParticles(16),
		blending(Shape::Blending_Default),
		localSimulation(true)
	{
		initial.spawnArea.type = SpawnArea::Type_Point;
		initial.spawnCount = 0.0f;
		initial.velocity.Set(Vec2(-1, -1), Vec2(1, 1));
		initial.rotation.Set(-PI, PI);
		particles.overParticleLife.acceleration = Vec2(0, -10);
		particles.overParticleLife.color = Color::White;
		particles.overParticleLife.size = Vec2(50, 50);
		particles.overParticleLife.rotationSpeed = 0.5f * PI;
		particles.overEmitterLife.spawnCount = 10;
		particles.overEmitterLife.cyclesTotal = 1.0f;
		particles.overEmitterLife.lifeTotal = 2.0f;
		particles.overEmitterLife.color = Color::White;
		particles.overEmitterLife.sizeScale = 1.0f;
		particles.overEmitterLife.rotationScale = 1.0f;
		emitter.cyclesTotal = 1.0f;
		emitter.lifeTotal = 10.0f;
	}
};

struct EffectResource : Resource
{
	std::vector<EmitterResource> emitters;

	EffectResource() : Resource("effect") {}
};

struct Particle
{
	float seed;
	float lifeLength;
	float lifeTotal;
	float cyclesCount;
	float cyclesTotal;
	Vec2 pos;
	Vec2 velocity;
	float rotation;
	Vec2 size;
	Color color;
};

struct Emitter
{
	bool toDelete;
	float seed;
	float spawnAccumulator;
	float lifeLength;
	float lifeTotal;
	float cyclesCount;
	float cyclesTotal;
	std::vector<Particle> particles;

	Emitter() :
		toDelete(false),
		spawnAccumulator(0.0f),
		lifeLength(0.0f),
		cyclesCount(0.0f)
	{}
};

struct Transform
{
	Vec2 pos;
	float rotation;
	float scale;

	Transform() :
		pos(0, 0),
		rotation(0),
		scale(1)
	{}
};

struct EffectObj
{
	bool toDelete;
	Transform previousTransform;
	Transform transform;
	Transform newTransform;
	EffectResource* resource;
	std::vector<Emitter> emitters;
	float spawnMultiplier;

	EffectObj() :
		toDelete(false),
		resource(NULL),
		spawnMultiplier(1.0f)
	{}
};

void Emitter_SpawnParticles(EffectObj* effect, Emitter* emitter, EmitterResource* resource, float deltaTime)
{
	const bool loopEmitterParams = !(resource->emitter.cyclesTotal == 1.0f);

	const float emitterLifeLengthNormalized = emitter->lifeLength / emitter->lifeTotal;

	if (emitter->spawnAccumulator >= 1.0f)
	{
		float numParticlesToSpawn = floorf(emitter->spawnAccumulator);
		emitter->spawnAccumulator -= numParticlesToSpawn;

		float emitterLifeTime = emitter->lifeLength;
		const float emitterLifeTimeStep = deltaTime / numParticlesToSpawn;

		Vec2 pos(0, 0);
		Vec2 posStep(0, 0);
		if (!resource->localSimulation)
		{
			posStep = (effect->transform.pos - effect->previousTransform.pos) / numParticlesToSpawn;
			pos = effect->previousTransform.pos;
		}

		const SpawnArea spawnArea = resource->initial.spawnArea;

		while (numParticlesToSpawn >= 1.0f)
		{
			numParticlesToSpawn -= 1.0f;

			if ((int) emitter->particles.size() == resource->maxParticles)
				continue;

			Particle& particle = vector_add(emitter->particles);
			particle.seed = Random::GetFloat(0.0f, 1.0f);
			particle.lifeLength = 0.0f;
			particle.lifeTotal = resource->particles.overEmitterLife.lifeTotal.SampleAt(emitterLifeLengthNormalized, Random::GetFloat(), loopEmitterParams);
			particle.cyclesCount = 0.0f;
			particle.cyclesTotal = resource->particles.overEmitterLife.cyclesTotal.SampleAt(emitterLifeLengthNormalized, Random::GetFloat(), loopEmitterParams);
			particle.pos = pos;
			particle.velocity = resource->initial.velocity.Generate();
			particle.rotation = resource->initial.rotation.Generate();
			particle.size = resource->particles.overParticleLife.size.SampleAt(0.0f, SeedToVec2(particle.seed, Seed_Size), false) * resource->particles.overEmitterLife.sizeScale.SampleAt(emitterLifeLengthNormalized, Random::GetFloat(), loopEmitterParams);
			particle.color = resource->particles.overParticleLife.color.SampleAt(0.0f, SeedToFloat(particle.seed, Seed_Color), false) * resource->particles.overEmitterLife.color.SampleAt(emitterLifeLengthNormalized, Random::GetFloat(), loopEmitterParams);

			switch (spawnArea.type)
			{
			    case SpawnArea::Type_Point:
			        break;
				case SpawnArea::Type_Circle:
				{
					float distanceFromCenter = Random::GetFloat(0, spawnArea.circle.radius * 2.0f);
					if (distanceFromCenter > spawnArea.circle.radius)
						distanceFromCenter = spawnArea.circle.radius * 2.0f - distanceFromCenter;
					Vec2 offset(0, distanceFromCenter);
					offset.Rotate(Random::GetFloat(0, 2 * PI));
					particle.pos += offset;
					break;
				}
				case SpawnArea::Type_Rectangle:
				{
					const Vec2 offset(Random::GetFloat(0, spawnArea.rectangle.width) - spawnArea.rectangle.width * 0.5f, Random::GetFloat(0, spawnArea.rectangle.height) - spawnArea.rectangle.height * 0.5f);
					particle.pos += offset;
					break;
				}
			}

			if (!resource->localSimulation)
			{
				particle.size *= effect->transform.scale;

				particle.rotation += effect->transform.rotation;

				particle.velocity *= effect->transform.scale;
				particle.velocity.Rotate(effect->transform.rotation);
			}

			emitterLifeTime += emitterLifeTimeStep;
			pos += posStep;
		}
	}
}

void Emitter_Update(EffectObj* effect, Emitter* emitter, EmitterResource* resource, float deltaTime)
{
	bool toDelete = effect->toDelete || emitter->toDelete;

	const bool isEmitterEndless = resource->emitter.cyclesTotal == 0.0f;
	const bool areParticlesEndless = resource->particles.overEmitterLife.cyclesTotal == 0.0f;

	if (toDelete)
	{
		if (areParticlesEndless)
			emitter->particles.clear();
		if (!emitter->particles.size())
			return;
	}

	// Update emitter

	if (!emitter->toDelete)
	{
		emitter->lifeLength += deltaTime;
		if (emitter->lifeLength >= emitter->lifeTotal)
		{
			if (emitter->cyclesCount + 1.0f >= emitter->cyclesTotal && !isEmitterEndless)
			{
				emitter->lifeLength = emitter->lifeTotal;
				emitter->toDelete = true;
				if (areParticlesEndless)
					emitter->particles.clear();
				if (!emitter->particles.size())
					return;
			}
			else
			{
				emitter->lifeLength -= emitter->lifeTotal;
				emitter->cyclesCount += 1.0f;
			}
		}
	}

	const float emitterLifeLengthNormalized = emitter->lifeLength / emitter->lifeTotal;
	const bool loopEmitterParams = !(resource->emitter.cyclesTotal == 1.0f) && !emitter->toDelete;

	const Color emitterColor = resource->particles.overEmitterLife.color.SampleAt(emitterLifeLengthNormalized, SeedToFloat(emitter->seed, Seed_Color), loopEmitterParams);
	const float emitterSizeScale = resource->particles.overEmitterLife.sizeScale.SampleAt(emitterLifeLengthNormalized, SeedToFloat(emitter->seed, Seed_SizeScale), loopEmitterParams);
	const float emitterRotationScale = resource->particles.overEmitterLife.rotationScale.SampleAt(emitterLifeLengthNormalized, SeedToFloat(emitter->seed, Seed_RotationScale), loopEmitterParams);

	const bool loopParticleParams = !(resource->particles.overEmitterLife.cyclesTotal == 1.0f);
	const bool hasVelocityData = resource->particles.overParticleLife.velocity.isLoaded;

	// Update particle life time and delete dead particles (preserve the order)

	if (emitter->particles.size())
	{
		int numToRemove = 0;
		std::vector<Particle>::iterator dst = emitter->particles.begin();
		std::vector<Particle>::iterator end = emitter->particles.end();
		for (std::vector<Particle>::iterator it = emitter->particles.begin(); it != end; ++it)
		{
			it->lifeLength += deltaTime;

			if (it->lifeLength >= it->lifeTotal)
			{
				it->lifeLength -= it->lifeTotal;
				it->cyclesCount += 1.0f;
				if (it->cyclesCount >= it->cyclesTotal && !areParticlesEndless)
				{
					numToRemove++;
					continue;
				}
			}

			if (it != dst)
				*dst = *it;

			++dst;
		}

		if (numToRemove)
			emitter->particles.resize(emitter->particles.size() - numToRemove);
	}

	// Simulate alive particles

	for (std::vector<Particle>::iterator it = emitter->particles.begin(); it != emitter->particles.end(); ++it)
	{
		const float lifeLengthNormalized = it->lifeLength / it->lifeTotal;

		if (hasVelocityData)
		{
			it->velocity = resource->particles.overParticleLife.velocity.SampleAt(lifeLengthNormalized, SeedToVec2(it->seed, Seed_Velocity), loopParticleParams);
			if (!resource->localSimulation)
			{
				it->velocity *= effect->transform.scale;
				it->velocity.Rotate(effect->transform.rotation);
			}
		}
		else
		{
			Vec2 acceleration = resource->particles.overParticleLife.acceleration.SampleAt(lifeLengthNormalized, SeedToVec2(it->seed, Seed_Acceleration), loopParticleParams);
			if (!resource->localSimulation)
			{
				acceleration *= effect->transform.scale;
				acceleration.Rotate(effect->transform.rotation);
			}

			it->velocity += acceleration * deltaTime;
		}
		it->pos += it->velocity * deltaTime;

		it->color = resource->particles.overParticleLife.color.SampleAt(lifeLengthNormalized, SeedToFloat(it->seed, Seed_Color), loopParticleParams) * emitterColor;

		it->size = resource->particles.overParticleLife.size.SampleAt(lifeLengthNormalized, SeedToVec2(it->seed, Seed_Size), loopParticleParams) * emitterSizeScale;

		it->rotation += resource->particles.overParticleLife.rotationSpeed.SampleAt(lifeLengthNormalized, SeedToFloat(it->seed, Seed_RotationSpeed), loopParticleParams) * emitterRotationScale * deltaTime;
	}

	// Spawn new ones

	if (toDelete)
		return;

	emitter->spawnAccumulator += resource->particles.overEmitterLife.spawnCount.SampleAt(emitterLifeLengthNormalized, SeedToFloat(emitter->seed, Seed_SpawnCount), loopEmitterParams) * effect->spawnMultiplier * deltaTime;

	Emitter_SpawnParticles(effect, emitter, resource, deltaTime);
}

bool Effect_Update(EffectObj* effect, float deltaTime)
{
	// Update transform

	effect->previousTransform = effect->transform;
	effect->transform = effect->newTransform;

	// Update emitters

	int numParticles = 0;
	bool allEmittersToDelete = true;
	for (unsigned int i = 0; i < effect->emitters.size(); i++)
	{
		Emitter* emitter = &effect->emitters[i];
		Emitter_Update(effect, emitter, &effect->resource->emitters[i], deltaTime);
		numParticles += emitter->particles.size();
		allEmittersToDelete &= emitter->toDelete;
	}

	// Delete effect?

	if ((effect->toDelete || allEmittersToDelete) && !numParticles)
	{
		Effect_Destroy(effect);
		return false;
	}

	return true;
}

struct ParticleVertex
{
	Vec2 pos;
	Vec2 tex;
	Color color;
};

void Emitter_Draw(EffectObj* effect, Emitter* emitter, EmitterResource* resource)
{
	if (emitter->particles.empty())
		return;

	// Generate verts

	std::vector<ParticleVertex> verts(emitter->particles.size() * 6);
	ParticleVertex* v = &verts[0];
	for (std::vector<Particle>::iterator it = emitter->particles.begin(); it != emitter->particles.end(); ++it, v += 6)
	{
		Vec2 pos = it->pos;
		if (resource->localSimulation)
		{
			pos *= effect->transform.scale;
			pos += effect->transform.pos;
			pos.Rotate(effect->transform.rotation);
		}

		v[0].pos = pos - it->size;
		v[1].pos = pos + Vec2(it->size[0], -it->size[1]);
		v[2].pos = pos + it->size;
		v[3].pos = pos + Vec2(-it->size[0], it->size[1]);

		v[0].tex.Set(0, 0);
		v[1].tex.Set(1, 0);
		v[2].tex.Set(1, 1);
		v[3].tex.Set(0, 1);

		for (int i = 0; i < 4; i++)
		{
			v[i].color = it->color;
			v[i].pos.Rotate(it->rotation, pos);
		}

		// Finalize second triangle of the particle quad

		v[4] = v[0];
		v[5] = v[2];
	}

	// Draw

	Shape::DrawParams params;
	params.SetGeometryType(Shape::Geometry::Type_Triangles);
	params.SetNumVerts(verts.size());
	params.SetPosition(&verts[0].pos, sizeof(ParticleVertex));
	params.SetTexCoord(&verts[0].tex, 0, sizeof(ParticleVertex));
	params.SetColor(&verts[0].color, 0, sizeof(ParticleVertex));

	Material& material = resource->material.IsValid() ? resource->material : App::GetDefaultMaterial();

	material.SetTechnique(resource->technique.empty() ? "tex_vcol" : resource->technique);
	material.SetTextureParameter("ColorMap", resource->colorMap);
	material.Draw(&params);
}

void Effect_Draw(EffectObj* effect)
{
	for (unsigned int i = 0; i < effect->emitters.size(); i++)
		Emitter_Draw(effect, &effect->emitters[i], &effect->resource->emitters[i]);
}

EffectResource* EffectResource_Load(const std::string& name, bool immediate)
{
	const std::string path = name + ".effect.xml";

	XMLDoc doc;
	if (!doc.Load(path))
	{
		Log::Error(string_format("Failed to load particle effect XML from %s", path.c_str()));
		return NULL;
	}

	XMLNode* effectNode = doc.AsNode()->GetFirstNode("effect");
	if (!effectNode)
	{
		Log::Error(string_format("Invalid particle effect XML file %s, reason: root 'effect' element not found", path.c_str()));
		return NULL;
	}

	XMLNode* emittersNode = effectNode->GetFirstNode("emitters");
	if (!emittersNode)
	{
		Log::Error(string_format("Invalid particle effect XML file %s, reason: 'emitters' element not found", path.c_str()));
		return NULL;
	}

	EffectResource* resource = new EffectResource();
	resource->name = name;
	resource->state = ResourceState_CreationInProgress;

	for (XMLNode* emitterNode = emittersNode->GetFirstNode("emitter"); emitterNode; emitterNode = emitterNode->GetNext("emitter"))
	{
		EmitterResource& emitter = vector_add(resource->emitters);

		const char* colorMapName = emitterNode->GetAttributeValue("colorMap");
		if (!colorMapName)
		{
			Log::Error(string_format("Emitter in particle effect %s doesn't define required 'colorMap' property", path.c_str()));
			delete resource;
			return NULL;
		}
		emitter.colorMap.Create(colorMapName, immediate);

		if (const char* materialName = emitterNode->GetAttributeValue("material"))
			emitter.material.Create(materialName, immediate);
		if (const char* techniqueName = emitterNode->GetAttributeValue("technique"))
			emitter.technique = techniqueName;

		emitterNode->GetAttributeValueInt("maxParticles", emitter.maxParticles, 16);
		emitter.maxParticles = min(emitter.maxParticles, 16384);

		emitterNode->GetAttributeValueBool("localSimulation", emitter.localSimulation, true);

		if (XMLNode* spawnAreaNode = emitterNode->GetFirstNode("spawnArea"))
		{
			const char* spawnAreaType = spawnAreaNode->GetAttributeValue("type");
			if (!strcmp(spawnAreaType, "point"))
				emitter.initial.spawnArea.type = SpawnArea::Type_Point;
			else if (!strcmp(spawnAreaType, "rectangle"))
			{
				emitter.initial.spawnArea.type = SpawnArea::Type_Rectangle;
				spawnAreaNode->GetAttributeValueFloat("width", emitter.initial.spawnArea.rectangle.width, 10.0f);
				spawnAreaNode->GetAttributeValueFloat("height", emitter.initial.spawnArea.rectangle.height, 10.0f);
			}
			else if (!strcmp(spawnAreaType, "circle"))
			{
				emitter.initial.spawnArea.type = SpawnArea::Type_Circle;
				spawnAreaNode->GetAttributeValueFloat("radius", emitter.initial.spawnArea.circle.radius, 10.0f);
			}
		}

		emitter.initial.spawnCount.Load( emitterNode->GetFirstNode("initialSpawnCount") );
		emitter.initial.velocity.Load( emitterNode->GetFirstNode("initialVelocity") );
		emitter.initial.rotation.Load( emitterNode->GetFirstNode("initialRotation") );
		emitter.particles.overParticleLife.velocity.Load( emitterNode->GetFirstNode("velocityOverParticleLife") );
		emitter.particles.overParticleLife.acceleration.Load( emitterNode->GetFirstNode("accelerationOverParticleLife") );
		emitter.particles.overParticleLife.color.Load( emitterNode->GetFirstNode("colorOverParticleLife") );
		emitter.particles.overParticleLife.size.Load( emitterNode->GetFirstNode("sizeOverParticleLife") );
		emitter.particles.overParticleLife.rotationSpeed.Load( emitterNode->GetFirstNode("rotationSpeedOverParticleLife") );
		emitter.particles.overEmitterLife.spawnCount.Load( emitterNode->GetFirstNode("spawnCountOverEmitterLife") );
		emitter.particles.overEmitterLife.cyclesTotal.Load( emitterNode->GetFirstNode("cyclesTotalOverEmitterLife") );
		emitter.particles.overEmitterLife.lifeTotal.Load( emitterNode->GetFirstNode("lifeTotalOverEmitterLife") );
		emitter.particles.overEmitterLife.color.Load( emitterNode->GetFirstNode("colorOverEmitterLife") );
		emitter.particles.overEmitterLife.sizeScale.Load( emitterNode->GetFirstNode("sizeScaleOverEmitterLife") );
		emitter.particles.overEmitterLife.rotationScale.Load( emitterNode->GetFirstNode("rotationScaleOverEmitterLife") );
		emitter.emitter.cyclesTotal.Load( emitterNode->GetFirstNode("emitterCyclesTotal") );
		emitter.emitter.lifeTotal.Load( emitterNode->GetFirstNode("emitterLifeTotal") );
	}

	return resource;
}

EffectObj* Effect_Create(const std::string& name, const Vec2& pos, float rotation, float scale, bool immediate)
{
	immediate = immediate || !g_supportAsynchronousResourceLoading;

	EffectResource* resource = static_cast<EffectResource*>(Resource_Find("effect", name));
	if (!resource && !(resource = EffectResource_Load(name, immediate)))
		return NULL;

	EffectObj* effect = new EffectObj();
	effect->transform.pos = pos;
	effect->transform.rotation = rotation;
	effect->transform.scale = scale;
	effect->newTransform = effect->previousTransform = effect->transform;
	effect->resource = resource;
	effect->emitters.resize( resource->emitters.size() );
	for (unsigned int i = 0; i < effect->emitters.size(); i++)
	{
		Emitter& emitter = effect->emitters[i];
		EmitterResource& emitterResource = resource->emitters[i];

		emitter.seed = Random::GetFloat(0.0f, 1.0f);
		emitter.cyclesTotal = emitterResource.emitter.cyclesTotal.Generate();
		emitter.lifeTotal = emitterResource.emitter.lifeTotal.Generate();
		emitter.spawnAccumulator = emitterResource.initial.spawnCount.Generate();

		Emitter_SpawnParticles(effect, &emitter, &emitterResource, 0.0f);
	}

	Resource_IncRefCount(effect->resource);

	return effect;
}

void Effect_Destroy(EffectObj* effect)
{
	if (!Resource_DecRefCount(effect->resource))
		delete effect->resource;
	delete effect;
}

void Effect_SetPosition(EffectObj* effect, const Vec2& pos)
{
	effect->newTransform.pos = pos;
}

void Effect_SetRotation(EffectObj* effect, float rotation)
{
	effect->newTransform.rotation = rotation;
}

void Effect_SetScale(EffectObj* effect, float scale)
{
	effect->newTransform.scale = scale;
}

void Effect_SetSpawnCountMultiplier(EffectObj* effect, float multiplier)
{
	effect->spawnMultiplier = multiplier;
}

EffectObj* Effect_Clone(EffectObj* other)
{
	EffectObj* effect = new EffectObj();
	effect->resource = other->resource;
	Resource_IncRefCount(effect->resource);

	effect->transform = other->transform;
	effect->newTransform = effect->previousTransform = effect->transform;
	effect->emitters.resize( effect->resource->emitters.size() );
	for (unsigned int i = 0; i < effect->emitters.size(); i++)
	{
		Emitter& emitter = effect->emitters[i];
		EmitterResource& emitterResource = effect->resource->emitters[i];

		emitter.seed = Random::GetFloat(0.0f, 1.0f);
		emitter.cyclesTotal = emitterResource.emitter.cyclesTotal.Generate();
		emitter.lifeTotal = emitterResource.emitter.lifeTotal.Generate();
		emitter.spawnAccumulator = emitterResource.initial.spawnCount.Generate();

		Emitter_SpawnParticles(effect, &emitter, &emitterResource, 0.0f);
	}

	return effect;
}
