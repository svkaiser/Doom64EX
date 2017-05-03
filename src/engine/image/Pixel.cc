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

#include <algorithm>
#include <imp/detail/Pixel.hh>

namespace {
  template <class... Args>
  constexpr auto infos_array_() -> Array<PixelInfo, sizeof...(Args)>
  {
      return {{ detail::pixel_info<Args> {}... }};
  }

  constexpr auto infos_ = infos_array_<void, Index8, Rgb, Rgba, Rgba5551>();
}

const PixelInfo& imp::get_pixel_info(PixelFormat format)
{
    return infos_[static_cast<size_t>(format)];
}
