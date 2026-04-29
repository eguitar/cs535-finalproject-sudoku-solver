#include <mpi.h>
#include <iostream>
#include <string>
#include <chrono>
#include "sudoku/sudoku.h"
#include "solver/brute_algo.h"
#include "solver/par_algo.h"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 3) {
        if (rank == 0)
            std::cerr << "Usage: " << argv[0] << " <board_file> <algorithm: brute|par>\n";
        MPI_Finalize();
        return 1;
    }

    std::string boardFile = argv[1];
    std::string algo = argv[2];

    Board board;
    if (rank == 0) {
        board = loadBoard(boardFile);
        std::cout << "Input board:\n";
        printBoard(board);
    }

    // Broadcast board to all MPI ranks
    MPI_Bcast(board.data(), N * N, MPI_INT, 0, MPI_COMM_WORLD);

    bool solved = false;
    auto start = std::chrono::high_resolution_clock::now();

    if (algo == "brute") {
        solved = bruteAlgoMPI(board, rank, size);
    } else if (algo == "par") {
        solved = parAlgoMPI(board, rank, size);
    } else {
        if (rank == 0) std::cerr << "Unknown algorithm: " << algo << "\n";
    }

    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();

    if (rank == 0) {
        if (solved) {
            std::cout << "\nSolved in " << elapsed << "s:\n";
            printBoard(board);
        } else {
            std::cout << "No solution found.\n";
        }
    }

    MPI_Finalize();
    return 0;
}
