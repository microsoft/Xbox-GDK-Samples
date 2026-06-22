// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

#include <DirectXMath.h>
#include <functional>
#include <stdexcept>
#include <vector>

namespace GameSaveSample
{
    struct GameTile
    {
        GameTile() {}
        wchar_t m_letter = 0;
        bool m_placed = false;
    };

    struct GameBoard
    {
        GameBoard(uint32_t boardType = 1, uint32_t boardWidth = c_boardWidth, uint32_t boardHeight = c_boardHeight) :
            m_boardType(boardType),
            m_boardWidth(boardWidth),
            m_boardHeight(boardHeight)
        {
            ResetBoard();
        }

        void ResetBoard()
        {
            uint32_t tiles = m_boardWidth * m_boardHeight;
            for (uint32_t i = 0; i < tiles; ++i)
            {
                m_board[i] = GameTile();
            }
        }

        // returns GameTile given a zero-based (x, y) position on the board
        GameTile& GetGameTile(const DirectX::XMUINT2& position)
        {
            size_t tile = size_t(position.x + (m_boardWidth * position.y));
            if (tile >= m_boardWidth * m_boardHeight)
            {
                throw std::invalid_argument("position is not valid for the current board size");
            }

            return m_board[tile];
        }

        uint32_t m_boardType; // for future use
        uint32_t m_updateCount = 0;
        uint32_t m_boardWidth;
        uint32_t m_boardHeight;
        GameTile m_board[25];
    };

    const std::function<void(GameBoard&)> fnSaveGameBoard = [](GameBoard& dataToSave)
    {
        dataToSave.m_updateCount++;
    };
}
