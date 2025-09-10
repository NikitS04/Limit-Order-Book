#pragma once
#include <atomic>
#include <cstddef>
#include <vector>
#include <type_traits>
namespace lob
{
  template <typename T>
  class SpscRing
  {
    static_assert(std::is_nothrow_move_constructible_v<T> || std::is_trivially_copyable_v<T>);

  public:
    explicit SpscRing(size_t c) : cap_(round_up_pow2(c)), mask_(cap_ - 1), buf_(cap_) {}
    bool try_push(const T &v) noexcept
    {
      auto h = head_.load(std::memory_order_relaxed);
      auto n = (h + 1) & mask_;
      if (n == tail_.load(std::memory_order_acquire))
        return false;
      buf_[h] = v;
      head_.store(n, std::memory_order_release);
      return true;
    }
    bool try_push(T &&v) noexcept
    {
      auto h = head_.load(std::memory_order_relaxed);
      auto n = (h + 1) & mask_;
      if (n == tail_.load(std::memory_order_acquire))
        return false;
      buf_[h] = std::move(v);
      head_.store(n, std::memory_order_release);
      return true;
    }
    bool try_pop(T &out) noexcept
    {
      auto t = tail_.load(std::memory_order_relaxed);
      if (t == head_.load(std::memory_order_acquire))
        return false;
      out = std::move(buf_[t]);
      tail_.store((t + 1) & mask_, std::memory_order_release);
      return true;
    }
    bool empty() const noexcept { return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire); }
    size_t capacity() const noexcept { return cap_ - 1; }

  private:
    static size_t round_up_pow2(size_t x)
    {
      size_t p = 1;
      while (p < x)
        p <<= 1;
      return p;
    }
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
    const size_t cap_, mask_;
    std::vector<T> buf_;
  };
}