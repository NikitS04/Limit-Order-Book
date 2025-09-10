#include "lob/book.hpp"
namespace lob
{
  bool Book::addLimit(OrderId id, Side s, Px px, Qty qty, TsNs ts)
  {
    if (qty <= 0)
      return false;
    if (index_.count(id))
      return false;
    PriceLevel *level = (s == Side::Buy) ? &bids_[px] : &asks_[px];
    RestingOrder *node = nullptr;
    try
    {
      node = pool_.alloc();
    }
    catch (...)
    {
      return false;
    }
    node->id = id;
    node->qty = qty;
    node->ts_ns = ts;
    node->prev = node->next = nullptr;
    level->push_back(node);
    index_[id] = IndexEntry{s, px, node};
    return true;
  }
  bool Book::cancel(OrderId id)
  {
    auto it = index_.find(id);
    if (it == index_.end())
      return false;
    auto entry = it->second;
    PriceLevel *level = (entry.side == Side::Buy) ? &bids_[entry.px] : &asks_[entry.px];
    level->erase(entry.node);
    pool_.free(entry.node);
    index_.erase(it);
    cleanup_level_if_empty(entry.side, entry.px);
    return true;
  }
  bool Book::modify(OrderId id, Qty new_qty, Px new_px, TsNs ts)
  {
    auto it = index_.find(id);
    if (it == index_.end())
      return false;
    Side side = it->second.side;
    if (!cancel(id))
      return false;
    return addLimit(id, side, new_px, new_qty, ts);
  }
  std::optional<Px> Book::bestBidPx() const
  {
    if (bids_.empty())
      return std::nullopt;
    return bids_.begin()->first;
  }
  std::optional<Px> Book::bestAskPx() const
  {
    if (asks_.empty())
      return std::nullopt;
    return asks_.begin()->first;
  }
  TopOfBook Book::top() const
  {
    TopOfBook t{};
    if (!bids_.empty())
    {
      t.bid_px = bids_.begin()->first;
      const PriceLevel &lvl = bids_.begin()->second;
      for (RestingOrder *p = lvl.head; p; p = p->next)
        t.bid_sz += p->qty;
    }
    if (!asks_.empty())
    {
      t.ask_px = asks_.begin()->first;
      const PriceLevel &lvl = asks_.begin()->second;
      for (RestingOrder *p = lvl.head; p; p = p->next)
        t.ask_sz += p->qty;
    }
    return t;
  }
  void Book::cleanup_level_if_empty(Side s, Px px)
  {
    if (s == Side::Buy)
    {
      auto it = bids_.find(px);
      if (it != bids_.end() && it->second.empty())
        bids_.erase(it);
    }
    else
    {
      auto it = asks_.find(px);
      if (it != asks_.end() && it->second.empty())
        asks_.erase(it);
    }
  }
}