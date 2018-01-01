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
      const T *mData { nullptr };
      std::size_t mLength { 0 };

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

      ArrayView() = default;

      ArrayView(const ArrayView&) = default;

      ArrayView(ArrayView&&) = default;

      template <std::size_t N>
      constexpr ArrayView(const T (*arr)[N]) noexcept:
          mData(arr),
          mLength(N)
      {
      }

      constexpr ArrayView(const T arr[], size_type len) noexcept:
          mData(arr),
          mLength(len)
      {
      }

      template <std::size_t N>
      constexpr ArrayView(const std::array<T, N> &arr) noexcept:
          mData(arr.data()),
          mLength(arr.length())
      {
      }

      ArrayView(const std::vector<T> &vec) noexcept:
          mData(vec.data()),
          mLength(vec.size())
      {
      }

      ArrayView& operator=(const ArrayView&) = default;

      ArrayView& operator=(ArrayView&&) = default;

      constexpr const_pointer data() const noexcept
      {
          return mData;
      }

      constexpr size_type size() const noexcept
      {
          return mLength;
      }

      constexpr size_type length() const noexcept
      {
          return mLength;
      }

      constexpr bool empty() const noexcept
      {
          return mLength == 0;
      }

      constexpr const_reference front() const noexcept
      {
          return mData[0];
      }

      constexpr const_reference back() const noexcept
      {
          return mData[mLength - 1];
      }

      constexpr const_reference operator[](size_type idx) const noexcept
      {
          return mData[idx];
      }

      constexpr const_iterator begin() const
      {
          return mData;
      }

      constexpr const_iterator cbegin() const
      {
          return mData;
      }

      constexpr const_iterator end() const
      {
          return mData + mLength;
      }

      constexpr const_iterator cend() const
      {
          return mData + mLength;
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
  };
}

#endif //__IMP_ARRAYVIEW__30829833
