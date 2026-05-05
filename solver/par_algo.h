#pragma once
#include "../sudoku/sudoku.h"

bool parAlgoSerial(Board& board);
bool parAlgoOpenMP(Board& board);
bool parAlgoMPI(Board& board, int rank, int size);
