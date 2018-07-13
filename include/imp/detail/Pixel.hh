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

#ifndef __IMP_PIXEL__13206666
#define __IMP_PIXEL__13206666

#include "prelude.hh"
#include "utility/ptr_iterator.hh"

/*
 * This header defines classes for low-level pixel operations,
 * as well as the Palette class.
 *
 * We distinguish pixel data into two groups: Color and Index.
 * Color pixels are pixels whose data is in-band. (ex: Rgb)
 * Index pixels are pixels whose color values can be found somewhere else, like a Palette. (ex: Index8)
 */

namespace imp {
  enum struct PixelFormat {
      none,
      index8,
      rgb,
      rgb565,
      rgba,
      rgba5551
  };

  namespace detail {
    template <int FromBits, int LShift, class = void>
    struct resize_component_shift {
        constexpr size_t operator()(size_t x) const
        { return x >> -LShift; }
    };

    template <int FromBits, int LShift>
    struct resize_component_shift<FromBits, LShift, std::enable_if_t<(LShift >= 0)>> {
        constexpr size_t operator()(size_t x) const
        { return (x << LShift) + resize_component_shift<FromBits, LShift - FromBits>()(x); }
    };

    template<int FromDepth, int ToDepth>
    constexpr size_t resize_component(size_t x)
    {
        return resize_component_shift<FromDepth, ToDepth - FromDepth>()(x);
    };

    /**
     * unit_type, get the smallest type that will fit n bytes
     */
    template <size_t NBytes>
    struct unit_type;

    template <>
    struct unit_type<1> { using type = uint8; };

    template <>
    struct unit_type<2> { using type = uint16; };

    template <>
    struct unit_type<3> { using type = uint32; };

    template <>
    struct unit_type<4> { using type = uint32; };
  }

#pragma pack(push, 1)

  template<PixelFormat Format, size_t Red, size_t Green, size_t Blue, size_t Alpha>
  struct Color {
      static constexpr auto format = Format;
      static constexpr size_t alpha_max = (1ULL << Alpha) - 1ULL;

      using unit = typename detail::unit_type<(Red + Green + Blue + Alpha + 7) / 8>::type;

      unit red   : Red;
      unit green : Green;
      unit blue  : Blue;
      unit alpha : Alpha;

      constexpr Color() = default;

      constexpr Color(const Color &) = default;

      constexpr Color(Color &&) = default;

      constexpr Color(size_t red, size_t green, size_t blue, size_t alpha):
          red(red),
          green(green),
          blue(blue),
          alpha(alpha)
      {}

      template<PixelFormat CFormat, size_t CRed, size_t CGreen, size_t CBlue, size_t CAlpha>
      constexpr Color(const Color<CFormat, CRed, CGreen, CBlue, CAlpha> &other):
          red(detail::resize_component<CRed, Red>(other.red)),
          green(detail::resize_component<CGreen, Green>(other.green)),
          blue(detail::resize_component<CBlue, Blue>(other.blue)),
          alpha(detail::resize_component<CAlpha, Alpha>(other.alpha))
      {}

      template<PixelFormat CFormat, size_t CRed, size_t CGreen, size_t CBlue>
      constexpr Color(const Color<CFormat, CRed, CGreen, CBlue, 0> &other):
          red(detail::resize_component<CRed, Red>(other.red)),
          green(detail::resize_component<CGreen, Green>(other.green)),
          blue(detail::resize_component<CBlue, Blue>(other.blue)),
          alpha(alpha_max)
      {}

      constexpr Color &operator=(const Color &) = default;

      constexpr Color &operator=(Color &&) = default;
  };

  template<PixelFormat Format, size_t Red, size_t Green, size_t Blue>
  struct Color<Format, Red, Green, Blue, 0> {
      static constexpr auto format = Format;

      using unit = typename detail::unit_type<(Red + Green + Blue + 7) / 8>::type;

      unit red   : Red;
      unit green : Green;
      unit blue  : Blue;

      constexpr Color() = default;

      constexpr Color(const Color &) = default;

      constexpr Color(Color &&) = default;

      constexpr Color(size_t red, size_t green, size_t blue):
          red(red),
          green(green),
          blue(blue)
      {}

      template<PixelFormat CFormat, size_t CRed, size_t CGreen, size_t CBlue, size_t CAlpha>
      constexpr Color(const Color<CFormat, CRed, CGreen, CBlue, CAlpha> &other):
          red(detail::resize_component<CRed, Red>(other.red)),
          green(detail::resize_component<CGreen, Green>(other.green)),
          blue(detail::resize_component<CBlue, Blue>(other.blue))
      {}

      constexpr Color &operator=(const Color &) = default;

      constexpr Color &operator=(Color &&) = default;
  };

  // RGB888 is a special case in that it's 3 bytes, but MSVC rounds it up to 4
  // because of bitmasks. So in this case, specify that it's exactly 3 uint8's.
  template <>
  struct Color<PixelFormat::rgb, 8, 8, 8, 0> {
      static constexpr auto format = PixelFormat::rgb;

      uint8 red {};
      uint8 green {};
      uint8 blue {};

      constexpr Color() = default;
      constexpr Color(const Color&) = default;
      constexpr Color(Color&&) = default;

      constexpr Color(size_t red, size_t green, size_t blue):
          red(red),
          green(green),
          blue(blue)
      {}

      template <PixelFormat CFormat, size_t CRed, size_t CGreen, size_t CBlue, size_t CAlpha>
      constexpr Color(const Color<CFormat, CRed, CGreen, CBlue, CAlpha>& other):
          red(detail::resize_component<CRed, 8>(other.red)),
          green(detail::resize_component<CGreen, 8>(other.green)),
          blue(detail::resize_component<CBlue, 8>(other.blue))
      {}

      constexpr Color& operator=(const Color&) = default;
      constexpr Color& operator=(Color&&) = default;
  };

  template<PixelFormat Format, size_t Depth>
  struct Index {
      static constexpr auto format = Format;

      uint8 index : Depth;

      constexpr Index() = default;

      constexpr Index(const Index &) = default;

      constexpr Index(Index &&) = default;

      constexpr Index(size_t index):
          index(index)
      {}

      constexpr Index &operator=(const Index &) = default;

      constexpr Index &operator=(Index &&) = default;
  };

#pragma pack(pop)

  template<PixelFormat Format, size_t Red, size_t Green, size_t Blue, size_t Alpha>
  std::ostream& operator<<(std::ostream& s, const Color<Format, Red, Green, Blue, Alpha> &c)
  { return s << "{ " << c.red << ", " << c.green << ", " << c.blue << ", " << c.alpha << " }"; }

  template<PixelFormat Format, size_t Red, size_t Green, size_t Blue>
  std::ostream& operator<<(std::ostream& s, const Color<Format, Red, Green, Blue, 0> &c)
  { return s << "{ " << c.red << ", " << c.green << ", " << c.blue << " }"; }

  using Index8 = Index<PixelFormat::index8, 8>;
  using Rgb = Color<PixelFormat::rgb, 8, 8, 8, 0>;
  using Rgb565 = Color<PixelFormat::rgb, 5, 6, 5, 0>;
  using Rgba = Color<PixelFormat::rgba, 8, 8, 8, 8>;
  using Rgba5551 = Color<PixelFormat::rgba5551, 5, 5, 5, 1>;

  static_assert(sizeof(Rgb) == 3, "Size of RGB struct must be 3 bytes");
  static_assert(sizeof(Rgb565) == 2, "Size of RGB565 struct must be 2 bytes");
  static_assert(sizeof(Rgba) == 4, "Size of RGBA struct must be 4 bytes");
  static_assert(sizeof(Rgba5551) == 2, "Size of RGBA5551 struct must be 2 bytes");

  namespace detail {
    template<class T>
    struct pixel_info {
        static constexpr auto format = PixelFormat::none;
        static constexpr bool is_color = false;
        static constexpr bool is_index = false;
        static constexpr bool is_pixel = false;
        static constexpr size_t depth = 0;
        static constexpr size_t width = 0;
    };

    template<PixelFormat F, size_t R, size_t G, size_t B, size_t A>
    struct pixel_info<Color<F, R, G, B, A>> {
        static constexpr auto format = F;
        static constexpr bool is_color = true;
        static constexpr bool is_index = false;
        static constexpr bool is_pixel = true;
        static constexpr size_t depth = R + G + B + A;
        static constexpr size_t width = (depth + 7) / 8;
    };

    template<PixelFormat F, size_t D>
    struct pixel_info<Index<F, D>> {
        static constexpr auto format = F;
        static constexpr bool is_color = false;
        static constexpr bool is_index = true;
        static constexpr bool is_pixel = true;
        static constexpr size_t depth = D;
        static constexpr size_t width = (depth + 7) / 8;
    };

    template <class T>
    constexpr auto is_color = pixel_info<std::decay_t<T>>::is_color;
    template <class T>
    constexpr auto is_index = pixel_info<std::decay_t<T>>::is_index;
    template <class T>
    constexpr auto is_pixel = pixel_info<std::decay_t<T>>::is_pixel;

    template <class T>
    constexpr auto is_color_it = is_color<std::iterator_traits<T>::value_type>;
    template <class T>
    constexpr auto is_index_it = is_index<std::iterator_traits<T>::value_type>;
    template <class T>
    constexpr auto is_pixel_it = is_pixel<std::iterator_traits<T>::value_type>;
  }

  /**
   * \brief A version of pixel_traits that's available at runtime
   *
   * When adding a new static constexpr field to pixel_traits, add it here
   * as well.
   */
  struct PixelInfo {
      PixelFormat format {};
      bool color {};
      bool index {};
      bool alpha {};

      size_t width {};

      constexpr PixelInfo() {}

      PixelInfo(PixelInfo&&) = default;

      PixelInfo(const PixelInfo&) = delete;

      template <class T>
      constexpr PixelInfo(const detail::pixel_info<T> &x):
          format(x.format),
          width(x.width) {}
  };

  const PixelInfo& get_pixel_info(PixelFormat format);

  template <class T>
  const PixelInfo& get_pixel_info()
  { return get_pixel_info(detail::pixel_info<T>::format); }

  template <class ColorFunc>
  auto match_color(PixelFormat type, ColorFunc func)
  {
      switch (type) {
      case PixelFormat::rgb:
          return func(Rgb{});

      case PixelFormat::rgb565:
          return func(Rgb565{});

      case PixelFormat::rgba:
          return func(Rgba{});

      case PixelFormat::rgba5551:
          return func(Rgba5551{});

      default:
          throw std::logic_error { "Couldn't match colour" };
      }
  }

  template <class ColorFunc, class IndexFunc>
  auto match_pixel(PixelFormat type, IndexFunc ifunc, ColorFunc cfunc)
  {
      switch (type) {
      case PixelFormat::none:
          throw std::logic_error { "Couldn't match pixel" };

      case PixelFormat::rgb:
          return cfunc(Rgb{});

      case PixelFormat::rgb565:
          return cfunc(Rgb565{});

      case PixelFormat::rgba:
          return cfunc(Rgba{});

      case PixelFormat::rgba5551:
          return cfunc(Rgba5551{});

      case PixelFormat::index8:
          return ifunc(Index8{});
      }
  }
}

#endif //__IMP_PIXEL__13206666
