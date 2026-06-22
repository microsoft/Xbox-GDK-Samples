//--------------------------------------------------------------------------------------
// File: SDKMesh.cpp
//
// The SDK Mesh format (.sdkmesh) is not a recommended file format for games.  
// It was designed to meet the specific needs of the SDK samples.  Any real-world 
// applications should avoid this file format in favor of a destination format that 
// meets the specific needs of the application.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "precomp.hpp"
#include "SDKmesh.h"
#include "D3D12Util.h"

#define ENDIAN_SWAP 0

#if ENDIAN_SWAP
#define SWAP16(type)  type = (((type >> 8) & 0x00ff) | ((type << 8) & 0xff00))
#define SWAP32(type)  type = (((type >>24) & 0x000000ff) | ((type >> 8) & 0x0000ff00) | ((type << 8) & 0x00ff0000) | ((type <<24) & 0xff000000) )
#define SWAP64(type)  type = (type>>56) | ((type<<40) & 0x00FF000000000000) | ((type<<24) & 0x0000FF0000000000) | ((type<<8)  & 0x000000FF00000000) | \
    ((type >> 8) & 0x00000000FF000000) | ((type >> 24) & 0x0000000000FF0000) | ((type >> 40) & 0x000000000000FF00) | (type << 56)
#else
#define SWAP16(type)
#define SWAP32(type)
#define SWAP64(type)
#endif


#ifndef SAFEDELETE
#define SAFEDELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#endif
#ifndef SAFEDELETEARRAY
#define SAFEDELETEARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFERELEASE
#define SAFERELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

//--------------------------------------------------------------------------------------
HRESULT CDXUTSDKMesh::CreateVertexBuffer(SDKMeshLoad* pLoader, SDKMESH_VERTEX_BUFFER_HEADER* pHeader, void* pVertices)
{
    HRESULT hr = S_OK;
    pHeader->DataOffset = 0;

    //Vertex Buffer
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = ( UINT )( pHeader->SizeBytes );
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    const bool IsCSP = IsCommonStatePromotionEnabled();

    hr = CreateDefaultResource(pLoader->pd3dDevice, 
                               &bufferDesc, 
                               IsCSP ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_GENERIC_READ, 
                               (void**)&pHeader->pVB);

    if (!IsCSP)
    {
        ResourceBarrier(pLoader->pCmdList, pHeader->pVB, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    }
    pLoader->pUploadHeap->CopyBufferDataToDefaultBuffer((const BYTE*)pVertices, pHeader->SizeBytes, pLoader->pCmdList, pHeader->pVB);
    if (!IsCSP)
    {
        ResourceBarrier(pLoader->pCmdList, pHeader->pVB, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    return hr;
}


//--------------------------------------------------------------------------------------
HRESULT CDXUTSDKMesh::CreateIndexBuffer(SDKMeshLoad* pLoader, SDKMESH_INDEX_BUFFER_HEADER* pHeader, void* pIndices)
{
    HRESULT hr = S_OK;
    pHeader->DataOffset = 0;
    //Index Buffer
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = ( UINT )( pHeader->SizeBytes );
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    const bool IsCSP = IsCommonStatePromotionEnabled();

    hr = CreateDefaultResource(pLoader->pd3dDevice,
                               &bufferDesc, 
                               IsCSP ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_GENERIC_READ, 
                               (void**)&pHeader->pIB);

    if (!IsCSP)
    {
        ResourceBarrier(pLoader->pCmdList, pHeader->pIB, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    }
    pLoader->pUploadHeap->CopyBufferDataToDefaultBuffer((const BYTE*)pIndices, pHeader->SizeBytes, pLoader->pCmdList, pHeader->pIB);
    if (!IsCSP)
    {
        ResourceBarrier(pLoader->pCmdList, pHeader->pIB, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    }
   
    return hr;
}



//--------------------------------------------------------------------------------------
HRESULT CDXUTSDKMesh::CreateFromFile( SDKMeshLoad* pLoader,
                                      LPCTSTR szFileName )
{
    HRESULT hr = S_OK;

    // Open the file
    m_hFile = CreateFile2( szFileName, /*FILE_READ_DATA*/GENERIC_READ, 
                        FILE_SHARE_READ, 
                        OPEN_EXISTING, NULL );
    if( INVALID_HANDLE_VALUE == m_hFile )
    {
        fprintf(stderr, "CreateFile of %S failed. GetLastError=%d\n", szFileName, GetLastError());
        return E_FAIL;
    }

    // Change the path to just the directory
    WCHAR* pLastBSlash = wcsrchr( m_strPathW, L'\\' );
    if( pLastBSlash )
        *( pLastBSlash + 1 ) = L'\0';
    else
        *m_strPathW = L'\0';

    WideCharToMultiByte( CP_ACP, 0, m_strPathW, -1, m_strPath, MAX_PATH, NULL, FALSE );

    // Get the file size
    LARGE_INTEGER FileSize;
    GetFileSizeEx( m_hFile, &FileSize );
    UINT cBytes = FileSize.LowPart;

    // Allocate memory
    m_pStaticMeshData = new BYTE[ cBytes ];
    if( !m_pStaticMeshData )
    {
        fprintf(stderr, "allocating m_pStaticMeshData (%d bytes) failed.\n", cBytes);
        CloseHandle( m_hFile );
        return E_OUTOFMEMORY;
    }

    // Read in the file
    DWORD dwBytesRead = 0;
    if( !ReadFile( m_hFile, m_pStaticMeshData, cBytes, &dwBytesRead, NULL ) )
    {
        hr = E_FAIL;
    }

    CloseHandle( m_hFile );

    if (FAILED(hr))
    {
        fprintf(stderr, "ReadFile failed. cBytes=%d. dwBytesRead=%d. GetLastError = %d\n", cBytes, dwBytesRead, GetLastError());
    }
    else
    {
        hr = CreateFromMemory( pLoader,
                               m_pStaticMeshData,
                               cBytes,
                               false );
        if (FAILED(hr))
        {
            fprintf(stderr, "CreateFromMemory failed. hr = %08x\n", static_cast<unsigned int>(hr));
        }
    }

    if( FAILED( hr ) )
        delete []m_pStaticMeshData;

    return hr;
}

HRESULT CDXUTSDKMesh::CreateFromMemory( SDKMeshLoad* pLoader,
                                        BYTE* pData,
                                        UINT DataBytes,
                                        bool bCopyStatic)
{
    HRESULT hr = E_FAIL;

    BYTE* pBufferData = nullptr;
    UINT64 BufferDataStart = 0;

    // Set outstanding resources to zero
    m_NumOutstandingResources = 0;

    if( bCopyStatic )
    {
        SDKMESH_HEADER* pHeader = ( SDKMESH_HEADER* )pData;

        SIZE_T StaticSize = ( SIZE_T )( pHeader->HeaderSize + pHeader->NonBufferDataSize );
        m_pHeapData = new BYTE[ StaticSize ];
        if( !m_pHeapData )
            return hr;

        m_pStaticMeshData = m_pHeapData;

        CopyMemory( m_pStaticMeshData, pData, StaticSize );
    }
    else
    {
        m_pHeapData = pData;
        m_pStaticMeshData = pData;
    }

    // Pointer fixup
    m_pMeshHeader = ( SDKMESH_HEADER* )m_pStaticMeshData;

#ifdef _XBOX
    SwapSDKMeshHeader(m_pMeshHeader);
#endif

    m_pVertexBufferArray = ( SDKMESH_VERTEX_BUFFER_HEADER* )( m_pStaticMeshData + m_pMeshHeader->VertexStreamHeadersOffset ); 

#ifdef _XBOX
    SwapVertexBufferHeaderArray( m_pVertexBufferArray ,m_pMeshHeader->NumVertexBuffers );
#endif

    m_pIndexBufferArray = ( SDKMESH_INDEX_BUFFER_HEADER* )( m_pStaticMeshData + m_pMeshHeader->IndexStreamHeadersOffset );
    
#ifdef _XBOX
    SwapIndexBufferHeaderArray( m_pIndexBufferArray , m_pMeshHeader->NumIndexBuffers );
#endif


    m_pMeshArray = ( SDKMESH_MESH* )( m_pStaticMeshData + m_pMeshHeader->MeshDataOffset );

#ifdef _XBOX
    SwapMeshArray(  m_pMeshArray ,  m_pMeshHeader->NumMeshes );
#endif

    m_pSubsetArray = ( SDKMESH_SUBSET* )( m_pStaticMeshData + m_pMeshHeader->SubsetDataOffset );

#ifdef _XBOX
    SwapSubsetArray( m_pSubsetArray , m_pMeshHeader->NumTotalSubsets );
#endif

    m_pFrameArray = ( SDKMESH_FRAME* )( m_pStaticMeshData + m_pMeshHeader->FrameDataOffset );

#ifdef _XBOX
    SwapFrameArray( m_pFrameArray , m_pMeshHeader->NumFrames );
#endif

    m_pMaterialArray = ( SDKMESH_MATERIAL* )( m_pStaticMeshData + m_pMeshHeader->MaterialDataOffset );

#ifdef _XBOX
    SwapMaterialArray( m_pMaterialArray , m_pMeshHeader->NumMaterials );
#endif

    // Setup subsets
    for( UINT i = 0; i < m_pMeshHeader->NumMeshes; i++ )
    {
        m_pMeshArray[i].pSubsets = ( UINT* )( m_pStaticMeshData + m_pMeshArray[i].SubsetOffset );
        m_pMeshArray[i].pFrameInfluences = ( UINT* )( m_pStaticMeshData + m_pMeshArray[i].FrameInfluenceOffset );
    }

    // error condition
    if( m_pMeshHeader->Version != SDKMESH_FILE_VERSION )
    {
        hr = E_NOINTERFACE;
        goto Error;
    }

    // Setup buffer data pointer
    pBufferData = pData + m_pMeshHeader->HeaderSize + m_pMeshHeader->NonBufferDataSize;

    // Get the start of the buffer data
    BufferDataStart = m_pMeshHeader->HeaderSize + m_pMeshHeader->NonBufferDataSize;

    // Create VBs
    m_ppVertices = new BYTE*[m_pMeshHeader->NumVertexBuffers];
    for( UINT i = 0; i < m_pMeshHeader->NumVertexBuffers; i++ )
    {
        BYTE* pVertices = NULL;
        pVertices = ( BYTE* )( pBufferData + ( m_pVertexBufferArray[i].DataOffset - BufferDataStart ) );

#ifdef _XBOX
        SwapBuffer((DWORD *)pVertices, m_pVertexBufferArray[i].SizeBytes);
#endif

        if (pLoader)
        {
            CreateVertexBuffer(pLoader, &m_pVertexBufferArray[i], pVertices);
        }

        m_ppVertices[i] = pVertices;
    }

    // Create IBs
    m_ppIndices = new BYTE*[m_pMeshHeader->NumIndexBuffers];
    for( UINT i = 0; i < m_pMeshHeader->NumIndexBuffers; i++ )
    {
        BYTE* pIndices = NULL;
        pIndices = ( BYTE* )( pBufferData + ( m_pIndexBufferArray[i].DataOffset - BufferDataStart ) );

#ifdef _XBOX
        SwapBuffer((DWORD *)pIndices, m_pIndexBufferArray[i].SizeBytes);
#endif

        if (pLoader)
        {
            CreateIndexBuffer(pLoader, &m_pIndexBufferArray[i], pIndices);
        }

        m_ppIndices[i] = pIndices;
    }

    // Create a place to store our bind pose frame matrices
    m_pBindPoseFrameMatrices = new MATRIX[ m_pMeshHeader->NumFrames ];
    if( !m_pBindPoseFrameMatrices )
        goto Error;

    // Create a place to store our transformed frame matrices
    m_pTransformedFrameMatrices = new MATRIX[ m_pMeshHeader->NumFrames ];
    if( !m_pTransformedFrameMatrices )
        goto Error;
    m_pWorldPoseFrameMatrices = new MATRIX[ m_pMeshHeader->NumFrames ];
    if( !m_pWorldPoseFrameMatrices )
        goto Error;

    hr = S_OK;
Error:



    return hr;
}


//--------------------------------------------------------------------------------------
CDXUTSDKMesh::CDXUTSDKMesh() : m_NumOutstandingResources( 0 ),
                               m_bLoading( false ),
                               m_hFile( 0 ),
                               m_hFileMappingObject( 0 ),
                               m_pMeshHeader( NULL ),
                               m_pStaticMeshData( NULL ),
                               m_pHeapData( NULL ),
                               m_pAnimationData( NULL ),
                               m_pAnimationHeader( NULL ),
                               m_ppVertices( NULL ),
                               m_ppIndices( NULL ),
                               m_pBindPoseFrameMatrices( NULL ),
                               m_pTransformedFrameMatrices( NULL ),
                               m_pWorldPoseFrameMatrices( NULL )
{
}


//--------------------------------------------------------------------------------------
CDXUTSDKMesh::~CDXUTSDKMesh()
{
    Destroy();
}


//--------------------------------------------------------------------------------------
HRESULT CDXUTSDKMesh::Create(SDKMeshLoad* pLoader, LPCTSTR szFileName)
{
    return CreateFromFile( pLoader, szFileName );
}


//--------------------------------------------------------------------------------------
void CDXUTSDKMesh::Destroy()
{

    if( m_pStaticMeshData )
    {
        if( m_pMaterialArray )
        {
            for( UINT64 m = 0; m < m_pMeshHeader->NumMaterials; m++ )
            {

                //if( m_pDev12 )
                {
                    /*
                    //ID3D11Resource* pRes = NULL;
                    if( m_pMaterialArray[m].pDiffuseSRV && !IsErrorResource( m_pMaterialArray[m].pDiffuseSRV ) )
                    {
                        //m_pMaterialArray[m].pDiffuseSRV->GetResource( &pRes );
                        //SAFE_RELEASE( pRes );

                        SAFERELEASE( m_pMaterialArray[m].pDiffuseSRV );
                    }
                    if( m_pMaterialArray[m].pNormalSRV && !IsErrorResource( m_pMaterialArray[m].pNormalSRV ) )
                    {
                        //m_pMaterialArray[m].pNormalSRV->GetResource( &pRes );
                        //SAFE_RELEASE( pRes );

                        SAFERELEASE( m_pMaterialArray[m].pNormalSRV );
                    }
                    if( m_pMaterialArray[m].pSpecularSRV && !IsErrorResource( m_pMaterialArray[m].pSpecularSRV ) )
                    {
                        //m_pMaterialArray[m].pSpecularSRV->GetResource( &pRes );
                        //SAFERELEASE( pRes );

                        SAFERELEASE( m_pMaterialArray[m].pSpecularSRV );
                    }
                    */
                }
            }
        }
        if (m_pVertexBufferArray != nullptr)
        {
            for (UINT32 i = 0; i < m_pMeshHeader->NumVertexBuffers; ++i)
            {
                SAFERELEASE(m_pVertexBufferArray[i].pVB);
            }
        }
        if (m_pIndexBufferArray != nullptr)
        {
            for (UINT32 i = 0; i < m_pMeshHeader->NumIndexBuffers; ++i)
            {
                SAFERELEASE(m_pIndexBufferArray[i].pIB);
            }
        }
    }

    SAFEDELETEARRAY( m_pHeapData );
    m_pStaticMeshData = NULL;
    SAFEDELETEARRAY( m_pAnimationData );
    SAFEDELETEARRAY( m_pBindPoseFrameMatrices );
    SAFEDELETEARRAY( m_pTransformedFrameMatrices );
    SAFEDELETEARRAY( m_pWorldPoseFrameMatrices );

    SAFEDELETEARRAY( m_ppVertices );
    SAFEDELETEARRAY( m_ppIndices );

    m_pMeshHeader = NULL;
    m_pVertexBufferArray = NULL;
    m_pIndexBufferArray = NULL;
    m_pMeshArray = NULL;
    m_pSubsetArray = NULL;
    m_pFrameArray = NULL;
    m_pMaterialArray = NULL;

    m_pAnimationHeader = NULL;
    m_pAnimationFrameData = NULL;

}


//--------------------------------------------------------------------------------------
D3D_PRIMITIVE_TOPOLOGY CDXUTSDKMesh::GetPrimitiveType11( SDKMESH_PRIMITIVE_TYPE PrimType )
{
    D3D_PRIMITIVE_TOPOLOGY retType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    switch( PrimType )
    {
        case PT_TRIANGLE_LIST:
            retType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            break;
        case PT_TRIANGLE_STRIP:
            retType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            break;
        case PT_LINE_LIST:
            retType = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
            break;
        case PT_LINE_STRIP:
            retType = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
            break;
        case PT_POINT_LIST:
            retType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
            break;
        case PT_TRIANGLE_LIST_ADJ:
            retType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
            break;
        case PT_TRIANGLE_STRIP_ADJ:
            retType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
            break;
        case PT_LINE_LIST_ADJ:
            retType = D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
            break;
        case PT_LINE_STRIP_ADJ:
            retType = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
            break;
    };

    return retType;
}

//--------------------------------------------------------------------------------------
DXGI_FORMAT CDXUTSDKMesh::GetIBFormat( UINT iMesh )
{
    switch( m_pIndexBufferArray[ m_pMeshArray[ iMesh ].IndexBuffer ].IndexType )
    {
        case IT_16BIT:
            return DXGI_FORMAT_R16_UINT;
        case IT_32BIT:
            return DXGI_FORMAT_R32_UINT;
    };
    return DXGI_FORMAT_R16_UINT;
}

//--------------------------------------------------------------------------------------
ID3D12Resource* CDXUTSDKMesh::GetVB( UINT iMesh, UINT iVB )
{
    return m_pVertexBufferArray[ m_pMeshArray[ iMesh ].VertexBuffers[iVB] ].pVB;
}

//--------------------------------------------------------------------------------------
ID3D12Resource* CDXUTSDKMesh::GetIB(UINT iMesh)
{
    return m_pIndexBufferArray[ m_pMeshArray[ iMesh ].IndexBuffer ].pIB;
}
SDKMESH_INDEX_TYPE CDXUTSDKMesh::GetIndexType( UINT iMesh ) 
{
    return ( SDKMESH_INDEX_TYPE ) m_pIndexBufferArray[m_pMeshArray[ iMesh ].IndexBuffer].IndexType;
}

//--------------------------------------------------------------------------------------
char* CDXUTSDKMesh::GetMeshPathA()
{
    return m_strPath;
}

//--------------------------------------------------------------------------------------
WCHAR* CDXUTSDKMesh::GetMeshPathW()
{
    return m_strPathW;
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetNumMeshes()
{
    if( !m_pMeshHeader )
        return 0;
    return m_pMeshHeader->NumMeshes;
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetNumMaterials()
{
    if( !m_pMeshHeader )
        return 0;
    return m_pMeshHeader->NumMaterials;
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetNumVBs()
{
    if( !m_pMeshHeader )
        return 0;
    return m_pMeshHeader->NumVertexBuffers;
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetNumIBs()
{
    if( !m_pMeshHeader )
        return 0;
    return m_pMeshHeader->NumIndexBuffers;
}

//--------------------------------------------------------------------------------------
ID3D12Resource* CDXUTSDKMesh::GetVBAt(UINT iVB)
{
    return m_pVertexBufferArray[ iVB ].pVB;
}

//--------------------------------------------------------------------------------------
ID3D12Resource* CDXUTSDKMesh::GetIBAt(UINT iIB)
{
    return m_pIndexBufferArray[ iIB ].pIB;
}


//--------------------------------------------------------------------------------------
BYTE* CDXUTSDKMesh::GetRawVerticesAt( UINT iVB )
{
    return m_ppVertices[iVB];
}

//--------------------------------------------------------------------------------------
BYTE* CDXUTSDKMesh::GetRawIndicesAt( UINT iIB )
{
    return m_ppIndices[iIB];
}

//--------------------------------------------------------------------------------------
SDKMESH_MATERIAL* CDXUTSDKMesh::GetMaterial( UINT iMaterial )
{
    return &m_pMaterialArray[ iMaterial ];
}

//--------------------------------------------------------------------------------------
SDKMESH_MESH* CDXUTSDKMesh::GetMesh( UINT iMesh )
{
    return &m_pMeshArray[ iMesh ];
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetNumSubsets( UINT iMesh )
{
    return m_pMeshArray[ iMesh ].NumSubsets;
}

//--------------------------------------------------------------------------------------
SDKMESH_SUBSET* CDXUTSDKMesh::GetSubset( UINT iMesh, UINT iSubset )
{
    return &m_pSubsetArray[ m_pMeshArray[ iMesh ].pSubsets[iSubset] ];
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetVertexStride( UINT iMesh, UINT iVB )
{
    return ( UINT )m_pVertexBufferArray[ m_pMeshArray[ iMesh ].VertexBuffers[iVB] ].StrideBytes;
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetNumFrames()
{
    return m_pMeshHeader->NumFrames;
}

//--------------------------------------------------------------------------------------
SDKMESH_FRAME* CDXUTSDKMesh::GetFrame( UINT iFrame )
{
    assert( iFrame < m_pMeshHeader->NumFrames );
    return &m_pFrameArray[ iFrame ];
}

//--------------------------------------------------------------------------------------
SDKMESH_FRAME* CDXUTSDKMesh::FindFrame( char* pszName )
{
    for( UINT i = 0; i < m_pMeshHeader->NumFrames; i++ )
    {
        if( _stricmp( m_pFrameArray[i].Name, pszName ) == 0 )
        {
            return &m_pFrameArray[i];
        }
    }
    return NULL;
}

//--------------------------------------------------------------------------------------
UINT64 CDXUTSDKMesh::GetNumVertices( UINT iMesh, UINT iVB )
{
    return m_pVertexBufferArray[ m_pMeshArray[ iMesh ].VertexBuffers[iVB] ].NumVertices;
}

//--------------------------------------------------------------------------------------
UINT64 CDXUTSDKMesh::GetNumIndices( UINT iMesh )
{
    return m_pIndexBufferArray[ m_pMeshArray[ iMesh ].IndexBuffer ].NumIndices;
}

//--------------------------------------------------------------------------------------
VECTOR3 CDXUTSDKMesh::GetMeshBBoxCenter( UINT iMesh )
{
    return m_pMeshArray[iMesh].BoundingBoxCenter;
}

//--------------------------------------------------------------------------------------
VECTOR3 CDXUTSDKMesh::GetMeshBBoxExtents( UINT iMesh )
{
    return m_pMeshArray[iMesh].BoundingBoxExtents;
}


void CDXUTSDKMesh::SwapSDKMeshHeader( SDKMESH_HEADER * psdkMeshheader)
{
    SWAP32(psdkMeshheader->Version);
    SWAP64(psdkMeshheader->HeaderSize);
    SWAP64(psdkMeshheader->NonBufferDataSize);
    SWAP64(psdkMeshheader->BufferDataSize);
    SWAP32(psdkMeshheader->NumVertexBuffers);
    SWAP32(psdkMeshheader->NumIndexBuffers);
    SWAP32(psdkMeshheader->NumMeshes);
    SWAP32(psdkMeshheader->NumTotalSubsets);
    SWAP32(psdkMeshheader->NumFrames);
    SWAP32(psdkMeshheader->NumMaterials);
    SWAP64(psdkMeshheader->VertexStreamHeadersOffset);
    SWAP64(psdkMeshheader->IndexStreamHeadersOffset);
    SWAP64(psdkMeshheader->MeshDataOffset);
    SWAP64(psdkMeshheader->SubsetDataOffset);
    SWAP64(psdkMeshheader->FrameDataOffset);
    SWAP64(psdkMeshheader->MaterialDataOffset);

}

void CDXUTSDKMesh::SwapVertexBufferHeaderArray(SDKMESH_VERTEX_BUFFER_HEADER * pVertexBufferHeaderArray , DWORD dwArrayLength)
{
    for(DWORD dwvbArrayItter = 0; dwvbArrayItter < dwArrayLength; dwvbArrayItter ++)
    {
        SWAP64(pVertexBufferHeaderArray[dwvbArrayItter].NumVertices);
        SWAP64(pVertexBufferHeaderArray[dwvbArrayItter].SizeBytes);
        SWAP64(pVertexBufferHeaderArray[dwvbArrayItter].StrideBytes);
        SWAP64(pVertexBufferHeaderArray[dwvbArrayItter].DataOffset);
    }
}


void CDXUTSDKMesh::SwapIndexBufferHeaderArray(SDKMESH_INDEX_BUFFER_HEADER * pIndexBufferHeaderArray , DWORD dwArrayLength)
{
    for(DWORD dwibArrayItter = 0; dwibArrayItter < dwArrayLength; dwibArrayItter ++)
    {
        SWAP64(pIndexBufferHeaderArray[dwibArrayItter].NumIndices);
        SWAP64(pIndexBufferHeaderArray[dwibArrayItter].SizeBytes);
        SWAP32(pIndexBufferHeaderArray[dwibArrayItter].IndexType);
        SWAP64(pIndexBufferHeaderArray[dwibArrayItter].DataOffset);
    }
}

void CDXUTSDKMesh::SwapMeshArray(SDKMESH_MESH * pMeshArray , DWORD dwArrayLength)
{
    for(DWORD dwmeshArrayItter = 0; dwmeshArrayItter < dwArrayLength; dwmeshArrayItter ++)
    {

        for(DWORD dwbItter = 0; dwbItter < MAX_VERTEX_STREAMS; dwbItter ++ )
        {
            SWAP32(pMeshArray[dwmeshArrayItter].VertexBuffers[dwbItter]);
        }

        SWAP32(pMeshArray[dwmeshArrayItter].IndexBuffer);
        SWAP32(pMeshArray[dwmeshArrayItter].NumSubsets);
        SWAP32(pMeshArray[dwmeshArrayItter].NumFrameInfluences);
        SWAP64(pMeshArray[dwmeshArrayItter].SubsetOffset);
        SWAP64(pMeshArray[dwmeshArrayItter].FrameInfluenceOffset);

    }
}

void CDXUTSDKMesh::SwapSubsetArray( SDKMESH_SUBSET * pSubsetArray , DWORD dwArrayLength )
{
    for(DWORD dwsubArrayItter = 0; dwsubArrayItter < dwArrayLength; dwsubArrayItter ++)
    {
        SWAP32(pSubsetArray[dwsubArrayItter].MaterialID);
        SWAP32(pSubsetArray[dwsubArrayItter].PrimitiveType);
        SWAP64(pSubsetArray[dwsubArrayItter].IndexStart);
        SWAP64(pSubsetArray[dwsubArrayItter].IndexCount);
        SWAP64(pSubsetArray[dwsubArrayItter].VertexStart);
        SWAP64(pSubsetArray[dwsubArrayItter].VertexCount);
    }
}

// TODO: Finish these.

void CDXUTSDKMesh::SwapFrameArray( SDKMESH_FRAME * pFrameArray , DWORD dwArrayLength ) {}

void CDXUTSDKMesh::SwapMaterialArray( SDKMESH_MATERIAL * pMaterialArray , DWORD dwArrayLength ) {}


void CDXUTSDKMesh::SwapBuffer( DWORD * pBuffer , UINT64 uiNumBytes )
{
    for(DWORD dwItter = 0; dwItter < uiNumBytes/4; dwItter++)
    {
        SWAP32(pBuffer[dwItter]);

        DWORD buff = pBuffer[dwItter];

        buff = buff;
    }
}