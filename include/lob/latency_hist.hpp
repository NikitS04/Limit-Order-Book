#pragma once
#include <array>
#include <cstdint>

namespace lob {

// 1,000,000 ns = 1 ms; overflow bin holds anything >= 1 ms
struct LatencyHist {
  static constexpr uint32_t MAX_NS = 1'000'000;
  std::array<uint64_t, MAX_NS + 1> bins{}; // [0..MAX_NS-1] in ns, MAX_NS = overflow
  uint64_t total{0};

  inline void record_ns(uint64_t ns) {
    uint32_t b = (ns > MAX_NS) ? MAX_NS : static_cast<uint32_t>(ns);
    ++bins[b];
    ++total;
  }

  // percentile in *nanoseconds*
  uint64_t pct_ns(double p) const {
    if (!total) return 0;
    uint64_t target = static_cast<uint64_t>(p * (total - 1));
    uint64_t acc = 0;
    for (uint32_t i = 0; i <= MAX_NS; ++i) {
      acc += bins[i];
      if (acc > target) return i;
    }
    return 0;
  }
};

} // namespace lob
