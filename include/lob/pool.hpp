#pragma once
#include <vector>
#include <cstddef>
namespace lob
{
  // Fixed-capacity object pool. All storage is allocated up front; alloc/free
  // never touch the heap. alloc() returns nullptr when the pool is exhausted
  // so the hot path never throws.
  template <typename T>
  class Pool
  {
    std::vector<T> storage_;
    std::vector<size_t> free_;

  public:
    explicit Pool(size_t cap) : storage_(cap), free_(cap)
    {
      for (size_t i = 0; i < cap; ++i)
        free_[i] = cap - 1 - i;
    }
    T *alloc() noexcept
    {
      if (free_.empty())
        return nullptr;
      size_t idx = free_.back();
      free_.pop_back();
      return &storage_[idx];
    }
    void free(T *p) noexcept
    {
      size_t idx = static_cast<size_t>(p - storage_.data());
      free_.push_back(idx); // capacity reserved in ctor, never reallocates
    }
    size_t capacity() const noexcept { return storage_.size(); }
    size_t available() const noexcept { return free_.size(); }
  };
}
