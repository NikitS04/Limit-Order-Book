#include "lob/book.hpp"
#include <cassert>
#include <iostream>
using namespace lob;
int main()
{
  Book b;
  assert(b.addLimit(1, Side::Buy, 100, 10, 1));
  assert(b.addLimit(2, Side::Buy, 100, 5, 2));
  auto top = b.top();
  assert(top.bid_px == 100 && top.bid_sz == 15);
  assert(b.addLimit(3, Side::Sell, 105, 7, 3));
  auto a = b.bestAskPx();
  assert(a && *a == 105);
  assert(b.cancel(2));
  top = b.top();
  assert(top.bid_sz == 10);
  assert(b.modify(3, 8, 104, 4));
  a = b.bestAskPx();
  assert(a && *a == 104);
  std::cout << "book_tests: OK\n";
  return 0;
}