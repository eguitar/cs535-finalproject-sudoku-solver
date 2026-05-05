#pragma once
#include "../sudoku/sudoku.h"

// Optimized parallel solver: constraint propagation + backtracking
// Uses MPI to distribute search branches across processes,
// OpenMP to parallelize within each process.
bool parAlgo(Board& board, int rank, int size);
