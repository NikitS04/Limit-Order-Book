# Limit Order Book + Matching Engine (C++20)

A deterministic price-time priority limit order book with a single-threaded matching core, fed by a three-stage pipeline (ingest, match, write) over lock-free SPSC ring buffers.

## Key Features

- **Price-time priority** with limit, market (IOC), cancel and modify orders
- **Deterministic core**: one thread owns the book, so the same input always produces the same trades
- **Lock-free pipeline**: SPSC rings with acquire/release ordering between ingest, matcher and writer threads
- **Pooled orders**: resting orders come from a preallocated pool with O(1) cancel via an order-ID index
- **Two benchmarks**: end-to-end pipeline (`lob`) and matching core in isolation (`lob_bench`)
- **Tested**: 17 test programs including a randomized invariant test, run under ASan/UBSan in Debug

## Order semantics

| Event | Behaviour |
|-------|-----------|
| `NEW` | Match against the opposite side up to the limit price; any remainder rests at the back of its price level |
| `MKT` | Match with no price limit; any unfilled remainder is discarded (immediate-or-cancel) |
| `CXL` | Remove the resting order |
| `MOD` | `qty` is the new total size. Size down at the same price keeps queue priority. A price change or size up loses priority: the order is pulled, re-matched (so a reprice through the spread trades rather than crossing the book), and any remainder rests at the back. `qty <= 0` cancels |

## Quick Start

```bash
cmake -S . -B build            # defaults to Release
cmake --build build -j
cd build && ctest --output-on-failure && cd ..

./build/lob --in data/demo_long.csv --trades trades.csv --depth depth.csv --snapshot-every 0
head trades.csv
```

For maximum performance on your own machine add `-DCMAKE_CXX_FLAGS="-march=native"`.

## Benchmarks

Generate a dataset. The default generator produces a mixed workload closer to real order flow than an add-only stream:

```bash
python3 bench_gen.py --out events.csv -n 1000000              # 45% new, 40% cancel, 10% modify, 5% market
python3 bench_gen.py --out events.csv -n 1000000 --mix 70,20,5,5
python3 bench_gen.py --out legacy.csv -n 1000000 --legacy     # original add-only stream
```

### Matching core (`lob_bench`)

Pre-parses every event into memory, then runs the exact matching code the pipeline uses on one thread, with no queues, no I/O and no clock reads inside the throughput loop. Latency is measured in a separate pass.

```bash
./build/lob_bench --in events.csv --runs 5
```

### End-to-end pipeline (`lob`)

```bash
./build/lob --in events.csv --trades /dev/null --depth /dev/null --snapshot-every 0 --ring 4194304
cat metrics.json
```

The pipeline's throughput is bounded by CSV parsing on the ingest thread: the matcher spends most of its time waiting for input, which is why `ingest_msgs_per_s` and `match_msgs_per_s` come out almost identical. Use `lob_bench` for the engine's own throughput.

### Example results

2-vCPU Intel Xeon @ 2.1 GHz cloud VM, GCC 13.3, `-O3`, 1M events, median of 5 runs. Re-run on your own hardware before quoting numbers.

| Workload | Core throughput | Core p50 / p99 / p99.9 | Pipeline (CSV-bound) |
|----------|-----------------|------------------------|----------------------|
| Mixed (45/40/10/5) | 7.6M msgs/s | 123 / 414 / 665 ns | 1.2 to 1.4M msgs/s |
| Add-only (`--legacy`) | 12.0M msgs/s | 102 / 311 / 1721 ns | 1.2 to 1.4M msgs/s |

**Latency methodology:** each sample brackets one event with two `steady_clock::now()` calls, which cost ~30 ns each on this machine (`lob_bench` measures and prints this). Treat the low percentiles as upper bounds. In the pipeline, latency also includes any time the matcher spends blocked on a full output ring.

### metrics.json

```json
{
  "input_msgs": 1000000,
  "matched_msgs": 1000000,
  "output_msgs": 286384,
  "snapshot_count": 0,
  "ingest_msgs_per_s": 1226976,
  "match_msgs_per_s": 1231335,
  "output_msgs_per_s": 352635,
  "match_p50_ns": 138,
  "match_p95_ns": 406,
  "match_p99_ns": 625,
  "match_p999_ns": 1324
}
```

## Usage

```bash
./build/lob --in <input.csv> --trades <trades.csv> --depth <depth.csv> \
            [--metrics metrics.json] [--snapshot-every N] [--ring CAPACITY]
```

| Option | Description |
|--------|-------------|
| `--in` | Input events CSV |
| `--trades` | Output trades CSV (`/dev/null` to ignore) |
| `--depth` | Output L1 depth snapshots CSV (`/dev/null` to ignore) |
| `--metrics` | Metrics JSON path (default `metrics.json`) |
| `--snapshot-every` | Write an L1 snapshot every N events (0 = off) |
| `--ring` | SPSC ring capacity (rounded up to a power of two) |

## Data Formats

**Input** `events.csv`: `ts_ns,side,type,id,price,qty`
- `side`: `B` or `S`
- `type`: `NEW`, `MKT`, `CXL`, `MOD`
- `price`: integer ticks; ignored for `MKT` and `CXL`
- `qty`: shares/contracts; for `MOD` this is the new total size

**Output** `trades.csv`: `ts_ns,buy_id,sell_id,price,qty`

**Output** `depth.csv`: `ts_ns,bid_px,bid_sz,ask_px,ask_sz`

## Design

- **Price levels:** `std::map` per side (bids descending, asks ascending), each level a doubly linked FIFO of resting orders with a cached total size for O(1) top-of-book.
- **Orders:** `RestingOrder` nodes come from a fixed-capacity pool (default 2M). When the pool is exhausted new orders are rejected cleanly.
- **Index:** `OrderId -> {side, price, node*}` for O(1) cancel and modify lookup.
- **Pipeline:** ingest thread parses CSV into a ring; matcher thread owns the book and pushes trades and L1 snapshots to two output rings; writer thread drains them to CSV.

## Limitations and next steps

- `std::map` levels and the `std::unordered_map` index allocate on insert, so the hot path is not allocation-free. Next step is a flat array of levels indexed by price tick and a flat or open-addressed order index.
- Single instrument.
- Producers spin-yield on full rings.
- CSV parsing bounds the end-to-end pipeline; a binary input format would lift it.

**Language**: C++20 | **Author**: Nikit Sajiv
