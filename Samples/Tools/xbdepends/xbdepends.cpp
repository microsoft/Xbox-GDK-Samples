//--------------------------------------------------------------------------------------
// File: xbdepends.cpp
//
// Microsoft Xbox Binary Dependencies Tool
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include <Windows.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <iterator>
#include <list>
#include <locale>
#include <memory>
#include <regex>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

namespace
{
    struct handle_closer { void operator()(HANDLE h) { if (h) CloseHandle(h); } };

    using ScopedHandle = std::unique_ptr<void, handle_closer>;

    inline HANDLE safe_handle(HANDLE h) { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }

    struct find_closer { void operator()(HANDLE h) { assert(h != INVALID_HANDLE_VALUE); if (h) FindClose(h); } };

    using ScopedFindHandle = std::unique_ptr<void, find_closer>;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

enum OPTIONS : uint32_t
{
    OPT_RECURSIVE = 1,
    OPT_NOLOGO,
    OPT_FILELIST,
    OPT_VERBOSE,
    OPT_RETAIL,
    OPT_LAYOUT,
    OPT_TARGET_XBOXONE,
    OPT_TARGET_SCARLETT,
    OPT_TARGET_PC,
    OPT_MAX
};

static_assert(OPT_MAX <= 32, "options is a unsigned int bitfield");

struct SConversion
{
    wchar_t szSrc[MAX_PATH];
};

struct SValue
{
    const wchar_t*  name;
    uint32_t        value;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

const SValue g_pOptions[] =
{
    { L"r",         OPT_RECURSIVE },
    { L"nologo",    OPT_NOLOGO },
    { L"flist",     OPT_FILELIST },
    { L"v",         OPT_VERBOSE },
    { L"retail",    OPT_RETAIL },
    { L"layout",    OPT_LAYOUT },
    { L"xboxone",   OPT_TARGET_XBOXONE },
    { L"scarlett",  OPT_TARGET_SCARLETT },
    { L"pc",        OPT_TARGET_PC },
    { nullptr,      0 }
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

namespace
{
#ifdef _PREFAST_
#pragma prefast(disable : 26018, "Only used with static internal arrays")
#endif

    uint32_t LookupByName(const wchar_t *name, const SValue *pArray)
    {
        while (pArray->name)
        {
            if (!_wcsicmp(name, pArray->name))
                return pArray->value;

            pArray++;
        }

        return 0;
    }

    void SearchForFiles(const wchar_t* path, std::list<SConversion>& files, bool recursive)
    {
        // Process files
        WIN32_FIND_DATAW findData = {};
        ScopedFindHandle hFile(safe_handle(FindFirstFileExW(path,
            FindExInfoBasic, &findData,
            FindExSearchNameMatch, nullptr,
            FIND_FIRST_EX_LARGE_FETCH)));
        if (hFile)
        {
            for (;;)
            {
                if (!(findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY)))
                {
                    wchar_t drive[_MAX_DRIVE] = {};
                    wchar_t dir[_MAX_DIR] = {};
                    _wsplitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);

                    SConversion conv = {};
                    _wmakepath_s(conv.szSrc, drive, dir, findData.cFileName, nullptr);
                    files.push_back(conv);
                }

                if (!FindNextFileW(hFile.get(), &findData))
                    break;
            }
        }

        // Process directories
        if (recursive)
        {
            wchar_t searchDir[MAX_PATH] = {};
            {
                wchar_t drive[_MAX_DRIVE] = {};
                wchar_t dir[_MAX_DIR] = {};
                _wsplitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
                _wmakepath_s(searchDir, drive, dir, L"*", nullptr);
            }

            hFile.reset(safe_handle(FindFirstFileExW(searchDir,
                FindExInfoBasic, &findData,
                FindExSearchLimitToDirectories, nullptr,
                FIND_FIRST_EX_LARGE_FETCH)));
            if (!hFile)
                return;

            for (;;)
            {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (findData.cFileName[0] != L'.')
                    {
                        wchar_t subdir[MAX_PATH] = {};

                        {
                            wchar_t drive[_MAX_DRIVE] = {};
                            wchar_t dir[_MAX_DIR] = {};
                            wchar_t fname[_MAX_FNAME] = {};
                            wchar_t ext[_MAX_FNAME] = {};
                            _wsplitpath_s(path, drive, dir, fname, ext);
                            wcscat_s(dir, findData.cFileName);
                            _wmakepath_s(subdir, drive, dir, fname, ext);
                        }

                        SearchForFiles(subdir, files, recursive);
                    }
                }

                if (!FindNextFileW(hFile.get(), &findData))
                    break;
            }
        }
    }

    void ProcessFileList(std::wifstream& inFile, std::list<SConversion>& files)
    {
        std::list<SConversion> flist;
        std::set<std::wstring> excludes;
        wchar_t fname[1024] = {};
        for (;;)
        {
            inFile >> fname;
            if (!inFile)
                break;

            if (*fname == L'#')
            {
                // Comment
            }
            else if (*fname == L'-')
            {
                if (flist.empty())
                {
                    wprintf(L"WARNING: Ignoring the line '%ls' in -flist\n", fname);
                }
                else
                {
                    if (wcspbrk(fname, L"?*") != nullptr)
                    {
                        std::list<SConversion> removeFiles;
                        SearchForFiles(&fname[1], removeFiles, false);

                        for (auto it : removeFiles)
                        {
                            _wcslwr_s(it.szSrc);
                            excludes.insert(it.szSrc);
                        }
                    }
                    else
                    {
                        std::wstring name = (fname + 1);
                        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
                        excludes.insert(name);
                    }
                }
            }
            else if (wcspbrk(fname, L"?*") != nullptr)
            {
                SearchForFiles(fname, flist, false);
            }
            else
            {
                SConversion conv = {};
                wcscpy_s(conv.szSrc, MAX_PATH, fname);
                flist.push_back(conv);
            }

            inFile.ignore(1000, '\n');
        }

        inFile.close();

        if (!excludes.empty())
        {
            // Remove any excluded files
            for (auto it = flist.begin(); it != flist.end();)
            {
                std::wstring name = it->szSrc;
                std::transform(name.begin(), name.end(), name.begin(), towlower);
                auto item = it;
                ++it;
                if (excludes.find(name) != excludes.end())
                {
                    flist.erase(item);
                }
            }
        }

        if (flist.empty())
        {
            wprintf(L"WARNING: No file names found in -flist\n");
        }
        else
        {
            files.splice(files.end(), flist);
        }
    }

    void PrintLogo()
    {
        wprintf(L"Microsoft (R) Xbox Binary Dependencies Tool\n");
        wprintf(L"Copyright (C) Microsoft Corp.\n");
#ifdef _DEBUG
        wprintf(L"*** Debug build ***\n");
#endif
        wprintf(L"\n");
    }

    void PrintUsage(const wchar_t* toolname)
    {
        wchar_t ext[_MAX_EXT] = {};
        wchar_t fname[_MAX_FNAME] = {};
        _wsplitpath_s(toolname, nullptr, 0, nullptr, 0, fname, _MAX_FNAME, ext, _MAX_EXT);

        PrintLogo();

        wprintf(L"Usage: %ls%ls <options> <files>\n", fname, ext);

        static const wchar_t* const s_usage =
            L"\n"
            L"   -xboxone | -scarlett | -pc\n"
            L"\n"
            L"   -retail             validate for retail deployment\n"
            L"   -layout             assume all processed files are in a deployment layout\n"
            L"   -r                  wildcard filename search is recursive\n"
            L"   -v                  verbose output\n"
            L"   -nologo             suppress copyright message\n"
            L"   -flist <filename>   use text file with a list of input files (one per line)\n"
            L"\n";

        wprintf(L"%ls", s_usage);
    }

    const wchar_t* GetErrorDesc(HRESULT hr)
    {
        static wchar_t desc[1024] = {};

        LPWSTR errorText = nullptr;

        DWORD result = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr,
            static_cast<DWORD>(hr),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&errorText), 0, nullptr);

        *desc = 0;

        if (result > 0 && errorText)
        {
            swprintf_s(desc, L": %ls", errorText);

            size_t len = wcslen(desc);
            if (len >= 2)
            {
                desc[len - 2] = 0;
                desc[len - 1] = 0;
            }

            if (errorText)
                LocalFree(errorText);

            for (wchar_t* ptr = desc; *ptr != 0; ++ptr)
            {
                if (*ptr == L'\r' || *ptr == L'\n')
                {
                    *ptr = L' ';
                }
            }
        }

        return desc;
    }

    enum class XBMODULE_CATEGORY : uint32_t
    {
        Unknown = 0,
        OS,
        GameOS,
        D3D,
        CRT,
        GDK,
        DXSDK,
        Vendor,
        User
    };

    enum class XBTARGET : uint32_t
    {
        Unknown = 0,
        XboxOne,
        Scarlett,
        PC
    };

    enum class VCMinimumVersion : uint32_t
    {
        Unknown = 0,
        VS2017,
        VS2017_15_7,
        VS2019,
        VS2019_16_2,
        VS2019_16_8,
        Ignore,
    };

    const wchar_t* GetCategoryString(XBMODULE_CATEGORY cat)
    {
        switch (cat)
        {
        case XBMODULE_CATEGORY::OS: return L"OS";
        case XBMODULE_CATEGORY::GameOS: return L"GameOS";
        case XBMODULE_CATEGORY::D3D : return L"D3D";
        case XBMODULE_CATEGORY::CRT: return L"CRT";
        case XBMODULE_CATEGORY::GDK: return L"GDK";
        case XBMODULE_CATEGORY::DXSDK: return L"DXSDK";
        case XBMODULE_CATEGORY::Vendor: return L"IHV";
        case XBMODULE_CATEGORY::User: return L"User";
        case XBMODULE_CATEGORY::Unknown:
        default:
            return L"???";
        }
    }

    struct XBModuleInfo
    {
        XBMODULE_CATEGORY                   category;
        const char*                         fileName;
        const IMAGE_IMPORT_DESCRIPTOR*      importDesc;
        const IMAGE_DELAYLOAD_DESCRIPTOR*   delayImportDesc;
    };

    // https://stackoverflow.com/questions/15960437/how-to-read-import-directory-table-in-c
    ptrdiff_t Rva2Offset(ptrdiff_t rva, const IMAGE_SECTION_HEADER* psh, const IMAGE_NT_HEADERS64* ntHeaders)
    {
        assert(psh != nullptr);
        assert(ntHeaders != nullptr);
        if (!rva)
            return 0;

        const IMAGE_SECTION_HEADER* pSeh = psh;

        for (size_t j = 0; j < ntHeaders->FileHeader.NumberOfSections; ++j)
        {
            if (rva >= pSeh->VirtualAddress
                && rva < ptrdiff_t(pSeh->VirtualAddress) + ptrdiff_t(pSeh->Misc.VirtualSize))
                break;

            pSeh++;
        }

        return (rva - pSeh->VirtualAddress + pSeh->PointerToRawData);
    }

    // This regular expression matches all API set and extension DLLs in the OS.
    const char* c_apiSetRegEx = "(api|ext)-ms-win-([A-Za-z0-9]*-)*l[0-9]+-[0-9]+-[0-9]+\\.dll";

    // This regular expression matches the CRT DLLs in the OS.
    const char* c_osCRTRegEx = "((api-ms-win-crt-([A-Za-z0-9]*-)*l[0-9]+-[0-9]+-[0-9]+)|msvcrt|ucrtbase)\\.dll";

    // Matches all Microsoft Foundation Class (MFC) Runtime DLLs.
    const char* c_mfcRegExDebug = "mfcm?[0-9]+u?d\\.dll";
    const char* c_mfcRegExRelease = "((mfcm?[0-9]+u?)|(mfc[0-9]+[A-Za-z]{3}))\\.dll";

    // Matches all Visual C++ Runtime (CRT) Debug DLLs and the debug version of the Unified CRT from the Windows 10 SDK.
    const char* c_crtRegExDebug = "(((concrt|msvcp|vccorlib|vcruntime|vcamp|vcomp)[0-9]+(_[0-9])?d(_[A-Za-z_]*)?)|(libomp[0-9]+d\\.x86_64)|ucrtbased|vcruntime140_threadsd)\\.dll";
    const wchar_t* c_crtRegExDebugW = L"(((concrt|msvcp|vccorlib|vcruntime|vcamp|vcomp)[0-9]+(_[0-9])?d(_[A-Za-z_]*)?)|(libomp[0-9]+d\\.x86_64)|ucrtbased|vcruntime140_threadsd)\\.dll";

    // Matches all Visual C++ Runtime (CRT) Release DLLs (ucrtbase.dll is part of the OS).
    const char* c_crtRegExRelease = "(((concrt|msvcp|msvcr|vccorlib|vcruntime|vcamp|vcomp)[0-9]+(_[0-9])?(_[A-Za-z_]+)?)|(libomp[0-9]+\\.x86_64))\\.dll";
    const wchar_t* c_crtRegExReleaseW = L"(((concrt|msvcp|msvcr|vccorlib|vcruntime|vcamp|vcomp)[0-9]+(_[0-9])?(_[A-Za-z_]+)?)|(libomp[0-9]+\\.x86_64))\\.dll";

    // Matches all the Xbox Transport DLLs.
    const char* c_xtfRegEx = "xtf[A-Za-z]+\\.dll";

    // Matches all new-style Windows components (Windows.Bar.Foo.dll).
    const char* c_winRegEx = "Windows\\.[A-Za-z\\.]+\\.dll";

#define VALIDATE_IS_SORTED(arr) \
        { \
            assert(std::size(arr) > 0); \
            const char* lastvalue = arr[0]; \
            for (size_t j = 1; j < std::size(arr); ++j) \
            { \
                assert(_stricmp(lastvalue, arr[j]) < 0); \
                lastvalue = arr[j]; \
            } \
        }

#include "KnownDLLs.h"

    bool CategorizeModules(
        _Inout_updates_all_(totalModules) XBModuleInfo* moduleList,
        size_t totalModules,
        XBTARGET& target,
        const wchar_t* basePath,
        const uint32_t options)
    {
        assert(moduleList != nullptr);

        using namespace KnownDLLs;

#ifdef _DEBUG
        // Ensure data tables are is in ascending order

        VALIDATE_IS_SORTED(c_CoreOS);
        VALIDATE_IS_SORTED(c_Win32);
        VALIDATE_IS_SORTED(c_GameOSOnly);
        VALIDATE_IS_SORTED(c_SystemOS);
        VALIDATE_IS_SORTED(c_PCOnly);
        VALIDATE_IS_SORTED(c_PCVendor);
        VALIDATE_IS_SORTED(c_LegacyERA);
        VALIDATE_IS_SORTED(c_LegacyDXSDK);
        VALIDATE_IS_SORTED(c_LegacyDXSDKDebug);
        VALIDATE_IS_SORTED(c_GDK);
        VALIDATE_IS_SORTED(c_DevOnlyGDK);
        VALIDATE_IS_SORTED(c_Direct3D_Legacy);
        VALIDATE_IS_SORTED(c_Direct3D_Stock);
        VALIDATE_IS_SORTED(c_Direct3D_XboxOne);
        VALIDATE_IS_SORTED(c_Direct3D_Scarlett);

#endif // _DEBUG

        std::regex apiset_regex(c_apiSetRegEx,
            std::regex_constants::ECMAScript | std::regex_constants::icase);

        std::regex mfc_dbg_regex(c_mfcRegExDebug,
            std::regex_constants::ECMAScript | std::regex_constants::icase);

        std::regex mfc_regex(c_mfcRegExRelease,
            std::regex_constants::ECMAScript | std::regex_constants::icase);

        std::regex crt_dbg_regex(c_crtRegExDebug,
            std::regex_constants::ECMAScript | std::regex_constants::icase);

        std::regex crt_regex(c_crtRegExRelease,
            std::regex_constants::ECMAScript | std::regex_constants::icase);

        std::regex xtf_regex(c_xtfRegEx,
            std::regex_constants::ECMAScript | std::regex_constants::icase);

        std::regex win_regex(c_winRegEx,
            std::regex_constants::ECMAScript | std::regex_constants::icase);

        auto pred = [](const char* a, const char* b) -> bool
        {
            return _stricmp(a, b) < 0;
        };

        if (target == XBTARGET::Unknown)
        {
            // Look for use of Direct3D to determine likely target.
            for (size_t j = 0; j < totalModules; ++j)
            {
                auto libname = moduleList[j].fileName;

                if (std::binary_search(c_Direct3D_Legacy, c_Direct3D_Legacy + std::size(c_Direct3D_Legacy), libname, pred))
                {
                    target = XBTARGET::PC;
                    wprintf(L"INFO: Use of legacy Direct3D implies PC target\n");
                    break;
                }
                else if (std::binary_search(c_Direct3D_Stock, c_Direct3D_Stock + std::size(c_Direct3D_Stock), libname, pred))
                {
                    target = XBTARGET::PC;
                    wprintf(L"INFO: Use of stock Direct3D implies PC target\n");
                    break;
                }
                else if (std::binary_search(c_Direct3D_XboxOne, c_Direct3D_XboxOne + std::size(c_Direct3D_XboxOne), libname, pred))
                {
                    target = XBTARGET::XboxOne;
                    wprintf(L"INFO: Use of Direct3D 12.X implies Xbox One target\n");
                    break;
                }
                else if (std::binary_search(c_Direct3D_Scarlett, c_Direct3D_Scarlett + std::size(c_Direct3D_Scarlett), libname, pred))
                {
                    target = XBTARGET::Scarlett;
                    wprintf(L"INFO: Use of Direct3D 12.X_S implies Scarlett target\n");
                    break;
                }
            }
        }

        bool legacyd3d = false;
        bool softstock = false;
        bool softxboxone = false;
        bool softscarlett = false;
        bool hardstock = false;
        bool hardxboxone = false;
        bool hardscarlett = false;

        std::vector<const char*> devOnly;
        std::vector<const char*> gameOSwarn;
        std::vector<const char*> win32warn;
        std::vector<const char*> pcwarn;
        std::vector<const char*> legacyDXWarn;
        std::vector<const char*> legacyERAWarn;
        for (size_t j = 0; j < totalModules; ++j)
        {
            auto libname = moduleList[j].fileName;

            if (std::regex_search(libname, apiset_regex))
            {
                moduleList[j].category = XBMODULE_CATEGORY::OS;
            }
            else if (std::regex_search(libname, crt_dbg_regex))
            {
                devOnly.push_back(libname);
                moduleList[j].category = XBMODULE_CATEGORY::CRT;
            }
            else if (std::regex_search(libname, crt_regex))
            {
                moduleList[j].category = XBMODULE_CATEGORY::CRT;
            }
            else if (std::regex_search(libname, mfc_dbg_regex))
            {
                devOnly.push_back(libname);
                if (target == XBTARGET::XboxOne || target == XBTARGET::Scarlett)
                {
                    pcwarn.push_back(libname);
                }
                moduleList[j].category = XBMODULE_CATEGORY::CRT;
            }
            else if (std::regex_search(libname, mfc_regex))
            {
                if (target == XBTARGET::XboxOne || target == XBTARGET::Scarlett)
                {
                    pcwarn.push_back(libname);
                }
                moduleList[j].category = XBMODULE_CATEGORY::CRT;
            }
            else if (std::regex_search(libname, xtf_regex))
            {
                devOnly.push_back(libname);
                moduleList[j].category = XBMODULE_CATEGORY::GDK;
            }
            else if (std::regex_search(libname, win_regex))
            {
                moduleList[j].category = XBMODULE_CATEGORY::OS;
            }
            else if (std::binary_search(c_CoreOS, c_CoreOS + std::size(c_CoreOS), libname, pred))
            {
                moduleList[j].category = XBMODULE_CATEGORY::OS;
            }
            else if (std::binary_search(c_Win32, c_Win32 + std::size(c_Win32), libname, pred))
            {
                if (target == XBTARGET::XboxOne || target == XBTARGET::Scarlett)
                {
                    win32warn.push_back(libname);
                }
                moduleList[j].category = XBMODULE_CATEGORY::OS;
            }
            else if (std::binary_search(c_SystemOS, c_SystemOS + std::size(c_SystemOS), libname, pred))
            {
                if (target == XBTARGET::XboxOne || target == XBTARGET::Scarlett)
                {
                    pcwarn.push_back(libname);
                }
                moduleList[j].category = XBMODULE_CATEGORY::OS;
            }
            else if (std::binary_search(c_GameOSOnly, c_GameOSOnly + std::size(c_GameOSOnly), libname, pred))
            {
                if (target == XBTARGET::PC)
                {
                    gameOSwarn.push_back(libname);
                }
                moduleList[j].category = XBMODULE_CATEGORY::GameOS;
            }
            else if (std::binary_search(c_PCOnly, c_PCOnly + std::size(c_PCOnly), libname, pred))
            {
                if (target == XBTARGET::XboxOne || target == XBTARGET::Scarlett)
                {
                    pcwarn.push_back(libname);
                }
                moduleList[j].category = XBMODULE_CATEGORY::OS;
            }
            else if (std::binary_search(c_PCVendor, c_PCVendor + std::size(c_PCVendor), libname, pred))
            {
                if (target == XBTARGET::XboxOne || target == XBTARGET::Scarlett)
                {
                    pcwarn.push_back(libname);
                }
                moduleList[j].category = XBMODULE_CATEGORY::Vendor;
            }
            else if (std::binary_search(c_LegacyDXSDKDebug, c_LegacyDXSDKDebug + std::size(c_LegacyDXSDKDebug), libname, pred))
            {
                legacyDXWarn.push_back(libname);
                devOnly.push_back(libname);
                moduleList[j].category = XBMODULE_CATEGORY::DXSDK;
            }
            else if (std::binary_search(c_LegacyDXSDK, c_LegacyDXSDK + std::size(c_LegacyDXSDK), libname, pred))
            {
                legacyDXWarn.push_back(libname);
                moduleList[j].category = XBMODULE_CATEGORY::DXSDK;
            }
            else if (std::binary_search(c_GDK, c_GDK + std::size(c_GDK), libname, pred))
            {
                moduleList[j].category = XBMODULE_CATEGORY::GDK;
            }
            else if (std::binary_search(c_DevOnlyGDK, c_DevOnlyGDK + std::size(c_DevOnlyGDK), libname, pred))
            {
                devOnly.push_back(libname);
                moduleList[j].category = XBMODULE_CATEGORY::GDK;
            }
            else if (std::binary_search(c_Direct3D_Legacy, c_Direct3D_Legacy + std::size(c_Direct3D_Legacy), libname, pred))
            {
                legacyd3d = true;
                moduleList[j].category = XBMODULE_CATEGORY::D3D;
            }
            else if (std::binary_search(c_Direct3D_Stock, c_Direct3D_Stock + std::size(c_Direct3D_Stock), libname, pred))
            {
                if (moduleList[j].delayImportDesc)
                    softstock = true;
                else
                    hardstock = true;

                moduleList[j].category = XBMODULE_CATEGORY::D3D;
            }
            else if (std::binary_search(c_Direct3D_XboxOne, c_Direct3D_XboxOne + std::size(c_Direct3D_XboxOne), libname, pred))
            {
                if (moduleList[j].delayImportDesc)
                    softxboxone = true;
                else
                    hardxboxone = true;

                moduleList[j].category = XBMODULE_CATEGORY::D3D;
            }
            else if (std::binary_search(c_Direct3D_Scarlett, c_Direct3D_Scarlett + std::size(c_Direct3D_Scarlett), libname, pred))
            {
                if (moduleList[j].delayImportDesc)
                    softscarlett = true;
                else
                    hardscarlett = true;

                moduleList[j].category = XBMODULE_CATEGORY::D3D;
            }
            else if (std::binary_search(c_LegacyERA, c_LegacyERA + std::size(c_LegacyERA), libname, pred))
            {
                legacyERAWarn.push_back(libname);
                moduleList[j].category = XBMODULE_CATEGORY::OS;
            }
            else
            {
                // Check for module file in same directory as our binary.
                wchar_t fullPath[MAX_PATH] = {};
                wcscpy_s(fullPath, basePath);

                wchar_t wLibName[MAX_PATH] = {};
                std::ignore = MultiByteToWideChar(CP_UTF8, 0, libname, -1, wLibName, MAX_PATH);

                wcscat_s(fullPath, wLibName);

                // If we find it, we assume it's a 'user DLL'
                std::ifstream inFile(fullPath, std::ios::in | std::ios::binary);
                if (inFile.is_open())
                {
                    moduleList[j].category = XBMODULE_CATEGORY::User;
                }
            }
        }

        if (!win32warn.empty())
        {
            wprintf(L"INFO: Using Win32 legacy DLLs for Game OS; recommend using xgameplatform.lib only\n");
            for (auto it = win32warn.cbegin(); it != win32warn.cend(); ++it)
            {
                wprintf(L"\t%hs\n", *it);
            }
        }

        bool ret = true;

        if (!legacyERAWarn.empty())
        {
            wprintf(L"ERROR: Found use of legacy ERA DLL that is not supported by Microsoft GDKX\n");
            for (auto it = legacyERAWarn.cbegin(); it != legacyERAWarn.cend(); ++it)
            {
                wprintf(L"\t%hs\n", *it);
            }
            ret = false;
        }

        if (!devOnly.empty() && (options & (1 << OPT_RETAIL)))
        {
            wprintf(L"ERROR: Using development only DLLs not for use in retail:\n");
            for (auto it = devOnly.cbegin(); it != devOnly.cend(); ++it)
            {
                wprintf(L"\t%hs\n", *it);
            }
            ret = false;
        }

        if (!gameOSwarn.empty())
        {
            wprintf(L"ERROR: Game OS only DLL referenced for PC\n");
            for (auto it = gameOSwarn.cbegin(); it != gameOSwarn.cend(); ++it)
            {
                wprintf(L"\t%hs\n", *it);
            }
            ret = false;
        }

        if (!pcwarn.empty())
        {
            wprintf(L"ERROR: Windows only DLL referred to for XboxOne/Scarlett\n");
            for (auto it = pcwarn.cbegin(); it != pcwarn.cend(); ++it)
            {
                wprintf(L"\t%hs\n", *it);
            }
            ret = false;
        }

        switch (target)
        {
        case XBTARGET::PC:
            if (!legacyDXWarn.empty())
            {
                wprintf(L"WARNING: Legacy DirectX SDK components found. Remove or use the DirectX Framework appx to deploy.\n");
                for (auto it = legacyDXWarn.cbegin(); it != legacyDXWarn.cend(); ++it)
                {
                    wprintf(L"\t%hs\n", *it);
                }
            }
            if (hardscarlett || hardxboxone || softscarlett || softxboxone)
            {
                wprintf(L"ERROR: Using Direct3D.X Runtimes on PC is not supported\n");
                ret = false;
            }
            break;

        case XBTARGET::XboxOne:
            if (legacyd3d)
            {
                wprintf(L"ERROR: Legacy Direct3D components (i.e. D3D8/D3D9/D3D10) are not supported for Xbox One\n");
            }
            if (!legacyDXWarn.empty())
            {
                wprintf(L"ERROR: Legacy DirectX SDK components are not supported for Xbox One");
                for (auto it = legacyDXWarn.cbegin(); it != legacyDXWarn.cend(); ++it)
                {
                    wprintf(L"\t%hs\n", *it);
                }
                ret = false;
            }
            if (hardstock || softstock)
            {
                wprintf(L"ERROR: Using Direct3D Stock Runtime on XboxOne is not supported\n");
                ret = false;
            }
            if (hardscarlett)
            {
                wprintf(L"ERROR: Using Direct3D.X for Scarlett on XboxOne is not supported\n");
                ret = false;
            }
            break;

        case XBTARGET::Scarlett:
            if (legacyd3d)
            {
                wprintf(L"ERROR: Legacy Direct3D components (i.e. D3D8/D3D9/D3D10) are not supported for Scarlett\n");
            }
            if (!legacyDXWarn.empty())
            {
                wprintf(L"ERROR: Legacy DirectX SDK components are not supported for Scarlett");
                for (auto it = legacyDXWarn.cbegin(); it != legacyDXWarn.cend(); ++it)
                {
                    wprintf(L"\t%hs\n", *it);
                }
                ret = false;
            }
            if (hardstock || softstock)
            {
                wprintf(L"ERROR: Using Direct3D Stock Runtime on Scarlett is not supported\n");
                ret = false;
            }
            if (hardxboxone)
            {
                wprintf(L"ERROR: Using Direct3D.X for XboxOne on Scarlett is not supported\n");
                ret = false;
            }
            break;

        default:
            if (!legacyDXWarn.empty())
            {
                wprintf(L"WARNING: Legacy DirectX SDK components found. Remove or use the DirectX Framework appx to deploy.\n");
                for (auto it = legacyDXWarn.cbegin(); it != legacyDXWarn.cend(); ++it)
                {
                    wprintf(L"\t%hs\n", *it);
                }
            }
            break;
        }

        if ((int)hardstock + (int)hardscarlett + (int)hardxboxone > 1)
        {
            wprintf(L"ERROR: Found a mix of Direct3D runtimes in the same EXE\n");
            ret = false;
        }

        return ret;
    }

    VCMinimumVersion VisualCRuntimeChecks(
        _In_z_ const wchar_t* baseName,
        _In_reads_(totalModules) const XBModuleInfo* moduleList,
        size_t totalModules,
        const XBTARGET target)
    {
        std::basic_regex<wchar_t> crt_dbg_regex(c_crtRegExDebugW,
            std::regex_constants::ECMAScript | std::regex_constants::icase);

        std::basic_regex<wchar_t> crt_regex(c_crtRegExReleaseW,
            std::regex_constants::ECMAScript | std::regex_constants::icase);

        bool isdebug = std::regex_search(baseName, crt_dbg_regex);
        if (std::regex_search(baseName, crt_regex) || isdebug)
        {
            // Only run this check on the VC++ CRT DLL themselves

            if (target == XBTARGET::XboxOne || target == XBTARGET::Scarlett)
            {
                for (size_t j = 0; j < totalModules; ++j)
                {
                    if (_stricmp(moduleList[j].fileName, "kernel32.dll") == 0)
                    {
                        if (isdebug)
                        {
                            wprintf(L"WARNING: For Xbox One & Scarlett, use the 'VC/Redist/MSVC/<toolset>/onecore/debug_nonredist/x64' version of the CRT instead of this version.\n");
                        }
                        else
                        {
                            wprintf(L"WARNING: For Xbox One & Scarlett, use the 'VC/Redist/MSVC/<toolset>/onecore/x64' version of the CRT instead of this version.\n");
                        }
                        break;
                    }
                }
            }

            return VCMinimumVersion::Ignore;
        }
        else
        {
            // Do not run this check on the VC++ CRT DLL themselves
            bool cppcxx = false;
            VCMinimumVersion ver = VCMinimumVersion::Unknown;

            for (size_t j = 0; j < totalModules; ++j)
            {
                if (moduleList[j].category != XBMODULE_CATEGORY::CRT)
                    continue;

                auto libname = moduleList[j].fileName;

                if ((_stricmp(libname, "vcruntime140.dll") == 0)
                    || (_stricmp(libname, "msvcp140.dll") == 0))
                {
                    // Our minimum supported version is VS 2017
                    ver = VCMinimumVersion::VS2017;
                }

                if (_stricmp(libname, "vccorlib140.dll") == 0)
                {
                    cppcxx = true;
                    ver = VCMinimumVersion::VS2017;
                }

                if (_stricmp(libname, "msvcp140_1.dll") == 0)
                {
                    ver = VCMinimumVersion::VS2017;
                }

                if (_stricmp(libname, "msvcp140_2.dll") == 0)
                {
                    ver = VCMinimumVersion::VS2017_15_7;
                }

                if (_stricmp(libname, "vcruntime140_1.dll") == 0)
                {
                    ver = VCMinimumVersion::VS2019;
                }

                if (_stricmp(libname, "msvcp140_codecvt_ids") == 0)
                {
                    ver = VCMinimumVersion::VS2019_16_2;
                }

                if (_stricmp(libname, "msvcp140_atomic_wait.dll") == 0)
                {
                    ver = VCMinimumVersion::VS2019_16_8;
                }
            }

            if (cppcxx)
            {
                wprintf(L"INFO: Uses the Windows Runtime C++/CX extensions\n");
            }

            return ver;
        }
    }

    bool CheckVisualCRTFiles(const std::set<std::string>& foundDLLs, VCMinimumVersion minVer)
    {
        if (minVer == VCMinimumVersion::Ignore)
            return true;

        if (minVer >= VCMinimumVersion::VS2019_16_8)
        {
            if (foundDLLs.find("msvcp140_atomic_wait.dll") == foundDLLs.end())
                return false;
        }

        if (minVer >= VCMinimumVersion::VS2019_16_2)
        {
            if (foundDLLs.find("msvcp140_codecvt_ids.dll") == foundDLLs.end())
                return false;
        }

        if (minVer >= VCMinimumVersion::VS2019)
        {
            if (foundDLLs.find("vcruntime140_1.dll") == foundDLLs.end())
                return false;
        }

        if (minVer >= VCMinimumVersion::VS2017_15_7)
        {
            if (foundDLLs.find("msvcp140_2.dll") == foundDLLs.end())
                return false;
        }

        if (minVer >= VCMinimumVersion::VS2017)
        {
            if (foundDLLs.find("msvcp140_1.dll") == foundDLLs.end())
                return false;
        }

        // VS 2015 CRT
        if (foundDLLs.find("vcruntime140.dll") == foundDLLs.end())
            return false;

        if (foundDLLs.find("msvcp140.dll") == foundDLLs.end())
            return false;

        return true;
    }

#include "GameOSAPIs.h"

    const char* c_additionalGameOSAPIs[] =
    {
        "CertOpenSystemStoreW",         // Missing from xgameplatform.lib
        "DStorageGetFactory",           // dstorage_x/xs.lib
        "MFResetDXGIDeviceManagerX",    // GDK mfplat.lib
    };
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------
// Entry-point
//--------------------------------------------------------------------------------------
#ifdef _PREFAST_
#pragma prefast(disable : 28198, "Command-line tool, frees all memory on exit")
#endif

int __cdecl wmain(_In_ int argc, _In_z_count_(argc) wchar_t* argv[])
{
    XBTARGET target = XBTARGET::Unknown;

    // Set locale for output since GetErrorDesc can get localized strings.
    std::locale::global(std::locale(""));

    // Process command line
    uint32_t options = 0;
    std::list<SConversion> conversion;

    for (int iArg = 1; iArg < argc; iArg++)
    {
        PWSTR pArg = argv[iArg];

        if (('-' == pArg[0]) || ('/' == pArg[0]))
        {
            pArg++;
            PWSTR pValue;

            for (pValue = pArg; *pValue && (':' != *pValue); pValue++);

            if (*pValue)
                *pValue++ = 0;

            uint32_t dwOption = LookupByName(pArg, g_pOptions);

            if (!dwOption || (options & (1 << dwOption)))
            {
                PrintUsage(argv[0]);
                return 1;
            }

            options |= 1 << dwOption;

            // Handle options with additional value parameter
            switch (dwOption)
            {
            case OPT_FILELIST:
                if (!*pValue)
                {
                    if ((iArg + 1 >= argc))
                    {
                        PrintUsage(argv[0]);
                        return 1;
                    }

                    iArg++;
                    pValue = argv[iArg];
                }
                break;
            }

            switch (dwOption)
            {
            case OPT_TARGET_XBOXONE:
                if (options & ((1 << OPT_TARGET_SCARLETT) | (1 << OPT_TARGET_PC)))
                {
                    wprintf(L"Can only use one of -xboxone, -scarlett, -pc\n");
                    return 1;
                }
                target = XBTARGET::XboxOne;
                break;

            case OPT_TARGET_SCARLETT:
                if (options & ((1 << OPT_TARGET_XBOXONE) | (1 << OPT_TARGET_PC)))
                {
                    wprintf(L"Can only use one of -xboxone, -scarlett, -pc\n");
                    return 1;
                }
                target = XBTARGET::Scarlett;
                break;

            case OPT_TARGET_PC:
                if (options & ((1 << OPT_TARGET_XBOXONE) | (1 << OPT_TARGET_SCARLETT)))
                {
                    wprintf(L"Can only use one of -xboxone, -scarlett, -pc\n");
                    return 1;
                }
                target = XBTARGET::PC;
                break;

            case OPT_FILELIST:
            {
                std::wifstream inFile(pValue);
                if (!inFile)
                {
                    wprintf(L"Error opening -flist file %ls\n", pValue);
                    return 1;
                }

                inFile.imbue(std::locale::classic());

                ProcessFileList(inFile, conversion);
            }
            break;
            }
        }
        else if (wcspbrk(pArg, L"?*") != nullptr)
        {
            size_t count = conversion.size();
            SearchForFiles(pArg, conversion, (options & (1 << OPT_RECURSIVE)) != 0);
            if (conversion.size() <= count)
            {
                wprintf(L"No matching files found for %ls\n", pArg);
                return 1;
            }
        }
        else if (GetFileAttributesW(pArg) & FILE_ATTRIBUTE_DIRECTORY)
        {
            wchar_t exepath[MAX_PATH] = {};
            wcscpy_s(exepath, pArg);
            wcscat_s(exepath, L"\\*.exe");

            size_t count = conversion.size();
            SearchForFiles(exepath, conversion, (options & (1 << OPT_RECURSIVE)) != 0);

            wchar_t dllpath[MAX_PATH] = {};
            wcscpy_s(dllpath, pArg);
            wcscat_s(dllpath, L"\\*.dll");

            SearchForFiles(dllpath, conversion, (options& (1 << OPT_RECURSIVE)) != 0);

            if (conversion.size() <= count)
            {
                wprintf(L"No matching files found for %ls\\*.exe;*.dll\n", pArg);
                return 1;
            }
        }
        else
        {
            SConversion conv = {};
            wcscpy_s(conv.szSrc, MAX_PATH, pArg);

            conversion.push_back(conv);
        }
    }

    if (conversion.empty())
    {
        wprintf(L"ERROR: Need at least 1 file.\n\n");
        PrintUsage(argv[0]);
        return 0;
    }

    if (~options & (1 << OPT_NOLOGO))
        PrintLogo();

    int retVal = 0;

    VCMinimumVersion overallMinVer = VCMinimumVersion::Unknown;
    std::set<std::string> foundDLLs;
    std::set<std::string> missingDLLs;
    std::set<std::string> missingDelayLoadDLLs;

    for (auto pConv = conversion.begin(); pConv != conversion.end(); ++pConv)
    {
        if (pConv != conversion.begin())
            wprintf(L"\n");

        wchar_t basePath[MAX_PATH] = {};
        wchar_t baseName[MAX_PATH] = {};
        {
            wchar_t drive[_MAX_DRIVE] = {};
            wchar_t dir[_MAX_DIR] = {};
            wchar_t fname[_MAX_FNAME] = {};
            wchar_t ext[_MAX_FNAME] = {};
            _wsplitpath_s(pConv->szSrc, drive, dir, fname, ext);
            _wmakepath_s(basePath, drive, dir, nullptr, nullptr);
            _wmakepath_s(baseName, nullptr, nullptr, fname, ext);
        }

        wprintf(L"reading '%ls'", baseName);
        fflush(stdout);

        ScopedHandle hFile(safe_handle(CreateFile2(pConv->szSrc, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr)));
        if (!hFile)
        {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            wprintf(L" FAILED (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
            retVal = 1;
            continue;
        }

        FILE_STANDARD_INFO fileInfo = {};
        if (!GetFileInformationByHandleEx(hFile.get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
        {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            wprintf(L" FAILED (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
            retVal = 1;
            continue;
        }

        if (fileInfo.EndOfFile.QuadPart < 256)
        {
            // A valid Win32 exe/dll should at least be large enough to hold a PE header
            // http://www.phreedom.org/research/tinype/
            constexpr HRESULT hr = HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
            wprintf(L" FAILED (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
            retVal = 1;
            continue;
        }

        ScopedHandle hFileMapping(CreateFileMappingW(hFile.get(), nullptr, PAGE_READONLY, 0, 0, nullptr));
        if (!hFileMapping)
        {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            wprintf(L" FAILED (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
            retVal = 1;
            continue;
        }

        auto fileBase = MapViewOfFile(hFileMapping.get(), FILE_MAP_READ, 0, 0, 0);
        if (!fileBase)
        {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            wprintf(L" FAILED (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
            retVal = 1;
            continue;
        }

        auto endPtr = static_cast<void*>(reinterpret_cast<uint8_t*>(fileBase) + fileInfo.EndOfFile.QuadPart);

        // Find PE header
        auto peHeader = static_cast<const PIMAGE_DOS_HEADER>(fileBase);
        if (peHeader->e_magic != IMAGE_DOS_SIGNATURE)
        {
            constexpr HRESULT hr = HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
            wprintf(L" FAILED (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
            retVal = 1;
            continue;
        }

        // Find NT header
        auto ntHeader = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
            static_cast<const uint8_t*>(fileBase) + peHeader->e_lfanew);

        if ((ntHeader+1) > endPtr)
        {
            constexpr HRESULT hr = HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
            wprintf(L" FAILED (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
            retVal = 1;
            continue;
        }

        if (ntHeader->Signature != IMAGE_NT_SIGNATURE)
        {
            constexpr HRESULT hr = HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
            wprintf(L" FAILED (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
            retVal = 1;
            continue;
        }

        if (ntHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64
            || ntHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        {
            wprintf(L"\nWARNING only scans x64 binaries, skipping\n");
            continue;
        }

        bool isdll = (ntHeader->FileHeader.Characteristics & IMAGE_FILE_DLL);
        bool isexe = (ntHeader->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE);
        if (isdll)
            wprintf(L" [DLL]\n");
        else if (isexe)
            wprintf(L" [EXE]\n");
        else
            wprintf(L" [???]\n");

        if (options & (1 << OPT_VERBOSE))
        {
            wprintf(L"\tLinker: %u.%02u\n",
                ntHeader->OptionalHeader.MajorLinkerVersion, ntHeader->OptionalHeader.MinorImageVersion);
            wprintf(L"\tOS: %u.%02u\n",
                ntHeader->OptionalHeader.MajorOperatingSystemVersion, ntHeader->OptionalHeader.MinorOperatingSystemVersion);
            wprintf(L"\tSubsystem: %u (%u.%02u)\n",
                ntHeader->OptionalHeader.Subsystem,
                ntHeader->OptionalHeader.MajorSubsystemVersion, ntHeader->OptionalHeader.MinorSubsystemVersion);
        }

        // Basic security warnings
        if (isdll)
        {
            if (!(ntHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE))
            {
                wprintf(L"WARNING: DLL is not built with /DYNAMICBASE\n");
            }
            if (!(ntHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NX_COMPAT))
            {
                wprintf(L"WARNING: DLL is not built with /NXCOMPAT\n");
            }
        }

        // Locate VERSIONINFO (if any)
        if (options & (1 << OPT_VERBOSE))
        {
            DWORD size = GetFileVersionInfoSizeW(pConv->szSrc, nullptr);
            if (size > 0)
            {
                auto verInfo = std::make_unique<uint8_t[]>(size);
                if (GetFileVersionInfoW(pConv->szSrc, 0, size, verInfo.get()))
                {
                    UINT ffiSize;
                    void* ffiPtr = nullptr;
                    if (VerQueryValueW(verInfo.get(), L"\\", &ffiPtr, &ffiSize))
                    {
                        if (ffiSize >= sizeof(VS_FIXEDFILEINFO))
                        {
                            auto fileInfoData = reinterpret_cast<VS_FIXEDFILEINFO*>(ffiPtr);
                            if (fileInfoData->dwSignature == 0xFEEF04BD)
                            {
                                wprintf(L"\tFileVersion: %u.%u.%u.%u\n",
                                    HIWORD(fileInfoData->dwFileVersionMS),
                                    LOWORD(fileInfoData->dwFileVersionMS),
                                    HIWORD(fileInfoData->dwFileVersionLS),
                                    LOWORD(fileInfoData->dwFileVersionLS));

                                wprintf(L"\tProductVersion: %u.%u.%u.%u\n",
                                    HIWORD(fileInfoData->dwProductVersionMS),
                                    LOWORD(fileInfoData->dwProductVersionMS),
                                    HIWORD(fileInfoData->dwProductVersionLS),
                                    LOWORD(fileInfoData->dwProductVersionLS));
                            }
                        }
                    }

                    LPVOID lpstr = nullptr;
                    UINT strLen = 0;
                    if (VerQueryValueW(verInfo.get(), L"\\StringFileInfo\\040904B0\\ProductName", &lpstr, &strLen))
                    {
                        wprintf(L"\tProductName: %ls\n", static_cast<const wchar_t*>(lpstr));
                    }

                    if (VerQueryValueW(verInfo.get(), L"\\StringFileInfo\\040904B0\\CompanyName", &lpstr, &strLen))
                    {
                        wprintf(L"\tCompany Name: %ls\n", static_cast<const wchar_t*>(lpstr));
                    }

                    if (VerQueryValueW(verInfo.get(), L"\\StringFileInfo\\040904B0\\FileDescription", &lpstr, &strLen))
                    {
                        wprintf(L"\tDescription: %ls\n", static_cast<const wchar_t*>(lpstr));
                    }

                    if (VerQueryValueW(verInfo.get(), L"\\StringFileInfo\\040904B0\\LegalCopyright", &lpstr, &strLen))
                    {
                        wprintf(L"\tCopyright: %ls\n", static_cast<const wchar_t*>(lpstr));
                    }

                    if (VerQueryValueW(verInfo.get(), L"\\StringFileInfo\\040904B0\\Comments", &lpstr, &strLen))
                    {
                        wprintf(L"\tComments: %ls\n", static_cast<const wchar_t*>(lpstr));
                    }
                }
            }
            else
            {
                wprintf(L"INFO: No version info (VERSIONINFO) found\n");
            }
        }

        {
            char name[MAX_PATH] = {};
            const int result = WideCharToMultiByte(CP_UTF8, 0, baseName, -1, name, MAX_PATH, nullptr, nullptr);
            if (result > 0)
            {
                std::ignore = _strlwr_s(name, result);
                foundDLLs.insert(name);
            }
        }

        // Process import modules.
        const auto moduleEntry = &ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        size_t moduleCount = (moduleEntry->Size >= sizeof(IMAGE_IMPORT_DESCRIPTOR))
            ? moduleEntry->Size / sizeof(IMAGE_IMPORT_DESCRIPTOR) - 1 : 0;

        const auto delayModuleEntry = &ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
        size_t delayModuleCount = (delayModuleEntry->Size >= sizeof(IMAGE_DELAYLOAD_DESCRIPTOR))
            ? delayModuleEntry->Size / sizeof(IMAGE_DELAYLOAD_DESCRIPTOR) - 1 : 0;

        size_t totalModuleCount = moduleCount + delayModuleCount;
        if (!totalModuleCount)
        {
            wprintf(L"INFO: No imports found\n");
            continue;
        }

        auto moduleInfo = std::make_unique<XBModuleInfo[]>(totalModuleCount);
        memset(moduleInfo.get(), 0, sizeof(XBModuleInfo) * totalModuleCount);

        auto secHeader = IMAGE_FIRST_SECTION(ntHeader);

        size_t index = 0;
        if (moduleEntry->Size > 0)
        {
            auto import = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(
                reinterpret_cast<const uint8_t*>(fileBase) + Rva2Offset(moduleEntry->VirtualAddress, secHeader, ntHeader));

            for (;; ++import)
            {
                if (!import->Name)
                    break;

                if (index >= moduleCount)
                {
                    wprintf(L"ERROR: Invalid import table\n");
                    retVal = 1;
                    continue;
                }

                auto libname = reinterpret_cast<const CHAR*>(
                    reinterpret_cast<const uint8_t*>(fileBase) + Rva2Offset(import->Name, secHeader, ntHeader));

                moduleInfo[index].fileName = libname;
                moduleInfo[index].importDesc = import;
                ++index;
            }
        }

        // Process delay import modules (if any).
        if (delayModuleEntry->Size > 0)
        {
            auto import = reinterpret_cast<const IMAGE_DELAYLOAD_DESCRIPTOR*>(
                reinterpret_cast<const uint8_t*>(fileBase) + Rva2Offset(delayModuleEntry->VirtualAddress, secHeader, ntHeader));

            size_t index2 = 0;
            for (;; ++import)
            {
                if (!import->DllNameRVA)
                    break;

                if (index >= totalModuleCount || index2 >= delayModuleCount)
                {
                    wprintf(L"ERROR: Invalid delay load import table\n");
                    retVal = 1;
                    continue;
                }

                auto libname = reinterpret_cast<const CHAR*>(
                    reinterpret_cast<const uint8_t*>(fileBase) + Rva2Offset(import->DllNameRVA, secHeader, ntHeader));

                moduleInfo[index].fileName = libname;
                moduleInfo[index].delayImportDesc = import;
                ++index; ++index2;
            }
        }

        if (index < totalModuleCount)
        {
            wprintf(L"WARNING: Unexpected number of imports found (expected %zu, found %zu)\n", totalModuleCount, index);
            totalModuleCount = index;
        }

        std::sort(moduleInfo.get(), moduleInfo.get() + totalModuleCount,
            [](const XBModuleInfo& a, const XBModuleInfo& b) -> bool
            {
                return _stricmp(a.fileName, b.fileName) < 0;
            });

        wprintf(L"INFO: Found %zu import modules\n", totalModuleCount);

        XBTARGET thisTarget = target;
        if (!CategorizeModules(moduleInfo.get(), totalModuleCount, thisTarget, basePath, options))
            retVal = 1;

        VCMinimumVersion minVSVer = VisualCRuntimeChecks(baseName, moduleInfo.get(), totalModuleCount, thisTarget);

        using namespace ::KnownAPIs;

        VALIDATE_IS_SORTED(c_GameOSAPIs);
        VALIDATE_IS_SORTED(c_additionalGameOSAPIs);

        auto pred = [](const char* a, const char* b) -> bool
        {
            return _stricmp(a, b) < 0;
        };

        // Process imports from modules.
        std::vector<std::pair<const char*,const char*>> disallowedAPIs;

        if (minVSVer == VCMinimumVersion::Ignore)
        {
            // Skip import checking for the VC and OS CRT files.
            wprintf(L"INFO: Known CRT DLL\n");
        }
        else
        {
            std::regex oscrt_regex(c_osCRTRegEx,
                std::regex_constants::ECMAScript | std::regex_constants::icase);

            for (size_t j = 0; j < totalModuleCount; ++j)
            {
                if (moduleInfo[j].category != XBMODULE_CATEGORY::OS
                    && moduleInfo[j].category != XBMODULE_CATEGORY::CRT)
                {
                    // We only process imports for a subset of modules.
                    continue;
                }

                auto libname = moduleInfo[j].fileName;

                if (std::regex_search(libname, oscrt_regex))
                {
                    // Skip scanning imports from the OS CRT files.
                    continue;
                }

                const IMAGE_THUNK_DATA64* thunk = nullptr;
                if (moduleInfo[j].delayImportDesc)
                {
                    auto dimport = moduleInfo[j].delayImportDesc;
                    thunk = reinterpret_cast<const IMAGE_THUNK_DATA64*>(
                        reinterpret_cast<const uint8_t*>(fileBase) + Rva2Offset(dimport->ImportNameTableRVA, secHeader, ntHeader));
                }
                else
                {
                    auto import = moduleInfo[j].importDesc;
                    thunk = reinterpret_cast<const IMAGE_THUNK_DATA64*>(
                        reinterpret_cast<const uint8_t*>(fileBase) + Rva2Offset(import->FirstThunk, secHeader, ntHeader));
                }

                if (thunk < endPtr)
                {
                    while (thunk->u1.AddressOfData != 0)
                    {
                        if (thunk >= endPtr)
                        {
                            wprintf(L"ERROR: Invalid import table\n");
                            retVal = 1;
                            break;
                        }

                        if (thunk->u1.AddressOfData & IMAGE_ORDINAL_FLAG64)
                        {
                            // Ignore ordinals.
                        }
                        else
                        {
                            auto byname = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(
                                reinterpret_cast<const uint8_t*>(fileBase) + Rva2Offset(thunk->u1.AddressOfData, secHeader, ntHeader));

                            switch (moduleInfo[j].category)
                            {
                            case XBMODULE_CATEGORY::OS:
                                if (thisTarget == XBTARGET::XboxOne || thisTarget == XBTARGET::Scarlett)
                                {
                                    if (!std::binary_search(c_GameOSAPIs, c_GameOSAPIs + std::size(c_GameOSAPIs),
                                        byname->Name, pred))
                                    {
                                        if (!std::binary_search(c_additionalGameOSAPIs, c_additionalGameOSAPIs + std::size(c_additionalGameOSAPIs),
                                            byname->Name, pred))
                                        {
                                            disallowedAPIs.push_back(std::make_pair<>(libname, byname->Name));
                                        }
                                    }
                                }
                                break;

                            case XBMODULE_CATEGORY::CRT:
                                if (minVSVer != VCMinimumVersion::Ignore)
                                {
                                    if (_stricmp(byname->Name, "__CxxFrameHandler4") == 0)
                                    {
                                        // d2FH4
                                        minVSVer = VCMinimumVersion::VS2019;
                                    }
                                }
                                break;

                            default:
                                break;
                            }
                        }

                        ++thunk;
                    }
                }
            }
        }

        // Output results
        const wchar_t* minVer = nullptr;
        switch (minVSVer)
        {
        case VCMinimumVersion::VS2017: minVer = L"VS 2017 (15.0)"; break;
        case VCMinimumVersion::VS2017_15_7: minVer = L"VS 2017 (15.7)"; break;
        case VCMinimumVersion::VS2019: minVer = L"VS 2019 (16.0)"; break;
        case VCMinimumVersion::VS2019_16_2: minVer = L"VS 2019 (16.2)"; break;
        case VCMinimumVersion::VS2019_16_8: minVer = L"VS 2019 (16.8)"; break;
        default: break;
        }

        if (minVSVer != VCMinimumVersion::Ignore)
        {
            if (overallMinVer < minVSVer)
                overallMinVer = minVSVer;
        }

        if (minVer)
        {
            wprintf(L"INFO: Dependencies require '%ls' or later C/C++ Runtime\n", minVer);
        }

        if (!disallowedAPIs.empty())
        {
            wprintf(L"ERROR: The following APIs are not in WINAPI_FAMILY_GAMES\n");
            for (auto it = disallowedAPIs.cbegin(); it != disallowedAPIs.cend(); ++it)
            {
                if (options & (1 << OPT_VERBOSE))
                {
                    wprintf(L"\t%hs!%hs\n", it->first, it->second);
                }
                else
                {
                    wprintf(L"\t%hs\n", it->second);
                }
            }
            retVal = 1;
        }

        size_t unresolvedhard = 0;
        size_t unresolvedsoft = 0;
        for (index = 0; index < totalModuleCount; ++index)
        {
            if (!(options & (1 << OPT_VERBOSE)) && moduleInfo[index].category != XBMODULE_CATEGORY::Unknown)
            {
                // Normally only shows 'unknown' category modules.
                continue;
            }

            if (moduleInfo[index].category == XBMODULE_CATEGORY::Unknown)
            {
                if (moduleInfo[index].importDesc)
                {
                    std::string name = moduleInfo[index].fileName;

#pragma warning(suppress : 4244)
                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

                    missingDLLs.insert(name);
                    ++unresolvedhard;
                }
                else
                {
                    std::string name = moduleInfo[index].fileName;

                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

                    missingDelayLoadDLLs.insert(name);
                    ++unresolvedsoft;
                }
            }

            wprintf(L"%ls '%hs' (%ls)\n",
                moduleInfo[index].importDesc ? L"  DLL" : L"DLoad",
                moduleInfo[index].fileName,
                GetCategoryString(moduleInfo[index].category));
        }

        if (unresolvedhard > 0)
        {
            wprintf(L"ERROR: %zu unresolved modules required to launch\n", unresolvedhard);
            retVal = 1;
        }

        if (unresolvedsoft > 0)
        {
            wprintf(L"WARNING: %zu unresolved delay load modules\n", unresolvedsoft);
        }

        // All name strings are references into the original file, so keep it alive until we are done.
        hFileMapping.reset();
    }

    if (options & (1 << OPT_LAYOUT))
    {
        wprintf(L"\n\n");

        bool found_in_other_folder = false;

        if (missingDLLs.empty())
        {
            wprintf(L"\tNo unidentified user DLLs encountered in layout.\n");
        }
        else
        {
            bool first = true;
            for (auto it : missingDLLs)
            {
                if (foundDLLs.find(it) != foundDLLs.end())
                {
                    found_in_other_folder = true;
                }
                else if (first)
                {
                    wprintf(L"ERROR: Could not locate these DLLs in the layout:\n\t%hs\n", it.c_str());
                    retVal = 1;
                    first = false;
                }
                else
                {
                    wprintf(L"\t%hs\n", it.c_str());
                }
            }
        }

        if (!missingDelayLoadDLLs.empty())
        {
            bool first = true;
            for (auto it : missingDelayLoadDLLs)
            {
                if (foundDLLs.find(it) != foundDLLs.end())
                {
                    found_in_other_folder = true;
                }
                else if (first)
                {
                    wprintf(L"ERROR: Could not locate these Delay Load DLLs in the layout:\n\t%hs\n", it.c_str());
                    retVal = 1;
                    first = false;
                }
                else
                {
                    wprintf(L"\t%hs\n", it.c_str());
                }
            }
        }

        if (found_in_other_folder)
        {
            wprintf(L"WARNING: Some user DLLs were found in other folders. May not be found at runtime.\n");
        }

        if (CheckVisualCRTFiles(foundDLLs, overallMinVer))
        {
            // Found all required REDIST files in layout
        }
        else if (target == XBTARGET::XboxOne || target == XBTARGET::Scarlett)
        {
            wprintf(L"ERROR: Missing Visual C/C++ Runtime DLLs in layout.\n");
            retVal = 1;
        }
        else
        {
            wprintf(L"WARNING: This project relies on finding the Visual C/C+ Runtime installed on the machine.\n");
        }

        if (foundDLLs.find("ucrtbase.dll") != foundDLLs.end())
        {
            wprintf(L"WARNING: 'ucrtbase.dll' is not required in the layout; already included in the OS");
        }
    }

    return retVal;
}
