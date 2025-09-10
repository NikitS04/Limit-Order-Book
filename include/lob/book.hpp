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
  struct PriceLevel
  {
    RestingOrder *head{nullptr};
    RestingOrder *tail{nullptr};
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
  class Book
  {
  public:
    explicit Book(size_t pool_capacity = 2'000'000) : pool_(pool_capacity) {}
    bool addLimit(OrderId, Side, Px, Qty, TsNs);
    bool cancel(OrderId);
    bool modify(OrderId, Qty, Px, TsNs);
    std::optional<Px> bestBidPx() const;
    std::optional<Px> bestAskPx() const;
    TopOfBook top() const;
    template <typename TradeCb>
    Qty matchBuy(Qty, OrderId, Px, TsNs, TradeCb);
    template <typename TradeCb>
    Qty matchSell(Qty, OrderId, Px, TsNs, TradeCb);
    const BidMap &bids() const { return bids_; }
    const AskMap &asks() const { return asks_; }

  private:
    void cleanup_level_if_empty(Side, Px);
    BidMap bids_;
    AskMap asks_;
    std::unordered_map<OrderId, IndexEntry> index_;
    Pool<RestingOrder> pool_;
  };
  template <typename TradeCb>
  Qty Book::matchBuy(Qty qty, OrderId incoming_id, Px limit_px, TsNs ts, TradeCb trade_cb)
  {
    while (qty > 0)
    {
      if (asks_.empty())
        break;
      auto best_it = asks_.begin();
      Px ask_px = best_it->first;
      if (limit_px >= 0 && ask_px > limit_px)
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
      if (limit_px >= 0 && bid_px < limit_px)
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