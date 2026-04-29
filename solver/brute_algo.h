#pragma once
#include "../sudoku/sudoku.h"

// Serial backtracking (baseline)
bool bruteAlgoSerial(Board& board);

// Parallel backtracking with OpenMP
bool bruteAlgoOpenMP(Board& board);

// Parallel backtracking with MPI (root distributes candidates to workers)
bool bruteAlgoMPI(Board& board, int rank, int size);
