#include "lob/book.hpp"
#include "lob/matcher.hpp"
#include "lob/spsc_ring.hpp"
#include <cassert>
#include <atomic>
using namespace lob;
int main()
{
  SpscRing<Event> in(1024);
  SpscRing<Trade> out(1024);
  SpscRing<DepthL1> depth(2048);
  Book book;
  Metrics m;
  std::atomic<bool> ingest_done{false};
  // 5 events
  in.try_push(Event{1, Type::Limit, Side::Buy, 1, 100, 1});
  in.try_push(Event{2, Type::Limit, Side::Sell, 2, 101, 1});
  in.try_push(Event{3, Type::Limit, Side::Buy, 3, 101, 1}); // crosses -> trade
  in.try_push(Event{4, Type::Limit, Side::Sell, 4, 102, 1});
  in.try_push(Event{5, Type::Limit, Side::Buy, 5, 102, 1}); // crosses -> trade
  ingest_done.store(true);
  Matcher match(in, out, depth, book, m, /*snapshot_every*/ 1);
  match.run(ingest_done);
  // depth should have snapshot for every processed event (5)
  DepthL1 d;
  int count = 0;
  while (depth.try_pop(d))
    ++count;
  assert(count == 5);
  return 0;
}