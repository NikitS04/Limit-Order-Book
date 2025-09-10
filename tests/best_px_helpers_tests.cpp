#include "lob/book.hpp"
#include <cassert>
using namespace lob;
int main()
{
  Book b;
  assert(!b.bestBidPx().has_value());
  assert(!b.bestAskPx().has_value());
  assert(b.addLimit(1, Side::Buy, 100, 1, 1));
  assert(b.addLimit(2, Side::Buy, 101, 1, 2));
  assert(b.addLimit(3, Side::Sell, 105, 1, 3));
  assert(b.addLimit(4, Side::Sell, 104, 2, 4));
  auto bb = b.bestBidPx();
  auto aa = b.bestAskPx();
  assert(bb && *bb == 101);
  assert(aa && *aa == 104);
  // remove best ask by crossing
  (void)b.matchBuy(2, 9, 200, 5, [&](TsNs, OrderId, OrderId, Px, Qty) {});
  aa = b.bestAskPx();
  assert(aa && *aa == 105);
  return 0;
}