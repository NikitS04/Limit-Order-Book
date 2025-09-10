#include "lob/book.hpp"
#include <cassert>
using namespace lob;
int main()
{
  Book b;
  assert(b.addLimit(1, Side::Sell, 100, 10, 1));
  // Buy 4 should partially fill head, leaving 6 at same head
  Qty rem = b.matchBuy(4, 77, 100, 2, [&](TsNs, OrderId, OrderId, Px, Qty) {});
  assert(rem == 0);
  auto a = b.bestAskPx();
  assert(a && *a == 100);
  TopOfBook t = b.top();
  assert(t.ask_sz == 6);
  // Next buy 6 fully clears the head
  rem = b.matchBuy(6, 78, 100, 3, [&](TsNs, OrderId, OrderId, Px, Qty) {});
  assert(rem == 0);
  assert(!b.bestAskPx().has_value());
  return 0;
}