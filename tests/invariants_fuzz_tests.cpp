#include "lob/apply.hpp"
#include <cassert>
#include <random>
#include <unordered_set>
#include <vector>
using namespace lob;

// Drives a long random stream of add / cancel / modify / market events through
// apply_event and checks the book's invariants after every single event:
//  - never crossed (best bid < best ask)
//  - no empty price levels left in either map
//  - cached level totals equal the sum of their orders
//  - every resting order is in the index at the right side/price, and vice versa
//  - every trade has positive quantity and conserves the incoming order's size
static void check_invariants(const Book &b)
{
  if (b.bestBidPx() && b.bestAskPx())
    assert(*b.bestBidPx() < *b.bestAskPx());
  size_t walked = 0;
  auto walk_side = [&](const auto &levels, Side side)
  {
    for (const auto &[px, lvl] : levels)
    {
      assert(!lvl.empty());
      Qty sum = 0;
      const RestingOrder *prev = nullptr;
      for (const RestingOrder *o = lvl.head; o; o = o->next)
      {
        assert(o->qty > 0);
        assert(o->prev == prev);
        auto e = b.find(o->id);
        assert(e && e->side == side && e->px == px && e->node == o);
        sum += o->qty;
        prev = o;
        ++walked;
      }
      assert(lvl.tail == prev);
      assert(lvl.total == sum);
    }
  };
  walk_side(b.bids(), Side::Buy);
  walk_side(b.asks(), Side::Sell);
  assert(walked == b.orderCount());
}

int main()
{
  for (uint32_t seed = 1; seed <= 20; ++seed)
  {
    std::mt19937 rng(seed);
    auto uni = [&](int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng); };
    Book b(100'000);
    std::vector<OrderId> ids;
    OrderId next_id = 1;
    for (int i = 0; i < 5'000; ++i)
    {
      Event e{};
      e.ts_ns = (TsNs)i;
      e.side = uni(0, 1) ? Side::Buy : Side::Sell;
      int r = uni(0, 99);
      if (ids.empty() || r < 45)
      {
        e.type = Type::Limit;
        e.id = next_id++;
        e.px = 100 + uni(-10, 10); // wide overlap, so lots of crossing
        e.qty = uni(1, 20);
        ids.push_back(e.id);
      }
      else if (r < 75)
      {
        e.type = Type::Cancel;
        e.id = ids[uni(0, (int)ids.size() - 1)];
      }
      else if (r < 92)
      {
        e.type = Type::Modify;
        e.id = ids[uni(0, (int)ids.size() - 1)];
        e.px = 100 + uni(-10, 10);
        e.qty = uni(0, 20);
      }
      else
      {
        e.type = Type::Market;
        e.id = next_id++;
        e.qty = uni(1, 40);
      }

      Qty traded = 0;
      apply_event(b, e, [&](TsNs, OrderId buy, OrderId sell, Px, Qty q)
                  {
                    assert(q > 0);
                    assert(buy != sell);
                    traded += q; });
      if (e.type == Type::Limit || e.type == Type::Market)
        assert(traded <= e.qty);
      check_invariants(b);
    }
  }
  return 0;
}
