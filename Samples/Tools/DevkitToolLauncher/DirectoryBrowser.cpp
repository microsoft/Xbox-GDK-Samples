//--------------------------------------------------------------------------------------
// DirectoryBrowser.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "DirectoryBrowser.h"

using namespace ATG;

File::File(size_t id, std::weak_ptr<Directory> parentDirectory, const std::string& fileName)
    : m_id(id)
    , m_parent(parentDirectory)
    , m_name(fileName)
{
}

File::File()
    : m_id(0)
{
};

std::string File::GetFullPath() const
{
    std::vector<std::shared_ptr<Directory>> parents;
    std::weak_ptr<Directory> currentParent = m_parent;
    while (true)
    {
        auto sp = currentParent.lock();
        if (sp)
        {
            parents.push_back(sp);
            currentParent = sp->GetParent();
        }
        else
        {
            break;
        }
    }

    if (parents.empty())
    {
        return GetName();
    }

    // Calculate path using parent names
    std::string path;
    auto rIter = parents.rbegin();
    while (rIter != parents.rend())
    {
        path += rIter->get()->GetName();
        path += "\\";

        ++rIter;
    }
    path += GetName();
    return path;
}

int File::GetDepth() const
{
    int depth = 0;
    std::weak_ptr<Directory> currentParent = m_parent;
    while (true)
    {
        auto sp = currentParent.lock();
        if (sp)
        {
            ++depth;
            currentParent = sp->GetParent();
        }
        else
        {
            break;
        }
    }
    return depth;
}

Directory::Directory(size_t id, std::weak_ptr<Directory> parentDirectory, const std::string& directoryName)
    : m_id(id)
    , m_parent(parentDirectory)
    , m_name(directoryName)
    , m_expanded(false)
{
}

Directory::Directory()
    : m_id(0)
    , m_expanded(false)
{
}

std::string Directory::GetFullPath() const
{
    std::vector<std::shared_ptr<Directory>> parents;
    std::weak_ptr<Directory> currentParent = m_parent;
    while (true)
    {
        auto sp = currentParent.lock();
        if (sp)
        {
            parents.push_back(sp);
            currentParent = sp->GetParent();
        }
        else
        {
            break;
        }
    }

    if (parents.empty())
    {
        return GetName();
    }

    // Calculate path using parent names
    std::string path;
    auto rIter = parents.rbegin();
    while (rIter != parents.rend())
    {
        path += rIter->get()->GetName();
        path += "\\";

        ++rIter;
    }
    path += GetName();
    return path;
}

int Directory::GetDepth() const
{
    int depth = 0;
    std::weak_ptr<Directory> currentParent = m_parent;
    while (true)
    {
        auto sp = currentParent.lock();
        if (sp)
        {
            ++depth;
            currentParent = sp->GetParent();
        }
        else
        {
            break;
        }
    }
    return depth;
}

void Directory::AddDirectory(std::shared_ptr<Directory> dir)
{
    m_dirs.push_back(dir);
}

void Directory::AddFile(std::shared_ptr<File> file)
{
    m_files.push_back(file);
}

void Directory::RemoveDirectory(std::shared_ptr<Directory> dir)
{
    auto iter = std::find(m_dirs.begin(), m_dirs.end(), dir);
    if (iter != m_dirs.end())
    {
        m_dirs.erase(iter);
    }
}

void Directory::RemoveFile(std::shared_ptr<File> file)
{
    auto iter = std::find(m_files.begin(), m_files.end(), file);
    if (iter != m_files.end())
    {
        m_files.erase(iter);
    }
}

DirectoryBrowser::Iterator::Iterator(DirectoryBrowser* directoryBrowser, int iteratorIndex, int iteratorVersion)
    : m_directoryBrowser(directoryBrowser)
    , m_iteratorIndex(iteratorIndex)
    , m_iteratorVersion(iteratorVersion)
{
}

DirectoryBrowser::Iterator::Iterator()
    : m_directoryBrowser(nullptr)
    , m_iteratorIndex(0)
    , m_iteratorVersion(0)
{
}

DirectoryBrowser::Iterator::Iterator(const Iterator& other)
    : m_directoryBrowser(other.m_directoryBrowser)
    , m_iteratorIndex(other.m_iteratorIndex)
    , m_iteratorVersion(other.m_iteratorVersion)
{
}

std::shared_ptr<Directory> DirectoryBrowser::Iterator::GetDirectory() const
{
    if (!IsValid())
    {
        return {};
    }

    return m_directoryBrowser->m_flattenedIteratorInfo[size_t(m_iteratorIndex)].m_directory;
}

std::shared_ptr<File> DirectoryBrowser::Iterator::GetFile() const
{
    if (!IsValid())
    {
        return {};
    }

    return m_directoryBrowser->m_flattenedIteratorInfo[size_t(m_iteratorIndex)].m_file;
}

bool DirectoryBrowser::Iterator::IsDirectory() const
{
    // Doesn't auto-convert to bool for some reason
    return GetDirectory().operator bool();
}

bool DirectoryBrowser::Iterator::IsFile() const
{
    // Doesn't auto-convert to bool for some reason
    return GetFile().operator bool();
}

bool DirectoryBrowser::Iterator::IsValid() const
{
    return m_directoryBrowser &&
        m_directoryBrowser->m_iteratorVersion == m_iteratorVersion &&
        m_iteratorIndex >= 0 &&
        size_t(m_iteratorIndex) < m_directoryBrowser->m_flattenedIteratorInfo.size();
}

int DirectoryBrowser::Iterator::GetDepth() const
{
    if (IsDirectory())
    {
        return GetDirectory()->GetDepth();
    }
    else if (IsFile())
    {
        return GetFile()->GetDepth();
    }
    else
    {
        return 0;
    }
}

std::string DirectoryBrowser::Iterator::GetName() const
{
    if (IsDirectory())
    {
        return GetDirectory()->GetName();
    }
    else if (IsFile())
    {
        return GetFile()->GetName();
    }
    else
    {
        return "";
    }
}

void DirectoryBrowser::Iterator::SetDirectoryExpanded(bool expanded)
{
    if (!IsDirectory())
    {
        throw std::logic_error("Can't call SetDirectoryExpanded on an iterator that is not representing a directory.");
    }

    return m_directoryBrowser->SetDirectoryExpanded(*this, expanded);
}

bool DirectoryBrowser::Iterator::GetDirectoryExpanded()
{
    if (!IsDirectory())
    {
        throw std::logic_error("Can't call GetDirectoryExpanded on an iterator that is not representing a directory.");
    }

    return GetDirectory()->GetExpanded();
}

DirectoryBrowser::Iterator& DirectoryBrowser::Iterator::operator+=(int num)
{
    if (!m_directoryBrowser ||
        m_directoryBrowser->m_iteratorVersion != m_iteratorVersion)
    {
        throw std::logic_error("Using invalidated iterator");
    }

    int newIteratorIndex = m_iteratorIndex + num;
    if (newIteratorIndex == 0)
    {
        m_iteratorIndex = 0;
    }
    else if (newIteratorIndex < -1 || size_t(newIteratorIndex) > m_directoryBrowser->m_flattenedIteratorInfo.size())
    {
        throw std::out_of_range("Iterator increment operation goes out-of-range");
    }
    else
    {
        // Iterator range is [-1, size] to represent invalid ranges 1 past beginning or end
        m_iteratorIndex = newIteratorIndex;
    }

    return *this;
}

DirectoryBrowser::Iterator& DirectoryBrowser::Iterator::operator-=(int num)
{
    return operator+=(-num);
}

DirectoryBrowser::Iterator& DirectoryBrowser::Iterator::operator=(const Iterator& other)
{
    if (&other == this)
    {
        return *this;
    }

    m_directoryBrowser = other.m_directoryBrowser;
    m_iteratorIndex = other.m_iteratorIndex;
    m_iteratorVersion = other.m_iteratorVersion;
    return *this;
}

bool DirectoryBrowser::Iterator::operator==(const Iterator& other)
{
    return m_directoryBrowser == other.m_directoryBrowser &&
        m_iteratorIndex == other.m_iteratorIndex &&
        m_iteratorVersion == other.m_iteratorVersion;
}

static void FixupRootDirString(std::string& str)
{
    while (true)
    {
        auto rIter = str.rbegin();
        if (rIter == str.rend())
        {
            break;
        }

        if ((*rIter) == '\\' ||
            (*rIter) == '/')
        {
            str.erase(std::next(rIter).base());
        }
        else
        {
            break;
        }
    }

    auto found = (str.find(':') != std::string::npos);
    if (!found)
    {
        str += ":";
    }
}

DirectoryBrowser::DirectoryBrowser(const char* rootDir)
    : m_rootDirStr(rootDir)
    , m_iteratorVersion(0)
    , m_currentId(0)
{
    FixupRootDirString(m_rootDirStr);

    Scan();
}

DirectoryBrowser::DirectoryBrowser(const char* rootDir, const char* extensionFilter)
    : m_rootDirStr(rootDir)
    , m_filterExtension(extensionFilter)
    , m_iteratorVersion(0)
    , m_currentId(0)
{
    FixupRootDirString(m_rootDirStr);
    if (m_filterExtension.size() > 0 &&
        m_filterExtension[0] != '.')
    {
        m_filterExtension = "." + m_filterExtension;
    }

    Scan();
}

DirectoryBrowser::DirectoryBrowser()
    : m_iteratorVersion(0)
    , m_currentId(0)
{
}

DirectoryBrowser::Iterator DirectoryBrowser::GetIteratorToFirst()
{
    return Iterator(this, 0, m_iteratorVersion);
}

DirectoryBrowser::Iterator DirectoryBrowser::GetIteratorToLast()
{
    return Iterator(this, int(m_flattenedIteratorInfo.size()) - 1, m_iteratorVersion);
}

static void ScanDirectory(std::shared_ptr<Directory> directory, std::string& filterExtension, std::function<size_t()> getNextIdFn)
{
    if (!directory)
    {
        return;
    }

    // Iterate all files and folders in path
    std::string path = directory->GetFullPath();
    std::vector<std::string> files;
    std::vector<std::string> directories;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path))
    {
        if (entry.is_directory())
        {
            directories.push_back(entry.path().filename().string());
        }
        else if(filterExtension.empty() || entry.path().extension().string() == filterExtension)
        {
            files.push_back(entry.path().filename().string());
        }
    }

    // Sort
    std::sort(files.begin(), files.end());
    std::sort(directories.begin(), directories.end());

    // Handle and store all directories
    for (std::string& dir : directories)
    {
        std::shared_ptr<Directory> newDirectory = std::make_shared<Directory>(getNextIdFn(), directory, dir);
        ScanDirectory(newDirectory, filterExtension, getNextIdFn);
        directory->AddDirectory(newDirectory);
    }

    // Store all files
    for (std::string& file : files)
    {
        directory->AddFile(std::make_shared<File>(getNextIdFn(), directory, file));
    }
}

void DirectoryBrowser::Scan()
{
    // Add the root directory
    m_rootDir = std::make_shared<Directory>(GetNextId(), std::weak_ptr<Directory>(), m_rootDirStr);

    // Scan it!
    ScanDirectory(m_rootDir, m_filterExtension,
        [&]()->size_t
        {
            return GetNextId();
        });

    UpdateFlattenedIteratorInfo();
}

static void IterateDirectory(std::vector<DirectoryBrowser::IteratorInfo>& flattenedList, std::shared_ptr<Directory> directory)
{
    // Base case
    if (!directory)
    {
        return;
    }

    // Add self
    flattenedList.push_back(directory);

    if (directory->GetExpanded())
    {
        // Iterate child directories
        std::vector<std::shared_ptr<Directory>> childDirs = directory->GetChildrenDirectories();
        for (std::shared_ptr<Directory> childDir : childDirs)
        {
            assert(childDir);

            IterateDirectory(flattenedList, childDir);
        }

        // Then iterate all files
        std::vector<std::shared_ptr<File>> childFiles = directory->GetChildrenFiles();
        for (std::shared_ptr<File> childFile : childFiles)
        {
            assert(childFile);

            flattenedList.push_back(childFile);
        }
    }
}

void DirectoryBrowser::UpdateFlattenedIteratorInfo()
{
    m_flattenedIteratorInfo.clear();
    ++m_iteratorVersion;
    IterateDirectory(m_flattenedIteratorInfo, m_rootDir);
}

void DirectoryBrowser::SetDirectoryExpanded(Iterator& iter, bool expanded)
{
    assert(iter.IsDirectory());

    if (iter.GetDirectoryExpanded() != expanded)
    {
        iter.GetDirectory()->SetExpanded(expanded);
        UpdateFlattenedIteratorInfo();
        iter.m_iteratorVersion = m_iteratorVersion;
        // The index will always be the same for an iterator referencing a directory that expands or collapses
    }
}

static void FindByIdRecursive(std::shared_ptr<Directory> dirToSearch, size_t id, std::shared_ptr<Directory>& outFoundDir, std::shared_ptr<File>& outFoundFile)
{
    if (!dirToSearch)
    {
        return;
    }

    if (dirToSearch->GetId() == id)
    {
        outFoundDir = dirToSearch;
        return;
    }

    auto childrenFiles = dirToSearch->GetChildrenFiles();
    for (auto file : childrenFiles)
    {
        if (file->GetId() == id)
        {
            outFoundFile = file;
            return;
        }
    }

    auto childrenDirs = dirToSearch->GetChildrenDirectories();
    for (auto dir : childrenDirs)
    {
        FindByIdRecursive(dir, id, outFoundDir, outFoundFile);
        if (outFoundDir || outFoundFile)
        {
            return;
        }
    }
}

void DirectoryBrowser::FindById(size_t id, std::shared_ptr<Directory>& outFoundDir, std::shared_ptr<File>& outFoundFile) const
{
    FindByIdRecursive(m_rootDir, id, outFoundDir, outFoundFile);
}
