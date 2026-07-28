"""Same pivot-high/pivot-low logic as the original Profit Predator backtest,
timed for direct comparison against the C++ port."""

import time
import pandas as pd


def pivothigh(series, left, right):
    pivots = pd.Series(index=series.index, dtype=float)
    for i in range(left, len(series) - right):
        window = series.iloc[i - left : i + right + 1]
        center_value = series.iloc[i]
        if center_value == window.max() and (window == center_value).sum() == 1:
            pivots.iloc[i] = center_value
    return pivots


def pivotlow(series, left, right):
    pivots = pd.Series(index=series.index, dtype=float)
    for i in range(left, len(series) - right):
        window = series.iloc[i - left : i + right + 1]
        center_value = series.iloc[i]
        if center_value == window.min() and (window == center_value).sum() == 1:
            pivots.iloc[i] = center_value
    return pivots


if __name__ == "__main__":
    df = pd.read_csv("bench_data.csv")

    start = time.perf_counter()
    ph = pivothigh(df["high"], left=3, right=3)
    pl = pivotlow(df["low"], left=3, right=3)
    elapsed_ms = (time.perf_counter() - start) * 1000

    print(f"Candles processed: {len(df)}")
    print(f"Pivot highs found: {ph.notna().sum()}")
    print(f"Pivot lows found:  {pl.notna().sum()}")
    print(f"Python compute time: {elapsed_ms:.2f} ms")
