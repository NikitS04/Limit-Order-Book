#include "lob/book.hpp"
#include "lob/matcher.hpp"
#include "lob/spsc_ring.hpp"
#include <cassert>
#include <iostream>
#include <atomic>
using namespace lob;
int main()
{
  SpscRing<Event> in(1024);
  SpscRing<Trade> out(1024);
  SpscRing<DepthL1> depth(1024);
  Book book;
  Metrics metrics;
  std::atomic<bool> ingest_done{false};
  Event e1{100, Type::Limit, Side::Sell, 10, 101, 5};
  Event e2{101, Type::Limit, Side::Sell, 11, 102, 5};
  Event e3{102, Type::Limit, Side::Buy, 12, 102, 9};
  in.try_push(e1);
  in.try_push(e2);
  in.try_push(e3);
  ingest_done.store(true);
  Matcher m(in, out, depth, book, metrics, 1);
  m.run(ingest_done);
  Trade t{};
  bool got1 = out.try_pop(t);
  assert(got1 && t.buy_id == 12 && t.sell_id == 10 && t.px == 101 && t.qty == 5);
  bool got2 = out.try_pop(t);
  assert(got2 && t.buy_id == 12 && t.sell_id == 11 && t.px == 102 && t.qty == 4);
  std::cout << "matcher_tests: OK\n";
  return 0;
}