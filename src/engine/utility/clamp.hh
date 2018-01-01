// -*- mode: c++ -*-
#ifndef __IMP_CLAMP__34482255
#define __IMP_CLAMP__34482255

#include "types.hh"

namespace imp {
  template <class T>
  constexpr T clamp(const T x, const T min, const T max)
  { return x > min ? (x < max ? x : max ) : min; }

  template <class T>
  class Clamp {
      T value_;
      T min_;
      T max_;

  public:
      constexpr Clamp(const Clamp&) = default;

      constexpr Clamp(T min, T max):
          value_(min),
          min_(min),
          max_(max) {}

      constexpr Clamp(T value, T min, T max):
          value_(clamp(value, min, max)),
          min_(min),
          max_(max) {}

      constexpr Clamp& operator=(const Clamp& other)
      {
          min_ = std::max(min_, other.min_);
          max_ = std::min(max_, other.max_);

          if (min_ > max_)
              std::swap(min_, max_);

          value_ = clamp(other.value_, min_, max_);
          return *this;
      }

      template <class U>
      constexpr Clamp& operator=(const U& other)
      {
          value_ = clamp(other, min_, max_);
          return *this;
      }

#define __IMP_CLAMP_ASSIGN_OPERATORS(_Op) \
      template <class U> constexpr \
      Clamp& operator _Op ## =(const Clamp<U>& other) \
      { value_ = clamp(value_ + other.value_, min_, max_); }  \
      template <class U> constexpr \
      Clamp& operator _Op ## =(const T& other) \
      { value_ = clamp(value_ + other, min_, max_); }

      __IMP_CLAMP_ASSIGN_OPERATORS(+)
      __IMP_CLAMP_ASSIGN_OPERATORS(-)
      __IMP_CLAMP_ASSIGN_OPERATORS(*)
      __IMP_CLAMP_ASSIGN_OPERATORS(/)
      __IMP_CLAMP_ASSIGN_OPERATORS(%)

#undef __IMP_CLAMP_ASSIGN_OPERATORS

      constexpr const T &value() const
      { return value_; }

      constexpr operator const T &() const
      { return value_; }
  };
}

#define __IMP_CLAMP_ARITHMETIC_OPERATORS(_Op) \
template <class T, class U> constexpr \
std::common_type_t<T, U> operator _Op(const imp::Clamp<T>& lhs, const imp::Clamp<U>& rhs) \
{ return lhs.value() _Op rhs.value(); } \
template <class T, class U> constexpr \
std::common_type_t<T, U> operator _Op(const imp::Clamp<T>& lhs, const U& rhs) \
{ return lhs.value() _Op rhs; } \
template <class T, class U> constexpr \
std::common_type_t<T, U> operator _Op(const T& lhs, const imp::Clamp<U>& rhs) \
{ return lhs _Op rhs.value(); }

__IMP_CLAMP_ARITHMETIC_OPERATORS(+)
__IMP_CLAMP_ARITHMETIC_OPERATORS(-)
__IMP_CLAMP_ARITHMETIC_OPERATORS(*)
__IMP_CLAMP_ARITHMETIC_OPERATORS(/)
__IMP_CLAMP_ARITHMETIC_OPERATORS(%)

#undef __IMP_CLAMP_ARITHMETIC_OPERATORS

#define __IMP_CLAMP_COMPARISON_OPERATORS(_Op) \
template <class T, class U> constexpr bool operator _Op(const imp::Clamp<T> &l, const imp::Clamp<U> &r) { return l.value() _Op static_cast<T>(r.value()); } \
template <class T, class U> constexpr bool operator _Op(const imp::Clamp<T> &l, const U &r) { return l.value() _Op static_cast<T>(r); } \
template <class T, class U> constexpr bool operator _Op(const T &l, const imp::Clamp<U> &r) { return l _Op static_cast<T>(r.value()); }

__IMP_CLAMP_COMPARISON_OPERATORS(==)
__IMP_CLAMP_COMPARISON_OPERATORS(!=)
__IMP_CLAMP_COMPARISON_OPERATORS(<)
__IMP_CLAMP_COMPARISON_OPERATORS(>)
__IMP_CLAMP_COMPARISON_OPERATORS(<=)
__IMP_CLAMP_COMPARISON_OPERATORS(>=)

#undef __IMP_CLAMP_COMPARISON_OPERATORS

#endif //__IMP_CLAMP__34482255
