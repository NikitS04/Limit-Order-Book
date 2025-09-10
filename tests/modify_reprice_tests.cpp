#include "lob/book.hpp"
#include <cassert>
using namespace lob;
int main()
{
  Book b;
  assert(b.addLimit(10, Side::Sell, 105, 5, 1));
  // Modify to better price 103; should move level
  assert(b.modify(10, 5, 103, 2));
  auto a = b.bestAskPx();
  assert(a && *a == 103);
  // Another order at 103 after -> FIFO should be 10 first
  assert(b.addLimit(11, Side::Sell, 103, 2, 3));
  // Incoming buy 6 @ 103: fills id 10 then id 11
  Qty rem = b.matchBuy(6, 99, 103, 4,
                       [&](TsNs, OrderId buy_id, OrderId sell_id, Px, Qty q)
                       {
                         static int step = 0;
                         if (step == 0)
                         {
                           assert(sell_id == 10 && q == 5);
                         }
                         if (step == 1)
                         {
                           assert(sell_id == 11 && q == 1);
                         }
                         ++step;
                       });
  assert(rem == 0);
  return 0;
}