#include "pch.h"

// the std::filesystem functionality is not available
// on other WINAPI_FAMILY_* platforms like XBOX
#if (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#include <filesystem>
#endif

#include "UICore.h"
#include "UIKeywords.h"
#include "UILog.h"

using namespace ATG::UITK;

NAMESPACE_ATG_UITK_BEGIN

INITIALIZE_CLASS_LOG_ERROR(UIAssert);

const ID ID::Default = ID();
const UIDisplayString emptyItemDisplayString;

ENUM_LOOKUP_TABLE(HorizontalAnchor,
    ID_ENUM_PAIR(UITK_VALUE(left), HorizontalAnchor::Left),
    ID_ENUM_PAIR(UITK_VALUE(center), HorizontalAnchor::Center),
    ID_ENUM_PAIR(UITK_VALUE(right), HorizontalAnchor::Right)
)

ENUM_LOOKUP_TABLE(VerticalAnchor,
    ID_ENUM_PAIR(UITK_VALUE(top), VerticalAnchor::Top),
    ID_ENUM_PAIR(UITK_VALUE(middle), VerticalAnchor::Middle),
    ID_ENUM_PAIR(UITK_VALUE(bottom), VerticalAnchor::Bottom)
)

char Anchor::s_debugHorizontalAnchors[3]{ 'L', 'C', 'R' };
char Anchor::s_debugVerticalAnchors[3]{ 'T', 'M', 'B' };

/* static */ Anchor Anchor::FromIDs(const ID& horizontal, const ID& vertical, const Anchor& defaultAnchor)
{
	Anchor result = defaultAnchor;

	UIEnumLookup(horizontal, result.Horizontal);
	UIEnumLookup(vertical, result.Vertical);

	return result;
}

std::tuple<const char *, const char *> ATG::UITK::Anchor::GetIDs() const
{
	static const char* ids[] = { UITK_VALUE(left), UITK_VALUE(center), UITK_VALUE(right),
                                 UITK_VALUE(top), UITK_VALUE(middle), UITK_VALUE(bottom) };
	const int hOffset = static_cast<int>(this->Horizontal);
	const int vOffset = static_cast<int>(this->Vertical) + 3;

	return std::tuple<const char *, const char *>(ids[hOffset], ids[vOffset]);
}

std::string NewUUID()
{
    GUID id = {};
    char buf[64] = {};

    CoCreateGuid(&id);

    sprintf_s(buf, "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
        id.Data1, id.Data2, id.Data3,
        id.Data4[0], id.Data4[1], id.Data4[2], id.Data4[3],
        id.Data4[4], id.Data4[5], id.Data4[6], id.Data4[7]);

    return std::string(buf);
}

uint32_t FrameComputedValues::s_currentFrame = 1;

bool Util::FileExists(const std::string& path)
{
    // the std::filesystem functionality is not available
    // on other WINAPI_FAMILY_* platforms like XBOX
#if (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)

    return std::filesystem::exists(path);

#else

    WIN32_FIND_DATAA findData;
    auto hFile = FindFirstFileA(path.c_str(), &findData);
    auto fileExists = hFile != INVALID_HANDLE_VALUE;
    if (fileExists)
    {
        FindClose(hFile);
    }
    return fileExists;

#endif
}

NAMESPACE_ATG_UITK_END
