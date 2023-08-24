#include <iostream>
#include <tuple>
#include <vector>
template <typename T, typename Allocator = std::allocator<T>>
class List {
 private:
  struct Node {
    T value;
    Node* prev;
    Node* next;
    Node(const T& value, Node* prev, Node* next)
        : value(value),
          prev(prev),
          next(next){};
    Node(T&& value, Node* prev, Node* next)
        : value(std::move(value)),
          prev(prev),
          next(next){};
    template <typename U, typename V>
    Node(U&& first, V&& second, Node* prev, Node* next)
        : value(std::move(first), std::move(second)),
          prev(prev),
          next(next) {
    }
  };

  typedef typename std::allocator_traits<Allocator>::template rebind_alloc<Node>
      node_alloc;
  Node* fake_node_;
  int list_size_ = 0;
  node_alloc node_allocator_;
  void clear_helper() {
    for (auto iter = begin(); iter != end(); ++iter) {
      std::allocator_traits<node_alloc>::destroy(node_allocator_,
                                                 iter.get_node());
      std::allocator_traits<node_alloc>::deallocate(node_allocator_,
                                                    iter.get_node(), 1);
    }
    list_size_ = 0;
  }

  void move_helper(List&& other) {
    fake_node_ = std::move(other.fake_node_);
    other.fake_node_ = nullptr;
    list_size_ = std::move(other.list_size_);
    other.list_size_ = 0;
    node_allocator_ = std::move(other.node_allocator_);
  }

  Node* create_node() {
    Node* new_node = static_cast<Node*>(
        std::allocator_traits<node_alloc>::allocate(node_allocator_, 1));
    new_node->prev = new_node->next = new_node;
    return new_node;
  }

  void bind_nodes(Node* left, Node* right) {
    left->next = right;
    right->prev = left;
  }

  template <typename OtherAllocator>
  List(const List<T, OtherAllocator>& other,
       const Allocator& alloc = Allocator())
      : List(alloc) {
    for (const auto& item : other) {
      insert_inside_impl(end(), item);
    }
  }

 public:
  Node* get_fake_node() const {
    return fake_node_;
  }

  template <bool const_>
  class BaseIterator {
   private:
    Node* current_node_;

   public:
    using value_type = T;
    using difference_type = ptrdiff_t;
    using reference = T&;
    using const_reference = T const&;
    using pointer = T*;
    using const_pointer = T const*;
    using iterator_category = std::bidirectional_iterator_tag;

    BaseIterator()
        : current_node_(nullptr){};
    BaseIterator(const Node* node)
        : current_node_(const_cast<Node*>(node)) {
    }
    BaseIterator(const BaseIterator<false>& other)
        : current_node_(other.get_node()){};
    BaseIterator<false>& operator=(const BaseIterator<false>& other) {
      if (this != &other) {
        current_node_ = other.get_node();
      }
      return *this;
    }

    Node* get_node() const {
      return current_node_;
    }

    bool operator==(const BaseIterator& other) const {
      return current_node_ == other.current_node_;
    }

    bool operator!=(const BaseIterator& other) const {
      return !(*this == other);
    }

    auto operator*() const
        -> std::conditional_t<const_, const_reference, reference> {
      return current_node_->value;
    }

    auto operator->() const
        -> std::conditional_t<const_, const_pointer, pointer> {
      return &current_node_->value;
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
  };

  using iterator = BaseIterator<false>;
  using const_iterator = BaseIterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() {
    return iterator(fake_node_->next);
  }
  iterator end() {
    return iterator(fake_node_);
  }
  const_iterator begin() const {
    return const_iterator(fake_node_->next);
  }
  const_iterator end() const {
    return const_iterator(fake_node_);
  }

  const_iterator cbegin() const {
    return const_iterator(fake_node_->next);
  }
  const_iterator cend() const {
    return const_iterator(fake_node_);
  }

  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(cend());
  }

  const_reverse_iterator crend() const {
    return const_reverse_iterator(cbegin());
  }

  reverse_iterator rbegin() {
    return reverse_iterator(end());
  }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(cend());
  }

  reverse_iterator rend() {
    return reverse_iterator(begin());
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(cbegin());
  }

  List(const Allocator& alloc = Allocator())
      : fake_node_(create_node()),
        node_allocator_(alloc) {
  }

  List(int count, const T& value, const Allocator& alloc = Allocator())
      : fake_node_(create_node()),
        node_allocator_(alloc) {
    try {
      for (int i = 0; i < count; ++i) {
        insert_inside_impl(begin(), value);
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  List(int count, const Allocator& alloc = Allocator())
      : fake_node_(create_node()),
        node_allocator_(alloc) {
    try {
      for (int i = 0; i < count; ++i) {
        insert_inside_impl(begin(), T(), alloc);
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  List(const List& another_list)
      : fake_node_(create_node()),
        node_allocator_(std::allocator_traits<Allocator>::
                            select_on_container_copy_construction(
                                another_list.node_allocator_)) {
    try {
      for (auto it = another_list.begin(); it != another_list.end(); ++it) {
        insert_inside_impl(end(), *it);
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  List(List&& another_list) {
    move_helper(std::move(another_list));
  }

  void swap(List& other) {
    std::swap(fake_node_, other.fake_node_);
    std::swap(node_allocator_, other.node_allocator_);
    std::swap(list_size_, other.list_size_);
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

  List& operator=(List&& another_list) {
    if (this == &another_list) {
      another_list.clear();
      return *this;
    }
    move_helper(std::move(another_list));
    return *this;
  }

  template <typename... Args>
  void insert_inside_impl(const_iterator index, Args&&... args) {
    auto previous = index.get_node()->prev;
    auto new_node =
        std::allocator_traits<node_alloc>::allocate(node_allocator_, 1);
    try {
      std::allocator_traits<node_alloc>::construct(node_allocator_, new_node,
                                                   std::forward<Args>(args)...,
                                                   previous, index.get_node());
    } catch (...) {
      std::allocator_traits<node_alloc>::deallocate(node_allocator_, new_node,
                                                    1);
    }
    bind_nodes(new_node, index.get_node());
    bind_nodes(previous, new_node);
    ++list_size_;
  }

  template <typename... Args>
  iterator insert(const_iterator index, Args&&... args) {
    insert_inside_impl(index, std::forward<Args>(args)...);
    return iterator(index.get_node()->prev->next);
  }

  template <typename... Args>
  void push_back(Args&&... args) {
    insert(end(), std::forward<Args>(args)...);
  }

  template <typename... Args>
  void push_front(Args&&... args) {
    insert(begin(), std::forward<Args>(args)...);
  }

  template <typename... Args>
  iterator emplace(const_iterator index, Args&&... args) {
    insert_inside_impl(index, std::forward<Args>(args)...);
    return iterator(index.get_node()->prev->next);
  }

  template <typename... Args>
  void emplace_back(Args&&... args) {
    emplace(end(), std::forward<Args>(args)...);
  }

  template <typename... Args>
  void emplace_front(Args&&... args) {
    emplace(begin(), std::forward<Args>(args)...);
  }

  iterator erase(const_iterator index) {
    auto tobe_deleted = index.get_node()->next;
    bind_nodes(index.get_node()->prev, tobe_deleted);
    std::allocator_traits<node_alloc>::destroy(node_allocator_,
                                               index.get_node());
    std::allocator_traits<node_alloc>::deallocate(node_allocator_,
                                                  index.get_node(), 1);
    --list_size_;
    return iterator(tobe_deleted);
  }

  void clear() {
    clear_helper();
    fake_node_->prev = fake_node_->next = fake_node_;
  }

  void pop_back() {
    erase(get_fake_node()->prev);
  }

  void pop_front() {
    erase(begin());
  }

  ~List() {
    if (!fake_node_)
      return;
    clear_helper();
    std::allocator_traits<node_alloc>::deallocate(node_allocator_, fake_node_,
                                                  1);
  }

  Allocator get_allocator() {
    return node_allocator_;
  }
  int size() const {
    return list_size_;
  }
};

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>,
          typename Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
 public:
  typedef std::pair<const Key, Value> UnoMapNode;
  typedef typename List<UnoMapNode, Alloc>::iterator iterator;
  typedef typename List<UnoMapNode, Alloc>::const_iterator const_iterator;

 private:
  List<UnoMapNode, Alloc> elements;
  std::vector<List<iterator, Alloc>> hash_table;
  static const size_t magic_size_of_table;
  size_t size_of_table = magic_size_of_table;
  size_t amount_of_elements = 0;
  constexpr static const double maximum_load_factor = 0.75;
  [[no_unique_address]] Hash hash_function = Hash();
  Equal comparator = Equal();
  Alloc UnoMapAllocator = Alloc();

  void swap(UnorderedMap& other) noexcept {
    using std::swap;
    swap(hash_function, other.hash_function);
    swap(comparator, other.comparator);
    swap(UnoMapAllocator, other.UnoMapAllocator);
    elements.swap(other.elements);
    hash_table.swap(other.hash_table);
    swap(size_of_table, other.size_of_table);
    swap(amount_of_elements, other.amount_of_elements);
  }

  void copyFrom(const UnorderedMap& another) {
    UnorderedMap tmp(another.hash_function, another.comparator,
                     another.UnoMapAllocator);

    tmp.elements = another.elements;

    swap(tmp);
  }

 public:
  iterator begin() {
    return elements.begin();
  }
  iterator end() {
    return elements.end();
  }
  const_iterator cbegin() {
    return elements.cbegin();
  }
  const_iterator cend() {
    return elements.cend();
  }

  UnorderedMap(const Hash& hash_function = Hash(),
               const Equal& comparator = Equal(),
               const Alloc& UnoMapAllocator = Alloc())
      : hash_function(hash_function),
        comparator(comparator),
        UnoMapAllocator(UnoMapAllocator) {
    hash_table.resize(size_of_table);
  }

  UnorderedMap(const UnorderedMap& another) {
    copyFrom(another);
  }

  UnorderedMap(UnorderedMap&& other) = default;

  ~UnorderedMap() = default;

  UnorderedMap& operator=(const UnorderedMap& other) {
    copyFrom(other);
    return *this;
  }

  UnorderedMap& operator=(UnorderedMap&& other) = default;

  iterator find(const Key& key) {
    size_t hash = hash_function(key) % size_of_table;
    for (auto& bucket : hash_table[hash]) {
      if (comparator((*bucket).first, key))
        return bucket;
    }
    return elements.end();
  }

  const Value& operator[](const Key& key) const {
    iterator it = emplace(key, Value()).first;
    return it->second;
  }

  Value& operator[](const Key& key) {
    iterator it = emplace(key, Value()).first;
    return it->second;
  }

  Value& operator[](Key&& key) {
    iterator it = emplace(std::forward<Key>(key), Value()).first;
    return it->second;
  }

  template <typename K = Key>
  typename std::conditional<std::is_const<K>::value, const Value&, Value&>::type
  at(Key key) {
    auto it = find(key);
    if (it == elements.end()) {
      throw std::out_of_range("This key is not in map!");
    }
    return it->second;
  }

  template <typename K = Key>
  typename std::conditional<std::is_const<K>::value, const Value&, Value&>::type
  at(Key key) const {
    auto it = find(key);
    if (it == elements.end()) {
      throw std::out_of_range("This key is not in map!");
    }
    return it->second;
  }

  void swap(UnorderedMap&& other) {
    if (std::allocator_traits<
            Alloc>::propagate_on_container_copy_assignment::value) {
      UnoMapAllocator = other.UnoMapAllocator;
    }
    hash_function = std::move(other.hash_function);
    hash_table = std::move(other.hash_table);
    comparator = std::move(other.comparator);
    elements = std::move(other.elements);
    size_of_table = other.size_of_table;
    amount_of_elements = other.amount_of_elements;
    maximum_load_factor = other.maximum_load_factor;
  }

  void erase(const_iterator it) {
    size_t hash = hash_function(it->first) % size_of_table;
    for (auto& x : hash_table[hash]) {
      if (comparator(it->first, x->first)) {
        x = *hash_table[hash].rend();
        hash_table[hash].pop_back();
        break;
      }
    }
    --amount_of_elements;
    elements.erase(it);
  }

  void erase(const_iterator fst, const_iterator snd) {
    for (auto it = fst; it != snd; it++) {
      erase(it);
    }
  }

  void reserve(size_t count) {
    hash_table = std::move(hash_table);
    hash_table.resize(count);
    for (iterator iter = elements.begin(); iter != elements.end(); ++iter) {
      hash_table[hash_function(iter->first) % size_of_table].push_back(iter);
    }
    size_of_table = count;
  }

  template <class... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    auto mover = std::allocator_traits<Alloc>::allocate(UnoMapAllocator, 1);
    try {
      std::allocator_traits<Alloc>::construct(UnoMapAllocator, mover,
                                              std::forward<Args>(args)...);
    } catch (...) {
      std::allocator_traits<Alloc>::deallocate(UnoMapAllocator, mover, 1);
    }
    iterator result = find(mover->first);
    if (result != elements.end()) {
      std::allocator_traits<Alloc>::destroy(UnoMapAllocator, mover);
      std::allocator_traits<Alloc>::deallocate(UnoMapAllocator, mover, 1);
      return std::make_pair(result, false);
    }
    return insert(std::move(*mover));
  }

  std::pair<iterator, bool> insert(UnoMapNode&& node) {
    auto it = find(node.first);
    if (it != elements.end()) {
      return std::make_pair(it, false);
    }
    size_t hash = hash_function(node.first) % size_of_table;
    ++amount_of_elements;
    if (load_factor() >= max_load_factor()) {
      reserve(2 * size_of_table);
      hash = hash_function(node.first) % size_of_table;
    }

    elements.insert_inside_impl(elements.begin(),
                                std::move(const_cast<Key&>(node.first)),
                                std::move(node.second));
    bool insert_second_noexcept = false;
    try {
      hash_table[hash].insert_inside_impl(hash_table[hash].end(),
                                          elements.begin());
      insert_second_noexcept = true;
    } catch (...) {
      elements.erase(elements.begin());
      --amount_of_elements;
      throw;
    }
    return std::make_pair(elements.begin(), true);
  }

  std::pair<iterator, bool> insert(const UnoMapNode& node) {
    return insert(
        std::move(UnoMapNode(const_cast<Key&>(node.first), node.second)));
  }

  template <typename InputIterator>
  void insert(InputIterator begin, InputIterator end) {
    auto it = begin;
    for (; it != end; ++it) {
      try {
        insert(*it);
      } catch (...) {
        auto start = begin;
        while (start != it) {
          if (insert(*start).second) {
            erase(find((*start).first));
          }
          ++start;
        }
        throw;
      }
    }
  }

  double load_factor() const {
    return static_cast<double>(amount_of_elements / size_of_table);
  }

  size_t size() {
    return amount_of_elements;
  }

  double max_load_factor() const {
    return maximum_load_factor;
  }
};

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Alloc>
const size_t UnorderedMap<Key, Value, Hash, Equal, Alloc>::magic_size_of_table =
    1334;
