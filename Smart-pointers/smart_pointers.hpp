#include <memory>
#include <utility>

struct BaseControlBlock {
  size_t shared_count = 1;
  size_t weak_count = 0;

  BaseControlBlock() = default;

  virtual void dispose() {}
  virtual void destroy() {}

  ~BaseControlBlock() = default;
};

template <typename T, typename Alloc>
struct ControlBlockShared : BaseControlBlock {
  using alloc_type = typename std::allocator_traits<
      Alloc>::template rebind_alloc<ControlBlockShared<T, Alloc>>;

  template <typename... Args>
  ControlBlockShared(Alloc alloc, alloc_type alloc_2, Args&&... args)
      : allocator(alloc), alloc_make_shared(alloc_2) {
    ptr = reinterpret_cast<T*>(object);
    std::construct_at(ptr, std::forward<Args>(args)...);
  }

  virtual ~ControlBlockShared() = default;

  virtual void dispose() override {
    std::allocator_traits<Alloc>::destroy(allocator, ptr);
  }

  virtual void destroy() override {
    auto alloc_copy = alloc_make_shared;
    this->~ControlBlockShared();
    std::allocator_traits<alloc_type>::deallocate(alloc_copy, this, 1);
  }

  T* ptr;
  Alloc allocator;
  alloc_type alloc_make_shared;
  alignas(T) char object[sizeof(T)];
};

template <typename T, typename Deleter = std::default_delete<T>,
          typename Alloc = std::allocator<T>>
struct ControlBlockWithDeleter : BaseControlBlock {
  using alloc_type = typename std::allocator_traits<
      Alloc>::template rebind_alloc<ControlBlockWithDeleter<T, Deleter, Alloc>>;

  ControlBlockWithDeleter(T* ptr, Deleter del = Deleter(),
                          Alloc alloc = Alloc())
      : object(ptr), deleter(del), allocator(alloc) {}

  virtual ~ControlBlockWithDeleter() = default;

  virtual void dispose() override { deleter(object); }

  virtual void destroy() override {
    auto alloc_copy = allocator;
    this->~ControlBlockWithDeleter();
    std::allocator_traits<alloc_type>::deallocate(alloc_copy, this, 1);
  }

  T* object;
  Deleter deleter;
  alloc_type allocator;
};

template <typename T>
class SharedPtr {
 private:
  T* ptr_ = nullptr;
  BaseControlBlock* cptr_ = nullptr;

  template <typename Y, typename Alloc, typename... Args>
  friend SharedPtr<Y> AllocateShared(const Alloc& alloc, Args&&... args);

  template <typename Y, typename... Args>
  friend SharedPtr<Y> MakeShared(Args&&... args);

  template <typename Y>
  friend class WeakPtr;

  template <typename Y, typename Alloc = std::allocator<Y>>
  SharedPtr(ControlBlockShared<Y, Alloc>* cptr)
      : cptr_(cptr), ptr_(cptr->ptr) {}

  void my_clean() {
    if (cptr_ == nullptr) {
      return;
    }
    --cptr_->shared_count;
    if (cptr_->shared_count == 0) {
      cptr_->dispose();
      if (cptr_->weak_count == 0) {
        cptr_->destroy();
      }
    }
  }

 public:
  SharedPtr() {}

  SharedPtr(std::nullptr_t) : SharedPtr() {}

  template <typename Y, typename Deleter = std::default_delete<Y>,
            typename Alloc = std::allocator<Y>>
  SharedPtr(Y* ptr, Deleter deleter = Deleter(), const Alloc& alloc = Alloc())
      : ptr_(ptr) {
    using alloc_type =
        typename std::allocator_traits<Alloc>::template rebind_alloc<
            ControlBlockWithDeleter<Y, Deleter, Alloc>>;
    alloc_type alloc_cb = alloc;

    cptr_ = std::allocator_traits<alloc_type>::allocate(alloc_cb, 1);
    std::allocator_traits<alloc_type>::construct(
        alloc_cb,
        static_cast<ControlBlockWithDeleter<Y, Deleter, Alloc>*>(cptr_), ptr,
        std::move(deleter), alloc);
  }

  SharedPtr(const SharedPtr& other) : cptr_(other.cptr_), ptr_(other.ptr_) {
    if (cptr_ != nullptr) {
      ++cptr_->shared_count;
    }
  }

  SharedPtr(SharedPtr&& other) : ptr_(other.ptr_), cptr_(other.cptr_) {
    other.cptr_ = nullptr;
    other.ptr_ = nullptr;
  }

  SharedPtr& operator=(const SharedPtr& other) {
    SharedPtr copy = other;
    swap(copy);
    return *this;
  }

  SharedPtr& operator=(SharedPtr&& other) noexcept {
    SharedPtr copy(std::move(other));
    swap(copy);
    return *this;
  }

  template <typename Y>
  friend class SharedPtr;

  template <typename Y>
  SharedPtr(const SharedPtr<Y>& other) {
    if constexpr (std::is_same_v<T, Y> || std::is_base_of_v<T, Y>) {
      cptr_ = other.cptr_;
      ptr_ = other.ptr_;
      if (cptr_) {
        cptr_->shared_count++;
      }
    } else {
      throw std::runtime_error("Bad assignment");
    }
  }

  template <typename Y>
  SharedPtr(SharedPtr&& other) {
    if constexpr (std::is_same_v<T, Y> || std::is_base_of_v<T, Y>) {
      cptr_ = other.cptr_;
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
      other.cptr_ = nullptr;
    } else {
      throw std::runtime_error("Bad assignment");
    }
  }

  template <typename Y>
  SharedPtr& operator=(const SharedPtr& other) {
    if constexpr (std::is_same_v<T, Y> || std::is_base_of_v<T, Y>) {
      SharedPtr copy(other);
      swap(copy);
    } else {
      throw std::runtime_error("Bad assignment");
    }
  }

  template <typename Y>
  SharedPtr& operator=(SharedPtr&& other) noexcept {
    if constexpr (std::is_same_v<T, Y> || std::is_base_of_v<T, Y>) {
      SharedPtr copy(std::move(other));
      swap(copy);
    } else {
      throw std::runtime_error("Bad assignment");
    }
  }

  ~SharedPtr() { my_clean(); }

  size_t use_count() const noexcept {
    if (cptr_ != nullptr) {
      return cptr_->shared_count;
    }
    return 0;
  }

  T* get() const noexcept { return ptr_; };

  T& operator*() { return *get(); }

  const T& operator*() const { return *get(); }

  T* operator->() { return get(); }

  const T* operator->() const { return get(); }

  void reset() noexcept { SharedPtr().swap(*this); }

  void swap(SharedPtr& other) noexcept {
    std::swap(ptr_, other.ptr_);
    std::swap(cptr_, other.cptr_);
  }
};

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> AllocateShared(const Alloc& alloc, Args&&... args) {
  using control_block_alloc_type = typename std::allocator_traits<
      Alloc>::template rebind_alloc<ControlBlockShared<T, Alloc>>;
  control_block_alloc_type control_block_alloc(alloc);
  auto control_block =
      std::allocator_traits<control_block_alloc_type>::allocate(
          control_block_alloc, 1);
  try {
    std::allocator_traits<control_block_alloc_type>::construct(
        control_block_alloc, control_block, alloc, control_block_alloc,
        std::forward<Args>(args)...);
  } catch (...) {
    std::allocator_traits<control_block_alloc_type>::deallocate(
        control_block_alloc, control_block, 1);
    throw;
  }
  return SharedPtr<T>(control_block);
}

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
  return AllocateShared<T, std::allocator<T>, Args...>(
      std::allocator<T>(), std::forward<Args>(args)...);
}

template <typename T>
class WeakPtr {
 private:
  BaseControlBlock* helper_ = nullptr;

 public:
  WeakPtr();

  WeakPtr(const SharedPtr<T>& ptr) : helper_(ptr.cptr_) {
    if (helper_ != nullptr) {
      helper_->weak_count++;
    }
  }

  WeakPtr(WeakPtr&& other) : helper_(other.helper_) { other.helper_ = nullptr; }

  WeakPtr& operator=(const WeakPtr& other) {
    WeakPtr copy(other);
    std::swap(copy.helper_, helper_);
    return *this;
  }

  WeakPtr& operator=(WeakPtr&& other) {
    WeakPtr copy(std::move(other));
    std::swap(copy.helper_, helper_);
    other.helper_ = nullptr;
    return *this;
  }

  bool expired() const { return helper_->shared_count == 0; }

  SharedPtr<T> lock() const {
    if (helper_->shared_count != 0) {
      return SharedPtr<T>(helper_);
    }
  }

  ~WeakPtr() {
    if (helper_ == nullptr) {
      return;
    }
    --helper_->weak_count;
    if (helper_->weak_count == 0 && helper_->shared_count == 0) {
      helper_->destroy();
    }
  }
};
