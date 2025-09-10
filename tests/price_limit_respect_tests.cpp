#include "lob/book.hpp"
#include <cassert>
using namespace lob;
int main()
{
  Book b;
  assert(b.addLimit(1, Side::Sell, 101, 5, 1));
  // Buy limit 100 should NOT trade at 101
  Qty rem = b.matchBuy(5, 10, 100, 2, [&](TsNs, OrderId, OrderId, Px, Qty)
                       { assert(false && "Should not trade"); });
  // Entire qty remains, caller will rest it later if needed
  assert(rem == 5);
  // Ensure ask still there
  auto a = b.bestAskPx();
  assert(a && *a == 101);
  return 0;
}