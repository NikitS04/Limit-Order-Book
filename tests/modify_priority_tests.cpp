#include "lob/book.hpp"
#include <cassert>
#include <vector>
using namespace lob;
static std::vector<OrderId> sell_into_bids(Book &b, Qty qty, Px px)
{
  std::vector<OrderId> hit;
  b.matchSell(qty, 999, px, 100, [&](TsNs, OrderId buy, OrderId, Px, Qty) { hit.push_back(buy); });
  return hit;
}
int main()
{
  // Reducing size at the same price keeps queue position.
  {
    Book b;
    assert(b.addLimit(1, Side::Buy, 100, 10, 1));
    assert(b.addLimit(2, Side::Buy, 100, 10, 2));
    assert(b.modify(1, 4, 100, 3));
    assert(b.top().bid_sz == 14);
    auto hit = sell_into_bids(b, 5, 100);
    assert(hit.size() == 2 && hit[0] == 1 && hit[1] == 2);
  }
  // Increasing size loses priority.
  {
    Book b;
    assert(b.addLimit(1, Side::Buy, 100, 10, 1));
    assert(b.addLimit(2, Side::Buy, 100, 10, 2));
    assert(b.modify(1, 20, 100, 3));
    assert(b.top().bid_sz == 30);
    auto hit = sell_into_bids(b, 5, 100);
    assert(hit.size() == 1 && hit[0] == 2);
  }
  // Price change loses priority even without crossing.
  {
    Book b;
    assert(b.addLimit(1, Side::Buy, 100, 10, 1));
    assert(b.addLimit(2, Side::Buy, 101, 10, 2));
    assert(b.addLimit(3, Side::Buy, 101, 10, 3));
    assert(b.modify(2, 10, 100, 4));
    assert(b.modify(2, 10, 101, 5));
    auto hit = sell_into_bids(b, 15, 101);
    assert(hit.size() == 2 && hit[0] == 3 && hit[1] == 2);
  }
  // new_qty <= 0 cancels.
  {
    Book b;
    assert(b.addLimit(1, Side::Sell, 105, 10, 1));
    assert(b.modify(1, 0, 105, 2));
    assert(!b.bestAskPx() && b.orderCount() == 0);
  }
  // Unknown id is rejected.
  {
    Book b;
    assert(!b.modify(42, 5, 100, 1));
  }
  return 0;
}
