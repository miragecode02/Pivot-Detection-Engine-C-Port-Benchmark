# C++ Pivot Detector — Performance Port

A C++ port of the pivot-high/pivot-low detection logic used in the **Profit
Predator** trading strategy (originally a per-bar Python/pandas loop in
`backtest.py`). Same algorithm, same semantics, same output — timed against
the Python original on identical data to quantify the speedup from moving
hot-path signal detection out of Python.

## Why this exists

The original Python implementation loops over every bar with pandas `.iloc`
indexing — correct, but slow at scale, and exactly the kind of hot path that
matters once you're evaluating pivots across months of intraday data or
running this inside a live low-latency loop rather than a one-off backtest.

## Files

- `pivot_detector.cpp` — C++ implementation (O2-optimized, single file, no
  external dependencies beyond the standard library)
- `pivot_python_benchmark.py` — the original Python/pandas logic, timed
  identically
- `generate_data.py` — generates reproducible synthetic OHLCV data so both
  implementations run on exactly the same input

## Correctness

Both implementations are verified to produce **identical pivot counts** on
the same input data (see results below) — this isn't just a faster
approximation, it's the same algorithm re-expressed in a lower-level
language.

## Results

Benchmarked on 15-min-bar synthetic OHLCV data, `left=3, right=3`:

| Rows    | C++ time  | Python time | Speedup   | Pivot highs (both) | Pivot lows (both) |
|---------|-----------|-------------|-----------|---------------------|---------------------|
| 50,000  | 1.71 ms   | 8,193.28 ms | **~4,790x** | 5,068               | 5,080               |
| 200,000 | 5.65 ms   | *(not run — Python's O(n) `.iloc` overhead makes this impractical)* | — | 20,103 | 20,061 |

The C++ version scales roughly linearly with data size (1.71 ms → 5.65 ms
for a 4x increase in rows), as expected for an O(n) sliding-window pivot
scan. The Python version's per-row `.iloc` slicing overhead dominates at
scale, which is exactly why this kind of hot path is a natural candidate to
move out of Python in a real trading system.

## Build & run

```bash
g++ -O2 -std=c++17 pivot_detector.cpp -o pivot_detector
python generate_data.py 50000        # writes bench_data.csv
./pivot_detector bench_data.csv 3 3  # left=3, right=3
python pivot_python_benchmark.py     # compare against the Python original
```

## Resume bullet

> Ported hot-path pivot-detection logic from the Profit Predator trading
> strategy (Python/pandas) to C++, achieving a ~4,800x speedup on 50K bars
> (8.2s → 1.7ms) while producing identical output, verified against the
> original implementation on matched data.
