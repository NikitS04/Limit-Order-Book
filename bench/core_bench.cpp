// Core-only benchmark: measures the matching engine in isolation.
//
// The threaded pipeline (lob) is bounded by CSV parsing on the ingest thread,
// so its match_msgs_per_s mostly reflects parser speed. This benchmark
// pre-parses every event into memory first, then drives apply_event() (the
// exact code the pipeline's matcher runs) on a single thread with no queues,
// no I/O and no per-event clock reads in the throughput pass.
//
// Usage: lob_bench --in events.csv [--runs 5]
#include "lob/apply.hpp"
#include "lob/csv.hpp"
#include "lob/latency_hist.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
using namespace lob;
using Clock = std::chrono::steady_clock;

static std::vector<Event> load(const std::string &path)
{
  std::ifstream f(path);
  if (!f)
  {
    std::fprintf(stderr, "Failed to open input: %s\n", path.c_str());
    std::exit(1);
  }
  std::vector<Event> ev;
  std::string line;
  std::getline(f, line); // header
  while (std::getline(f, line))
    if (auto e = parse_event_csv_line(line, false))
      ev.push_back(*e);
  return ev;
}

// Median cost of one steady_clock::now() call, so latency numbers can be read
// with the measurement overhead in mind.
static double clock_overhead_ns()
{
  std::vector<double> s;
  for (int r = 0; r < 9; ++r)
  {
    constexpr int N = 1'000'000;
    auto a = Clock::now();
    Clock::time_point x{};
    for (int i = 0; i < N; ++i)
      x = Clock::now();
    auto b = Clock::now();
    (void)x;
    s.push_back(std::chrono::duration<double, std::nano>(b - a).count() / N);
  }
  std::sort(s.begin(), s.end());
  return s[s.size() / 2];
}

int main(int argc, char **argv)
{
  std::string in = "events.csv";
  int runs = 5;
  for (int i = 1; i < argc; ++i)
  {
    if (!std::strcmp(argv[i], "--in") && i + 1 < argc)
      in = argv[++i];
    else if (!std::strcmp(argv[i], "--runs") && i + 1 < argc)
      runs = std::max(1, std::atoi(argv[++i]));
  }

  const std::vector<Event> ev = load(in);
  size_t n_new = 0, n_mkt = 0, n_cxl = 0, n_mod = 0;
  for (const auto &e : ev)
    switch (e.type)
    {
    case Type::Limit: ++n_new; break;
    case Type::Market: ++n_mkt; break;
    case Type::Cancel: ++n_cxl; break;
    case Type::Modify: ++n_mod; break;
    }
  const double N = (double)ev.size();
  std::printf("events: %zu  (new %.0f%%, cancel %.0f%%, modify %.0f%%, market %.0f%%)\n",
              ev.size(), 100 * n_new / N, 100 * n_cxl / N, 100 * n_mod / N, 100 * n_mkt / N);

  // Throughput: whole-run wall time, no per-event timing. Fresh book per run;
  // report the median run.
  std::vector<double> rates;
  uint64_t trades = 0;
  for (int r = 0; r < runs; ++r)
  {
    auto book = std::make_unique<Book>();
    uint64_t t = 0;
    auto cb = [&](TsNs, OrderId, OrderId, Px, Qty) { ++t; };
    auto a = Clock::now();
    for (const auto &e : ev)
      apply_event(*book, e, cb);
    auto b = Clock::now();
    rates.push_back(N / std::chrono::duration<double>(b - a).count());
    trades = t;
  }
  std::sort(rates.begin(), rates.end());
  std::printf("trades: %llu\n", (unsigned long long)trades);
  std::printf("throughput: median %.2fM msgs/s  (min %.2fM, max %.2fM, %d runs)\n",
              rates[rates.size() / 2] / 1e6, rates.front() / 1e6, rates.back() / 1e6, runs);

  // Latency: per-event timing on a separate pass. Each sample includes the
  // cost of the clock reads, reported below.
  auto book = std::make_unique<Book>();
  LatencyHist hist;
  auto noop = [](TsNs, OrderId, OrderId, Px, Qty) {};
  for (const auto &e : ev)
  {
    auto a = Clock::now();
    apply_event(*book, e, noop);
    auto b = Clock::now();
    hist.record_ns((uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count());
  }
  std::printf("latency (per event, includes clock overhead): p50 %llu ns | p95 %llu ns | p99 %llu ns | p99.9 %llu ns\n",
              (unsigned long long)hist.pct_ns(0.50), (unsigned long long)hist.pct_ns(0.95),
              (unsigned long long)hist.pct_ns(0.99), (unsigned long long)hist.pct_ns(0.999));
  std::printf("clock overhead: ~%.1f ns per steady_clock::now() call\n", clock_overhead_ns());
  return 0;
}
