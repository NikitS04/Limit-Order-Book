#include "lob/book.hpp"
#include <cassert>
using namespace lob;
int main()
{
  Book b;
  // three orders on same price level
  assert(b.addLimit(1, Side::Buy, 100, 5, 1));
  assert(b.addLimit(2, Side::Buy, 100, 6, 2));
  assert(b.addLimit(3, Side::Buy, 100, 7, 3));
  // cancel middle
  assert(b.cancel(2));
  TopOfBook t = b.top();
  assert(t.bid_px == 100 && t.bid_sz == (5 + 7));
  // cancel head
  assert(b.cancel(1));
  t = b.top();
  assert(t.bid_sz == 7);
  // cancel tail (now same as head)
  assert(b.cancel(3));
  assert(!b.bestBidPx().has_value());
  return 0;
}