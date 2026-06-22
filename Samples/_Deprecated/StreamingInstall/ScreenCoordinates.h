//--------------------------------------------------------------------------------------
// ScreenCoordinates.h
//
// Advanced Technology Group ( ATG )
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

class ScreenCoordinates
{
public:
    ScreenCoordinates( const D3D12_RECT& area, float scrollBarHeight, unsigned numRows, unsigned numCols );
    ~ScreenCoordinates() {}

    D3D12_VIEWPORT GetViewportForOverallProgressBar() const;
    D3D12_VIEWPORT GetViewportForChunkMode() const;
    D3D12_VIEWPORT GetViewportForChunkThumbnail( unsigned row, unsigned col ) const;
    D3D12_VIEWPORT GetViewportForChunkThumbnailHighlight( unsigned row, unsigned col ) const;
	D3D12_VIEWPORT GetViewportForChunkProgressBar(unsigned row, unsigned col) const;
	D3D12_VIEWPORT GetViewportForChunkName( unsigned row, unsigned col ) const;

private:
    D3D12_VIEWPORT GetViewportForChunkInfo( unsigned row, unsigned col ) const;

    D3D12_RECT m_area;
    float      m_scrollBarHeight;
    unsigned   m_numRows;
    unsigned   m_numCols;
};

