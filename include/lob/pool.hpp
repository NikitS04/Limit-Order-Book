#pragma once
#include <vector>
#include <cstddef>
#include <stdexcept>
namespace lob
{
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
    T *alloc()
    {
      if (free_.empty())
        throw std::runtime_error("Pool exhausted");
      size_t idx = free_.back();
      free_.pop_back();
      return &storage_[idx];
    }
    void free(T *p)
    {
      size_t idx = static_cast<size_t>(p - storage_.data());
      free_.push_back(idx);
    }
    size_t capacity() const { return storage_.size(); }
  };
}