// -*- mode: c++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2016 Zohar Malamant
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------

#ifndef __IMP_STRINGVIEW__523920528
#define __IMP_STRINGVIEW__523920528

#include <boost/utility/string_view.hpp>
#include <boost/functional/hash.hpp>

namespace imp {
  using StringView = ::boost::string_view;

  namespace string_view_literals {
    constexpr StringView operator ""_sv(const char *str, size_t len) noexcept
    { return { str, len }; }
  }
}

namespace boost {
  template <class C, class CT>
  inline size_t hash_value(const basic_string_view<C, CT>& val)
  { return hash_range(val.begin(), val.end()); }
}

#ifndef IMP_DONT_POLLUTE_GLOBAL_NAMESPACE
using namespace imp::string_view_literals;
#endif

#endif //__IMP_STRINGVIEW__523920528
