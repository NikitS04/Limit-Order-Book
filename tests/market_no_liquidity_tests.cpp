#include "lob/apply.hpp"
#include <cassert>
using namespace lob;
int main()
{
  auto noop = [](TsNs, OrderId, OrderId, Px, Qty) {};
  // matchBuy with nothing on the other side returns the full quantity.
  {
    Book b;
    Qty rem = b.matchBuy(10, 1, kNoLimit, 1, noop);
    assert(rem == 10);
    assert(!b.bestAskPx() && !b.bestBidPx());
  }
  // A market order's unfilled remainder is discarded (IOC), never rested.
  {
    Book b;
    apply_event(b, Event{1, Type::Market, Side::Buy, 1, 0, 10}, noop);
    assert(!b.bestAskPx() && !b.bestBidPx() && b.orderCount() == 0);
  }
  // Partial fill: sweeps available liquidity, remainder discarded.
  {
    Book b;
    assert(b.addLimit(1, Side::Sell, 101, 3, 1));
    Qty filled = 0;
    apply_event(b, Event{2, Type::Market, Side::Buy, 2, 0, 10},
                [&](TsNs, OrderId, OrderId, Px, Qty q) { filled += q; });
    assert(filled == 3);
    assert(!b.bestAskPx() && !b.bestBidPx() && b.orderCount() == 0);
  }
  return 0;
}
