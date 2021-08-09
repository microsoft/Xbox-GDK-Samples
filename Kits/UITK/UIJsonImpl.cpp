#include "pch.h"
#include "UICore.h"
#include "UIKeywords.h"
#include "UISerializer.h"
#include "UIElement.h"
#include "UIManager.h"
#include "Json.h"

NAMESPACE_ATG_UITK_BEGIN

void to_json(json& j, _In_ const Anchor& anchor)
{
	j = anchor.GetIDs();
}

void from_json(const json& j, _Out_ Anchor& anchor)
{
	auto v = j.get<std::vector<std::string>>();
	assert(v.size() == 2);
	anchor = Anchor::FromIDs(ATG::UITK::ID(v[0]), ATG::UITK::ID(v[1]));
}

std::vector<std::string> GetKeysFromObject(const json& jsObj)
{
	std::vector<std::string> keys;

	for (auto& el : jsObj.items())
	{
		keys.emplace_back(el.key());
	}

	return keys;
}

NAMESPACE_ATG_UITK_END
