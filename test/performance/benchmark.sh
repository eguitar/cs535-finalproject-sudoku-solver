#!/bin/bash
# =============================================================================
# benchmark.sh  —  Compare brute_algo (serial) vs parAlgo (MPI+OpenMP)
#                  across all test boards and configurations.
#
# Usage:
#   chmod +x test/performance/benchmark.sh
#   cd <project-root>
#   make
#   bash test/performance/benchmark.sh
#
# On a SLURM cluster, wrap in an sbatch job or run interactively via:
#   srun --ntasks=8 --cpus-per-task=8 bash test/performance/benchmark.sh
# =============================================================================

SOLVER="./sudoku_solver"
RESULTS_DIR="test/performance"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULTS_CSV="$RESULTS_DIR/results_$TIMESTAMP.csv"
SUMMARY_TXT="$RESULTS_DIR/summary_$TIMESTAMP.txt"

# Per-board timeout (seconds) — boards exceeding this are marked TIMEOUT
TIMEOUT_S=30

# MPI process counts to test for par algo
MPI_PROCS=(1 2 4 8)

# OpenMP thread counts to test for par algo
OMP_THREADS=(1 2 4 8)

# All board files grouped by size
BOARDS_4x4=(
    "test/boards/4x4/easy_1.txt"
    "test/boards/4x4/easy_2.txt"
    "test/boards/4x4/medium_1.txt"
    "test/boards/4x4/medium_2.txt"
    "test/boards/4x4/hard_1.txt"
    "test/boards/4x4/hard_2.txt"
)
BOARDS_9x9=(
    "test/boards/9x9/easy_1.txt"
    "test/boards/9x9/easy_2.txt"
    "test/boards/9x9/medium_1.txt"
    "test/boards/9x9/medium_2.txt"
    "test/boards/9x9/hard_1.txt"
    "test/boards/9x9/hard_2.txt"
)
BOARDS_16x16=(
    "test/boards/16x16/easy_1.txt"
    "test/boards/16x16/easy_2.txt"
    "test/boards/16x16/medium_1.txt"
    "test/boards/16x16/medium_2.txt"
    "test/boards/16x16/hard_1.txt"
    "test/boards/16x16/hard_2.txt"
)
BOARDS_25x25=(
    "test/boards/25x25/easy_1.txt"
    "test/boards/25x25/easy_2.txt"
    "test/boards/25x25/medium_1.txt"
    "test/boards/25x25/medium_2.txt"
    "test/boards/25x25/hard_1.txt"
    "test/boards/25x25/hard_2.txt"
)
BOARDS_36x36=(
    "test/boards/36x36/easy_1.txt"
    "test/boards/36x36/easy_2.txt"
    "test/boards/36x36/medium_1.txt"
    "test/boards/36x36/medium_2.txt"
    "test/boards/36x36/hard_1.txt"
    "test/boards/36x36/hard_2.txt"
)
BOARDS_49x49=(
    "test/boards/49x49/easy_1.txt"
    "test/boards/49x49/easy_2.txt"
    "test/boards/49x49/medium_1.txt"
    "test/boards/49x49/medium_2.txt"
    "test/boards/49x49/hard_1.txt"
    "test/boards/49x49/hard_2.txt"
)

ALL_BOARDS=("${BOARDS_4x4[@]}" "${BOARDS_9x9[@]}" "${BOARDS_16x16[@]}" \
            "${BOARDS_25x25[@]}" "${BOARDS_36x36[@]}" "${BOARDS_49x49[@]}")

# ── helpers ──────────────────────────────────────────────────────────────────

check_solver() {
    if [ ! -f "$SOLVER" ]; then
        echo "ERROR: $SOLVER not found. Run 'make' first." >&2
        exit 1
    fi
}

# Run solver with a timeout and extract wall-clock time from its output.
# Returns "TIMEOUT", "UNSOLVED", or the numeric time string.
run_and_time() {
    local cmd="$1"
    local output
    output=$(eval "timeout ${TIMEOUT_S}s $cmd" 2>&1)
    local status=$?

    if [ $status -eq 124 ]; then
        echo "TIMEOUT(>${TIMEOUT_S}s)"
    elif echo "$output" | grep -q "Solved in"; then
        echo "$output" | grep "Solved in" | awk '{print $3}' | tr -d 's'
    elif echo "$output" | grep -q "No solution"; then
        echo "UNSOLVED"
    else
        echo "ERROR($status)"
    fi
}

# ── build ─────────────────────────────────────────────────────────────────────

echo "======================================================================"
echo "  Sudoku Solver Benchmark  —  $(date)"
echo "======================================================================"
echo ""

check_solver

# ── CSV header ────────────────────────────────────────────────────────────────

echo "board,size,difficulty,variant,algo,mpi_procs,omp_threads,time_s,speedup_vs_brute" \
    > "$RESULTS_CSV"

declare -A BRUTE_TIMES   # board -> brute time (for speedup calculation)

# ── Phase 1: brute algo (serial, np=1) on all boards ─────────────────────────

echo "--- Phase 1: Brute Force (serial) ---"
echo ""
printf "%-45s  %-10s\n" "Board" "Time (s)"
printf "%-45s  %-10s\n" "-----" "--------"

for board in "${ALL_BOARDS[@]}"; do
    size=$(basename "$(dirname "$board")")
    difficulty=$(basename "$board" .txt)
    variant="${difficulty##*_}"
    diff_name="${difficulty%%_*}"

    OMP_NUM_THREADS=1 cmd="mpirun -np 1 $SOLVER $board brute"
    t=$(run_and_time "OMP_NUM_THREADS=1 mpirun -np 1 $SOLVER $board brute")

    printf "%-45s  %-10s\n" "$board" "$t"
    BRUTE_TIMES["$board"]="$t"
    echo "$board,$size,$diff_name,$variant,brute,1,1,$t,1.00" >> "$RESULTS_CSV"
done

echo ""

# ── Phase 2: par algo across MPI_PROCS x OMP_THREADS configurations ──────────

echo "--- Phase 2: Par Algo (MPI + OpenMP) ---"
echo ""
printf "%-45s  %-6s  %-11s  %-10s  %-10s\n" \
       "Board" "Procs" "OMP Threads" "Time (s)" "Speedup"
printf "%-45s  %-6s  %-11s  %-10s  %-10s\n" \
       "-----" "-----" "-----------" "--------" "-------"

for board in "${ALL_BOARDS[@]}"; do
    size=$(basename "$(dirname "$board")")
    difficulty=$(basename "$board" .txt)
    variant="${difficulty##*_}"
    diff_name="${difficulty%%_*}"
    brute_t="${BRUTE_TIMES[$board]}"

    for np in "${MPI_PROCS[@]}"; do
        for nt in "${OMP_THREADS[@]}"; do
            t=$(run_and_time "OMP_NUM_THREADS=$nt mpirun -np $np $SOLVER $board par")

            # Compute speedup (skip if brute or par didn't solve)
            speedup="N/A"
            if [[ "$brute_t" =~ ^[0-9] ]] && [[ "$t" =~ ^[0-9] ]]; then
                speedup=$(awk "BEGIN {printf \"%.2f\", $brute_t / $t}")
            fi

            printf "%-45s  %-6s  %-11s  %-10s  %-10s\n" \
                   "$board" "$np" "$nt" "$t" "$speedup"
            echo "$board,$size,$diff_name,$variant,par,$np,$nt,$t,$speedup" \
                >> "$RESULTS_CSV"
        done
    done
    echo ""
done

# ── Phase 3: Summary ──────────────────────────────────────────────────────────

echo "======================================================================"
echo "  Summary"
echo "======================================================================"
{
    echo "Benchmark run: $(date)"
    echo "Results CSV:   $RESULTS_CSV"
    echo ""
    echo "Best par speedup per board size:"
    for sz in 4x4 9x9 16x16 25x25 36x36 49x49; do
        best=$(grep ",$sz," "$RESULTS_CSV" | grep ",par," \
               | awk -F',' '{print $9}' \
               | grep -E '^[0-9]' \
               | sort -n | tail -1)
        echo "  $sz : ${best:-N/A}x"
    done
} | tee "$SUMMARY_TXT"

echo ""
echo "Full results saved to: $RESULTS_CSV"
echo "Summary saved to:      $SUMMARY_TXT"
