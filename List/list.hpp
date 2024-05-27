#pragma once
#include <stdlib.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <type_traits>

template <typename T, typename Allocator = std::allocator<T>>
class List {
 public:
  struct fake_node;
  struct node;

  template <bool IsConst>
  class common_iterator;

  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  using allocator_type = Allocator;
  using alloc_traits = std::allocator_traits<
      typename std::allocator_traits<Allocator>::template rebind_alloc<node>>;
  using value_type = T;

  List() {}

  explicit List(size_t count, const Allocator& alloc = Allocator())
      : alloc_(alloc) {
    for (size_t ind = 0; ind < count; ++ind) {
      node* new_node = alloc_traits::allocate(alloc_, 1);
      try {
        alloc_traits::construct(alloc_, new_node);
        add_node_back(new_node);
      } catch (...) {
        alloc_traits::deallocate(alloc_, new_node, 1);
        destroy_list();
        throw;
      }
    }
  }

  List(size_t count, const T& value, const Allocator& alloc = Allocator())
      : List(count, alloc) {
    for (iterator iter = begin(); iter != end(); ++iter) {
      *iter = value;
    }
  }

  List(const List& other)
      : alloc_(
            alloc_traits::select_on_container_copy_construction(other.alloc_)) {
    for (iterator iter = other.begin(); iter != other.end(); ++iter) {
      node* new_node = alloc_traits::allocate(alloc_, 1);
      try {
        construct_node_back(new_node, std::move(*iter));
      } catch (...) {
        alloc_traits::deallocate(alloc_, new_node, 1);
        destroy_list();
        throw;
      }
    }
  }

  List(std::initializer_list<T> init, const Allocator& alloc = Allocator())
      : alloc_(alloc) {
    for (auto iter = init.begin(); iter != init.end(); ++iter) {
      node* new_node = alloc_traits::allocate(alloc_, 1);
      try {
        construct_node_back(new_node, std::move(*iter));
      } catch (...) {
        alloc_traits::deallocate(alloc_, new_node, 1);
        destroy_list();
        throw;
      }
    }
  }

  ~List() { destroy_list(); }

  List& operator=(const List& other) {
    List copy(other);
    if (alloc_traits::propagate_on_container_copy_assignment::value &&
        alloc_ != other.alloc_) {
      alloc_ = other.alloc_;
    }

    if (!empty()) {
      std::swap((&fake_)->next->prev, (&copy.fake_)->next->prev);
      std::swap((&fake_)->prev->next, (&copy.fake_)->prev->next);
    } else {
      (&copy.fake_)->prev->next = static_cast<node*>(&fake_);
      (&copy.fake_)->next->prev = static_cast<node*>(&fake_);
    }
    std::swap(fake_, copy.fake_);
    std::swap(now_sz_, copy.now_sz_);
    return *this;
  }

  iterator begin() const {
    iterator begin((&fake_)->next);
    return begin;
  }

  iterator end() const {
    iterator end((&fake_)->prev);
    ++end;
    return end;
  }

  const_iterator cbegin() const {
    const_iterator const_begin((&fake_)->next);
    return const_begin;
  }

  const_iterator cend() const {
    const_iterator const_end(static_cast<node*>(&fake_));
    return const_end;
  }

  const_reverse_iterator rbegin() const {
    const_iterator const_end(static_cast<node*>(&fake_));
    return std::make_reverse_iterator(const_end);
  }

  const_reverse_iterator rend() const {
    const_iterator const_begin((&fake_)->next);
    return std::make_reverse_iterator(const_begin);
  }

  reverse_iterator rbegin() {
    return std::make_reverse_iterator(end(static_cast<node*>(&fake_)));
  }

  reverse_iterator rend() {
    return std::make_reverse_iterator(begin_((&fake_)->next));
  }

  const_reverse_iterator crbegin() const {
    const_iterator const_end(static_cast<node*>(&fake_));
    return std::make_reverse_iterator(const_end);
  }

  const_reverse_iterator crend() const {
    const_iterator const_begin(fake_->next);
    return make_reverse_iterator(const_begin);
  }

  T& front() { return fake_->next->value; }

  const T& front() const { return fake_->next->value; }

  T& back() { return fake_->prev->value; }

  const T& back() const { return fake_->prev->value; }

  bool empty() const { return (now_sz_ == 0); }

  size_t size() const { return now_sz_; }

  void push_back(const T& value) {
    node* new_node = alloc_traits::allocate(alloc_, 1);
    construct_node_back(new_node, std::move(value));
  }

  void push_front(const T& value) {
    node* new_node = alloc_traits::allocate(alloc_, 1);
    construct_node_front(new_node, std::move(value));
  }

  void pop_back() {
    node* new_node = (&fake_)->prev;
    (&fake_)->prev = new_node->prev;
    new_node->prev->next = static_cast<node*>(&fake_);
    alloc_traits::destroy(alloc_, new_node);
    alloc_traits::deallocate(alloc_, new_node, 1);
    --now_sz_;
  }

  void pop_front() {
    node* new_node = (&fake_)->next;
    (&fake_)->next = new_node->next;
    new_node->next->prev = static_cast<node*>(&fake_);
    alloc_traits::destroy(alloc_, new_node);
    alloc_traits::deallocate(alloc_, new_node, 1);
    --now_sz_;
  }

  void push_back(T&& value) {
    node* new_node = alloc_traits::allocate(alloc_, 1);
    construct_node_back(new_node, std::move(value));
  }

  void push_front(T&& value) {
    node* new_node = alloc_traits::allocate(alloc_, 1);
    construct_node_front(new_node, std::move(value));
  }

  const typename std::allocator_traits<Allocator>::template rebind_alloc<node>&
  get_allocator() const {
    return alloc_;
  }

 private:
  fake_node fake_;
  size_t now_sz_ = 0;
  typename std::allocator_traits<Allocator>::template rebind_alloc<node> alloc_;

  void destroy_list() {
    while (now_sz_ > 0) {
      pop_back();
    }
  }

  template <typename... Args>
  void construct_node_back(node* new_node, Args&&... args) {
    alloc_traits::construct(alloc_, new_node, new_node, new_node,
                            std::forward<Args>(args)...);
    add_node_back(new_node);
  }

  void add_node_back(node* new_node) {
    if ((&fake_)->prev == nullptr) {
      (&fake_)->prev = static_cast<node*>(&fake_);
    }
    if ((&fake_)->next == nullptr) {
      (&fake_)->next = static_cast<node*>(&fake_);
    }
    new_node->prev = (&fake_)->prev;
    new_node->next = static_cast<node*>(&fake_);
    (&fake_)->prev->next = new_node;
    (&fake_)->prev = new_node;
    now_sz_++;
  }

  template <typename... Args>
  void construct_node_front(node* new_node, Args&&... args) {
    alloc_traits::construct(alloc_, new_node, new_node, new_node,
                            std::forward<Args>(args)...);
    if ((&fake_)->next == nullptr) {
      (&fake_)->next = static_cast<node*>(&fake_);
    }
    if ((&fake_)->prev == nullptr) {
      (&fake_)->prev = static_cast<node*>(&fake_);
    }
    new_node->next = (&fake_)->next;
    new_node->prev = static_cast<node*>(&fake_);
    (&fake_)->next->prev = new_node;
    (&fake_)->next = new_node;
    now_sz_++;
  }
};

template <typename T, typename Allocator>
struct List<T, Allocator>::fake_node {
  node* prev;
  node* next;

  fake_node() : prev(nullptr), next(nullptr) {}

  fake_node(node* prev_2, node* next_2) : prev(prev_2), next(next_2) {}
};

template <typename T, typename Allocator>
struct List<T, Allocator>::node : public fake_node {
  T value;

  node() : fake_node(nullptr, nullptr) {}

  node(node* prev_2, node* next_2, T&& value_2)
      : fake_node(prev_2, next_2), value((value_2)) {}

  node(node* prev_2, node* next_2, const T& value_2)
      : fake_node(prev_2, next_2), value(value_2) {}
};

template <typename T, typename Allocator>
template <bool IsConst>
class List<T, Allocator>::common_iterator {
 private:
  node* curr_;

 public:
  using type = std::conditional_t<IsConst, const T, T>;
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = T;
  using pointer = type*;
  using reference = type&;
  using difference_type = int64_t;

  common_iterator() {}

  common_iterator(node* ptr) : curr_(ptr) {}

  common_iterator(const common_iterator& other) : curr_(other.curr_) {}

  common_iterator& operator=(const common_iterator& other) {
    curr_ = other.curr_;
  }

  reference operator*() { return curr_->value; }

  reference operator*() const { return curr_->value; }

  pointer operator->() { return &curr_->value; }

  pointer operator->() const { return &curr_->value; }

  common_iterator& operator++() {
    curr_ = curr_->next;
    return *this;
  }

  common_iterator operator++(int) {
    common_iterator tmp = *this;
    ++(*this);
    return tmp;
  }

  common_iterator& operator--() {
    curr_ = curr_->prev;
    return *this;
  }

  common_iterator operator--(int) {
    common_iterator tmp = *this;
    --(*this);
    return tmp;
  }

  bool operator==(const common_iterator& other) const {
    return (curr_ == other.curr_);
  }

  bool operator!=(const common_iterator& other) const {
    return !(*this == other);
  }
};
