#include "Tiny2D.h"
#include "Tiny2D_Common.h"

std::map<std::string, std::string> g_strings;

Localization::Param::Param() :
	type(Type_Uninitialized)
{}

Localization::Param::Param(const std::string& _name, int value) :
	name(_name),
	type(Type_Int),
	intValue(value)
{}

Localization::Param::Param(const std::string& _name, const char* value) :
	name(_name),
	type(Type_String),
	stringValue(value)
{}

Localization::Param::Param(const std::string& _name, const std::string& value) :
	name(_name),
	type(Type_String),
	stringValue(value)
{}


bool Localization::LoadSet(const std::string& name)
{
	const std::string path = name + "_" + App::GetLanguageSymbol() + ".translations.xml";

	XMLDoc doc;
	if (!doc.Load(path))
	{
		Log::Error("Failed to load localization set from " + path);
		return false;
	}

	XMLNode* translationsNode = doc.AsNode()->GetFirstNode("translations");
	if (!translationsNode)
	{
		Log::Error("Failed to find 'translationSet' node in " + path);
		return false;
	}

	// Load groups

	for (XMLNode* groupNode = XMLNode_GetFirstNode(translationsNode, "group"); groupNode; groupNode = XMLNode_GetNext(groupNode, "group"))
	{
		const std::string groupName = XMLNode_GetAttributeValue(groupNode, "name");

		// Load translations

		for (XMLNode* translationNode = XMLNode_GetFirstNode(groupNode, "translation"); translationNode; translationNode = XMLNode_GetNext(translationNode, "translation"))
		{
			const std::string translationName = XMLNode_GetAttributeValue(translationNode, "name");
			const char* value = XMLNode_GetAttributeValue(translationNode, "value");

			const std::string fullName = name + "." + groupName + "." + translationName;

			g_strings[fullName] = value;
		}
	}

	return true;
}

void Localization::UnloadSet(const std::string& name)
{
	const std::string setPrefix = name + ".";

	for (std::map<std::string, std::string>::iterator it = g_strings.begin(); it != g_strings.end(); ++it)
		if (!strncmp(it->first.c_str(), setPrefix.c_str(), setPrefix.length()))
			g_strings.erase(it);
}

void Localization::UnloadAllSets()
{
	g_strings.clear();
}

const std::string Localization::Get(const char* stringName, const Localization::Param* params, int numParams)
{
	std::string* localizedPtr = map_find<std::string, std::string>(g_strings, stringName);
	if (!localizedPtr)
		return "<STRING>";
	if (numParams == 0)
		return *localizedPtr;

	std::string localized = *localizedPtr;
	for (int i = 0; i < numParams; i++)
	{
		const Localization::Param& param = params[i];
		switch (param.type)
		{
		case Localization::Param::Type_Int:
			string_replace_all(localized, "[" + param.name + "]", string_from_int(param.intValue));
			break;
		case Localization::Param::Type_String:
			string_replace_all(localized, "[" + param.name + "]", param.stringValue);
			break;
		}
	}
	return localized;
}