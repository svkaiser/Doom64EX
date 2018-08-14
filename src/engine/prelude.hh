// -*- mode: c++ -*-
#ifndef __IMP_PRELUDE__56107413
#define __IMP_PRELUDE__56107413

#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <cstdint>
#include <chrono>
#include <fmt/format.h>
#include <fmt/ostream.h>

// Include standard utilities
#include "utility/types.hh"
#include "utility/array_view.hh"
#include "utility/string_view.hh"
#include "utility/guard.hh"
#include "utility/optional.hh"
#include "imp/detail/Memory.hh"
#include "core/log.hh"


namespace imp {
  struct dummy_t {
      template <class T>
      operator T() const
      { return {}; }
  };

  struct UserData {
      virtual ~UserData() {}
  };

  /*! Pad an integer so it's divisible by N */
  template <size_t N>
  constexpr size_t pad(size_t x)
  { return x + ((N - (x & (N - 1))) & (N - 1)); }

  using String = std::string;
  template <class T, class Deleter = std::default_delete<T>>
  using Vector = std::vector<T>;
  template <class T>
  using InitList = std::initializer_list<T>;
  template <class T, size_t N>
  using Array = std::array<T, N>;

  inline std::ostream& operator<<(std::ostream& s, StringView sv)
  {
      return s.write(sv.data(), sv.length());
  }
}

template <class T, class Deleter>
inline std::ostream& operator<<(std::ostream& s, const std::unique_ptr<T, Deleter>& ptr)
{
    return s << static_cast<void*>(ptr.get());
}

template <class T>
inline std::ostream& operator<<(std::ostream& s, const std::shared_ptr<T>& ptr)
{
    return s << static_cast<void*>(ptr.get());
}

template <class T>
inline std::ostream& operator<<(std::ostream& s, const std::weak_ptr<T>& ptr)
{
    if (ptr.expired()) {
        return s << "(expired)";
    } else {
        auto shared = ptr.lock();
        return s << shared;
    }
}

#ifndef IMP_DONT_POLLUTE_GLOBAL_NAMESPACE
using namespace std::string_literals;
using namespace std::chrono_literals;
using namespace imp;
#endif

#endif //__IMP_PRELUDE__56107413
