//--------------------------------------------------------------------------------------
// File: compresstool.cpp
//
// A "quick & dirty" compression tool inspired by the classic MS-DOS COMPRESS.EXE and
// EXPAND.EXE commands. This tool implements a SZDD/KWAJ-style file format, and makes
// use of the Compression API introduced with Windows 8.
//
// "SZDD" (1990) is the signature of the original MS-DOS COMPRESS.EXE / EXPAND.EXE
// file format. "KWAJ" (1993) is a similar format which supported additional
// compression methods.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
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
#include <cwchar>
#include <fstream>
#include <list>
#include <locale>
#include <memory>
#include <set>
#include <utility>

#ifndef NTDDI_WIN10_FE
#undef WINAPI_FAMILY_PARTITION
#define WINAPI_FAMILY_PARTITION(Partitions) 1
#endif

#include <compressapi.h>

#ifndef NTDDI_WIN10_FE
#undef WINAPI_FAMILY_PARTITION
#define WINAPI_FAMILY_PARTITION(Partitions) (Partitions)
#endif

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

    class auto_delete_file
    {
    public:
        auto_delete_file(HANDLE hFile) noexcept : m_handle(hFile) {}

        auto_delete_file(const auto_delete_file&) = delete;
        auto_delete_file& operator=(const auto_delete_file&) = delete;

        auto_delete_file(const auto_delete_file&&) = delete;
        auto_delete_file& operator=(const auto_delete_file&&) = delete;

        ~auto_delete_file()
        {
            if (m_handle)
            {
                FILE_DISPOSITION_INFO info = {};
                info.DeleteFile = TRUE;
                (void)SetFileInformationByHandle(m_handle, FileDispositionInfo, &info, sizeof(info));
            }
        }

        void clear() noexcept { m_handle = nullptr; }

    private:
        HANDLE m_handle;
    };
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

enum OPTIONS : uint32_t
{
    OPT_RECURSIVE = 1,
    OPT_UNCOMPRESS,
    OPT_MSZIP,
    OPT_TOLOWER,
    OPT_OVERWRITE,
    OPT_NOLOGO,
    OPT_TIMING,
    OPT_FILELIST,
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
    { L"u",         OPT_UNCOMPRESS },
    { L"z",         OPT_MSZIP },
    { L"l",         OPT_TOLOWER },
    { L"y",         OPT_OVERWRITE },
    { L"nologo",    OPT_NOLOGO },
    { L"timing",    OPT_TIMING },
    { L"flist",     OPT_FILELIST },
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
                        std::transform(name.begin(), name.end(), name.begin(), towlower);
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
        wprintf(L"Microsoft (R) SZDD/KWAJ-style compression tool for Windows & Xbox\n");
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
            L"   -r                  wildcard filename search is recursive\n"
            L"   -u                  uncompress files rather than compress\n"
            L"   -z                  compress with MSZIP rather than LZMS\n"
            L"   -l                  force output filename to lower case\n"
            L"   -y                  overwrite existing output file (if any)\n"
            L"   -nologo             suppress copyright message\n"
            L"   -timing             Display elapsed processing time\n"
            L"   -flist <filename>   use text file with a list of input files (one per line)\n"
            "\n";

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
        }

        return desc;
    }

    HRESULT ReadData(_In_z_ const wchar_t* szFile, std::unique_ptr<uint8_t[]>& blob, size_t& blobSize)
    {
        blob.reset();

        ScopedHandle hFile(safe_handle(CreateFile2(szFile, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr)));
        if (!hFile)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        // Get the file size
        FILE_STANDARD_INFO fileInfo;
        if (!GetFileInformationByHandleEx(hFile.get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        // File is too big for 32-bit allocation, so reject read (4 GB should be plenty large enough)
        if (fileInfo.EndOfFile.HighPart > 0)
        {
            return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
        }

        // Zero-sized files assumed to be invalid
        if (fileInfo.EndOfFile.LowPart < 1)
        {
            return E_FAIL;
        }

        // Read file
        blob.reset(new (std::nothrow) uint8_t[fileInfo.EndOfFile.LowPart]);
        if (!blob)
        {
            return E_OUTOFMEMORY;
        }

        DWORD bytesRead = 0;
        if (!ReadFile(hFile.get(), blob.get(), fileInfo.EndOfFile.LowPart, &bytesRead, nullptr))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (bytesRead != fileInfo.EndOfFile.LowPart)
        {
            return E_FAIL;
        }

        blobSize = fileInfo.EndOfFile.LowPart;

        return S_OK;
    }
}

namespace
{
    // The format used by this tool is the same design as the classic SZDD/KAWJ formats used by MS-DOS.
    // Files generated by this tool, however, are not compatible with EXPAND.EXE or GnuWin32 MSCOMPRESS.EXE.

    constexpr uint8_t c_CFileSignatureLen = 8;
    constexpr uint8_t c_CFileVersion = 0x41;

    const uint8_t c_Signature[c_CFileSignatureLen] = { 0x41, 0x46, 0x43, 0x57, 0x47, 0x50, 0x53, 0x4d };

#pragma pack(push,1)
    struct CFileHeader
    {
        uint8_t     magic[c_CFileSignatureLen]; // Must match c_Signature below
        uint8_t     mode;                       // COMPRESS_ALGORITHM_x enum
        uint8_t     version;
        wchar_t     lastChar;
        uint32_t    uncompressedSize;
    };
#pragma pack(pop)

    static_assert(sizeof(CFileHeader) == 16, "File header size mismatch");

    PVOID SimpleAlloc(PVOID, SIZE_T Size)
    {
        return malloc(Size);
    }

    VOID SimpleFree(PVOID, PVOID Memory)
    {
        free(Memory);
    }

    struct compressor_closer { void operator()(void* h) { if (h) CloseCompressor(static_cast<COMPRESSOR_HANDLE>(h)); } };

    HRESULT CompressFile(
        _In_reads_bytes_(dataLen) const void* data,
        size_t dataLen,
        DWORD algorithm,
        wchar_t origChar,
        const wchar_t* compressFile)
    {
        if (!data || !dataLen || !compressFile)
            return E_INVALIDARG;

        if (dataLen > UINT32_MAX)
        {
            return E_FAIL;
        }

        COMPRESS_ALLOCATION_ROUTINES allocData = { SimpleAlloc, SimpleFree, nullptr };

        std::unique_ptr<void, compressor_closer> compressor;
        {
            COMPRESSOR_HANDLE h = nullptr;
            if (!CreateCompressor(
                algorithm,
                &allocData,
                &h))
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }

            compressor.reset(h);
        }

        if (algorithm == COMPRESS_ALGORITHM_LZMS)
        {
            DWORD blockSize = 1 * 1024 * 1024; // 1 MB recommended for LZMS

            if (!SetCompressorInformation(
                static_cast<COMPRESSOR_HANDLE>(compressor.get()),
                COMPRESS_INFORMATION_CLASS_BLOCK_SIZE,
                &blockSize,
                sizeof(DWORD)))
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }
        }

        // Query max compressed size.
        SIZE_T compressedBufferSize;
        if (!Compress(
            static_cast<COMPRESSOR_HANDLE>(compressor.get()),
            nullptr,
            dataLen,
            nullptr,
            0,
            &compressedBufferSize))
        {
            DWORD errorCode = GetLastError();
            if (errorCode != ERROR_INSUFFICIENT_BUFFER)
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }
        }

        std::unique_ptr<uint8_t[]> compressedData(new (std::nothrow) uint8_t[compressedBufferSize]);
        if (!compressedData)
        {
            return E_OUTOFMEMORY;
        }

        SIZE_T compressedSize;
        if (!Compress(
            static_cast<COMPRESSOR_HANDLE>(compressor.get()),
            data,
            dataLen,
            compressedData.get(),
            compressedBufferSize,
            &compressedSize))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (compressedSize > (UINT32_MAX - sizeof(CFileHeader)))
        {
            return E_FAIL;
        }

        // Create compressed file.
        ScopedHandle hFile(safe_handle(CreateFile2(compressFile, GENERIC_WRITE, 0, CREATE_ALWAYS, nullptr)));
        if (!hFile)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        auto_delete_file delonfail(hFile.get());

        // Write header.
        CFileHeader fileHeader = {};
        memcpy(fileHeader.magic, c_Signature, c_CFileSignatureLen);
        fileHeader.mode = static_cast<uint8_t>(algorithm);
        fileHeader.version = c_CFileVersion;
        fileHeader.lastChar = origChar;
        fileHeader.uncompressedSize = static_cast<DWORD>(dataLen);

        DWORD bytesWritten;
        if (!WriteFile(hFile.get(), &fileHeader, static_cast<DWORD>(sizeof(CFileHeader)), &bytesWritten, nullptr))
            return HRESULT_FROM_WIN32(GetLastError());

        if (bytesWritten != sizeof(CFileHeader))
            return E_FAIL;

        // Write compressed data.
        if (!WriteFile(hFile.get(), compressedData.get(), static_cast<DWORD>(compressedSize), &bytesWritten, nullptr))
            return HRESULT_FROM_WIN32(GetLastError());

        if (bytesWritten != static_cast<DWORD>(compressedSize))
            return E_FAIL;

        delonfail.clear();

        return S_OK;
    }

    struct decompressor_closer { void operator()(void* h) { if (h) CloseDecompressor(static_cast<DECOMPRESSOR_HANDLE>(h)); } };

    HRESULT DecompressFile(
        _In_reads_bytes_(dataLen) const void* data,
        size_t dataLen,
        const wchar_t* fileName)
    {
        if (!data || !fileName)
            return E_INVALIDARG;

        if (dataLen <= sizeof(CFileHeader))
            return E_FAIL;

        auto hdr = reinterpret_cast<const CFileHeader*>(data);
        if (memcmp(hdr, c_Signature, c_CFileSignatureLen) != 0)
            return E_FAIL;

        if (hdr->version != c_CFileVersion)
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

        switch (hdr->mode)
        {
        case COMPRESS_ALGORITHM_MSZIP:  // https://en.wikipedia.org/wiki/Deflate
        case COMPRESS_ALGORITHM_LZMS:   // https://en.wikipedia.org/wiki/Quantum_compression
            break;

        default:
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }

        COMPRESS_ALLOCATION_ROUTINES allocData = { SimpleAlloc, SimpleFree, nullptr };

        std::unique_ptr<void, decompressor_closer> decompressor;
        {
            DECOMPRESSOR_HANDLE h = nullptr;
            if (!CreateDecompressor(
                static_cast<DWORD>(hdr->mode),
                &allocData,
                &h))
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }

            decompressor.reset(h);
        }

        // Query decompressed buffer size.
        auto payload = reinterpret_cast<const void*>(reinterpret_cast<const uint8_t*>(data) + sizeof(CFileHeader));
        size_t payloadLen = dataLen - sizeof(CFileHeader);

        SIZE_T bufferSize;
        if (!Decompress(
            static_cast<DECOMPRESSOR_HANDLE>(decompressor.get()),
            payload,
            payloadLen,
            nullptr,
            0,
            &bufferSize))
        {
            DWORD errorCode = GetLastError();
            if (errorCode != ERROR_INSUFFICIENT_BUFFER)
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }
        }

        // Ensure our header and the compression API agree.
        if (hdr->uncompressedSize != bufferSize)
            return E_FAIL;

        std::unique_ptr<uint8_t[]> expandedData(new (std::nothrow) uint8_t[bufferSize]);
        if (!expandedData)
        {
            return E_OUTOFMEMORY;
        }

        SIZE_T fileSize;
        if (!Decompress(
            static_cast<DECOMPRESSOR_HANDLE>(decompressor.get()),
            payload,
            payloadLen,
            expandedData.get(),
            bufferSize,
            &fileSize))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (fileSize > UINT32_MAX)
        {
            return E_FAIL;
        }

        // Create expanded file.
        ScopedHandle hFile(safe_handle(CreateFile2(fileName, GENERIC_WRITE, 0, CREATE_ALWAYS, nullptr)));
        if (!hFile)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        auto_delete_file delonfail(hFile.get());

        DWORD bytesWritten;
        if (!WriteFile(hFile.get(), expandedData.get(), static_cast<DWORD>(fileSize), &bytesWritten, nullptr))
            return HRESULT_FROM_WIN32(GetLastError());

        if (bytesWritten != fileSize)
            return E_FAIL;

        delonfail.clear();

        return S_OK;
    }
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

    LARGE_INTEGER qpcFreq = {};
    (void)QueryPerformanceFrequency(&qpcFreq);

    LARGE_INTEGER qpcStart = {};
    (void)QueryPerformanceCounter(&qpcStart);

    int retVal = 0;

    for (auto pConv = conversion.begin(); pConv != conversion.end(); ++pConv)
    {
        wchar_t drive[_MAX_DRIVE] = {};
        wchar_t dir[_MAX_DIR] = {};
        wchar_t ext[_MAX_EXT] = {};
        wchar_t fname[_MAX_FNAME] = {};
        _wsplitpath_s(pConv->szSrc, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);

        if (pConv != conversion.begin())
            wprintf(L"\n");

        wchar_t lastChar = 0;
        size_t len = wcslen(ext);
        if (len > 0)
        {
            lastChar = ext[len - 1];
        }

        if (options & (1 << OPT_UNCOMPRESS))
        {
            if (lastChar != L'_')
            {
                wprintf(L"skipping '%ls' as it lacks the '_' end marker\n", pConv->szSrc);
                continue;
            }

            wprintf(L"expanding %ls", pConv->szSrc);
            fflush(stdout);
        }
        else
        {
            if (lastChar == L'_')
            {
                wprintf(L"skipping '%ls' as it already has the '_' end marker\n", pConv->szSrc);
                continue;

            }

            wprintf(L"compressing [%ls] %ls",
                (options& (1 << OPT_MSZIP)) ? L"MSZIP" : L"LZMS",
                pConv->szSrc);
            fflush(stdout);
        }

        // Load source file to process.
        std::unique_ptr<uint8_t[]> blob;
        size_t blobSize;
        HRESULT hr = ReadData(pConv->szSrc, blob, blobSize);
        if (FAILED(hr))
        {
            wprintf(L" FAILED (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
            retVal = 1;
            continue;
        }

        // Determine target filename to write.
        wchar_t origChar = 0;
        wchar_t ext2[_MAX_EXT] = {};
        if (options & (1 << OPT_UNCOMPRESS))
        {
            if (blobSize < sizeof(CFileHeader))
            {
                wprintf(L" FAILED - File too small to contain valid header.\n");
                retVal = 1;
                continue;
            }

            auto hdr = reinterpret_cast<CFileHeader*>(blob.get());
            if (memcmp(hdr, c_Signature, c_CFileSignatureLen) != 0)
            {
                wprintf(L" FAILED - Invalid compress header signature.\n");
                retVal = 1;
                continue;
            }

            if (hdr->version != c_CFileVersion)
            {
                wprintf(L" FAILED - Unknown compress header version (%u).\n", hdr->version);
                retVal = 1;
                continue;
            }

            if (ext[0] == L'.' && ext[1] == L'_' && !ext[2])
            {
                *ext2 = 0;
            }
            else if (len >= 3)
            {
                wcscpy_s(ext2, ext);
                ext2[len - 1] = hdr->lastChar;
            }
            else
            {
                wprintf(L" FAILED - Unexpected location of '_' in filename.\n");
                retVal = 1;
                continue;
            }
        }
        else
        {
            if (len >= 3)
            {
                wcscpy_s(ext2, ext);
                origChar = ext[len - 1];
                ext2[len - 1] = L'_';
            }
            else
            {
                wcscpy_s(ext2, L"._");
            }
        }

        wchar_t destName[MAX_PATH] = {};
        _wmakepath_s(destName, drive, dir, fname, ext2);

        if (options & (1 << OPT_TOLOWER))
        {
            (void)_wcslwr_s(destName);
        }

        if (~options & (1 << OPT_OVERWRITE))
        {
            if (GetFileAttributesW(destName) != INVALID_FILE_ATTRIBUTES)
            {
                wprintf(L"\nERROR: Output file %ls already exists, use -y to overwrite!\n", destName);
                return 1;
            }
        }

        if (options & (1 << OPT_UNCOMPRESS))
        {
            hr = DecompressFile(blob.get(), blobSize, destName);
        }
        else
        {
            hr = CompressFile(blob.get(), blobSize,
                (options & (1 << OPT_MSZIP)) ? COMPRESS_ALGORITHM_MSZIP : COMPRESS_ALGORITHM_LZMS,
                origChar, destName);
        }
        if (FAILED(hr))
        {
            wprintf(L" FAILED (%08X%ls)\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
            retVal = 1;
            continue;
        }

        wprintf(L" done.\n");
    }

    if (options & (uint64_t(1) << OPT_TIMING))
    {
        LARGE_INTEGER qpcEnd = {};
        (void)QueryPerformanceCounter(&qpcEnd);

        LONGLONG delta = qpcEnd.QuadPart - qpcStart.QuadPart;
        wprintf(L"\n Processing time: %f seconds\n", double(delta) / double(qpcFreq.QuadPart));
    }

    return retVal;
}
