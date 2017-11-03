// -*- mode: c++ -*-
#ifndef __IMP_TYPES__14445451
#define __IMP_TYPES__14445451

#include <cstdint>
#include <type_traits>

namespace imp {
  // Integers
  using byte = std::uint8_t;

  using int8 = std::int8_t;
  using int16 = std::int16_t;
  using int32 = std::int32_t;
  using int64 = std::int64_t;

  using uint8 = std::uint8_t;
  using uint16 = std::uint16_t;
  using uint32 = std::uint32_t;
  using uint64 = std::uint64_t;

  inline namespace int_literals {
#define __IMP_INT_LITERAL(_Type, _Literal) \
    constexpr _Type operator"" _Literal(unsigned long long int x) \
    { return static_cast<_Type>(x); }

    __IMP_INT_LITERAL(int8, _i8)
    __IMP_INT_LITERAL(int16, _i16)
    __IMP_INT_LITERAL(int32, _i32)
    __IMP_INT_LITERAL(int64, _i64)

    __IMP_INT_LITERAL(uint8, _u8)
    __IMP_INT_LITERAL(uint16, _u16)
    __IMP_INT_LITERAL(uint32, _u32)
    __IMP_INT_LITERAL(uint64, _u64)

#undef __IMP_INT_LITERAL
  }

  // Standard library aliasing
  template<bool Val>
  using bool_type = std::integral_constant<bool, Val>;

  using nullptr_t = std::nullptr_t;

  using false_type = std::false_type;

  using true_type = std::true_type;

  template <bool V>
  using enable_if = std::enable_if_t<V>;

  // Tags
  struct noinit_tag {};

  // Misc
  namespace _detail {
    template<class... Args>
    struct is_any_same;

    /** When there are no more types to check -> false */
    template<class T>
    struct is_any_same<T>: false_type {};

    /** When the first two types are the same -> true */
    template<class T, class... Args>
    struct is_any_same<T, T, Args...> : true_type {};

    /** When the first two types are not the same -> recurse */
    template<class T, class U, class... Args>
    struct is_any_same<T, U, Args...> : is_any_same<T, Args...> {};
  }

  /** A metafunction to be able to detect the validity of an expression at compile time. See C++17's `std::void_t` */
  template<class... Args>
  using void_t = void;

  /** Analogue of std::is_same, but for N types */
  template<class... Args>
  constexpr auto is_any_same = _detail::is_any_same<Args...>::value;
}

#endif //__IMP_TYPES__14445451
