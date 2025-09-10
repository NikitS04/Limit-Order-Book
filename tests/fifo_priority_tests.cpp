#include "lob/book.hpp"
#include <cassert>
using namespace lob;
int main()
{
  Book b;
  // two orders same price -> FIFO
  assert(b.addLimit(1, Side::Sell, 105, 3, 1));
  assert(b.addLimit(2, Side::Sell, 105, 4, 2));
  // incoming buy crosses both; first fill must hit id=1 entirely before id=2
  Qty rem = b.matchBuy(5, /*incoming*/ 99, /*limit*/ 105, /*ts*/ 3,[&](TsNs, OrderId buy_id, OrderId sell_id, Px px, Qty q)
    {
      static int step = 0;
      if (step == 0)
      {
        assert(sell_id == 1 && q == 3 && px == 105);
      }
      if (step == 1)
      {
        assert(sell_id == 2 && q == 2 && px == 105);
      }
      ++step;
    });
  assert(rem == 0);
  return 0;
}