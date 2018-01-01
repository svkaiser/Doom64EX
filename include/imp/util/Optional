// -*- mode: c++ -*-
#ifndef __IMP_OPTIONAL__85951861
#define __IMP_OPTIONAL__85951861

#include <boost/optional.hpp>

namespace imp {
  template <class T>
  using Optional = boost::optional<T>;

  using nullopt_t = boost::none_t;
  const nullopt_t nullopt { boost::none_t::init_tag {} };

  template <class T, class... Args>
  inline Optional<T> make_optional(Args&&... args)
  {
      return { T(std::forward<Args>(args)...) };
  }
}

#endif //__IMP_OPTIONAL__85951861
