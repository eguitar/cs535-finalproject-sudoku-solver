#pragma once
#include <array>
#include <string>
#include <iostream>

static const int N = 9;

using Board = std::array<std::array<int, N>, N>;

// 0 = empty cell
Board loadBoard(const std::string& filename);
void printBoard(const Board& board);
bool isValid(const Board& board, int row, int col, int num);
bool isSolved(const Board& board);
