#pragma once

#include <cstddef>

class RingBuffer {
 public:
  explicit RingBuffer(size_t capacity) : size_max_(capacity) {
    arr_ = new int[size_max_];
  }

  size_t Size() const { return size_now_; }

  bool Empty() const { return (size_now_ == 0); }

  bool TryPush(int element) {
    if (size_now_ == size_max_) {
      return false;
    }
    arr_[end_] = element;
    end_ = (end_ + 1) % size_max_;
    ++size_now_;
    return true;
  }

  bool TryPop(int* element) {
    if (size_now_ == 0) {
      return false;
    }
    *element = arr_[begin_];
    begin_ = (begin_ + 1) % size_max_;
    --size_now_;
    return true;
  }

  ~RingBuffer() { delete[] arr_; }

 private:
  size_t size_max_;
  size_t size_now_ = 0;
  size_t begin_ = 0;
  size_t end_ = 0;
  int* arr_;
};
