// -*- mode: c++ -*-
#ifndef __IMP_ARRAYVIEW__30829833
#define __IMP_ARRAYVIEW__30829833

#include <type_traits>
#include <iterator>
#include <array>
#include <vector>

namespace imp {
  template <class T>
  class ArrayView
  {
      const T *data_ { nullptr };
      std::size_t length_ { 0 };

  public:
      using value_type = const T;
      using const_pointer = const T *;
      using pointer = const_pointer;
      using const_reference = const T &;
      using reference = const_reference;
      using const_iterator = const_pointer;
      using iterator = const_iterator;
      using const_reverse_iterator = std::reverse_iterator<const_iterator>;
      using reverse_iterator = const_reverse_iterator;
      using size_type = std::size_t;
      using difference_type = std::ptrdiff_t;

      using vector_type = std::vector<value_type>;

      ArrayView() = default;

      ArrayView(const ArrayView&) = default;

      ArrayView(ArrayView&&) = default;

      template <std::size_t N>
      constexpr ArrayView(const T (*arr)[N]) noexcept:
          data_(arr),
          length_(N)
      {
      }

      constexpr ArrayView(const T arr[], size_type len) noexcept:
          data_(arr),
          length_(len)
      {
      }

      template <std::size_t N>
      constexpr ArrayView(const std::array<T, N> &arr) noexcept:
          data_(arr.data()),
          length_(arr.length())
      {
      }

      ArrayView(const std::vector<T> &vec) noexcept:
          data_(vec.data()),
          length_(vec.size())
      {
      }

      ArrayView& operator=(const ArrayView&) = default;

      ArrayView& operator=(ArrayView&&) = default;

      constexpr const_pointer data() const noexcept
      {
          return data_;
      }

      constexpr size_type size() const noexcept
      {
          return length_;
      }

      constexpr size_type length() const noexcept
      {
          return length_;
      }

      constexpr bool empty() const noexcept
      {
          return length_ == 0;
      }

      constexpr const_reference front() const noexcept
      {
          return data_[0];
      }

      constexpr const_reference back() const noexcept
      {
          return data_[length_ - 1];
      }

      constexpr const_reference operator[](size_type idx) const noexcept
      {
          return data_[idx];
      }

      constexpr const_iterator begin() const
      {
          return data_;
      }

      constexpr const_iterator cbegin() const
      {
          return data_;
      }

      constexpr const_iterator end() const
      {
          return data_ + length_;
      }

      constexpr const_iterator cend() const
      {
          return data_ + length_;
      }

      constexpr const_reverse_iterator rbegin() const
      {
          return { cend() };
      }

      constexpr const_reverse_iterator crbegin() const
      {
          return { cend() };
      }

      constexpr const_reverse_iterator rend() const
      {
          return { cbegin() };
      }

      constexpr const_reverse_iterator crend() const
      {
          return { cbegin() };
      }

      vector_type to_vector() const
      { return { begin(), end() }; }
  };
}

#endif //__IMP_ARRAYVIEW__30829833
