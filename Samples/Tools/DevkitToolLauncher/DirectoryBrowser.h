//--------------------------------------------------------------------------------------
// DirectoryBrowser.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace ATG
{

    class Directory;
    class DirectoryBrowser;

    // Represents a single file
    class File
    {
    public:

        File(size_t id, std::weak_ptr<Directory> parentDirectory, const std::string& fileName);
        File();

        std::string GetName() const { return m_name; }
        std::string GetFullPath() const;
        std::weak_ptr<Directory> GetParent() const { return m_parent; }
        int GetDepth() const;

        bool operator<(const File& other)
        {
            return m_name < other.m_name;
        }
        bool operator==(const File& other)
        {
            return m_name == other.m_name &&
                m_parent.lock() == other.m_parent.lock();
        }

        size_t GetId() const { return m_id; }

    protected:

        size_t                      m_id;

        std::weak_ptr<Directory>    m_parent;
        std::string                 m_name;
    };

    // Represents a single directory
    class Directory : public std::enable_shared_from_this<Directory>
    {
    public:

        Directory(size_t id, std::weak_ptr<Directory> parentDirectory, const std::string& directoryName);
        Directory();

        std::string GetName() const { return m_name; }
        std::string GetFullPath() const;
        std::weak_ptr<Directory> GetParent() const { return m_parent; }
        std::vector<std::shared_ptr<Directory>> GetChildrenDirectories() const { return m_dirs; }
        std::vector<std::shared_ptr<File>> GetChildrenFiles() const { return m_files; }
        bool HasChildren() const { return m_dirs.size() > 0 || m_files.size() > 0; }
        int GetDepth() const;

        void AddDirectory(std::shared_ptr<Directory> dir);
        void AddFile(std::shared_ptr<File> file);
        void RemoveDirectory(std::shared_ptr<Directory> dir);
        void RemoveFile(std::shared_ptr<File> file);

        bool GetExpanded() const { return m_expanded; }

        bool operator<(const Directory& other)
        {
            return m_name < other.m_name;
        }
        bool operator==(const Directory& other)
        {
            return m_name == other.m_name &&
                m_parent.lock() == other.m_parent.lock();
        }

        size_t GetId() const { return m_id; }

    protected:

        friend class DirectoryBrowser;
        void SetExpanded(bool expanded) { m_expanded = expanded; }

    protected:

        size_t                                  m_id;

        std::vector<std::shared_ptr<Directory>> m_dirs;
        std::vector<std::shared_ptr<File>>      m_files;
        std::weak_ptr<Directory>                m_parent;
        std::string                             m_name;

        bool                                    m_expanded;
    };

    // Represents a browser over a path
    class DirectoryBrowser
    {
    public:

        // Iteration info that's stored in a flattened list based on expanded/collapsed states.
        // Only either the directory or file should be valid at a time.
        struct IteratorInfo
        {
            IteratorInfo() {}
            IteratorInfo(std::shared_ptr<Directory> directory)
                : m_directory(directory)
            {
            }
            IteratorInfo(std::shared_ptr<File> file)
                : m_file(file)
            {
            }

            std::shared_ptr<Directory>  m_directory;
            std::shared_ptr<File>       m_file;
        };

        // An iterator to go through files/folders in alphabetical order. Respects expanded/collapsed state of directories.
        // The iterator will go through directories before going through files.
        // Iterator behavior is undefined if the DirectoryBrowser changes.
        class Iterator
        {
        public:

            Iterator(DirectoryBrowser* directoryBrowser, int iteratorIndex, int iteratorVersion);
            Iterator();
            Iterator(const Iterator& other);

            std::shared_ptr<Directory> GetDirectory() const;
            std::shared_ptr<File> GetFile() const;
            bool IsDirectory() const;
            bool IsFile() const;
            bool IsValid() const;
            int GetIndex() const { return m_iteratorIndex; }
            int GetDepth() const;
            std::string GetName() const;

            // Only call these on iterators representing directories.
            // This call expands/collapses the referenced iterator. The expansion/collapsing normally invalidates iterators.
            // However, using this method will update the iterator to be valid after the call, allowing it to still be used
            // as a sort-of selection marker.
            void SetDirectoryExpanded(bool expanded);
            bool GetDirectoryExpanded();

            // other operators
            operator bool() const { return IsValid(); }
            Iterator& operator+=(int num);
            Iterator& operator-=(int num);
            Iterator& operator=(const Iterator& other);
            bool operator==(const Iterator& other);
            bool operator!=(const Iterator& other) { return !operator==(other); }

            // prefix operators
            Iterator& operator++() { return operator+=(1); }
            Iterator& operator--() { return operator-=(1); }

            // postfix operators
            Iterator operator++(int) { Iterator copy(*this); operator++(); return copy; }
            Iterator operator--(int) { Iterator copy(*this); operator--(); return copy; }

        protected:

            friend class DirectoryBrowser;

            DirectoryBrowser*   m_directoryBrowser;
            int                 m_iteratorIndex;
            int                 m_iteratorVersion;
        };
        friend class Iterator;

    public:

        // Sets up the directoryBrowser to browser the specified directory.
        // extensionFilter can be set to a specific file extension (ex: ".exe") to only
        // show files with that extension.
        DirectoryBrowser(const char* rootDir);
        DirectoryBrowser(const char* rootDir, const char* extensionFilter);
        DirectoryBrowser();

        // Call to get an iterator for going through the entire hierarchy order as if listed in a browser.
        // Use IsValid() on the returned iterator to know when it past an end.
        Iterator GetIteratorToFirst();

        // Gets an iterator to the last element. However, not that the iterator is a normal iterator, so you must use
        // a decrement operator until the iterator becomes invalid.
        Iterator GetIteratorToLast();

        // Returns the number of elements that would be iterated with an iterator. This number represents the flattened
        // list of expanded directories and files, NOT the total files and directories contained in the class.
        // This number can change when directories are expanded/collapsed.
        size_t GetNumIteratedElements() const { return m_flattenedIteratorInfo.size(); }

        std::shared_ptr<Directory> GetRootDirectory() const { return m_rootDir; }

        // Scans the root directory for all files and folders.
        // The current implementation will completely reset all data, so all iterators will be invalidated and all states will
        // become collapsed.
        void Scan();

    protected:

        // Creates a new flattened iterator info list. Old iterators are invalidated via version bump.
        void UpdateFlattenedIteratorInfo();

        // Expands the specified directory referenced by the iterator to be expanded/closed. If this results in an update,
        // The flattened info must be updated. The passed-in iterator is updated to still be valid with the update.
        void SetDirectoryExpanded(Iterator& iter, bool expanded);

        // Directory/file ids
        size_t GetNextId() const { return m_currentId++; }

        // Finds a directory or file by the specified id.
        void FindById(size_t id, std::shared_ptr<Directory>& outFoundDir, std::shared_ptr<File>& outFoundFile) const;

    protected:

        std::string                 m_rootDirStr;
        std::string                 m_filterExtension;

        std::shared_ptr<Directory>  m_rootDir;

        // Flattened browser cache for iterators
        std::vector<IteratorInfo>   m_flattenedIteratorInfo;
        int                         m_iteratorVersion;

        mutable size_t              m_currentId;
    };

}
