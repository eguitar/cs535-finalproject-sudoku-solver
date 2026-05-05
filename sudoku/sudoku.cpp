#include "sudoku.h"
#include <fstream>
#include <stdexcept>

Board loadBoard(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Cannot open board file: " + filename);

    std::vector<int> vals;
    int v;
    while (file >> v)
        vals.push_back(v);

    int total = static_cast<int>(vals.size());
    int N = static_cast<int>(std::round(std::sqrt(total)));
    if (N * N != total)
        throw std::runtime_error("Board has " + std::to_string(total) +
                                 " values — expected a perfect square");

    Board board(N);
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            board.cells[r][c] = vals[r * N + c];
    return board;
}

void printBoard(const Board& board) {
    int N = board.N, B = board.boxN;
    int colW = (N >= 10) ? 3 : 2;
    std::string sep(N * colW + (B - 1) * 2, '-');

    for (int r = 0; r < N; r++) {
        if (r % B == 0 && r != 0)
            std::cout << sep << "\n";
        for (int c = 0; c < N; c++) {
            if (c % B == 0 && c != 0) std::cout << " |";
            int val = board.cells[r][c];
            if (val == 0)
                std::cout << std::setw(colW) << '.';
            else
                std::cout << std::setw(colW) << val;
        }
        std::cout << "\n";
    }
}

bool isValid(const Board& board, int row, int col, int num) {
    int N = board.N, B = board.boxN;
    for (int i = 0; i < N; i++) {
        if (board.cells[row][i] == num || board.cells[i][col] == num)
            return false;
    }
    int br = (row / B) * B, bc = (col / B) * B;
    for (int r = br; r < br + B; r++)
        for (int c = bc; c < bc + B; c++)
            if (board.cells[r][c] == num)
                return false;
    return true;
}

bool isSolved(const Board& board) {
    for (int r = 0; r < board.N; r++)
        for (int c = 0; c < board.N; c++)
            if (board.cells[r][c] == 0)
                return false;
    return true;
}
