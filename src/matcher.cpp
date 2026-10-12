#include "lob/matcher.hpp"
#include "lob/apply.hpp"
#include <chrono>
#include <thread>
namespace lob
{
  void Matcher::run(std::atomic<bool> &ingest_done)
  {
    using Clock = std::chrono::steady_clock;
    Event e;
    while (true)
    {
      bool had = false;
      while (in_.try_pop(e))
      {
        had = true;
        // Note: this window includes two clock reads (~20-40 ns on typical
        // x86 Linux) and any time spent blocked pushing trades to a full
        // output ring. bench/core_bench.cpp measures the engine without either.
        auto t0 = Clock::now();
        process(e);
        auto t1 = Clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        metrics_.match_latency.record_ns((uint64_t)ns);
        ++processed_;
        maybe_snapshot(e.ts_ns);
        metrics_.matched_msgs++;
      }
      if (!had)
      {
        if (ingest_done.load(std::memory_order_acquire) && in_.empty())
          break;
        std::this_thread::yield();
      }
    }
  }

  void Matcher::process(const Event &e)
  {
    apply_event(book_, e, [this](TsNs ts, OrderId buy_id, OrderId sell_id, Px px, Qty q)
                { emit_trade(ts, buy_id, sell_id, px, q); });
  }

  void Matcher::emit_trade(TsNs ts, OrderId buy_id, OrderId sell_id, Px px, Qty q)
  {
    Trade t{ts, buy_id, sell_id, px, q};
    while (!out_.try_push(t))
      std::this_thread::yield();
  }

  void Matcher::maybe_snapshot(TsNs ts)
  {
    if (snapshot_every_ <= 0)
      return;
    if (processed_ % snapshot_every_ == 0)
    {
      auto t = book_.top();
      DepthL1 d{ts, t.bid_px, t.bid_sz, t.ask_px, t.ask_sz};
      while (!depth_out_.try_push(d))
        std::this_thread::yield();
      metrics_.snapshot_count++;
    }
  }
}
