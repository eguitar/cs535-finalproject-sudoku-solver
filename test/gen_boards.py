"""
gen_boards.py  —  Generate valid Sudoku boards for any N = B^2 size.

Formula: cell(r, c) = (B*(r%B) + r//B + c) % N + 1
This produces a valid Latin square with correct rows, columns, and BxB boxes.

Usage:  python3 test/gen_boards.py
"""

import os
import math

def make_grid(N):
    B = int(math.isqrt(N))
    grid = []
    for r in range(N):
        row = [(B * (r % B) + r // B + c) % N + 1 for c in range(N)]
        grid.append(row)
    return grid

def apply_mask(grid, N, zero_condition):
    """Return grid with cells zeroed where zero_condition(r, c) is True."""
    masked = []
    for r in range(N):
        row = [0 if zero_condition(r, c) else grid[r][c] for c in range(N)]
        masked.append(row)
    return masked

def write_board(path, board, N):
    width = len(str(N)) + 1
    with open(path, "w") as f:
        for row in board:
            f.write(" ".join(f"{v:{width}}" for v in row).rstrip() + "\n")
    zeros = sum(v == 0 for row in board for v in row)
    pct = 100 * zeros // (N * N)
    print(f"  wrote {path}  ({zeros} empty / {N*N} cells, {pct}% empty)")

def generate_size(N, out_dir, mod):
    """
    Generate 6 board files for a given N in out_dir.
    mod  — the modulus used for difficulty patterns (= boxN for clean spacing)
    Difficulties:
      easy   : 1/mod  cells empty  (~1 per mod-group)
      medium : 2/mod  cells empty
      hard   : 3/mod  cells empty
    Two variants per difficulty use different residue sets.
    """
    os.makedirs(out_dir, exist_ok=True)
    grid = make_grid(N)

    configs = {
        "easy_1":   lambda r, c: (r + c) % mod == 0,
        "easy_2":   lambda r, c: (r + c) % mod == mod // 2,
        "medium_1": lambda r, c: (r + c) % mod in {0, 2},
        "medium_2": lambda r, c: (r + c) % mod in {1, 3},
        "hard_1":   lambda r, c: (r + c) % mod in {0, 1, 2},
        "hard_2":   lambda r, c: (r + c) % mod in {mod-3, mod-2, mod-1},
    }

    for name, cond in configs.items():
        board = apply_mask(grid, N, cond)
        write_board(os.path.join(out_dir, f"{name}.txt"), board, N)

if __name__ == "__main__":
    base = os.path.join(os.path.dirname(__file__), "boards")

    print("\n=== 36x36 ===")
    generate_size(36, os.path.join(base, "36x36"), mod=6)

    print("\n=== 49x49 ===")
    generate_size(49, os.path.join(base, "49x49"), mod=7)

    print("\nDone.")
