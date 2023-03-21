#pragma once
#include <cmath>
#include <cstring>
#include <iostream>

class String {
 private:
  void allocate_new_memory(size_t new_capacity);

  size_t size_;
  char* array_;
  size_t capacity_;

 public:
  void shrink_to_fit();

  String() : size_(0), array_(new char[1]), capacity_(0) { array_[0] = '\0'; }

  String(const char* cstyle);

  String(char letter) : String(1, letter) {}

  String(size_t n, char c);

  String(const String& string);

  ~String() { delete[] array_; }

  String& operator=(const String& string);

  void swap(String& string);

  char& operator[](size_t index) { return array_[index]; }

  const char& operator[](size_t index) const { return array_[index]; }

  size_t length() const { return size_; }

  size_t size() const { return size_; }

  size_t capacity() const { return capacity_; }

  char& front() { return array_[0]; }

  const char& front() const { return array_[0]; }

  char& back() { return array_[size_ - 1]; }

  const char& back() const { return array_[size_ - 1]; }

  bool empty() { return size_ == 0; }

  void clear();

  char* data() { return array_; }

  const char* data() const { return array_; }

  String substr(size_t start, size_t count) const;

  size_t find(const String& substring) const;

  size_t rfind(const String& substring) const;

  void push_back(char letter);

  void pop_back() { array_[--size_] = '\0'; }

  String& operator+=(char letter);

  String& operator+=(const String& string);

  bool operator==(const String& second_string);

  bool is_equal(const String& substring, size_t index) const;
};

void String::shrink_to_fit() {
  allocate_new_memory(size_);
}

void String::allocate_new_memory(size_t new_capacity) {
  char* new_string = new char[new_capacity + 1];
  memcpy(new_string, array_, size_);
  delete[] array_;
  capacity_ = new_capacity;
  array_ = new_string;
}

String& String::operator+=(const String& string) {
  allocate_new_memory(size_ + string.size_);
  memcpy(array_ + size_, string.array_, string.size_);
  size_ += string.size_;
  array_[size_] = '\0';
  return *this;
}

bool String::is_equal(const String& substring, size_t index) const {
  for (size_t i = 0; i < substring.length(); ++i) {
    if (array_[index + i] != substring[i]) {
      return false;
    }
  }
  return true;
}

bool String::operator==(const String& second) {
  if (size_ != second.length()) {
    return false;
  }
  for (size_t i = 0; i < size_; ++i) {
    if (array_[i] != second[i]) {
      return false;
    }
  }
  return true;
}

String& String::operator+=(char letter) {
  push_back(letter);
  return *this;
}

void String::swap(String& string) {
  std::swap(array_, string.array_);
  std::swap(size_, string.size_);
  std::swap(capacity_, string.capacity_);
}

void String::push_back(char letter) {
  if (size_ == capacity_) {
    allocate_new_memory(2 * capacity_);
  }
  array_[size_] = letter;
  array_[++size_] = '\0';
}

void String::clear() {
  size_ = 0;
  array_[0] = '\0';
}

String String::substr(size_t start, size_t count) const {
  String substring(count, '\0');
  memcpy(substring.array_, array_ + start, count);
  return substring;
}

String& String::operator=(const String& string) {
  if (this == &string) {
    return *this;
  }
  if (capacity_ <= string.size_) {
    String copy = string;
    swap(copy);
    return *this;
  }
  memcpy(array_, string.array_, size_ + 1);
  size_ = string.size_;
  return *this;
}

String::String(const String& string)
    : size_(string.size_),
      array_(new char[string.capacity_ + 1]),
      capacity_(string.capacity_) {
  memcpy(array_, string.array_, size_ + 1);
}

String::String(size_t n, char c)
    : size_(n), array_(new char[n + 1]), capacity_(n) {
  memset(array_, c, n);
  array_[n] = '\0';
}

String::String(const char* cstyle)
    : size_(strlen(cstyle)),
      array_(new char[size_ + 1]),
      capacity_(size_) {
  memcpy(array_, cstyle, size_ + 1);
}

size_t String::find(const String& substring) const {
  if (substring.size_ > size_) {
    return size_;
  }
  for (size_t i = 0; i <= size_ - substring.size_; ++i) {
    if (is_equal(substring, i)) {
      return i;
    }
  }
  return size_;
}

size_t String::rfind(const String& substring) const {
  if (substring.size_ > size_) {
    return size_;
  }
  for (size_t i = size_ - substring.size_ + 1; i >= 1; --i) {
    if (is_equal(substring, i - 1)) {
      return i - 1;
    }
  }
  return size_;
}

String operator+(String first_string, const String& second_string) {
  first_string += second_string;
  return first_string;
}

std::ostream& operator<<(std::ostream& os, const String& output_string) {
  os << output_string.data();
  return os;
}

std::istream& operator>>(std::istream& is, String& input_string) {
  input_string.clear();
  char letter;
  while (is.get(letter) && !is.eof() && !isspace(letter)) {
    input_string.push_back(letter);
  }
  return is;
}

bool operator==(const String& first_string, const String& second_string) {
  if (first_string.length() != second_string.length()) {
    return false;
  }
  for (size_t i = 0; i < first_string.length(); ++i) {
    if (first_string[i] != second_string[i]) {
      return false;
    }
  }
  return true;
}

bool operator!=(const String& first_string, const String& second_string) {
  return !(first_string == second_string);
}

bool operator<(const String& first_string, const String& second_string) {
  for (size_t i = 0; i < std::min(first_string.length(), second_string.length()); ++i) {
    if (first_string[i] < second_string[i]) {
      return true;
    }
  }
  if (first_string.length() != second_string.length()) {
    return first_string.length() < second_string.length();
  }
  return false;
}

bool operator>(const String& first_string, const String& second_string) {
  return second_string < first_string;
}

bool operator<=(const String& first_string, const String& second_string) {
  return !(first_string > second_string);
}

bool operator>=(const String& first_string, const String& second_string) {
  return !(first_string < second_string);
}
