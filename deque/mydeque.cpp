#include <vector>

#include <array>

#include <stdexcept>

#include <deque>

template <typename T>
class Deque {
private:

  static const size_t bucket_size = 32;
  mutable std::vector<T*> data_;
  size_t outer_front = 1;
  size_t outer_back = 0;
  size_t inner_front = 0;
  size_t inner_back = bucket_size - 1;
  static const size_t delta = 3; // константа для изменения размера массива указателей

public:

  Deque();
  ~Deque(); // правило трёх: если есть что-то из [деструктор, оператор копирования, оператор присваивания], то нужно явно определить все 3
  Deque(size_t);
  Deque(size_t, const T&);
  Deque(const Deque<T>& other);

  size_t size() const;

  void swap(Deque<T>& another);
  Deque<T>& operator=(const Deque<T>& other);

  T& operator[](size_t index);
  const T& operator[](size_t index) const;
  T& at(size_t index);
  const T& at(size_t index) const;

  void resize();
  void push_back(const T& element);
  void pop_back();
  void push_front(const T& element);
  void pop_front();


  // ------------------------------------- ИТЕРАТОРЫ ---------------------------------------------//

  template<typename Value>
  class base_iterator {
  private:
    T** buckets_;
    size_t item_index_;

    // для reverse iterator

  public:

    using iterator_category = std::random_access_iterator_tag;
    using value_type = Value;
    using difference_type = ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    base_iterator(T** array, size_t index) : buckets_(array), item_index_(index) {}
    base_iterator(const Deque<T>& deque, size_t index) : buckets_(deque.data_.data()), item_index_(index) {}

    operator base_iterator<const Value>() const {
      return base_iterator<const Value>(buckets_, item_index_);
    }

    reference operator*() const {
      return buckets_[item_index_ / bucket_size][item_index_ % bucket_size];
    }

    pointer operator->() const {
      return buckets_[item_index_ / bucket_size] + (item_index_ % bucket_size);
    }

    base_iterator& operator++() {
      ++item_index_;
      return *this;
    }

    base_iterator& operator--() {
      --item_index_;
      return *this;
    }

    base_iterator operator++(int) {
      base_iterator copy = *this;
      ++*this;
      return copy;
    }

    base_iterator operator--(int) {
      base_iterator copy = *this;
      --*this;
      return copy;
    }

    base_iterator& operator+=(difference_type num) {
      item_index_ += num;
      return *this;
    }

    base_iterator& operator-=(difference_type num) {
      item_index_ -= num;
      return *this;
    }

    base_iterator operator+(difference_type num) const {
      base_iterator copy = *this;
      copy += num;
      return copy;
    }

    base_iterator operator-(difference_type num) const {
      base_iterator copy = *this;
      copy -= num;
      return copy;
    }

    difference_type operator-=(const base_iterator& another) const {
      return item_index_ - another.item_index_;
    }

    difference_type operator-(const base_iterator& another) const {
      return *this -= another;
    }

    bool operator==(const base_iterator& another) const {
      return item_index_ == another.item_index_;
    }

    bool operator!=(const base_iterator& another) const {
      return item_index_ != another.item_index_;
    }

    auto operator<=>(const base_iterator& another) const {
      return item_index_ <=> another.item_index_;
    }

    [[nodiscard]]
    size_t get_index() const {
      return item_index_;
    }

  };

  using iterator = base_iterator<T>;
  using const_iterator = base_iterator<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() {
    return iterator(data_.data(), outer_front * bucket_size + inner_front);
  }

  iterator end() {
    return iterator(data_.data(), bucket_size * outer_back + inner_back + 1);
  }

  const_iterator begin() const {
    return const_iterator(data_.data(), outer_front * bucket_size + inner_front);
  }

  const_iterator end() const {
    return const_iterator(data_.data(), bucket_size * outer_back + inner_back + 1);
  }

  const_iterator cbegin() const {
    return const_iterator(data_.data(), outer_front * bucket_size + inner_front);
  }

  const_iterator cend() const {
    return const_iterator(data_.data(), bucket_size * outer_back + inner_back + 1);
  }

  reverse_iterator rbegin() {
    return reverse_iterator(end());
  }

  reverse_iterator rend() {
    return reverse_iterator(begin());
  }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(cend());
  }

  const_reverse_iterator crend() const {
    return const_reverse_iterator(cbegin());
  }

  void insert(iterator iter, const T&);
  void erase(iterator iter); // удалить из контейнера по итератору
};

template <typename T>
void Deque<T>::insert(Deque<T>::iterator iter, const T& element) {
  if (iter == end()) {
    push_back(element);
  } else if (iter == begin()) {
    push_front(element);
  } else {
    push_back(element); // добавляем фиктивный элемент в конец
    for (auto it = end(); it != iter; --it) {
      *it = *(it - 1); // перемещаем все элементы
    }
    // кладём на нужное место element, предварительно очищая это место
    (data_[iter.get_index() / bucket_size] + (iter.get_index() % bucket_size))->~T();
    new (data_[iter.get_index() / bucket_size] + (iter.get_index() % bucket_size)) T(element);
  }
}

template <typename T>
void Deque<T>::erase(Deque<T>::iterator iter) {
  if (iter == end()) {
    pop_back();
  } else if (iter == begin()) {
    pop_front();
  } else {
    for (auto it = iter; it != end() - 1; ++it) {
      *it = *(it + 1);
    }
    pop_back();
  }
}

// ------------------------------------- КОНСТРУКТОРЫ ------------------------------------- //

template<typename T>
Deque<T>::Deque() {
  data_.push_back(nullptr);
  data_.push_back(nullptr);
}

template<typename T>
Deque<T>::Deque(size_t count) : Deque() {
  size_t alpha;
  if (count % bucket_size == 0) {
    alpha = (count / bucket_size);
  } else {
    alpha = (count / bucket_size) + 1;
  }
  if (std::is_default_constructible_v<T>) { // если у T есть дефолтный конструктор
    for (size_t i = 0; i < count; ++i) {
      try {
        push_back(T());
      } catch (...) { // если исключение при push_back, удаляем все добавленные до исключения элементы
        for (int j = 0; j < i; ++j) {
          pop_back();
        }
        throw;
      }
    }
  } else { // если дефолт конструктора нет, просто выделим память
    for (int i = 0; i < alpha; ++i) {
      data_[i] = reinterpret_cast<T*> (new char[sizeof(T) * bucket_size]);
    }
  }
}

template<typename T>
Deque<T>::Deque(size_t count, const T& element) : Deque() {
  for (size_t i = 0; i < count; ++i) {
    push_back(element);
  }
}


template<typename T>
Deque<T>::Deque(const Deque<T>& another) {

  outer_front = another.outer_front;
  outer_back = another.outer_back;
  inner_front = another.inner_front;
  inner_back = another.inner_back;

  size_t outer = 0;
  size_t inner = 0;

  try {
    for (; outer < another.data_.size(); ++outer) {
      if (another.data_[outer] == nullptr) {
        data_.push_back(nullptr);
      } else {
        T* bucket = reinterpret_cast<T*>(new char[bucket_size * sizeof(T)]);
        if (outer == outer_front) {
          for (; inner < bucket_size - inner_front; ++inner) { // проверяем первый бакет
            new (bucket + inner_front + inner) T(another.data_[outer][inner]);
          }
        } else if (outer != outer_back) { // заполняем внутренние бакеты
          for (; inner < bucket_size; ++inner) {
            new (bucket + inner) T(another.data_[outer][inner]);
          }
        } else if (outer == outer_back) { // проверяем последний бакет
          for (; inner < inner_back; ++inner) {
            new (bucket + inner) T(another.data_[outer][inner]);
          }
        }
        data_.push_back(bucket);
      }
    }
  } catch (...) {
    for (size_t delete_outer = 0; delete_outer <= outer; ++delete_outer) { // del полностью все бакеты до того на к-ом исключение (его не трогаем)
      if (data_[delete_outer] == nullptr) {
        continue;
      }
      if (delete_outer < outer) {
        for (size_t del_inner = 0; del_inner < bucket_size; ++del_inner) {
          (data_[outer] + del_inner)->~T();
        }
      } else {
        for (size_t del_inner = 0; del_inner < inner; ++del_inner) { // обрабатываем бакет на котором получили исключение
          (data_[outer] + del_inner)->~T();
        }
      }
      delete[] reinterpret_cast<char*>(data_[delete_outer]);
    }
    throw;
  }
}

template<typename T>
Deque<T>::~Deque() {
  size_t outer = outer_front;
  size_t inner = 0;
  for (; outer <= outer_back; ++outer) {
    if (outer == outer_front) {
      for (; inner < bucket_size - inner_front; ++inner) {
        (data_[outer] + inner_front + inner)->~T();
      }
    }
    if (outer > outer_front && outer < outer_back) {
      for (; inner < bucket_size; ++inner) {
        (data_[outer] + inner)->~T();
      }
    }
    if (outer == outer_back) {
      for (; inner < inner_back; ++inner) {
        (data_[outer] + inner)->~T();
      }
    }
    delete[] reinterpret_cast<char*>(data_[outer]);
  }
}


// --------------------------------------------------------------------------------------------------------------------------------

template<typename T>
void Deque<T>::swap(Deque<T>& another) {
  std::swap(data_, another.data_);
  std::swap(outer_front, another.outer_front);
  std::swap(outer_back, another.outer_back);
  std::swap(inner_front, another.inner_front);
  std::swap(inner_back, another.inner_back);
}

template<typename T>
Deque<T>& Deque<T>::operator=(const Deque<T>& another) {
  if (this == &another) {
    return *this;
  }
  Deque<T> copy = another;
  swap(copy);
  return *this;
}

template<typename T>
size_t Deque<T>::size() const {
  return (outer_back - outer_front) * bucket_size + (inner_back - inner_front + 1);
}

template<typename T>
T& Deque<T>::operator[] (size_t index) {
  size_t outer_index = outer_front + ((inner_front + index) / bucket_size);
  size_t inner_index = (inner_front + index) % bucket_size;
  return data_[outer_index][inner_index];
}

template<typename T>
const T& Deque<T>::operator[] (size_t index) const {
  size_t outer_index = outer_front + ((inner_front + index) / bucket_size);
  size_t inner_index = (inner_front + index) % bucket_size;
  return data_[outer_index][inner_index];
}

template<typename T>
T& Deque<T>::at (size_t index) {
  size_t outer_index = outer_front + ((inner_front + index) / bucket_size);
  size_t inner_index = (inner_front + index) % bucket_size;
  if (index < 0 || (index >= size())) {
    throw std::out_of_range("Out of range exception");
  } else {
    return data_[outer_index][inner_index];
  }
}

template<typename T>
const T& Deque<T>::at (size_t index) const {
  size_t outer_index = outer_front + ((inner_front + index) / bucket_size);
  size_t inner_index = (inner_front + index) % bucket_size;
  if (index < 0 || (index >= size())) {
    throw std::out_of_range("Out of range exception");
  } else {
    return data_[outer_index][inner_index];
  }
}

// ------------------------------------------------- PUSH_BACK/FRONT, POP_BACK/FRONT ---------------------------------------------- //
template<typename T>
void Deque<T>::resize() {
  std::vector<T*> new_data_(data_.size() * delta);
  for (size_t i = 0; i < data_.size(); ++i) {
    new_data_[i + data_.size()] = data_[i];
  }
  outer_front += data_.size();
  outer_back += data_.size();
  data_ = new_data_;
}


template<typename T>
void Deque<T>::push_back(const T& element) {
  if (inner_back < (bucket_size - 1) ) { // если в последнем bucket есть место для нового элемента
    ++inner_back;
    try {
      new (data_[outer_back] + inner_back) T(element);
    } catch (...) {
      --inner_back;
      throw;
    }
  } else if (outer_back < (data_.size() - 1)) { // если последний bucket полностью заполнен и можно добавить еще бакет
    ++outer_back;
    data_[outer_back] = reinterpret_cast<T*>(new char[bucket_size * sizeof(T)]);
    try {
      new (data_[outer_back] + 0) T(element);
    } catch (...) {
      delete[] reinterpret_cast<char*>(data_[outer_back]);
      --outer_back;
      throw;
    }
    inner_back = 0;
  } else { // нужно расширять внешний массив указателей
    resize();
    if (inner_back < (bucket_size - 1) ) { // если в последнем bucket есть место для нового элемента
      ++inner_back;
      try {
        new (data_[outer_back] + inner_back) T(element);
      } catch (...) {
        for (int j = 0; j < inner_back; ++j) {
          (data_[outer_back] + j)->~T();
        }
        throw;
      }
    }
    if ((inner_back == (bucket_size - 1) ) && (outer_back < (data_.size() - 1) )) { // если последний bucket полностью заполнен и можно добавить еще бакет
      ++outer_back;
      data_[outer_back] = reinterpret_cast<T*>(new char[bucket_size * sizeof(T)]);
      try {
        new (data_[outer_back] + 0) T(element);
      } catch (...) {
        delete[] reinterpret_cast<char*>(data_[outer_back]);
        --outer_back;
        throw;
      }
      inner_back = 0;
    }
  }
}

template<typename T>
void Deque<T>::pop_back() {
  if (size() == 0) {
    return;
  }
  data_[outer_back][inner_back].~T();
  if (inner_back > 0) {
    --inner_back;
  } else {
    delete[] reinterpret_cast<char*>(data_[outer_back]);
    --outer_back;
    inner_back = bucket_size - 1;
  }
}


template<typename T>
void Deque<T>::push_front(const T& element) {
  if (inner_front > 0) { // в первом бакете есть места
    --inner_front;
    try {
      new (data_[outer_front] + inner_front) T(element);
    } catch (...) {
      ++inner_front;
      throw;
    }
  } else if (outer_front > 0) { // в первом бакете мест нет но можно сделать новый бакет
    --outer_front;
    data_[outer_front] = reinterpret_cast<T*>(new char[bucket_size * sizeof(T)]);
    try {
      new (data_[outer_front] + bucket_size - 1) T(element);
    } catch (...) {
      delete[] reinterpret_cast<char*>(data_[outer_front]);
      ++outer_front;
      throw;
    }
    inner_front = bucket_size - 1;
  } else { // нужно расширять массив
    resize();
    if (inner_front > 0) { // в первом бакете есть места
      try {
        new (data_[outer_front] + inner_front) T(element);
      } catch (...) {
        (data_[outer_front] + inner_front) -> ~T();
        throw;
      }
    } else if (outer_front > 0) { // в первом бакете мест нет но можно сделать новый бакет
      --outer_front;
      data_[outer_front] = reinterpret_cast<T*>(new char[bucket_size * sizeof(T)]);
      try {
        new (data_[outer_front] + bucket_size - 1) T(element);
      } catch (...) {
        delete[] reinterpret_cast<char*>(data_[outer_front]);
        ++outer_front;
        throw;
      }
      inner_front = bucket_size - 1;
    }
  }
}

template<typename T>
void Deque<T>::pop_front() {
  if (size() == 0) {
    return;
  }
  data_[outer_front][inner_front].~T();
  if (inner_front != bucket_size - 1) {
    ++inner_front;
  } else {
    delete[] reinterpret_cast<char*>(data_[outer_front]);
    ++outer_front;
    inner_front = 0;
  }
}
