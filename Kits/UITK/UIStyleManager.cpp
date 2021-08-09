//--------------------------------------------------------------------------------------
// File: UIStyleManager.cpp
//
// Authored by: ATG
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "pch.h"

#include "UIDebugConfig.h"
#include "UIStyleManager.h"
#include "UIStyleImpl.h"
#include "UIStyleRendererD3D.h"
#include "UIStyleRendererNull.h"
#include "UIMath.h"

NAMESPACE_ATG_UITK_BEGIN

INITIALIZE_CLASS_LOG_DEBUG(UIStyleManager);

/*public:*/

UIStyleManager::UIStyleManager(UIDataDefinitions& dataDefinitions) :
	m_styleIdCounter(1),
	m_dataDefinitions(dataDefinitions)
{
	RegisterInternalStyleFactories();
}

UIStylePtr UIStyleManager::GetById(const ID& id) const
{
	const auto& iter = m_stylesById.find(id);

	if (iter != m_stylesById.end())
	{
		return iter->second;
	}

	return UIStylePtr(nullptr);
}

ID UIStyleManager::LoadStyleFromData(const ID& context, UIDataPtr data, bool createAnonymousId)
{
	// what we will need to do here is use the appropriate factory
	// through the 'MakeStyleFromData()' method, store that style
	// into the map (unless there is a collision with style IDs) and
	// then return the ID
	auto newStyle = MakeStyleFromData(context, data, createAnonymousId);
	m_stylesById[newStyle->GetID()] = newStyle;
	return newStyle->GetID();
}

void UIStyleManager::InitializeStyleRenderer(UIStyleRendererPtr&& styleRenderer)
{
	m_styleRenderer = std::move(styleRenderer);
}

void UIStyleManager::ResetStyleRenderer()
{
    m_styleRenderer.reset();
}

/*private:*/

void UIStyleManager::RegisterInternalStyleFactories()
{
    RegisterStyleFactory<UINullStyleFactory>(UINullStyle::ClassId());
    RegisterStyleFactory<UIBasicStyleFactory>(UIBasicStyle::ClassId());
    RegisterStyleFactory<UITextStyleFactory>(UITextStyle::ClassId());
	RegisterStyleFactory<UISpriteStyleFactory>(UISpriteStyle::ClassId());
}

UIStylePtr UIStyleManager::MakeStyleFromData(const ID& context, UIDataPtr data, bool createAnonymousId)
{
	if (!data->IsObject() || !data->Exists(UITK_FIELD(classId)))
	{
		throw UIException(context, "Incorrect and/or malformed data file being loaded.");
	}

    std::string classId = data->Get<std::string>(UITK_FIELD(classId));
    
	auto rootStyleClassId = ID(classId);
    ID rootStyleId = data->GetIfExists<ID>(UITK_FIELD(id), ID::Default);

    if (createAnonymousId)
    {
        rootStyleId = ID::CreateUUID(rootStyleId.AsStr());
    }

	if (!rootStyleId)
	{
#if UI_ALLOW_ANONYMOUS_STYLES
		rootStyleId = GenerateStyleID(rootStyleClassId.AsStr());
#else
        throw UIException(context, "'id' field must be present in a non-inline style object");
#endif
	}

    /*
    This block of code implements the differences between style schema v1 and v2.
    In element schema v1, the style-type fields are siblings of the core fields (e.g. id, classId, etc.)
    In element schema v2, the style-type fields are in a child object named after the type of style (e.g. basicStyle, spriteStyle, etc.)
    For backwards compatibility, we support both formats.
    To keep the parsing logic consistent between v1 and v2, we promote the style-type fields to be siblings of the core fields as they are in v1.
     */
    classId[0] = char(std::tolower(classId[0])); // force initial word is camelCase
    if (data->Exists(classId))
    {
        data->ApplyPatch(data->GetObjectValue(classId));
    }
    else
    {
        for (auto& key : data->GetKeys())
        {
            if (ID(key) == rootStyleId)
            {
                UILOG_WARN("Key { %s } matches the classId { %s } but is not camelCased correctly and did not match.", key.c_str(), rootStyleId.AsCStr());
                data->ApplyPatch(data->GetObjectValue(key));
#if UI_STRICT
                throw std::exception("Field did not use proper camelCasing!");
#endif
                break;
            }
        }
    }

	return AllocateStyle(rootStyleClassId, rootStyleId, data);
}

UIStylePtr UIStyleManager::AllocateStyle(const ID& styleClassId, const ID& styleId, UIDataPtr data)
{
	auto iterator = m_styleFactories.find(styleClassId);

	if (iterator == m_styleFactories.end())
	{
		throw UIException(styleClassId, "No style class name registered to allocate.");
	}

	auto inheritsFromId = ID(data->GetIfExists<std::string>(UITK_FIELD(inheritsFromId), ""));

	return std::shared_ptr<UIStyle>(iterator->second->Create(*this, styleId, inheritsFromId, data));
}

ID UIStyleManager::GenerateStyleID(const std::string& styleClassId)
{
	std::stringstream ss;
	ss << styleClassId << m_styleIdCounter++;
	return ID(ss.str());
}

void UIStyleManager::FlattenAllStyles()
{
	for (auto& stylePair : m_stylesById)
	{
		stylePair.second->Flatten();
	}
}

NAMESPACE_ATG_UITK_END
