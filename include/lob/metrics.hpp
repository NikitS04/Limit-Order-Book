#pragma once
#include <cstdint>
#include <chrono>
#include "latency_hist.hpp"
namespace lob
{
  struct Metrics
  {
    uint64_t input_msgs{0}, matched_msgs{0}, output_msgs{0}, snapshot_count{0};
    double ingest_msgs_per_s{0.0}, match_msgs_per_s{0.0}, output_msgs_per_s{0.0};
    std::chrono::steady_clock::time_point t0_ingest{}, t1_ingest{}, t0_match{}, t1_match{}, t0_output{}, t1_output{};
    LatencyHist match_latency_us;
  };
}