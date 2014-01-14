#include "Tiny2D.h"
#include "Tiny2D_Common.h"

Texture::Texture() : obj(NULL) {}
Texture::Texture(const Texture& other) : obj(NULL) { *this = other; }
Texture::~Texture() { Destroy(); }
void Texture::operator = (const Texture& other)
{
	if (obj == other.obj)
		return;
	Destroy();
	obj = const_cast<TextureObj*>(other.obj);
	if (obj)
		Resource_IncRefCount((Resource*) obj);
}
bool Texture::operator == (const Texture& other) const { return obj == other.obj; }
bool Texture::operator != (const Texture& other) const { return obj != other.obj; }
bool Texture::Create(const std::string& path, bool immediate) { if (obj) Texture_Destroy(obj); obj = Texture_Create(path, immediate); return obj != NULL; }
bool Texture::CreateRenderTarget(int width, int height) { if (obj) Texture_Destroy(obj); obj = Texture_CreateRenderTarget(width, height); return obj != NULL; }
void Texture::Destroy() { if (obj) { Texture_Destroy(obj); obj = NULL; } }
bool Texture::Save(const std::string& path, bool saveAlpha) { return obj && Texture_Save(obj, path, saveAlpha); }
bool Texture::IsValid() const { return obj != NULL; }
void Texture::Draw(const Shape::DrawParams* params, const Sampler& sampler) { if (obj) Texture_Draw(obj, params, sampler); }
void Texture::Draw(float left, float top, float rotation, float scale, const Color& color) { if (obj) Texture_Draw(obj, left, top, rotation, scale, color); }
int Texture::GetWidth() { return obj ? Texture_GetWidth(obj) : 0; }
int	Texture::GetHeight() { return obj ? Texture_GetHeight(obj) : 0; }
int Texture::GetRealWidth() { return obj ? Texture_GetRealWidth(obj) : 0; }
int	Texture::GetRealHeight() { return obj ? Texture_GetRealHeight(obj) : 0; }
bool Texture::GetPixels(std::vector<unsigned char>& pixels, bool removeAlpha) { return obj && Texture_GetPixels(obj, pixels, removeAlpha); }
void Texture::BeginDrawing(const Color* clearColor) { if (obj) Texture_BeginDrawing(obj, clearColor); }
void Texture::Clear(const Color& color) { if (obj) Texture_Clear(obj, color); }
void Texture::EndDrawing() { if (obj) Texture_EndDrawing(obj); }

Material::Material() : obj(NULL) {}
Material::Material(const Material& other) : obj(NULL) { *this = other; }
Material::~Material() { Destroy(); }
void Material::operator = (const Material& other) { Destroy(); obj = other.obj ? Material_Clone(const_cast<MaterialObj*>(other.obj)) : NULL; }
bool Material::Create(const std::string& name, bool immediate) { if (obj) Material_Destroy(obj); obj = Material_Create(name, immediate); return obj != NULL; }
void Material::Destroy() { if (obj) { Material_Destroy(obj); obj = NULL; } }
bool Material::IsValid() const { return obj != NULL; }
void Material::SetTechnique(const std::string& name) { if (obj) Material_SetTechnique(obj, name); }
int Material::GetParameterIndex(const std::string& name) { return obj ? Material_GetParameterIndex(obj, name) : -1; }
void Material::SetIntParameter(int index, const int* value, int count) { if (obj) Material_SetIntParameter(obj, index, value, count); }
void Material::SetIntParameter(const std::string& name, const int* value, int count) { if (obj) Material_SetIntParameter(obj, name, value, count); }
void Material::SetFloatParameter(int index, const float* value, int count) { if (obj) Material_SetFloatParameter(obj, index, value, count); }
void Material::SetFloatParameter(const std::string& name, const float* value, int count) { if (obj) Material_SetFloatParameter(obj, name, value, count); }
void Material::SetFloatParameter(const std::string& name, const float value) { if (obj) Material_SetFloatParameter(obj, name, &value, 1); }
void Material::SetTextureParameter(int index, Texture& value, const Sampler& sampler) { if (obj) Material_SetTextureParameter(obj, index, Texture_Get(value), sampler); }
void Material::SetTextureParameter(const std::string& name, Texture& value, const Sampler& sampler) { if (obj) Material_SetTextureParameter(obj, name, Texture_Get(value), sampler); }
void Material::Draw(const Shape::DrawParams* params) { if (obj) Material_Draw(obj, params); }
void Material::DrawFullscreenQuad() { if (obj) Material_DrawFullscreenQuad(obj); }

Font::Font() : obj(NULL) {}
Font::Font(const Font& other) : obj(NULL) { *this = other; }
Font::~Font() { Destroy(); }
void Font::operator = (const Font& other)
{
	if (obj == other.obj)
		return;
	Destroy();
	obj = const_cast<FontObj*>(other.obj);
	if (obj)
		Resource_IncRefCount((Resource*) obj);
}
bool Font::Create(const std::string& path, int size, unsigned int flags, bool immediate) { if (obj) Font_Destroy(obj); obj = Font_Create(path, size, flags, immediate); return obj != NULL; }
void Font::Destroy() { if (obj) { Font_Destroy(obj); obj = NULL; } }
bool Font::IsValid() const { return obj != NULL; }
void Font::CacheGlyphs(const std::string& text) { if (obj) Font_CacheGlyphs(obj, text); }
void Font::CacheGlyphsFromFile(const std::string& path) { if (obj) Font_CacheGlyphsFromFile(obj, path); }
void Font::Draw(const Text::DrawParams* params) { if (obj) Font_Draw(obj, params); }
void Font::Draw(const char* text, const Vec2& position, const Color& color) { if (obj) Font_Draw(obj, text, position, color); }
void Font::CalculateSize(const Text::DrawParams* params, float& width, float& height) { if (obj) Font_CalculateSize(obj, params, width, height); else width = height = 0.0f; }

Sprite::Sprite() : obj(NULL) {}
Sprite::Sprite(const Sprite& other) : obj(NULL) { *this = other; }
Sprite::~Sprite() { Destroy(); }
void Sprite::operator = (const Sprite& other) { Destroy(); obj = other.obj ? Sprite_Clone(const_cast<SpriteObj*>(other.obj)) : NULL; }
bool Sprite::Create(const std::string& name, bool immediate) { if (obj) Sprite_Destroy(obj); obj = Sprite_Create(name, immediate); return obj != NULL; }
void Sprite::Destroy() { if (obj) { Sprite_Destroy(obj); obj = NULL; } }
bool Sprite::IsValid() const { return obj != NULL; }
void Sprite::SetEventCallback(Sprite::EventCallback callback, void* userData) { if (obj) Sprite_SetEventCallback(obj, callback, userData); }
void Sprite::Update(float deltaTime) { if (obj) Sprite_Update(obj, deltaTime); }
void Sprite::PlayAnimation(const std::string& name, AnimationMode mode, float transitionTime) { if (obj) Sprite_PlayAnimation(obj, name, mode, transitionTime); }
void Sprite::Draw(const Sprite::DrawParams* params) { if (obj) Sprite_Draw(obj, params); }
void Sprite::Draw(const Vec2& position, float rotation) { if (obj) Sprite_Draw(obj, position, rotation); }
void Sprite::DrawCentered(const Vec2& center, float rotation) { Draw(Vec2(center.x - (float) GetWidth() * 0.5f, center.y - (float) GetHeight() * 0.5f), rotation); }
int Sprite::GetWidth() { return obj ? Sprite_GetWidth(obj) : 0; }
int Sprite::GetHeight() { return obj ? Sprite_GetHeight(obj) : 0; }

File::File() : file(NULL) {}
File::~File() { Close(); }
bool File::Open(const std::string& path, File::OpenMode openMode) { if (file) File_Close(file); file = File_Open(path, openMode); return file != NULL; }
void File::Close() { if (file) { File_Close(file); file = NULL; } }
int File::GetSize() { return file ? File_GetSize(file) : 0; }
void File::Seek(int offset) { if (file) File_Seek(file, offset); }
int File::GetOffset() { return file ? File_GetOffset(file) : 0; }
bool File::Read(void* dst, int size) { return file ? File_Read(file, dst, size) : false; }
bool File::Write(const void* src, int size) { return file ? File_Write(file, src, size) : false; }
File::File(const File&) {}
void File::operator = (const File&) {}

XMLDoc::XMLDoc() : doc(NULL) {}
XMLDoc::~XMLDoc() { Destroy(); }
bool XMLDoc::Load(const std::string& path) { if (doc) XMLDoc_Destroy(doc); doc = XMLDoc_Load(path); return doc != NULL; }
bool XMLDoc::Create(const std::string& version, const std::string& encoding) { if (doc) XMLDoc_Destroy(doc); doc = XMLDoc_Create(version, encoding); return doc != NULL; }
bool XMLDoc::Save(const std::string& path) { return doc ? XMLDoc_Save(doc, path) : false; }
void XMLDoc::Destroy() { if (doc) { XMLDoc_Destroy(doc); doc = NULL; } }
XMLNode* XMLDoc::AsNode() { return doc ? XMLDoc_AsNode(doc) : NULL; }
XMLDoc::XMLDoc(XMLDoc&) {}
void XMLDoc::operator = (XMLDoc&) {}

XMLNode* XMLNode::GetNext(const char* name) { return XMLNode_GetNext(this, name); }
const char* XMLNode::GetName() const { return XMLNode_GetName(this); }
const char* XMLNode::GetValue() const { return XMLNode_GetValue(this); }
XMLNode* XMLNode::GetFirstNode(const char* name) { return XMLNode_GetFirstNode(this, name); }
int XMLNode::CalcNumNodes(const char* name) { return XMLNode_CalcNumNodes(this, name); }
XMLAttribute* XMLNode::GetFirstAttribute(const char* name) { return XMLNode_GetFirstAttribute(this, name); }
const char* XMLNode::GetAttributeValue(const char* name) { return XMLNode_GetAttributeValue(this, name); }
bool XMLNode::GetAttributeValueBool(const char* name, bool& result) { return XMLNode_GetAttributeValueBool(this, name, result); }
bool XMLNode::GetAttributeValueFloat(const char* name, float& result) { return XMLNode_GetAttributeValueFloat(this, name, result); }
bool XMLNode::GetAttributeValueFloatArray(const char* name, float* result, int count) { return XMLNode_GetAttributeValueFloatArray(this, name, result, count); }
bool XMLNode::GetAttributeValueInt(const char* name, int& result) { return XMLNode_GetAttributeValueInt(this, name, result); }
const char* XMLNode::GetAttributeValue(const char* name, const char* defaultValue) { return XMLNode_GetAttributeValue(this, name, defaultValue); }
void XMLNode::GetAttributeValueBool(const char* name, bool& result, const bool defaultValue) { return XMLNode_GetAttributeValueBool(this, name, result, defaultValue); }
void XMLNode::GetAttributeValueFloat(const char* name, float& result, const float defaultValue) { return XMLNode_GetAttributeValueFloat(this, name, result, defaultValue); }
void XMLNode::GetAttributeValueInt(const char* name, int& result, const int defaultValue) { return XMLNode_GetAttributeValueInt(this, name, result, defaultValue); }
XMLNode* XMLNode::AddNode(const char* name) { return XMLNode_AddNode(this, name); }
XMLAttribute* XMLNode::AddAttribute(const char* name, const char* value) { return XMLNode_AddAttribute(this, name, value); }
XMLAttribute* XMLNode::AddAttributeBool(const char* name, bool value) { return XMLNode_AddAttributeBool(this, name, value); }
XMLAttribute* XMLNode::AddAttributeInt(const char* name, int value) { return XMLNode_AddAttributeInt(this, name, value); }
XMLAttribute* XMLNode::AddAttributeFloat(const char* name, float value) { return XMLNode_AddAttributeFloat(this, name, value); }
XMLNode::XMLNode() {}
XMLNode::XMLNode(const XMLNode&) {}
void XMLNode::operator = (const XMLNode&) {}

XMLAttribute* XMLAttribute::GetNext() { return XMLAttribute_GetNext(this); }
const char* XMLAttribute::GetName() const { return XMLAttribute_GetName(this); }
const char* XMLAttribute::GetValue() const { return XMLAttribute_GetValue(this); }
XMLAttribute::XMLAttribute() {}
XMLAttribute::XMLAttribute(const XMLAttribute&) {}
void XMLAttribute::operator = (const XMLAttribute&) {}

Sound::Sound() : obj(NULL) {}
Sound::Sound(const Sound& other) : obj(NULL) { *this = other; }
Sound::~Sound() { Destroy(); }
void Sound::operator = (const Sound& other) { Destroy(); obj = other.obj ? Sound_Clone(const_cast<SoundObj*>(other.obj)) : NULL; }
bool Sound::Create(const std::string& path, bool isMusic, bool immediate) { if (obj) Sound_Destroy(obj); obj = Sound_Create(path, isMusic, immediate); return obj != NULL; }
void Sound::Destroy(float fadeOutTime) { if (obj) { Sound_Destroy(obj, fadeOutTime); obj = NULL; } }
bool Sound::IsValid() const { return obj != NULL; }
void Sound::SetVolume(float volume) { if (obj) Sound_SetVolume(obj, volume); }
void Sound::SetPitch(float volume) { if (obj) Sound_SetPitch(obj, volume); }
void Sound::Play(bool loop, float fadeInTime) { if (obj) Sound_Play(obj, loop, fadeInTime); }
void Sound::Stop() { if (obj) Sound_Stop(obj); }
bool Sound::IsPlaying() { return obj ? Sound_IsPlaying(obj) : false; }

Effect::Effect() : obj(NULL) {}
Effect::Effect(const Effect& other) : obj(NULL) { *this = other; }
Effect::~Effect() { Destroy(); }
void Effect::operator = (const Effect& other) { Destroy(); obj = other.obj ? Effect_Clone(const_cast<EffectObj*>(other.obj)) : NULL; }
bool Effect::Create(const std::string& path, const Vec2& pos, float rotation, bool immediate) { if (obj) Effect_Destroy(obj); obj = Effect_Create(path, pos, rotation, immediate); return obj != NULL; }
void Effect::Destroy() { if (obj) { Effect_Destroy(obj); obj = NULL; } }
bool Effect::IsValid() const { return obj != NULL; }
void Effect::Update(float deltaTime) { if (obj && !Effect_Update(obj, deltaTime)) obj = NULL; }
void Effect::Draw() { if (obj) Effect_Draw(obj); }
void Effect::SetPosition(const Vec2& pos) { if (obj) Effect_SetPosition(obj, pos); }
void Effect::SetRotation(float rotation) { if (obj) Effect_SetRotation(obj, rotation); }
void Effect::SetScale(float scale) { if (obj) Effect_SetScale(obj, scale); }
void Effect::SetSpawnCountMultiplier(float multiplier) { if (obj) Effect_SetSpawnCountMultiplier(obj, multiplier); }
