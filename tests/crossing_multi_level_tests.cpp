#include "lob/book.hpp"
#include <cassert>
using namespace lob;
int main()
{
  Book b;
  assert(b.addLimit(1, Side::Sell, 101, 2, 1));
  assert(b.addLimit(2, Side::Sell, 102, 3, 2));
  assert(b.addLimit(3, Side::Sell, 103, 4, 3));
  // Market buy of 8 should sweep 101,102 and part of 103
  Qty rem = b.matchBuy(8, 90, -1, 4, [&](TsNs, OrderId buy_id, OrderId sell_id, Px px, Qty q) {});
  assert(rem == 0);
  auto a = b.bestAskPx();
  assert(a && *a == 103); // remaining at 103
  TopOfBook top = b.top();
  assert(top.ask_px == 103 && top.ask_sz == (4 - (8 - 2 - 3))); // 4 - 3 = 1
  return 0;
}