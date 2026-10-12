#include "lob/spsc_ring.hpp"
#include "lob/event.hpp"
#include "lob/trade.hpp"
#include "lob/book.hpp"
#include "lob/matcher.hpp"
#include "lob/metrics.hpp"
#include "lob/csv.hpp"
#include <thread>
#include <fstream>
#include <iostream>
#include <atomic>
#include <cstring>
#include <chrono>
using namespace lob;

struct Cli
{
  std::string in_path{"data/demo_long.csv"};
  std::string trades_path{"trades.csv"};
  std::string depth_path{"depth.csv"};
  std::string metrics_path{"metrics.json"};
  int snapshot_every{1000};
  size_t ring_cap{1 << 20};
};

static Cli parse_cli(int argc, char **argv)
{
  Cli c;
  for (int i = 1; i < argc; ++i)
  {
    if (!std::strcmp(argv[i], "--in") && i + 1 < argc)
      c.in_path = argv[++i];
    else if (!std::strcmp(argv[i], "--trades") && i + 1 < argc)
      c.trades_path = argv[++i];
    else if (!std::strcmp(argv[i], "--depth") && i + 1 < argc)
      c.depth_path = argv[++i];
    else if (!std::strcmp(argv[i], "--metrics") && i + 1 < argc)
      c.metrics_path = argv[++i];
    else if (!std::strcmp(argv[i], "--snapshot-every") && i + 1 < argc)
      c.snapshot_every = std::stoi(argv[++i]);
    else if (!std::strcmp(argv[i], "--ring") && i + 1 < argc)
      c.ring_cap = (size_t)std::stoul(argv[++i]);
  }
  return c;
}

static double seconds(std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b)
{
  return std::chrono::duration<double>(b - a).count();
}

static void write_metrics_json(const std::string &path, const Metrics &m)
{
  const uint64_t p50 = m.match_latency.pct_ns(0.50);
  const uint64_t p95 = m.match_latency.pct_ns(0.95);
  const uint64_t p99 = m.match_latency.pct_ns(0.99);
  const uint64_t p999 = m.match_latency.pct_ns(0.999);
  std::ofstream o(path);
  o << "{\n"
    << "  \"input_msgs\": " << m.input_msgs << ",\n"
    << "  \"matched_msgs\": " << m.matched_msgs << ",\n"
    << "  \"output_msgs\": " << m.output_msgs << ",\n"
    << "  \"snapshot_count\": " << m.snapshot_count << ",\n"
    << "  \"ingest_msgs_per_s\": " << (uint64_t)m.ingest_msgs_per_s << ",\n"
    << "  \"match_msgs_per_s\": " << (uint64_t)m.match_msgs_per_s << ",\n"
    << "  \"output_msgs_per_s\": " << (uint64_t)m.output_msgs_per_s << ",\n"
    << "  \"match_p50_ns\": " << p50 << ",\n"
    << "  \"match_p95_ns\": " << p95 << ",\n"
    << "  \"match_p99_ns\": " << p99 << ",\n"
    << "  \"match_p999_ns\": " << p999 << "\n"
    << "}\n";
}

int main(int argc, char **argv)
{
  Cli cli = parse_cli(argc, argv);
  SpscRing<Event> in(cli.ring_cap);
  SpscRing<Trade> out(cli.ring_cap);
  SpscRing<DepthL1> depth_out(cli.ring_cap);
  Book book;
  Metrics metrics;
  std::atomic<bool> ingest_done{false};
  std::atomic<bool> match_done{false};

  std::thread ingest([&]
                     {
    metrics.t0_ingest = std::chrono::steady_clock::now();
    std::ifstream ifs(cli.in_path);
    if (!ifs)
    {
      std::cerr << "Failed to open input: " << cli.in_path << "\n";
      ingest_done.store(true, std::memory_order_release);
      return;
    }
    std::string line;
    bool header = true;
    while (std::getline(ifs, line))
    {
      if (header)
      {
        header = false;
        continue;
      }
      auto e_opt = parse_event_csv_line(line, false);
      if (!e_opt)
        continue;
      ++metrics.input_msgs;
      while (!in.try_push(*e_opt))
        std::this_thread::yield();
    }
    metrics.t1_ingest = std::chrono::steady_clock::now();
    ingest_done.store(true, std::memory_order_release); });

  std::thread match([&]
                    {
    metrics.t0_match = std::chrono::steady_clock::now();
    Matcher m(in, out, depth_out, book, metrics, cli.snapshot_every);
    m.run(ingest_done);
    metrics.t1_match = std::chrono::steady_clock::now();
    match_done.store(true, std::memory_order_release); });

  std::thread writer([&]
                     {
    metrics.t0_output = std::chrono::steady_clock::now();
    std::ofstream trades(cli.trades_path), depth(cli.depth_path);
    write_trades_csv_header(trades);
    write_depth_csv_header(depth);
    Trade t;
    DepthL1 d;
    while (true)
    {
      bool progressed = false;
      while (out.try_pop(t))
      {
        append_trade_csv(trades, t);
        ++metrics.output_msgs;
        progressed = true;
      }
      while (depth_out.try_pop(d))
      {
        append_depth_csv(depth, d);
        progressed = true;
      }
      if (!progressed)
      {
        if (match_done.load(std::memory_order_acquire) && out.empty() && depth_out.empty())
          break;
        std::this_thread::yield();
      }
    }
    trades.flush();
    depth.flush();
    metrics.t1_output = std::chrono::steady_clock::now(); });

  ingest.join();
  match.join();
  writer.join();

  // All threads joined: metrics are safe to read from here.
  if (double s = seconds(metrics.t0_ingest, metrics.t1_ingest); s > 0)
    metrics.ingest_msgs_per_s = metrics.input_msgs / s;
  if (double s = seconds(metrics.t0_match, metrics.t1_match); s > 0)
    metrics.match_msgs_per_s = metrics.matched_msgs / s;
  if (double s = seconds(metrics.t0_output, metrics.t1_output); s > 0)
    metrics.output_msgs_per_s = metrics.output_msgs / s;
  write_metrics_json(cli.metrics_path, metrics);

  std::cout << "pipeline: ingest " << (uint64_t)metrics.ingest_msgs_per_s
            << " msg/s | match " << (uint64_t)metrics.match_msgs_per_s
            << " msg/s | p50 " << metrics.match_latency.pct_ns(0.50)
            << " ns | p99 " << metrics.match_latency.pct_ns(0.99) << " ns\n"
            << "note: end-to-end rate is bounded by CSV ingest; run lob_bench for core matching throughput\n";
  return 0;
}
