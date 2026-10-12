#include "lob/book.hpp"
#include "lob/matcher.hpp"
#include "lob/spsc_ring.hpp"
#include <cassert>
#include <atomic>
#include <vector>
using namespace lob;
int main()
{
  // 1) Repricing a bid through the best ask must trade, not leave a crossed book.
  {
    Book b;
    assert(b.addLimit(1, Side::Sell, 105, 5, 1));
    assert(b.addLimit(2, Side::Buy, 100, 5, 2));
    std::vector<Trade> fills;
    assert(b.modify(2, 5, 110, 3, [&](TsNs ts, OrderId buy, OrderId sell, Px px, Qty q)
                    { fills.push_back(Trade{ts, buy, sell, px, q}); }));
    assert(fills.size() == 1);
    assert(fills[0].buy_id == 2 && fills[0].sell_id == 1 && fills[0].px == 105 && fills[0].qty == 5);
    assert(!b.bestBidPx() && !b.bestAskPx());
    assert(b.orderCount() == 0);
  }
  // 2) Partial cross: remainder rests at the new price.
  {
    Book b;
    assert(b.addLimit(1, Side::Sell, 105, 3, 1));
    assert(b.addLimit(2, Side::Buy, 100, 5, 2));
    Qty filled = 0;
    assert(b.modify(2, 5, 110, 3, [&](TsNs, OrderId, OrderId, Px, Qty q) { filled += q; }));
    assert(filled == 3);
    auto t = b.top();
    assert(t.bid_px == 110 && t.bid_sz == 2);
    assert(!b.bestAskPx());
  }
  // 3) Same scenario through the Matcher: trades must reach the output ring.
  {
    SpscRing<Event> in(64);
    SpscRing<Trade> out(64);
    SpscRing<DepthL1> depth(64);
    Book book;
    Metrics m;
    std::atomic<bool> done{false};
    in.try_push(Event{1, Type::Limit, Side::Sell, 1, 105, 5});
    in.try_push(Event{2, Type::Limit, Side::Buy, 2, 100, 5});
    in.try_push(Event{3, Type::Modify, Side::Buy, 2, 110, 5});
    done.store(true);
    Matcher match(in, out, depth, book, m, 0);
    match.run(done);
    Trade t{};
    assert(out.try_pop(t));
    assert(t.buy_id == 2 && t.sell_id == 1 && t.px == 105 && t.qty == 5);
    assert(!out.try_pop(t));
    assert(!book.bestBidPx() && !book.bestAskPx());
  }
  return 0;
}
