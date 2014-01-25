#include "Tiny2D.h"
#include "Tiny2D_Common.h"

namespace Tiny2D
{

Sprite::DrawParams::DrawParams() :
	color(Color::White),
	position(10, 10),
	rect(NULL),
	texCoordRect(NULL),
	scale(1),
	rotation(0),
	flipX(false),
	flipY(false)
{}

bool SpriteResource_CheckCreated(SpriteResource* resource)
{
	if (resource->state != ResourceState_Creating)
		return resource->state == ResourceState_Created;

	if (resource->hasAtlas)
	{
		const Resource* texture = (const Resource*) Texture_Get(resource->atlas);
		if (texture->state == ResourceState_Creating)
			return false;
		else if (texture->state == ResourceState_AsyncError)
		{
			resource->state = ResourceState_AsyncError;
			Log::Error(string_format("Sprite %s texture atlas failed to load (asynchronously)", resource->name.c_str()));
			return false;
		}
	}
	else for (std::map<std::string, SpriteResource::Animation>::iterator it = resource->animations.begin(); it != resource->animations.end(); ++it)
		for (std::vector<SpriteResource::Frame>::iterator it2 = it->second.frames.begin(); it2 != it->second.frames.end(); ++it2)
		{
			const Resource* texture = (const Resource*) Texture_Get(it2->texture);
			if (texture->state == ResourceState_Creating)
				return false;
			else if (texture->state == ResourceState_AsyncError)
			{
				resource->state = ResourceState_AsyncError;
				Log::Error(string_format("Sprite %s failed to load (asynchronously)", resource->name.c_str()));
				return false;
			}
		}

	resource->state = ResourceState_Created;

	if (resource->hasAtlas)
	{
		resource->width = (int) resource->defaultAnimation->frames[0].rectangle.width;
		resource->height = (int) resource->defaultAnimation->frames[0].rectangle.height;
	}
	else
	{
		resource->width = resource->defaultAnimation->frames[0].texture.GetWidth();
		resource->height = resource->defaultAnimation->frames[0].texture.GetHeight();
	}
	Assert(resource->width && resource->height);

	// Invert atlas entry rectangles

	if (resource->hasAtlas)
	{
		const float atlasWidth = (float) resource->atlas.GetWidth();
		const float atlasHeight = (float) resource->atlas.GetHeight();

		for (std::map<std::string, SpriteResource::Animation>::iterator it = resource->animations.begin(); it != resource->animations.end(); ++it)
			for (std::vector<SpriteResource::Frame>::iterator it2 = it->second.frames.begin(); it2 != it->second.frames.end(); ++it2)
			{
				Rect& rect = it2->rectangle;
				rect.left = rect.left / atlasWidth;
				rect.top = rect.top / atlasHeight;
				rect.width = rect.width / atlasWidth;
				rect.height = rect.height / atlasHeight;
			}
	}

	return true;
}

SpriteObj* Sprite_Clone(SpriteObj* other)
{
	SpriteObj* sprite = new SpriteObj();
	sprite->resource = other->resource;
	Resource_IncRefCount(sprite->resource);
	Sprite_PlayAnimation(sprite);
	return sprite;
}

typedef std::map<std::string, Rect> AtlasInfo;

bool Sprite_LoadAtlasInfo(const std::string& atlasName, AtlasInfo& atlasInfo)
{
	const size_t dotIndex = atlasName.find_last_of('.');
	if (dotIndex == std::string::npos)
	{
		Log::Error("Sprite atlas " + atlasName + " has invalid extension");
		return false;
	}

	const std::string path = atlasName.substr(0, dotIndex) + ".xml";

	XMLDoc doc;
	if (!doc.Load(path))
	{
		Log::Error("Sprite atlas info not found under " + path);
		return false;
	}

	XMLNode* rootNode = doc.AsNode()->GetFirstNode();
	if (!rootNode)
	{
		Log::Error("Failed to load sprite atlas info from " + path + ", reason: root node 'XnaContent' not found.");
		return false;
	}

	XMLNode* subRootNode = rootNode->GetFirstNode();
	if (!subRootNode)
	{
		Log::Error("Failed to load sprite atlas info from " + path + ", reason: subroot node 'Asset' not found.");
		return false;
	}

	for (XMLNode* itemNode = XMLNode_GetFirstNode(subRootNode); itemNode; itemNode = XMLNode_GetNext(itemNode))
	{
		XMLNode* keyNode = XMLNode_GetFirstNode(itemNode);
		XMLNode* valueNode = XMLNode_GetNext(keyNode);
		if (!keyNode || !valueNode)
		{
			Log::Error("Failed to load sprite atlas info from " + path + ", reason: 'Item' node has no 'Key' or 'Value' child node.");
			return false;
		}

		const char* key = XMLNode_GetValue(keyNode);
		const char* value = XMLNode_GetValue(valueNode);
		if (!key || !value)
		{
			Log::Error("Failed to load sprite atlas info from " + path + ", reason: 'Key' or 'Value' are empty.");
			return false;
		}

		Rect rect;
		if (sscanf(value, "%f %f %f %f", &rect.left, &rect.top, &rect.width, &rect.height) != 4)
		{
			Log::Error("Failed to load sprite atlas info from " + path + ", reason: 'Value' couldn't be parsed into 4 numbers (left, top, width, height).");
			return false;
		}

		atlasInfo[key] = rect;
	}
	return true;
}

std::string ExtractFileName(const std::string& path)
{
	const size_t dotIndex = path.find_last_of('.');
	if (dotIndex == std::string::npos)
		return std::string();
	const size_t slashIndex = max(path.find_last_of('/'), path.find_last_of('\\'));
	if (slashIndex == std::string::npos)
		return path.substr(0, dotIndex);
	return path.substr(slashIndex, dotIndex - slashIndex);
}

SpriteObj* Sprite_Create(const std::string& name, bool immediate)
{
	immediate = immediate || !g_supportAsynchronousResourceLoading;

	SpriteResource* resource = static_cast<SpriteResource*>(Resource_Find("sprite", name));
	if (resource)
		Resource_IncRefCount(resource);

	// Create sprite from XML

	else if (!strstr(name.c_str(), "."))
	{
		const std::string path = name + ".sprite.xml";

		XMLDoc doc;
		if (!doc.Load(path))
		{
			Log::Error("Failed to load sprite resource from " + path);
			return NULL;
		}

		XMLNode* spriteNode = doc.AsNode()->GetFirstNode("sprite");
		if (!spriteNode)
		{
			Log::Error("Failed to load sprite resource from " + path + ", reason: root node 'sprite' not found.");
			return NULL;
		}

		MaterialObj* material = NULL;

		if (const char* materialName = XMLNode_GetAttributeValue(spriteNode, "material"))
		{
			material = Material_Create(materialName, immediate);
			if (!material)
			{
				Log::Error("Failed to load sprite resource from " + path + ", reason: can't load material " + materialName);
				return NULL;
			}
		}

		resource = new SpriteResource();
		resource->state = ResourceState_Creating;
		resource->name = name;
		Material_SetHandle(material, resource->material);
		Resource_IncRefCount(resource);

		// Load (optionally) texture atlas

		AtlasInfo atlasInfo;
		const char* atlasName = XMLNode_GetAttributeValue(spriteNode, "atlas");
		if (atlasName)
		{
			resource->atlas.Create(atlasName, immediate);
			if (!Texture_Get(resource->atlas))
			{
				delete resource;
				Log::Error("Failed to load sprite resource from " + path + ", reason: can't load texture atlas " + atlasName);
				return NULL;
			}
			if (!Sprite_LoadAtlasInfo(atlasName, atlasInfo))
			{
				delete resource;
				Log::Error("Failed to load sprite resource from " + path + ", reason: can't load texture atlas info " + atlasName);
				return NULL;
			}
			resource->hasAtlas = true;
		}

		// Load animations

		std::string defaultAnimationName;

		for (XMLNode* animNode = XMLNode_GetFirstNode(spriteNode, "animation"); animNode; animNode = XMLNode_GetNext(animNode, "animation"))
		{
			const std::string name = XMLNode_GetAttributeValue(animNode, "name");
			SpriteResource::Animation& anim = map_add(resource->animations, name);
			anim.name = name;

			// Get frame time

			XMLNode_GetAttributeValueFloat(animNode, "frameTime", anim.frameTime, 0.1f);

			// Check blend mode

			//XMLNode_GetAttributeValueBool(animNode, "blending", anim->blendFrames);

			// Check if default

			bool isDefault;
			if (defaultAnimationName.empty() || (XMLNode_GetAttributeValueBool(animNode, "isDefault", isDefault) && isDefault))
				defaultAnimationName = anim.name;

			// Load all frames and events

			float time = 0.0f;
			for (XMLNode* elemNode = XMLNode_GetFirstNode(animNode); elemNode; elemNode = XMLNode_GetNext(elemNode))
			{
				const char* elemName = XMLNode_GetName(elemNode);

				if (!strcmp(elemName, "frame"))
				{
					SpriteResource::Frame& frame = vector_add(anim.frames);
					const char* textureName = XMLNode_GetAttributeValue(elemNode, "texture");
					if (resource->hasAtlas)
					{
						const std::string fileName = ExtractFileName(textureName);
						Rect* rect = map_find(atlasInfo, fileName);
						if (!rect)
						{
							Log::Error("Failed to load sprite resource from " + path + ", reason: failed to find texture " + textureName + " in texture atlas " + atlasName);
							delete resource;
							return NULL;
						}
						frame.rectangle = *rect;
					}
					else
					{
						frame.texture.Create(textureName, immediate);
						if (frame.texture.GetState() != ResourceState_Created)
						{
							Log::Error("Failed to load sprite resource from " + path + ", reason: failed to load texture " + textureName);
							delete resource;
							return NULL;
						}
					}

					time += anim.frameTime;
				}
				else if (!strcmp(elemName, "event"))
				{
					SpriteResource::Event& ev = vector_add(anim.events);
					ev.time = time;
					ev.name = XMLNode_GetAttributeValue(elemNode, "name");
				}
			}

			anim.totalTime = (float) anim.frames.size() * anim.frameTime;
		}

		resource->defaultAnimation = &resource->animations[defaultAnimationName];

		SpriteResource_CheckCreated(resource);
	}

	// Create sprite from texture

	else
	{
		TextureObj* texture = Texture_Create(name, immediate);
		if (!texture)
		{
			Log::Error("Failed to create sprite resource from texture " + name);
			return NULL;
		}

		resource = new SpriteResource();
		resource->state = ResourceState_Creating;
		resource->name = name;
		Resource_IncRefCount(resource);

		// Add simple animation with one animation frame

		SpriteResource::Animation& animation = map_add(resource->animations, resource->name);
		animation.frameTime = 1.0f;
		animation.totalTime = 1.0f;
		SpriteResource::Frame& frame = vector_add(animation.frames);
		Texture_SetHandle(texture, frame.texture);

		resource->defaultAnimation = &animation;
		SpriteResource_CheckCreated(resource);

		Texture_Destroy(texture);
	}

	if (!resource)
		return NULL;

	// Create sprite

	SpriteObj* sprite = new SpriteObj();
	sprite->resource = resource;

	Sprite_PlayAnimation(sprite);

	return sprite;
}

void Sprite_Destroy(SpriteObj* sprite)
{
	if (!Resource_DecRefCount(sprite->resource))
		delete sprite->resource;
	delete sprite;
}

void Sprite_SetEventCallback(SpriteObj* sprite, Sprite::EventCallback callback, void* userData)
{
	sprite->callback = callback;
	sprite->userData = userData;
}

void Sprite_FireAnimationEvents(SpriteObj* sprite, SpriteResource::Animation* animation, float oldTime, float newTime, float dt)
{
	if (!sprite->callback)
		return;

	for (std::vector<SpriteResource::Event>::iterator it = animation->events.begin(); it != animation->events.end(); ++it)
		if (oldTime <= it->time && it->time < newTime)
			sprite->callback(it->name, it->value, sprite->userData);
}

void Sprite_Update(SpriteObj* sprite, float dt)
{
	std::vector<SpriteObj::AnimationInstance> animationInstancesCopy = sprite->animationInstances;
	sprite->animationInstances.clear();

	bool doneAnim = false;
	SpriteObj::AnimationInstance* instWhenDone = NULL;

	for (std::vector<SpriteObj::AnimationInstance>::iterator it = animationInstancesCopy.begin(); it != animationInstancesCopy.end(); ++it)
	{
		const float prevInstTime = it->time;

		if (it->mode == Sprite::AnimationMode_OnceWhenDone || it->mode == Sprite::AnimationMode_LoopWhenDone)
			instWhenDone = &(*it);
		else
			it->time += dt;

		if (it->time >= it->animation->totalTime)
		{
			switch (it->mode)
			{
			case Sprite::AnimationMode_Loop:
				it->time = fmod(it->time, it->animation->totalTime);
				Sprite_FireAnimationEvents(sprite, it->animation, prevInstTime, it->time, dt);
				doneAnim = true;
				break;
			case Sprite::AnimationMode_Once:
				Sprite_FireAnimationEvents(sprite, it->animation, prevInstTime, it->animation->totalTime, dt);
				doneAnim = true;
				continue;
			case Sprite::AnimationMode_OnceAndFreeze:
				it->time = it->animation->totalTime;
				Sprite_FireAnimationEvents(sprite, it->animation, prevInstTime, it->time, dt);
				break;
		    default:
			    Assert(!"Unsupported sprite animation mode");
		        break;
			}
		}
		else
			Sprite_FireAnimationEvents(sprite, it->animation, prevInstTime, it->time, dt);

		it->weight += it->weightChangeSpeed * dt;

		if (it->weight <= 0.0f)
			continue;

		if (it->weight >= 1.0f)
		{
			it->weight = 1.0f;
			it->weightChangeSpeed = 0.0f;
		}

		sprite->animationInstances.push_back(*it);
	}

	// Kick off "when done" instance

	if ((doneAnim || sprite->animationInstances.size() == 1) && instWhenDone)
	{
		instWhenDone->mode = (instWhenDone->mode == Sprite::AnimationMode_OnceWhenDone) ? Sprite::AnimationMode_Once : Sprite::AnimationMode_Loop;
		instWhenDone->weight = 1.0f;
		SpriteObj::AnimationInstance instWhenDoneObj = *instWhenDone;

		sprite->animationInstances.clear();
		sprite->animationInstances.push_back(instWhenDoneObj);
	}

	// Start default animation if there's no animations left

	if (!sprite->animationInstances.size())
		Sprite_PlayAnimation(sprite);
}

void Sprite_PlayAnimation(SpriteObj* sprite, const std::string& name, Sprite::AnimationMode mode, float transitionTime)
{
	// Get animation

	SpriteResource::Animation* animation = NULL;
	if (name == std::string())
		animation = sprite->resource->defaultAnimation;
	else
	{
		std::map<std::string, SpriteResource::Animation>::iterator it = sprite->resource->animations.find(name);
		if (it == sprite->resource->animations.end())
		{
			Log::Error(string_format("Animation %s not found in sprite %s", name.c_str(), sprite->resource->name.c_str()));
			return;
		}
		animation = &it->second;
	}

	// Check if not already played

	for (std::vector<SpriteObj::AnimationInstance>::iterator it = sprite->animationInstances.begin(); it != sprite->animationInstances.end(); ++it)
		if (it->animation == animation)
			return;

	// Fade out or kill all other animations

	const float weightChangeSpeed = transitionTime == 0.0f ? 0.0f : 1.0f / transitionTime;

	if (mode != Sprite::AnimationMode_OnceWhenDone && mode != Sprite::AnimationMode_LoopWhenDone)
	{
		if (transitionTime == 0.0f)
			sprite->animationInstances.clear();
		else
			for (std::vector<SpriteObj::AnimationInstance>::iterator it = sprite->animationInstances.begin(); it != sprite->animationInstances.end(); ++it)
				it->weightChangeSpeed = -weightChangeSpeed;
	}

	// Create animation instance

	SpriteObj::AnimationInstance& animationInstance = vector_add(sprite->animationInstances);
	animationInstance.animation = animation;
	animationInstance.mode = mode;
	animationInstance.time = 0.0f;
	animationInstance.weight = transitionTime == 0.0f ? 1.0f : 0.0f;
	animationInstance.weightChangeSpeed = weightChangeSpeed;
}

void Sprite_RectifySpriteUVs(float* uv, const Rect& rect)
{
	for (int i = 0; i < 8; i += 2)
	{
		uv[i] = uv[i] * rect.width + rect.left;
		uv[i + 1] = uv[i + 1] * rect.height + rect.top;
	}
}

void Sprite_Draw(SpriteObj* sprite, const Sprite::DrawParams* params)
{
	if (!SpriteResource_CheckCreated(sprite->resource))
		return;

	// Get animation instance

	SpriteObj::AnimationInstance* animationInstance = NULL;
	for (std::vector<SpriteObj::AnimationInstance>::iterator it = sprite->animationInstances.begin(); it != sprite->animationInstances.end(); ++it)
		if (it->mode != Sprite::AnimationMode_OnceWhenDone && it->mode != Sprite::AnimationMode_LoopWhenDone && (!animationInstance || it->weight > animationInstance->weight))
			animationInstance = &(*it);
	Assert(animationInstance);

	// Determine textures to draw

	Texture* texture0 = NULL;
	Texture* texture1 = NULL;
	Shape::DrawParams texParams;
	float lerp = 0.0f;

	float uv[8] =
	{
		0, 0,
		1, 0,
		1, 1,
		0, 1
	};

	float uv1[8];

	if (params->texCoordRect)
	{
		uv[0] = params->texCoordRect->left; uv[1] = params->texCoordRect->top;
		uv[2] = params->texCoordRect->Right(); uv[3] = params->texCoordRect->top;
		uv[4] = params->texCoordRect->Right(); uv[5] = params->texCoordRect->Bottom();
		uv[6] = params->texCoordRect->left; uv[7] = params->texCoordRect->Bottom();
	}

	if (params->flipX)
		for (int i = 0; i < 8; i += 2)
			uv[i] = 1.0f - uv[i];
	if (params->flipY)
		for (int i = 1; i < 8; i += 2)
			uv[i] = 1.0f - uv[i];

	SpriteResource::Animation* animation = animationInstance->animation;
	if (animation->frames.size() == 1)
	{
		if (sprite->resource->hasAtlas)
		{
			texture0 = &sprite->resource->atlas;
			Sprite_RectifySpriteUVs(uv, animation->frames[0].rectangle);
		}
		else
			texture0 = &animation->frames[0].texture;
		texParams.color = params->color;
		texParams.SetNumVerts(4);
		texParams.SetTexCoord(uv);
	}
	else
	{
		const float numFramesF = (float) animation->frames.size();
		const float frameIndexF = numFramesF * (animationInstance->time / animation->totalTime);
		const float firstFrameIndexF = floorf(frameIndexF);

		const int firstFrameIndex = clamp((int) firstFrameIndexF, (int) 0, (int) animation->frames.size() - 1);
		const int nextFrameIndex = (firstFrameIndex + 1) % animation->frames.size();

		SpriteResource::Frame& firstFrame = animation->frames[firstFrameIndex];
		SpriteResource::Frame& nextFrame = animation->frames[nextFrameIndex];

		if (sprite->resource->hasAtlas)
		{
			texture0 = texture1 = &sprite->resource->atlas;
			for (int i = 0; i < 8; i++)
				uv1[i] = uv[i];
			Sprite_RectifySpriteUVs(uv, firstFrame.rectangle);
			Sprite_RectifySpriteUVs(uv1, nextFrame.rectangle);
		}
		else
		{
			texture0 = &firstFrame.texture;
			texture1 = &nextFrame.texture;
		}

		texParams.SetNumVerts(4);
		texParams.SetTexCoord(uv, 0);
		texParams.SetTexCoord(uv1, 1);

		lerp = frameIndexF - firstFrameIndexF;
	}

	float xy[8] =
	{
		params->position.x, params->position.y,
		params->position.x + sprite->resource->width * params->scale, params->position.y,
		params->position.x + sprite->resource->width * params->scale, params->position.y + sprite->resource->height * params->scale,
		params->position.x, params->position.y + sprite->resource->height * params->scale
	};

	if (params->rect)
	{
		xy[0] = params->rect->left; xy[1] = params->rect->top;
		xy[2] = params->rect->Right(); xy[3] = params->rect->top;
		xy[4] = params->rect->Right(); xy[5] = params->rect->Bottom();
		xy[6] = params->rect->left; xy[7] = params->rect->Bottom();
	}

	if (params->rotation != 0)
	{
		const float centerX = (xy[0] + xy[2]) * 0.5f;
		const float centerY = (xy[1] + xy[5]) * 0.5f;

		const float rotationSin = sinf(params->rotation);
		const float rotationCos = cosf(params->rotation);

		float* xyPtr = xy;
		for (int i = 0; i < 4; i++, xyPtr += 2)
			Vertex_Rotate(xyPtr[0], xyPtr[1], centerX, centerY, rotationSin, rotationCos);
	}

	texParams.SetPosition(xy);

	// Draw

	Material* material = &sprite->resource->material;
	if (material->GetState() != ResourceState_Created)
		material = &App::GetDefaultMaterial();
	material->SetFloatParameter("Color", (const float*) &texParams.color, 4);
	if (texture1)
	{
		material->SetTechnique("tex_lerp_col");
		material->SetTextureParameter("ColorMap0", *texture0);
		material->SetTextureParameter("ColorMap1", *texture1);
		material->SetFloatParameter("Scale", &lerp);
		material->Draw(&texParams);
	}
	else
	{
		material->SetTechnique("tex_col");
		material->SetTextureParameter("ColorMap", *texture0);
		material->Draw(&texParams);
	}
}

int Sprite_GetWidth(SpriteObj* sprite)
{
	if (!SpriteResource_CheckCreated(sprite->resource))
		return 0;
	return sprite->resource->width;
}

int Sprite_GetHeight(SpriteObj* sprite)
{
	if (!SpriteResource_CheckCreated(sprite->resource))
		return 0;
	return sprite->resource->height;
}

};
