// pivot_detector.cpp
// ---------------------------------------------------------------------------
// C++ port of the pivot-high/pivot-low detection used in the Profit Predator
// trading strategy (originally implemented in Python/pandas).
//
// A pivot high at index i is a bar whose `high` is strictly the maximum in
// the window [i-left, i+right], and analogously for pivot lows. This mirrors
// TradingView's pivothigh()/pivotlow() and the Python version used in the
// backtest (see build_features.py / backtest.py in the Profit Predator repo).
//
// Usage:
//   ./pivot_detector data.csv left right
//
// CSV format (header required): timestamp,open,high,low,close,volume
//
// Prints: number of pivot highs/lows found, and wall-clock time taken,
// so it can be directly compared against the equivalent pandas loop.
// ---------------------------------------------------------------------------

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

struct Candle {
    long long timestamp;
    double open, high, low, close, volume;
};

// Reads a CSV with header: timestamp,open,high,low,close,volume
std::vector<Candle> load_csv(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path);
    }

    std::vector<Candle> candles;
    std::string line;
    std::getline(file, line);  // skip header

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string field;
        Candle c{};

        std::getline(ss, field, ',');
        c.timestamp = std::stoll(field);
        std::getline(ss, field, ',');
        c.open = std::stod(field);
        std::getline(ss, field, ',');
        c.high = std::stod(field);
        std::getline(ss, field, ',');
        c.low = std::stod(field);
        std::getline(ss, field, ',');
        c.close = std::stod(field);
        std::getline(ss, field, ',');
        c.volume = std::stod(field);

        candles.push_back(c);
    }
    return candles;
}

// Pivot high: bar i's high is the unique maximum within [i-left, i+right].
// Returns a vector the same length as `values`, with NaN where there is no
// confirmed pivot, matching the semantics of the Python pivothigh() function.
std::vector<double> pivot_high(const std::vector<double>& values, int left, int right) {
    const double NaN = std::numeric_limits<double>::quiet_NaN();
    std::vector<double> pivots(values.size(), NaN);

    for (size_t i = left; i + right < values.size(); ++i) {
        double center = values[i];
        double window_max = center;
        int max_count = 1;

        for (size_t j = i - left; j <= i + right; ++j) {
            if (j == i) continue;
            if (values[j] > window_max) {
                window_max = values[j];
            }
        }
        // Recount ties against the true window max (mirrors the pandas
        // `(window == center_value).sum() == 1` uniqueness check)
        if (center == window_max) {
            for (size_t j = i - left; j <= i + right; ++j) {
                if (values[j] == center) max_count++;
            }
            // subtract the double-count of i itself relative to loop above
            max_count -= 1;
            if (max_count == 1) {
                pivots[i] = center;
            }
        }
    }
    return pivots;
}

// Pivot low: symmetric to pivot_high, using the minimum instead of maximum.
std::vector<double> pivot_low(const std::vector<double>& values, int left, int right) {
    const double NaN = std::numeric_limits<double>::quiet_NaN();
    std::vector<double> pivots(values.size(), NaN);

    for (size_t i = left; i + right < values.size(); ++i) {
        double center = values[i];
        double window_min = center;

        for (size_t j = i - left; j <= i + right; ++j) {
            if (j == i) continue;
            if (values[j] < window_min) {
                window_min = values[j];
            }
        }
        if (center == window_min) {
            int count = 0;
            for (size_t j = i - left; j <= i + right; ++j) {
                if (values[j] == center) count++;
            }
            if (count == 1) {
                pivots[i] = center;
            }
        }
    }
    return pivots;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <csv_path> <left> <right>\n";
        return 1;
    }

    std::string csv_path = argv[1];
    int left = std::stoi(argv[2]);
    int right = std::stoi(argv[3]);

    std::vector<Candle> candles;
    try {
        candles = load_csv(csv_path);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::vector<double> highs, lows;
    highs.reserve(candles.size());
    lows.reserve(candles.size());
    for (const auto& c : candles) {
        highs.push_back(c.high);
        lows.push_back(c.low);
    }

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<double> pivot_highs = pivot_high(highs, left, right);
    std::vector<double> pivot_lows = pivot_low(lows, left, right);

    auto end = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();

    int high_count = 0, low_count = 0;
    for (double v : pivot_highs) if (!std::isnan(v)) high_count++;
    for (double v : pivot_lows) if (!std::isnan(v)) low_count++;

    std::cout << "Candles processed: " << candles.size() << "\n";
    std::cout << "Pivot highs found: " << high_count << "\n";
    std::cout << "Pivot lows found:  " << low_count << "\n";
    std::cout << "C++ compute time:  " << elapsed_ms << " ms\n";

    return 0;
}
