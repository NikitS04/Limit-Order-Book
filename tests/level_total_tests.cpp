#include "lob/book.hpp"
#include <cassert>
using namespace lob;
// top() reads a cached per-level total; it must stay in sync through every
// mutation path.
static Qty walk(const PriceLevel &l)
{
  Qty s = 0;
  for (auto *p = l.head; p; p = p->next)
    s += p->qty;
  return s;
}
static void check(const Book &b)
{
  for (auto &kv : b.bids())
    assert(kv.second.total == walk(kv.second));
  for (auto &kv : b.asks())
    assert(kv.second.total == walk(kv.second));
}
int main()
{
  Book b;
  assert(b.addLimit(1, Side::Sell, 101, 5, 1));
  assert(b.addLimit(2, Side::Sell, 101, 7, 2));
  assert(b.addLimit(3, Side::Sell, 101, 4, 3));
  check(b);
  assert(b.top().ask_sz == 16);
  b.matchBuy(6, 50, 101, 4, [](TsNs, OrderId, OrderId, Px, Qty) {}); // full 1, partial 2
  check(b);
  assert(b.top().ask_sz == 10);
  assert(b.cancel(3)); // tail
  check(b);
  assert(b.top().ask_sz == 6);
  assert(b.modify(2, 2, 101, 5)); // reduce in place
  check(b);
  assert(b.top().ask_sz == 2);
  assert(b.addLimit(4, Side::Buy, 99, 3, 6));
  assert(b.addLimit(5, Side::Buy, 99, 9, 7));
  assert(b.cancel(4)); // head
  check(b);
  assert(b.top().bid_sz == 9);
  return 0;
}
