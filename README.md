# Limit Order Book + Matching Engine (C++20)

A high-performance, deterministic price–time Limit Order Book (LOB) with single-threaded matching core and multi-threaded pipeline architecture.


## Key Features

- **Deterministic Core**: Single-threaded matching engine ensures consistent results
- **Lock-Free Concurrency**: SPSC queues with acquire/release semantics
- **Built-in Metrics**: Real-time throughput and latency reporting (p50/p95/p99)
- **High Performance**: >500K messages/second on modern hardware

## Quick Start

Start the program in these 3 commands:

### 1. Configure & Build (Release, Optimized)
```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_CXX_FLAGS_RELEASE="-O3 -march=native -flto=thin" ..
cmake --build . -j
```

### 2. Run Demo with Multiple Fills
```bash
./lob --in ../data/demo_long.csv \
      --trades trades.csv \
      --depth depth.csv \
      --snapshot-every 0
```

### 3. Inspect Results
```bash
head -n 10 trades.csv
```

Count trades (minus header → ~200 trades in demo):
```bash
tail -n +2 trades.csv | wc -l
```

## 🔥 1,000,000-Event Benchmark

### Generate Synthetic Dataset
```bash
# From build/ directory
python3 ../bench_gen.py --out events.csv -n 1000000 --mid 10000 --seed 1
```

### Run High-Performance Test
```bash
./lob --in events.csv \
      --trades /dev/null \
      --depth /dev/null \
      --snapshot-every 0 \
      --ring 4194304

cat metrics.json
```

### Expected Output
```json
{
  "input_msgs": 1000000,
  "matched_msgs": 1000000,
  "output_msgs": 271315,
  "snapshot_count": 0,
  "ingest_msgs_per_s": 1124470.0,
  "match_msgs_per_s": 1124470.0,
  "output_msgs_per_s": 305084.0,
  "match_p50_ns": 42,
  "match_p95_ns": 167,
  "match_p99_ns": 500,
  "match_p50_us": 0.042,
  "match_p95_us": 0.167,
  "match_p99_us": 0.5
}
```

### Quick Summary
```bash
python3 - << 'PY'
import json
m = json.load(open("metrics.json"))
print(f"match: {m['match_msgs_per_s']:.0f} msg/s | p50 {m['match_p50_ns']} ns | p95 {m['match_p95_ns']} ns | p99 {m['match_p99_ns']} ns")
PY
```

##  Build & Test

### Build
```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_CXX_FLAGS_RELEASE="-O3 -march=native -flto=thin" ..
cmake --build . -j
```

### Run Tests (12 tests total)
```bash
ctest --output-on-failure
```

## Usage

```bash
./lob --in <input.csv> --trades <trades.csv> --depth <depth.csv> \
      [--snapshot-every N] [--ring CAPACITY]
```

### Command Line Options

| Option | Description |
|--------|-------------|
| `--in` | Input events CSV file |
| `--trades` | Output trades CSV (use `/dev/null` to ignore) |
| `--depth` | Output L1 depth snapshots CSV (use `/dev/null` to ignore) |
| `--snapshot-every` | Write L1 snapshot every N events (0 = off) |
| `--ring` | Capacity for SPSC rings (power-of-two recommended) |

## Data Formats

### Input: events.csv
```csv
ts_ns,side,type,id,price,qty
```

- **side**: `B` (Buy) or `S` (Sell)
- **type**: `NEW` (limit), `MKT` (market), `CXL` (cancel), `MOD` (modify)
- **price**: Integer ticks (e.g., cents); ignored for `MKT`/`CXL`
- **qty**: Shares/contracts

### Output: trades.csv
```csv
ts_ns,buy_id,sell_id,price,qty
```

### Output: depth.csv (L1 snapshots)
```csv
ts_ns,bid_px,bid_sz,ask_px,ask_sz
```

### Output: metrics.json
- `*_msgs_per_s` — Ingest/match/write rates
- `match_p50_ns` / `_p95_ns` / `_p99_ns` — Event processing latency (nanoseconds)
- `match_p50_us` / `_p95_us` / `_p99_us` — Same latencies in microseconds

**Language**: C++20 | **Author**: Nikit Sajiv
