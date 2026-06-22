//--------------------------------------------------------------------------------------
// BaseFileInterface.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "File.h"

// all pathnames for files do not include the mount point
class BaseFileInterface
{
public:
    struct PackedFileEntry
    {
        uint64_t nameHash;
        uint64_t startLocation;
        uint64_t sizeOnDisk;
        uint64_t sizeInMemory;
        union
        {
            uint64_t flags;
        };
        PackedFileEntry(uint64_t newNameHash = 0, uint64_t newLocation = 0, uint64_t newSizeOnDisk = 0, uint64_t newSizeInMemory = 0) :
            nameHash(newNameHash)
            , startLocation(newLocation)
            , sizeOnDisk(newSizeOnDisk)
            , sizeInMemory(newSizeInMemory ? newSizeInMemory : newSizeOnDisk)
            , flags(0)
        {}

        void WriteEntry(uint64_t* address)
        {
            *address++ = nameHash;
            *address++ = startLocation;
            *address++ = sizeOnDisk;
            *address++ = sizeInMemory;
            *address++ = flags;
        }
    };

public:
    virtual ~BaseFileInterface() {}
    static std::filesystem::path NormalizeFilename(const std::filesystem::path& fullName)
    {
        std::wstring toret(fullName.wstring());
        std::for_each(toret.begin(), toret.end(), [](wchar_t& letter) {letter = towlower(letter); });
        std::for_each(toret.begin(), toret.end(), [](wchar_t& letter) {if (letter == L'/') letter = L'\\'; });
        return std::filesystem::path(toret);
    }

    static std::string NormalizeFilename(const std::string& fullName)
    {
        std::string toret(fullName);
        std::for_each(toret.begin(), toret.end(), [](char& letter) {letter = static_cast<char> (tolower(letter)); });
        std::for_each(toret.begin(), toret.end(), [](char& letter) {if (letter == '/') letter = '\\'; });
        return toret;
    }

    static std::wstring NormalizeFilename(const std::wstring& fullName)
    {
        std::wstring toret(fullName);
        std::for_each(toret.begin(), toret.end(), [](wchar_t& letter) {letter = towlower(letter); });
        std::for_each(toret.begin(), toret.end(), [](wchar_t& letter) {if (letter == L'/') letter = L'\\'; });
        return toret;
    }

    static uint64_t GenerateFilenameHash(const std::filesystem::path& fullName)
    {
        std::hash<std::wstring> hash_fn;
        return hash_fn(NormalizeFilename(fullName).wstring());
    }

    static uint64_t GenerateFilenameHash(const std::wstring& fullName)
    {
        std::hash<std::wstring> hash_fn;
        return hash_fn(NormalizeFilename(fullName));
    }

    static uint64_t GenerateFilenameHash(const std::string& fullName)
    {
        using convert_type = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_type, wchar_t> converter;

        std::wstring temp = converter.from_bytes(fullName);

        std::hash<std::wstring> hash_fn;
        return hash_fn(NormalizeFilename(temp));
    }

    static bool ReadFile(const std::wstring& fileName, std::string& fileContents)
    {
        FILE* input;
        _wfopen_s(&input, fileName.c_str(), L"r");
        if (!input)
            return false;
        fseek(input, 0, SEEK_END);
        int32_t len = ftell(input);
        fseek(input, 0, SEEK_SET);

        {
            std::unique_ptr<char> tempBuffer(new char[len + 1U]);
            char* inputBuffer = tempBuffer.get();
            memset(tempBuffer.get(), 0, len + 1U);
            while (!feof(input))
            {
                *inputBuffer++ = static_cast<char> (getc(input));
            }
            *(inputBuffer - 1) = 0;     // terminator at end of file
            fileContents = tempBuffer.get();
        }

        fclose(input);
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    /// \brief AppendPath
    /// \details ChunkPGO::AppendPath
    /// \details Append the filename to a path that has already been normalized
    ///          Handles . at the start of a path and slashes at the end
    /// \return The new appended filename and path
    /// \param sourceName The name of the file in question, can also include a partial path at the start
    /// \param path The path to append at the end of sourceName
    //////////////////////////////////////////////////////////////////////////
    static std::string AppendPath(const std::string& sourceName, const std::string& path)
    {
        if (path.size() == 0)
            return sourceName;
        std::string toret(sourceName);
        while (toret.size() && (toret[0] == '.')) { toret = toret.substr(1); }
        if (toret.size() != 0)
        {
            if (toret[toret.size() - 1] != '\\')
                toret += "\\";
        }
        if (path[0] == '.')
            toret += path.substr(1);
        else
            toret += path;
        if (toret.size() && (toret[toret.size() - 1] != '\\')) { toret += "\\"; }
        return toret;
    }

    //////////////////////////////////////////////////////////////////////////
    /// \brief AppendPath
    /// \details ChunkPGO::AppendPath
    /// \details Append the filename to a path that has already been normalized
    ///          Handles . at the start of a path and slashes at the end
    /// \return The new appended filename and path
    /// \param sourceName The name of the file in question, can also include a partial path at the start
    /// \param path The path to append at the end of sourceName
    //////////////////////////////////////////////////////////////////////////
    static std::wstring AppendPath(const std::wstring& sourceName, const std::wstring& path)
    {
        if (path.size() == 0)
            return sourceName;
        std::wstring toret(sourceName);
        while (toret.size() && (toret[0] == L'.')) { toret = toret.substr(1); }
        if (toret.size() != 0)
        {
            if (toret[toret.size() - 1] != L'\\')
                toret += L"\\";
        }
        if (path[0] == L'.')
            toret += path.substr(1);
        else
            toret += path;
        if (toret.size() && (toret[toret.size() - 1] != L'\\')) { toret += L"\\"; }
        return toret;
    }

    typedef uint64_t FileHandle;		// really just the name hash
    static const FileHandle s_invalidFileHandle = UINT64_MAX;

    virtual void OpenPackFile(const std::wstring& /*fileName*/, uint32_t /*creationFlags*/, uintptr_t& /*platformHandle*/, BaseFileInterface* /*fileInterface*/) {}
    virtual void ClosePackFile() {}

    virtual void SetDirectoryOffset(const std::wstring& /*dirName*/) {}		// This directory is added as a prefix to all OpenFile calls

    virtual FileHandle OpenFile(const std::wstring& fileName, uint32_t creationDisposition, uint32_t flags, uintptr_t& platformHandle) = 0;
    virtual void CloseFile(FileHandle file) = 0;

    virtual uint64_t LookupFileByName(const std::wstring& rootFile, uint64_t /*offset*/, uint64_t /*size*/) { return GenerateFilenameHash(rootFile); }
    virtual uint64_t LookupFileByHash(uint64_t rootFile, uint64_t /*offset*/, uint64_t /*size*/) { return rootFile; }

    virtual size_t SetReadPointer(FileHandle file, uint64_t newLocation) = 0;
    virtual HRESULT ReadFile(FileHandle file, void* data, uint32_t size) = 0;
    virtual HRESULT AsyncRead(FileHandle file, void* buffer, uint32_t bytesToRead, uint64_t readLocation, uintptr_t userData, FileSystem::File::AsyncRequestId& requestId, FileSystem::File::AsyncCallback callback, bool useEvent) = 0;
};
