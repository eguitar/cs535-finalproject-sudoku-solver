#include "par_algo.h"
#include <omp.h>
#include <mpi.h>
#include <vector>
#include <queue>
#include <atomic>
#include <climits>
#include <algorithm>

// Bitmask of candidate values per cell: bit v set means value v is still possible.
// Bits 1..N are used; bit 0 is unused.
using Cands = std::vector<std::vector<uint32_t>>;

// How many levels deep we spawn OpenMP tasks before switching to serial recursion.
static constexpr int OMP_TASK_DEPTH = 3;

// ─── helpers ────────────────────────────────────────────────────────────────

// Place value v at (r,c) and remove it from every peer's candidate set.
static void place(Board& b, Cands& cands, int r, int c, int v) {
    b.cells[r][c] = v;
    cands[r][c]   = 0;
    const uint32_t mask = ~(1u << v);
    const int N = b.N, B = b.boxN;
    for (int i = 0; i < N; i++) { cands[r][i] &= mask; cands[i][c] &= mask; }
    const int br = (r/B)*B, bc = (c/B)*B;
    for (int dr = 0; dr < B; dr++)
        for (int dc = 0; dc < B; dc++)
            cands[br+dr][bc+dc] &= mask;
}

// Build initial candidate bitmasks from a board's given values.
static Cands buildCands(const Board& b) {
    const int N = b.N;
    const uint32_t full = (1u << (N+1)) - 2;   // bits 1..N set
    Cands cands(N, std::vector<uint32_t>(N, full));
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            if (b.cells[r][c]) cands[r][c] = 0;
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            if (b.cells[r][c]) {
                const uint32_t mask = ~(1u << b.cells[r][c]);
                const int B = b.boxN;
                for (int i = 0; i < N; i++) { cands[r][i] &= mask; cands[i][c] &= mask; }
                const int br = (r/B)*B, bc = (c/B)*B;
                for (int dr = 0; dr < B; dr++)
                    for (int dc = 0; dc < B; dc++)
                        cands[br+dr][bc+dc] &= mask;
            }
    return cands;
}

// Naked-single propagation: if a cell has exactly one candidate, place it.
// Repeats until no more progress. Returns false on contradiction.
static bool propagate(Board& b, Cands& cands) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (int r = 0; r < b.N; r++)
            for (int c = 0; c < b.N; c++) {
                if (b.cells[r][c]) continue;
                if (!cands[r][c]) return false;
                if (__builtin_popcount(cands[r][c]) == 1) {
                    place(b, cands, r, c, __builtin_ctz(cands[r][c]));
                    changed = true;
                }
            }
    }
    return true;
}

// MRV (Minimum Remaining Values): pick the empty cell with fewest candidates.
// Returns {-1,-1} if the board is fully filled.
static std::pair<int,int> mrvCell(const Board& b, const Cands& cands) {
    int N = b.N, best = INT_MAX;
    std::pair<int,int> cell{-1, -1};
    for (int r = 0; r < N && best > 2; r++)
        for (int c = 0; c < N && best > 2; c++) {
            if (b.cells[r][c]) continue;
            int n = __builtin_popcount(cands[r][c]);
            if (n < best) { best = n; cell = {r, c}; }
        }
    return cell;
}

// ─── OpenMP solver ──────────────────────────────────────────────────────────

// Recursive solve with constraint propagation + MRV.
// Spawns OpenMP tasks for the first OMP_TASK_DEPTH levels, then goes serial.
// Uses raw pointers for found/solution so task data-sharing is unambiguous.
static void solveOMP(Board b, Cands cands, int depth,
                     std::atomic<bool>* found, Board* solution) {
    if (found->load(std::memory_order_relaxed)) return;
    if (!propagate(b, cands)) return;

    if (isSolved(b)) {
        bool expected = false;
        if (found->compare_exchange_strong(expected, true)) {
            #pragma omp critical
            *solution = b;
        }
        return;
    }

    auto [r, c] = mrvCell(b, cands);
    if (r == -1) return;

    uint32_t mask = cands[r][c];

    if (depth < OMP_TASK_DEPTH) {
        while (mask) {
            if (found->load(std::memory_order_relaxed)) return;
            int v = __builtin_ctz(mask); mask &= mask - 1;
            Board  nb = b;
            Cands  nc = cands;
            place(nb, nc, r, c, v);
            #pragma omp task firstprivate(nb, nc)
            solveOMP(nb, nc, depth + 1, found, solution);
        }
    } else {
        while (mask) {
            if (found->load(std::memory_order_relaxed)) return;
            int v = __builtin_ctz(mask); mask &= mask - 1;
            Board  nb = b;
            Cands  nc = cands;
            place(nb, nc, r, c, v);
            solveOMP(nb, nc, depth + 1, found, solution);
        }
    }
}

// ─── MPI helpers ────────────────────────────────────────────────────────────

static std::vector<int> flattenBoard(const Board& b) {
    const int N = b.N;
    std::vector<int> f(N * N);
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            f[r*N + c] = b.cells[r][c];
    return f;
}

static Board unflattenBoard(const std::vector<int>& f, int N) {
    Board b(N);
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            b.cells[r][c] = f[r*N + c];
    return b;
}

// BFS on the search tree: expand cells via MRV + propagation until we have
// at least `target` partial boards to hand out as independent subproblems.
static std::vector<Board> generateSubproblems(Board board, int target) {
    Cands cands = buildCands(board);
    if (!propagate(board, cands)) return {};
    if (isSolved(board))          return {board};

    using Entry = std::pair<Board, Cands>;
    std::queue<Entry> q;
    q.push({board, cands});

    while ((int)q.size() < target && !q.empty()) {
        auto [b, ca] = q.front(); q.pop();
        if (isSolved(b)) { q.push({b, ca}); break; }

        auto [r, c] = mrvCell(b, ca);
        if (r == -1) { q.push({b, ca}); continue; }

        uint32_t mask = ca[r][c];
        while (mask) {
            int v = __builtin_ctz(mask); mask &= mask - 1;
            Board nb = b; Cands nc = ca;
            place(nb, nc, r, c, v);
            if (propagate(nb, nc))
                q.push({nb, nc});
        }
    }

    std::vector<Board> result;
    while (!q.empty()) { result.push_back(q.front().first); q.pop(); }
    return result;
}

// ─── entry point ────────────────────────────────────────────────────────────

bool parAlgo(Board& board, int rank, int size) {
    const int N            = board.N;
    const int cellsPerBoard = N * N;

    // ── MPI phase 1: rank 0 generates subproblems and broadcasts them ────────
    int numSubs = 0;
    std::vector<int> allFlat;

    if (rank == 0) {
        auto subs = generateSubproblems(board, size * 4);
        numSubs   = (int)subs.size();
        allFlat.resize(numSubs * cellsPerBoard);
        for (int i = 0; i < numSubs; i++) {
            auto f = flattenBoard(subs[i]);
            std::copy(f.begin(), f.end(), allFlat.begin() + i * cellsPerBoard);
        }
    }

    MPI_Bcast(&numSubs, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (numSubs == 0) return false;

    allFlat.resize(numSubs * cellsPerBoard);
    MPI_Bcast(allFlat.data(), numSubs * cellsPerBoard, MPI_INT, 0, MPI_COMM_WORLD);

    // Round-robin assignment: rank k owns subproblems k, k+size, k+2*size, …
    std::vector<Board> myBoards;
    for (int i = rank; i < numSubs; i += size) {
        std::vector<int> f(allFlat.begin() +  i      * cellsPerBoard,
                           allFlat.begin() + (i + 1) * cellsPerBoard);
        myBoards.push_back(unflattenBoard(f, N));
    }

    // ── OpenMP phase: solve assigned subproblems in parallel ─────────────────
    std::atomic<bool> found(false);
    Board solution;

    #pragma omp parallel
    {
        #pragma omp single
        {
            for (auto& b : myBoards) {
                if (found.load(std::memory_order_relaxed)) break;
                Cands cands = buildCands(b);
                #pragma omp task firstprivate(b, cands)
                solveOMP(b, cands, 0, &found, &solution);
            }
        }
        // implicit taskwait at end of parallel region ensures all tasks finish
    }

    // ── MPI phase 2: gather result from whichever rank solved it ─────────────
    int localSolved  = found.load() ? 1 : 0;
    int globalSolved = 0;
    MPI_Allreduce(&localSolved, &globalSolved, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    if (globalSolved == 0) return false;

    // Lowest-ranked solver broadcasts the solution
    int myTag      = localSolved ? rank : size;   // size = sentinel "no solution"
    int winnerRank = 0;
    MPI_Allreduce(&myTag, &winnerRank, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);

    std::vector<int> solFlat(cellsPerBoard);
    if (rank == winnerRank) solFlat = flattenBoard(solution);
    MPI_Bcast(solFlat.data(), cellsPerBoard, MPI_INT, winnerRank, MPI_COMM_WORLD);

    board = unflattenBoard(solFlat, N);
    return true;
}
