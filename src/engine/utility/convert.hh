// -*- mode: c++ -*-
#ifndef __IMP_CONVERT__63784703
#define __IMP_CONVERT__63784703

#include <boost/lexical_cast.hpp>
#include "types.hh"
#include "string_view.hh"

namespace imp {
  template <class T>
  T from_string(StringView str)
  {
    return boost::lexical_cast<T>(str);
  }

  template <class T, typename = std::enable_if_t<std::is_integral<T>::value>>
  constexpr size_t to_size(T i)
  {
      // This if-statement should hopefully be removed for unsigned integrals for -O3.
      if (i < 0)
          throw std::logic_error("Casting negative integer to size_t");
      return static_cast<size_t>(i);
  };
}

#endif //__IMP_CONVERT__63784703
