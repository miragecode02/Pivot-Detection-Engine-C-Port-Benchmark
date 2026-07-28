"""Generates a synthetic OHLCV CSV for benchmarking the C++ vs Python pivot
detectors on identical data. Not for strategy validation -- just fixed,
reproducible input so both implementations can be timed fairly."""

import numpy as np
import pandas as pd
import sys

n = int(sys.argv[1]) if len(sys.argv) > 1 else 50000
rng = np.random.default_rng(42)

returns = rng.normal(0, 0.002, n)
close = 30000 * np.exp(np.cumsum(returns))
high = close * (1 + np.abs(rng.normal(0, 0.001, n)))
low = close * (1 - np.abs(rng.normal(0, 0.001, n)))
open_ = np.roll(close, 1)
open_[0] = close[0]
volume = rng.lognormal(5, 0.5, n)
timestamp = np.arange(n) * 900_000  # 15-min bars in ms

df = pd.DataFrame(
    {
        "timestamp": timestamp,
        "open": open_,
        "high": high,
        "low": low,
        "close": close,
        "volume": volume,
    }
)
df.to_csv("bench_data.csv", index=False)
print(f"Wrote {n} rows to bench_data.csv")
