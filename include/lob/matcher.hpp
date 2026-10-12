#pragma once
#include "event.hpp"
#include "trade.hpp"
#include "book.hpp"
#include "spsc_ring.hpp"
#include "metrics.hpp"
#include <atomic>
namespace lob
{
  struct DepthL1
  {
    TsNs ts_ns{};
    Px bid_px{};
    Qty bid_sz{};
    Px ask_px{};
    Qty ask_sz{};
  };
  class Matcher
  {
  public:
    Matcher(SpscRing<Event> &in, SpscRing<Trade> &out, SpscRing<DepthL1> &depth_out, Book &book, Metrics &metrics, int snapshot_every)
        : in_(in), out_(out), depth_out_(depth_out), book_(book), metrics_(metrics), snapshot_every_(snapshot_every) {}
    void run(std::atomic<bool> &ingest_done);
    void process(const Event &e);

  private:
    void emit_trade(TsNs ts, OrderId buy_id, OrderId sell_id, Px px, Qty q);
    void maybe_snapshot(TsNs ts);
    SpscRing<Event> &in_;
    SpscRing<Trade> &out_;
    SpscRing<DepthL1> &depth_out_;
    Book &book_;
    Metrics &metrics_;
    const int snapshot_every_;
    std::uint64_t processed_{0};
  };
}
