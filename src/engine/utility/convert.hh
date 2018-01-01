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
}

#endif //__IMP_CONVERT__63784703
