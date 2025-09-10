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
    void onLimit(const Event &e);
    void onMarket(const Event &e);
    void onCancel(const Event &e);
    void onModify(const Event &e);

  private:
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