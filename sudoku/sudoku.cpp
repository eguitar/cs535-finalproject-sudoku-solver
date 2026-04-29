#include "sudoku.h"
#include <fstream>
#include <stdexcept>

Board loadBoard(const std::string& filename) {
    Board board{};
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Cannot open board file: " + filename);
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            file >> board[r][c];
    return board;
}

void printBoard(const Board& board) {
    for (int r = 0; r < N; r++) {
        if (r % 3 == 0 && r != 0)
            std::cout << "------+-------+------\n";
        for (int c = 0; c < N; c++) {
            if (c % 3 == 0 && c != 0) std::cout << " |";
            std::cout << " " << (board[r][c] == 0 ? '.' : (char)('0' + board[r][c]));
        }
        std::cout << "\n";
    }
}

bool isValid(const Board& board, int row, int col, int num) {
    for (int i = 0; i < N; i++) {
        if (board[row][i] == num || board[i][col] == num)
            return false;
    }
    int br = (row / 3) * 3, bc = (col / 3) * 3;
    for (int r = br; r < br + 3; r++)
        for (int c = bc; c < bc + 3; c++)
            if (board[r][c] == num)
                return false;
    return true;
}

bool isSolved(const Board& board) {
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            if (board[r][c] == 0)
                return false;
    return true;
}
