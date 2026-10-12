#pragma once
#include <vector>
#include <cstdint>

namespace lob {

// 1 ns buckets up to 1 ms; the last bin is an overflow bucket for >= 1 ms.
// Heap-backed: 8 MB of bins as a std::array blew the stack whenever a
// Metrics object was declared as a local.
struct LatencyHist {
  static constexpr uint32_t MAX_NS = 1'000'000;
  std::vector<uint64_t> bins = std::vector<uint64_t>(MAX_NS + 1, 0);
  uint64_t total{0};

  inline void record_ns(uint64_t ns) {
    uint32_t b = (ns > MAX_NS) ? MAX_NS : static_cast<uint32_t>(ns);
    ++bins[b];
    ++total;
  }

  // percentile in nanoseconds, p in [0, 1]
  uint64_t pct_ns(double p) const {
    if (!total) return 0;
    uint64_t target = static_cast<uint64_t>(p * (total - 1));
    uint64_t acc = 0;
    for (uint32_t i = 0; i <= MAX_NS; ++i) {
      acc += bins[i];
      if (acc > target) return i;
    }
    return MAX_NS;
  }
};

} // namespace lob
