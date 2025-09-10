#include "lob/book.hpp"
#include <cassert>
#include <unordered_set>
using namespace lob;
int main()
{
  Book b;
  for (int i = 1; i <= 100; ++i)
    assert(b.addLimit(i, (i % 2) ? Side::Buy : Side::Sell, 100 + (i % 5), 1 + (i % 3), i));
  // Cancel every 3rd id
  for (int i = 3; i <= 100; i += 3)
    assert(b.cancel(i));
  // Walk maps and count resting ids
  std::unordered_set<OrderId> ids;
  for (auto &kv : b.bids())
  {
    for (RestingOrder *p = kv.second.head; p; p = p->next)
      ids.insert(p->id);
  }
  for (auto &kv : b.asks())
  {
    for (RestingOrder *p = kv.second.head; p; p = p->next)
      ids.insert(p->id);
  }
  // Expect 100 - floor(100/3) remaining
  size_t expected = 100 - (100 / 3);
  assert(ids.size() == expected);
  return 0;
}