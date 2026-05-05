#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>

struct Board {
    int N;       // full dimension  (4, 9, 16)
    int boxN;    // box dimension   (2, 3,  4)
    std::vector<std::vector<int>> cells;

    Board() : N(0), boxN(0) {}
    explicit Board(int n)
        : N(n), boxN(static_cast<int>(std::round(std::sqrt(n)))),
          cells(n, std::vector<int>(n, 0)) {}
};

Board loadBoard(const std::string& filename);
void  printBoard(const Board& board);
bool  isValid(const Board& board, int row, int col, int num);
bool  isSolved(const Board& board);
