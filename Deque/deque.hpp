
#pragma once
#include <stdlib.h>

#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

#include "cstddef"

template <typename T, typename Allocator = std::allocator<T>>
class Deque {
 public:
  template <bool IsConst>
  class common_iterator;

  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  using allocator_type = Allocator;
  using alloc_traits = std::allocator_traits<Allocator>;
  using value_type = T;

  Deque() {}

  Deque(const Allocator& alloc) : alloc_(alloc) {}

  Deque(size_t count, const Allocator& alloc = Allocator())
      : alloc_(alloc),
        node_((count + kSize - 1) / kSize),
        middle_(node_ / 2),
        begin_(iterator(&big_arr_, -middle_, nullptr, &middle_)),
        end_(iterator(&big_arr_, node_ - middle_ - 1, nullptr, &middle_)),
        now_sz_(count) {
    big_arr_.resize(node_);
    assign_arr(0, node_);
    if (now_sz_ != 0) {
      begin_.curr_ = big_arr_[middle_ + begin_.index_];
      end_.curr_ = big_arr_[middle_ + end_.index_] + (count - 1) % kSize;
      ++end_;
    }
    filling();
  }

  Deque(size_t size, const T& value, const Allocator& alloc = Allocator())
      : Deque(size, alloc) {
    for (iterator iter = begin_; iter != end_; ++iter) {
      try {
        alloc_traits::construct(alloc_, iter.curr_, std::move(value));
      } catch (...) {
        for (iterator iter_del = iter; iter_del - begin_ > 0; --iter_del) {
          alloc_traits::destroy(alloc_, iter_del.curr_);
        }
        for (int64_t counter = 0; counter < end_.index_ - begin_.index_;
             ++counter) {
          alloc_traits::deallocate(
              alloc_, big_arr_[middle_ + begin_.index_ + counter], kSize);
        }
        throw;
      }
    }
  }

  Deque(const Deque& other)
      : alloc_(
            alloc_traits::select_on_container_copy_construction(other.alloc_)),
        now_sz_(other.now_sz_),
        node_(other.node_),
        middle_(other.middle_),
        begin_(iterator(&big_arr_, other.begin_.index_, nullptr, &middle_)),
        end_(iterator(&big_arr_, other.end_.index_, nullptr, &middle_)) {
    big_arr_.resize(node_);
    if (empty()) {
      big_arr_.reserve(0);
      return;
    }
    assign_arr(begin_.index_ + middle_, end_.index_ + 1 + middle_);
    begin_.curr_ = big_arr_[middle_ + begin_.index_];
    end_.curr_ = big_arr_[middle_ + end_.index_] + (now_sz_ - 1) % kSize;
    ++end_;
    filling_in_copy(other.begin_);
  }

  Deque(Deque&& other)
      : now_sz_(other.now_sz_),
        alloc_(
            alloc_traits::select_on_container_copy_construction(other.alloc_)),
        big_arr_(other.big_arr_),
        node_(other.node_),
        middle_(other.middle_),
        begin_(other.begin_),
        end_(other.end_) {
    begin_.ptr_arr_ = &big_arr_;
    begin_.middle_ = &middle_;
    end_.ptr_arr_ = &big_arr_;
    end_.middle_ = &middle_;
    other.big_arr_.clear();
    other.middle_ = 0;
    other.begin_ = iterator();
    other.end_ = iterator();
    other.node_ = 0;
    other.now_sz_ = 0;
  }

  Deque(std::initializer_list<T> init, const Allocator& alloc = Allocator())
      : Deque(init.size(), alloc) {
    for (iterator iter = begin_; iter != end_; ++iter) {
      alloc_traits::destroy(alloc_, iter.curr_);
    }
    if (empty()) {
      big_arr_.reserve(0);
      return;
    }
    iterator iter = begin_;
    for (auto iter_2 = init.begin(); iter_2 != init.end(); ++iter_2, ++iter) {
      try {
        alloc_traits::construct(alloc_, iter.curr_, *(iter_2));
      } catch (...) {
        for (iterator iter_del = iter; iter_del - begin_ > 0; --iter_del) {
          alloc_traits::destroy(alloc_, iter_del.curr_);
        }
        for (int64_t counter = 0; counter < end_.index_ - begin_.index_ + 1;
             ++counter) {
          alloc_traits::deallocate(
              alloc_, big_arr_[middle_ + begin_.index_ + counter], kSize);
        }
        throw;
      }
    }
  }

  ~Deque() {
    if (!big_arr_.empty()) {
      bool statement = (end_.curr_ - big_arr_[middle_ + end_.index_] == 0);
      for (iterator iter = begin_; iter != end_; ++iter) {
        alloc_traits::destroy(alloc_, iter.curr_);
      }
      if (statement) {
        --end_;
      }
      for (int64_t counter = 0; counter < end_.index_ - begin_.index_ + 1;
           ++counter) {
        alloc_traits::deallocate(
            alloc_, big_arr_[middle_ + begin_.index_ + counter], kSize);
      }
    }
  }

  Deque& operator=(const Deque& other) {
    if (alloc_traits::propagate_on_container_copy_assignment::value &&
        alloc_ != other.alloc_) {
      alloc_ = other.alloc_;
    }
    Deque copy(other);
    big_swap(copy, *this);
    return *this;
  }

  Deque& operator=(Deque&& other) {
    Deque copy = std::move(other);
    if (alloc_traits::propagate_on_container_copy_assignment::value &&
        alloc_ != other.alloc_) {
      alloc_ = other.alloc_;
    }
    big_swap(copy, *this);
    return *this;
  }

  size_t size() const { return now_sz_; }

  bool empty() const { return now_sz_ == 0; }

  T& operator[](size_t index) {
    int64_t ind = index;
    return *(begin_ + ind);
  }

  const T& operator[](size_t index) const {
    int64_t ind = index;
    return *(begin_ + ind);
  }

  T& at(size_t index) {
    if (index >= now_sz_) {
      throw std::out_of_range("...");
    }
    int64_t ind = index;
    return *(begin_ + ind);
  }

  const T& at(size_t index) const {
    if (index >= now_sz_) {
      throw std::out_of_range("...");
    }
    int64_t ind = index;
    return *(begin_ + ind);
  }

  void push_back(const T& value) {
    if (empty()) {
      Deque mine(1);
      alloc_traits::destroy(alloc_, mine.begin_.curr_);
      *this = mine;
      alloc_traits::construct(alloc_, begin_.curr_, value);
      return;
    }
    add_back(*this, value);
  }

  void push_front(const T& value) {
    if (empty()) {
      Deque mine(1);
      alloc_traits::destroy(alloc_, mine.begin_.curr_);
      *this = mine;
      alloc_traits::construct(alloc_, begin_.curr_, value);
      return;
    }
    add_front(*this, value);
  }

  void push_back(T&& value) {
    if (empty()) {
      Deque mine(1);
      alloc_traits::destroy(alloc_, mine.begin_.curr_);
      *this = std::move(mine);
      alloc_traits::construct(alloc_, begin_.curr_, std::move(value));
      return;
    }
    add_back(*this, std::move(value));
  }

  void push_front(T&& value) {
    if (empty()) {
      Deque mine(1);
      alloc_traits::destroy(alloc_, mine.begin_.curr_);
      *this = std::move(mine);
      alloc_traits::construct(alloc_, begin_.curr_, std::move(value));
      return;
    }
    add_front(*this, std::move(value));
  }

  void pop_back() {
    --end_;
    alloc_traits::destroy(alloc_, end_.curr_);
    if (end_.curr_ - big_arr_[middle_ + end_.index_] == 0) {
      alloc_traits::deallocate(alloc_, big_arr_[middle_ + end_.index_], kSize);
    }
    --now_sz_;
  }

  void pop_front() {
    alloc_traits::destroy(alloc_, begin_.curr_);
    if (begin_.curr_ - big_arr_[middle_ + begin_.index_] == kSize - 1) {
      alloc_traits::deallocate(alloc_, big_arr_[middle_ + begin_.index_],
                               kSize);
    }
    ++begin_;
    --now_sz_;
  }

  void insert(iterator iter, const T& value) {
    if (big_arr_.empty()) {
      Deque mine(1);
      alloc_traits::destroy(alloc_, mine.begin_.curr_);
      *this = mine;
      alloc_traits::construct(alloc_, begin_.curr_, std::move(value));
      return;
    }
    if (end_.curr_ - big_arr_[middle_ + end_.index_] == kSize - 1) {
      if (end_.index_ == middle_) {
        realloc();
      }
      big_arr_[middle_ + end_.index_ + 1] =
          alloc_traits::allocate(alloc_, kSize);
    }
    for (iterator iter_2 = end_; iter_2 > iter; --iter_2) {
      *iter_2 = *(iter_2 - 1);
    }
    alloc_traits::construct(alloc_, iter.curr_, value);
    ++end_;
    ++now_sz_;
  }

  void erase(iterator iter) {
    alloc_traits::destroy(alloc_, iter.curr_);
    iterator iter_fir = iter;
    iterator iter_sec = ++iter;
    for (; iter_sec != end_; ++iter_fir, ++iter_sec) {
      *iter_fir = *iter_sec;
    }
    if (end_.curr_ - big_arr_[middle_ + end_.index_] == 0) {
      alloc_traits::deallocate(alloc_, big_arr_[middle_ + end_.index_], kSize);
    }
    --end_;
    alloc_traits::destroy(alloc_, end_.curr_);
    --now_sz_;
  }

  template <typename... Args>
  void emplace_back(Args&&... args) {
    if (empty()) {
      Deque mine(1);
      alloc_traits::destroy(alloc_, mine.begin_.curr_);
      *this = std::move(mine);
      alloc_traits::construct(alloc_, begin_.curr_,
                              std::forward<Args>(args)...);
      return;
    }
    add_back(*this, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void emplace_front(Args&&... args) {
    if (empty()) {
      Deque mine(1);
      alloc_traits::destroy(alloc_, mine.begin_.curr_);
      *this = std::move(mine);
      alloc_traits::construct(alloc_, begin_.curr_,
                              std::forward<Args>(args)...);
      return;
    }
    add_front(*this, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void emplace(iterator iter, Args&&... args) {
    if (empty()) {
      Deque mine(1);
      alloc_traits::destroy(alloc_, mine.begin_.curr_);
      *this = std::move(mine);
      alloc_traits::construct(alloc_, begin_.curr_,
                              std::forward<Args>(args)...);
      return;
    }
    if (end_.curr_ - big_arr_[middle_ + end_.index_] == kSize - 1) {
      if (end_.index_ == middle_) {
        realloc();
      }
      big_arr_[middle_ + end_.index_ + 1] =
          alloc_traits::allocate(alloc_, kSize);
    }
    for (iterator iter_2 = end_; iter_2 > iter; --iter_2) {
      *iter_2 = *(iter_2 - 1);
    }
    alloc_traits::construct(alloc_, iter.curr_, std::forward<Args>(args)...);
    ++end_;
    ++now_sz_;
  }

  iterator begin() { return begin_; }

  iterator end() { return end_; }

  const_iterator cbegin() const {
    const_iterator const_begin(begin_.ptr_arr_, begin_.index_, begin_.curr_,
                               begin_.middle_);
    return const_begin;
  }

  const_iterator cend() const {
    const_iterator const_end(end_.ptr_arr_, end_.index_, end_.curr_,
                             end_.middle_);
    return const_end;
  }

  const_reverse_iterator rbegin() const {
    const_iterator const_end(end_.ptr_arr_, end_.index_, end_.curr_,
                             end_.middle_);
    return std::make_reverse_iterator(const_end);
  }

  const_reverse_iterator rend() const {
    const_iterator const_begin(begin_.ptr_arr_, begin_.index_, begin_.curr_,
                               begin_.middle_);
    return std::make_reverse_iterator(const_begin);
  }

  reverse_iterator rbegin() { return std::make_reverse_iterator(end_); }

  reverse_iterator rend() { return std::make_reverse_iterator(begin_); }

  const_reverse_iterator crbegin() const {
    const_iterator const_end(end_.ptr_arr_, end_.index_, end_.curr_,
                             end_.middle_);
    return std::make_reverse_iterator(const_end);
  }

  const_reverse_iterator crend() const {
    const_iterator const_begin(begin_.ptr_arr_, begin_.index_, begin_.curr_,
                               begin_.middle_);
    return make_reverse_iterator(const_begin);
  }

  const allocator_type& get_allocator() const { return alloc_; }

 private:
  int64_t node_ = 0;  // количество маленьких массивов(для удобства)
  std::vector<T*> big_arr_;  // большой вектор массивов
  int64_t middle_;  // хранит индекс середины большого массива(нужна для
                    // аллокации после пушей)
  iterator begin_;  // итератор на начало дека
  iterator end_;  // итератор на следующую ячейку после последней в деке
  static const int64_t kSize = 32;  // размер маленького массивчика
  size_t now_sz_ = 0;               // текущий размер дека

  allocator_type alloc_;  //мой аллокатор

  void assign_arr(int64_t start, int64_t finish) {
    for (int64_t ind = start; ind < finish; ++ind) {
      try {
        big_arr_[ind] = alloc_traits::allocate(alloc_, kSize);
      } catch (...) {
        for (int64_t ret_ind = 0; ret_ind < ind; ++ret_ind) {
          alloc_traits::deallocate(alloc_, big_arr_[ret_ind], kSize);
        }
        throw;
      }
    }
  }

  void filling() {
    if constexpr (std::is_default_constructible<T>::value) {
      for (iterator iter = begin_; iter != end_; ++iter) {
        try {
          alloc_traits::construct(alloc_, iter.curr_);
        } catch (...) {
          for (iterator ret_it = begin_; ret_it != iter; ++ret_it) {
            alloc_traits::destroy(alloc_, iter.curr_);
          }
          for (int64_t ret_ind = 0; ret_ind < node_; ++ret_ind) {
            alloc_traits::deallocate(alloc_, big_arr_[ret_ind], kSize);
          }
          throw;
        }
      }
    }
  }

  void filling_in_copy(const iterator& it_begin) {
    iterator iter = it_begin;
    for (iterator iter_2 = begin_; iter_2 != end_; ++iter_2, ++iter) {
      try {
        alloc_traits::construct(alloc_, iter_2.curr_, *iter);
      } catch (...) {
        for (iterator iter_del = iter_2; iter_del - begin_ > 0; --iter_del) {
          alloc_traits::destroy(alloc_, iter_del.curr_);
        }
        for (int64_t counter = 0; counter < end_.index_ - begin_.index_ + 1;
             ++counter) {
          alloc_traits::deallocate(
              alloc_, big_arr_[middle_ + begin_.index_ + counter], kSize);
        }
        throw;
      }
    }
  }

  void big_swap(Deque& first, Deque& second) {
    std::swap(second.big_arr_, first.big_arr_);
    std::swap(second.node_, first.node_);
    std::swap(second.middle_, first.middle_);
    std::swap(second.begin_.curr_, first.begin_.curr_);
    std::swap(second.begin_.index_, first.begin_.index_);
    std::swap(second.begin_.middle_, first.begin_.middle_);
    std::swap(second.end_.curr_, first.end_.curr_);
    std::swap(second.end_.index_, first.end_.index_);
    std::swap(second.end_.middle_, first.end_.middle_);
    if (first.big_arr_.size() == 0) {
      second.begin_.ptr_arr_ = &second.big_arr_;
      second.end_.ptr_arr_ = &second.big_arr_;
    }
    second.begin_.middle_ = &second.middle_;
    second.end_.middle_ = &second.middle_;
    std::swap(second.now_sz_, first.now_sz_);
  }

  void realloc() {
    node_ = 3 * node_;
    std::vector<T*> copy_arr(node_, nullptr);
    int64_t copy_middle = ((node_) / 2);
    for (int64_t ind = 0; ind <= end_.index_ - begin_.index_; ++ind) {
      copy_arr[ind + copy_middle + begin_.index_] =
          big_arr_[middle_ + begin_.index_ + ind];
    }
    big_arr_ = std::move(copy_arr);
    middle_ = copy_middle;
  }

  template <typename... Args>
  void add_front(Deque& deq, Args&&... args) {
    if (deq.middle_ + deq.begin_.index_ == 0) {
      realloc();
    }
    if (deq.begin_.curr_ - deq.big_arr_[deq.middle_ + deq.begin_.index_] == 0) {
      --deq.begin_.index_;
      deq.big_arr_[deq.middle_ + deq.begin_.index_] =
          alloc_traits::allocate(alloc_, kSize);
      deq.begin_.curr_ =
          deq.big_arr_[deq.middle_ + deq.begin_.index_] + kSize - 1;
    } else {
      --deq.begin_;
    }
    try {
      alloc_traits::construct(alloc_, begin_.curr_,
                              std::forward<Args>(args)...);
    } catch (...) {
      if (deq.begin_.curr_ - deq.big_arr_[deq.middle_ + deq.begin_.index_] ==
          kSize - 1) {
        alloc_traits::deallocate(
            alloc_, deq.big_arr_[deq.middle_ + deq.end_.index_], kSize);
      }
      throw;
    }
    ++deq.now_sz_;
  }

  template <typename... Args>
  void add_back(Deque& deq, Args&&... args) {
    if (deq.node_ - 1 == deq.end_.index_ + deq.middle_) {
      realloc();
    }
    if (deq.end_.curr_ - deq.big_arr_[deq.middle_ + deq.end_.index_] == 0) {
      deq.big_arr_[deq.middle_ + deq.end_.index_] =
          alloc_traits::allocate(alloc_, kSize);
      deq.end_.curr_ = deq.big_arr_[deq.middle_ + deq.end_.index_];
    }
    try {
      alloc_traits::construct(alloc_, deq.end_.curr_,
                              std::forward<Args>(args)...);
    } catch (...) {
      if (deq.end_.curr_ - deq.big_arr_[deq.middle_ + deq.end_.index_] == 0) {
        alloc_traits::deallocate(
            alloc_, deq.big_arr_[deq.middle_ + deq.end_.index_], kSize);
      }
      throw;
    }
    ++deq.end_;
    ++deq.now_sz_;
  }
};

template <typename T, typename Allocator>
template <bool IsConst>
class Deque<T, Allocator>::common_iterator {
 private:
  friend class Deque;
  std::vector<T*>* ptr_arr_;  // указатель на большой массив(нужно для того,
                              // чтобы итераторы не инвалидировались)
  int64_t index_;  // индекс массивчика, где находиться curr_ относительно
                   // середины большого массива
  T* curr_;  // указатель на ячейку, на которую указывает итератор
  int64_t* middle_;  // индекс середины большого
                     // массив

 public:
  using type = std::conditional_t<IsConst, const T, T>;
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using pointer = type*;
  using reference = type&;
  using difference_type = int64_t;

  common_iterator()
      : ptr_arr_(nullptr), index_(0), curr_(nullptr), middle_(nullptr) {}

  common_iterator(std::vector<T*>* arr, int64_t index, T* curr, int64_t* middle)
      : ptr_arr_(arr), index_(index), curr_(curr), middle_(middle) {}

  common_iterator(const common_iterator& other)
      : ptr_arr_(other.ptr_arr_),
        index_(other.index_),
        curr_(other.curr_),
        middle_(other.middle_) {}

  common_iterator& operator=(const common_iterator& other) {
    ptr_arr_ = other.ptr_arr_;
    curr_ = other.curr_;
    index_ = other.index_;
    middle_ = other.middle_;
    return *this;
  }

  type& operator*() { return *curr_; }

  const type& operator*() const { return *curr_; }

  type* operator->() { return curr_; }

  const type* operator->() const { return curr_; }

  common_iterator& operator++() {
    if (curr_ - (*ptr_arr_)[*middle_ + index_] == kSize - 1) {
      ++index_;
      curr_ = (*ptr_arr_)[*middle_ + index_];
    } else {
      ++curr_;
    }
    return *this;
  }

  common_iterator operator++(int) {
    common_iterator tmp = *this;
    ++(*this);
    return tmp;
  }

  common_iterator& operator--() {
    if (curr_ - (*ptr_arr_)[*middle_ + index_] == 0) {
      --index_;
      curr_ = (*ptr_arr_)[*middle_ + index_] + (kSize - 1);
    } else {
      --curr_;
    }
    return *this;
  }

  common_iterator operator--(int) {
    common_iterator tmp = *this;
    --(*this);
    return tmp;
  }

  common_iterator& operator+=(int64_t num) {
    if (num == 0) {
      return *this;
    }
    int64_t helper = kSize - (curr_ - (*ptr_arr_)[*middle_ + index_]) - 1;
    index_ += (num - helper + kSize - 1) / kSize;
    curr_ = (*ptr_arr_)[*middle_ + index_] + num - helper -
            kSize * ((num - helper + kSize - 1) / kSize - 1) - 1;
    return *this;
  }

  common_iterator operator+(int64_t num) const {
    common_iterator tmp = *this;
    tmp += num;
    return tmp;
  }

  common_iterator& operator-=(int64_t num) {
    if (num == 0) {
      return *this;
    }
    int64_t helper = curr_ - (*ptr_arr_)[*middle_ + index_];
    index_ -= (num - helper + kSize - 1) / kSize;
    curr_ =
        (*ptr_arr_)[*middle_ + index_] + kSize - 1 -
        (num - helper - kSize * ((num - helper + kSize - 1) / kSize - 1) - 1);
    return *this;
  }

  common_iterator operator-(int64_t num) const {
    common_iterator tmp = *this;
    tmp -= num;
    return tmp;
  }

  int64_t operator-(const common_iterator& other) const {
    if (ptr_arr_ == nullptr) {
      return 0;
    }
    int64_t diff;
    if (index_ != other.index_) {
      if (index_ > other.index_) {
        diff = kSize - (other.curr_ - (*ptr_arr_)[*middle_ + other.index_]) +
               (curr_ - (*ptr_arr_)[*middle_ + index_]) +
               kSize * (index_ - other.index_ - 1);
      } else {
        diff = -(other - *this);
      }
    } else {
      diff = curr_ - other.curr_;
    }
    return diff;
  }

  bool operator<(const common_iterator& other) const {
    return (*this - other) < 0;
  }

  bool operator>(const common_iterator& other) const { return other < *this; }

  bool operator<=(const common_iterator& other) const {
    return !(*this > other);
  }

  bool operator>=(const common_iterator& other) const {
    return !(*this < other);
  }

  bool operator==(const common_iterator& other) const {
    return !(*this < other) && !(other < *this);
  }
  bool operator!=(const common_iterator& other) const {
    return !(*this == other);
  }
};
