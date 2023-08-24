#pragma once

#include <array>

#include <stdexcept>

#include <utility>

template <size_t N>
class StackStorage {
 public:
  int8_t data[N];
  size_t shift;

  StackStorage()
      : shift(0) {
  }

  StackStorage(const StackStorage<N>& other) = delete;
  StackStorage& operator=(const StackStorage& other) = delete;
  StackStorage(StackStorage&& other) = default;
  StackStorage& operator=(StackStorage&& other) {
    data = std::move(other.data);
    std::swap(shift, other.size);
    return *this;
  }
};

template <typename T, size_t N>
class StackAllocator {
 public:
  StackStorage<N>* stack;

  template <typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };

  using value_type = T;

  StackAllocator() = default;
  ~StackAllocator() = default;

  StackAllocator(StackStorage<N>& adress_stack)
      : stack(&adress_stack){};

  StackAllocator(StackStorage<N>* st)
      : stack(st) {
  }

  template <typename U>
  StackAllocator(const StackAllocator<U, N>& another)
      : stack(another.stack){};

  T* allocate(size_t count) const {
    size_t aligned = stack->shift % alignof(T);
    if (aligned > 0) {
      aligned = alignof(T) - aligned;
    }
    stack->shift += aligned;
    T* new_place = reinterpret_cast<T*>(stack->data + stack->shift);
    size_t size = count * sizeof(T);
    if (stack->shift + size > N or count == 0) {
      throw std::bad_alloc();
    }
    stack->shift += size;
    return new_place;
  }

  void deallocate(T* pointer, size_t count) const {
    auto bytes_to_free = count * sizeof(T);
    if (reinterpret_cast<int8_t*>(pointer) + bytes_to_free ==
        stack->data + stack->shift) {
      stack->shift -= bytes_to_free;
    }
  }
};

template <typename T, typename Allocator = std::allocator<T>>
class List {
 public:
  using value_type = T;

  struct BaseNode {
    BaseNode* next;
    BaseNode* prev;
  };

  struct Node : BaseNode {
    T value;

    Node(BaseNode* prev, BaseNode* next, const T& new_value)
        : BaseNode{prev, next},
          value(new_value) {
    }
    Node(BaseNode* prev, BaseNode* next)
        : BaseNode{prev, next} {
    }
  };

  template <bool const_>
  struct BaseIterator {
   public:
    using value_type = T;
    using reference = std::conditional_t<const_, const T&, T&>;
    using pointer = std::conditional_t<const_, const T*, T*>;
    using difference_type = ptrdiff_t;
    using const_reference = const value_type&;
    using iterator_category = std::bidirectional_iterator_tag;

   private:
    BaseNode* current_node_;

   public:
    BaseNode* get_curnode() const {
      return current_node_;
    }

    BaseIterator() = default;
    ~BaseIterator() = default;

    BaseIterator(BaseNode* item)
        : current_node_(item) {
    }

    operator BaseIterator<true>() const {
      return BaseIterator<true>(current_node_);
    }

    pointer operator->() const {
      return &static_cast<Node*>(current_node_)->value;
    }

    reference operator*() const {
      Node* node = static_cast<Node*>(current_node_);
      return static_cast<reference>(node->value);
    }

    BaseIterator& operator++() {
      current_node_ = current_node_->next;
      return *this;
    }

    BaseIterator operator++(int) {
      auto copy = *this;
      ++*this;
      return copy;
    }

    BaseIterator& operator--() {
      current_node_ = current_node_->prev;
      return *this;
    }

    BaseIterator operator--(int) {
      auto copy = *this;
      --*this;
      return copy;
    }

    bool operator==(const BaseIterator& other) const {
      return current_node_ == other.current_node_;
    }

    bool operator!=(const BaseIterator& other) const {
      return current_node_ != other.current_node_;
    }
  };

  using iterator = BaseIterator<false>;
  using const_iterator = BaseIterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() {
    return iterator(fake_node_.next);
  }

  iterator end() {
    return iterator(&fake_node_);
  }

  const_iterator begin() const {
    return const_iterator(fake_node_.next);
  }

  const_iterator end() const {
    return const_iterator(&fake_node_);
  }

  const_iterator cbegin() const {
    return const_iterator(fake_node_.next);
  }

  const_iterator cend() const {
    return const_iterator(&fake_node_);
  }

  reverse_iterator rbegin() {
    return std::make_reverse_iterator(end());
  }

  reverse_iterator rend() {
    return std::make_reverse_iterator(begin());
  }

  const_reverse_iterator rbegin() const {
    return std::make_reverse_iterator(cend());
  }

  const_reverse_iterator rend() const {
    return std::make_reverse_iterator(cbegin());
  }

  const_reverse_iterator crbegin() const {
    return std::make_reverse_iterator(cend());
  }
  const_reverse_iterator crend() const {
    return std::make_reverse_iterator(cbegin());
  }

 private:
  typedef typename std::allocator_traits<Allocator>::template rebind_alloc<Node>
      node_alloc;

  size_t list_size_ = 0;
  node_alloc node_allocator_;
  mutable BaseNode fake_node_{&fake_node_, &fake_node_};
  using allocator_type = Allocator;

  void bind_new_node(BaseNode* index_node, BaseNode* new_node) {
    index_node->prev->next = new_node;
    index_node->prev = new_node;
  }

  void bind_old_node(BaseNode* first, BaseNode* second) {
    first->next = second;
    second->prev = first;
  }

  template <bool has_value, bool ReturnNode>
  auto insert_inside_impl(
      const_iterator index,
      std::conditional_t<has_value, const T&, int> value = 0) {
    Node* new_node = node_allocator_.allocate(1);
    try {
      if constexpr (has_value) {
        std::allocator_traits<node_alloc>::construct(
            node_allocator_, new_node, index.get_curnode(),
            index.get_curnode()->prev, value);
      } else {
        std::allocator_traits<node_alloc>::construct(node_allocator_, new_node,
                                                     index.get_curnode(),
                                                     index.get_curnode()->prev);
      }
      bind_new_node(index.get_curnode(), new_node);
      ++list_size_;
      if constexpr (ReturnNode) {
        return iterator(new_node);
      }
    } catch (...) {
      node_allocator_.deallocate(new_node, 1);
      throw;
    }
  }

  void insert_value_inside(const_iterator index, const T& value) {
    insert_inside_impl<true, false>(index, value);
  }

  void insert_novalue_inside(const_iterator index) {
    insert_inside_impl<false, false>(index);
  }

  iterator insert_value_inside_return(const_iterator index, const T& value) {
    return insert_inside_impl<true, true>(index, value);
  }

  void clear() {
    BaseNode* new_node = fake_node_.next;
    while (new_node != &fake_node_) {
      BaseNode* tmp = new_node->next;
      static_cast<Node*>(new_node)->~Node();
      node_allocator_.deallocate(static_cast<Node*>(new_node), 1);
      new_node = tmp;
    }
    fake_node_.next = &fake_node_;
    fake_node_.prev = &fake_node_;
    list_size_ = 0;
  }

  List(const List& other, const Allocator& allocator)
      : node_allocator_(allocator) {
    try {
      for (BaseNode* new_node = other.fake_node_.next;
           new_node != &other.fake_node_; new_node = new_node->next) {
        insert_value_inside(end(), static_cast<Node*>(new_node)->value);
      }
    } catch (...) {
      clear();
      throw;
    }
  }

 public:
  List()
      : node_allocator_() {
  }

  List(const Allocator& allocator)
      : node_allocator_(allocator) {
  }

  List(size_t count, const T& value, const Allocator& allocator)
      : node_allocator_(allocator) {
    try {
      for (size_t i = 0; i < count; ++i) {
        insert_value_inside(end(), value);
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  List(size_t count, const Allocator& allocator)
      : node_allocator_(allocator) {
    try {
      for (size_t i = 0; i < count; ++i) {
        insert_novalue_inside(end());
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  List(size_t count)
      : List(count, Allocator()){};

  List(size_t count, const T& value)
      : List(count, value, Allocator()){};

  List(const List& other)
      : List(other,
             std::allocator_traits<node_alloc>::
                 select_on_container_copy_construction(other.node_allocator_)) {
  }

  ~List() {
    clear();
  }

  void swap(List& other) {
    std::swap(fake_node_, other.fake_node_);
    std::swap(node_allocator_, other.node_allocator_);
    std::swap(list_size_, other.list_size_);

    // отдельно перекладываем fake_node_:
    // первая и последняя ноды указывают на фэйк, после свапа они продолжат
    // указывать на фэйковую ноду старого листа, откуда мы их типо перетащили в
    // новый
    fake_node_.next->prev = fake_node_.prev->next = &fake_node_;
    other.fake_node_.next->prev = other.fake_node_.prev->next =
        &other.fake_node_;
  }

  List& operator=(const List& other) {
    if (this != &other) {
      List temp(other,
                std::allocator_traits<
                    node_alloc>::propagate_on_container_copy_assignment::value
                    ? other.node_allocator_
                    : node_allocator_);
      swap(temp);
    }
    return *this;
  }

  void push_back(const T& value) {
    insert_value_inside(end(), value);
  }

  void pop_back() {
    Node* tobe_deleted = static_cast<Node*>(fake_node_.prev);
    bind_old_node(fake_node_.prev->prev, &fake_node_);
    std::allocator_traits<node_alloc>::destroy(node_allocator_, tobe_deleted);
    node_allocator_.deallocate(tobe_deleted, 1);
    --list_size_;
  }

  void push_front(const T& value) {
    insert_value_inside(begin(), value);
  }

  void pop_front() {
    Node* tobe_deleted = static_cast<Node*>(fake_node_.next);
    bind_old_node(&fake_node_, fake_node_.next->next);
    std::allocator_traits<node_alloc>::destroy(node_allocator_, tobe_deleted);
    node_allocator_.deallocate(tobe_deleted, 1);
    --list_size_;
  }

  iterator insert(const_iterator index, const T& value) {
    return insert_value_inside_return(index, value);
  }

  iterator erase(const_iterator index) {
    BaseNode* tobe_deleted = index.get_curnode()->next;
    bind_old_node(index.get_curnode()->prev, index.get_curnode()->next);
    std::allocator_traits<node_alloc>::destroy(
        node_allocator_, static_cast<Node*>(index.get_curnode()));
    node_allocator_.deallocate(static_cast<Node*>(index.get_curnode()), 1);
    --list_size_;
    return tobe_deleted;
  }

  size_t size() const {
    return list_size_;
  }

  allocator_type get_allocator() const {
    return node_allocator_;
  }
};
