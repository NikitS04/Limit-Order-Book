#include "lob/book.hpp"
namespace lob
{
  bool Book::addLimit(OrderId id, Side s, Px px, Qty qty, TsNs ts)
  {
    if (qty <= 0 || px < 0)
      return false;
    if (index_.count(id))
      return false;
    // Allocate before touching the maps so an exhausted pool can't leave an
    // empty price level behind.
    RestingOrder *node = pool_.alloc();
    if (!node)
      return false;
    node->id = id;
    node->qty = qty;
    node->ts_ns = ts;
    node->prev = node->next = nullptr;
    PriceLevel *level = (s == Side::Buy) ? &bids_[px] : &asks_[px];
    level->push_back(node);
    index_.emplace(id, IndexEntry{s, px, node});
    return true;
  }

  bool Book::cancel(OrderId id)
  {
    auto it = index_.find(id);
    if (it == index_.end())
      return false;
    const IndexEntry entry = it->second;
    PriceLevel *level = levelOf(entry.side, entry.px);
    level->erase(entry.node);
    pool_.free(entry.node);
    index_.erase(it);
    cleanup_level_if_empty(entry.side, entry.px);
    return true;
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
      t.bid_sz = bids_.begin()->second.total;
    }
    if (!asks_.empty())
    {
      t.ask_px = asks_.begin()->first;
      t.ask_sz = asks_.begin()->second.total;
    }
    return t;
  }

  // Only called for orders present in the index, so the level must exist.
  // Uses find() rather than operator[] so a bug can't silently create levels.
  PriceLevel *Book::levelOf(Side s, Px px)
  {
    if (s == Side::Buy)
      return &bids_.find(px)->second;
    return &asks_.find(px)->second;
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
