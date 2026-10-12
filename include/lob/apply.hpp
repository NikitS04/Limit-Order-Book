#pragma once
#include "book.hpp"
#include "event.hpp"
namespace lob
{
  // Applies one event to the book. This is the whole matching engine; the
  // threaded pipeline (Matcher) and the core-only benchmark both call it, so
  // the benchmark measures exactly the code the pipeline runs.
  //
  //  Limit  : match against the opposite side up to the limit price, rest remainder
  //  Market : match with no price limit, unfilled remainder is discarded (IOC)
  //  Cancel : remove the resting order
  //  Modify : see Book::modify
  template <typename TradeCb>
  inline void apply_event(Book &book, const Event &e, TradeCb &&cb)
  {
    switch (e.type)
    {
    case Type::Limit:
    {
      if (e.qty <= 0 || e.px < 0)
        return;
      Qty rem = (e.side == Side::Buy) ? book.matchBuy(e.qty, e.id, e.px, e.ts_ns, cb)
                                      : book.matchSell(e.qty, e.id, e.px, e.ts_ns, cb);
      if (rem > 0)
        book.addLimit(e.id, e.side, e.px, rem, e.ts_ns);
      return;
    }
    case Type::Market:
      if (e.qty <= 0)
        return;
      (void)((e.side == Side::Buy) ? book.matchBuy(e.qty, e.id, kNoLimit, e.ts_ns, cb)
                                   : book.matchSell(e.qty, e.id, kNoLimit, e.ts_ns, cb));
      return;
    case Type::Cancel:
      book.cancel(e.id);
      return;
    case Type::Modify:
      if (e.px < 0)
        return;
      book.modify(e.id, e.qty, e.px, e.ts_ns, cb);
      return;
    }
  }
}
