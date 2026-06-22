//--------------------------------------------------------------------------------------
// ScreenCoordinates.cpp
//
// Advanced Technology Group ( ATG )
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "ScreenCoordinates.h"


//--------------------------------------------------------------------------------------
// Name: Default contructor
// Desc: Set default initial values
//--------------------------------------------------------------------------------------
ScreenCoordinates::ScreenCoordinates(const D3D12_RECT& area, float scrollBarHeight, unsigned numRows, unsigned numCols)
    :m_area(area),
    m_scrollBarHeight(scrollBarHeight),
    m_numRows(numRows),
    m_numCols(numCols)
{
}


//--------------------------------------------------------------------------------------
// Name: GetViewportForOverallProgressBar
// Desc: Returns the viewport to be used to render the whole package progress bar
//--------------------------------------------------------------------------------------
D3D12_VIEWPORT ScreenCoordinates::GetViewportForOverallProgressBar() const
{
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = static_cast< float >( m_area.left );
    viewport.TopLeftY = m_area.bottom - m_scrollBarHeight;
    viewport.Width    = static_cast< float >( m_area.right - m_area.left );
    viewport.Height   = m_scrollBarHeight;

    return viewport;
}


//--------------------------------------------------------------------------------------
// Name: GetViewportForChunkMode
// Desc: Returns the viewport to be used to display the chunk color when in chunk mode
//--------------------------------------------------------------------------------------
D3D12_VIEWPORT ScreenCoordinates::GetViewportForChunkMode() const
{
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftY = m_area.top + m_scrollBarHeight * 2;
    viewport.Height   = m_area.bottom - viewport.TopLeftY - m_scrollBarHeight * 2;
    viewport.Width    = viewport.Height * 1.66f;
    viewport.TopLeftX = m_area.left + ( m_area.right - m_area.left - viewport.Width ) / 2.0f;

    return viewport;
}


//--------------------------------------------------------------------------------------
// Name: GetViewportForChunkThumbnail
// Desc: Returns the viewport to be used to display a specific chunk color thumbnail on 
//       the menu
//--------------------------------------------------------------------------------------
D3D12_VIEWPORT ScreenCoordinates::GetViewportForChunkThumbnail( unsigned row, unsigned col ) const
{
    D3D12_VIEWPORT viewport = GetViewportForChunkInfo( row, col );
    D3D12_VIEWPORT viewport2 = {};
    viewport2.Width  = viewport.Width * 0.66f;
    viewport2.Height = viewport.Height - m_scrollBarHeight * 6;
    viewport2.TopLeftX = viewport.TopLeftX + ( viewport.Width - viewport2.Width ) / 2;
    viewport2.TopLeftY = viewport.TopLeftY + m_scrollBarHeight * 2;

    return viewport2;
}


//--------------------------------------------------------------------------------------
// Name: GetViewportForChunkThumbnailHighlight
// Desc: Returns the viewport to be used to display a highlight around a specific chunk 
//       color thumbnail on the menu
//--------------------------------------------------------------------------------------
D3D12_VIEWPORT ScreenCoordinates::GetViewportForChunkThumbnailHighlight( unsigned row, unsigned col ) const
{
    float frame = m_scrollBarHeight / 3;
    D3D12_VIEWPORT viewport = GetViewportForChunkThumbnail( row, col );
    viewport.TopLeftX -= frame;
    viewport.TopLeftY -= frame;              
    viewport.Width    += frame * 2;
    viewport.Height   += frame * 2;

    return viewport;
}


//--------------------------------------------------------------------------------------
// Name: GetViewportForChunkThumbnailHighlight
// Desc: Returns the viewport to be used to display a progress bar for a specific chunk 
//       color thumbnail on the menu
//--------------------------------------------------------------------------------------
D3D12_VIEWPORT ScreenCoordinates::GetViewportForChunkProgressBar( unsigned row, unsigned col ) const
{
    D3D12_VIEWPORT viewport = GetViewportForChunkInfo( row, col );
    D3D12_VIEWPORT viewport2 = {};
        
    viewport2.Width  = viewport.Width * 0.88f;
    viewport2.Height = m_scrollBarHeight;
    viewport2.TopLeftX = viewport.TopLeftX + ( viewport.Width - viewport2.Width ) / 2;
    viewport2.TopLeftY = viewport.TopLeftY + viewport.Height - m_scrollBarHeight * 2.5f;

    return viewport2;
}

//--------------------------------------------------------------------------------------
// Name: GetViewportForChunkName
// Desc: Returns the viewport to be used to display the name for a specific chunk 
//       thumbnail on the menu
//--------------------------------------------------------------------------------------
D3D12_VIEWPORT ScreenCoordinates::GetViewportForChunkName(unsigned row, unsigned col) const
{
    D3D12_VIEWPORT viewport = GetViewportForChunkInfo(row, col);
    D3D12_VIEWPORT viewport2 = {};

	viewport2.Width = viewport.Width * 5;
	viewport2.Height = viewport.Height / 3;
	viewport2.TopLeftX = viewport.TopLeftX + (viewport.Width - viewport2.Width) / 2;
	viewport2.TopLeftY = viewport.TopLeftY + m_scrollBarHeight * 3;

	return viewport2;
}

//--------------------------------------------------------------------------------------
// Name: GetViewportForChunkThumbnailHighlight
// Desc: Returns the viewport to be used to display the chunk thumbnail, progress bar 
//       and highlight.
//--------------------------------------------------------------------------------------
D3D12_VIEWPORT ScreenCoordinates::GetViewportForChunkInfo( unsigned row, unsigned col ) const
{
    D3D12_VIEWPORT viewport = GetViewportForChunkMode();
    viewport.Width    /= m_numCols;
    viewport.Height   /= m_numRows;
    viewport.TopLeftX += viewport.Width * col;
    viewport.TopLeftY += viewport.Height * row;

    return viewport;
}