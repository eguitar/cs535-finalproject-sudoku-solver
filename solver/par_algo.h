#pragma once
#include "../sudoku/sudoku.h"

// Serial constraint propagation + backtracking (baseline)
bool parAlgoSerial(Board& board);

// Parallel version with OpenMP
bool parAlgoOpenMP(Board& board);

// Parallel version with MPI
bool parAlgoMPI(Board& board, int rank, int size);
