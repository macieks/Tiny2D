#include "Tiny2D.h"
#include "Tiny2D_Common.h"

// Bloom

struct Bloom
{
	float blendFactor;
	int numBlurSteps;
	float blurKernel;
	Material material;
};

Bloom* bloom = NULL;

void Postprocessing::EnableBloom(bool enable, float blendFactor, float blurKernel, int numBlurSteps)
{
	if (!bloom && enable)
	{
		bloom = new Bloom();

		const int w = App::GetWidth();
		const int h = App::GetHeight();

		bloom->material.Create("common/postprocessing");

		bloom->blendFactor = blendFactor;
		bloom->blurKernel = blurKernel;
		bloom->numBlurSteps = numBlurSteps;
	}
	else if (bloom && !enable)
	{
		delete bloom;
		bloom = NULL;
	}
}

bool Postprocessing::IsBloomEnabled()
{
	return bloom != NULL;
}

void Postprocessing_DrawBloom(Texture& output, Texture& scene)
{
	Assert(bloom);

	const int width = App::GetWidth();
	const int height = App::GetHeight();

	// Downsample to half size

	Texture halfSizeRenderTexture = RenderTexturePool::Get(width / 2, height / 2);
	halfSizeRenderTexture.BeginDrawing();
	bloom->material.SetTechnique("downsample2x2");
	bloom->material.SetTextureParameter("ColorMap", scene);
	bloom->material.DrawFullscreenQuad();
	halfSizeRenderTexture.EndDrawing();

	// Downsample to quarter size

	Texture quarterSizeRenderTextures[2];

	quarterSizeRenderTextures[0] = RenderTexturePool::Get(width / 4, height / 4);
	quarterSizeRenderTextures[0].BeginDrawing();
	bloom->material.SetTextureParameter("ColorMap", halfSizeRenderTexture);
	bloom->material.DrawFullscreenQuad();
	quarterSizeRenderTextures[0].EndDrawing();
	RenderTexturePool::Release(halfSizeRenderTexture);

	// Blur downsampled scene vertically and horizontally

	if (bloom->numBlurSteps)
	{
		bloom->material.SetFloatParameter("BlurKernel", &bloom->blurKernel);
		quarterSizeRenderTextures[1] = RenderTexturePool::Get(width / 4, height / 4);
	}

	for (int i = 0; i < bloom->numBlurSteps; i++)
	{
		quarterSizeRenderTextures[1].BeginDrawing();
		bloom->material.SetTechnique("vertical_blur");
		bloom->material.SetTextureParameter("ColorMap", quarterSizeRenderTextures[0], Sampler::DefaultPostprocess);
		bloom->material.DrawFullscreenQuad();
		quarterSizeRenderTextures[1].EndDrawing();

		quarterSizeRenderTextures[0].BeginDrawing();
		bloom->material.SetTechnique("horizontal_blur");
		bloom->material.SetTextureParameter("ColorMap", quarterSizeRenderTextures[1], Sampler::DefaultPostprocess);
		bloom->material.DrawFullscreenQuad();
		quarterSizeRenderTextures[0].EndDrawing();
	}

	if (bloom->numBlurSteps)
		RenderTexturePool::Release(quarterSizeRenderTextures[1]);

	// Blend result with the scene

	output.BeginDrawing();
	bloom->material.SetTechnique("blend");
	bloom->material.SetTextureParameter("ColorMap0", quarterSizeRenderTextures[0]);
	bloom->material.SetTextureParameter("ColorMap1", scene, Sampler::DefaultPostprocess);
	bloom->material.SetFloatParameter("BlendFactor", &bloom->blendFactor);
	bloom->material.DrawFullscreenQuad();
	output.EndDrawing();

	RenderTexturePool::Release(quarterSizeRenderTextures[0]);
}

// Old TV

struct OldTV
{
	float time;
	Texture dustMap;
	Texture lineMap;
	Texture tvAndNoiseMap;
	Material material;
};

OldTV* oldtv = NULL;

bool Postprocessing::IsOldTVEnabled()
{
	return oldtv != NULL;
}

void Postprocessing::EnableOldTV(bool enable)
{
	if (!oldtv && enable)
	{
		oldtv = new OldTV();
		oldtv->time = 0.0f;
		oldtv->dustMap.Create("common/dust.png");
		oldtv->lineMap.Create("common/line.png");
		oldtv->tvAndNoiseMap.Create("common/tv.png");
		oldtv->material.Create("common/postprocessing");
	}
	else
	{
		delete oldtv;
		oldtv = NULL;
	}
}

void Postprocessing_DrawOldTV(Texture& renderTarget, Texture& scene)
{
	renderTarget.BeginDrawing();

	oldtv->material.SetTechnique("oldtv");

	oldtv->material.SetTextureParameter("ColorMap", scene, Sampler::DefaultPostprocess);
	oldtv->material.SetFloatParameter("Time", oldtv->time);

	oldtv->material.SetFloatParameter("OverExposureAmount", 0.1f);
	oldtv->material.SetFloatParameter("DustAmount", 4.0f);
	oldtv->material.SetFloatParameter("FrameJitterFrequency", 3.0f);
	oldtv->material.SetFloatParameter("MaxFrameJitter", 1.4f);
	const Color filmColor(1.0f, 0.7559052f, 0.58474624f, 1.0f);
	oldtv->material.SetFloatParameter("FilmColor", (const float*) &filmColor, 4);
	oldtv->material.SetFloatParameter("GrainThicknes", 1.0f);
	oldtv->material.SetFloatParameter("GrainAmount", 0.8f);
	oldtv->material.SetFloatParameter("ScratchesAmount", 3.0f);
	oldtv->material.SetFloatParameter("ScratchesLevel", 0.7f);

	Sampler dustSampler;
	dustSampler.SetFiltering(true, true);
	dustSampler.SetWrapMode(Sampler::WrapMode_ClampToBorder, Sampler::WrapMode_ClampToBorder, Color::White);
	oldtv->material.SetTextureParameter("DustMap", oldtv->dustMap, dustSampler);

	Sampler lineSampler;
	lineSampler.SetFiltering(false, false);
	lineSampler.SetWrapMode(Sampler::WrapMode_ClampToBorder, Sampler::WrapMode_Clamp, Color::White);
	oldtv->material.SetTextureParameter("LineMap", oldtv->lineMap, lineSampler);

	Sampler tvSampler;
	tvSampler.SetFiltering(true, true);
	tvSampler.SetWrapMode(Sampler::WrapMode_Clamp, Sampler::WrapMode_Clamp);
	oldtv->material.SetTextureParameter("TvMap", oldtv->tvAndNoiseMap, tvSampler);

	Sampler noiseSampler;
	noiseSampler.SetFiltering(true, true);
	noiseSampler.SetWrapMode(Sampler::WrapMode_Repeat, Sampler::WrapMode_Repeat);
	oldtv->material.SetTextureParameter("NoiseMap", oldtv->tvAndNoiseMap, noiseSampler);

	oldtv->material.DrawFullscreenQuad();

	renderTarget.EndDrawing();
}

void Postprocessing_UpdateOldTV(float deltaTime)
{
	Assert(oldtv);

	oldtv->time += deltaTime;
	if (oldtv->time > 100.0f)
		oldtv->time = 0.0f;
}

// Quake

struct Quake
{
	bool enabled;

	float time;

	float lastHitTime;
	float hitLength;
	Vec2 offset;
	Vec2 targetOffset;

	float strength;

	Material material;
};

Quake* quake = NULL;

void Postprocessing_ShutdownQuake()
{
	if (quake)
	{
		delete quake;
		quake = NULL;
	}
}

bool Postprocessing::IsQuakeEnabled()
{
	return quake && quake->enabled;
}

void Postprocessing::StartQuake(bool start, float strength, float length)
{
	if (!quake)
	{
		quake = new Quake();
		quake->material.Create("common/postprocessing");
		quake->enabled = false;
	}

	if (start)
	{
		quake->enabled = true;
		quake->time = 0.0f;
		quake->lastHitTime = 0;
		quake->hitLength = 0;
		quake->offset[0] = quake->offset[1] = 0.0f;
		quake->targetOffset[0] = quake->targetOffset[1] = 0.0f;
	}
	else
		quake->enabled = false;
}

void Postprocessing_UpdateQuake(float deltaTime)
{
	if (!quake->enabled)
		return;

	quake->time += deltaTime;

	// Update quake

	bool toCenter = true;

	if (quake->time - quake->lastHitTime > quake->hitLength)
	{
		const float hitSizeX = 0.02f;
		const float hitSizeY = 0.1f;

		const float startFadeOutTime = 0.2f;
		const float fadeOutTime = 0.15f;

		quake->targetOffset.Set(
			hitSizeX * ((Random::GetInt() & 1) ? 1.0f : -1.0f),
			hitSizeY * ((Random::GetInt() & 1) ? 1.0f : -1.0f));
		if (quake->lastHitTime > startFadeOutTime)
		{
			quake->targetOffset *= max(0.0f, (fadeOutTime - (quake->lastHitTime - startFadeOutTime)) / fadeOutTime);
		}

		quake->lastHitTime = quake->time;
		quake->hitLength = Random::GetFloat(0.1f, 0.1f);

		if (toCenter)
			quake->offset = quake->targetOffset;
	}

	Vec2 toTargetDir;
	if (toCenter)
		toTargetDir = Vec2(0, 0) - quake->offset;
	else
		toTargetDir = quake->targetOffset - quake->offset;
	float toTargetLength = toTargetDir.Length();
	toTargetDir.Normalize();

	float moveBy = clamp(toTargetLength, 0.0f, (toCenter ? 0.8f : 1.0f) * deltaTime);

	quake->offset += toTargetDir * moveBy;

	if (quake->offset.Length() <= 0.001f)
		quake->enabled = false;
}

void Postprocessing_DrawQuake(Texture& output, Texture& scene)
{
	output.BeginDrawing(&Color::Black);
	quake->material.SetTechnique("quake");
	quake->material.SetTextureParameter("ColorMap", scene);
	quake->material.SetFloatParameter("Offset", (const float*) &quake->targetOffset, 2);
	quake->material.DrawFullscreenQuad();
	output.EndDrawing();
}

struct RainyGlass
{
	float time;
	float deltaTime;

	struct Droplet
	{
		float size;
		Vec2 pos;
		Vec2 velocity;
		float nextVelocityChangeTime;
	};

	Texture dropletTexture;

	std::vector<Droplet> droplets;
	unsigned int maxDroplets;
	float lastSpawnTime;
	float spawnFrequency;

	Texture dropletBuffer;

	Material material;

	Color dropletColor;
	float rainEvaporation;
};

RainyGlass* rain = NULL;

bool Postprocessing::IsRainyGlassEnabled()
{
	return rain != NULL;
}

void Postprocessing::EnableRainyGlass(bool enable, float spawnFrequency)
{
	if (!rain && enable)
	{
		rain = new RainyGlass();
		rain->time = 0.0f;
		rain->dropletTexture.Create("common/rain_droplet.png");
		rain->material.Create("common/postprocessing");
		rain->dropletColor = Color(0.2f, 0.1f, 0.7f, 1.0f) * 0.35f;

		rain->maxDroplets = 256;
		rain->time = 0.0f;
		rain->lastSpawnTime = 0;
		rain->rainEvaporation = 0;
	}
	else
	{
		delete rain;
		rain = NULL;
	}

	if (enable)
	{
		rain->spawnFrequency = spawnFrequency;
	}
}

void Postprocessing_UpdateRainyGlass(float deltaTime)
{
	rain->deltaTime = min(1.0f / 30.0f, deltaTime);
}

void Postprocessing_UpdateRainyGlassStep(float deltaTime)
{
	rain->time += deltaTime;

	const float dropletSpeed = 0.7f;
	const float dropletMinSpawnSize = 0.01f;
	const float dropletMaxSpawnSize = 0.02f;
	const float dropletMaxSize = 0.05f;

	const float newDirPercentage = 0.2f;
	const float minNextChangeTime = 0.05f;
	const float maxNextChangeTime = 0.5f;

	std::vector<RainyGlass::Droplet>& droplets = rain->droplets;

	// Kill old droplets

	for (unsigned int i = 0; i < droplets.size(); )
	{
		RainyGlass::Droplet& d = droplets[i];
		if (d.pos.y > 1.0f + d.size * 0.5f)
		{
			d = droplets.back();
			droplets.pop_back();
		}
		else
			i++;
	}

	// Spawn new droplets

	if (rain->time - rain->lastSpawnTime > rain->spawnFrequency && droplets.size() < rain->maxDroplets)
	{
		rain->lastSpawnTime = rain->time;

		RainyGlass::Droplet& d = vector_add(droplets);
		d.size = Random::GetFloat(dropletMinSpawnSize, dropletMaxSpawnSize);
		d.pos.Set(Random::GetFloat(0, 1), 0 - d.size);

		d.velocity.Set(Random::GetFloat(-0.4f, 0.0f), Random::GetFloat(1.0f, 1.0f));
		d.velocity.Normalize();
		d.velocity *= dropletSpeed * (d.size / dropletMaxSize);
		d.nextVelocityChangeTime = rain->time + Random::GetFloat(minNextChangeTime, maxNextChangeTime);
	}

	// Update existing droplets

	std::vector<RainyGlass::Droplet>::iterator dropletsEnd = droplets.end();
	for (std::vector<RainyGlass::Droplet>::iterator it = droplets.begin(); it != dropletsEnd; ++it)
	{
		if (it->nextVelocityChangeTime <= rain->time)
		{
			Vec2 newDir(Random::GetFloat(-1, 1), Random::GetFloat(0.5f, 1.0f));
			newDir.Normalize();

			it->velocity.Normalize();
			it->velocity = Vec2::Lerp(it->velocity, newDir, newDirPercentage);
			it->velocity.Normalize();
			it->velocity *= dropletSpeed * (it->size / dropletMaxSize);
			it->nextVelocityChangeTime = rain->time + Random::GetFloat(minNextChangeTime, maxNextChangeTime);
		}
		it->pos += it->velocity * deltaTime;
	}

	rain->rainEvaporation += deltaTime;
}

void Postprocessing_DrawRainyGlass(Texture& renderTarget, Texture& scene)
{
	Texture dropletTarget = RenderTexturePool::Get(256, 256);

	// Clear droplet render target to default front-facing direction

	const Color frontClearColor(0.5f /* x == 0 */, 0.5f /* y = 0 */, 1.0f /* z = 1 */, 0.0f);
	dropletTarget.BeginDrawing(&frontClearColor);

	// Evaporation

	if (rain->dropletBuffer.IsValid())
	{
		if (rain->rainEvaporation > 0.02f) // Apply rain evaporation
		{
			rain->material.SetTechnique("rain_evaporation");
			rain->rainEvaporation = 0;
		}
		else // Just copy previous buffer (skip evaporation step this time)
		{
			rain->material.SetTechnique("copy_texture");
		}

		rain->material.SetTextureParameter("ColorMap", rain->dropletBuffer, Sampler::DefaultPostprocess);
		rain->material.DrawFullscreenQuad();
	}

	// Droplets

	const Vec2 sizeScale( (float) dropletTarget.GetWidth(), (float) dropletTarget.GetHeight() );

	while (rain->deltaTime > 0.0f)
	{
		const float stepDeltaTime = min(rain->deltaTime, 1 / 60.0f);
		rain->deltaTime -= stepDeltaTime;
		Postprocessing_UpdateRainyGlassStep(stepDeltaTime);

		const unsigned int numDroplets = rain->droplets.size();
		if (!numDroplets)
			continue;

		std::vector<Vec2> verts(numDroplets * 4);
		std::vector<Vec2> tex(numDroplets * 4);
		std::vector<unsigned short> indices(numDroplets * 6);

		int currVertexIndex = 0;
		unsigned short* currIndex = &indices[0];

		std::vector<RainyGlass::Droplet>::iterator dropletsEnd = rain->droplets.end();
		for (std::vector<RainyGlass::Droplet>::iterator it = rain->droplets.begin(); it != dropletsEnd; ++it)
		{
			Vec2 size = Vec2(it->size, it->size);
			Vec2 pos = it->pos - size * 0.5f;

			size *= sizeScale;
			pos *= sizeScale;

			Vec2* currVertex = &verts[currVertexIndex];
			*(currVertex++) = pos;
			*(currVertex++) = pos + Vec2(size.x, 0.0f);
			*(currVertex++) = pos + size;
			*(currVertex++) = pos + Vec2(0.0f, size.y);

			Vec2* currTex = &tex[currVertexIndex];
			(currTex++)->Set(0.0f, 0.0f);
			(currTex++)->Set(1.0f, 0.0f);
			(currTex++)->Set(1.0f, 1.0f);
			(currTex++)->Set(0.0f, 1.0f);

			*(currIndex++) = currVertexIndex;
			*(currIndex++) = currVertexIndex + 1;
			*(currIndex++) = currVertexIndex + 2;
			*(currIndex++) = currVertexIndex;
			*(currIndex++) = currVertexIndex + 2;
			*(currIndex++) = currVertexIndex + 3;

			currVertexIndex += 4;
		}

		Shape::DrawParams drawParams;
		drawParams.SetGeometryType(Shape::Geometry::Type_Triangles);
		drawParams.SetNumVerts(verts.size());
		drawParams.SetPosition(&verts[0], sizeof(Vec2));
		drawParams.SetTexCoord(&tex[0], 0, sizeof(Vec2));
		drawParams.SetIndices(indices.size(), &indices[0]);

		Material& defaultMaterial = App::GetDefaultMaterial();

		defaultMaterial.SetTechnique("tex_col");
		defaultMaterial.SetFloatParameter("Color", (const float*) &Color::White, 4);
		defaultMaterial.SetTextureParameter("ColorMap", rain->dropletTexture);
		defaultMaterial.Draw(&drawParams);
	}

	dropletTarget.EndDrawing();

	// Swap droplet buffer

	if (rain->dropletBuffer.IsValid())
		RenderTexturePool::Release(rain->dropletBuffer);
	rain->dropletBuffer = dropletTarget;

	// Apply droplet buffer distortion to scene

	renderTarget.BeginDrawing();

	Sampler colorSampler = Sampler::DefaultPostprocess;
	colorSampler.minFilterLinear = true;
	colorSampler.magFilterLinear = true;

	rain->material.SetTechnique("rain_distortion");
	rain->material.SetFloatParameter("DropletColor", (const float*) &rain->dropletColor, 4);
	rain->material.SetTextureParameter("ColorMap", scene, colorSampler);
	rain->material.SetTextureParameter("NormalMap", rain->dropletBuffer);
	rain->material.DrawFullscreenQuad();

	renderTarget.EndDrawing();
}