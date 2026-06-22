// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

#include <vector>

struct GameSaveBlobMetadata
{
    GameSaveBlobMetadata( const char* blobName = nullptr, uint32_t blobSize = 0 )
       : m_blobName(blobName),
         m_blobSize(blobSize)
    {
    }

    std::string   m_blobName;
    uint32_t      m_blobSize;
};

struct GameSaveContainerMetadata
{
   GameSaveContainerMetadata( const std::string& containerName, const std::string& containerDisplayName ) :
      m_containerDisplayName( containerDisplayName ),
      m_containerName( containerName )
   {
      ResetData();
   }

    GameSaveContainerMetadata(const char* containerName, const char* containerDisplayName = nullptr) :
        m_containerDisplayName(containerDisplayName),
        m_containerName(containerName)
    {
        ResetData();
    }

    void ResetData()
    {
        m_blobs.clear();
        m_changedSinceLastSync = false;
        m_isGameDataOnDisk = false;
        m_lastModified = 0;
        m_needsSync = false;
        m_totalSize = 0;
    }

    std::vector<GameSaveBlobMetadata>   m_blobs;
    bool                                m_changedSinceLastSync;
    std::string                         m_containerDisplayName;
    std::string                         m_containerName;
    bool                                m_isGameDataOnDisk;
    time_t                              m_lastModified;
    bool                                m_needsSync;
    uint64_t                            m_totalSize;
};
