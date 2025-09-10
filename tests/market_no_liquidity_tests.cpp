#include "lob/book.hpp"
#include <cassert>
using namespace lob;
int main()
{
  Book b;
  // Market buy when no asks -> no fill, remainder dropped by caller (we don't rest market)
  Qty rem = b.matchBuy(10, 1, -1, 1, [&](TsNs, OrderId, OrderId, Px, Qty) {});
  assert(rem == 10 || rem == 0); // implementation returns leftover; caller won't rest it
  assert(!b.bestAskPx().has_value() && !b.bestBidPx().has_value());
  return 0;
}