#ifndef __KEXLIB_FRAMEWORK_PIXEL_HPP__
#define __KEXLIB_FRAMEWORK_PIXEL_HPP__

namespace pixel {
struct rgba {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t alpha;
};

struct rgb {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

struct indexed {
  uint8_t idx;
};

template <typename _Pixel>
struct PixelTraits {
    typedef _Pixel pixel_type;

    static constexpr BitsPerPixel = sizeof();
};

}

#endif //__KEXLIB_FRAMEWORK_PIXEL_HPP__
