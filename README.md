Limit Order Book + Matching Engine (C++20) — Zero-Alloc Hot Path

Deterministic price–time Limit Order Book (LOB) with a single-threaded matching core and a multi-threaded pipeline (ingest → match → write). The hot path uses an intrusive FIFO + preallocated pool (no heap allocations per order). Built-in metrics report throughput and p50/p95/p99 latency.

Language/Std: C++20

Key design: Deterministic core, lock-free SPSC queues (acquire/release), zero-alloc hot path

Outputs: trades.csv, depth.csv, metrics.json

Benchmarks: Reproducible via bench_gen.py and documented build flags

Quickstart (3 commands)
# 1) configure & build (Release, tuned)
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_FLAGS_RELEASE="-O3 -march=native -flto=thin" ..
cmake --build . -j

# 2) run a tiny demo with many fills
./lob --in ../data/demo_long.csv --trades trades.csv --depth depth.csv --snapshot-every 0

# 3) inspect results
head -n 10 trades.csv


You’ll see multiple trades immediately. To count them:

# minus header → ~200 trades in the bundled demo_long.csv
tail -n +2 trades.csv | wc -l

Try it yourself: 1,000,000-event benchmark

Generate a synthetic dataset (Python 3 required):

# from build/
python3 ../bench_gen.py --out events.csv -n 1000000 --mid 10000 --seed 1


Run the engine with I/O minimized (avoids disk bottlenecks):

./lob --in events.csv --trades /dev/null --depth /dev/null --snapshot-every 0 --ring 4194304
cat metrics.json


You’ll get a JSON like:

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


On Apple Silicon w/ Release -O3 -march=native -flto=thin, expect ≥ 500K msgs/s; on our test Mac we measured ~1.12M msgs/s.

Handy one-liner to summarize:

python3 - << 'PY'
import json; m=json.load(open("metrics.json"))
print(f"match: {m['match_msgs_per_s']:.0f} msg/s | p50 {m['match_p50_ns']} ns | p95 {m['match_p95_ns']} ns | p99 {m['match_p99_ns']} ns")
PY

Build & test
# configure & build
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_FLAGS_RELEASE="-O3 -march=native -flto=thin" ..
cmake --build . -j

# run unit + integration tests (12 tests)
ctest --output-on-failure

CLI
./lob --in <path.csv> --trades <trades.csv> --depth <depth.csv> \
      [--snapshot-every N] [--ring CAPACITY]


--in input events CSV

--trades output trades CSV (use /dev/null to ignore)

--depth output L1 depth snapshots CSV (use /dev/null to ignore)

--snapshot-every write L1 snapshot every N events (0 = off)

--ring capacity for SPSC rings (power-of-two recommended)

Data formats

events.csv

ts_ns,side,type,id,price,qty


side: B or S

type: NEW (limit), MKT (market), CXL (cancel), MOD (modify)

price: integer ticks (e.g., cents); ignored for MKT/CXL

qty: shares/contracts

trades.csv

ts_ns,buy_id,sell_id,price,qty


depth.csv (L1 snapshots)

ts_ns,bid_px,bid_sz,ask_px,ask_sz


metrics.json (throughput + latency)

*_msgs_per_s — ingest/match/write rates

match_p50_ns / _p95_ns / _p99_ns — event processing latency (matcher only), nanoseconds

match_p50_us / _p95_us / _p99_us — same in microseconds

Architecture (high level)
CSV ingest  ──►  SPSC<Ring<Event>>  ──►  Matcher(core)  ──►  SPSC<Ring<Trade>>  ──►  CSV writer
                               │                         └─►  SPSC<Ring<DepthL1>>


Deterministic core: only the matcher mutates book state.

Order book: per-side std::map<price → level>. Each level is an intrusive FIFO of RestingOrder nodes.

Zero-alloc hot path: nodes come from a preallocated pool; cancels are O(1) via {side, px, node*} index.

Matching: price–time priority; market/limit; modify = cancel+add.

Latency: measured per event in matcher, aggregated into a fixed-bin (ns) histogram → p50/p95/p99.

Reproduce the “~200 trades” demo

We include data/demo_long.csv (≈ 300 events yielding ≈ 200 trades):

./lob --in ../data/demo_long.csv --trades trades.csv --depth depth.csv --snapshot-every 0
tail -n +2 trades.csv | wc -l   # ~200
head -n 10 trades.csv

Tuning & tips

Use Release + -O3 -march=native -flto=thin.

Keep outputs at /dev/null and snapshots off (--snapshot-every 0) for clean throughput numbers.

Increase ring size for heavy bursts: --ring 4194304.

Close background apps; keep laptop on power.

Troubleshooting

Error: not a CMake build directory (missing CMakeCache.txt)
Run cmake -S . -B build ... first, then cmake --build build.

Tests fail with linker errors for template methods
Make sure template implementations (e.g., matchBuy/matchSell) are in the header (include/lob/book.hpp).

Pool exhausted
Increase pool size (constructor arg in Book(size_t pool_capacity)), or reduce simultaneous resting orders in your workload.

What this demonstrates (for your CV/README)

Throughput: e.g., ~1.12M msgs/s on 1M events (Apple Silicon, Release tuned).

Latency: e.g., p50 ~ 42 ns, p95 ~ 167 ns, p99 ~ 500 ns (synthetic run).

Deterministic + safe concurrency: SPSC rings with acquire/release; single writer core.

Performance engineering: zero-alloc hot path via intrusive structures & pools; batched queues; minimal I/O.

For transparency/reproducibility, include your CPU model:

sysctl -n machdep.cpu.brand_string  # macOS
sysctl -n hw.ncpu

Repo layout
include/lob/
  types.hpp, event.hpp, trade.hpp
  spsc_ring.hpp, pool.hpp, book.hpp
  matcher.hpp, metrics.hpp, latency_hist.hpp, csv.hpp
src/
  book.cpp, matcher.cpp, metrics.cpp, main.cpp
tests/
  book_tests.cpp, matcher_tests.cpp
  ... (10 more focused tests)
data/
  demo_long.csv
bench_gen.py
