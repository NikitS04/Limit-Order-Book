#include "lob/book.hpp"
#include <cassert>
using namespace lob;
int main()
{
  Book b(2);
  assert(b.addLimit(1, Side::Buy, 100, 1, 1));
  assert(b.addLimit(2, Side::Buy, 100, 1, 2));
  // Pool full: rejected, and no empty level left behind at 101.
  assert(!b.addLimit(3, Side::Buy, 101, 1, 3));
  assert(b.bids().size() == 1);
  assert(*b.bestBidPx() == 100);
  assert(b.orderCount() == 2);
  // Freeing a node makes room again.
  assert(b.cancel(1));
  assert(b.poolAvailable() == 1);
  assert(b.addLimit(3, Side::Buy, 101, 1, 4));
  assert(*b.bestBidPx() == 101);
  return 0;
}
