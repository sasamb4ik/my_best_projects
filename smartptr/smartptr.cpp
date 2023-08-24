#pragma once

template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr;

template <typename T>
class EnableSharedFromThis;

struct BaseControlBlock {
  size_t shared_counter_ = 0;
  size_t weak_counter_ = 0;
  virtual void kill_fields() = 0;
  virtual void kill_object() = 0;
  virtual ~BaseControlBlock() = default;
};

template <typename T, typename Alloc = std::allocator<T>>
class ControlBlockAllocate : public BaseControlBlock {
private:
  using AllocThis = typename std::allocator_traits<
          Alloc>::template rebind_alloc<ControlBlockAllocate<T, Alloc>>;
  using StorageType =
          typename std::aligned_storage<sizeof(T), alignof(T)>::type;

  T* pointer_;
  Alloc alloc_;
  AllocThis allocCopy_;
  StorageType memory_;

  void kill_fields() override {
    T* object = reinterpret_cast<T*>(&memory_);
    std::allocator_traits<Alloc>::destroy(alloc_, object);
  }

  void kill_object() override {
    typename std::allocator_traits<AllocThis>::template rebind_alloc<
            ControlBlockAllocate>
            allocThis(allocCopy_);
    allocThis.deallocate(this, 1);
  }

public:
  template <typename... Args>
  ControlBlockAllocate(Alloc alloc, AllocThis allocThis, Args&&... args)
      : pointer_(nullptr),
        alloc_(alloc),
        allocCopy_(allocThis) {
    T* object = reinterpret_cast<T*>(&memory_);
    std::allocator_traits<Alloc>::construct(alloc, object,
                                            std::forward<Args>(args)...);
    pointer_ = object;
  }

  T* get_pointer() noexcept {
    return reinterpret_cast<T*>(&memory_);
  }

  ~ControlBlockAllocate() override = default;
};

template <typename T, typename Alloc = std::allocator<T>,
         typename Deleter = std::default_delete<T>>
class PointerControlBlock : public BaseControlBlock {
private:
  using AllocThis =
          typename std::allocator_traits<Alloc>::template rebind_alloc<
                  PointerControlBlock<T, Alloc, Deleter>>;

  T* pointer_ = nullptr;
  AllocThis allocCopy_;
  Deleter deleter_;

  void kill_fields() override {
    if (pointer_) {
      deleter_(pointer_);
      pointer_ = nullptr;
    }
  }

  void kill_object() override {
    auto allocThis = allocCopy_;
    std::destroy_at(this);
    std::allocator_traits<AllocThis>::deallocate(allocThis, this, 1);
  }

public:
  PointerControlBlock(T* ptr, const AllocThis& alloc = AllocThis(),
                      const Deleter& del = Deleter())
      : pointer_(ptr), allocCopy_(alloc), deleter_(del) {}

  ~PointerControlBlock() override = default;
};

template <typename T>
class SharedPtr {
private:
  template <typename U>
  friend class SharedPtr;

  template <typename U, typename Alloc, typename... Args>
  friend SharedPtr<U> allocateShared(Alloc alloc, Args&&... args);

  template <typename U>
  friend class WeakPtr;

  BaseControlBlock* control_block_ = nullptr;
  T* pointer_ = nullptr;

  void check_and_deallocate() {
    if (control_block_->shared_counter_ == 0) {
      control_block_->kill_fields();
      if (control_block_->weak_counter_ == 0) {
        control_block_->kill_object();
      }
    }
  }

  SharedPtr(BaseControlBlock* controlBlock, T* ptr)
      : control_block_(controlBlock), pointer_(ptr) {
  }

  template <typename U>
  void increment_shared_counter(const SharedPtr<U>& another) {
    if (another.control_block_) {
      another.control_block_->shared_counter_++;
    }
  }

public:
  SharedPtr() noexcept = default;

  SharedPtr(SharedPtr<T>&& another) noexcept
      : control_block_(std::exchange(another.control_block_, nullptr)),
        pointer_(std::exchange(another.pointer_, nullptr)) {
  }

  template <typename U>
  SharedPtr(SharedPtr<U>&& another) noexcept
      : control_block_(std::exchange(another.control_block_, nullptr)),
        pointer_(std::exchange(another.pointer_, nullptr)) {
  }

  template <typename U>
  SharedPtr(U* pointer) : SharedPtr(pointer, std::default_delete<U>(), std::allocator<U>()) {}

  SharedPtr(const SharedPtr<T>& another) : control_block_(another.control_block_), pointer_(another.pointer_) {
    increment_shared_counter(another);
  }

  template <typename U>
  SharedPtr(const SharedPtr<U>& another) : control_block_(another.control_block_), pointer_(another.pointer_) {
    increment_shared_counter(another);
  }

  template <typename Deleter, typename Alloc>
  SharedPtr(T* ptr, const Deleter& del, const Alloc& alloc) {
    using ControlBlockType = PointerControlBlock<T, Alloc, Deleter>;
    using AllocThis = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockType>;

    AllocThis allocThisCopy = alloc;
    auto controlBlock = std::allocator_traits<AllocThis>::allocate(allocThisCopy, 1);

    try {
      new (controlBlock) ControlBlockType(ptr, allocThisCopy, del);
      control_block_ = controlBlock;
      pointer_ = static_cast<T*>(ptr);
      control_block_->shared_counter_++;
    } catch (...) {
      std::allocator_traits<AllocThis>::deallocate(allocThisCopy, controlBlock, 1);
      throw;
    }
  }

  template <typename Deleter>
  SharedPtr(T* ptr, Deleter& d)
      : SharedPtr(ptr, d, std::allocator<T>()) {
  }

  void swap(SharedPtr<T>& another) noexcept {
    std::swap(control_block_, another.control_block_);
    std::swap(pointer_, another.pointer_);
  }

  template <typename U>
  SharedPtr<T>& operator=(SharedPtr<U> another) {
    SharedPtr<T>(std::move(another)).swap(*this);
    return *this;
  }

  SharedPtr<T>& operator=(const SharedPtr<T>& another) {
    SharedPtr<T>(another).swap(*this);
    return *this;
  }

  SharedPtr<T>& operator=(SharedPtr<T>&& another) noexcept {
    SharedPtr<T>(std::move(another)).swap(*this);
    return *this;
  }

  template <typename U>
  SharedPtr<T>& operator=(U* another) {
    SharedPtr<T>(another).swap(*this);
    return *this;
  }

  T* get() const {
    return pointer_;
  }

  [[nodiscard]] size_t use_count() const {
    if (control_block_) {
      return control_block_->shared_counter_;
    }
    return 0;
  }

  T& operator*() const {
    return *get();
  }

  T* operator->() const {
    return get();
  }

  void reset(T* pointer = nullptr) {
    if (control_block_) {
      control_block_->shared_counter_--;
      check_and_deallocate();
      control_block_ = nullptr;
      pointer_ = nullptr;
    }

    if (pointer) {
      *this = SharedPtr<T>(pointer);
    }
  }

  ~SharedPtr() {
    if (!control_block_) {
      return;
    }
    control_block_->shared_counter_--;
    check_and_deallocate();
  }
};

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(Alloc alloc, Args&&... args) {
  using ControlBlockType = ControlBlockAllocate<T, Alloc>;
  using AllocThis = typename std::allocator_traits<
          Alloc>::template rebind_alloc<ControlBlockType>;

  AllocThis allocThisCopy = alloc;
  auto controlBlock =
          std::allocator_traits<AllocThis>::allocate(allocThisCopy, 1);

  try {
    new (controlBlock)
            ControlBlockType(alloc, allocThisCopy, std::forward<Args>(args)...);

    T* managedObject = controlBlock->get_pointer();

    SharedPtr<T> sharedPtr(controlBlock, managedObject);

    if (sharedPtr.control_block_) {
      sharedPtr.control_block_->shared_counter_++;
    }

    if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
      sharedPtr.control_block_->ptr = sharedPtr.get();
    }

    return sharedPtr;
  } catch (...) {
    std::allocator_traits<AllocThis>::deallocate(allocThisCopy, controlBlock,
                                                 1);
    throw;
  }
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  return allocateShared<T, std::allocator<T>>(std::allocator<T>(),
                                              std::forward<Args>(args)...);
}

template <typename T>
class WeakPtr {
private:
  BaseControlBlock* control_block_ = nullptr;
  T* pointer_ = nullptr;

  template <typename U>
  friend class WeakPtr;

  template <typename U>
  void increment_weak_counter(const WeakPtr<U>& another) {
    if (another.control_block_) {
      another.control_block_->weak_counter_++;
    }
  }

public:
  WeakPtr() = default;

  WeakPtr(const WeakPtr<T>& another) : control_block_(another.control_block_), pointer_(another.pointer_) {
    increment_weak_counter(another);
  }

  template <typename U>
  WeakPtr(const WeakPtr<U>& another) : control_block_(another.control_block_), pointer_(another.pointer_) {
    increment_weak_counter(another);
  }

  template <typename U>
  WeakPtr(WeakPtr<U>&& another) {
    control_block_ = std::exchange(another.control_block_, nullptr);
    pointer_ = std::exchange(static_cast<T*>(another.pointer_), nullptr);
  }

  WeakPtr(WeakPtr<T>&& another) {
    control_block_ = std::exchange(another.control_block_, nullptr);
    pointer_ = std::exchange(another.pointer_, nullptr);
  }

  void swap(WeakPtr<T>& another) noexcept {
    std::swap(control_block_, another.control_block_);
    std::swap(pointer_, another.pointer_);
  }

  WeakPtr<T>& operator=(WeakPtr<T> another) noexcept {
    WeakPtr<T>(std::move(another)).swap(*this);
    return *this;
  }

  template <typename U>
  WeakPtr<T>& operator=(const WeakPtr<U>& another) noexcept {
    WeakPtr<T>(another).swap(*this);
    return *this;
  }

  template <typename U>
  WeakPtr<T>& operator=(WeakPtr<U>&& another) noexcept {
    WeakPtr<T>(std::move(another)).swap(*this);
    return *this;
  }

  template <typename U>
  WeakPtr(const SharedPtr<U>& another) : control_block_(another.control_block_), pointer_(another.pointer_) {
    if (control_block_) {
      control_block_->weak_counter_++;
    }
  }

  [[nodiscard]] size_t use_count() const noexcept {
    if (!control_block_) {
      return 0;
    }
    return control_block_->shared_counter_;
  }

  [[nodiscard]] bool expired() const noexcept {
    if (!control_block_) {
      return true;
    }
    return control_block_->shared_counter_ == 0;
  }

  SharedPtr<T> lock() const noexcept {
    SharedPtr<T> answer(control_block_, pointer_);

    if (answer.control_block_) {
      answer.control_block_->shared_counter_++;
    }

    return answer;
  }

  ~WeakPtr() {
    if (!control_block_) {
      return;
    }
    control_block_->weak_counter_--;
    if (control_block_->weak_counter_ == 0 &&
        control_block_->shared_counter_ == 0) {
      control_block_->kill_object();
    }
  }
};

template <typename T>
class EnableSharedFromThis {
private:
  mutable WeakPtr<T> weak_pointer_;

  EnableSharedFromThis() = default;

  EnableSharedFromThis(const EnableSharedFromThis&) {
  }

  EnableSharedFromThis& operator=(const EnableSharedFromThis&) {
    return *this;
  }

public:
  SharedPtr<T> shared_from_this() {
    return SharedPtr<T>(weak_pointer_.lock());
  }

  SharedPtr<const T> shared_from_this() const {
    return SharedPtr<const T>(weak_pointer_.lock());
  }

  virtual ~EnableSharedFromThis() = default;
};
