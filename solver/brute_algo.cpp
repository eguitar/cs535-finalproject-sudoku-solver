#include "brute_algo.h"

static bool solve(Board& board) {
    int N = board.N;
    for (int r = 0; r < N; r++) {
        for (int c = 0; c < N; c++) {
            if (board.cells[r][c] == 0) {
                for (int num = 1; num <= N; num++) {
                    if (isValid(board, r, c, num)) {
                        board.cells[r][c] = num;
                        if (solve(board))
                            return true;
                        board.cells[r][c] = 0;
                    }
                }
                return false;
            }
        }
    }
    return true;
}

bool bruteAlgoSerial(Board& board) {
    return solve(board);
}
