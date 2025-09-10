#include "lob/matcher.hpp"
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
        auto t0 = Clock::now();
        switch (e.type)
        {
        case Type::Limit:
          onLimit(e);
          break;
        case Type::Market:
          onMarket(e);
          break;
        case Type::Cancel:
          onCancel(e);
          break;
        case Type::Modify:
          onModify(e);
          break;
        }
        auto t1 = Clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        metrics_.match_latency_us.record_ns((uint64_t)ns);
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
  void Matcher::onLimit(const Event &e)
  {
    Qty rem = (e.side == Side::Buy) ? book_.matchBuy(e.qty, e.id, e.px, e.ts_ns, [&](TsNs ts, OrderId buy_id, OrderId sell_id, Px px, Qty q)
                                                     { Trade t{ts,buy_id,sell_id,px,q}; while(!out_.try_push(t)){}; })
                                    : book_.matchSell(e.qty, e.id, e.px, e.ts_ns, [&](TsNs ts, OrderId buy_id, OrderId sell_id, Px px, Qty q)
                                                      { Trade t{ts,buy_id,sell_id,px,q}; while(!out_.try_push(t)){}; });
    if (rem > 0)
      book_.addLimit(e.id, e.side, e.px, rem, e.ts_ns);
  }
  void Matcher::onMarket(const Event &e)
  {
    const Px ignore = -1;
    (void)((e.side == Side::Buy) ? book_.matchBuy(e.qty, e.id, ignore, e.ts_ns, [&](TsNs ts, OrderId buy_id, OrderId sell_id, Px px, Qty q)
                                                  { Trade t{ts,buy_id,sell_id,px,q}; while(!out_.try_push(t)){}; })
                                 : book_.matchSell(e.qty, e.id, ignore, e.ts_ns, [&](TsNs ts, OrderId buy_id, OrderId sell_id, Px px, Qty q)
                                                   { Trade t{ts,buy_id,sell_id,px,q}; while(!out_.try_push(t)){}; }));
  }
  void Matcher::onCancel(const Event &e) { book_.cancel(e.id); }
  void Matcher::onModify(const Event &e) { book_.modify(e.id, e.qty, e.px, e.ts_ns); }
  void Matcher::maybe_snapshot(TsNs ts)
  {
    if (snapshot_every_ <= 0)
      return;
    if (processed_ % snapshot_every_ == 0)
    {
      auto t = book_.top();
      DepthL1 d{ts, t.bid_px, t.bid_sz, t.ask_px, t.ask_sz};
      while (!depth_out_.try_push(d))
      {
      };
      metrics_.snapshot_count++;
    }
  }
}