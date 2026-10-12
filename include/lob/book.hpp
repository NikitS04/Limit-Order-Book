#pragma once
#include "types.hpp"
#include "trade.hpp"
#include "pool.hpp"
#include <map>
#include <unordered_map>
#include <optional>
#include <algorithm>
namespace lob
{
  struct RestingOrder
  {
    OrderId id{};
    Qty qty{};
    TsNs ts_ns{};
    RestingOrder *prev{nullptr};
    RestingOrder *next{nullptr};
  };

  // FIFO queue of resting orders at one price. `total` caches the summed
  // quantity so top-of-book size is O(1) instead of a walk over the list.
  struct PriceLevel
  {
    RestingOrder *head{nullptr};
    RestingOrder *tail{nullptr};
    Qty total{0};
    bool empty() const { return head == nullptr; }
    void push_back(RestingOrder *n)
    {
      n->next = nullptr;
      n->prev = tail;
      if (tail)
        tail->next = n;
      else
        head = n;
      tail = n;
      total += n->qty;
    }
    RestingOrder *pop_front()
    {
      RestingOrder *n = head;
      if (!n)
        return nullptr;
      head = n->next;
      if (head)
        head->prev = nullptr;
      else
        tail = nullptr;
      n->next = n->prev = nullptr;
      total -= n->qty;
      return n;
    }
    void erase(RestingOrder *n)
    {
      if (!n)
        return;
      if (n->prev)
        n->prev->next = n->next;
      else
        head = n->next;
      if (n->next)
        n->next->prev = n->prev;
      else
        tail = n->prev;
      n->next = n->prev = nullptr;
      total -= n->qty;
    }
  };
  using BidMap = std::map<Px, PriceLevel, std::greater<Px>>;
  using AskMap = std::map<Px, PriceLevel, std::less<Px>>;
  struct TopOfBook
  {
    Px bid_px{0};
    Qty bid_sz{0};
    Px ask_px{0};
    Qty ask_sz{0};
  };
  struct IndexEntry
  {
    Side side{};
    Px px{};
    RestingOrder *node{};
  };

  // Passing this as limit_px to matchBuy/matchSell disables the price check
  // (market orders).
  inline constexpr Px kNoLimit = -1;

  class Book
  {
  public:
    explicit Book(size_t pool_capacity = 2'000'000) : pool_(pool_capacity) {}

    // Rests an order without matching. Callers that may cross must match first.
    bool addLimit(OrderId, Side, Px, Qty, TsNs);
    bool cancel(OrderId);

    // Modify semantics (new_qty is the new total size):
    //  - new_qty <= 0                     -> cancel
    //  - same price and new_qty <= current -> reduce in place, keeps queue priority
    //  - otherwise (reprice or size up)   -> cancel, match against the opposite
    //    side, rest any remainder at the back of the queue. A reprice through
    //    the spread therefore trades instead of leaving a crossed book.
    template <typename TradeCb>
    bool modify(OrderId, Qty new_qty, Px new_px, TsNs, TradeCb);
    bool modify(OrderId id, Qty new_qty, Px new_px, TsNs ts)
    {
      return modify(id, new_qty, new_px, ts, [](TsNs, OrderId, OrderId, Px, Qty) {});
    }

    std::optional<Px> bestBidPx() const;
    std::optional<Px> bestAskPx() const;
    TopOfBook top() const;
    template <typename TradeCb>
    Qty matchBuy(Qty, OrderId, Px, TsNs, TradeCb);
    template <typename TradeCb>
    Qty matchSell(Qty, OrderId, Px, TsNs, TradeCb);
    const BidMap &bids() const { return bids_; }
    const AskMap &asks() const { return asks_; }
    std::optional<IndexEntry> find(OrderId id) const
    {
      auto it = index_.find(id);
      if (it == index_.end())
        return std::nullopt;
      return it->second;
    }
    size_t orderCount() const { return index_.size(); }
    size_t poolAvailable() const { return pool_.available(); }

  private:
    PriceLevel *levelOf(Side, Px);
    void cleanup_level_if_empty(Side, Px);
    BidMap bids_;
    AskMap asks_;
    std::unordered_map<OrderId, IndexEntry> index_;
    Pool<RestingOrder> pool_;
  };

  template <typename TradeCb>
  bool Book::modify(OrderId id, Qty new_qty, Px new_px, TsNs ts, TradeCb trade_cb)
  {
    auto it = index_.find(id);
    if (it == index_.end())
      return false;
    const IndexEntry entry = it->second;
    if (new_qty <= 0)
      return cancel(id);

    if (new_px == entry.px && new_qty <= entry.node->qty)
    {
      PriceLevel *lvl = levelOf(entry.side, entry.px);
      lvl->total -= entry.node->qty - new_qty;
      entry.node->qty = new_qty;
      return true;
    }

    cancel(id);
    Qty rem = (entry.side == Side::Buy) ? matchBuy(new_qty, id, new_px, ts, trade_cb)
                                        : matchSell(new_qty, id, new_px, ts, trade_cb);
    if (rem > 0)
      return addLimit(id, entry.side, new_px, rem, ts);
    return true;
  }

  template <typename TradeCb>
  Qty Book::matchBuy(Qty qty, OrderId incoming_id, Px limit_px, TsNs ts, TradeCb trade_cb)
  {
    while (qty > 0)
    {
      if (asks_.empty())
        break;
      auto best_it = asks_.begin();
      Px ask_px = best_it->first;
      if (limit_px != kNoLimit && ask_px > limit_px)
        break;
      auto &lvl = best_it->second;
      if (lvl.empty())
      {
        asks_.erase(best_it);
        continue;
      }
      RestingOrder *head = lvl.head;
      Qty fill = std::min(qty, head->qty);
      head->qty -= fill;
      lvl.total -= fill;
      qty -= fill;
      trade_cb(ts, incoming_id, head->id, ask_px, fill);
      if (head->qty == 0)
      {
        lvl.pop_front();
        index_.erase(head->id);
        pool_.free(head);
        if (lvl.empty())
          asks_.erase(best_it);
      }
    }
    return qty;
  }
  template <typename TradeCb>
  Qty Book::matchSell(Qty qty, OrderId incoming_id, Px limit_px, TsNs ts, TradeCb trade_cb)
  {
    while (qty > 0)
    {
      if (bids_.empty())
        break;
      auto best_it = bids_.begin();
      Px bid_px = best_it->first;
      if (limit_px != kNoLimit && bid_px < limit_px)
        break;
      auto &lvl = best_it->second;
      if (lvl.empty())
      {
        bids_.erase(best_it);
        continue;
      }
      RestingOrder *head = lvl.head;
      Qty fill = std::min(qty, head->qty);
      head->qty -= fill;
      lvl.total -= fill;
      qty -= fill;
      trade_cb(ts, head->id, incoming_id, bid_px, fill);
      if (head->qty == 0)
      {
        lvl.pop_front();
        index_.erase(head->id);
        pool_.free(head);
        if (lvl.empty())
          bids_.erase(best_it);
      }
    }
    return qty;
  }
}
