//--------------------------------------------------------------------------------------
// File: DirectXTexEXR.cpp
//
// DirectXTex Auxillary functions for using the OpenEXR library
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

//Uncomment if you add DirectXTexEXR to your copy of the DirectXTex library
//#include "DirectXTexP.h"

#include "DirectXTexEXR.h"

#include <DirectXPackedVector.h>

#include <cassert>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>

//
// Requires the OpenEXR library <http://www.openexr.com/> and ZLIB <http://www.zlib.net>
//

#ifdef __clang__
#pragma clang diagnostic ignored "-Wswitch-enum"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wdeprecated-dynamic-exception-spec"
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wshadow-field-in-constructor"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#pragma warning(push)
#pragma warning(disable : 4244 4996 26439 26495 26496 26812)
#include <ImfRgbaFile.h>
#include <ImfIO.h>
#pragma warning(pop)

#ifdef __clang__
#pragma clang diagnostic pop
#endif

static_assert(sizeof(Imf::Rgba) == 8, "Mismatch size");

using namespace DirectX;
using PackedVector::XMHALF4;

// Comment out this first anonymous namespace if you add the include of DirectXTexP.h above
#ifdef WIN32
namespace
{
    struct handle_closer { void operator()(HANDLE h) noexcept { assert(h != INVALID_HANDLE_VALUE); if (h) CloseHandle(h); } };

    using ScopedHandle = std::unique_ptr<void, handle_closer>;

    inline HANDLE safe_handle(HANDLE h) noexcept { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }

    class auto_delete_file
    {
    public:
        auto_delete_file(HANDLE hFile) noexcept : m_handle(hFile) {}

        auto_delete_file(const auto_delete_file&) = delete;
        auto_delete_file& operator=(const auto_delete_file&) = delete;

        auto_delete_file(auto_delete_file&&) = delete;
        auto_delete_file& operator=(auto_delete_file&&) = delete;

        ~auto_delete_file()
        {
            if (m_handle)
            {
                FILE_DISPOSITION_INFO info = {};
                info.DeleteFile = TRUE;
                std::ignore = SetFileInformationByHandle(m_handle, FileDispositionInfo, &info, sizeof(info));
            }
        }

        void clear() noexcept { m_handle = nullptr; }

    private:
        HANDLE m_handle;
    };
}
#endif

#ifdef WIN32
namespace
{
    class com_exception : public std::exception
    {
    public:
        com_exception(HRESULT hr) noexcept : result(hr) {}

        const char* what() const noexcept override
        {
            static char s_str[64] = {};
            sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
            return s_str;
        }

        HRESULT get_result() const noexcept { return result; }

    private:
        HRESULT result;
    };

    class InputStream : public Imf::IStream
    {
    public:
        InputStream(HANDLE hFile, const char fileName[]) :
            IStream(fileName), m_hFile(hFile)
        {
            const LARGE_INTEGER dist = {};
            LARGE_INTEGER result;
            if (!SetFilePointerEx(m_hFile, dist, &result, FILE_END))
            {
                throw com_exception(HRESULT_FROM_WIN32(GetLastError()));
            }

            m_EOF = result.QuadPart;

            if (!SetFilePointerEx(m_hFile, dist, nullptr, FILE_BEGIN))
            {
                throw com_exception(HRESULT_FROM_WIN32(GetLastError()));
            }
        }

        InputStream(const InputStream&) = delete;
        InputStream& operator = (const InputStream&) = delete;

        InputStream(InputStream&&) = delete;
        InputStream& operator=(InputStream&&) = delete;

        bool read(char c[], int n) override
        {
            DWORD bytesRead;
            if (!ReadFile(m_hFile, c, static_cast<DWORD>(n), &bytesRead, nullptr))
            {
                throw com_exception(HRESULT_FROM_WIN32(GetLastError()));
            }

            const LARGE_INTEGER dist = {};
            LARGE_INTEGER result;
            if (!SetFilePointerEx(m_hFile, dist, &result, FILE_CURRENT))
            {
                throw com_exception(HRESULT_FROM_WIN32(GetLastError()));
            }

            return result.QuadPart >= m_EOF;
        }

        Imf::Int64 tellg() override
        {
            const LARGE_INTEGER dist = {};
            LARGE_INTEGER result;
            if (!SetFilePointerEx(m_hFile, dist, &result, FILE_CURRENT))
            {
                throw com_exception(HRESULT_FROM_WIN32(GetLastError()));
            }
            return static_cast<Imf::Int64>(result.QuadPart);
        }

        void seekg(Imf::Int64 pos) override
        {
            LARGE_INTEGER dist;
            dist.QuadPart = static_cast<LONGLONG>(pos);
            if (!SetFilePointerEx(m_hFile, dist, nullptr, FILE_BEGIN))
            {
                throw com_exception(HRESULT_FROM_WIN32(GetLastError()));
            }
        }

        void clear() override
        {
            SetLastError(0);
        }

    private:
        HANDLE m_hFile;
        LONGLONG m_EOF;
    };

    class OutputStream : public Imf::OStream
    {
    public:
        OutputStream(HANDLE hFile, const char fileName[]) :
            OStream(fileName), m_hFile(hFile) {}

        OutputStream(const OutputStream&) = delete;
        OutputStream& operator = (const OutputStream&) = delete;

        OutputStream(OutputStream&&) = delete;
        OutputStream& operator=(OutputStream&&) = delete;

        void write(const char c[], int n) override
        {
            DWORD bytesWritten;
            if (!WriteFile(m_hFile, c, static_cast<DWORD>(n), &bytesWritten, nullptr))
            {
                throw com_exception(HRESULT_FROM_WIN32(GetLastError()));
            }
        }

        Imf::Int64 tellp() override
        {
            const LARGE_INTEGER dist = {};
            LARGE_INTEGER result;
            if (!SetFilePointerEx(m_hFile, dist, &result, FILE_CURRENT))
            {
                throw com_exception(HRESULT_FROM_WIN32(GetLastError()));
            }
            return static_cast<Imf::Int64>(result.QuadPart);
        }

        void seekp(Imf::Int64 pos) override
        {
            LARGE_INTEGER dist;
            dist.QuadPart = static_cast<LONGLONG>(pos);
            if (!SetFilePointerEx(m_hFile, dist, nullptr, FILE_BEGIN))
            {
                throw com_exception(HRESULT_FROM_WIN32(GetLastError()));
            }
        }

    private:
        HANDLE m_hFile;
    };
}
#endif // WIN32


//=====================================================================================
// Entry-points
//=====================================================================================

//-------------------------------------------------------------------------------------
// Obtain metadata from EXR file on disk
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::GetMetadataFromEXRFile(const wchar_t* szFile, TexMetadata& metadata)
{
    if (!szFile)
        return E_INVALIDARG;

#ifdef WIN32
    char fileName[MAX_PATH] = {};
    const int result = WideCharToMultiByte(CP_UTF8, 0, szFile, -1, fileName, MAX_PATH, nullptr, nullptr);
    if (result <= 0)
    {
        *fileName = 0;
    }

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    ScopedHandle hFile(safe_handle(CreateFile2(
        szFile,
        GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING,
        nullptr)));
#else
    ScopedHandle hFile(safe_handle(CreateFileW(
        szFile,
        GENERIC_READ, FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr)));
#endif
    if (!hFile)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    InputStream stream(hFile.get(), fileName);
#else
    std::wstring wFileName(szFile);
    std::string fileName(wFileName.cbegin(), wFileName.cend());
#endif

    HRESULT hr = S_OK;

    try
    {
#ifdef WIN32
        Imf::RgbaInputFile file(stream);
#else
        Imf::RgbaInputFile file(fileName.c_str());
#endif

        const auto dw = file.dataWindow();

        const int width = dw.max.x - dw.min.x + 1;
        const int height = dw.max.y - dw.min.y + 1;

        if (width < 1 || height < 1)
            return E_FAIL;

        metadata.width = static_cast<size_t>(width);
        metadata.height = static_cast<size_t>(height);
        metadata.depth = metadata.arraySize = metadata.mipLevels = 1;
        metadata.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        metadata.dimension = TEX_DIMENSION_TEXTURE2D;
    }
#ifdef WIN32
    catch (const com_exception& exc)
    {
#ifdef _DEBUG
        OutputDebugStringA(exc.what());
#endif
        hr = exc.get_result();
    }
#endif
#if defined(WIN32) && defined(_DEBUG)
    catch (const std::exception& exc)
    {
        OutputDebugStringA(exc.what());
        hr = E_FAIL;
    }
#else
    catch (const std::exception&)
    {
        hr = E_FAIL;
    }
#endif
    catch (...)
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}


//-------------------------------------------------------------------------------------
// Load a EXR file from disk
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::LoadFromEXRFile(const wchar_t* szFile, TexMetadata* metadata, ScratchImage& image)
{
    if (!szFile)
        return E_INVALIDARG;

    image.Release();

    if (metadata)
    {
        memset(metadata, 0, sizeof(TexMetadata));
    }

#ifdef WIN32
    char fileName[MAX_PATH] = {};
    const int result = WideCharToMultiByte(CP_UTF8, 0, szFile, -1, fileName, MAX_PATH, nullptr, nullptr);
    if (result <= 0)
    {
        *fileName = 0;
    }

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    ScopedHandle hFile(safe_handle(CreateFile2(
        szFile,
        GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING,
        nullptr)));
#else
    ScopedHandle hFile(safe_handle(CreateFileW(
        szFile,
        GENERIC_READ, FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr)));
#endif
    if (!hFile)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    InputStream stream(hFile.get(), fileName);
#else
    std::wstring wFileName(szFile);
    std::string fileName(wFileName.cbegin(), wFileName.cend());
#endif

    HRESULT hr = S_OK;

    try
    {
#ifdef WIN32
        Imf::RgbaInputFile file(stream);
#else
        Imf::RgbaInputFile file(fileName.c_str());
#endif

        auto const dw = file.dataWindow();

        const int width = dw.max.x - dw.min.x + 1;
        const int height = dw.max.y - dw.min.y + 1;

        if (width < 1 || height < 1)
            return E_FAIL;

        if (metadata)
        {
            metadata->width = static_cast<size_t>(width);
            metadata->height = static_cast<size_t>(height);
            metadata->depth = metadata->arraySize = metadata->mipLevels = 1;
            metadata->format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            metadata->dimension = TEX_DIMENSION_TEXTURE2D;
        }

        hr = image.Initialize2D(DXGI_FORMAT_R16G16B16A16_FLOAT,
            static_cast<size_t>(width), static_cast<size_t>(height), 1u, 1u);
        if (FAILED(hr))
            return hr;

        file.setFrameBuffer(reinterpret_cast<Imf::Rgba*>(image.GetPixels()) - dw.min.x - dw.min.y * width, 1, static_cast<size_t>(width));
        file.readPixels(dw.min.y, dw.max.y);
    }
#ifdef WIN32
    catch (const com_exception& exc)
    {
#ifdef _DEBUG
        OutputDebugStringA(exc.what());
#endif
        hr = exc.get_result();
    }
#endif
#if defined(WIN32) && defined(_DEBUG)
    catch (const std::exception& exc)
    {
        OutputDebugStringA(exc.what());
        hr = E_FAIL;
    }
#else
    catch (const std::exception&)
    {
        hr = E_FAIL;
    }
#endif
    catch (...)
    {
        hr = E_UNEXPECTED;
    }

    if (FAILED(hr))
    {
        image.Release();
    }

    return hr;
}


//-------------------------------------------------------------------------------------
// Save a EXR file to disk
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::SaveToEXRFile(const Image& image, const wchar_t* szFile)
{
    if (!szFile)
        return E_INVALIDARG;

    if (!image.pixels)
        return E_POINTER;

    if (image.width > INT32_MAX || image.height > INT32_MAX)
        return /* HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) */ static_cast<HRESULT>(0x80070032L);

    switch (image.format)
    {
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        if ((image.rowPitch % 8) > 0)
            return E_FAIL;
        break;

    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32_FLOAT:
        break;

    default:
        return /* HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) */ static_cast<HRESULT>(0x80070032L);
    }

#ifdef WIN32
    char fileName[MAX_PATH] = {};
    const int result = WideCharToMultiByte(CP_UTF8, 0, szFile, -1, fileName, MAX_PATH, nullptr, nullptr);
    if (result <= 0)
    {
        *fileName = 0;
    }
    // Create file and write header
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    ScopedHandle hFile(safe_handle(CreateFile2(
        szFile,
        GENERIC_WRITE, 0, CREATE_ALWAYS,
        nullptr)));
#else
    ScopedHandle hFile(safe_handle(CreateFileW(
        szFile,
        GENERIC_WRITE, 0,
        nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
        nullptr)));
#endif
    if (!hFile)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    auto_delete_file delonfail(hFile.get());

    OutputStream stream(hFile.get(), fileName);
#else
    std::wstring wFileName(szFile);
    std::string fileName(wFileName.cbegin(), wFileName.cend());
#endif

    HRESULT hr = S_OK;

    try
    {
        const int width = static_cast<int>(image.width);
        const int height = static_cast<int>(image.height);

#ifdef WIN32
        Imf::RgbaOutputFile file(stream, Imf::Header(width, height), Imf::WRITE_RGBA);
#else
        Imf::RgbaOutputFile file(fileName.c_str(), Imf::Header(width, height), Imf::WRITE_RGBA);
#endif

        if (image.format == DXGI_FORMAT_R16G16B16A16_FLOAT)
        {
            file.setFrameBuffer(reinterpret_cast<const Imf::Rgba*>(image.pixels), 1, image.rowPitch / 8);
            file.writePixels(height);
        }
        else
        {
            const uint64_t bytes = image.width * image.height;

            if (bytes > UINT32_MAX)
            {
                return /* HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW) */ static_cast<HRESULT>(0x80070216L);
            }

            std::unique_ptr<XMHALF4> temp(new (std::nothrow) XMHALF4[static_cast<size_t>(bytes)]);
            if (!temp)
                return E_OUTOFMEMORY;

            file.setFrameBuffer(reinterpret_cast<const Imf::Rgba*>(temp.get()), 1, image.width);

            auto sPtr = image.pixels;
            auto dPtr = temp.get();
            if (image.format == DXGI_FORMAT_R32G32B32A32_FLOAT)
            {
                for (int j = 0; j < height; ++j)
                {
                    auto srcPtr = reinterpret_cast<const XMFLOAT4*>(sPtr);
                    auto destPtr = dPtr;
                    for (int k = 0; k < width; ++k, ++srcPtr, ++destPtr)
                    {
                        const XMVECTOR v = XMLoadFloat4(srcPtr);
                        PackedVector::XMStoreHalf4(destPtr, v);
                    }

                    sPtr += image.rowPitch;
                    dPtr += width;

                    file.writePixels(1);
                }
            }
            else
            {
                assert(image.format == DXGI_FORMAT_R32G32B32_FLOAT);

                for (int j = 0; j < height; ++j)
                {
                    auto srcPtr = reinterpret_cast<const XMFLOAT3*>(sPtr);
                    auto destPtr = dPtr;
                    for (int k = 0; k < width; ++k, ++srcPtr, ++destPtr)
                    {
                        XMVECTOR v = XMLoadFloat3(srcPtr);
                        v = XMVectorSelect(g_XMIdentityR3, v, g_XMSelect1110);
                        PackedVector::XMStoreHalf4(destPtr, v);
                    }

                    sPtr += image.rowPitch;
                    dPtr += width;

                    file.writePixels(1);
                }
            }
        }
    }
#ifdef WIN32
    catch (const com_exception& exc)
    {
#ifdef _DEBUG
        OutputDebugStringA(exc.what());
#endif
        hr = exc.get_result();
    }
#endif
#if defined(WIN32) && defined(_DEBUG)
    catch (const std::exception& exc)
    {
        OutputDebugStringA(exc.what());
        hr = E_FAIL;
    }
#else
    catch (const std::exception&)
    {
        hr = E_FAIL;
    }
#endif
    catch (...)
    {
        hr = E_UNEXPECTED;
    }

    if (FAILED(hr))
        return hr;

#ifdef WIN32
    delonfail.clear();
#endif

    return S_OK;
}
